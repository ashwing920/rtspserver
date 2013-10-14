#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "picture_t.h"
#include "log.h"

#include "type.h"
#include "H264encLibApi.h"
#include "enc_type.h"

#define DEFAULT_FPS 25

static VENC_DEVICE *g_pCedarV;
static __vbv_data_ctrl_info_t g_outputDataInfo;
static int64_t start_timestamp;
static int64_t cur_pts;
static void * virAddr;
static uint32_t phyAddrY, phyAddrCb;
static int Ysize, Csize;

static int32_t internal_WaitFinishCB(int32_t uParam1, void *pMsg)
{
	return cedarv_wait_ve_ready();
}

static int32_t internal_GetFrmBufCB(int32_t uParam1,  void *pFrmBufInfo)
{
	VEnc_FrmBuf_Info encBuf;

	memset((void*)&encBuf, 0, sizeof(VEnc_FrmBuf_Info));
	
	encBuf.addrY = (uint8_t *) phyAddrY;
	encBuf.addrCb = (uint8_t *) phyAddrCb;
	encBuf.pts_valid = 1;
	encBuf.pts = cur_pts;

	encBuf.color_fmt = PIXEL_YUV420;
	encBuf.color_space = BT601;

	memcpy(pFrmBufInfo, (void*)&encBuf, sizeof(VEnc_FrmBuf_Info));
	
	return 0;
}

int encoder_init(struct picture_t *info)
{
	int ret;
	__video_encode_format_t enc_fmt;
	VENC_DEVICE *pCedarV = NULL;

	start_timestamp = -1;
	Ysize = info->width * info->height;
	Csize = Ysize/2;

	ret = VE_hardware_Init(0);
	if (ret < 0) {
		fprintf(stderr, "cedarx_hardware_init failed\n");
		return 0;
	}
	
	ret = -1;
	pCedarV = H264EncInit(&ret);
	if (ret < 0) {
		fprintf(stderr, "H264EncInit failed\n");
		return 0;
	}

	enc_fmt.src_width = info->width;
	enc_fmt.src_height = info->height;
	enc_fmt.width = info->width;
	enc_fmt.height = info->height;
	enc_fmt.frame_rate = 1;
	enc_fmt.color_format = PIXEL_YUV420;
	enc_fmt.color_space = BT601;
	enc_fmt.qp_max = 40;
	enc_fmt.qp_min = 20;
	enc_fmt.avg_bit_rate = 1024*1024;
	enc_fmt.maxKeyInterval = 8;
	
    //enc_fmt.profileIdc = 77; /* main profile */

	enc_fmt.profileIdc = 66; /* baseline profile */
	enc_fmt.levelIdc = 31;

	pCedarV->IoCtrl(pCedarV, VENC_SET_ENC_INFO_CMD, (__u32) &enc_fmt);
	 
	ret = pCedarV->open(pCedarV);
	if (ret < 0)
	{
		fprintf(stderr, "open H264Enc failed\n");
		return 0;
	}
	
	pCedarV->GetFrmBufCB = internal_GetFrmBufCB;
	pCedarV->WaitFinishCB = internal_WaitFinishCB;

	VE_set_frequence(320);

	g_pCedarV = pCedarV;

	virAddr = cedar_sys_phymalloc_map(Ysize+Csize, 1024);
	if(!virAddr){
		fprintf(stderr, "cedar_sys_phymalloc_map failed\n");
		return 0;
	}

	phyAddrY = cedarv_address_vir2phy(virAddr);
	if(!phyAddrY){
		fprintf(stderr, "cedarv_address_vir2phy failed\n");
		return 0;
	}
	phyAddrY |= 0x40000000;
	phyAddrCb = phyAddrY + Ysize;

	return 1;
}
static int do_encode()
{
	int ret;

	g_pCedarV->IoCtrl(g_pCedarV, VENC_LIB_CMD_SET_MD_PARA , 0);
	ret = g_pCedarV->encode(g_pCedarV);
	if(ret < 0)
		return 0;

	memset(&g_outputDataInfo, 0 , sizeof(__vbv_data_ctrl_info_t));
	ret = g_pCedarV->GetBitStreamInfo(g_pCedarV, &g_outputDataInfo);
	if(ret < 0)
		return 0;

	// fprintf(stderr, " debug: priLen=%d, data0Len=%d, data1Len=%d\n", 
	// 	g_outputDataInfo.privateDataLen,
	// 	g_outputDataInfo.uSize0,
	// 	g_outputDataInfo.uSize1);

	return 1;
}
int encoder_encode_headers(struct encoded_pic_t *headers_out)
{
	if(!do_encode())
		return 0;

	if(g_outputDataInfo.privateDataLen <= 0){
		return 0;
	}

	headers_out->length = g_outputDataInfo.privateDataLen;
	headers_out->buffer = g_outputDataInfo.privateData;
	return 1;
}
static void I420toNV12(unsigned char *pNV12, const unsigned char *pI420, int C_Size)
{
	int halfC = C_Size/2;
	const unsigned char *pCb = pI420;
	const unsigned char *pCr = pI420 + halfC;
	int j;
	for(j=0; j<halfC; j++){
		*pNV12 = *pCb;
		pNV12++;
		*pNV12 = *pCr;
		pNV12++;

		pCr++;
		pCb++;
	}
}
int encoder_encode_frame(struct picture_t *raw_pic, struct encoded_pic_t *output)
{
	int64_t pts = raw_pic->timestamp.tv_usec + ((int64_t)raw_pic->timestamp.tv_sec) * 1000000;

	memcpy(virAddr, raw_pic->buffer, Ysize);
	I420toNV12(virAddr+Ysize, raw_pic->buffer+Ysize, Csize);

	if(start_timestamp == -1)
		start_timestamp = pts;
	cur_pts = pts - start_timestamp;
	
	if(!do_encode())
		return 0;

	output->length = g_outputDataInfo.uSize0;
	output->buffer = g_outputDataInfo.pData0;
	output->timepoint = cur_pts;
	output->frame_type = g_outputDataInfo.keyFrameFlag ? FRAME_TYPE_I : FRAME_TYPE_P;
	return 1;
}
void ResetTime(struct picture_t *raw_pic,struct encoded_pic_t *output)
{
	int64_t pts = raw_pic->timestamp.tv_usec + ((int64_t)raw_pic->timestamp.tv_sec) * 1000000;
	start_timestamp = pts;
	cur_pts = pts - start_timestamp;
	output->timepoint = cur_pts;
}
void encoder_release(struct encoded_pic_t *output)
{
	g_pCedarV->ReleaseBitStreamInfo(g_pCedarV, g_outputDataInfo.idx);
}
void encoder_close()
{
	if (virAddr)
	{
		cedar_sys_phyfree_map(virAddr);
		virAddr = NULL;
	}
	if (g_pCedarV)
	{
		g_pCedarV->close(g_pCedarV);
		H264EncExit(g_pCedarV);
		g_pCedarV = NULL;
	}

	VE_hardware_Exit(0);

}
