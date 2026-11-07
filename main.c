/* PSP TV Fullscreen v1.0.0-rc7: 480i + 480p full-width freeze candidate
 * PSP-3000 (04g), FW 6.61, GAME-only.
 * Progressive: 16:9 -> preset 5 (698x480); 4:3 -> preset 1 (698x360).
 * Interlaced: 16:9 -> preset 5 (698x480); 4:3 -> preset 1 (698x360).
 * Progressive NTSC 0x2D2/LCDC mode1 retains the confirmed rc1 path.
 * For 480i, select the native full-width preset in XMB 0xD1, BEFORE the
 * normal Sony GAME transition to 0x6D1. Never force a 480i LCDC refresh.
 * The previously observed split was traced to the GBS MA/BOB state.
 * Restores Sony native full-width geometry; GBS input must run in BOB mode.
 * Other TV modes are READ-ONLY; unknown layouts are never modified.
 * Temporary diagnostic CSV: ms0:/PSPTVFullscreen_probe.csv
 */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <pspreg.h>

PSP_MODULE_INFO("PSPTVFullscreen", 0x1000, 1, 0);
extern int sceLcdc_driver_451FE1A1(int preset);
extern int sceLcdc_driver_E9DBD35F(int lcdc_mode, int width, int height, int hw_format);

#define LCDC_BASE 0xBE140000u
#define POLL_US 100000u
#define STABLE_US 450000u
#define REFRESH_DELAY_US 400000u
#define STARTUP_TIMEOUT_US 12000000u
#define AFTER_REFRESH_US 1800000u
#define PROBE_PATH "ms0:/PSPTVFullscreen_probe.csv"
#define ROWS_MAX 96u

/* 480i: the only write is the native preset selector, while the system
 * still reports XMB 0xD1/720x503. Allow Sony to configure LCDC normally
 * during its subsequent 0x6D1/720x479 GAME transition.
 * The rc4 direct mode17 write is deliberately DISABLED for 480i. */
enum DisplayKind {
    OTHER=0, LCD=1, AV_P_BASE=2, AV_P_SCALED=3,
    AV_I_BASE=4, AV_I_SCALED=5
};
enum EventKind { SAMPLE=0, SELECTED=1, REFRESHED=2, FINISHED=3, FAILED=4 };

typedef struct {
    unsigned int ms, event, kind, mode, width, height;
    unsigned int syncdiff, xres, yres, shiftx, shifty, realx, realy;
    unsigned int enabled, inputx, inputy, aspect, preset;
    unsigned int select_return, refresh_return, hw_format;
    unsigned int lcdc_flags, coef_h_half, coef_v_half, coef_h_step, coef_v_step;
} DiagRow;
static DiagRow g_rows[ROWS_MAX];
static unsigned int g_row_count=0;
static volatile int g_running=1;
static SceUID g_thread=-1;
static int g_aspect=-1,g_preset=0,g_select_return=-1,g_refresh_return=-1,g_hw_format=-1;
static int g_refreshed=0;
static int g_early_interlace=0;

static unsigned int reg32(unsigned int off) {
    return *(volatile const unsigned int *)(unsigned long)(LCDC_BASE+off);
}

static int read_tv_type(void) {
    static const char root[]="/system";
    struct RegParam p={0};
    REGHANDLE reg=0,cat=0;
    unsigned int aspect=0xFFFFFFFFu, i;
    int ret;
    p.regtype=1;
    p.namelen=sizeof(root)-1u;
    p.unk2=1; p.unk3=1;
    for(i=0;i<sizeof(root);i++) p.name[i]=root[i];
    ret=sceRegOpenRegistry(&p,1,&reg);
    if(ret<0) return -1;
    ret=sceRegOpenCategory(reg,"/CONFIG/DISPLAY",1,&cat);
    if(ret<0){sceRegCloseRegistry(reg);return -1;}
    ret=sceRegGetKeyValueByName(cat,"aspect_ratio",&aspect,4u);
    sceRegCloseCategory(cat);
    sceRegCloseRegistry(reg);
    if(ret<0 || aspect>1u) return -1;
    return (int)aspect;
}

/* Outputs diagnostic base state even when classification is OTHER. */
static enum DisplayKind get_kind(int *mode_out,int *w_out,int *h_out) {
    int mode=-1,w=0,h=0;
    unsigned int xres,yres,syncdiff,enabled,realx,realy,inx,iny;
    if(sceDisplayGetMode(&mode,&w,&h)<0) {
        *mode_out=-1;*w_out=0;*h_out=0;
        return OTHER;
    }
    *mode_out=mode; *w_out=w; *h_out=h;
    if(w!=480 || h!=272) return OTHER;
    xres=reg32(0x11Cu);yres=reg32(0x12Cu);
    if(mode==0 && xres==480u && yres==272u) return LCD;
    syncdiff=reg32(0x104u);
    if(syncdiff!=1u || xres!=720u) return OTHER;
    enabled=reg32(0x180u);
    realx=reg32(0x148u);realy=reg32(0x14Cu);
    inx=reg32(0x184u);iny=reg32(0x188u);

