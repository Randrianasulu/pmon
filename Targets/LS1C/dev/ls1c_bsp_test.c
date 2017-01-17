#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>

#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <flash.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/net/if.h>
#include "mod_vgacon.h"
#include "mod_display.h"

#include <pflash.h>

#define CONFIG_PAGE_SIZE_64KB
//#include "mipsregs.h"
#include "gzip.h"
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */


#include <termio.h>
#include <machine/pio.h>
#include <machine/bus.h>
#include <pmon/dev/ns16550.h>




#if 1
typedef char _s8;
typedef unsigned char _u8;

typedef short _s16;
typedef unsigned short _u16;

typedef int _s32;
typedef unsigned int _u32;

typedef float _fp32;
#endif

#define    regrl(addr) ((*(volatile unsigned int *)(addr)))
#define    regwl(addr,val) ((*(volatile unsigned int *)(addr))=(val))

#define WTREG4(addr, val)	(*(volatile _u32 *)(addr) = (val))
#define WTREG1(addr, val)	(*(volatile _u8 *)(addr) = (val))
#define RDREG4(addr, val)	((val) = *(volatile _u32 *)(addr))
#define RDREG1(addr, val)	((val) = *(volatile _u8 *)(addr))



//PWM0 	 				
#define CNTR0 	0xbfe5c000 	//R/W 	主计数器 	0x0
#define HRC0 	0xbfe5c004 	//R/W 	高脉冲定时参考寄存器 	0x0
#define LRC0 	0xbfe5c008 	//R/W 	低脉冲定时参考寄存器 	0x0
#define CTRL0 	0xbfe5c00c 	//R/W 	控制寄存器 	0x0
//PWM1 	 				
#define CNTR1 	0xbfe5c010 	//R/W 	主计数器 	0x0
#define HRC1 	0xbfe5c014 	//R/W 	高脉冲定时参考寄存器 	0x0
#define LRC1 	0xbfe5c018 	//R/W 	低脉冲定时参考寄存器 	0x0
#define CTRL1 	0xbfe5c01c 	//R/W 	控制寄存器 	0x0


//GPIO寄存器 	 				
#define GPIO_CFG0 	0xbfd010c0 	//R/W 	GPIO配置寄存器0 	0xffe00040
#define GPIO_CFG1 	0xbfd010c4 	//R/W 	GPIO配置寄存器1 	0xfc00_3f3f
#define GPIO_CFG2 	0xbfd010c8 	//R/W 	GPIO配置寄存器2 	0x1ff0_3fff
#define GPIO_CFG3 	0xbfd010cc 	//R/W 	GPIO配置寄存器3 	0x0
#define CAN0_BASEADDR 0xbfe50000
#define CAN1_BASEADDR 0xbfe54000

/*mux 寄存器*/
#define MUX_BASE1	0xbfd011c0
#define MUX_BASE2	0xbfd011d0
#define MUX_BASE3	0xbfd011e0
#define MUX_BASE4	0xbfd011f0
#define MUX_BASE5	0xbfd01200

/***************************************************************/
void delay_100ms(int i)
{
	int j;
	for ( ; i>0; i--)
		for( j=0; j<8000000; j++);
}

/***************************************************************************
 * Description: get_clock()
 * Parameters: 
 * Author  :Sunyoung_yg 
 * Date    : 2015-11-15
 ***************************************************************************/
static unsigned int md_pipefreq, md_cpufreq;

static void get_clock()
{

	unsigned int val = *(volatile unsigned int *)0xbfe78030;
	unsigned int cpu_div = *(volatile unsigned int*)0xbfe78034;

	cpu_div = (cpu_div >> 8) & 0x3f;
	md_pipefreq = ((val >> 8) & 0xff) * 6 / cpu_div * 1000000 ;
	val = (1 << ((val & 0x3) + 1)) % 5;
	md_cpufreq  = md_pipefreq / val;
}
/***************************************************************/

static void uart_put_char(unsigned int uart_base, char c)
{
	unsigned char val;
	val = *(volatile unsigned char *)(uart_base + 0x5);
	while( !(val & 0x20)){
		val = *(volatile unsigned char *)(uart_base + 0x5);
	}
	*(volatile unsigned char *)(uart_base + 0x0) = c;
}


/***************************************************************************
 * Description:
 * Parameters: 
 * Author    :Sunyoung_yg 
 * Date      : 2014-06-03
 ***************************************************************************/
 void hcntr_test(void)
{
		
}






/***************************************************************************
 * Description: 接收TestPort发来的数据，并发回去,当TestPort 发送ctrl+c (0x3)
 * 时,pmon 终止测试，返回命令行。
 * Parameters:
 * Author  :Sunyoung_yg 
 * Date    : 2016-04-29
 ***************************************************************************/
 char uart_echochar(DevEntry *p)
{
//	volatile ns16550dep *dp;
	int i=0, j=0;
	char  buf[100];
	bzero(buf, 100);
	int c;
	while (ns16550(OP_RXRDY, p, NULL, NULL)) {
		buf[i++] = ns16550(OP_RX, p, NULL, NULL);
		delay(100);
	}
	buf[i] = '\0';

	if(i){
		if(buf[0] == 0x3) return 1;  /* ctrl c*/
		printf("%s", buf);
	//	while( i>0 && ns16550(OP_TXRDY, p, NULL, NULL)){
		while(i--){
			ns16550(OP_TX, p, NULL, buf[j++]);
			delay(100);
		}
	}
	return 0;
}




/***************************************************************************
 * Description:
 * Parameters: 
 * Author  :Sunyoung_yg 
 * Date    : 2016-04-29
 ***************************************************************************/
