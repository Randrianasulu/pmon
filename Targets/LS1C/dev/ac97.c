#include <pmon.h>
#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <linux/types.h>
#include <target/i2c.h>
void uda1342_test();
#define udelay delay

#define uda_dev_addr    0x34
#define uda1342_base    0xbfe60000
#define IISVERSION  (volatile unsigned int *)(uda1342_base + 0x0)
#define IISCONFIG   (volatile unsigned int *)(uda1342_base + 0x4)
#define IISSTATE  (volatile unsigned int *)(uda1342_base + 0x8)



#define ac97_base 0xbfe74000  // 0xbfe74000
#define confreg_base 0xbfd00000
#define dma_base  0xbfe74080  //0xbf004280
#define ac97_reg_write(addr, val) 
#define ac97_reg_read(addr) 
//#define ac97_reg_write(addr, val) do{ *(volatile u32 *)(ac97_base+(addr))=val; }while(0)
//#define ac97_reg_read(addr) *(volatile u32 * )(ac97_base+(addr))
//#define udelay(n)   
#define dma_reg_write(addr, val) do{ *(volatile u32 *)(dma_base+(addr))=val; }while(0)
//#define dma_reg_read(addr) *(volatile u32 * )(dma_base+(addr))
#define dma_reg_read(addr) *(volatile u32 *)(addr)


//yq:0x70-->0x68
        
#define DMA_BUF	0x00800000
#define BUF_SIZE	0x200000        //2MB
//#define  BUF_SIZE 0x7000
#define SYNC_W 1    //sync cache for writing data
#define CPU2FIFO 1

#define AC97_RECORD 0
#define AC97_PLAY   1

#define REC_DMA_BUF		(DMA_BUF+ BUF_SIZE)        //0x00a00000
#define REC_BUF_SIZE		(BUF_SIZE>>1)

#define DEBUG
#ifdef DEBUG
#undef TR
#define TR0(fmt, args...) printf("SynopGMAC: " fmt, ##args)
#define TR(fmt, args...) printf("SynopGMAC: " fmt, ##args)
#else
#define TR0(fmt, args...)
#define TR(fmt, args...) // not debugging: nothing
#endif

static unsigned short sample_rate=0xac44;

static int ac97_rw=0;

static struct desc{
	u32 ordered;		//下一个描述符地址寄存器
	u32 saddr;			//内存地址寄存器
	u32 daddr;			//设备地址寄存器
	u32 length;		//长度寄存器
	u32 step_length;	//间隔长度寄存器
	u32 step_times;	//循环次数寄存器
	u32 cmd;			//控制寄存器
};

static unsigned short uda1342_reg[7][2] = {
    { 0x0000, 0x1a12, },
    { 0x0101, 0x0014, },
    { 0x1010, 0x0000, },
    { 0x1111, 0x0000, },
    { 0x1212, 0x0000, },
    { 0x2020, 0x0830, },
    { 0x2121, 0x0830, },
};

static struct desc *DMA_DESC_BASE;
static struct desc *dma_desc2_addr;

static u32 play_desc1[7]={
	0x1,                       //need to be filled
	DMA_BUF & 0x1fffffff,     
//	0xdfe72420,                //(ac97_base&0x9fffffff)+0x20,           //9fffffff?fun
    (uda1342_base + 0x10) & 0x1fffffff,
	0x8,
	0x0,
	(BUF_SIZE/8/4),
	0x00001001
};

static u32 play_desc2[7]={
	0x00001,
	DMA_BUF & 0x1fffffff,
//	0xdfe72420,                                //( ac97_base&0x9fffffff)+0x20,
    (uda1342_base + 0x10) & 0x1fffffff,
	0x6,
	0x0,
	(BUF_SIZE/8/6/2),
	0x00001001	
};

static u32 rec_desc1[7]={
	0x1,                       //need to be filled
	(REC_DMA_BUF|0xa0000000)&0x1fffffff,                    //(ac97_base&0x9fffffff)+0x20,           //9fffffff?func
//	0x9fe74c4c,
    (uda1342_base + 0xc) & 0x1fffffff,
	0x8,
	0x0,
	(BUF_SIZE/8/4),
	0x00000001
};

static u32 rec_desc2[7]={
	0x1,                       //need to be filled
	(REC_DMA_BUF|0xa0000000)&0x1fffffff,                    //(ac97_base&0x9fffffff)+0x20,      //9fffffff?func
//	0x9fe74c4c,
    (uda1342_base + 0xc) & 0x1fffffff,
	0x6,
	0x0,
	(BUF_SIZE/8/6/2),
	0x00000001
};


