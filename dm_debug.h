#ifndef DM_DEBUG
#define DM_DEBUG

// debug
#include <stdio.h>

void dmlogopen(char *location, char* mode);
void dmlogclose();
void dmlog(char *msg);
void dmlogi(char *msg, int i);
void dmlogii(char *msg, int i, int j);
void dmlogiii(char *msg, int i, int j, int k);
void dmlogf(char *msg, double f);
void dmlogff(char *msg, double f1, double f2);
void dmlogc(char *msg, char c);
void dmlogxy(char *msg, int x, int y);

#endif
