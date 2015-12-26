/*
 *   This is a simple program that demonstrate the linkage
 *   between PMON2000 and a loaded program.
 */

typedef long long off_t;

struct callvectors {
	int     (*open) (char *, int, int);
	int     (*close) (int);
	int     (*read) (int, void *, int);
	int     (*write) (int, void *, int);
	off_t   (*lseek) (int, off_t, int);
	int     (*printf) (const char *, ...);
	void    (*cacheflush) (void);
	char    *(*gets) (char *);
};

struct callvectors *callvec;

#define	printf (*callvec->printf)
#define	gets   (*callvec->gets)


#ifdef HAVE_FLOAT
#include <math.h>
#define printf newprintf
#endif

#if _ABIO32
#define LADDIU "addiu"
#else
#define LADDIU "daddiu"
#endif

int scan()
{
	void *p;
	for(p=0x90000000;p<0xa0000000;p=p+4)
	{
		if((((long)p)&0xffff)==0) printf("%x\n", p);
		*(volatile int *)p;
	}

}


int guess(unsigned long addr, unsigned long end)
{
	while(addr<end)
	{
		printf("%x\n", addr);
		asm volatile(
				".set push;\n"
				".set noreorder;\n"
				".set mips64;\n"
				"1:lw %1,(%2);\n"
				"beqzl  %1,1f;\n"
				"nop;\n"
				LADDIU " %0,4;\n"
				"andi %1, %0, 0xffff;\n"
				"beqz %1,2f;\n"
				"nop;\n"
				"b 1b;\n"
				"nop;\n"
				"1:lw %1,(%0);\n"
				"b 1b;\n"
				"nop;\n"
				"2:\n"
				".set pop;\n"
				:"=r"(addr):"r"(0),"r"(0xa0000000),"0"(addr)
			    );
	}
}


int guess1(unsigned long addr, unsigned long end)
{
	while(addr<end)
	{
		printf("%x\n", addr);
		asm volatile(
				".set push;\n"
				".set noreorder;\n"
				".set mips64;\n"
				"1:lw %1,(%2);\n"
				"beqzl  %1,1f;\n"
				"nop;\n"
				LADDIU " %0,4;\n"
				"andi %1, %0, 0xff;\n"
				"beqz %1,2f;\n"
				"nop;\n"
				"b 1b;\n"
				"nop;\n"
				"1:jr %0;\n"
				"nop;\n"
				"2:\n"
				".set pop;\n"
				:"=r"(addr):"r"(0),"r"(0xa0000000),"0"(addr)
			    );
	}
}


int mymain(long from,int to)
{
#ifdef HAVE_FLOAT
tgt_fpuenable();
#endif
//	printf("buf=0x%x,c=0x%x,len=0x%x\n",buf,c,len);
//	memset(buf,c,len);
	printf("begin test\n");
	if(*(volatile int *)0xa0000000 == 0)
	*(volatile int *)0xa0000000 = 0x25;
	if(*(volatile int *)0xa0000000 == 0)
	{
	 printf("0xa0000000 can not write");
	 return 0;
	}

//	scan();

	if(from && to)
	{
	guess1(from, to);
	}
	else
	{
	guess1(0xbf000000, 0xc0000000);
	guess1(0x9f000000, 0xa0000000);
	}
	return 0;
}



void __gccmain(void);
void __gccmain(void){}
int
main(int argc, char **argv, char **env, struct callvectors *cv)
{
	callvec = cv;
	printf("begin guess\n");
	mymain(0,0);
	return(0);
}