void uart_config_gpio(int channel)
{
	unsigned int val,
				 mux_base1=0xbfd011c0,
				 mux_base2=0xbfd011d0,
				 mux_base3=0xbfd011e0,
				 mux_base4=0xbfd011f0,
				 mux_base5=0xbfd01200;

	//gpio mux init
	switch(channel)
	{
		case 0: 
		//uart 0   (gpio74/75 mux2)
			val = *(volatile unsigned int *)(mux_base1+0x8);
			val &= ~(0x3<<(74-64));
			*(volatile unsigned int *)(mux_base1+0x8) = val;

			val = *(volatile unsigned int *)(mux_base2+0x8);
			val |= (0x3<<(74-64));
			*(volatile unsigned int *)(mux_base2+0x8) = val;

			val = *(volatile unsigned int *)(mux_base3+0x8);
			val &= ~(0x3<<(74-64));
			*(volatile unsigned int *)(mux_base3+0x8) = val;

			val = *(volatile unsigned int *)(mux_base4+0x8);
			val &= ~(0x3<<(74-64));
			*(volatile unsigned int *)(mux_base4+0x8) = val;

			val = *(volatile unsigned int *)(mux_base5+0x8);
			val &= ~(0x3<<(74-64));
			*(volatile unsigned int *)(mux_base5+0x8) = val;
			break;
		case 1:  
#if EJTAG_TO_GPIO
			//uart 1   (gpio 02/03 mux4)
			val = *(volatile unsigned int *)(mux_base1);
			val &= ~(0x3<<(2));
			*(volatile unsigned int *)(mux_base1) = val;
			
			val = *(volatile unsigned int *)(mux_base2);
			val &= ~(0x3<<(2));
			*(volatile unsigned int *)(mux_base2) = val;

			val = *(volatile unsigned int *)(mux_base3);
			val &= ~(0x3<<(2));
			*(volatile unsigned int *)(mux_base3) = val;

			val = *(volatile unsigned int *)(mux_base4);
			val |= (0x3<<(2));
			*(volatile unsigned int *)(mux_base4) = val;
			
			val = *(volatile unsigned int *)(mux_base5);
			val &= ~(0x3<<(2));
			*(volatile unsigned int *)(mux_base5) = val;
#else
			//uart 1   (gpio76/77 mux2)
			val = *(volatile unsigned int *)(mux_base1+0x8);
			val &= ~(0x3<<(76-64));
			*(volatile unsigned int *)(mux_base1+0x8) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x8);
			val |= (0x3<<(76-64));
			*(volatile unsigned int *)(mux_base2+0x8) = val;

			val = *(volatile unsigned int *)(mux_base3+0x8);
			val &= ~(0x3<<(76-64));
			*(volatile unsigned int *)(mux_base3+0x8) = val;

			val = *(volatile unsigned int *)(mux_base4+0x8);
			val &= ~(0x3<<(76-64));
			*(volatile unsigned int *)(mux_base4+0x8) = val;

			val = *(volatile unsigned int *)(mux_base5+0x8);
			val &= ~(0x3<<(76-64));
			*(volatile unsigned int *)(mux_base5+0x8) = val;
#endif
			break;

		case 2:	

#if  EJTAG_TO_GPIO
			//uart2  (gpio 04,05 mux4)
			val = *(volatile unsigned int *)(mux_base1);
			val &= ~(0x3<<(4));
			*(volatile unsigned int *)(mux_base1) = val;
			
			val = *(volatile unsigned int *)(mux_base2);
			val &= ~(0x3<<(4));
			*(volatile unsigned int *)(mux_base2) = val;

			val = *(volatile unsigned int *)(mux_base3);
			val &= ~(0x3<<(4));
			*(volatile unsigned int *)(mux_base3) = val;

			val = *(volatile unsigned int *)(mux_base4);
			val |= (0x3<<(4));
			*(volatile unsigned int *)(mux_base4) = val;

			val = *(volatile unsigned int *)(mux_base5);
			val &= ~(0x3<<(4));
			*(volatile unsigned int *)(mux_base5) = val;
//			printf(" mux_4: 0x%08x ...\r\n",  *(volatile unsigned int *)(mux_base4);
#else
			//uart2  (gpio 36/37 mux1)
			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x3<<(36-32));
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x4);
			val |= (0x3<<(36-32));
			*(volatile unsigned int *)(mux_base2+0x4) = val;

			val = *(volatile unsigned int *)(mux_base3+0x4);
			val &= ~(0x3<<(36-32));
			*(volatile unsigned int *)(mux_base3+0x4) = val;

			val = *(volatile unsigned int *)(mux_base4+0x4);
			val &= ~(0x3<<(36-32));
			*(volatile unsigned int *)(mux_base4+0x4) = val;

			val = *(volatile unsigned int *)(mux_base5+0x4);
			val &= ~(0x3<<(36-32));
			*(volatile unsigned int *)(mux_base5+0x4) = val;
#endif

			break;
		case 3:
			printf(" ----------------->uart 1233\r\n");
#if 1 //EJTAG_TO_GPIO
			printf(" ejtag to gpio\r\n");
			//uart3  gpio 00,01 mux4
			val = *(volatile unsigned int *)(mux_base1);
			val &= ~(0x3);
			*(volatile unsigned int *)(mux_base1) = val;
			
			val = *(volatile unsigned int *)(mux_base2);
			val &= ~(0x3);
			*(volatile unsigned int *)(mux_base2) = val;

			val = *(volatile unsigned int *)(mux_base3);
			val &= ~(0x3);
			*(volatile unsigned int *)(mux_base3) = val;

			val = *(volatile unsigned int *)(mux_base4);
			val |= (0x3);
			*(volatile unsigned int *)(mux_base4) = val;

			val = *(volatile unsigned int *)(mux_base5);
			val &= ~(0x3);
			*(volatile unsigned int *)(mux_base5) = val;
