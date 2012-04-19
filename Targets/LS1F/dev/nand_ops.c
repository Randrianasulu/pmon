#include<pmon.h>
#include<asm.h>
#include<machine/types.h>
#include<linux/mtd/mtd.h>
#include<linux/mtd/nand.h>
#include<linux/mtd/partitions.h>
#include<sys/malloc.h>
#define MYDBG 
//printf("%s:%x\n",__func__,__LINE__);


#define _WL(x,y)    (*((volatile unsigned int*)(x)) = (y))
#define _RL(x,y)    ((y) = *((volatile unsigned int*)(x)))

#define NAND_WL(x,y)    _WL((x) + NAND_BASE,(y))
#define NAND_RL(x,y)    _RL((x) + NAND_BASE,(y))
#define DMA_WL(x,y)    _WL((x) + DMA_DESP,(y))
#define DMA_RL(x,y)    _RL((x) + DMA_DESP,(y))



#define RAM_ADDR            0xa4100000
#define DMA_DESP            0xa4001000
#define NAND_TMP            0xa4800000

#define BLOCK_TO_PAGE(x)  ((x)<<6)
#define PAGE_TO_BLOCK(x)  ((x)>>6)

#define PAGE_TO_FLASH(x)  ((x)<<11)
#define PAGE_TO_FLASH_H(x)  ((x)>>21)

#define PAGE_TO_FLASH_S(x)  ((x)<<12)
#define PAGE_TO_FLASH_H_S(x)  ((x)>>20)

#define FLASH_TO_PAGE(x)  ((x)>>11)
#define VT_TO_PHY(x)      ((x) & 0x0fffffff)   

#define PAGES_A_BLOCK       0x40

#define NAND_BASE           0xbfe78000
#define CMD                 0x0
#define ADDRL               0x4
#define ADDRH               0x8
#define TIMING              0xc
#define IDL                 0x10
#define IDH                 0x14
#define PARAM               0x18
#define OPNUM               0x1c
#define CS_RDY_MAP          0x20
#define DMA_ACCESS_ADDR     0x40

#define DMA_ORDER           0x0
#define DMA_SADDR           0x4
#define DMA_DEV             0x8
#define DMA_LEN             0xc
#define DMA_SET_LEN         0x10
#define DMA_TIMES           0x14
#define DMA_CMD             0x18


#define STOP_DMA            (_WL(0xbfd01160,0x10))
#define START_DMA(x)       do{  _WL(0xbfd01160,(VT_TO_PHY(x)|0x8));}while(0)
#define ASK_DMA(x)          (_WL(0xbfd01160,(VT_TO_PHY(x)|0x4)))

#define en_count(tmp)            do{asm volatile("mfc0 %0,$23;li $2,0x2000000;or $2,%0;mtc0 $2,$23;":"=r"(tmp)::"$2"); }while(0)
#define dis_count(tmp)           do{asm volatile("mtc0 %0,$23;"::"r"(tmp)); }while(0)

//#define SET_GPIO0_OUT(x)    do{x=(x==0?0:1);x=(*((volatile unsigned int*)0xbfd010f0) & ~0x1)|x; _WL(0xbfd010f0,(x));}while(0)
//#define SET_GPIO1_IN(x)    ((*((volatile unsigned int*)0xbfd010e0) & 0x2)>>1)


#if 0
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int  u32;
typedef unsigned long long u64;
#endif
int is_bad_block(u32 block);
int erase_block(u32 block);
int read_pages(u32 ram,u32 page,u32 pages,u8 flag);//flag:0==main 1==spare 2==main+spare
int write_pages(u32 ram,u32 page,u32 pages,u8 flag);//flag:0==main 1==spare 2==main+spare
int read_nand(u32 ram,u32 flash, u32 len,u8 flag);
int write_nand(u32 ram,u32 flash,u32 len,u8 flag);
int erase_nand(u32 start,u32 blocks);

int chipsize=0x08000000;

