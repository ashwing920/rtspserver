#ifndef __VDRV_ENC_I_H__
#define __VDRV_ENC_I_H__
#include "type.h"

#define OVERLAY_PALLETE_BASE_SIZE  1024         /* 中间件申请给调色板使用的空间大小*/
#define OVERLAY_PIXELS_BUF_BASE_SIZE  50*1024   /* 中间件申请给overlay数据使用的空间大小*/

#define MAX_OVERLAY_BLOCK_NUM 40
#define OVERLAY_PALETTE_SIZE  64
#define OVERLAY_DATA_BUF_SIZE  0x40000          /*256k*/
#define OVERLAY_MAX_SRC_GROUP_NUM 20            /* overlay最多包含多少组资源 */

//add 
#define OVERLAY_MAX_SRC_INGROUP 13
#define OVERLAY_MAX_LIST_LEN 20
#define EPDK_YES 0


typedef struct pos_t
{
	unsigned int x;
	unsigned int y;
}pos_t;

typedef struct __rectsz_t
{
	unsigned int width;
	unsigned int height;
}__rectsz_t;



typedef struct overlay_pic_info
{
    __u8                ID;                 //src id
    __rectsz_t          size;               //for the size of one picture
    __u8*               pOverlayPixel;           //the index of the RGB data
}overlay_pic_info_t;

typedef struct overlay_src_init
{
    __u8                srcPicNum;                 //src id
    overlay_pic_info_t  srcPic[OVERLAY_MAX_SRC_INGROUP];
    __u32               ppalette_yuv[16];              //the palette of yuv format
    __u8                color_num;                 //the color number of the palette
}overlay_src_init_t;


typedef struct dis_par
{
    pos_t   pos;               //the position of the log
    __u8     total;
    __u8     IDlist[OVERLAY_MAX_LIST_LEN];    //the index of the display of the picture
}overlay_dis_par_t;
  /* _MOD_HERB_GINGKO_H_ */



typedef struct overlay_block_header
{
    __u32    Next_BlkHdr_Addr;
    __u32    hor_plane;
    __u32    ver_plane;
    __u32    Mb_Pal_Idx;
}overlay_block_header_t;


typedef struct overlay_info
{
    __u8                    isEnabled;                              /* 当前有多少block */
    __u32                   nblock;                                 /* 当前有多少block */
    __u32                   nsrc;                                   /* 当前有多少组资源 */
    overlay_src_init_t      srcPicGroup[OVERLAY_MAX_SRC_GROUP_NUM]; /* 资源信息 */
    overlay_dis_par_t       dispInfo[MAX_OVERLAY_BLOCK_NUM];        /* 显示信息 */
    __u8                    dispGroupIndex[MAX_OVERLAY_BLOCK_NUM];  /* 显示图片在资源中的index */

    overlay_block_header_t* pblkHdrTbl;                             /*block header表*/
    __u32                   blkHdrTblSize;
    __u8*                   ppalette;                               /*设置给寄存器的调色板信息地址*/
    __u32                   paletteBufSize;
    __u8*                   pdata;                                  /*设置给寄存器的数据地址*/
    __u32                   dataBufSize;
}overlay_info_t;



#endif