/***************************************************************************
 * Description: get_clock()
 * Parameters:
 *				md_cpufreq is the sdram's freq
 *				md_pipefreq is the cpu's freq
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

void  init_audio_data(void)
{
	unsigned int *data = (unsigned int*)(DMA_BUF|0xa0000000);
	int i;


	for (i=0; i<((BUF_SIZE)>>3); i++){
		data[i*2] = (i*11)<<5 + i*3 + i;
		data[i*2+1] = (i*11)<<4 + i*5 + i;
	}

	for(i=0; i<40; i++){
		TR("%x:  data:%x\n",((DMA_BUF|0xa0000000)+(i*4)),*(volatile unsigned int *)(DMA_BUF|0xa0000000+(i*4)));
	}
	TR("===init audio data complete\n");
}

void set_uda_reg(unsigned char *dev_add)
{
    printf ("lxy: dev_addr = 0x%x !\n", *dev_add);
    unsigned char i, k;
    unsigned short reg_buf[7];

    for (i=0; i<7; i++)
        tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, uda1342_reg[i][0], (unsigned char *)(&uda1342_reg[i][1]), 1); 
    
    for (i=0; i<7; i++)
    {
        tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, uda1342_reg[i][0], (unsigned char *)(&reg_buf[i]), 1); 
        printf ("reg: 0x%x = 0x%x !\n", uda1342_reg[i][0], reg_buf[i]);
    }
}

void uda1342_config()
{
    unsigned char rat_cddiv;
    unsigned char rat_bitdiv;
    unsigned char dev_addr[2];
    unsigned short value;

	get_clock();
    dev_addr[0] = uda_dev_addr;

//rat_bitdiv = bclk_div=Fsdram/2/BCLK -1
//rat_cddiv  = mclk_div = Fsdram/2/(MCLK)-1
	
    rat_bitdiv = md_cpufreq/2/1500000-1;
	rat_cddiv  = md_cpufreq/2/(1500000*8)-1;
	
    * IISCONFIG = (16<<24) | (16<<16) | (rat_bitdiv<<8) | (rat_cddiv<<0) ;
//    * IISSTATE = 0xc220;
    * IISSTATE = 0xc200;
    value =0x8000;
    uda1342_test();
//    tgt_i2cinit();
//    tgt_i2cwrite1(I2C_HW_SINGLE, dev_addr, 1, 0x0, (unsigned char *)(&value), 1); //lxy: reset uda1342 all register
//    set_uda_reg(dev_addr);
}
 


void dma_config(void)
{
	u32 addr;
	u32 addr2;

	DMA_DESC_BASE = (struct desc*)malloc(sizeof(struct desc) + 32);
	addr = DMA_DESC_BASE = ((u32)DMA_DESC_BASE+31) & (~31);
	TR("===addr:%x\n",DMA_DESC_BASE);

	dma_desc2_addr = (struct desc*)malloc(sizeof(struct desc) + 32);
	addr2 = dma_desc2_addr = ((u32)dma_desc2_addr+31) & (~31);
	TR("===addr2:%x\n",dma_desc2_addr);
	
	/* play */
	if (AC97_PLAY == ac97_rw) {
        * IISSTATE = (0xc200 | 0x1080);
		DMA_DESC_BASE->ordered = play_desc1[0] | (addr & 0x1fffffff);    ///addr&0xffffffe0;
		DMA_DESC_BASE->saddr = play_desc1[1];   //0x00010000;//addr&0x1fffffff;
		DMA_DESC_BASE->daddr = play_desc1[2];
		DMA_DESC_BASE->length = play_desc1[3];
		DMA_DESC_BASE->step_length = play_desc1[4];
		DMA_DESC_BASE->step_times = play_desc1[5];
		DMA_DESC_BASE->cmd = play_desc1[6];
		pci_sync_cache(0, (unsigned long)addr, 32*7, SYNC_W);
		TR("===play1\n");
		TR("===desc1:%x;%x,%x,%x,%x\n",DMA_DESC_BASE->ordered,DMA_DESC_BASE->saddr,DMA_DESC_BASE->daddr,DMA_DESC_BASE->length,DMA_DESC_BASE->step_times);	 
		TR("===mem desc:%x,%x\n",*(volatile unsigned int*)(addr),*(volatile unsigned int*)(addr+4));
		
		dma_desc2_addr->ordered = play_desc2[0] | (addr2 & 0x1fffffff);
		dma_desc2_addr->saddr = play_desc2[1];//0x00001000;//addr2&0x1fffffff;                     //play_desc2[1];
		dma_desc2_addr->daddr = play_desc2[2];
		dma_desc2_addr->length = play_desc2[3];
		dma_desc2_addr->step_length = play_desc2[4];
		dma_desc2_addr->step_times = play_desc2[5];
		dma_desc2_addr->cmd = play_desc2[6];
		pci_sync_cache(0, (unsigned long)addr2, 32*7, SYNC_W);
		TR("===play2");
		TR("===desc2:%x\n", dma_desc2_addr->ordered);
    
		addr = (u32)(addr & 0x1fffffff);

		TR("===addr:%x\n",addr);
		TR("===addr':%x",addr|0x00000009);
		do{
			*(volatile u32*)(confreg_base+0x1160) = addr|0x00000009;
			delay(1000);
			TR("dma register:%x\n",(*(volatile u32*)(confreg_base+0x1160)));
		}while(0);
	}
	/* record 记录 */
	else{
        * IISSTATE = (0xc200 | 0x2800);
		DMA_DESC_BASE->ordered = rec_desc1[0] | (addr & 0x1fffffff);    ///addr&0xffffffe0;
		DMA_DESC_BASE->saddr = rec_desc1[1];   //0x00010000;//addr&0x1fffffff;
		DMA_DESC_BASE->daddr = rec_desc1[2];
		DMA_DESC_BASE->length = rec_desc1[3];
		DMA_DESC_BASE->step_length = rec_desc1[4];
		DMA_DESC_BASE->step_times = rec_desc1[5];
		DMA_DESC_BASE->cmd = rec_desc1[6];
		TR("========rec1\n");
		pci_sync_cache(0, (unsigned long)addr, 32*7, SYNC_W);

		TR("====rec desc:%x;%x,%x,%x,%x\n",DMA_DESC_BASE->ordered,DMA_DESC_BASE->saddr,DMA_DESC_BASE->daddr,DMA_DESC_BASE->length,DMA_DESC_BASE->step_times);	 
		dma_desc2_addr->ordered = rec_desc2[0] | (addr2 & 0x1fffffff);
		dma_desc2_addr->saddr = rec_desc2[1];//0x00001000;//addr2&0x1fffffff;                     //play_desc2[1];
		dma_desc2_addr->daddr = rec_desc2[2];
		dma_desc2_addr->length = rec_desc2[3];
		dma_desc2_addr->step_length = rec_desc2[4];
		dma_desc2_addr->step_times = rec_desc2[5];
		dma_desc2_addr->cmd = rec_desc2[6];
		pci_sync_cache(0, (unsigned long)addr2, 32*7, SYNC_W);
		TR("===rec2");
		TR("====desc2:%x\n", dma_desc2_addr->ordered);

		addr=(u32)(addr&0x1fffffff);    
		TR("====addr:%x\n",addr);
		TR("====addr':%x",addr|0x0000000a);
		do{
			*(volatile u32*)(confreg_base+0x1160) = addr|0x0000000a;
			delay(1000);	
			TR("dma register:%x\n",(*(volatile u32*)(confreg_base+0x1160)));
		}while(0);
	}
    printf ("iis_state = 0x%08x !\n", (* IISSTATE));
}

