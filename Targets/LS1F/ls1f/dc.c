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

#define TEST_800x600 1
//#undef TEST_800x600
#define DC_FB0 1
//#undef DC_FB0 
#define DC_FB1 1

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


    char *MEM_ptr = 0xA2000000;
    char *ADDR_CURSOR = 0xA6000000;
#ifdef DC_FB1 
    char *MEM_ptr_1 = 0xA2000000;
#endif
int dc_init()
{
   int MEM_ADDR =0;
#ifdef DC_FB1 
   int MEM_ADDR_1 =0;
#endif
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
   /*gpu pll ctrl*/
   *(volatile int *)0xbfd00414 = 0x1518;
   /*pix pll ctrl */
   *(volatile int *)0xbfd00424 = 0x1510;

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

   printf("frame buffer addr: %x \n",(MEM_ADDR+0x00));

   printf("frame buffer data: %x \n",readl((MEM_ptr+0x00)));
  
  //config display controller reg 
  printf("Disable the panel 0\n");
  write_reg((DC_BASE_ADDR+0x00),0x00000000);
#ifdef DC_FB0
  printf("framebuffer configuration RGB565\n");
  write_reg((DC_BASE_ADDR+0x00),0x00000003);
  printf("framebuffer address 0x%x\n", MEM_ADDR);
  write_reg((DC_BASE_ADDR+0x20),MEM_ADDR  );
//  printf("framebuffer stride\n");
//  write_reg((DC_BASE_ADDR+0x40),0x00000500);
  printf("framebuffer Display Configuration\n");
  write_reg((0xbc301360  +0x00),0x00000000);
  printf("framebuffer Display Dither Table Low\n");
  write_reg((0xbc301360  +0x20),0x00000000);
  printf("framebuffer Display Dither Table High\n");
  write_reg((0xbc301360  +0x40),0x00000000);
  printf("framebuffer Pane Configuration\n");
  write_reg((0xbc301360  +0x60),0x80001311);
  printf("framebuffer Pane Timing\n");
  write_reg((0xbc301360  +0x80),0x00000000);
  //write_reg((0xbc301360  +0x80),0x33333333);
#endif

#if 0
  printf("framebuffer HDisplay\n");
  write_reg((0xbc301400  +0x00),0x014f0140);
  printf("framebuffer HSync\n");
  write_reg((0xbc301400  +0x20),0x414a0145);
  printf("framebuffer VDisplay\n");
  write_reg((0xbc301400  +0x80),0x00fa00f0);
  printf("framebuffer VSync\n");
  write_reg((0xbc301400  +0xa0),0x40f700f5);
#endif

  printf("framebuffer Cursor Configuration\n");
#ifdef DC_FB0
  write_reg((0xbc301520  +0x00),0x00020200);
#endif
#ifdef DC_FB1
  write_reg((0xbc301520  +0x00),0x00020210);
#endif
  printf("framebuffer Cursor Address\n");
  write_reg((0xbc301530  +0x00),ADDR_CURSOR);
  printf("framebuffer Cursor Location\n");
  write_reg((0xbc301540  +0x00),0x00060122);
  printf("framebuffer Cursor Background\n");
  write_reg((0xbc301550  +0x00),0x00eeeeee);
  printf("what hell is this register for ?\n");
  write_reg((0xbc301560  +0x00),0x00aaaaaa);
//==================panel 1========================
//  printf(" write_reg((DC_BASE_ADDR_1+0x00),0x00100000);\n");
//  write_reg((DC_BASE_ADDR_1+0x00),0x00100000);

  #ifdef DC_FB1
  printf(" write_reg((DC_BASE_ADDR_1+0x20),MEM_ADDR  );\n");
  write_reg((DC_BASE_ADDR_1+0x20),MEM_ADDR_1  );
//800  write_reg((DC_BASE_ADDR_1+0x40),0x00000C80);
  printf(" write_reg((0xbc301370  +0x00),0x00000000);\n");
  write_reg((0xbc301370  +0x00),0x00000000);
  printf(" write_reg((0xbc301370  +0x20),0x00000000);\n");
  write_reg((0xbc301370  +0x20),0x00000000);
  printf(" write_reg((0xbc301370  +0x40),0x00000000);\n");
  write_reg((0xbc301370  +0x40),0x00000000);
  //printf(" write_reg((0xbc301370  +0x60),0x80001111);\n");
  printf(" write_reg((0xbc301370  +0x60),0x80001311);\n");
  write_reg((0xbc301370  +0x60),0x80001311);
 // printf(" write_reg((0xbc301370  +0x80),0x33333333);\n");
  printf(" write_reg((0xbc301370  +0x80),0x000000000);\n");
  write_reg((0xbc301370  +0x80),0x00000000);
  // write_reg((0xbc301370  +0x80),0x33333333);
  #endif

//800x600@60
    #if 0
    write_reg((0xbc301410  +0x00),0x04000320);
    write_reg((0xbc301410  +0x20),0x43900340);
    write_reg((0xbc301410  +0x80),0x026e0258);
    write_reg((0xbc301410  +0xa0),0x425c0259);
    #endif

//  write_reg((0xbc301410  +0x00),0x04000320);
//  write_reg((0xbc301410  +0x20),0x43900340);
//  write_reg((0xbc301410  +0x80),0x026E0258);
//  write_reg((0xbc301410  +0xa0),0x425C0259);
//  
/*704x598  
  write_reg((0xbc301410  +0x00),0x038002C0);
  write_reg((0xbc301410  +0x20),0x432002D8);
  write_reg((0xbc301410  +0x80),0x026B0256);
  write_reg((0xbc301410  +0xa0),0x425A0257);
*/

// 640x480@59.9hz    25.18Mhz by wangchao.
#if defined(X640x480)
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x03200280); 640x480\n");
  write_reg((0xbc301410  +0x00),0x03200280);
  printf(" write_reg((0xbc301410  +0x20),0x42F00290);\n");
  write_reg((0xbc301410  +0x20),0x42F00290);
  printf(" write_reg((0xbc301410  +0x80),0x020D01E0);\n");
  write_reg((0xbc301410  +0x80),0x020D01E0);
  printf(" write_reg((0xbc301410  +0xa0),0x41EC01EA);\n");
  write_reg((0xbc301410  +0xa0),0x41EC01EA);
  #endif
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x03200280); 640x480\n");
  write_reg((0xbc301400  +0x00),0x03200280);
  printf(" write_reg((0xbc301400  +0x20),0x42F00290);\n");
  write_reg((0xbc301400  +0x20),0x42F00290);
  printf(" write_reg((0xbc301400  +0x80),0x020D01E0);\n");
  write_reg((0xbc301400  +0x80),0x020D01E0);
  printf(" write_reg((0xbc301400  +0xa0),0x41EC01EA);\n");
  write_reg((0xbc301400  +0xa0),0x41EC01EA);
  #endif