void nand_udelay(unsigned int us)
{
    int tmp=0;
//    en_count(tmp);
    udelay(us);
//    dis_count(tmp);
}
static void nand_send_cmd(u32 cmd,u32 page)//Send addr and cmd
{

    NAND_WL(CMD,0);
    if(cmd & (0x1<<9)){
        if(cmd &(0x1<<8)){
            NAND_WL(ADDRL,PAGE_TO_FLASH_S(page));
            //printf("ADDRL is 0x%08x\n", (page*0x840)|0x800);
        }else{
            NAND_WL(ADDRL,PAGE_TO_FLASH_S(page)+0x800);
        }
//        NAND_WL(ADDRH,PAGE_TO_FLASH_H_S(page));
    }else{
        NAND_WL(ADDRL,PAGE_TO_FLASH(page));
//        NAND_WL(ADDRH,PAGE_TO_FLASH_H_S(page));
    }
    {
        int val=0;
        //printf("flashaddr==0x%08x,cmd==%d\n",NAND_RL(ADDRL,val),cmd);
    }

    NAND_WL(CMD,cmd & (~0xff));
    NAND_WL(CMD,cmd);
}
#if 0
void gpio_init(void)
{
    _WL(0xbfd010c0,0x3);//enable gpio0 and gpio1
    _WL(0xbfd010d0,0x2);//gpio0 output,gpio1 intput
    
}
#endif

static u8 rdy_status(void)
{
    return ((*((u32*) (NAND_BASE+CMD)) & (0x1<<16)) == 0 ? 0:1);
}
static u8 done_status(void)
{
    return ((*((volatile u32*) (NAND_BASE+CMD)) & (0x1<<10)) == 0 ? 0:1);
}

static u8 nand_op_ok(u32 len,u32 ram)
{
    #ifdef LS1FSOC
    u32 ask=0,dmalen=0,dmastatus=0;
    while(1){
    ASK_DMA(DMA_DESP);
    _RL(DMA_DESP+0x4,ask);
    _RL(DMA_DESP+0xc,dmalen);
    _RL(DMA_DESP+0x18,dmastatus);
//    printf("ask==0x%08x,damlen==0x%08x,dmastatus==0x%08x\n",ask,dmalen,dmastatus);
//    printf("ask should==0x%08x,len==0x%08x\n", (VT_TO_PHY(ram)+((len+3)/4)*4),len);
    if(ask == (VT_TO_PHY(ram)+((len+3)/4)*4)){break;}else{return 0;}
    nand_udelay(60);
    }
    while(!rdy_status())
        nand_udelay(20);
    #else
    while(!done_status())
        nand_udelay(20);
    #endif
    return 0;
}
static void dma_config(u32 len,u32 ddr,u32 cmd)
{
    MYDBG
    DMA_WL(DMA_ORDER,0);
    MYDBG
    DMA_WL(DMA_SADDR,VT_TO_PHY(ddr));
    MYDBG
    DMA_WL(DMA_DEV,0x1fe78040);
    MYDBG
    DMA_WL(DMA_LEN,(len+3)/4);
    MYDBG
    DMA_WL(DMA_SET_LEN,0);
    MYDBG
    DMA_WL(DMA_TIMES,1);
    MYDBG
    DMA_WL(DMA_CMD,cmd);
    MYDBG
}
int is_bad_block(u32 block)
{
    u32 page=0;
    page = BLOCK_TO_PAGE(block);
    read_pages(RAM_ADDR,page,1,1);
    if(*(( u8*) (RAM_ADDR))!= 0xff)
    {
        //printf("bad block %d is %x\n", block, PAGE_TO_FLASH(BLOCK_TO_PAGE(block)));
        return 1;
    }
    return 0;
}
int erase_block(u32 block)
{
    u32 flash_l=0,flash_h=0;
    NAND_WL(TIMING,0x206);
    NAND_WL(OPNUM,0x1);
    //printf("block==0x%08x\n",block);
    nand_send_cmd(0x9,BLOCK_TO_PAGE(block));
    MYDBG
    nand_udelay(50);
    MYDBG
    #ifdef LS1FSOC
    while(!rdy_status())
    nand_udelay(30);
    #else
    while(!done_status())
    nand_udelay(30);
    #endif
    return 0;
}
int read_pages(u32 ram,u32 page,u32 pages,u8 flag)//flag:0==main 1==spare 2==main+spare
{
    u32 len=0,dma_cmd=0,nand_cmd=0;
    u32 ret = 0,tmp;
    switch(flag){
        case 0:
            len = pages * 0x800;
            nand_cmd = 0x103;
            break;
        case 1:
            len = pages * 0x40;
            nand_cmd = 0x203;
            break;
        case 2:
            len = pages * 0x840;
            nand_cmd = 0x303;
            break;
        default:
            len = 0;
            nand_cmd = 0;
            break;
    }
    MYDBG
    dma_config(len,ram,dma_cmd);
    MYDBG
    STOP_DMA;
    MYDBG
    nand_udelay(5);
    MYDBG
    START_DMA(DMA_DESP);
//    printf("0xbfd001160===0x%08x\n",*(volatile unsigned int *)(0xbfd01160));
    NAND_WL(TIMING,0x206);
    NAND_WL(OPNUM,len);
    nand_send_cmd(nand_cmd,page);
    nand_udelay(20000);
    MYDBG
    ret = nand_op_ok(len,ram);
//    nand_udelay(100);
    MYDBG
   if(ret){
        printf("nandread 0x%08x page 0x%08x pages have some error\n",page,pages);
   } 
   return ret;
}
#if 0
int write_pages(u32 ram,u32 page,u32 pages,u8 flag)
{
    u32 len=0,dma_cmd=0x1000,nand_cmd=0;
    u32 ret=0;
    switch(flag){
        case 0:
            len = pages * 0x800;
            nand_cmd = 0x105;
            break;
        case 1:
            len = pages * 0x40;
            nand_cmd = 0x205;
            break;
        case 2:
            len = pages * 0x840;
            nand_cmd = 0x305;
            break;
        default:
            len = 0;
            nand_cmd = 0;
            break;
    }
    printf("len==0x%08x,pages==0x%08x\n",len,pages);
    dma_config(len,ram,dma_cmd);
    STOP_DMA;
    nand_udelay(10);
    START_DMA(DMA_DESP);
    NAND_WL(TIMING,0x412);
    NAND_WL(OPNUM,len);
    nand_send_cmd(nand_cmd,page);
    nand_udelay(1000);
        MYDBG;
    ret = nand_op_ok(len,ram);
        MYDBG;
   if(ret){
        printf("nandwrite 0x%08x page 0x%08x pages have some error\n",page,pages);
   } 
   return ret;
}
#endif

