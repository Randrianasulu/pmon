//Created by xiexin for Display Controller pmon test 
//Oct 6th,2009
#include <pmon.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/malloc.h>
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;
typedef int bool;
typedef unsigned long dma_addr_t;

//#define TEST_800x600 1
#undef TEST_800x600
#ifdef LS1FSOC
#define DC_FB1 1
#undef DC_FB0
#else
#define DC_FB0 1
#undef DC_FB1
#endif

#define writeb(val, addr) (*(volatile u8*)(addr) = (val))
#define writew(val, addr) (*(volatile u16*)(addr) = (val))
#define writel(val, addr) (*(volatile u32*)(addr) = (val))
#define readb(addr) (*(volatile u8*)(addr))
#define readw(addr) (*(volatile u16*)(addr))
#define readl(addr) (*(volatile u32*)(addr))

#define write_reg(addr,val) writel(val,addr)

#define EXTRA_PIXEL  0
#define DC_BASE_ADDR 0xbc301240
#define DC_BASE_ADDR_1 0xbc301250

#define RANDOM_HEIGHT_Z 37

static char *ADDR_CURSOR = 0xA6000000;
static char *MEM_ptr = 0xA2000000;
static int MEM_ADDR =0;

struct vga_struc{
           long pclk;
           int hr,hss,hse,hfl;
	   int vr,vss,vse,vfl;
}
vgamode[] =
{
{/*"640x480_70.00"*/    28560,  640,    664,    728,    816,    480,    481,    484,    500,    },
{/*"640x640_60.00"*/	33100,	640,	672,	736,	832,	640,	641,	644,	663,	},
{/*"640x768_60.00"*/	39690,	640,	672,	736,	832,	768,	769,	772,	795,	},
{/*"640x800_60.00"*/	42130,	640,	680,	744,	848,	800,	801,	804,	828,	},
{/*"800x480_70.00"*/    35840,  800,    832,    912,    1024,   480,    481,    484,    500,    },
{/*"800x600_60.00"*/	38220,	800,	832,	912,	1024,	600,	601,	604,	622,	},
{/*"800x640_60.00"*/	40730,	800,	832,	912,	1024,	640,	641,	644,	663,	},
{/*"832x600_60.00"*/	40010,	832,	864,	952,	1072,	600,	601,	604,	622,	},
{/*"832x608_60.00"*/	40520,	832,	864,	952,	1072,	608,	609,	612,	630,	},
{/*"1024x480_60.00"*/	38170,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	},
{/*"1024x600_60.00"*/	48960,	1024,	1064,	1168,	1312,	600,	601,	604,	622,	},
{/*"1024x640_60.00"*/	52830,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	},
{/*"1024x768_60.00"*/	64110,	1024,	1080,	1184,	1344,	768,	769,	772,	795,	},
{/*"1152x764_60.00"*/   71380,  1152,   1208,   1328,   1504,   764,    765,    768,    791,    },
{/*"1280x800_60.00"*/   83460,  1280,   1344,   1480,   1680,   800,    801,    804,    828,    },
{/*"1280x1024_55.00"*/  98600,  1280,   1352,   1488,   1696,   1024,   1025,   1028,   1057,   },
{/*"1440x800_60.00"*/   93800,  1440,   1512,   1664,   1888,   800,    801,    804,    828,    },
{/*"1440x900_67.00"*/   120280, 1440,   1528,   1680,   1920,   900,    901,    904,    935,    },
};

enum{
OF_BUF_CONFIG=0,
OF_BUF_ADDR=0x20,
OF_BUF_STRIDE=0x40,
OF_BUF_ORIG=0x60,
OF_DITHER_CONFIG=0x120,
OF_DITHER_TABLE_LOW=0x140,
OF_DITHER_TABLE_HIGH=0x160,
OF_PAN_CONFIG=0x180,
OF_PAN_TIMING=0x1a0,
OF_HDISPLAY=0x1c0,
OF_HSYNC=0x1e0,
OF_VDISPLAY=0x240,
OF_VSYNC=0x260,
OF_DBLBUF=0x340,
};

#define MYDBG printf(":%d\n",__LINE__);
#define PLL_FREQ_REG(x) *(volatile unsigned int *)(0xbfe78030+x)

#ifdef LS1FSOC
int caclulatefreq(long long XIN,long long PCLK)
{
long N=4,NO=4,OD=2,M,FRAC;
int flag=0;
long  out;
long long MF;
printf("PCLK=%lld\n",PCLK);

while(flag==0){
flag=1;
printf("N=%lld\n",N);
if(XIN/N<5000) {N--;flag=0;}
if(XIN/N>50000) {N++;flag=0;}
}
flag=0;
while(flag==0){
flag=1;
if(PCLK*NO<200000) {NO*=2;OD++;flag=0;}
if(PCLK*NO>700000) {NO/=2;OD--;flag=0;}
}
MF=PCLK*N*NO*262144/XIN;
MF %= 262144;
M=PCLK*N*NO/XIN;
FRAC=(int)(MF);
out = (FRAC<<14)+(OD<<12)+(N<<8)+M;

printf("in this case, M=%llx ,N=%llx, OD=%llx, FRAC=%llx\n",M,N,OD,FRAC);
return out;
}