#elif defined(X640x640)
  printf(" write_reg((0xbc301410  +0x00),0x03400280); 640x640\n");
  write_reg((0xbc301410  +0x00),0x03400280);
  printf(" write_reg((0xbc301410  +0x20),0x42E00290);\n");
  write_reg((0xbc301410  +0x20),0x42E00290);
  printf(" write_reg((0xbc301410  +0x80),0x02970280);\n");
  write_reg((0xbc301410  +0x80),0x02970280);
  printf(" write_reg((0xbc301410  +0xa0),0x42840281);\n");
  write_reg((0xbc301410  +0xa0),0x42840281);
#elif defined(X640x768)
  printf(" write_reg((0xbc301410  +0x00),0x03400280); 640x768\n");
  write_reg((0xbc301410  +0x00),0x03400280);
  printf(" write_reg((0xbc301410  +0x20),0x42E00290);\n");
  write_reg((0xbc301410  +0x20),0x42E00290);
  printf(" write_reg((0xbc301410  +0x80),0x02970300);\n");
  write_reg((0xbc301410  +0x80),0x031B0300);
  printf(" write_reg((0xbc301410  +0xa0),0x43040301);\n");
  write_reg((0xbc301410  +0xa0),0x43040301);
#elif defined(X640x800)
  printf(" write_reg((0xbc301410  +0x00),0x03500280); 640x800\n");
  write_reg((0xbc301410  +0x00),0x03500280);
  printf(" write_reg((0xbc301410  +0x20),0x42E802A8);\n");
  write_reg((0xbc301410  +0x20),0x42E802A8);
  printf(" write_reg((0xbc301410  +0x80),0x033C0320);\n");
  write_reg((0xbc301410  +0x80),0x033C0320);
  printf(" write_reg((0xbc301410  +0xa0),0x43240321);\n");
  write_reg((0xbc301410  +0xa0),0x43240321);