int write_pages(u32 ram,u32 page,u32 pages,u8 flag)
{
    u32 len=0,dma_cmd=0x1000,nand_cmd=0;
    u32 ret=0,step=1;
    int val=pages;
#if 1
    switch(flag){
        case 0:
            len = step*0x800;
            nand_cmd = 0x105;
            break;
        case 1:
            len =  step*0x40;
            nand_cmd = 0x205;
            break;
        case 2:
            len = step*0x840;
            nand_cmd = 0x305;
            break;
        default:
            len = 0;
            nand_cmd = 0;
            break;
    }
#endif
    for(;val>0;val-=step){
        //printf("len==0x%08x,pages==0x%08x\n",len,val);
        dma_config(len,ram,dma_cmd);
        STOP_DMA;
        nand_udelay(10);
        START_DMA(DMA_DESP);
        NAND_WL(TIMING,0x206);
        NAND_WL(OPNUM,len);
        nand_send_cmd(nand_cmd,page);
        nand_udelay(300);
        ret = nand_op_ok(len,ram);
        //nand_udelay(300);
        ram+=len;
        page+=step;
        MYDBG;
        if(ret){
            printf("nandwrite 0x%08x page 0x%08x pages have some error\n",page,pages);
        }
    } 
    return ret;
}
int error_check(u32 ram,u32 flash,u32 len)
{ 
    if(FLASH_TO_PAGE(flash) >= 0xffffffff){
        printf("the FLASH addr is big,this program don't work on this FLASH addr...\n");
        return -1;
    } 
    if(flash & 0x7ff){
        printf("the FLASH addr don't align/* Need 0x800B alignment*/\n");
        return -1;
    }
    if(ram & 0x1f){
        printf("the RAM addr 0x%08x don't align/* Need 32B alignment*/\n",ram);
        return -1;
    }
    if(len < 0 || len > chipsize)
    {
        printf("the LEN 0x%08x(%d) is a unvalid number\n",len,len);
        return -1;
    }
    return 0;
}
int read_nand(u32 ram,u32 flash,u32 len,u8 flag)
{
    u32 page=0;
    u32 ret = 0;
    u32 b_pages = 0,a_pages=0;
    u32 pages = 0,block=0,chunkblock=0,chunkpage=0;
    ret = error_check(ram,flash,len);
    if(ret)
        return ret;
    page = FLASH_TO_PAGE(flash);//start page 
//        printf("%x:page==0x%08x\n",__LINE__,page);
    while(is_bad_block(PAGE_TO_BLOCK(page)))
    {
//        printf("%x:page==0x%08x\n",__LINE__,page);
//        printf("%x:block==0x%08x\n",__LINE__,PAGE_TO_BLOCK(page));
//        printf("block==%x,page==%x\n",PAGE_TO_BLOCK(page),page);
//        MYDBG;
        page+=PAGES_A_BLOCK;
    
        page = (BLOCK_TO_PAGE(PAGE_TO_BLOCK(page)));
//        printf("%x:page==0x%08x\n",__LINE__,page);
        printf("the FLASH addr 0x%08x is a bad block,auto change FLASH addr 0x%08x\n",flash,PAGE_TO_FLASH(page));
    }
    switch(flag){
        case 0:
            chunkblock = 0x20000;
            chunkpage = 0x800;
            break;
        case 1:
            chunkblock = 0x1000;
            chunkpage = 0x40;
            break;
        case 2:
            chunkblock = 0x21000;
            chunkpage = 0x840;
            break;
        default:
            chunkblock = 0;
            chunkpage = 0;
            break;
    }    
    printf("reading ");
    pages = (len + chunkpage - 1)/chunkpage;
//    printf("%x:pages==0x%08x\n",__LINE__,pages);
    b_pages = page % PAGES_A_BLOCK;
    b_pages = (PAGES_A_BLOCK - b_pages);
    if(pages > b_pages && b_pages > 0){
        ret = read_pages(ram,page,b_pages,flag);
        pages -= b_pages;
        page += b_pages;
        ram += (chunkpage * b_pages); 
    }else if(pages <= b_pages && b_pages > 0){
        ret = read_pages(ram,page,pages,flag);
        pages = 0;
    }
    if(ret){return ret;}
    if(pages){
        block = pages / PAGES_A_BLOCK;
        a_pages = pages % PAGES_A_BLOCK;
    }
    for(;block > 0;block--){
        while(is_bad_block(PAGE_TO_BLOCK(page))){
            printf("the FLASH addr 0x%08x is bad block,try next block...\n",PAGE_TO_FLASH(page));
            page += PAGES_A_BLOCK;
        }
        ret = read_pages(ram,page,PAGES_A_BLOCK,flag);
        printf(". ");
        if(ret) {return ret;}
        pages -= PAGES_A_BLOCK;
        page += PAGES_A_BLOCK ;
        ram += chunkblock;
    } 
    if(a_pages){
        while(is_bad_block(PAGE_TO_BLOCK(page))){
            printf("the FLASH addr 0x%08x is bad block,try next block...\n",PAGE_TO_FLASH(page));
            page += PAGES_A_BLOCK;
        }
        ret = read_pages(ram,page,a_pages,flag);
        if(ret){return ret;}
    }
        printf(". done\n");
    return ret;
}
int write_nand(u32 ram,u32 flash,u32 len,u8 flag)
{
    u32 page=0,start_page=0;
    u32 ret = 0;
    u32 b_pages = 0,a_pages=0;
    u32 pages = 0,block=0,chunkblock=0,chunkpage=0;
    ret = error_check(ram,flash,len);
    if(ret)
        return ret;
    page = FLASH_TO_PAGE(flash);//start page 
    while(is_bad_block(PAGE_TO_BLOCK(page)))
    {
        page+=PAGES_A_BLOCK;

        page = (BLOCK_TO_PAGE(PAGE_TO_BLOCK(page)));
        printf("the FLASH addr 0x%08x is a bad block,auto change FLASH addr 0x%08x\n",flash,PAGE_TO_FLASH(page));
    }
    switch(flag){
        case 0:
            chunkblock = 0x20000;
            chunkpage = 0x800;
            break;
        case 1:
            chunkblock = 0x1000;
            chunkpage = 0x40;
            break;
        case 2:
            chunkblock = 0x21000;
            chunkpage = 0x840;
            break;
        default:
            chunkblock = 0;
            chunkpage = 0;
            break;
    }
    printf("writing. ");    
    pages = (len + chunkpage - 1)/chunkpage;
    printf("%x:pages==0x%08x\n",__LINE__,pages);
    start_page = page % PAGES_A_BLOCK;
    if(start_page){
        b_pages = (PAGES_A_BLOCK - start_page);
    }
    if(b_pages > 0){
        ret = read_pages(NAND_TMP,page - start_page,start_page,2);
        if(ret){return ret;}
        erase_block(PAGE_TO_BLOCK(page));
        ret = write_pages(NAND_TMP,page - start_page,start_page,2);
        printf(". ");
        if(ret){return ret;}
    }
    if(pages > b_pages && b_pages > 0){
        ret = write_pages(ram,page,b_pages,flag);
        pages -= b_pages;
        page += b_pages;
        ram += (chunkpage * b_pages); 
    }else if(pages <= b_pages && b_pages > 0){
        ret = write_pages(ram,page,pages,flag);
        pages = 0;
    }
    if(ret){return ret;}
    if(pages){
        block = pages / PAGES_A_BLOCK;
        a_pages = pages % PAGES_A_BLOCK;
    }
 //   printf("block==0x%08x,a_pages==0x%08x\n",block,a_pages);
    for(;block > 0;block--){
        MYDBG
        erase_block(PAGE_TO_BLOCK(page));
        while(is_bad_block(PAGE_TO_BLOCK(page))){
            printf("the FLASH addr 0x%08x is bad block,try next block...\n",PAGE_TO_FLASH(page));
            page += PAGES_A_BLOCK;
            erase_block(PAGE_TO_BLOCK(page));
        }
        MYDBG
        ret = write_pages(ram,page,PAGES_A_BLOCK,flag);
        MYDBG
        printf(". *");
        if(ret) {return ret;}
        pages -= PAGES_A_BLOCK;
        page += PAGES_A_BLOCK ;
        ram += chunkblock;
    } 
        MYDBG
    if(a_pages){
        erase_block(page);
        while(is_bad_block(PAGE_TO_BLOCK(page))){
            printf("the FLASH addr 0x%08x is bad block,try next block...\n",PAGE_TO_FLASH(page));
            page += PAGES_A_BLOCK;
            erase_block(PAGE_TO_BLOCK(page));
        }
        ret = write_pages(ram,page,a_pages,flag);
        if(ret){return ret;}
    }
        printf(". done\n");
    return ret;
}

