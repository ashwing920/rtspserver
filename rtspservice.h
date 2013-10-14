/*
 * rtspservice.h
 * 肖军
 * 2011.07.12创建
 */
#ifndef _RTSP_H
#define _RTSP_H
#include "rtsputils.h"

#define RTSP_DEBUG 1
void CallBackNotifyRtspExit(char s8IsExit);
void *ThreadRtsp(void *pArgs);
int rtsp_server(RTSP_buffer *rtsp);
#endif /* _RTSP_H */