#elif defined(X800x480)  //800x480
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x03E00320); 800x480\n");
  write_reg((0xbc301400  +0x00),0x03E00320);
  printf(" write_reg((0xbc301400  +0x20),0x43800330);\n");
  write_reg((0xbc301400  +0x20),0x43800330);
  printf(" write_reg((0xbc301400  +0x80),0x01F101E0);\n");
  write_reg((0xbc301400  +0x80),0x01F101E0);
  printf(" write_reg((0xbc301400  +0xa0),0x41E401E1);\n");
  write_reg((0xbc301400  +0xa0),0x41E401E1);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x03E00320); 800x480\n");
  write_reg((0xbc301410  +0x00),0x03E00320);
  printf(" write_reg((0xbc301410  +0x20),0x43800330);\n");
  write_reg((0xbc301410  +0x20),0x43800330);
  printf(" write_reg((0xbc301410  +0x80),0x01F101E0);\n");
  write_reg((0xbc301410  +0x80),0x01F101E0);
  printf(" write_reg((0xbc301410  +0xa0),0x41E401E1);\n");
  write_reg((0xbc301410  +0xa0),0x41E401E1);
  #endif
#elif defined(X800x600)  //1024x768
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x04000320); 800x600\n");
  write_reg((0xbc301400  +0x00),0x04000320);
  printf(" write_reg((0xbc301400  +0x20),0x43800338);\n");
  write_reg((0xbc301400  +0x20),0x43800338);
  printf(" write_reg((0xbc301400  +0x80),0x02710258);\n");
  write_reg((0xbc301400  +0x80),0x02710258);
  printf(" write_reg((0xbc301400  +0xa0),0x425B0259);\n");
  write_reg((0xbc301400  +0xa0),0x425B0259);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x04000320); 800x600\n");
  write_reg((0xbc301410  +0x00),0x04000320);
  printf(" write_reg((0xbc301410  +0x20),0x43800338);\n");
  write_reg((0xbc301410  +0x20),0x43800338);
  printf(" write_reg((0xbc301410  +0x80),0x02710258);\n");
  write_reg((0xbc301410  +0x80),0x02710258);
  printf(" write_reg((0xbc301410  +0xa0),0x425B0259);\n");
  write_reg((0xbc301410  +0xa0),0x425B0259);
  #endif
#elif defined(X800x640)  //800x640
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x04000320); 800x640\n");
  write_reg((0xbc301400  +0x00),0x04000320);
  printf(" write_reg((0xbc301400  +0x20),0x43900340);\n");
  write_reg((0xbc301400  +0x20),0x43900340);
  printf(" write_reg((0xbc301400  +0x80),0x02970280);\n");
  write_reg((0xbc301400  +0x80),0x02970280);
  printf(" write_reg((0xbc301400  +0xa0),0x42840281);\n");
  write_reg((0xbc301400  +0xa0),0x42840281);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x04000320); 800x640\n");
  write_reg((0xbc301410  +0x00),0x04000320);
  printf(" write_reg((0xbc301410  +0x20),0x43900340);\n");
  write_reg((0xbc301410  +0x20),0x43900340);
  printf(" write_reg((0xbc301410  +0x80),0x02970280);\n");
  write_reg((0xbc301410  +0x80),0x02970280);
  printf(" write_reg((0xbc301410  +0xa0),0x42840281);\n");
  write_reg((0xbc301410  +0xa0),0x42840281);
  #endif
