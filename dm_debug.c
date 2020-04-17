#ifndef DM_DEBUG
#define DM_DEBUG

// debug
#include <stdio.h>

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
void dmlogi(char *msg, int i)
{
	fprintf(dm_fp, "%s %d \n", msg, i);
	fflush(dm_fp);
}
void dmlogf(char *msg, double f)
{
	fprintf(dm_fp, "%s %f \n", msg, f);
	fflush(dm_fp);
}
void dmlogii(char *msg, int i, int j)
{
	fprintf(dm_fp, "%s %d %d \n", msg, i, j);
	fflush(dm_fp);
}
void dmlogiii(char *msg, int i, int j, int k)
{
	fprintf(dm_fp, "%s %d %d %d \n", msg, i, j, k);
	fflush(dm_fp);
}
void dmlogc(char *msg, char c)
{
	fprintf(dm_fp, "%s %c \n", msg, c);
	fflush(dm_fp);
}
void dmlogxy(char *msg, int x, int y)
{
	fprintf(dm_fp, "%s x %d y %d \n", msg, x, y);
	fflush(dm_fp);
}

#endif
