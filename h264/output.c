#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "picture_t.h"
#include "matroska_ebml.h"
#include "simplerecorder.h"

#define OUTPUT_FILENAME "output.mkv"
#define FPS 25
static int width, height;
mk_writer *writer;
static int cache_size=0;
static uint8_t *cache=NULL;
static int writing_frame = 0;

//======================================================
char * base64_encode( const unsigned char * bindata, char * base64, int binlength );
//===================================================

static uint8_t *findDelimiter(uint8_t **_p, uint8_t *end)
{
	int cnt_0 = 0;
	uint8_t *ret = 0, *p = *_p;
	while(p < end){
		if(!*p){
			if(!ret)
				ret=p;
			cnt_0++;
		}else{
			if(*p==1 && cnt_0>=2){
				(*_p)++;
				return ret;
			}
			cnt_0 = 0;
			ret = 0;
		}
		p++;
		(*_p)++;
	}
	return NULL;
}
static int write_frame_data(uint8_t *ptr, int size)
{
	if(size+4>cache_size){
		cache_size=size+4;
		cache = realloc(cache, cache_size);
		if(!cache)
			return 0;
	}
	 *cache = (size>>24);
	 *(cache+1) = (size>>16)&255;
	 *(cache+2) = (size>>8)&255;
	 *(cache+3) = (size)&255;
	memcpy(cache+4, ptr, size);
	if(mk_add_frame_data(writer, cache, size+4) < 0)
		return 0;
	return 1;
}
int output_init(struct picture_t *info, const char *str)
{	
	if((writer=mk_create_writer(str)) == 0){
		fprintf(stderr,"mk_create_writer failed:%s\n",str);
		return 0;
	}
	width = info->width;
	height = info->height;
	return 1;
}
int output_write_headers(struct encoded_pic_t *headers,struct profileid_sps_pps *psp)
{
	uint8_t *avcC; // AVCDecoderConfigurationRecord
	int avcC_len;

	uint8_t *tmp, *last = NULL;
	uint8_t *sps, *pps, *sei;
	int sps_len=0, pps_len=0, sei_len=0;

	int ret;

	uint64_t frame_duration = 1000000000ull/FPS;

	char *base64profileid;
	char *base64sps;
	char *base64pps;

	for(tmp = headers->buffer;;){
		uint8_t *st = findDelimiter(&tmp, headers->buffer + headers->length);
		int len = st ? st-last : headers->buffer + headers->length - last;
		if(last){
			switch(*last&31){
			case 6:
				sei = last;
				sei_len = len;
				break;
			case 7:
				sps = last;
				sps_len = len;
				break;
			case 8:
				pps = last;
				pps_len = len;
				break;
			}
		}
		last = tmp;
		if(!st)
			break;
	}
	// printf("sps @%x,%d,%x\n",(int)(sps),sps_len,(int)(*sps));
	// printf("pps @%x,%d,%x\n",(int)(pps),pps_len,(int)(*pps));
	// printf("sei @%x,%d,%x\n",(int)(sei),sei_len,(int)(*(sei)));

	avcC_len = 5 + 1 + 2 + sps_len + 1 + 2 + pps_len;
	avcC = malloc( avcC_len );
	if( !avcC )
		return 0;

	avcC[0] = 1;
	avcC[1] = sps[1];
	avcC[2] = sps[2];
	avcC[3] = sps[3];
	avcC[4] = 0xff; // nalu size length is four bytes
	avcC[5] = 0xe1; // one sps

	avcC[6] = sps_len >> 8;
	avcC[7] = sps_len;

	memcpy( avcC+8, sps, sps_len );

	avcC[8+sps_len] = 1; // one pps
	avcC[9+sps_len] = pps_len >> 8;
	avcC[10+sps_len] = pps_len;

	memcpy( avcC+11+sps_len, pps, pps_len );

	//==============add====================
	sprintf(psp->base64profileid,"%x%x%x",sps[1],sps[2],sps[3]);
	//psp->base64profileid=base64profileid;
	base64_encode( (unsigned char *)sps, psp->base64sps, sps_len );
	//psp->base64sps=base64sps;
	base64_encode( (unsigned char *)pps, psp->base64pps, pps_len );
	//psp->base64pps=base64pps;
	//========================================

	ret = mk_write_header( writer, "simplerecorder", "V_MPEG4/ISO/AVC",
						   avcC, avcC_len, frame_duration, 1000000,
						   width, height, width, height, DS_PIXELS);

	free( avcC );

	if(ret < 0)
		return 0;

	if(sei_len){
		if(mk_start_frame(writer) < 0)
			return 0;

		writing_frame = 1;

		if(!write_frame_data(sei, sei_len))
			return 0;
	}

	return 1;
}
int output_write_frame(struct encoded_pic_t *encoded)
{
	uint8_t *ptr = encoded->buffer;
	int length = encoded->length;
	if(length == 0)
		return 1;
	while(length>0 && *ptr == 0){
		ptr++;
		length--;
	}
	if(!length || *ptr != 0x01){
		fprintf(stderr, "invalid NAL delimiter\n");
		return 0;
	}

	if(!writing_frame)
		if(mk_start_frame(writer) < 0)
			return 0;
	if(!write_frame_data(ptr+1, length-1))
		return 0;
	if(mk_set_frame_flags(writer, encoded->timepoint*1000 /*us -> ns*/, 
		encoded->frame_type==FRAME_TYPE_I, encoded->frame_type==FRAME_TYPE_B)<0)
		return 0;
	writing_frame = 0;

	return 1;
}
void output_close()
{
	mk_close(writer, 0);
	//free(cache);
}

void output_print(const char *str)
{
		printf("%s:cache:%x,cache_size:%d\n",str,(int)(cache),cache_size);
}

char * base64_encode( const unsigned char * bindata, char * base64, int binlength )
{
    int i, j;
    unsigned char current;
	char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}