#elif defined(X832x600)  //832x600
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x04300340); 832x600\n");
  write_reg((0xbc301400  +0x00),0x04300340);
  printf(" write_reg((0xbc301400  +0x20),0x43B80360);\n");
  write_reg((0xbc301400  +0x20),0x43B80360);
  printf(" write_reg((0xbc301400  +0x80),0x026E0258);\n");
  write_reg((0xbc301400  +0x80),0x026E0258);
  printf(" write_reg((0xbc301400  +0xa0),0x425B0259);\n");
  write_reg((0xbc301400  +0xa0),0x425C0259);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x04300340); 832x600\n");
  write_reg((0xbc301410  +0x00),0x04300340);
  printf(" write_reg((0xbc301410  +0x20),0x43B80360);\n");
  write_reg((0xbc301410  +0x20),0x43B80360);
  printf(" write_reg((0xbc301410  +0x80),0x026E0258);\n");
  write_reg((0xbc301410  +0x80),0x026E0258);
  printf(" write_reg((0xbc301410  +0xa0),0x425B0259);\n");
  write_reg((0xbc301410  +0xa0),0x425C0259);
  #endif
#elif defined(X832x608)  //832x600
  printf(" write_reg((0xbc301410  +0x00),0x04300340); 832x608\n");
  write_reg((0xbc301410  +0x00),0x04300340);
  printf(" write_reg((0xbc301410  +0x20),0x43B80360);\n");
  write_reg((0xbc301410  +0x20),0x43B80360);
  printf(" write_reg((0xbc301410  +0x80),0x02760260);\n");
  write_reg((0xbc301410  +0x80),0x02760258);
  printf(" write_reg((0xbc301410  +0xa0),0x42640261);\n");
  write_reg((0xbc301410  +0xa0),0x42640261);
#elif defined(X1024x480)  //1024x480
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x05000400); 1024x480\n");
  write_reg((0xbc301400  +0x00),0x05000400);
  printf(" write_reg((0xbc301400  +0x20),0x44800418);\n");
  write_reg((0xbc301400  +0x20),0x44800418);
  printf(" write_reg((0xbc301400  +0x80),0x01F101E0);\n");
  write_reg((0xbc301400  +0x80),0x01F101E0);
  printf(" write_reg((0xbc301400  +0xa0),0x41E401E1);\n");
  write_reg((0xbc301400  +0xa0),0x41E401E1);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x05000400); 1024x480\n");
  write_reg((0xbc301410  +0x00),0x05000400);
  printf(" write_reg((0xbc301410  +0x20),0x44800418);\n");
  write_reg((0xbc301410  +0x20),0x44800418);
  printf(" write_reg((0xbc301410  +0x80),0x01F101E0);\n");
  write_reg((0xbc301410  +0x80),0x01F101E0);
  printf(" write_reg((0xbc301410  +0xa0),0x41E401E1);\n");
  write_reg((0xbc301410  +0xa0),0x41E401E1);
  #endif

#elif defined(X1024x600)  //1024x600
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x05200400); 1024x600\n");
  write_reg((0xbc301400  +0x00),0x05200400);
  printf(" write_reg((0xbc301400  +0x20),0x49000428);\n");
  write_reg((0xbc301400  +0x20),0x49000428);
  printf(" write_reg((0xbc301400  +0x80),0x026E0258);\n");
  write_reg((0xbc301400  +0x80),0x026E0258);
  printf(" write_reg((0xbc301400  +0xa0),0x425C0259);\n");
  write_reg((0xbc301400  +0xa0),0x425C0259);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x05200400); 1024x600\n");
  write_reg((0xbc301410  +0x00),0x05200400);
  printf(" write_reg((0xbc301410  +0x20),0x49000428);\n");
  write_reg((0xbc301410  +0x20),0x49000428);
  printf(" write_reg((0xbc301410  +0x80),0x026E0258);\n");
  write_reg((0xbc301410  +0x80),0x026E0258);
  printf(" write_reg((0xbc301410  +0xa0),0x425C0259);\n");
  write_reg((0xbc301410  +0xa0),0x425C0259);
  #endif