/*
void dma_setup_trans(u32 * src_addr,u32 size)
{
	// play
	if (AC97_PLAY==ac97_rw){
		dma_reg_write(0x0,src_addr);
		dma_reg_write(0x4,size>>2);
	}
	//record
	else{
		dma_reg_write(0x10,src_addr);
		dma_reg_write(0x14,size>>2);
	}
}
*/

/* 测试录音结果 */
int ac97_test(int argc,char **argv)
{
	char cmdbuf[100];
	int i;

    printf("ac97_test argc = %d\n", argc);

	if(argc!=2 && argc!=1)return -1;
	if(argc==2){
		sprintf(cmdbuf,"load -o 0x%x -r %s",DMA_BUF|0xa0000000,argv[1]);
		printf("load -o 0x%x -r %s\n",DMA_BUF|0xa0000000,argv[1]);
		do_cmd(cmdbuf);
	}
	ac97_rw = AC97_PLAY;
uda1342_config();
//ac97_config(); // add
#ifdef CONFIG_CHINESE
	printf("开始放音：注意听是否跟所录声音一致\n");
#else
	printf("begin test\n");
#endif
	
	/* 需要配置DMA */
	dma_config();
	TR("play data on 0x%x, sz=0x%x\n", (DMA_BUF|0xa0000000), BUF_SIZE);
	*(volatile u32*)(confreg_base+0x1160) = 0x00200005;
	TR("dma state1:%x\n%x\n%x\n%x\n%x\n", dma_reg_read(0xa0200004), dma_reg_read(0xa0200008), dma_reg_read(0xa020000c), dma_reg_read(0xa0200014), dma_reg_read(0xa0200018));
	*(volatile u32*)(confreg_base+0x1160) = 0x00200005;
	TR("dma state2:%x\n%x\n%x\n%x\n%x\n", dma_reg_read(0xa0200004), dma_reg_read(0xa0200008), dma_reg_read(0xa020000c), dma_reg_read(0xa0200014), dma_reg_read(0xa0200018));

	//1.wait a trans complete
//	while(((dma_reg_read(0x2c))&0x1)==0)
//	idle();

	//2.clear int status  bit.
//	dma_reg_write(0x2c,0x1);
	/* 等待 录音时间 */
	for(i=0; i<8; i++){
		udelay(1000000);
		printf("."); 
	}
	/* codec reset 复位AC97 codec */
	ac97_reg_write(0x18,0x0|(0x0<<16)|(0<<31));
//	codec_wait(10000);
#ifdef CONFIG_CHINESE
	printf("放音结束\n");
#else
	printf("test done\n");
#endif
	return 0;
}

