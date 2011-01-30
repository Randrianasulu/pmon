#include<pmon.h>
#include<machine/types.h>
#include<linux/mtd/mtd.h>
#include<linux/mtd/nand.h>
#include<linux/mtd/partitions.h>
#include<sys/malloc.h>

#ifndef __iomem
#define __iomem
#endif

#define DMA_ACCESS_ADDR     0x1fe78040
#define ORDER_REG_ADDR      (0xbfd01160)
#define MAX_BUFF_SIZE	4096
#define PAGE_SHIFT      11
#define CHIP_DELAY_TIMEOUT (2*HZ/10)

#define NAND_CMD        0x1
#define NAND_ADDRL      0x2
#define NAND_ADDRH      0x4
#define NAND_TIMING     0x8
#define NAND_IDL        0x10
#define NAND_STATUS_IDL 0x20
#define NAND_PARAM      0x40
#define NAND_OP_NUM     0X80
#define NAND_CS_RDY_MAP 0x100

#define DMA_ORDERAD     0x1
#define DMA_SADDR       0x2
#define DMA_DADDR       0x4
#define DMA_LENGTH      0x8
#define DMA_STEP_LENGTH 0x10
#define DMA_STEP_TIMES  0x20
#define DMA_CMD         0x40


enum{
    ERR_NONE        = 0,
    ERR_DMABUSERR   = -1,
    ERR_SENDCMD     = -2,
    ERR_DBERR       = -3,
    ERR_BBERR       = -4,
};
enum{
    STATE_READY = 0,
    STATE_BUSY  ,
};

struct ls1g_nand_platform_data{
        int enable_arbiter;
        struct mtd_partition *parts;
        unsigned int nr_parts;
};
struct ls1g_nand_cmdset {
        uint32_t    cmd_valid:1;
	uint32_t    read:1;
	uint32_t    write:1;
	uint32_t    erase_one:1;
	uint32_t    erase_con:1;
	uint32_t    read_id:1;
	uint32_t    reset:1;
	uint32_t    read_sr:1;
	uint32_t    op_main:1;
	uint32_t    op_spare:1;
	uint32_t    done:1;
        uint32_t    resv1:5;//11-15 reserved
        uint32_t    nand_rdy:4;//16-19
        uint32_t    nand_ce:4;//20-23
        uint32_t    resv2:8;//24-32 reserved
};
#if 0
struct ls1g_nand_flash {
//	struct ls1g_nand_timing *timing; /* NAND Flash timing */
	struct ls1g_nand_cmdset *cmdset;

	uint32_t page_per_block;/* Pages per block (PG_PER_BLK) */
	uint32_t page_size;	/* Page size in bytes (PAGE_SZ) */
	uint32_t flash_width;	/* Width of Flash memory (DWIDTH_M) */
	uint32_t dfc_width;	/* Width of flash controller(DWIDTH_C) */
	uint32_t num_blocks;	/* Number of physical blocks in Flash */
	uint32_t chip_id;

	/* NOTE: these are automatically calculated, do not define */
	size_t		oob_size;
	size_t		read_id_bytes;
};
#endif
struct ls1g_nand_dma_desc{
        uint32_t    orderad;
        uint32_t    saddr;
        uint32_t    daddr;
        uint32_t    length;
        uint32_t    step_length;
        uint32_t    step_times;
        uint32_t    cmd;
};
struct ls1g_nand_dma_cmd{
        uint32_t    dma_int_mask:1;
        uint32_t    dma_int:1;
        uint32_t    dma_sl_tran_over:1;
        uint32_t    dma_tran_over:1;
        uint32_t    dma_r_state:4;
        uint32_t    dma_w_state:4;
        uint32_t    dma_r_w:1;
        uint32_t    dma_cmd:2;
        uint32_t    revl:17;
};
struct ls1g_nand_desc{
        uint32_t    cmd;
        uint32_t    addrl;
        uint32_t    addrh;
        uint32_t    timing;
        uint32_t    idl;//readonly
        uint32_t    status_idh;//readonly
        uint32_t    param;
        uint32_t    op_num;
        uint32_t    cs_rdy_map;
};
struct ls1g_nand_info {
	struct nand_chip	nand_chip;

//	struct platform_device	    *pdev;
        /* MTD data control*/
	unsigned int 		buf_start;
	unsigned int		buf_count;
        /* NAND registers*/
	void __iomem		*mmio_base;
        struct ls1g_nand_desc   nand_regs;
        unsigned int            nand_addrl;
        unsigned int            nand_addrh;
        unsigned int            nand_timing;
        unsigned int            nand_op_num;
        unsigned int            nand_cs_rdy_map;
        unsigned int            nand_cmd;