    /* Confirmed PSP-3000 NTSC progressive baseline. */
    if(mode==0x2D2 && yres==480u) {
        if(enabled==0u && realx==480u && realy==272u &&
           reg32(0x144u)==120u && reg32(0x140u)==104u)
            return AV_P_BASE;
        if(enabled==1u && realx==698u &&
           (realy==360u || realy==480u) && inx==480u && iny==272u)
            return AV_P_SCALED;
    }

    /* PSP-3000 04g FW6.61 ARK-5 measured 480i GAME state:
     * mode=0x6D1, output 720x479, shiftX=120, shiftY=103,
     * source=480x272, scaler disabled. The earlier 503-508 guard
     * incorrectly described the XMB transition, not the game mode. */
    if(mode==0x6D1 && (yres==479u || yres==480u)) {
        if(yres==479u && enabled==0u && realx==480u && realy==272u &&
           reg32(0x144u)==120u && reg32(0x140u)==103u)
            return AV_I_BASE;
        if(enabled==1u && realx==698u &&
           (realy==360u || realy==480u) && inx==480u && iny==272u)
            return AV_I_SCALED;
    }
    return OTHER;
}

static int current_hw_format(void) {
    void *fb=0;
    int stride=0,format=-1;
    if(sceDisplayGetFrameBuf(&fb,&stride,&format,0)<0 || fb==0 ||
       stride<480 || (stride&63)!=0 || format<0 || format>3)
        return -1;
    return (int)((0x39u>>((unsigned int)format*2u))&3u);
}

