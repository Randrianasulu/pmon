#include <stdio.h>
#include "include/fcr.h"
#include <stdlib.h>
#include <ctype.h>
#undef _KERNEL
#include <errno.h>
#include <pmon.h>
#include <include/types.h>

#define SPI_BASE  0x1fe80000
#define PMON_ADDR 0xa1000000
#define FLASH_ADDR 0x000000

#define SPCR      0x0
#define SPSR      0x1
#define TXFIFO    0x2
#define SPER      0x3
#define PARAM     0x4
#define SOFTCS    0x5
#define PARAM2    0x6

#define SET_SPI(addr,val)        KSEG1_STORE8(SPI_BASE+addr,val)
#define GET_SPI(addr)            KSEG1_LOAD8(SPI_BASE+addr)


void spi_initw()
{ 
	unsigned char val;

  	SET_SPI(SPSR, 0xc0); 
  	printf("SPSR:%x\n",GET_SPI(0x1));
	
  	SET_SPI(PARAM, 0x40);             //espr:0100
  	printf("PARAM:%x\n",GET_SPI(0x4));
	
 	SET_SPI(SPER, 0x05); //spre:01 
	
  	SET_SPI(PARAM2,0x01); 
  	printf("====CS:%x\n",GET_SPI(0x5));
	
  	SET_SPI(SPCR, 0x50);
  	printf("SPCR:%x\n",GET_SPI(0x0));
  
}



///////////////////read status reg /////////////////

int read_sr(void)
{
	int val;
	
	SET_SPI(0x5,0x01);
	SET_SPI(0x2,0x05);

	while((GET_SPI(0x1))&0x1 == 0x1){
		
	}
	val = GET_SPI(0x2);
	SET_SPI(0x2,0x00);

	while((GET_SPI(0x1))&0x1 == 0x1){
				
	}
	val = GET_SPI(0x2);
	
	SET_SPI(0x5,0x11);
      
	return val;
}


////////////set write enable//////////
int set_wren(void)
{
	int res;
	
	res = read_sr();
	while(res&0x01 == 1)
	{
		res = read_sr();
	}
	
	SET_SPI(0x5,0x01);
	
	SET_SPI(0x2,0x6);
       	while((GET_SPI(0x1))&0x1 == 0x1){
	}
	GET_SPI(0x2);

	SET_SPI(0x5,0x11);

	return 1;
}

///////////////////////write status reg///////////////////////
int write_sr(char val)
{
	int res;
	
	set_wren();
	
	res = read_sr();
	while(res&0x01 == 1)
	{
		res = read_sr();
	}
	
	SET_SPI(0x5,0x01);

	SET_SPI(0x2,0x01);
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);

	SET_SPI(0x2,val);
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	SET_SPI(0x5,0x11);
	
	return 1;
	
}

///////////erase all memory/////////////
int erase_all(void)
{
	int res;
	
	set_wren();
	res = read_sr();
	while(res&0x01 == 1)
	{
		res = read_sr();
	}
	
	SET_SPI(0x5,0x1);
	
	SET_SPI(0x2,0xC7);
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	
	SET_SPI(0x5,0x11);

	return 1;
}

int erase_sector(char addr2,char addr1,char addr0)
{
	int res;
	
	set_wren();
	res = read_sr();
	while(res&0x01 == 1)
	{
		res = read_sr();
	}
	
	SET_SPI(0x5,0x01);
	
	SET_SPI(0x2,0xd8);
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	SET_SPI(0x2,addr2);
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	SET_SPI(0x2,addr1);
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	SET_SPI(0x2,addr0);
	
       	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	
	SET_SPI(0x5,0x11);

	return 1;

}

int write_pmon(void)
{
	int ret=0;
	long int i=0,j=0;
	int a=0;
	int val;
	unsigned char *data;
	unsigned char addr2,addr1,addr0;
	int addr=0;
	
//	spi_initw();

	val = read_sr();
	while(val&0x01 == 1)
	{
		val = read_sr();
	}
	SET_SPI(0x5,0x01);
	
// read flash id command
	SET_SPI(0x2,0xab);
	while((GET_SPI(0x1))&0x1 == 0x1){
				
	}
	GET_SPI(0x2);
	
        SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
				
	}
	GET_SPI(0x2);
	
        SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	
        SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
				
	}
        GET_SPI(0x2);