	/* DMA information */

        struct ls1g_nand_dma_desc  dma_regs;
        unsigned int            order_reg_addr;  
        unsigned int            dma_orderad;
        unsigned int            dma_saddr;
        unsigned int            dma_daddr;
        unsigned int            dma_length;
        unsigned int            dma_step_length;
        unsigned int            dma_step_times;
        unsigned int            dma_cmd;
        unsigned int		drcmr_dat;//dma descriptor address;
	unsigned int 		drcmr_dat_phys;
        size_t                  drcmr_dat_size;
	unsigned char		*data_buff;//dma data buffer;
	unsigned int 		data_buff_phys;
	size_t			data_buff_size;
        unsigned int            data_ask;
        unsigned int            data_ask_phys;
        unsigned int            data_length;
	/* relate to the command */
	unsigned int		state;
//	int			use_ecc;	/* use HW ECC ? */
	size_t			data_size;	/* data size in FIFO */
        unsigned int            cmd;
//	struct completion 	cmd_complete;
        unsigned int            seqin_column;
        unsigned int            seqin_page_addr;
};

struct ls1g_nand_ask_regs{
        unsigned int dma_length;
        unsigned int dma_mem_addr;
        unsigned int dma_dev_addr;
        unsigned int dma_order_addr;
        unsigned int dma_step_times;
        unsigned int dma_step_length;
        unsigned int dma_state_tmp;
};

static struct mtd_info *ls1g_soc_mtd = NULL;
#if 0
struct mtd_info *_soc_mtd = NULL;
#define KERNEL_AREA_SIZE 32*1024*1024
 const struct mtd_partition partition_info[] = {
//	{name ,size,offset,mask_flags }
        {"kernel",KERNEL_AREA_SIZE,0,0},
        {"os",0,KERNEL_AREA_SIZE,0},
        {(void *)0,0,0,0}
};
#endif
static struct nand_ecclayout hw_largepage_ecclayout = {
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};
#define show_data_debug  1
#define show_debug(x,y)    // show_debug_msk(x,y)
#define show_debug_msk(x,y)   do{ if(show_data_debug) {printk(KERN_ERR "%s:\n",__func__);show_data(x,y);} }while(0)

static void show_data(void * base,int num)
{
    int i=0;
    unsigned char *arry=( unsigned char *) base;
//    printk(KERN_ERR "%s: \n",__func__);
    for(i=0;i<num;i++){
        if(!(i % 32)){
            printk(KERN_ERR "\n");
        }
        if(!(i % 16)){
            printk("  ");
        }
        printk("%02x ",arry[i]);
    }
    printk(KERN_ERR "\n");
    
}



static int ls1g_nand_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	return 0;
}
static int ls1g_nand_ecc_correct(struct mtd_info *mtd,
		uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct ls1g_nand_info *info = mtd->priv;
	/*
	 * Any error include ERR_SEND_CMD, ERR_DBERR, ERR_BUSERR, we
	 * consider it as a ecc error which will tell the caller the
	 * read fail We have distinguish all the errors, but the
	 * nand_read_ecc only check this function return value
	 */
	return 0;
}

static void ls1g_nand_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	return;
}




static int ls1g_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
    udelay(50);
    return 0;
}
static void ls1g_nand_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}
static int ls1g_nand_dev_ready(struct mtd_info *mtd)
{
	return 1;
}
static void ls1g_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
        struct ls1g_nand_info *info = mtd->priv;
        int i,real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);

        show_debug(info->data_buff,0x40);

        info->buf_start += real_len;
}
static u16 ls1g_nand_read_word(struct mtd_info *mtd)
{
        struct ls1g_nand_info *info = mtd->priv;
        u16 retval = 0xFFFF;
        if(!(info->buf_start & 0x1) && info->buf_start < info->buf_count){
            retval = *(u16 *)(info->data_buff + info->buf_start);
        }
        info->buf_start += 2;
        return retval;


}
static uint8_t ls1g_nand_read_byte(struct mtd_info *mtd)
{
        struct ls1g_nand_info *info = mtd->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		/* Has just send a new command? */
		retval = info->data_buff[(info->buf_start)++];
        show_debug(info->data_buff,6);
	return retval;
}
static void ls1g_nand_write_buf(struct mtd_info *mtd,const uint8_t *buf, int len)
{
        int i;
        struct ls1g_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);
