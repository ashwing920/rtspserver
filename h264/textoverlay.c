#include "font_data.h"
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include "picture_t.h"

void osd_print(struct picture_t *pic, const char *str)
{
	int i;
	for(i=0; i<16; i++){
		int j = 0;
		const char *p = str;
		while(*p){
			uint8_t tmp = ASCII[*p-32][i];
			int k;
			for(k=0; k<8; k++){
				uint8_t *px = &(pic->buffer[i*pic->width + j]);
				*px = (tmp & 1) ? 255 : *px*3ul/4;
				tmp >>= 1;
				j++;
			}
			p++;
		}
	}
}