// commad end
		
        SET_SPI(0x2,0x00);
        while((GET_SPI(0x1))&0x1 == 0x1){
      			
        }
        val = GET_SPI(0x2);
        printf("id:%x\n",val);
//     while ((*(volatile unsigned char *)(0xbfe80001))&0x01);
	val = GET_SPI(0x1);
	printf("====spsr value:%x\n",val);
	
	SET_SPI(0x5,0x10);
// erase the flash     
	write_sr(0x00);
	erase_all();
	
/*	addr2 = addr1 = addr0 = 0x00;
	
	for(i=0;i<64;i++){
		erase_sector(addr2,addr1,addr0);
		addr2++;
	}
*/

// write flash
	i=j=0;
	while(1)
	{
		a=256*i;
		addr2 = ((FLASH_ADDR+a)&0xff0000)>>16;
		addr1 = ((FLASH_ADDR+a)&0x00ff00)>>8;
		addr0 = ((FLASH_ADDR+a)&0x0000ff);

		set_wren();
		val = read_sr();
		while(val&0x01 == 1)
		{
			val = read_sr();
		}
		
		SET_SPI(0x5,0x01);
		
// writing sector command
		SET_SPI(0x2,0x2);		
        	while((GET_SPI(0x1))&0x1 == 0x1){
      			
		}
		val = GET_SPI(0x2);

// addr
		SET_SPI(0x2,addr2);     
        	while((GET_SPI(0x1))&0x1 == 0x1){
      			
        	}
	        val = GET_SPI(0x2);
		SET_SPI(0x2,addr1);
        	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	        }
	        val = GET_SPI(0x2);
		SET_SPI(0x2,addr0);
	        while((GET_SPI(0x1))&0x1 == 0x1){
      			
	        }
	        val = GET_SPI(0x2);
// addr end
		
//		printf("===PP send \n");
		while(j<256)
		{
/*		     	if(addr%16 == 0 ){
				printf("    %08x\n",addr);
			}
*/
			if(addr == 0x00060000){
				printf("end\n");
				ret=1;
				break;
			}

			data =(unsigned char*) (PMON_ADDR+a+j);
//			printf(" %02x ",*data);
			
			SET_SPI(0x2,*data);
	                while((GET_SPI(0x1))&0x1 == 0x1){
      			
	                }
	                val = GET_SPI(0x2);
			j++;
			addr++;	
			
		}
		if(ret) break;
		j=0;
		SET_SPI(0x5,0x11);
		i++;
	
	}

	return 1;
}

int read_pmon(void)
{
	unsigned char addr2,addr1,addr0;
	unsigned char data;
	int val;
	int i=100000;
	int addr=0;
	
//	spi_initw();
	val = read_sr();
	while(val&0x01 == 1)
	{
		val = read_sr();
	}
	
	SET_SPI(0x5,0x01);
      			
// read flash command 
	SET_SPI(0x2,0x03);
	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	
// addr
	SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
        GET_SPI(0x2);
	
	SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
	
	SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	GET_SPI(0x2);
// addr end
	
/*	SET_SPI(0x2,0x00);
	while((GET_SPI(0x1))&0x1 == 0x1){
      			
	}
	data = GET_SPI(0x2);
	
	data = GET_SPI(0x2);
	printf("%02x   ",data);
*/
        
	while(i--)
	{
		SET_SPI(0x2,0x00);
		while((GET_SPI(0x1))&0x1 == 0x1){
      			
		}
	        data = GET_SPI(0x2);
		printf("%02x   ",data);
		if(addr%16 == 0 ){
			printf("    %08x\n",addr);
		}
		addr++;	
	}
	return 1;
	
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"write_pmon","",0,"write_pmon",write_pmon,0,99,CMD_REPEAT},
	{"read_pmon","",0,"read_pmon",read_pmon,0,99,CMD_REPEAT},
	{"spi_initw","",0,"spi_initw",spi_initw,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds,1);
}





