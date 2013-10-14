#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static char log_buf[256];

void applog(const char *format, ...)
{
	char buf[64];
	va_list args;
	va_start(args, format);
	vsprintf(buf, format, args);
	strcat(log_buf,buf);
	va_end(args);
}
void applog_flush()
{
	printf("\r\x1b[K%s", log_buf);
	fflush(stdout);
	log_buf[0] = '\0';
}