#elif defined(X1024x640)  //1024x600
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x05300400); 1024x640\n");
  write_reg((0xbc301400  +0x00),0x05300400);
  printf(" write_reg((0xbc301400  +0x20),0x49800430);\n");
  write_reg((0xbc301400  +0x20),0x49800430);
  printf(" write_reg((0xbc301400  +0x80),0x02970280);\n");
  write_reg((0xbc301400  +0x80),0x02970280);
  printf(" write_reg((0xbc301400  +0xa0),0x42840281);\n");
  write_reg((0xbc301400  +0xa0),0x42840281);
  #endif
  #ifdef DC_FB1
  printf(" write_reg((0xbc301410  +0x00),0x05300400); 1024x640\n");
  write_reg((0xbc301410  +0x00),0x05300400);
  printf(" write_reg((0xbc301410  +0x20),0x49800430);\n");
  write_reg((0xbc301410  +0x20),0x49800430);
  printf(" write_reg((0xbc301410  +0x80),0x02970280);\n");
  write_reg((0xbc301410  +0x80),0x02970280);
  printf(" write_reg((0xbc301410  +0xa0),0x42840281);\n");
  write_reg((0xbc301410  +0xa0),0x42840281);
  #endif
#elif defined(X1024x768)  //1024x768
#ifdef DC_FB1
  printf(" write_reg((0xbc301400  +0x00),0x04D00400); 1024x768\n");
  write_reg((0xbc301410  +0x00),0x04D00400);
  printf(" write_reg((0xbc301400  +0x20),0x44680408);\n");
  write_reg((0xbc301410  +0x20),0x44680408);
  printf(" write_reg((0xbc301400  +0x80),0x030E0300);\n");
  write_reg((0xbc301410  +0x80),0x030E0300);
  printf(" write_reg((0xbc301400  +0xa0),0x43040301);\n");
  write_reg((0xbc301410  +0xa0),0x43040301);
#endif
#ifdef DC_FB0
  printf(" write_reg((0xbc301410  +0x00),0x05400400); 1024x768\n");
  write_reg((0xbc301410  +0x00),0x05400400);
  printf(" write_reg((0xbc301410  +0x20),0x44A00438);\n");
  write_reg((0xbc301410  +0x20),0x44A00438);
  printf(" write_reg((0xbc301410  +0x80),0x031B0300);\n");
  write_reg((0xbc301410  +0x80),0x031B0300);
  printf(" write_reg((0xbc301410  +0xa0),0x43040301);\n");
  write_reg((0xbc301410  +0xa0),0x43040301);
#endif
#endif
/*
  printf(" write_reg((0xbc301520  +0x00),0x00020202);\n");
  write_reg((0xbc301520  +0x00),0x00020202);
  printf(" write_reg((0xbc301530  +0x00),MEM_ADDR  );\n");
  write_reg((0xbc301530  +0x00),MEM_ADDR  );
  printf(" write_reg((0xbc301540  +0x00),0x00020002);\n");
  write_reg((0xbc301540  +0x00),0x00020002);
  printf(" write_reg((0xbc301550  +0x00),0x00eeeeee);\n");
  write_reg((0xbc301550  +0x00),0x00eeeeee);
  printf(" write_reg((0xbc301560  +0x00),0x00aaaaaa);\n");
  write_reg((0xbc301560  +0x00),0x00aaaaaa);
*/
#ifdef DC_FB0
  printf(" write_reg(0xbc301580,0x%x  );\n",MEM_ADDR);
  write_reg(0xbc301580,MEM_ADDR );
#if defined(CONFIG_VIDEO_32BPP)
  printf(" write_reg((DC_BASE_ADDR+0x00),0x00100104); X8R8G8B8 \n");
  write_reg((DC_BASE_ADDR+0x00),0x00100104);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x768\n");
  write_reg((DC_BASE_ADDR+0x40),0x00001000); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00001000); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00001000); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00001000); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000D00); 832x608 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000D00); //832
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000D00); 832x600 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000D00); //832
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000D00); 800x600 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000D00); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000C80); 800x640 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000C80); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000D00); 800x600 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000D00); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000C80); 800x600 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000C80); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000C00); 800x600 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000D00); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000C80); 800x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000C80); //640
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000A00); 640x800 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000A00); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000A00); 640x768 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000A00); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000A00); 640x640 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000A00); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000A00); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000A00); //640
#else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000A00); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000A00); //640
#endif //1024x768