//        info->buf_count = real_len;

	memcpy(info->data_buff + info->buf_start, buf, real_len);
            show_debug(info->data_buff,0x20);
	info->buf_start += real_len;
}
static int ls1g_nand_verify_buf(struct mtd_info *mtd,const uint8_t *buf, int len)
{
	return 0;
}
static void ls1g_nand_cmdfunc(struct mtd_info *mtd, unsigned command,int column, int page_addr);
static void ls1g_nand_init_mtd(struct mtd_info *mtd,struct ls1g_nand_info *info);

int ls1g_nand_init(struct mtd_info *mtd)
{
    int ret=0;
    ret = ls1g_nand_pmon_info_init(mtd->priv,mtd);
    ls1g_nand_init_mtd(mtd,(struct ls1g_nand_info *)(mtd->priv));
    return ret;
}
static void ls1g_nand_init_mtd(struct mtd_info *mtd,struct ls1g_nand_info *info)
{
	struct nand_chip *this = &info->nand_chip;

	this->options = 8;//(f->flash_width == 16) ? NAND_BUSWIDTH_16: 0;

	this->waitfunc		= ls1g_nand_waitfunc;
	this->select_chip	= ls1g_nand_select_chip;
	this->dev_ready		= ls1g_nand_dev_ready;
	this->cmdfunc		= ls1g_nand_cmdfunc;
	this->read_word		= ls1g_nand_read_word;
	this->read_byte		= ls1g_nand_read_byte;
	this->read_buf		= ls1g_nand_read_buf;
	this->write_buf		= ls1g_nand_write_buf;
	this->verify_buf	= ls1g_nand_verify_buf;

        this->ecc.mode		= NAND_ECC_HW;
	this->ecc.hwctl		= ls1g_nand_ecc_hwctl;
	this->ecc.calculate	= ls1g_nand_ecc_calculate;
	this->ecc.correct	= ls1g_nand_ecc_correct;
	this->ecc.size		= 2048;
        this->ecc.bytes         = 24;

//	this->ecc.layout = &hw_largepage_ecclayout;
//        mtd->owner = THIS_MODULE;
}

static unsigned ls1g_nand_status(struct ls1g_nand_info *info)
{
    struct ls1g_nand_desc *nand_regs = (volatile struct ls1g_nand_desc *)(info->mmio_base);
    struct ls1g_nand_cmdset *nand_cmd = (struct ls1g_nand_cmdset *)(&(nand_regs->cmd));
    udelay(100);
    return(nand_cmd->done);
}

/*
 *  flags & 0x1   orderad
 *  flags & 0x2   saddr
 *  flags & 0x4   daddr
 *  flags & 0x8   length
 *  flags & 0x10  step_length
 *  flags & 0x20  step_times
 *  flags & 0x40  cmd
 ***/
static void dma_setup(unsigned int flags,struct ls1g_nand_info *info)
{
    struct ls1g_nand_dma_desc *dma_base = (volatile struct ls1g_nand_dma_desc *)(info->drcmr_dat);
        dma_base->orderad = (flags & DMA_ORDERAD)== DMA_ORDERAD ? info->dma_regs.orderad : info->dma_orderad;
        dma_base->saddr = (flags & DMA_SADDR)== DMA_SADDR ? info->dma_regs.saddr : info->dma_saddr;
        dma_base->daddr = (flags & DMA_DADDR)== DMA_DADDR ? info->dma_regs.daddr : info->dma_daddr;
        dma_base->length = (flags & DMA_LENGTH)== DMA_LENGTH ? info->dma_regs.length: info->dma_length;
        info->data_length = info->dma_regs.length;
        dma_base->step_length = (flags & DMA_STEP_LENGTH)== DMA_STEP_LENGTH ? info->dma_regs.step_length: info->dma_step_length;
        dma_base->step_times = (flags & DMA_STEP_TIMES)== DMA_STEP_TIMES ? info->dma_regs.step_times: info->dma_step_times;
        dma_base->cmd = (flags & DMA_CMD)== DMA_CMD ? info->dma_regs.cmd: info->dma_cmd; 
        *(volatile unsigned int *)info->order_reg_addr = ((unsigned int )info->drcmr_dat_phys) | 0x1<<3;
        memset(&(info->dma_regs),0,sizeof(struct ls1g_nand_dma_desc));
}
static void dma_ask(struct ls1g_nand_info *info)
{
    memset((char *)info->data_ask,0,sizeof(struct ls1g_nand_ask_regs));
    *(volatile unsigned int *)info->order_reg_addr = 0x1<<2|(info->data_ask_phys)& 0xfffffff0;
}
/**
 *  flags & 0x1     cmd
 *  flags & 0x2     addrl
 *  flags & 0x4     addrh
 *  flags & 0x8     timing
 *  flags & 0x10    idl
 *  flags & 0x20    status_idh
 *  flags & 0x40    param
 *  flags & 0x80    op_num
 *  flags & 0x100   cs_rdy_map
 ****/