#else 
			//uart3  gpio 33,34 ? mux2
			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x6);
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x4);
			val |= (0x6);
			*(volatile unsigned int *)(mux_base2+0x4) = val;

			val = *(volatile unsigned int *)(mux_base3+0x4);
			val &= ~(0x6);
			*(volatile unsigned int *)(mux_base3+0x4) = val;

			val = *(volatile unsigned int *)(mux_base4+0x4);
			val &= ~(0x6);
			*(volatile unsigned int *)(mux_base4+0x4) = val;

			val = *(volatile unsigned int *)(mux_base5+0x4);
			val &= ~(0x6);
			*(volatile unsigned int *)(mux_base5+0x4) = val;
#endif
			break;
#if 0
		case 4：
		case 5：
		case 6:
		//uart4,5,6 作为普通串口  uart4: gpio 58,59   uart5: gpio 60,61   uart6: gpio 62 63
			
			val = *(volatile unsigned int *)(0xbfd00420);
			val &= ~(0x1<<18);
			*(volatile unsigned int *)(0xbfd00420) = val;

			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x3<<(58+(channel-4)*2-32));
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			break;
#else
		case 4:
			//uart4 普通串口  
			val = *(volatile unsigned int *)(0xbfd00420);
			val &= ~(0x1<<18);
			*(volatile unsigned int *)(0xbfd00420) = val;

			//config  gpio58,59 第五复用
			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x3<<(58-32));
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x4);
			val &= ~(0x3<<(58-32));
			*(volatile unsigned int *)(mux_base2+0x4) = val;

			val = *(volatile unsigned int *)(mux_base3+0x4);
			val &= ~(0x3<<(58-32));
			*(volatile unsigned int *)(mux_base3+0x4) = val;

			val = *(volatile unsigned int *)(mux_base4+0x4);
			val &= ~(0x3<<(58-32));
			*(volatile unsigned int *)(mux_base4+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base5+0x4);
			val |= (0x3<<(58-32));
			*(volatile unsigned int *)(mux_base5+0x4) = val;
			
			break;
		case 5:
			//uart5 普通串口  
			val = *(volatile unsigned int *)(0xbfd00420);
			val &= ~(0x1<<18);
			*(volatile unsigned int *)(0xbfd00420) = val;
			//config  gpio 60,61 第五复用
			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x3<<(60-32));
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x4);
			val &= ~(0x3<<(60-32));
			*(volatile unsigned int *)(mux_base2+0x4) = val;

			val = *(volatile unsigned int *)(mux_base3+0x4);
			val &= ~(0x3<<(60-32));
			*(volatile unsigned int *)(mux_base3+0x4) = val;

			val = *(volatile unsigned int *)(mux_base4+0x4);
			val &= ~(0x3<<(60-32));
			*(volatile unsigned int *)(mux_base4+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base5+0x4);
			val |= (0x3<<(60-32));
			*(volatile unsigned int *)(mux_base5+0x4) = val;
			
			break;
		case 6:

			//uart6 普通串口  
			val = *(volatile unsigned int *)(0xbfd00420);
			val &= ~(0x1<<18);
			*(volatile unsigned int *)(0xbfd00420) = val;
#if 0
			//config  gpio 62,63 第五复用
			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x3<<(62-32));
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x4);
			val &= ~(0x3<<(62-32));
			*(volatile unsigned int *)(mux_base2+0x4) = val;

			val = *(volatile unsigned int *)(mux_base3+0x4);
			val &= ~(0x3<<(62-32));
			*(volatile unsigned int *)(mux_base3+0x4) = val;

			val = *(volatile unsigned int *)(mux_base4+0x4);
			val &= ~(0x3<<(62-32));
			*(volatile unsigned int *)(mux_base4+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base5+0x4);
			val |= (0x3<<(62-32));
			*(volatile unsigned int *)(mux_base5+0x4) = val;
#else
			//config  gpio 46,47 第五复用
			val = *(volatile unsigned int *)(mux_base1+0x4);
			val &= ~(0x3<<(46-32));
			*(volatile unsigned int *)(mux_base1+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x4);
			val &= ~(0x3<<(46-32));
			*(volatile unsigned int *)(mux_base2+0x4) = val;

			val = *(volatile unsigned int *)(mux_base3+0x4);
			val &= ~(0x3<<(46-32));
			*(volatile unsigned int *)(mux_base3+0x4) = val;

			val = *(volatile unsigned int *)(mux_base4+0x4);
			val &= ~(0x3<<(46-32));
			*(volatile unsigned int *)(mux_base4+0x4) = val;
			
			val = *(volatile unsigned int *)(mux_base5+0x4);
			val |= (0x3<<(46-32));
			*(volatile unsigned int *)(mux_base5+0x4) = val;
#endif
			break;
#endif
		case 7:
	    	/*	
			//uart7 普通串口  
			val = *(volatile unsigned int *)(0xbfd00420);
			val &= ~(0x1<<18);
			*(volatile unsigned int *)(0xbfd00420) = val;
		    */
