#ifndef DM_DEBUG
#define DM_DEBUG

// debug
#include <stdio.h>

void dmlogopen(char *location, char* mode);
void dmlogclose();
void dmlog(char *msg);
void dmlogf(char *format, ...);

#endif