/* 把codec 输入的数据通过DMA传输到内存 */
int ac97_read(int argc,char **argv)
{
	int i;
	unsigned int j;
	unsigned short * rec_buff;
	unsigned int  *  ply_buff;
	ac97_rw = AC97_RECORD;

	init_audio_data();

	uda1342_config();
    printf ("lxy: after uda1342_config");

	/*1.dma_config read*/
	dma_config();
	
	/*2.ac97 config read*/
//	ac97_config();

	/*3.set dma desc*/
#ifdef CONFIG_CHINESE
	printf("请用麦克风录一段音 ");
#else
	printf("please speck to microphone ");
#endif
	for(i=0;i<8;i++){
		udelay(1000000);
//		printf("."); 
        printf ("iis_read data = 0x%x !\n", *(volatile unsigned int *)(0xbfe6000c));
	}
#ifdef CONFIG_CHINESE
	printf("录音结束\n");
#else
	printf("rec done\n");
#endif
	
	/* 0xbfd01160 DMA模块控制寄存器位 */
	*(volatile u32*)(confreg_base + 0x1160) = 0x00200006;
	TR("dma state1:%x\n%x\n%x\n%x\n%x\n",dma_reg_read(0xa0200004),dma_reg_read(0xa0200008),dma_reg_read(0xa020000c),dma_reg_read(0xa0200014),dma_reg_read(0xa0200018));
//	dma_setup_trans(REC_DMA_BUF,REC_BUF_SIZE);

	/*4.wait dma done,return*/ 
//	while(((dma_reg_read(0x2c))&0x8)==0)
//		udelay(1000);//1 ms 
//	dma_reg_write(0x2c,0x8);
	
#if 1   //lxy
	/*5.transform single channel to double channel*/
	rec_buff = (unsigned short *)(REC_DMA_BUF | 0xa0000000);
	ply_buff = (unsigned int *)(DMA_BUF | 0xa0000000);

	for(i=0; i<(REC_BUF_SIZE<<1); i++){
		j = 0x0000ffff & (unsigned int) rec_buff[i*2];
		ply_buff[i] = (j<<16)|(j);
	}
#else
    
	/*5.transform single channel to double channel*/
	rec_buff = (unsigned short *)(REC_DMA_BUF | 0xa0000000);
	ply_buff = (unsigned int *)(DMA_BUF | 0xa0000000);

	for(i=0; i<(REC_BUF_SIZE<<1); i++){
		j = 0x0000ffff & (unsigned int) rec_buff[i];
		ply_buff[i] = (j<<16)|(j);
	}
#endif

#if 0
	rec_buff = (unsigned int *)(REC_DMA_BUF | 0xa0000000);
	ply_buff = (unsigned int *)(DMA_BUF | 0xa0000000);

	for(i=0; i<REC_BUF_SIZE; i++)
    {
		ply_buff[i] = rec_buff[i];
	}
#endif

	return 0;
}


