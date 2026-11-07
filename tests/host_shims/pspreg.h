#ifndef H_PSPREG
#define H_PSPREG
typedef int REGHANDLE;
struct RegParam {int regtype; unsigned int namelen;int unk2,unk3;char name[64];};
int sceRegOpenRegistry(struct RegParam*,int,REGHANDLE*);
int sceRegOpenCategory(REGHANDLE,const char*,int,REGHANDLE*);
int sceRegGetKeyValueByName(REGHANDLE,const char*,void*,unsigned int);
int sceRegCloseCategory(REGHANDLE);
int sceRegCloseRegistry(REGHANDLE);
#endif