#else

#define abs(x) ((x<0)?(-x):x)
#define min(a,b) ((a<b)?a:b)

int caclulatefreq(long long XIN,long long PCLK)
{
	int i;
	long long clk,clk1;
	int start,end;
	int mi;
	int pll,ctrl,div,div1,frac;
	pll=PLL_FREQ_REG(0);
	ctrl=PLL_FREQ_REG(4);
	printf("pll=0x%x,ctrl=0x%x\n",pll,ctrl);
	printf("cpu freq is %d\n",tgt_pipefreq());
	start=-1;
	end=1;
/*ls1b02:dc clk divided 4 */
	PCLK *= 4;

	for(i=start;i<=end;i++)
	{
	clk=(12+i+(pll&0x3f))*33333333/2;
	div=clk/(long)PCLK/1000;
	clk1=(12+i+1+(pll&0x3f))*33333333/2;
	div1=clk1/(long)PCLK/1000;
	if(div!=div1)break;
	}

	if(div!=div1)
	{
	frac=((PCLK*1000*div1)*2*1024/33333333 - (12+i+(pll&0x3f))*1024)&0x3ff;
	pll = (pll & ~0x3ff3f)|(frac<<8)|((pll&0x3f)+i);
	ctrl = ctrl&~(0x1f<<26)|(div1<<26)|(1<<31);
	}
	else
	{
	clk=(12+start+(pll&0x3f))*33333333/2;
	clk1=(12+end+(pll&0x3f))*33333333/2;
	if(abs((long)clk/div/1000-PCLK)<abs((long)clk1/(div+1)/1000-PCLK))
	{
	pll = (pll & ~0x3ff3f)|((pll&0x3f)+start);
	ctrl = ctrl&~(0x1f<<26)|(div<<26)|(1<<31);
	}
	else
	{
	pll = (pll & ~0x3ff3f)|((pll&0x3f)+end);
	ctrl = ctrl&~(0x1f<<26)|((div+1)<<26)|(1<<31);
	}
	}

	printf("new pll=%x: ctrl=%x\n",pll,ctrl);
	ctrl |= 0x2a00;
	PLL_FREQ_REG(4) = ctrl; 
	PLL_FREQ_REG(0) = pll;
	delay(1000);
	initserial(0);
	_probe_frequencies();
	printf("cpu freq is %d\n",tgt_pipefreq());
	return 0;
}

#endif

int config_cursor()
{
  printf("framebuffer Cursor Configuration\n");
  write_reg((0xbc301520  +0x00),0x00020200);
  printf("framebuffer Cursor Address\n");
  write_reg((0xbc301530  +0x00),ADDR_CURSOR);
  printf("framebuffer Cursor Location\n");
  write_reg((0xbc301540  +0x00),0x00060122);
  printf("framebuffer Cursor Background\n");
  write_reg((0xbc301550  +0x00),0x00eeeeee);
  printf("what hell is this register for ?\n");
  write_reg((0xbc301560  +0x00),0x00aaaaaa);
}

static int fb_xsize, fb_ysize;

