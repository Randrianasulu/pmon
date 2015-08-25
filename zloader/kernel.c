#include "kernel.h"
const int read, write, open, close, printf, vsprintf, getenv, CpuTertiaryCacheSize;
const char *argv[] ={"g", "console=ttyS0,115200 rdinit=/sbin/init",0 };
const char *env[] ={"memsize=128", "highmemsize=0", };

int initmips()
{
((int (*)(int, char **,char **))ENTRY)(2,argv,env);
}

