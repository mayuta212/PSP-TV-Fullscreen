#ifndef H_PSPKERNEL
#define H_PSPKERNEL
#define PSP_MODULE_INFO(a,b,c,d)
typedef int SceUID;
typedef unsigned long SceSize;
SceUID sceKernelCreateThread(const char*,int(*)(SceSize,void*),int,int,int,void*);
int sceKernelStartThread(SceUID,SceSize,void*);
int sceKernelDeleteThread(SceUID);
int sceKernelWaitThreadEnd(SceUID,void*);
unsigned int sceKernelGetSystemTimeLow(void);
int sceKernelDelayThread(unsigned int);
#endif