#elif defined(CONFIG_VIDEO_16BPP)
  printf(" write_reg((DC_BASE_ADDR+0x00),0x00100103); R5G6B5 \n");
  write_reg((DC_BASE_ADDR+0x00),0x00100103);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x768\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000680); 832x608\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000680); //800
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000680); 832x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000680); //832
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x800\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x768\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#endif  //1024x768

#elif defined(CONFIG_VIDEO_15BPP)
  printf(" write_reg((DC_BASE_ADDR+0x00),0x00100102); R5G5B5\n");
  write_reg((DC_BASE_ADDR+0x00),0x00100102);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800);1024x768\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000680); 832x608\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000680); //832
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000680); 832x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000680); //800
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x640 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x800\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x768\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#endif  //1024x768

#elif defined(CONFIG_VIDEO_12BPP)
  printf(" write_reg((DC_BASE_ADDR+0x00),0x00100101); R4G4B4\n");
  write_reg((DC_BASE_ADDR+0x00),0x00100101);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x768 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000800); 1024x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000800); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000680); 832x608\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000680); //800
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000680); 832x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000680); //800
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x640 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000640); 800x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000640); //800
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x800\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x768\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x640\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#else
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR+0x40),0x00000500); //640
#endif  //1024x768

#else  //640x480-32Bits
  printf(" write_reg((DC_BASE_ADDR+0x00),0x00100104);\n");
  write_reg((DC_BASE_ADDR+0x00),0x00100104);
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00000A00);\n");
  write_reg((DC_BASE_ADDR+0x40),0x00000A00); //640
#endif //32Bits

#endif  //DC_FB0



#ifdef DC_FB1
  printf(" write_reg(0xbc301590,0x%x  );\n",MEM_ADDR_1);
  write_reg(0xbc301590,MEM_ADDR_1 );

#if defined(CONFIG_VIDEO_32BPP)
  printf(" write_reg((DC_BASE_ADDR_1+0x00),0x00100104); X8R8G8B8 \n");
  write_reg((DC_BASE_ADDR_1+0x00),0x00100104);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x768\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00001000); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00001000); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00001000); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00001000); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); 832x608 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); //832
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); 832x600 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); //832
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000C00); 800x600 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000C80); 800x640 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000C80); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000C00); 800x600 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000C80); 800x600 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000C80); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000C00); 800x600 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000D00); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000C80); 800x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000C80); //640
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); 640x800 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); 640x768 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); 640x640 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
#else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
#endif //1024x768

#elif defined(CONFIG_VIDEO_16BPP)
  printf(" write_reg((DC_BASE_ADDR_1+0x00),0x00100103); R5G6B5 \n");
  write_reg((DC_BASE_ADDR_1+0x00),0x00100103);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x768\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x480\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000680); 832x608\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000680); //800
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000680); 832x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000680); //832
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x480\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x800\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x768\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x480\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#endif  //1024x768

#elif defined(CONFIG_VIDEO_15BPP)
  printf(" write_reg((DC_BASE_ADDR_1+0x00),0x00100102); R5G5B5\n");
  write_reg((DC_BASE_ADDR_1+0x00),0x00100102);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800);1024x768\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000680); 832x608\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000680); //832
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000680); 832x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000680); //800
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x640 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x800\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x768\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#endif  //1024x768

#elif defined(CONFIG_VIDEO_12BPP)
  printf(" write_reg((DC_BASE_ADDR_1+0x00),0x00100101); R4G4B4\n");
  write_reg((DC_BASE_ADDR_1+0x00),0x00100101);

#if defined(X1024x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x768 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000800); 1024x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X1024x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000800); //1024
#elif defined(X832x608)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000680); 832x608\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000680); //800
#elif defined(X832x600)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000680); 832x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000680); //800
#elif defined(X800x640)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x640 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X800x600)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X800x480)
  #ifdef TEST_800x600
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000600); 800x600\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000700); //800
  #else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000640); 800x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000640); //800
  #endif
#elif defined(X640x800)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x800\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x768)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x768\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x640)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x640\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#elif defined(X640x480)
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#else
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000500); 640x480 \n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000500); //640
#endif  //1024x768

