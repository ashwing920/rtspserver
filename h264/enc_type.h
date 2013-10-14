#ifndef _ENC_TYPE_
#define _ENC_TYPE_

#include <pthread.h>


extern int cedarx_hardware_init(int mode);
extern int cedarx_hardware_exit(int mode);
extern void *cedar_sys_phymalloc_map(unsigned int size, int align);
extern void cedar_sys_phyfree_map(void *buf);
extern unsigned int cedarv_address_vir2phy(void *addr);
extern long long avs_counter_get_time_ms();
extern int cedarv_wait_ve_ready();
extern long long gettimeofday_curr();
extern int JpegEnc(void * pBufOut, int * bufSize, int addrY, int addrC, int width, int height, int pixelFmt);



#define VE_hardware_Init(mode)  cedarx_hardware_init(mode)
#define VE_hardware_Exit(mode)  cedarx_hardware_exit(mode)
#define PHY_MALLOC(size,align)	cedar_sys_phymalloc_map((size), (align))
#define PHY_FREE(pbuf)          cedar_sys_phyfree_map((void *)(pbuf))
#define VirAddr2PhyAddr(x)		cedarv_address_vir2phy((void *)(x))
//#define VE_Get_time_ms(...)		avs_counter_get_time_ms()
#define VE_set_frequence(x)     cedarv_set_ve_freq(x);


#define ENC_FIFO_LEVEL 4
#define MAX_DISP_ELEMENTS   32



#if 1


#define LOGV(...)   ((void)0)
#define LOGD(...)   ((void)0)
#define LOGI(...)   ((void)0)
#define LOGW(...)   ((void)0)
#define LOGE(...)   ((void)0)
#define LOGH
#define LOGS


#else

#define LOGV(...) ((void)printf("V/" LOG_TAG ": "));		 \
		((void)printf("(%d) ",__LINE__));	   \
		((void)printf(__VA_ARGS__));		  \
		((void)printf("\n"))

#define LOGD(...) ((void)printf("D/" LOG_TAG ": "));		 \
		((void)printf("(%d) ",__LINE__));	   \
		((void)printf(__VA_ARGS__));		  \
		((void)printf("\n"))

#define LOGW(...) ((void)printf("W/" LOG_TAG ": "));		 \
		((void)printf("(%d) ",__LINE__));	   \
		((void)printf(__VA_ARGS__));		  \
		((void)printf("\n"))

#define LOGE(...) ((void)printf("E/" LOG_TAG ": "));		 \
		((void)printf("(%d) ",__LINE__));	   \
		((void)printf(__VA_ARGS__));		  \
		((void)printf("\n"))
#endif





typedef struct disp_element_t
{
	unsigned int curr_disp_frame_id;
	unsigned int dec_frame_id;
}disp_element_t;

typedef struct disp_queue_t
{
	disp_element_t disp_elements[MAX_DISP_ELEMENTS];
	int wr_idx;
	int rd_idx;
}disp_queue_t;



#endif