#define uda_address     0x34
#define gc0308_address  0x42

unsigned char gc0308_data[][2] = {
    {0xfe, 0x00},
    {0x24, 0xa2},
    {0x25, 0x0f},
    {0x26, 0x02},
};

void gc0308_test()
{
    unsigned char i;
    unsigned char dev_add[2];
    unsigned char read_buf[4];
    *dev_add = gc0308_address;

    for (i=0; i<4; i++)
    {
        tgt_i2cwrite1(I2C_SINGLE, dev_add, 1, gc0308_data[i][0], gc0308_data[i]+1, 1);     
    }
    
    for (i=0; i<4; i++)
    {
        tgt_i2cread1(I2C_SINGLE, dev_add, 1, gc0308_data[i][0], read_buf+i, 1);
    }

    printf("read data from gc0308: \n");
    for (i=0; i<4; i++)
        printf (" 0x%x, ", read_buf[i]);
    printf ("\n");
}


unsigned short reg1[7] = {
//    0x1c02,
//    0x1a02,
    0x1402,  //Ê¹ÄÜÊä³öDC-filter£¬input Í¨µÀ2£¬DAC power-on, sysclk 256fs
//    0x1402,
//    0x1002,
//    0x1202,
    0x0014,  //Ê¹ÄÜ»ìÒôÆ÷£¬Silence detection period: 4800 samples
//    0x0010,
//    0xc003,
//    0xff63,
//    0xff00,
    0xff03,  //¸ßµÍÒôÔöÇ¿×î´ó£¬  treble  setting: 6dB
//    0xfc00,
//    0xcf83,
    0x0000,  //
    0x0000,  //
    0x0030,  //adc input1 mixer gain 0x30
    0x0030,  //adc input1 mixer gain 0x30
//    0x0230,
//    0x0230,
//    0x0830,
//    0x0830,
};

void uda1342_test()
{
    unsigned char dev_add[2];
    unsigned short data;
    unsigned char reg;
    int ret = 0;
    *dev_add = uda_address;
    reg = 0x10; 
    unsigned short data1[7];
    unsigned char i;

//    *((volatile unsigned int *)(0xbfe60004)) = (16<<24) | (16<<16) | (7<<8) | (0<<0);
    tgt_i2cinit();

    tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data), 1);
    printf ("data = 0x%x !\n", data);
    
//    while (1)
//    {
#if 1
	//µÍÒôÔöÇ¿ ×î´ó
    data = 0xfc00;  
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data), 1);
    if (ret == -1)
        printf ("write error......\n");

    data = 0x0;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data), 1);
    if (ret == -1)
        printf ("read error......\n");
    printf ("data = 0x%x !\n", data);
#endif

/*
    data = 0x8000;
    reg =0x0;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data), 1);
*/

#if 1
	//Ð´Êý¾Ý
    reg = 0x0;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[0]), 1);
    reg = 0x1;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[1]), 1);
    reg = 0x10;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[2]), 1);
    reg = 0x11;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[3]), 1);
    reg = 0x12;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[4]), 1);
    reg = 0x20;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[5]), 1);
    reg = 0x21;
    ret = tgt_i2cwrite1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&reg1[6]), 1);
    
    //¶Á³öÀ´
    reg = 0x0;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[0]), 1);
    reg = 0x1;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[1]), 1);
    reg = 0x10;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[2]), 1);
    reg = 0x11;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[3]), 1);
    reg = 0x12;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[4]), 1);
    reg = 0x20;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[5]), 1);
    reg = 0x21;
    ret = tgt_i2cread1(I2C_HW_SINGLE, dev_add, 1, reg, (unsigned char *)(&data1[6]), 1);
	//´òÓ¡Ð£Ñé
    for (i=0; i<7; i++)
    {
        printf ("data1[%d] = 0x%x ---> 0x%x!\n", i, data1[i], reg1[i]);
    }
#endif
//    }
}


































static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"ac97_test","file",0,"ac97_test file",ac97_test,0,99,CMD_REPEAT},
	{"ac97_read","",0,"ac97_read",ac97_read,0,99,CMD_REPEAT},
//	{"ac97_config","",0,"ac97_config",ac97_config,0,99,CMD_REPEAT},
	{"uda1342_config","",0,"uda1342_config",uda1342_config,0,99,CMD_REPEAT},
	{"init_audio_data","",0,"init_audio_data",init_audio_data,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

