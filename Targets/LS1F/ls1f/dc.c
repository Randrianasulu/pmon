//Created by xiexin for Display Controller pmon test 
//Oct 6th,2009



#include <stdlib.h>
#include <stdio.h>
#include <sys/malloc.h>
//#include <sys/mbuf.h>



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

#define DIS_WIDTH  FB_XSIZE  // 640 // 800
#define DIS_HEIGHT FB_YSIZE // 480 //  600 
#define EXTRA_PIXEL  0
#define DC_BASE_ADDR 0xbc301240
#define DC_BASE_ADDR_1 0xbc301250

#define RANDOM_HEIGHT_Z 37

static char *ADDR_CURSOR = 0xA6000000;
static char *MEM_ptr = 0xA2000000;
static int MEM_ADDR =0;
#ifdef DC_FB1 
static char *MEM_ptr_1 = 0xA2000000;
static int MEM_ADDR_1 =0;
#endif

struct vga_struc{
           float pclk;
           int hr,hss,hse,hfl;
	   int vr,vss,vse,vfl;
}
vgamode[] =
{{/*"640x480_60.00"*/	23.86,	640,	656,	720,	800,	480,	481,	484,	497,	},
{/*"640x640_60.00"*/	33.10,	640,	672,	736,	832,	640,	641,	644,	663,	},
{/*"640x768_60.00"*/	39.69,	640,	672,	736,	832,	768,	769,	772,	795,	},
{/*"640x800_60.00"*/	42.13,	640,	680,	744,	848,	800,	801,	804,	828,	},
{/*"800x480_60.00"*/	29.58,	800,	816,	896,	992,	480,	481,	484,	497,	},
{/*"800x600_60.00"*/	38.22,	800,	832,	912,	1024,	600,	601,	604,	622,	},
{/*"800x640_60.00"*/	40.73,	800,	832,	912,	1024,	640,	641,	644,	663,	},
{/*"832x600_60.00"*/	40.01,	832,	864,	952,	1072,	600,	601,	604,	622,	},
{/*"832x608_60.00"*/	40.52,	832,	864,	952,	1072,	608,	609,	612,	630,	},
{/*"1024x480_60.00"*/	38.17,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	},
{/*"1024x600_60.00"*/	48.96,	1024,	1064,	1168,	1312,	600,	601,	604,	622,	},
{/*"1024x640_60.00"*/	52.83,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	},
{/*"1024x768_60.00"*/	64.11,	1024,	1080,	1184,	1344,	768,	769,	772,	795,	},
{/*"1024x768_60.00"*/	64.11,	1024,	1080,	1184,	1344,	768,	769,	772,	795,	},
{/*"1024x768_60.00"*/	64.11,	1024,	1080,	1184,	1344,	768,	769,	772,	795,	},
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


int config_fb(unsigned long base)
{
int i,got;
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

  for(i=0,got=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++)
  {
	  if(vgamode[i].hr == FB_XSIZE && vgamode[i].vr == FB_YSIZE){
		  got=1;
		  write_reg((base+OF_HDISPLAY),(vgamode[i].hfl<<16)|vgamode[i].hr);
		  write_reg((base+OF_HSYNC),0x40000000|(vgamode[i].hse<<16)|vgamode[i].hss);
		  write_reg((base+OF_VDISPLAY),(vgamode[i].vfl<<16)|vgamode[i].vr);
		  write_reg((base+OF_VSYNC),0x40000000|(vgamode[i].vse<<16)|vgamode[i].vss);
		  break;
	  }
  }

  if(!got)
  {
	  printf("\n\n\nunsupported framebuffer resolution\n\n\n");
	  return;
  }

#if defined(CONFIG_VIDEO_32BPP)
  write_reg((base+0x00),0x00100104);
  write_reg((base+OF_BUF_STRIDE),FB_XSIZE*4); //1024
#elif defined(CONFIG_VIDEO_16BPP)
  write_reg((base+0x00),0x00100103);
  write_reg((base+OF_BUF_STRIDE),FB_XSIZE*2); //1024
#elif defined(CONFIG_VIDEO_15BPP)
  write_reg((base+0x00),0x00100102);
  write_reg((base+OF_BUF_STRIDE),FB_XSIZE*2); //1024
#elif defined(CONFIG_VIDEO_12BPP)
  write_reg((base+0x00),0x00100101);
  write_reg((base+OF_BUF_STRIDE),FB_XSIZE*2); //1024
#else  //640x480-32Bits
  write_reg((base+OF_BUF_CONFIG),0x00100104);
  write_reg((base+OF_BUF_STRIDE),FB_XSIZE*4); //640
#endif //32Bits

}

int dc_init()
{
   int print_count;
   int i;
   int PIXEL_COUNT = DIS_WIDTH * DIS_HEIGHT +  EXTRA_PIXEL; 
   int MEM_SIZE; // = PIXEL_COUNT * 4;
  // int MEM_SIZE = PIXEL_COUNT * 2;
   int init_R = 0;
   int init_G = 0;
   int init_B = 0;
   int j;
   int ii=0,tmp=0;
   int MEM_SIZE_3 = MEM_SIZE /6; 

    int line_length=0;
   
   int  print_addr;
   int print_data;
   printf("enter dc_init...\n");
#ifdef LS1FSOC
   /*inner gpu dc logic fifo pll ctrl,must large then outclk*/
   *(volatile int *)0xbfd00414 = 0xc5d6641f;
   /*output pix1 clock  pll ctrl*/
   *(volatile int *)0xbfd00410 = 0xc5d6641e;
   /*output pix2 clock pll ctrl */
   *(volatile int *)0xbfd00424 = 0xc5d6641e;
#endif

#if defined(CONFIG_VIDEO_32BPP)
MEM_SIZE = PIXEL_COUNT * 4;
line_length = FB_XSIZE * 4;
#elif defined(CONFIG_VIDEO_16BPP)
MEM_SIZE = PIXEL_COUNT * 2;
line_length = FB_XSIZE * 2;
#elif defined(CONFIG_VIDEO_15BPP)
MEM_SIZE = PIXEL_COUNT * 2;
line_length = FB_XSIZE * 2;
#elif defined(CONFIG_VIDEO_12BPP)
MEM_SIZE = PIXEL_COUNT * 2;
line_length = FB_XSIZE * 2;
#else
MEM_SIZE = PIXEL_COUNT * 4;
line_length = FB_XSIZE * 4;
#endif

MEM_ADDR = (long)MEM_ptr&0x0fffffff;

#ifdef DC_FB1 
MEM_ADDR_1 = (long)MEM_ptr_1&0x0fffffff;
#endif

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

#ifdef DC_FB1
return MEM_ptr_1;
#endif

return MEM_ptr;
}

