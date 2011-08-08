#include <pmon.h>
static int mmio=0;
#define GPIO_DIR_REG 		(volatile unsigned int *)(mmio + 0x1000c)
#define GPIO_DATA_REG		(volatile unsigned int *)(mmio + 0x10004)
#define G_OUTPUT		1
#define G_INPUT			0
#define GPIO_SDA_DIR_SHIFT	15
#define	GPIO_SCL_DIR_SHIFT	14
#define GPIO_SDA_DATA_SHIFT	15
#define GPIO_SCL_DATA_SHIFT	14

#define GPIO_CONF_DONE	(1<<17)
#define GPIO_DCLK	(1<<18)
#define GPIO_nCONFIG	(1<<19)
#define GPIO_DATA	(1<<20)
#define GPIO_nSTATUS	(1<<21)

static int fpga_init()
{
pcitag_t tag;
int tmp;
		if(!mmio)
		{
		tag=_pci_make_tag(0,14,0);
	
		mmio = _pci_conf_readn(tag,0x14,4);
		mmio =(int)mmio|(0xb0000000);
		tmp = *(volatile int *)(mmio + 0x40);
		*(volatile int *)(mmio + 0x40) =tmp|0x40;
		
		}
	*GPIO_DATA_REG = (*GPIO_DATA_REG & ~GPIO_DCLK)| GPIO_nCONFIG|GPIO_DATA;
	*GPIO_DIR_REG =  (*GPIO_DIR_REG & ~( GPIO_CONF_DONE | GPIO_nSTATUS)) | GPIO_DCLK|GPIO_nCONFIG|GPIO_DATA;
		return 0;
}

static int cmd_fpga(int argc,char **argv)
{
int fd;
unsigned char c,val0;
fpga_init();
val0 =  *GPIO_DATA_REG & ~(GPIO_DCLK|GPIO_DATA);
#define set_gpio(clk,val) *GPIO_DATA_REG = val0|(clk?GPIO_DCLK:0)|(val?GPIO_DATA:0);


	fd = open(argv[1],O_RDONLY);

		*GPIO_DATA_REG &= ~GPIO_nCONFIG;
		while(*GPIO_DATA_REG & GPIO_nSTATUS) delay(1);
		delay(10);
		*GPIO_DATA_REG |= GPIO_nCONFIG;
		delay(10);
		while(*GPIO_DATA_REG & GPIO_nSTATUS == 0) delay(1);
		delay(1);


	while(read(fd,&c,1) == 1)
	{
		int i;
		for(i=0;i<8;i++)
		{
		set_gpio(0,c&1);
		delay(100);
		set_gpio(1,c&1);
		c = c>>1;
		}

	}
		close(fd);
}