#else  //640x480-32Bits
  printf(" write_reg((DC_BASE_ADDR_1+0x00),0x00100104);\n");
  write_reg((DC_BASE_ADDR_1+0x00),0x00100104);
  printf(" write_reg((DC_BASE_ADDR_1+0x40),0x00000A00);\n");
  write_reg((DC_BASE_ADDR_1+0x40),0x00000A00); //640
#endif //32Bits

#endif

#if 0 //double fb test
  #ifdef DC_FB0
  printf(" write_reg((0xbc301400  +0x00),0x05000400); DVI FB0 1024x480\n");
  write_reg((0xbc301400  +0x00),0x05000400);
  printf(" write_reg((0xbc301400  +0x20),0x44800418);\n");
  write_reg((0xbc301400  +0x20),0x44800418);
  printf(" write_reg((0xbc301400  +0x80),0x01F101E0);\n");
  write_reg((0xbc301400  +0x80),0x01F101E0);
  printf(" write_reg((0xbc301400  +0xa0),0x41E401E1);\n");
  write_reg((0xbc301400  +0xa0),0x41E401E1);
  
  printf(" write_reg((DC_BASE_ADDR+0x00),0x00100104); X8R8G8B8 \n");
  write_reg((DC_BASE_ADDR+0x00),0x00100104);
  printf(" write_reg((DC_BASE_ADDR+0x40),0x00001000); 1024x480\n");
  write_reg((DC_BASE_ADDR+0x40),0x00001000); //1024
  #endif
