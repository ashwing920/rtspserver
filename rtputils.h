/*
 * rtputils.h
 * 王冠
 * 2011.06.20创建
 * v0.1
 */
#ifndef _RTPUTILS_H
#define _RTPUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RTP_PKT_LENGTH     1400

#define H264                    96
#define G711					97

typedef enum
{
	_h264		= 0x100,
	_h264nalu,
	_mjpeg,
	_g711		= 0x200,
}EmRtpPayload;

unsigned int RtpCreate(unsigned int u32IP, int s32Port, EmRtpPayload emPayload);
void RtpDelete(unsigned int u32Rtp);
unsigned int RtpSend(unsigned int u32Rtp, char *pData, int s32DataSize, unsigned int u32TimeStamp);

#ifdef __cplusplus
}
#endif

#endif /* _RTPUTILS_H */