#if 0				
			//config  gpio 64,65 第五复用
			val = *(volatile unsigned int *)(mux_base1+0x8);
			val &= ~(0x3<<(64-64));
			*(volatile unsigned int *)(mux_base1+0x8) = val;
			
			val = *(volatile unsigned int *)(mux_base2+0x8);
			val &= ~(0x3<<(64-64));
			*(volatile unsigned int *)(mux_base2+0x8) = val;

			val = *(volatile unsigned int *)(mux_base3+0x8);
			val &= ~(0x3<<(64-64));
			*(volatile unsigned int *)(mux_base3+0x8) = val;

			val = *(volatile unsigned int *)(mux_base4+0x8);
			val &= ~(0x3<<(64-64));
			*(volatile unsigned int *)(mux_base4+0x8) = val;
			
			val = *(volatile unsigned int *)(mux_base5+0x8);
			val |= (0x3<<(64-64));
			*(volatile unsigned int *)(mux_base5+0x8) = val;			
#elif 0
				//config  gpio 87,88 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(87-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(87-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(87-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(87-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(87-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;			
#else
				//config  gpio 56,57 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x4);
				val &= ~(0x3<<(56-32));
				*(volatile unsigned int *)(mux_base1+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x4);
				val &= ~(0x3<<(56-32));
				*(volatile unsigned int *)(mux_base2+0x4) = val;

				val = *(volatile unsigned int *)(mux_base3+0x4);
				val &= ~(0x3<<(56-32));
				*(volatile unsigned int *)(mux_base3+0x4) = val;

				val = *(volatile unsigned int *)(mux_base4+0x4);
				val &= ~(0x3<<(56-32));
				*(volatile unsigned int *)(mux_base4+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x4);
				val |= (0x3<<(56-32));
				*(volatile unsigned int *)(mux_base5+0x4) = val;			
#endif
				break;
			case 8:
		    		
				//uart8 普通串口  
				val = *(volatile unsigned int *)(0xbfe4c904);
				val &= ~(0x1);
				*(volatile unsigned int *)(0xbfe4c904) = val;
			    
#if 0			
				//config  gpio 66,67 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(66-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(66-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(66-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(66-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(66-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;
#elif 0
				//config  gpio 89,90 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(89-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(89-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(89-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(89-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(89-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;
#else
				//config  gpio 54,55 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x4);
				val &= ~(0x3<<(54-32));
				*(volatile unsigned int *)(mux_base1+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x4);
				val &= ~(0x3<<(54-32));
				*(volatile unsigned int *)(mux_base2+0x4) = val;

				val = *(volatile unsigned int *)(mux_base3+0x4);
				val &= ~(0x3<<(54-32));
				*(volatile unsigned int *)(mux_base3+0x4) = val;

				val = *(volatile unsigned int *)(mux_base4+0x4);
				val &= ~(0x3<<(54-32));
				*(volatile unsigned int *)(mux_base4+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x4);
				val |= (0x3<<(54-32));
				*(volatile unsigned int *)(mux_base5+0x4) = val;			
				
#endif
				break;
			case 9:
		    		
				//uart9 普通串口  
				val = *(volatile unsigned int *)(0xbfe4c904);
				val &= ~(0x1);
				*(volatile unsigned int *)(0xbfe4c904) = val;
			
#if 0
				//config  gpio 68,69 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(68-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(68-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(68-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(68-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(68-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;
#elif 0
				//config  gpio 85,86 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(85-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(85-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(85-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(85-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(85-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;
#else
	     		//config  gpio 52,53 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x4);
				val &= ~(0x3<<(52-32));
				*(volatile unsigned int *)(mux_base1+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x4);
				val &= ~(0x3<<(52-32));
				*(volatile unsigned int *)(mux_base2+0x4) = val;

				val = *(volatile unsigned int *)(mux_base3+0x4);
				val &= ~(0x3<<(52-32));
				*(volatile unsigned int *)(mux_base3+0x4) = val;

				val = *(volatile unsigned int *)(mux_base4+0x4);
				val &= ~(0x3<<(52-32));
				*(volatile unsigned int *)(mux_base4+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x4);
				val |= (0x3<<(52-32));
				*(volatile unsigned int *)(mux_base5+0x4) = val;			
				
#endif
				break;
			case 10:	

				//uart10 普通串口  
				val = *(volatile unsigned int *)(0xbfe4c904);
				val &= ~(0x1);
				*(volatile unsigned int *)(0xbfe4c904) = val;
#if 0			    
				//config  gpio 70,71 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(70-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(70-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(70-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(70-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(70-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;
#elif 0
				//config  gpio 50,51 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x4);
				val &= ~(0x3<<(50-32));
				*(volatile unsigned int *)(mux_base1+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x4);
				val &= ~(0x3<<(50-32));
				*(volatile unsigned int *)(mux_base2+0x4) = val;

				val = *(volatile unsigned int *)(mux_base3+0x4);
				val &= ~(0x3<<(50-32));
				*(volatile unsigned int *)(mux_base3+0x4) = val;

				val = *(volatile unsigned int *)(mux_base4+0x4);
				val &= ~(0x3<<(50-32));
				*(volatile unsigned int *)(mux_base4+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x4);
				val |= (0x3<<(50-32));
				*(volatile unsigned int *)(mux_base5+0x4) = val;			