int erase_nand(u32 start,u32 blocks)
{
    int i=0;
    printf("erasing ");
    for(;i<blocks;i++){
        erase_block(i+start);
        printf(". ");
    }
        printf(" done\n");
    MYDBG
    return 0;
}
#if 0
//int mymain_nand(u8 ops,u32 ram,u32 flash,u32 len,u8 flag)
int mymain_nand(int argc,char **argv)
{
    u8 osp=0,flag=0;
    u32 ram=0,flash=0,u32 len=0;
    printf("\n\t\t*****Nand Flash OPS*****\n\n");
    {
        printf("Uargs:callbin ./bin/nand_ops.bin OPS startblock blocks\n");
        printf("Uargs:callbin ./bin/nand_ops.bin OPS ram flashaddr len flag\n");
        printf("Uargs:flag: 0=main; 1=spare; 2=spare+main\n\n");
        printf("Uargs:OPS:  0=write nand; 1=read nand; 2=erase nand\n\n");
    }
//    printf("\nbuf==0x%08x,ram==0x%08x,flash==0x%08x,len==0x%08x,flag==0x%08x\n",buf,ram,flash,len,flag);

    ops=strtoul(argv[1],0,0);
    ram=strtoul(argv[2],0,0);
    flash=strtoul(argv[3],0,0);
    len=strtoul(argv[4],0,0);
    flag=strtoul(argv[5],0,0);
#if 1
    switch(ops){
        case 2:
            erase_nand(ram,flash);
            printf("erase nand ops OK...\n");
            break;
        case 1:
            read_nand(ram,flash,len,flag);
            printf("read nand ops OK...\n");
            break;
        case 0:
            write_nand(ram,flash,len,flag);
            printf("write nand ops OK...\n");
            break;
        default:
            printf("\ninput invalid OPS(0 erase_nand; 1 read_nand; 2 write_nand)...\n\n");
            break;
    }
#endif

    return 0;
}
#endif
int mynand_erase(int argc,char **argv)
{
    u32 start=0,blocks=0;
    if(argc!=3){
        printf("args error...\n");
        printf("uargs:erase_nand <startblock> <blocks>\n");
        printf("example:erase_nand 1 0x10\n");
        return -1;
    }
    start=strtoul(argv[1],0,0);
    blocks=strtoul(argv[2],0,0);
//    printf("start==0x%08x,blocks==0x%08x\n",start,blocks);
    erase_nand(start,blocks);
    printf("erase nand ops OK...\n");
    return 0;
}
int mynand_read(int argc,char **argv)
{
    u32 ram=0,flash=0,len=0;
    u8 flag=0;
    char *dst=NULL;
    if(argc!=5){
        printf("args error...\n");
        printf("uargs:read_nand <ramaddr>  <flashaddr>  <len> <flag>\n");
        printf("uargs:flag = (m/M(main),s/S(spare),a/A(main+spare))\n");
        printf("example:read_nand 0xa1000000  0x0  0x20000 m\n");
        return -1;
    }
    if(!strncmp(argv[4],"m",1) || !strncmp(argv[4],"M",1)){flag=0;}
    else if(!strncmp(argv[4],"s",1) || !strncmp(argv[4],"S",1)){flag=1;}
    else if(!strncmp(argv[4],"a",1) || !strncmp(argv[4],"A",1)){flag=2;}
    else{flag=0;}
    ram=strtoul(argv[1],0,0);
    flash=strtoul(argv[2],0,0);
    len=strtoul(argv[3],0,0);
//    printf("ram==0x%08x,flash==0x%08x\n",ram,flash,len,flag);
    read_nand(ram,flash,len,flag);
    printf("read nand ops OK...\n");
    return 0;
}
int mynand_write(int argc,char **argv)
{
    u32 ram=0,flash=0,len=0;
    u8 flag=0;
    char *dst=NULL;
    if(argc!=5){
        printf("args error...\n");
        printf("uargs:write_nand <ramaddr>  <flashaddr>  <len>  <flag>\n");
        printf("uargs:flag = (m/M(main),s/S(spare),a/A(main+spare))\n");
        printf("example:write_nand 0xa1000000  0x0  0x20000 m\n");
        return -1;
    }
    if(!strncmp(argv[4],"m",1) || !strncmp(argv[4],"M",1)){flag=0;}
    else if(!strncmp(argv[4],"s",1) || !strncmp(argv[4],"S",1)){flag=1;}
    else if(!strncmp(argv[4],"a",1) || !strncmp(argv[4],"A",1)){flag=2;}
    else{flag=0;}
    ram=strtoul(argv[1],0,0);
    flash=strtoul(argv[2],0,0);
    len=strtoul(argv[3],0,0);
//    printf("ram==0x%08x,flash==0x%08x\n",ram,flash,len,flag);
    write_nand(ram,flash,len,flag);
    printf("write nand ops OK...\n");
    return 0;
}