static void nand_setup(unsigned int flags ,struct ls1g_nand_info *info)
{
//    printk("addrl+++++++++++++++++++++==%x\n\n",info->nand_regs.addrl);
    struct ls1g_nand_desc *nand_base = (struct ls1g_nand_desc *)(info->mmio_base);
    nand_base->addrl = (flags & NAND_ADDRL)==NAND_ADDRL ? info->nand_regs.addrl: info->nand_addrl;
    nand_base->addrh = (flags & NAND_ADDRH)==NAND_ADDRH ? info->nand_regs.addrh: info->nand_addrh;
    nand_base->timing = (flags & NAND_TIMING)==NAND_TIMING ? info->nand_regs.timing: info->nand_timing;
    nand_base->op_num = (flags & NAND_OP_NUM)==NAND_OP_NUM ? info->nand_regs.op_num: info->nand_op_num;
    nand_base->cs_rdy_map = (flags & NAND_CS_RDY_MAP)==NAND_CS_RDY_MAP ? info->nand_regs.cs_rdy_map: info->nand_cs_rdy_map;
    if(flags & NAND_CMD){
        nand_base->cmd = info->nand_regs.cmd;
        if(info->nand_regs.cmd & 0x20){
            while(!ls1g_nand_status(info));
            *(int *)(info->data_buff) = nand_base->idl;
        }
    }
    else
        nand_base->cmd = info->nand_cmd;
    memset(&(info->nand_regs),0,sizeof(struct ls1g_nand_desc));
    
}
static  int sync_dma(struct ls1g_nand_info *info)
{
    int *end; 
        end =( (unsigned int )(info->data_buff_phys)&0x1fffffff) + info->data_length * 4;
    struct ls1g_nand_ask_regs *ask = info->data_ask;
    while(1){
        dma_ask(info);
        udelay(100);
//       printf("\n\nask->dma_mem_addr===0x%08x,0x%08x\n\n",(int)&(ask->dma_mem_addr),ask->dma_mem_addr); 
//        printf("\n\norder_mem ================= 0x%08x,0x%08x,0x%08x\n\n",ask->dma_mem_addr,end,ask->dma_length);
        if(ask->dma_mem_addr == end)
            break;
    }
        return 0;
}
static void ls1g_nand_cmdfunc(struct mtd_info *mtd, unsigned command,int column, int page_addr)
{
        struct ls1g_nand_info *info = mtd->priv;
        int ret,i,nandcmd;
//        int timeout = CHIP_DELAY_TIMEOUT;
        unsigned int base;
//        init_completion(&info->cmd_complete);
        info->cmd = command;
        switch(command){
            case NAND_CMD_READOOB:
                info->state = STATE_BUSY; 
                info->buf_count = mtd->oobsize - column;
                info->buf_start = 0;
                if(info->buf_count <=0 )
                    break;
                /*nand regs set*/
                info->nand_regs.addrh =  page_addr >> (32 - PAGE_SHIFT);
                info->nand_regs.addrl = (page_addr << PAGE_SHIFT) + column + 2048;
                info->nand_regs.op_num = info->buf_count;
               /*nand cmd set */ 
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;
                /*dma regs config*/
                info->dma_regs.length = (info->buf_count + 3)/4;
                ((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
                /*dma GO set*/       
                nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
                dma_setup(DMA_LENGTH|DMA_CMD,info);
                sync_dma(info);
                break;
            case NAND_CMD_READ0:
                info->state = STATE_BUSY;
                info->buf_count = mtd->oobsize + mtd->writesize - column;
                info->buf_start = 0;
                if(info->buf_count <=0 )
                    break;
                info->nand_regs.addrh =  page_addr >> (32 - PAGE_SHIFT);
                info->nand_regs.addrl = (page_addr << PAGE_SHIFT) + column ;
                info->nand_regs.op_num = info->buf_count;
               /*nand cmd set */ 
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_main = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
                /*dma regs config*/
                info->dma_regs.length = (info->buf_count + 3)/4;
                ((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
                dma_setup(DMA_LENGTH|DMA_CMD,info);
                nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
                sync_dma(info);
                break;
            case NAND_CMD_SEQIN:
                info->buf_count = mtd->oobsize + mtd->writesize ;
                info->buf_start = 0;
                info->seqin_column = column;
		info->seqin_page_addr = page_addr;
//                complete(&info->cmd_complete);
                break;
            case NAND_CMD_PAGEPROG:
                info->state = STATE_BUSY;
                if(info->buf_count <= 0 )
                    break;
                   /*nand regs set*/
                info->nand_regs.addrh =  info->seqin_page_addr >> (32 - PAGE_SHIFT);
                info->nand_regs.addrl = (info->seqin_page_addr << PAGE_SHIFT) + info->seqin_column ;
                info->nand_regs.op_num = info->buf_start;
               /*nand cmd set */ 
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->write = 1;
//                if(info->seqin_column == (1 << PAGE_SHIFT) || info->buf_count <= 0x40 || info->buf_count > (1 << PAGE_SHIFT))
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_spare = 1;
                if(info->seqin_column == 0)
                    ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->op_main = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
                /*dma regs config*/
                info->dma_regs.length = (info->buf_start + 3)/4;
                ((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_int_mask = 0;
                ((struct ls1g_nand_dma_cmd *)&(info->dma_regs.cmd))->dma_r_w = 1;
                nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
                dma_setup(DMA_LENGTH|DMA_CMD,info);
                sync_dma(info);
                break;
            case NAND_CMD_RESET:
                info->state = STATE_BUSY;
               /*nand cmd set */ 
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->reset = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
                nand_setup(NAND_CMD,info);
//                while(!ls1g_nand_status(info))
                    udelay(10);
                info->state = STATE_READY;
//                complete(&info->cmd_complete);
                break;
            case NAND_CMD_ERASE1:
                info->state = STATE_BUSY;
                   /*nand regs set*/
                info->nand_regs.addrh =  page_addr >> (32 - PAGE_SHIFT);
                info->nand_regs.addrl = (page_addr << PAGE_SHIFT) ;
                info->nand_regs.op_num = 1;
               /*nand cmd set */ 
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->erase_one = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1; 
                nand_setup(NAND_ADDRL|NAND_ADDRH|NAND_OP_NUM|NAND_CMD,info);
//                while(!ls1g_nand_status(info)){
                    udelay(30);    
//                }
                info->state = STATE_READY;
//                complete(&info->cmd_complete);
                break;
            case NAND_CMD_STATUS:
                info->buf_count = 0x1;
                info->buf_start = 0x0;
                *(unsigned char *)info->data_buff=ls1g_nand_status(info) | 0x80;
//                complete(&info->cmd_complete);
                break;
            case NAND_CMD_READID:
                info->state = STATE_BUSY;
                info->buf_count = 0x4;
                info->buf_start = 0;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->read_id = 1;
                ((struct ls1g_nand_cmdset*)&(info->nand_regs.cmd))->cmd_valid = 1;             
                nand_setup(NAND_CMD,info);
//                while(!ls1g_nand_status(info));
                info->state = STATE_READY;
//                complete(&info->cmd_complete);
                break;
            case NAND_CMD_ERASE2:
            case NAND_CMD_READ1:
//                complete(&info->cmd_complete);
                break;
            default :
                printk(KERN_ERR "non-supported command.\n");
//                complete(&info->cmd_complete);
		break;
        }
//            wait_for_completion_timeout(&info->cmd_complete,timeout);
}

int ls1g_nand_detect(struct mtd_info *mtd)
{
        printf("NANDFlash info:\nerasesize\t%d B\nwritesize\t%d B\noobsize  \t%d B\n",mtd->erasesize, mtd->writesize,mtd->oobsize );
        return (mtd->erasesize != 1<<17 || mtd->writesize != 1<<11 || mtd->oobsize != 1<<6);

}
static void ls1g_nand_init_info(struct ls1g_nand_info *info)
{
    info->nand_addrl = 0x0;
    info->nand_addrh = 0x0;
    info->nand_timing = 0x4<<8 | 0x12;
    info->nand_op_num = 0x0;
    info->nand_cs_rdy_map = 0x88442200;
    info->nand_cmd = 0;

    info->dma_orderad = 0;
    info->dma_saddr = info->data_buff_phys;
    info->dma_daddr = DMA_ACCESS_ADDR;
    info->dma_length = 0x0;
    info->dma_step_length = 0x0;
    info->dma_step_times = 0x1;
    info->dma_cmd = 0x0;

    info->order_reg_addr = ORDER_REG_ADDR;
}
int ls1g_nand_pmon_info_init(struct ls1g_nand_info *info,struct mtd_info *mtd)
{	
        
        info->drcmr_dat =  malloc(sizeof(struct ls1g_nand_dma_desc),M_DMAMAP,M_WAITOK);
        if(info->drcmr_dat == NULL)
            return -1;
	info->drcmr_dat_phys = (info->drcmr_dat) & 0x1fffffff;

        info->mmio_base = 0x1fe78000 | 0xa0000000;
    	
        info->data_buff = malloc(MAX_BUFF_SIZE,M_DMAMAP,M_WAITOK);
        if(info->data_buff == NULL)
            return -1;
        info->data_buff_phys = (unsigned int)(info->data_buff) & 0x1fffffff;
        
        info->data_ask = malloc(sizeof(struct ls1g_nand_ask_regs),M_DMAMAP,M_WAITOK);
        if(info->data_ask ==NULL)
            return -1;
        info->data_ask_phys = info->data_ask & 0x1fffffff;


        ls1g_nand_init_info(info);
/*
        if(ls1g_nand_detect(mtd)){
                printk(KERN_ERR "PMON driver don't support the NANDFlash!\n");
                return -1;
        }
*/        
	return 0;

}

static void find_good_part(struct mtd_info *ls1g_soc_mtd)
{
int offs;
int start=-1;
char name[20];
int idx=0;
for(offs=0;offs< ls1g_soc_mtd->size;offs+=ls1g_soc_mtd->erasesize)
{
if(ls1g_soc_mtd->block_isbad(ls1g_soc_mtd,offs)&& start>=0)
{
	sprintf(name,"g%d",idx++);
	add_mtd_device(ls1g_soc_mtd,start,offs-start,name);
	start=-1;
}
else if(start<0)
{
 start=offs;
}

}

if(start>=0)
{
	sprintf(name,"g%d",idx++);
	add_mtd_device(ls1g_soc_mtd,start,offs-start,name);
}
}


int ls1g_soc_nand_init(void)
{
	struct nand_chip *this;

	/* Allocate memory for MTD device structure and private data */
	ls1g_soc_mtd = malloc(sizeof(struct mtd_info) + sizeof(struct nand_chip),M_DEVBUF,M_WAITOK);
	if (!ls1g_soc_mtd) {
		printk("Unable to allocate fcr_soc NAND MTD device structure.\n");
		return -ENOMEM;
	}
	/* Get pointer to private data */
	this = (struct nand_chip *)(&ls1g_soc_mtd[1]);

	/* Initialize structures */
	memset(ls1g_soc_mtd, 0, sizeof(struct mtd_info));
	memset(this, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	ls1g_soc_mtd->priv = this;
	/* 15 us command delay time */
	this->chip_delay = 15;
        if(ls1g_nand_init(ls1g_soc_mtd)){
            printf("\n\nerror: PMON nandflash driver have some error!\n\n");
            return -ENXIO;
        }
	/* Scan to find existence of the device */
	if (nand_scan(ls1g_soc_mtd, 1)) {
		free(ls1g_soc_mtd,M_DEVBUF);
		return -ENXIO;
	}
        if(ls1g_nand_detect(ls1g_soc_mtd)){
            printf("error: PMON driver don't support the NANDFlash!\n ");
            return -ENXIO;
        }       

	/* Register the partitions */
//	add_mtd_partitions(fcr_soc_mtd, partition_info, NUM_PARTITIONS);
	add_mtd_device(ls1g_soc_mtd,0,0,"total");
	add_mtd_device(ls1g_soc_mtd,0,0x2000000,"kernel");
	add_mtd_device(ls1g_soc_mtd,0x2000000,0,"os");

	find_good_part(ls1g_soc_mtd);


	/* Return happy */
	return 0;
}

