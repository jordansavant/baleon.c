#ifndef DM_DEBUG
#define DM_DEBUG

// debug
#include <stdio.h>
#include <stdarg.h>

FILE* dm_fp;
void dmlogopen(char *location, char *mode)
{
	dm_fp = fopen(location, mode);
}
void dmlogclose()
{
	fclose(dm_fp);
}
void dmlog(char *msg)
{
	fprintf(dm_fp, "%s\n", msg);
	fflush(dm_fp);
}
void dmlogf(char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	vfprintf(dm_fp, format, argptr);
	fprintf(dm_fp, "\n");
	va_end(argptr);
	fflush(dm_fp);
}

#endif