static void record(unsigned int ms,int event,int kind,int mode,int w,int h) {
    DiagRow *r;
    if(g_row_count>=ROWS_MAX) return;
    r=&g_rows[g_row_count++];
    r->ms=ms; r->event=(unsigned int)event; r->kind=(unsigned int)kind;
    r->mode=(unsigned int)mode;r->width=(unsigned int)w;r->height=(unsigned int)h;
    r->syncdiff=reg32(0x104u);
    r->xres=reg32(0x11Cu);r->yres=reg32(0x12Cu);
    r->shiftx=reg32(0x144u);r->shifty=reg32(0x140u);
    r->realx=reg32(0x148u);r->realy=reg32(0x14Cu);
    r->enabled=reg32(0x180u);r->inputx=reg32(0x184u);r->inputy=reg32(0x188u);
    r->aspect=(unsigned int)g_aspect;r->preset=(unsigned int)g_preset;
    r->select_return=(unsigned int)g_select_return;
    r->refresh_return=(unsigned int)g_refresh_return;
    r->hw_format=(unsigned int)g_hw_format;
    r->lcdc_flags=reg32(0x1A0u);
    r->coef_h_half=reg32(0x18Cu);
    r->coef_v_half=reg32(0x190u);
    r->coef_h_step=reg32(0x194u);
    r->coef_v_step=reg32(0x198u);
}
static char *num(char *p,unsigned int v) {
    char t[11];int n=0;
    do {t[n++]=(char)('0'+v%10u);v/=10u;}while(v && n<11);
    while(n) *p++=t[--n];
    return p;
}
static char *hex(char *p,unsigned int v) {
    static const char digits[]="0123456789ABCDEF";
    int k;*p++='0';*p++='x';
    for(k=7;k>=0;k--) *p++=digits[(v>>(k*4))&15u];
    return p;
}
static void write_csv(void) {
    static const char header[]="ms,event,kind,mode,w,h,syncdiff,xres,yres,shiftx,shifty,realx,realy,enabled,inx,iny,aspect,preset,select_return,refresh_return,hw_format,lcdc_flags,coef_h_half,coef_v_half,coef_h_step,coef_v_step\n";
    SceUID fd=sceIoOpen(PROBE_PATH,PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    unsigned int i;
    if(fd<0) return;
    sceIoWrite(fd,header,sizeof(header)-1u);
    for(i=0;i<g_row_count;i++) {
        const DiagRow *r=&g_rows[i];
        char line[512],*p=line;
#define D(v) do{p=num(p,(v));*p++=',';}while(0)
#define X(v) do{p=hex(p,(v));*p++=',';}while(0)
        D(r->ms);D(r->event);D(r->kind);X(r->mode);D(r->width);D(r->height);
        D(r->syncdiff);D(r->xres);D(r->yres);D(r->shiftx);D(r->shifty);
        D(r->realx);D(r->realy);D(r->enabled);D(r->inputx);D(r->inputy);
        X(r->aspect);D(r->preset);X(r->select_return);X(r->refresh_return);
        X(r->hw_format);X(r->lcdc_flags);X(r->coef_h_half);
        X(r->coef_v_half);X(r->coef_h_step);p=hex(p,r->coef_v_step);*p++='\n';
#undef D
#undef X
        sceIoWrite(fd,line,(SceSize)(p-line));
    }
    sceIoClose(fd);
}

static int worker(SceSize args,void *argp) {
    unsigned int start=sceKernelGetSystemTimeLow();
    unsigned int stable_at=start,selected_at=0,refreshed_at=0,last_log=0;
    unsigned int interlace_seen_at=0;
    int last_kind=-1,last_mode=-1,selected=0;
    int kind=OTHER,mode=-1,w=0,h=0;
    (void)args;(void)argp;
    while(g_running) {
        unsigned int now=sceKernelGetSystemTimeLow();
        unsigned int elapsed=(unsigned int)(now-start);
        kind=get_kind(&mode,&w,&h);
        if(kind!=last_kind || mode!=last_mode) {
            stable_at=now;last_kind=kind;last_mode=mode;
            record(elapsed/1000u,SAMPLE,kind,mode,w,h);
            last_log=now;
        } else if((unsigned int)(now-last_log)>=1000000u) {
            record(elapsed/1000u,SAMPLE,kind,mode,w,h);
            last_log=now;
        }
        if(elapsed>=STARTUP_TIMEOUT_US) break;

        /* Preselect 480i scaler BEFORE the normal GAME display transition.
         * No manual LCDC SetMode and no writes to display registers.
         * If the module started late, do not enter this path. */
        if(!selected && mode==0xD1 && w==720 && h==503 && elapsed<=600000u) {
            g_early_interlace=1;
            g_aspect=read_tv_type();
            selected=1;
            if(g_aspect<0) {
                record(elapsed/1000u,FAILED,kind,mode,w,h);
                break;
            }
            g_preset=(g_aspect==0)?5:1;
            g_select_return=sceLcdc_driver_451FE1A1(g_preset);
            selected_at=now;
            record(elapsed/1000u,SELECTED,kind,mode,w,h);
            if(g_select_return!=0) break;
        }
        if(!selected && kind==AV_I_BASE && (unsigned int)(now-stable_at)>=STABLE_US) {
            /* Too late to safely preselect; never force LCDC id17. */
            record(elapsed/1000u,FAILED,kind,mode,w,h);
            break;
        }
        if(!selected && kind!=OTHER && (unsigned int)(now-stable_at)>=STABLE_US) {
            g_aspect=read_tv_type();
            selected=1;
            if(g_aspect<0) {
                record(elapsed/1000u,FAILED,kind,mode,w,h);
                break;
            }
            g_preset=(g_aspect==0)?5:1;
            g_select_return=sceLcdc_driver_451FE1A1(g_preset);
            selected_at=now;
            record(elapsed/1000u,SELECTED,kind,mode,w,h);
            if(g_select_return!=0 || kind==LCD) break;
        }
        if(selected) {
            if(g_early_interlace) {
                if(kind==AV_I_BASE || kind==AV_I_SCALED) {
                    if(interlace_seen_at==0) interlace_seen_at=now;
                    if(kind==AV_I_SCALED &&
                       (unsigned int)(now-interlace_seen_at)>=300000u) break;
                    if((unsigned int)(now-interlace_seen_at)>=3000000u) break;
                }
                /* Never run an explicit LCDC mode reconfiguration here. */
            } else {
            if(kind==AV_P_SCALED) break;
            if(!g_refreshed && kind==AV_P_BASE &&
               (unsigned int)(now-selected_at)>=REFRESH_DELAY_US &&
               (unsigned int)(now-stable_at)>=STABLE_US) {
                g_refreshed=1;
                g_hw_format=current_hw_format();
                if(g_hw_format>=0) {
                    /* Known-good progressive path only. */
                    g_refresh_return=sceLcdc_driver_E9DBD35F(1,480,272,g_hw_format);
                }
                refreshed_at=now;
                record(elapsed/1000u,REFRESHED,kind,mode,w,h);
            }
            if(g_refreshed && (unsigned int)(now-refreshed_at)>=AFTER_REFRESH_US)
                break;
            }
        }
        sceKernelDelayThread(POLL_US);
    }
    kind=get_kind(&mode,&w,&h);
    record((unsigned int)(sceKernelGetSystemTimeLow()-start)/1000u,FINISHED,kind,mode,w,h);
    write_csv();
    return 0;
}

int module_start(SceSize args,void *argp) {
    (void)args;(void)argp;
    g_running=1;
    g_thread=sceKernelCreateThread("PSPTVFullRC7",worker,0x18,0x3000,0,0);
    if(g_thread<0) return 1;
    if(sceKernelStartThread(g_thread,0,0)<0) {
        sceKernelDeleteThread(g_thread);g_thread=-1;return 1;
    }
    return 0;
}
int module_stop(SceSize args,void *argp) {
    (void)args;(void)argp;
    g_running=0;
    if(g_thread>=0) {
        sceKernelWaitThreadEnd(g_thread,0);
        sceKernelDeleteThread(g_thread);g_thread=-1;
    }
    return 0;
}
