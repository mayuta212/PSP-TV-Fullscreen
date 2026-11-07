/* Host-side state-flow test. NOT a PSP hardware test.
 * Compile with a minimal PSPSDK interface shim; tests main.c unchanged. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include "../main.c"

enum Scenario { INTERLACE, PROGRESSIVE, UNKNOWN };
static enum Scenario scenario;
static unsigned int fake_now;
static int fake_aspect, selection_count, refresh_count, selected_preset;

static void putreg(unsigned int off, unsigned int value) {
    *(volatile unsigned int *)(unsigned long)(LCDC_BASE+off) = value;
}

int sceDisplayGetMode(int *m,int *w,int *h) {
    if (scenario==INTERLACE && fake_now<800000) {
        *m=0xD1;*w=720;*h=503;
    } else if (scenario==INTERLACE) {
        *m=0x6D1;*w=480;*h=272;
    } else if (scenario==PROGRESSIVE) {
        *m=0x2D2;*w=480;*h=272;
    } else {
        *m=0x13;*w=512;*h=512;
    }
    return 0;
}
int sceDisplayGetFrameBuf(void **fb,int *stride,int *format,int sync) {
    (void)sync;
    *fb=(void *)(unsigned long)0x04000000;
    *stride=512; *format=3;
    return 0;
}
int sceRegOpenRegistry(struct RegParam *p,int mode,REGHANDLE *h) {(void)p;(void)mode;*h=1;return 0;}
int sceRegOpenCategory(REGHANDLE h,const char *name,int mode,REGHANDLE *c) {(void)h;(void)name;(void)mode;*c=2;return 0;}
int sceRegGetKeyValueByName(REGHANDLE c,const char *name,void *dst,unsigned int len) {
    (void)c;(void)name;(void)len;
    if(fake_aspect<0) return -1;
    *(unsigned int*)dst=(unsigned int)fake_aspect;return 0;
}
int sceRegCloseCategory(REGHANDLE c){(void)c;return 0;}
int sceRegCloseRegistry(REGHANDLE h){(void)h;return 0;}
SceUID sceIoOpen(const char *name,int flags,int mode){(void)name;(void)flags;(void)mode;return -1;}
int sceIoWrite(SceUID id,const void *buf,SceSize size){(void)id;(void)buf;return (int)size;}
int sceIoClose(SceUID id){(void)id;return 0;}
SceUID sceKernelCreateThread(const char *n,int(*p)(SceSize,void*),int pri,int stack,int opts,void *param) {
    (void)n;(void)p;(void)pri;(void)stack;(void)opts;(void)param;return 1;
}
int sceKernelStartThread(SceUID id,SceSize args,void *params){(void)id;(void)args;(void)params;return 0;}
int sceKernelDeleteThread(SceUID id){(void)id;return 0;}
int sceKernelWaitThreadEnd(SceUID id,void *p){(void)id;(void)p;return 0;}
unsigned int sceKernelGetSystemTimeLow(void){return fake_now;}
static void apply_native_geometry(void) {
    putreg(0x180,1); putreg(0x184,480);putreg(0x188,272);
    putreg(0x148,698);putreg(0x14c,selected_preset==5?480:360);
    putreg(0x144,11);putreg(0x140,selected_preset==5?0:(scenario==INTERLACE?59:60));
}
int sceKernelDelayThread(unsigned int us){
    unsigned int prev=fake_now;
    fake_now+=us;
    if (scenario==INTERLACE && prev<800000 && fake_now>=800000) {
        putreg(0x104,1);putreg(0x11c,720);putreg(0x12c,479);putreg(0x1A0,7);
        if(selection_count) apply_native_geometry();
        else {putreg(0x180,0);putreg(0x148,480);putreg(0x14c,272);
              putreg(0x144,120);putreg(0x140,103);}
    }
    return 0;
}
int sceLcdc_driver_451FE1A1(int preset){
    selected_preset=preset;selection_count++;
    /* Selection alone leaves GAME registers unchanged until refresh. */
    return 0;
}
int sceLcdc_driver_E9DBD35F(int mode,int w,int h,int format){
    (void)format;
    if(mode!=1 || w!=480 || h!=272 || scenario!=PROGRESSIVE) abort();
    refresh_count++; apply_native_geometry();
    return 0;
}

#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL scenario=%d aspect=%d: %s at %d\n",scenario,fake_aspect,#c,__LINE__);exit(1);}}while(0)
static void test(enum Scenario sc,int asp){
    memset((void *)(unsigned long)LCDC_BASE,0,0x2000);
    scenario=sc;fake_aspect=asp;fake_now=0;
    selection_count=0;refresh_count=0;selected_preset=0;
    g_running=1;g_refreshed=0;g_early_interlace=0;g_aspect=-1;g_preset=0;
    g_select_return=-1;g_refresh_return=-1;g_hw_format=-1;g_row_count=0;
    putreg(0x148,480);putreg(0x14c,272);putreg(0x184,480);putreg(0x188,272);
    if(sc==INTERLACE){putreg(0x11c,720);putreg(0x12c,503);putreg(0x104,0);}
    if(sc==PROGRESSIVE){putreg(0x11c,720);putreg(0x12c,480);putreg(0x104,1);putreg(0x144,120);putreg(0x140,104);}
    worker(0,0);
    if(sc==UNKNOWN||asp<0){CHECK(selection_count==0);CHECK(refresh_count==0);}
    else {
        CHECK(selection_count==1);
        CHECK(selected_preset==(asp==0?5:1));
        CHECK(reg32(0x148)==698);
        CHECK(reg32(0x14c)==(asp==0?480:360));
        CHECK(reg32(0x144)==11);
        CHECK(refresh_count==(sc==PROGRESSIVE?1:0));
        if(sc==INTERLACE){CHECK(reg32(0x12c)==479);CHECK(reg32(0x1A0)==7);}
        if(sc==PROGRESSIVE){CHECK(reg32(0x12c)==480);}
    }
    printf("PASS %-11s aspect=%d select=%d refresh=%d geometry=%ux%u x=%u y=%u\n",
           sc==INTERLACE?"480i":sc==PROGRESSIVE?"480p":"unknown",asp,
           selection_count,refresh_count,reg32(0x148),reg32(0x14c),reg32(0x144),reg32(0x140));
}
int main(void) {
    void *p=mmap((void *)(unsigned long)LCDC_BASE,0x2000,
                 PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(p==MAP_FAILED){perror("mmap");return 2;}
    CHECK(p==(void *)(unsigned long)LCDC_BASE);
    test(INTERLACE,0);test(INTERLACE,1);
    test(PROGRESSIVE,0);test(PROGRESSIVE,1);
    test(UNKNOWN,0);test(INTERLACE,-1);
    return 0;
}
