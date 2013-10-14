#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <time.h>
#include "picture_t.h"
#include "simplerecorder.h"

#define FB_NAME  "/dev/fb0"
static int fd_scr;
static int scr_width, scr_height, scr_bpp;
static struct fb_var_screeninfo vinfo;
static unsigned char *scr_buf, *bgr24_buf;

/******************from libv4lconvert*******************/
#define YUV2R(y, u, v) ({ \
		int r = (y) + ((((v) - 128) * 1436) >> 10); r > 255 ? 255 : r < 0 ? 0 : r; })
#define YUV2G(y, u, v) ({ \
		int g = (y) - ((((u) - 128) * 352 + ((v) - 128) * 731) >> 10); g > 255 ? 255 : g < 0 ? 0 : g; })
#define YUV2B(y, u, v) ({ \
		int b = (y) + ((((u) - 128) * 1814) >> 10); b > 255 ? 255 : b < 0 ? 0 : b; })

#define CLIP(color) (unsigned char)(((color) > 0xFF) ? 0xff : (((color) < 0) ? 0 : (color)))
void v4lconvert_yuv420_to_bgr24__(const unsigned char *src, unsigned char *dest,
		int width, int height, int yvu)
{
	int i, j;

	const unsigned char *ysrc = src;
	const unsigned char *usrc, *vsrc;

	if (yvu) {
		vsrc = src + width * height;
		usrc = vsrc + (width * height) / 4;
	} else {
		usrc = src + width * height;
		vsrc = usrc + (width * height) / 4;
	}

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j += 2) {
#if 1 /* fast slightly less accurate multiplication free code */
			int u1 = (((*usrc - 128) << 7) +  (*usrc - 128)) >> 6;
			int rg = (((*usrc - 128) << 1) +  (*usrc - 128) +
					((*vsrc - 128) << 2) + ((*vsrc - 128) << 1)) >> 3;
			int v1 = (((*vsrc - 128) << 1) +  (*vsrc - 128)) >> 1;

			*dest++ = CLIP(*ysrc + u1);
			*dest++ = CLIP(*ysrc - rg);
			*dest++ = CLIP(*ysrc + v1);
			ysrc++;

			*dest++ = CLIP(*ysrc + u1);
			*dest++ = CLIP(*ysrc - rg);
			*dest++ = CLIP(*ysrc + v1);
#else
			*dest++ = YUV2B(*ysrc, *usrc, *vsrc);
			*dest++ = YUV2G(*ysrc, *usrc, *vsrc);
			*dest++ = YUV2R(*ysrc, *usrc, *vsrc);
			ysrc++;

			*dest++ = YUV2B(*ysrc, *usrc, *vsrc);
			*dest++ = YUV2G(*ysrc, *usrc, *vsrc);
			*dest++ = YUV2R(*ysrc, *usrc, *vsrc);
#endif
			ysrc++;
			usrc++;
			vsrc++;
		}
		/* Rewind u and v for next line */
		if (!(i & 1)) {
			usrc -= width / 2;
			vsrc -= width / 2;
		}
	}
}
/*******************************************************/

static int min(int x, int y)
{
	return x<y ? x : y;
}
int preview_init(struct picture_t *info)
{
	// if(geteuid() != 0){
	// 	fprintf(stderr, "must be run as root\n");
	// 	return 0;
	// }

	fd_scr = open(FB_NAME, O_RDWR);
	if(fd_scr < 0){
		perror("open framebuffer "FB_NAME);
		return 0;
	}
	if(ioctl(fd_scr, FBIOGET_VSCREENINFO, &vinfo) < 0) {
		perror("FBIOGET_VSCREENINFO");
		goto error;
	}

	if(ioctl(fd_scr, FBIOBLANK, FB_BLANK_UNBLANK) < 0) {
		perror("FBIOBLANK");
		// goto error;
	}
	system("echo 0 >/sys/class/graphics/fbcon/cursor_blink");

	scr_bpp = vinfo.bits_per_pixel;
	scr_width = vinfo.xres;
	scr_height = vinfo.yres;

	printf("framebuffer: width=%d, height=%d, bpp=%d\n", scr_width, scr_height, scr_bpp);
	if(scr_bpp!=32 && scr_bpp!=24){
		fprintf(stderr, "%s\n", "scr_bpp!=32 && scr_bpp!=24, not supported");
		goto error;
	}

	scr_buf = mmap(0, scr_width*scr_height*scr_bpp/8, PROT_READ|PROT_WRITE, MAP_SHARED, fd_scr, 0);
	if(scr_buf <= 0) {
		perror("mmap framebuffer");
		goto error;
	}

	bgr24_buf = malloc(info->width*info->height*3);
	if(!bgr24_buf){
		perror("malloc");
		goto error_malloc;
	}
	return 1;

error_malloc:
	munmap(scr_buf, scr_width*scr_height*scr_bpp/8);
error:
	close(fd_scr);
	return 0;
}
int preview_display(struct picture_t *pic)
{
	int min_width = min(pic->width, scr_width);
	int min_height = min(pic->height, scr_height);

	v4lconvert_yuv420_to_bgr24__(pic->buffer, bgr24_buf, pic->width, pic->height, 0);

	if(scr_bpp == 32){
		int i,j;
		for(i=0; i<min_height; i++){
			unsigned char *ptr_d, *ptr_s;

			ptr_d = scr_buf+4*scr_width*i;
			ptr_s = bgr24_buf+3*pic->width*i;

			for(j=0; j<min_width; j++){
				//B,G,R,A
				*ptr_d++ = *ptr_s++;
				*ptr_d++ = *ptr_s++;
				*ptr_d++ = *ptr_s++;
				*ptr_d++ = 0;
			}
		}
	}else if(scr_bpp == 24){
		int i,j;
		for(i=0; i<min_height; i++){
			unsigned char *ptr_d, *ptr_s;

			ptr_d = scr_buf+3*scr_width*i;
			ptr_s = bgr24_buf+3*pic->width*i;

			for(j=0; j<min_width; j++){
				*ptr_d++ = *ptr_s++;
				*ptr_d++ = *ptr_s++;
				*ptr_d++ = *ptr_s++;
			}
		}
	}

	return 1;
}
void preview_close()
{
	free(bgr24_buf);
	munmap(scr_buf, scr_width*scr_height*scr_bpp/8);
	close(fd_scr);
}