int config_fb(unsigned long base)
{
int i,mode=-1;

  for(i=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++)
  {
	  int out;
	  if(vgamode[i].hr == fb_xsize && vgamode[i].vr == fb_ysize){
		  mode=i;
#ifdef LS1FSOC
		  out = caclulatefreq(APB_CLK/1000,vgamode[i].pclk);
		  printf("out=%x\n",out);
   /*inner gpu dc logic fifo pll ctrl,must large then outclk*/
   *(volatile int *)0xbfd00414 = out+1;
   /*output pix1 clock  pll ctrl*/
   *(volatile int *)0xbfd00410 = out;
   /*output pix2 clock pll ctrl */
   *(volatile int *)0xbfd00424 = out;
#elif !defined(CONFIG_FB_DYN)
		  caclulatefreq(APB_CLK/1000,vgamode[i].pclk);
#endif
		  break;
	  }
  }

  if(mode<0)
  {
	  printf("\n\n\nunsupported framebuffer resolution\n\n\n");
	  return;
  }


//  Disable the panel 0
  write_reg((base+OF_BUF_CONFIG),0x00000000);
// framebuffer configuration RGB565
  write_reg((base+OF_BUF_CONFIG),0x00000003);
  write_reg((base+OF_BUF_ADDR),MEM_ADDR  );
  write_reg(base+OF_DBLBUF,MEM_ADDR );
  write_reg((base+OF_DITHER_CONFIG),0x00000000);
  write_reg((base+OF_DITHER_TABLE_LOW),0x00000000);
  write_reg((base+OF_DITHER_TABLE_HIGH),0x00000000);
  write_reg((base+OF_PAN_CONFIG),0x80001311);
  write_reg((base+OF_PAN_TIMING),0x00000000);

  write_reg((base+OF_HDISPLAY),(vgamode[mode].hfl<<16)|vgamode[mode].hr);
  write_reg((base+OF_HSYNC),0x40000000|(vgamode[mode].hse<<16)|vgamode[mode].hss);
  write_reg((base+OF_VDISPLAY),(vgamode[mode].vfl<<16)|vgamode[mode].vr);
  write_reg((base+OF_VSYNC),0x40000000|(vgamode[mode].vse<<16)|vgamode[mode].vss);

#if defined(CONFIG_VIDEO_32BPP)
  write_reg((base+OF_BUF_CONFIG),0x00100104);
  write_reg((base+OF_BUF_STRIDE),fb_xsize*4); //1024
#elif defined(CONFIG_VIDEO_16BPP)
  write_reg((base+OF_BUF_CONFIG),0x00100103);
  write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#elif defined(CONFIG_VIDEO_15BPP)
  write_reg((base+OF_BUF_CONFIG),0x00100102);
  write_reg((base+OF_BUF_STRIDE),fb_xsize*2); //1024
#elif defined(CONFIG_VIDEO_12BPP)
  write_reg((base+OF_BUF_CONFIG),0x00100101);
  write_reg((base+OF_BUF_STRIDE),fb_xsize*2); //1024
#else  //640x480-32Bits
  write_reg((base+OF_BUF_CONFIG),0x00100104);
  write_reg((base+OF_BUF_STRIDE),fb_xsize*4); //640
#endif //32Bits

#ifdef LS1GSOC
/*fix ls1g dc
 *first switch to tile mode
 *change origin register to 0
 *goback nomal mode
 */
  {
  int val;
  val = readl((base+OF_BUF_CONFIG));
  write_reg((base+OF_BUF_CONFIG),val|0x10);
  write_reg((base+OF_BUF_ORIG),0);
  readl((base+OF_BUF_ORIG));
  delay(1000000);
  readl((base+OF_BUF_CONFIG));
  write_reg((base+OF_BUF_CONFIG),val);
  }
#endif

}

int dc_init()
{
   int print_count;
   int i;
   int init_R = 0;
   int init_G = 0;
   int init_B = 0;
   int j;
   int ii=0,tmp=0;

   
   int  print_addr;
   int print_data;
   printf("enter dc_init...\n");

   fb_xsize  = getenv("xres")? strtoul(getenv("xres"),0,0):FB_XSIZE;
   fb_ysize  = getenv("yres")? strtoul(getenv("yres"),0,0):FB_YSIZE;

MEM_ADDR = (long)MEM_ptr&0x0fffffff;


if(MEM_ptr == NULL)
{
	printf("frame buffer memory malloc failed!\n ");
	exit(0);
}
 
for(ii=0;ii<0x1000; ii+=4)
*(volatile unsigned int *)(ADDR_CURSOR + ii) = 0x88f31f4f;

ADDR_CURSOR = (long)ADDR_CURSOR & 0x0fffffff;

printf("frame buffer addr: %x \n",MEM_ADDR);
  
#ifdef DC_FB0
config_fb(DC_BASE_ADDR);
#endif
#ifdef DC_FB1 
config_fb(DC_BASE_ADDR_1);
#endif
config_cursor();


printf("display controller reg config complete!\n");


return MEM_ptr;
}

static int cmd_dc_freq(int argc,char **argv)
{
	int out;
	long sysclk;
	long pclk;
	if(argc<2)return -1;
	pclk=strtoul(argv[1],0,0);
	if(argc>2) sysclk=strtoul(argv[2],0,0);
	else sysclk=33333;
	out = caclulatefreq(sysclk,pclk);
	printf("out=%x\n",out);
	/*inner gpu dc logic fifo pll ctrl,must large then outclk*/
	*(volatile int *)0xbfd00414 = out+1;
	/*output pix1 clock  pll ctrl*/
	*(volatile int *)0xbfd00410 = out;
	/*output pix2 clock pll ctrl */
	*(volatile int *)0xbfd00424 = out;

	return 0;
}

static int cmd_caclfreq(int argc,char **argv)
{
	_probe_frequencies();
	printf("freq is %d\n",tgt_pipefreq());
	return 0;
}

static int cmd_initserial(int argc,char **argv)
{
	initserial(argc>1?strtoul(argv[1],0,0):0);
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"dc_freq"," pclk sysclk", 0, "config dc clk(khz)",cmd_dc_freq, 0, 99, CMD_REPEAT},
	{"caclfreq","", 0, "cacl freq",cmd_caclfreq, 0, 99, CMD_REPEAT},
	{"initserial","[ddrclk]", 0, "cacl freq",cmd_initserial, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

