#ifndef H_IO
#define H_IO
#include <pspkernel.h>
#define PSP_O_WRONLY 1
#define PSP_O_CREAT 2
#define PSP_O_TRUNC 4
SceUID sceIoOpen(const char*,int,int);
int sceIoWrite(SceUID,const void*,SceSize);
int sceIoClose(SceUID);
#endif
