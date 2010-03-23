/*
 *	This is a program for loongson 2g southbridge memory space scan
 *	Auther: sw
 */

#include "memscan.h"
#include <stdio.h>

#define CACHEDMEM	0x80000000		//kseg0
#define UNCACHEDMEM	0Xa0000000		//kseg1

void memscan()
{
	int i;
	printf("\n====begin to scan memspace...\n");
// 0xbfe00000 - 0xbfef,ffff	
	for(i = 0xbfe40000;i < 0xbfe7ffff;i = i+4)
	{	
		printf("==addr: %08x      val: %02x ",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
		printf("   val: %02x ",readb(i+1 | UNCACHEDMEM));
		printf("   val: %02x ",readb(i+2 | UNCACHEDMEM));
		printf("   val: %02x \n",readb(i+3 | UNCACHEDMEM));
	}
	printf("====scan from 0xbfe40000 - 0xbfe7,ffff done\n");

//kseg1	- DDR Slave 0 
// 0x0 - 0x0fff,ffff	
	for(i = 0x0;i < 0x0fffffff;i++)
		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
	printf("====scan from 0x0 - 0x0fff,ffff done\n");

//kuseg - reserved
	for(i = 0x20000000;i < 0x3fffffff;i++)
		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
	printf("====scan from 0x2000,0000 - 0x3fff,ffff done\n");
	
//kuseg	- DDR Slave 1
// 0x4000,0000 - 0x7fff,ffff
	for(i = 0x40000000;i < 0x7fffffff;i++)
		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
	printf("====scan from 0x4000,0000 - 0x7fff,ffff done\n");
		
	
}