#endif
printf("display controller reg config complete!\n");

  printf("read reg addr begin...\n");

  printf("read panel 0 reg:\n");
  print_data = readl((DC_BASE_ADDR+0x00));
  printf("read first data successfully!\n");
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x00),print_data);
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x00),(readl((DC_BASE_ADDR+0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x20),(readl((DC_BASE_ADDR+0x20))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x40),(readl((DC_BASE_ADDR+0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x00),(readl((0xbc301360  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x20),(readl((0xbc301360  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x40),(readl((0xbc301360  +0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x60),(readl((0xbc301360  +0x60))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301360  +0x80),(readl((0xbc301360  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0x00),(readl((0xbc301400  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0x20),(readl((0xbc301400  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0x80),(readl((0xbc301400  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301400  +0xa0),(readl((0xbc301400  +0xa0))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301520  +0x00),(readl((0xbc301520  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301530  +0x00),(readl((0xbc301530  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301540  +0x00),(readl((0xbc301540  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301550  +0x00),(readl((0xbc301550  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301560  +0x00),(readl((0xbc301560  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR+0x00),(readl((DC_BASE_ADDR+0x00))));

  printf("\n\nread panel 1 reg:\n");

  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x00),(readl((DC_BASE_ADDR_1+0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x20),(readl((DC_BASE_ADDR_1+0x20))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x40),(readl((DC_BASE_ADDR_1+0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x00),(readl((0xbc301370  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x20),(readl((0xbc301370  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x40),(readl((0xbc301370  +0x40))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x60),(readl((0xbc301370  +0x60))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301370  +0x80),(readl((0xbc301370  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0x00),(readl((0xbc301410  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0x20),(readl((0xbc301410  +0x20))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0x80),(readl((0xbc301410  +0x80))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301410  +0xa0),(readl((0xbc301410  +0xa0))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301520  +0x00),(readl((0xbc301520  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301530  +0x00),(readl((0xbc301530  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301540  +0x00),(readl((0xbc301540  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301550  +0x00),(readl((0xbc301550  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(0xbc301560  +0x00),(readl((0xbc301560  +0x00))));
  printf("reg addr:%x,reg data:%x\n",(DC_BASE_ADDR_1+0x00),(readl((DC_BASE_ADDR_1+0x00))));
  printf("========reg addr:%x,reg data:%x\n",0xbc301590,readl(0xbc301590));
    #ifdef DC_FB1
    return MEM_ptr_1;
    #endif
   
   return MEM_ptr;
    
}

#if 1

int test_dc_line1(int argc ,char **argv)
{

   int i,ii,tmp;
   int PIXEL_COUNT = DIS_WIDTH * DIS_HEIGHT +  EXTRA_PIXEL; 
   int MEM_SIZE; // = PIXEL_COUNT * 4;
   int line_length = 0;
   int line_length2 = 0;
   int TEST_WIDTH=0;

   if(argc !=2 )
   {
        TEST_WIDTH = 8;
        printf("test width : %d \n",TEST_WIDTH);
   }
   else
   {
        TEST_WIDTH = strtoul(argv[1],0,0);
        printf("test width : %d \n",TEST_WIDTH);    
   }

 #if defined(CONFIG_VIDEO_32BPP)
        MEM_SIZE = PIXEL_COUNT * 4 ;
        line_length2 = DIS_WIDTH * 8;
        line_length = TEST_WIDTH * 4;
        //for (i=MEM_SIZE;i< (MEM_SIZE+0xC000);i+=line_length)
        for (i=0;i< MEM_SIZE; i += line_length2)
        {
                if((i/line_length)%2 ==1)
                {
                    for(ii=0;ii<line_length; ii+=4)
                        *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00000000;
                }
                else 
                {
                    if((i/line_length)%RANDOM_HEIGHT_Z ==2)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x0000ff00;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==4)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00ffff00;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==6)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00ff00ff;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==8)
                    { 
                        for(ii=0x0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00808080;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==10)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x005a5a5a;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==12)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00333333;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==14)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00cccccc;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==16)
                    { 
                        for(ii=0x0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00654321;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==RANDOM_HEIGHT_Z)
                    { 
                        for(ii=0x0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00cc2288;
                    }
                }
                    if((i/line_length)%RANDOM_HEIGHT_Z ==15)
                    {
                        for(ii=0;ii<line_length; ii+=4)
                            *(volatile unsigned int *)(MEM_ptr + ii + i) = 0x00ffffff;
                    }
            }


#elif defined(CONFIG_VIDEO_16BPP)
        MEM_SIZE = PIXEL_COUNT * 2 ;
        line_length2 = DIS_WIDTH * 4;
        line_length = TEST_WIDTH * 2;
        //for (i=MEM_SIZE;i< (MEM_SIZE+0xC000);i+=0xA00)
        for (i=0;i< MEM_SIZE; i += line_length2)
        {
                if((i/line_length) % 2 ==1)
                {
                    // tmp = i + line_length - ( i%line_length );
                    for(ii=0;ii< line_length ; ii+=2)
                        *(volatile unsigned short *)(MEM_ptr + ii + i) = 0x0000;
                }
                else 
                {
                    if((i/line_length)%RANDOM_HEIGHT_Z ==2)
                    {
                 //   tmp = i + line_length - ( i % line_length );
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0x07e0;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==4)
                    {
                  //  tmp = i + line_length - ( i%line_length );
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0xffe0;
                    }
                    #if 0
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==6)
                    {
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0xf81f;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==8)
                    { 
                        for(ii=0x0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0x8010;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==10)
                    {
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0x5a5a;
                    }
                    #endif
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==12)
                    {
                    //tmp = i + line_length - ( i%line_length );
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0x3333;
                    }
                    
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==14)
                    {
                   // tmp = i + line_length - ( i%line_length );
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0xf800;  // 0xcccc;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z ==16)
                    { 
                        for(ii=0x0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0xffff;
                    }
                    else if((i/line_length)%RANDOM_HEIGHT_Z == (RANDOM_HEIGHT_Z-1))
                    { 
                   // tmp = i + line_length - ( i%line_length );
                        for(ii=0x0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0x001f; //  0x2288;
                    }
                }
                #if 0
                    if((i/line_length)%RANDOM_HEIGHT_Z ==15)
                    {
                   // tmp = i + line_length - ( i%line_length );
                        for(ii=0;ii<line_length; ii+=2)
                            *(volatile unsigned short *)(MEM_ptr + ii + i) = 0xffff;
                    }
                #endif
            }
#endif

}

#include <pmon.h>

static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"dc_init", "", 0, "GC300 DC init", dc_init, 0, 99, CMD_REPEAT},
    {"dc_test_dp", "", 0, "GC300 DC test draw point", test_dc_line1, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}
#endif