int my_yaffs_write(int argc, char **argv)
{
    u32 ram=0, flash=0, len=0;
    u8 flag=0;
    if(argc != 4){
        printf("args error...\n");
        printf("uargs:yaffs_write <ramaddr>  <flash_partition>  <len>\n");
        printf("uargs:flash_partition:<mtdblock0/mtd0> <mtdblock1/mtd1>\n");
        printf("example:yaffs_write 0xa1000000  mtdblock0  0x20000\n");
        return -1;
    }
    ram=strtoul(argv[1],0,0);
    len=strtoul(argv[3],0,0);
    if(!strncmp(argv[2],"mtdblock0",9)||!strncmp(argv[2],"mtd0",4)){flash=0x0;}
    if(!strncmp(argv[2],"mtdblock1",9)||!strncmp(argv[2],"mtd1",4)){flash=0x2000000;}
    write_nand(ram,flash,len,2);
    printf("yaffs write nand ops Ok ...\n");
    return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"erase_nand","val",0,"NANDFlash OPS:erase_nand <startblock> <blocks>",mynand_erase,0,99,CMD_REPEAT},
	{"read_nand","val",0,"NANDFlash OPS:read_nand <ramaddr>  <flashaddr>  <len>  <flag(a/A/m/M/s/S)>",mynand_read,0,99,CMD_REPEAT},
	{"write_nand","val",0,"NANDFlash OPS:write_nand <ramaddr>  <flashaddr>  <len>  <flag(a/A/m/M/s/S)>",mynand_write,0,99,CMD_REPEAT},
	{"yaffs_write","val",0,"NANDFlash OPS:yaffs_write <ramaddr>  <flash_partition(mtdblock0/mtdblock1)>  <len>",my_yaffs_write,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}