#else
				//config uart10  gpio 82,84 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x5<<(82-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;

				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x5<<(82-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x5<<(82-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x5<<(82-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;

				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x5<<(82-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;			
#endif
				break;	
			case 11:	
				//uart11 普通串口  
				val = *(volatile unsigned int *)(0xbfe4c904);
				val &= ~(0x1);
				*(volatile unsigned int *)(0xbfe4c904) = val;
#if 0
				//config  gpio 72,73 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x8);
				val &= ~(0x3<<(72-64));
				*(volatile unsigned int *)(mux_base1+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x8);
				val &= ~(0x3<<(72-64));
				*(volatile unsigned int *)(mux_base2+0x8) = val;

				val = *(volatile unsigned int *)(mux_base3+0x8);
				val &= ~(0x3<<(72-64));
				*(volatile unsigned int *)(mux_base3+0x8) = val;

				val = *(volatile unsigned int *)(mux_base4+0x8);
				val &= ~(0x3<<(72-64));
				*(volatile unsigned int *)(mux_base4+0x8) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x8);
				val |= (0x3<<(72-64));
				*(volatile unsigned int *)(mux_base5+0x8) = val;
#else
				//config  gpio 48,49 第五复用
				val = *(volatile unsigned int *)(mux_base1+0x4);
				val &= ~(0x3<<(48-32));
				*(volatile unsigned int *)(mux_base1+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base2+0x4);
				val &= ~(0x3<<(48-32));
				*(volatile unsigned int *)(mux_base2+0x4) = val;

				val = *(volatile unsigned int *)(mux_base3+0x4);
				val &= ~(0x3<<(48-32));
				*(volatile unsigned int *)(mux_base3+0x4) = val;

				val = *(volatile unsigned int *)(mux_base4+0x4);
				val &= ~(0x3<<(48-32));
				*(volatile unsigned int *)(mux_base4+0x4) = val;
				
				val = *(volatile unsigned int *)(mux_base5+0x4);
				val |= (0x3<<(48-32));
				*(volatile unsigned int *)(mux_base5+0x4) = val;			
#endif
				break;	
			default:
				break;
	}
	return;
}

#define EJTAG_TO_GPIO 0
/***************************************************************/
static int uart_test(unsigned int channel)
{
	DevEntry *pTestPort, *pTermPort;
	unsigned int val, i, uart_base;

	uart_config_gpio(channel);

	pTestPort = (DevEntry *)malloc(sizeof(DevEntry));
	pTermPort = (DevEntry *)malloc(sizeof(DevEntry));

	memset(pTestPort, sizeof(DevEntry), 0);
	memset(pTermPort, sizeof(DevEntry), 0);
	pTermPort->sio = COM_BASE;

		//##################### uart control regs init
		char str[50]={0};
		switch(channel)
		{
			case 0 ... 3:
				pTestPort->sio = (0xbfe40000 + channel*0x4000);
				//p->freq = 
				uart_base = (0xbfe40000 + channel*0x4000);
				sprintf(str, "uart[%d] test ok!\r\n",channel);
				break;

			case 4 ... 11:
				pTestPort->sio = (0xbfe40000 + channel*0x100);
				uart_base = (0xbfe4c000 + channel*0x100);
				sprintf(str, "uart[%d] test ok!\r\n",channel);
				break;
			default:
				break;
		}
		//设置Baud rate
		//读出默认的串口三　　分频值
		unsigned char div0, div1, tmp;
		tmp = *(volatile unsigned char *)(0xbfe4c003);
		*(volatile unsigned char *)(0xbfe4c003) = (tmp | (0x1<<7));
		div0 = *(volatile unsigned char *)(0xbfe4c000);
		div1 = *(volatile unsigned char *)(0xbfe4c001);
		*(volatile unsigned char *)(0xbfe4c003) = tmp;

		*(volatile unsigned char *)(uart_base+0x2) = 0x7;
		*(volatile unsigned char *)(uart_base+0x3) = 0x80;

		*(volatile unsigned char *)(uart_base+0x0) = div0;
		*(volatile unsigned char *)(uart_base+0x1) = div1;
		*(volatile unsigned char *)(uart_base+0x3) = 0x3;
//		*(volatile unsigned char *)(uart_base+0x4) = 0x3;  //uart9
//		,不能配置此寄存器, bit0 置位后，uart9-11 为uart8的全功能引脚
		*(volatile unsigned char *)(uart_base+0x1) = 0x0;

		pTestPort->freq = md_cpufreq;
		ns16550(OP_BAUD, pTestPort, NULL, 115200);

		//########## print test character
		val = strlen(str);
		for(i=0; i<val; i++)
			uart_put_char(uart_base,str[i]);

		char line[100], ret=0;

		while(1)
		{
			/* receive from test uart */
			ret = uart_echochar(pTestPort);
			if(ret)
				return ret;
			scandevs();
		}
}


/***************************************************************************
 * Description:
 * Version : 1.00
 * Author  : Sunyoung 
 * Language: C
 * Date    : 2014-01-23
 ***************************************************************************/
#define sys_toywrite0 	0xbfe64024	//W 	TOY低32位数值写入
#define sys_toywrite1 	0xbfe64028	//W 	TOY高32位数值写入
#define sys_toyread0 	0xbfe6402C	//R 	TOY低32位数值读出
#define sys_toyread1 	0xbfe64030	//R 	TOY高32位数值读出

#define sys_rtcctrl 	0xbfe64040	//RW 	TOY和RTC控制寄存器
void rtc_test(void)
{
	_u32 val1, val2;
	_u32 tmp;
	tmp  =  ((0&0xf)<<0);     //msecond, width=4
	tmp |= ((59&0x3f)<<4);    //seconds, width=6
	tmp |= ((30&0x3f)<<10);   //minutes, width=6
	tmp |= ((10&0x1f)<<16);   //hours, width=5
	tmp |= ((30&0x1f)<<21);   //days, width=5
	tmp |= ((9&0x3f)<<26);    //month, width=6

	printf("enter rtc_test...\r\n");
	WTREG4(sys_toywrite0, tmp);

	printf("enter rtc_test...\r\n");
	WTREG4(sys_toywrite1, 13);
//	WTREG4(sys_rtcctrl, 0x0d00);

//	delay_100ms(20);
	int i;
	printf("year-month-day  hour:minute:second:msec\r\n");
	for ( i = 0; i < 12; i++){
		for(tmp=0; tmp<5; tmp++) {
			RDREG4(sys_toyread0, val1);
			RDREG4(sys_toyread1, val2);
		}

		printf(" 20%d-%02d-%02d   %02d:%02d:%02d.%d \r\n", val2, ((val1>>26)&0x3f),
		   ((val1>>21)&0x1f), ((val1>>16)&0x1f), ((val1>>10)&0x3f), ((val1>>4)&0x3f), ((val1>>0)&0xf));

		delay_100ms(10);
	}
}

void touch_test(void)
{
	
//	WTREG4(sys_toywrite0, tmp);

	unsigned int val, i;

	//gpio 06,92 first use  is ADC_XN,ADC_YN
	val = *(volatile unsigned int *)(0xbfd011c8);
	val |= (1 << (92 - 64));
	*(volatile unsigned int *)(0xbfd011c8) = val;



	val = *(volatile unsigned int *)(0xbfd011c0);
	val |= (1 << (6));
	*(volatile unsigned int *)(0xbfd011c0) = val;

	delay_100ms(1);
	delay_100ms(1);
	
	WTREG4(0xbfe74000, 0x20000010);
	WTREG4(0xbfe74008, 0xc0);
	WTREG4(0xbfe74004, 0x10);
	RDREG4(0xbfe7401c, val);
	
	delay_100ms(1);
	
	for(i=0; i<1000; i++)
	{
		RDREG4(0xbfe7401c, val);
	//	printf("  x:%d,  y:%d \r\n", (val&(0xffff<<16))>>16, val&0xffff);
	
		printf("  x:%d,  y:%d \r\n", val&0x3ff,(val>>16)&0x3ff);
		delay_100ms(1);
	}
}
/***************************************************************************
 * Description: test can, can0 send string, and can1 read it. 
 * Parameters: 
 * Author  :Sunyoung_yg 
 * Date    : 2014-12-16
 ***************************************************************************/
//----------------------------------
//
 #define raw_wl(x,y) (*(volatile unsigned char*)(x + 0xbfe50000) = y)
 #define raw_rl(x) (*(volatile unsigned char*)(x + 0xbfe50000))

void can_gpio_init(void)
{
	unsigned int val;
	//config  can  gpio  use the third function
	RDREG4(0xbfd011c4, val);
	val &= ~(0xf << (54-32));
	WTREG4(0xbfd011c4, val );	

	RDREG4(0xbfd011d4, val);
	val &= ~(0xf << (54-32));
	WTREG4(0xbfd011d4, val);	

	RDREG4(0xbfd011e4, val);
	val |= (0xf << (54-32));
//	val &= ~(0xf << (54-32));
	WTREG4(0xbfd011e4, val);	

	RDREG4(0xbfd011f4, val);
	val &= ~(0xf << (54-32));
	WTREG4(0xbfd011f4, val);	

	RDREG4(0xbfd01204, val);
	val &= ~(0xf << (54-32));
	WTREG4(0xbfd01204, val);	
    
}
 static int can0_init()
{
	 //init       
 /*   *(volatile unsigned int*)(0xbfd010c8) = 0xf00000;*/
	raw_wl(0x0,0x01);
    raw_wl(0x4,0x00);
    raw_wl(0x1f,0xc0);
    raw_wl(0x10,0x00);
    raw_wl(0x11,0x00);
    raw_wl(0x12,0x00);
    raw_wl(0x13,0x00);
    raw_wl(0x14,0xff);
    raw_wl(0x15,0xff);
    raw_wl(0x16,0xff);
    raw_wl(0x17,0xff);
    raw_wl(0x08,0x5a);
return 0;
}
static int can0_open()
 {
     //open
    raw_wl(0x04,0x00);
    raw_wl(0x06,0x45);
    raw_wl(0x07,0x26);
    raw_wl(0x0f,0x00);
    raw_wl(0x0e,0x00);
    raw_wl(0x00,0x00);  /*enable int*/
    raw_wl(0x04,0xff);
#if 0
	*(volatile unsigned int *) 0xbfd01054 = 0x40;  /*edge int*/
	*(volatile unsigned int *) 0xbfd01050 = 0x40;  /*high level int*/
	*(volatile unsigned int *) 0xbfd01044 = 0x40;

#endif	
	return 0;
}

 static int can0_send()
{

	printf("can0 send:  1 2 3 4 5 6 7\r\n");
    raw_wl(0x10,0x08);
    raw_wl(0x11,0x00);
    raw_wl(0x12,0x30);
    raw_wl(0x13,0x31);
    raw_wl(0x14,0x32);
    raw_wl(0x15,0x33);
    raw_wl(0x16,0x34);
    raw_wl(0x17,0x35);
    raw_wl(0x18,0x36);
    raw_wl(0x19,0x37);
    raw_wl(0x1a,0x00);
    raw_wl(0x01,0x01);

    return 0;
}
static int can_send(void)
{
	can0_init();
	can0_open();
	can0_send();
	return 0;
}
static int can0_reg(int argc,char **argv)//can0_reg addr num
{
    int reg =0,num=0,i;
    if(argc == 3){
        reg =strtoul(argv[1],0,0);
        num = strtoul(argv[2],0,0);
   //      num = min(num,16);                                                                                                                                                                 
    }else{
        reg=0;
        num=16;
    }
    printf("0x%08x:",0xbfe50000+reg);
    for(i=0;i<num;i++){
       if(i % 16 == 0 && i!=0)
		printf("\n0x%08x:",0xbfe50000+reg+(i/16)*16);
       if(i % 8==0)
         printf(" ");
         printf("%02x ",raw_rl(reg+i)); 
    }
    printf("\n");
    return 0;
}


 static int can1_init()
{
 /*   *(volatile unsigned int*)(0xbfd010c8) = 0xf00000;*/
     //init       
    raw_wl(0x4000,0x01);
    raw_wl(0x4004,0x00);
    raw_wl(0x401f,0xc0);
    raw_wl(0x4010,0x00);
    raw_wl(0x4011,0x00);
    raw_wl(0x4012,0x00);
    raw_wl(0x4013,0x00);
    raw_wl(0x4014,0xff);
    raw_wl(0x4015,0xff);
    raw_wl(0x4016,0xff);
    raw_wl(0x4017,0xff);
    raw_wl(0x4008,0x5a);
return 0;
 }
static int can1_open()
{
     //open
    raw_wl(0x4004,0x00);
    raw_wl(0x4006,0x45);
    raw_wl(0x4007,0x26);
    raw_wl(0x400f,0x00);
    raw_wl(0x400e,0x00);
    raw_wl(0x4000,0x00);
    raw_wl(0x4004,0xff);
 
    return 0;
}

 static int can1_read()
{
	char * b;
	int i=0;
	b = (char*)(0xbfe54012);
	printf("can1 received:");
	for(i=0;i < 8;i++)
		printf("%2c",*(b+i));
	printf("\r\n");
    return 0;
}
static int can_read()
{
	can1_init();
	can1_open();
	can1_read();
}
static int can1_reg(int argc,char **argv)//can0_reg addr num
{
    int reg =0,num=0,i;
    if(argc == 3){
        reg =strtoul(argv[1],0,0);
        num = strtoul(argv[2],0,0);
   //      num = min(num,16);                                                                                                                                                                 
    }else{
        reg=0;
        num=16;
    }
    printf("0x%08x:",0xb2e50000+reg);
    for(i=0;i<num;i++){
       if(i % 16 == 0 && i!=0)
		printf("\n0x%08x:",0xbfe54000+reg+(i/16)*16);
       if(i % 8==0)
         printf(" ");
         printf("%02x ",raw_rl(reg+i)); 
    }
    printf("\n");
    return 0;
}


/***************************************************************************
 * Description: pwm test
 * Parameters: 
 * Author  :Sunyoung_yg 
 * Date    : 2015-01-14
 ***************************************************************************/
int pwm_test(void)
{
	_u32 tmp;
	get_clock();	
#if 0
	//pwm1 use GPIO 92
	RDREG4(GPIO_CFG2, tmp);
	tmp &= 0xefffffff;
	WTREG4(GPIO_CFG2, tmp);

	//pwm3 use GPIO 37 ,sencond func
	RDREG4(MUX_BASE1+0x4, tmp);
	tmp |= (0x1<<5);
	WTREG4(MUX_BASE1+0x4, tmp);
#endif
	//pwm0 use GPIO 6
	RDREG4(GPIO_CFG0, tmp);
	tmp &= ~(0x1<<6);
	WTREG4(GPIO_CFG0, tmp);

	WTREG4(CNTR0, 0);
	WTREG4(HRC0, 499);
	WTREG4(LRC0, 999);
	WTREG4(CTRL0, 1);
	printf(" ls1c pwm0 output F(sdram)/1000 K Hz square! please test!\r\n");

//pwm3 use GPIO53 (the fourth fucn)
	RDREG4(MUX_BASE4+0x4, tmp);
	tmp |= 0x1<<(53-32);
	WTREG4(MUX_BASE4+0x4, tmp);

	WTREG4(CNTR0+0x30, 0);
	WTREG4(HRC0+0x30, 4999);
	WTREG4(LRC0+0x30, 9999);
	WTREG4(CTRL0+0x30, 1);
	printf(" ls1c pwm3 output F(sdram)/10000 K Hz square! please test gpio53\r\n");
#if 0
	tmp = 0x089;
	WTREG4(CNTR0, tmp);
	tmp = 0x089;
	WTREG4(HRC0, tmp);
	tmp = 0x0139;
	WTREG4(LRC0, tmp);
	tmp = 0x041;
	WTREG4(CTRL0, tmp);

	tmp = 0x04;
	WTREG4(CNTR1, tmp);
	tmp = 0x089;
	WTREG4(HRC1, tmp);
	tmp = 0x0139;
	WTREG4(LRC1, tmp);
	tmp = 0x041;
	WTREG4(CTRL1, tmp);
#endif
	return 0;
}
/***************************************************************************
 * Description: watch dog test!
 * Parameters: 
 * Author  :Sunyoung_yg 
 * Date    : 2016-01-29
 ***************************************************************************/
int wdt_test(void)
{	
	_u32 val;
	WTREG4(0xbfe5c060, 0xffffffff);
	RDREG4(0xbfe5c060, val);
	if(val != 0x1) {
		printf("watch dog test panic \n");
	}

	WTREG4(0xbfe5c064, 0xffffffff);
	RDREG4(0xbfe5c064, val);
	if(val != 0xffffffff) { 
		printf("watch dog test panic \n");
	}


	WTREG4(0xbfe5c064, 0x1ffff);
	WTREG4(0xbfe5c068, 0x1);
	RDREG4(0xbfe5c060, val);
	printf("watch dog test pass \n");
	return 0;
}


int can_test(void)
{

	can_gpio_init();
#if 1
	can_send();

	delay_100ms(1);

	can_read();
#else



	_u32 val;
	
	_u8 tmp;
	int i;

	

//	delay(100);	

	// can1_init
	WTREG1(CAN1_BASEADDR+0x00, 0x01);		//复位请求
	WTREG1(CAN1_BASEADDR+0x04, 0x00);		//禁止中断
	WTREG1(CAN1_BASEADDR+0x01, 0x80);		//扩展模式
	WTREG1(CAN1_BASEADDR+0x1f, 0xc0);		//？？？
	WTREG1(CAN1_BASEADDR+0x10, 0x00);		//清零 验收代码 0 	
	WTREG1(CAN1_BASEADDR+0x11, 0x00);		//。。。。。。。1
	WTREG1(CAN1_BASEADDR+0x12, 0x00);		//。。。。。。。2
	WTREG1(CAN1_BASEADDR+0x13, 0x00);		//。。。。。。。3
	WTREG1(CAN1_BASEADDR+0x14, 0xff);		//验收屏蔽  0
	WTREG1(CAN1_BASEADDR+0x15, 0xff);		//。。。。。1
	WTREG1(CAN1_BASEADDR+0x16, 0xff);		//。。。。。2
	WTREG1(CAN1_BASEADDR+0x17, 0xff);		//。。。。。3
	WTREG1(CAN1_BASEADDR+0x08, 0x5a);		//保留???????????
	delay(100);
	// can1_open
	WTREG1(CAN1_BASEADDR+0x04, 0x00);		//禁止中断
	WTREG1(CAN1_BASEADDR+0x06, 0x18);		//0x18   , 0x17
	WTREG1(CAN1_BASEADDR+0x07, 0x45);		//0x34	, 0x45 	
	WTREG1(CAN1_BASEADDR+0x0f, 0x00);		
	WTREG1(CAN1_BASEADDR+0x0e, 0x00);		
	WTREG1(CAN1_BASEADDR+0x00, 0x00);		
	WTREG1(CAN1_BASEADDR+0x04, 0xff);		


	delay(100);	
	//can_send #0123456
	WTREG1(CAN1_BASEADDR+0x10, 0x08);		//发送长度8字节
	WTREG1(CAN1_BASEADDR+0x11, 0x00);		
	WTREG1(CAN1_BASEADDR+0x12, 0x00);		
	WTREG1(CAN1_BASEADDR+0x13, 0x30);		
	WTREG1(CAN1_BASEADDR+0x14, 0x31);		
	WTREG1(CAN1_BASEADDR+0x15, 0x32);		
	WTREG1(CAN1_BASEADDR+0x16, 0x33);		
	WTREG1(CAN1_BASEADDR+0x17, 0x34);		
	WTREG1(CAN1_BASEADDR+0x18, 0x35);		
	WTREG1(CAN1_BASEADDR+0x19, 0x36);		
	WTREG1(CAN1_BASEADDR+0x1a, 0x00);		
//	WTREG1(CAN1_BASEADDR+0x01, 0x81);		
	printf(" \n\ncan1 transmit:\n");
	for(i=0x10; i<0x19; i++) {
		RDREG1(CAN1_BASEADDR+i, tmp);
//		tmp &= 0xff;
		printf(" 0x%x  ", tmp);
	}

	WTREG1(CAN1_BASEADDR+0x01, 0x81);		
	delay(100);	
	
	printf(" \n\ncan1 reg state:\n");
	for(i=0x00; i<0x08; i++) {
		RDREG1(CAN1_BASEADDR+i, tmp);
		tmp &= 0xff;
		printf(" 0x%x  ", tmp);
	}

	printf(" \n\ncan1 receive:\n");
	for(i=0x20; i<0x29; i++) {
		RDREG1(CAN1_BASEADDR+i, tmp);
		tmp &= 0xff;
		printf(" 0x%x  ", tmp);
	}

	

	printf(" \n\ncan0 receive:\n");
	for(i=0x20; i<0x29; i++) {
		RDREG1(CAN0_BASEADDR+i, tmp);
		tmp &= 0xff;
		printf(" 0x%x  ", tmp);
	}

#endif
	return 0;
}




/***************************************************************************
 * Description:
 * Parameters: 
 * Author  :Sunyoung_yg 
 * Date    : 2014-08-07
 ***************************************************************************/
/*cmd:
 * ls1c_bsp_test uart uartno
 *               touch
 *				 rtc
 *				 can
 */




static int cmd_ls1c_bsp_test(int argc,char **argv)
{
	unsigned  int channel;
	if(argc < 2)
	{
		printf(" cmd error!\r\n");
		return -1;
	}
	get_clock();
	if(!strcmp(argv[1],"uart"))
	{
		channel=strtoul(argv[2],0,0);
		if((channel<0) || (channel>11)){
			printf("cmd exp:bsp_test uart [channel:0~11] \r\n");
			return -1;
		}
		uart_test(channel);
	}
	else if(!strcmp(argv[1],"rtc"))
	{
		rtc_test();
	}
	else if (!strcmp(argv[1],"touch"))
	{
		touch_test();
	}
	else if (!strcmp(argv[1],"can"))
	{
		can_test();
	}
	else if (!strcmp(argv[1],"pwm"))
	{
		pwm_test();
	}
	else if (!strcmp(argv[1],"wdt"))
	{
		wdt_test();
	}
	return 0;
}
//----------------------------------
static const Cmd Cmds[] =
{
	{"ls1c_bsp_test"},
	{"ls1c_bsp_test","[dev][args]", 0, "", cmd_ls1c_bsp_test, 0, 99, CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


