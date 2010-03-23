/** \file
 * This is the network dependent layer to handle network related functionality.
 * This file is tightly coupled to neworking frame work of linux 2.6.xx kernel.
 * The functionality carried out in this file should be treated as an example only
 * if the underlying operating system is not Linux. 
 * 
 * \note Many of the functions other than the device specific functions
 *  changes for operating system other than Linux 2.6.xx
 * \internal 
 *-----------------------------REVISION HISTORY-----------------------------------
 * Synopsys			01/Aug/2007				Created
 */

/*
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>


#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
*/


#include "synopGMAC_Host.h"
#include "synopGMAC_plat.h"
#include "synopGMAC_network_interface.h"
#include "synopGMAC_Dev.h"


//static struct timer_list synopGMAC_cable_unplug_timer;
static u32 GMAC_Power_down; // This global variable is used to indicate the ISR whether the interrupts occured in the process of powering down the mac or not


struct synopGMACNetworkAdapter * synopGMACadapter_0;
struct synopGMACNetworkAdapter * synopGMACadapter_1;

//u32 synopGMACMappedAddr = 0xbfe10000;
u64 synopGMACMappedAddr = 0; // this is of no use in this driver! liyifu on 2010-01-12
u32 synop_pci_using_dac = 0;
//zgj #define MAC_ADDR {0x00, 0x55, 0x7B, 0xB5, 0x7D, 0xF7}	//sw: may be it should be F7 7D B5 7B 55 00
#define MAC_ADDR {0x00, 0x55, 0x7B, 0x98, 0x7D, 0xF7}	//sw: may be it should be F7 7D B5 7B 55 00

u32 regbase = 0xbfe10000;	//sw: not used
//zgj char mac_addr[6] = {0x00,0x55,0x7B,0xB5,0x7D,0xF7};
char mac_addr[6] = {0x00,0x55,0x7B,0x98,0x7D,0xF7};

void dumppkghd(struct ether_header *eh,int tp);
int set_lpmode(synopGMACdevice * gmacdev);

unsigned int rx_test_count = 0;
unsigned int tx_normal_test_count = 0;
unsigned int tx_abnormal_test_count = 0;
unsigned int tx_stopped_test_count = 0;

unsigned int rxcnt = 0;
unsigned int txcnt = 0;
unsigned long long time_cnt = 0;

/*Sample Wake-up frame filter configurations*/

u32 synopGMAC_wakeup_filter_config0[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x00000000,	// For Filter1 CRC is not computed may be it is 0x0000
					0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
					0x5F5F5F5F,     // For Filter3 CRC is based on 0,1,2,3,4,6,8,9,10,11,12,14,16,17,18,19,20,22,24,25,26,27,28,30 bytes from offset
					0x09000000,     // Filter 0,1,2 are disabled, Filter3 is enabled and filtering applies to only multicast packets
					0x1C000000,     // Filter 0,1,2 (no significance), filter 3 offset is 28 bytes from start of Destination MAC address 
					0x00000000,     // No significance of CRC for Filter0 and Filter1
					0xBDCC0000      // No significance of CRC for Filter2, Filter3 CRC is 0xBDCC
					};
u32 synopGMAC_wakeup_filter_config1[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x00000000,	// For Filter1 CRC is not computed may be it is 0x0000
					0x7A7A7A7A,	// For Filter2 CRC is based on 1,3,4,5,6,9,11,12,13,14,17,19,20,21,25,27,28,29,30 bytes from offset
					0x00000000,     // For Filter3 CRC is not computed may be it is 0x0000
					0x00010000,     // Filter 0,1,3 are disabled, Filter2 is enabled and filtering applies to only unicast packets
					0x00100000,     // Filter 0,1,3 (no significance), filter 2 offset is 16 bytes from start of Destination MAC address 
					0x00000000,     // No significance of CRC for Filter0 and Filter1
					0x0000A0FE      // No significance of CRC for Filter3, Filter2 CRC is 0xA0FE
					};
u32 synopGMAC_wakeup_filter_config2[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x000000FF,	// For Filter1 CRC is computed on 0,1,2,3,4,5,6,7 bytes from offset
					0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
					0x00000000,     // For Filter3 CRC is not computed may be it is 0x0000
					0x00000100,     // Filter 0,2,3 are disabled, Filter 1 is enabled and filtering applies to only unicast packets
					0x0000DF00,     // Filter 0,2,3 (no significance), filter 1 offset is 223 bytes from start of Destination MAC address 
					0xDB9E0000,     // No significance of CRC for Filter0, Filter1 CRC is 0xDB9E
					0x00000000      // No significance of CRC for Filter2 and Filter3 
					};

/*
The synopGMAC_wakeup_filter_config3[] is a sample configuration for wake up filter. 
Filter1 is used here
Filter1 offset is programmed to 50 (0x32)
Filter1 mask is set to 0x000000FF, indicating First 8 bytes are used by the filter
Filter1 CRC= 0x7EED this is the CRC computed on data 0x55 0x55 0x55 0x55 0x55 0x55 0x55 0x55

Refer accompanied software DWC_gmac_crc_example.c for CRC16 generation and how to use the same.
*/

u32 synopGMAC_wakeup_filter_config3[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x000000FF,	// For Filter1 CRC is computed on 0,1,2,3,4,5,6,7 bytes from offset
					0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
					0x00000000,     // For Filter3 CRC is not computed may be it is 0x0000
					0x00000100,     // Filter 0,2,3 are disabled, Filter 1 is enabled and filtering applies to only unicast packets
					0x00003200,     // Filter 0,2,3 (no significance), filter 1 offset is 50 bytes from start of Destination MAC address 
					0x7eED0000,     // No significance of CRC for Filter0, Filter1 CRC is 0x7EED, 
					0x00000000      // No significance of CRC for Filter2 and Filter3 
					};
/**
 * Function used to detect the cable plugging and unplugging.
 * This function gets scheduled once in every second and polls
 * the PHY register for network cable plug/unplug. Once the 
 * connection is back the GMAC device is configured as per
 * new Duplex mode and Speed of the connection.
 * @param[in] u32 type but is not used currently. 
 * \return returns void.
 * \note This function is tightly coupled with Linux 2.6.xx.
 * \callgraph
 */


static int rtl8211_config_init(synopGMACdevice *gmacdev, u32 sel)
{
	int retval, err;
	u16 data;

	data = 0x6400;
	//data = 0x0;

	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x12,data, sel);
	
#if SYNOP_PHY_FORCELINK
//sw: set manual master/slave
	
	printf("phy force link master\n");
	
//sw: set-an slave	
	err = synopGMAC_read_phy_reg(0x5,1,0x9, &data,0);
	data = data | 0x1c00;
	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x9,data, sel);

//sw: set-an 10m
	err = synopGMAC_read_phy_reg(0x5,1,0x4, &data,0);
	data = data & 0xfe7f;
	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x4,data, sel);

//sw: set line-mdi mode	
	err = synopGMAC_read_phy_reg(0x5,1,0x10, &data,0);
	data = data & 0xff9f;
	data = data | 0x20;
	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x10,data, sel);
	
//sw: restart an	
/*
	err = synopGMAC_read_phy_reg(0x5,1,0x0, &data,0);
	data = data | 0x0200;
	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x0,data, sel);
*/
//sw: reset phy
	err = synopGMAC_read_phy_reg(0x5,1,0x0, &data,0);
	data = data | 0x8000;
	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x0,data, sel);

#endif

	
#if SYNOP_PHY_LOOPBACK
	data = 0x5140;
	err = synopGMAC_write_phy_reg((u64 *)gmacdev->MacBase,gmacdev->PhyBase,0x00,data ,sel);
#endif
	if (err < 0)
		return err;
	return 0;
}


static void synopGMAC_linux_powerup_mac(synopGMACdevice *gmacdev, u32 sel)
{
	GMAC_Power_down = 0;	// Let ISR know that MAC is out of power down now
	if( synopGMAC_is_magic_packet_received(gmacdev, sel))
		TR("GMAC wokeup due to Magic Pkt Received\n");
	if(synopGMAC_is_wakeup_frame_received(gmacdev, sel))
		TR("GMAC wokeup due to Wakeup Frame Received\n");
	//Disable the assertion of PMT interrupt
	synopGMAC_pmt_int_disable(gmacdev, sel);
	//Enable the mac and Dma rx and tx paths
	synopGMAC_rx_enable(gmacdev, sel);
       	synopGMAC_enable_dma_rx(gmacdev, sel);

	synopGMAC_tx_enable(gmacdev, sel);
	synopGMAC_enable_dma_tx(gmacdev, sel);
	return;
}


/**
  * This sets up the transmit Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures for ring mode or chain mode.
  * This function depends on the pcidev structure for allocation consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in tx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->TxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements  
  */

s32 synopGMAC_setup_tx_desc_queue(synopGMACdevice * gmacdev,u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc * bf1;

	DmaDesc *first_desc = NULL;
	//DmaDesc *second_desc = NULL;
	//dma_addr_t dma_addr;
	gmacdev->TxDescCount = 0;

	TR("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n",((sizeof(DmaDesc) * no_of_desc)));
	//	first_desc = plat_alloc_consistent_dmaable_memory (pcidev, sizeof(DmaDesc) * no_of_desc,&dma_addr);
	first_desc = (DmaDesc *)plat_alloc_memory(sizeof(DmaDesc) * no_of_desc+15);	//sw: 128 aligned
//sw: map to uncached addr	
//	first_desc = CACHED_TO_UNCACHED((unsigned long)(first_desc));
	
	if(first_desc == NULL){
		TR("Error in Tx Descriptors memory allocation\n");
		return -ESYNOPGMACNOMEM;
	}
	first_desc = ((u32)first_desc) & ~15;


	//	memset((u32)first_desc,0, sizeof(DmaDesc) * no_of_desc);
	gmacdev->TxDescCount = no_of_desc;
	gmacdev->TxDesc      = first_desc;

	bf1  = (DmaDesc *)CACHED_TO_UNCACHED((unsigned long)(gmacdev->TxDesc));	
	//gmacdev->TxDescDma  = (unsigned long)vtophys((unsigned long)bf1);
	gmacdev->TxDescDma  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)bf1);
	//gmacdev->TxDesc     =  bf1;

	//	gmacdev->TxDescDma   = dma_addr;
	//	gmacdev->TxDescDma   = (dma_addr_t) first_desc;

	for(i =0; i < gmacdev -> TxDescCount; i++){
		synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
#if SYNOP_TOP_DEBUG
		printf("\n%02d %08x \n",i,(unsigned int)(gmacdev->TxDesc + i));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i))->status);
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->length));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->buffer1));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->buffer2));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->data1));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->data2));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->dummy1));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->dummy2));
		//		printf("%02d %08x \n",i,(unsigned int)(gmacdev->TxDesc + i));
		//		printf("%02d %08x \n",i,(unsigned int)(gmacdev->TxDesc + i));
#endif
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc  = 0; 

	pci_sync_cache(0, (vm_offset_t)first_desc,4*8*(gmacdev->TxDescCount), SYNC_W);
	return -ESYNOPGMACNOERR;
}


/**
  * This sets up the receive Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures in ring mode or chain mode.
  * This function depends on the pcidev structure for allocation of consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in rx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->RxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements  
  */
s32 synopGMAC_setup_rx_desc_queue(synopGMACdevice * gmacdev,u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc * bf1;
	DmaDesc *first_desc = NULL;
	//DmaDesc *second_desc = NULL;
	//dma_addr_t dma_addr;
	gmacdev->RxDescCount = 0;

	TR("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n",((sizeof(DmaDesc) * no_of_desc)));
	//	first_desc = plat_alloc_consistent_dmaable_memory (pcidev, sizeof(DmaDesc) * no_of_desc, &dma_addr);
	first_desc = plat_alloc_memory (sizeof(DmaDesc) * no_of_desc+15);		//sw: 2word aligned
	
//sw: map to uncached addr	
//	first_desc = CACHED_TO_UNCACHED((unsigned long)(first_desc));

	if(first_desc == NULL){
		TR("Error in Rx Descriptor Memory allocation in Ring mode\n");
		return -ESYNOPGMACNOMEM;
	}
	first_desc = (DmaDesc *)((u32)first_desc & ~15);


	gmacdev->RxDescCount = no_of_desc;
	gmacdev->RxDesc      = first_desc;
	//	gmacdev->RxDescDma   = dma_addr;

	bf1  = (DmaDesc *)CACHED_TO_UNCACHED((unsigned long)(gmacdev->RxDesc));	
	gmacdev->RxDescDma  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)bf1);
	//gmacdev->RxDesc     = bf1;

	//	gmacdev->RxDescDma   = (dma_addr_t) first_desc;

	for(i =0; i < gmacdev -> RxDescCount; i++){
		synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
#if SYNOP_TOP_DEBUG
		TR("%02d %08x \n",i, (unsigned int)(gmacdev->RxDesc + i));
#endif
	}

	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;

	gmacdev->BusyRxDesc   = 0; 

	pci_sync_cache(0, (vm_offset_t)first_desc,4*8*(gmacdev->RxDescCount), SYNC_W);

	return -ESYNOPGMACNOERR;
}

/**
 * Function to handle housekeeping after a packet is transmitted over the wire.
 * After the transmission of a packet DMA generates corresponding interrupt 
 * (if it is enabled). It takes care of returning the sk_buff to the linux
 * kernel, updating the networking statistics and tracking the descriptors.
 * @param[in] pointer to net_device structure. 
 * \return void.
 * \note This function runs in interrupt context
 */
void synop_handle_transmit_over(struct synopGMACNetworkAdapter * tp)
{
	struct	synopGMACNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
//	struct pci_dev *pcidev;
	s32 desc_index;
	u32 data1, data2;
	u32 status;
	u32 length1, length2;
	u64 dma_addr1, dma_addr2;
#ifdef ENH_DESC_8W
	u32 ext_status;
	u16 time_stamp_higher;
	u32 time_stamp_high;
	u32 time_stamp_low;
#endif
	adapter = tp;
	if(adapter == NULL){
#if SYNOP_TX_DEBUG
		TR("Unknown Device\n");
#endif
		return;
	}
	
	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
#if SYNOP_TX_DEBUG
		TR("GMAC device structure is missing\n");
#endif
		return;
	}

	/*Handle the transmit Descriptors*/
	do {
#ifdef ENH_DESC_8W
	desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2,&ext_status,&time_stamp_high,&time_stamp_low);
        synopGMAC_TS_read_timestamp_higher_val(gmacdev, &time_stamp_higher);
#else
	desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
#endif
	//desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr, &length, &data1);
		if(desc_index >= 0 && data1 != 0){
#if SYNOP_TX_DEBUG
			printf("Finished Transmit at Tx Descriptor %d for skb 0x%08x and buffer = %08x whose status is %08x \n", desc_index,data1,dma_addr1,status);
#endif
			#ifdef	IPC_OFFLOAD
			if(synopGMAC_is_tx_ipv4header_checksum_error(gmacdev, status)){
#if SYNOP_TX_DEBUG
			TR("Harware Failed to Insert IPV4 Header Checksum\n");
#else
			;
#endif
			}
			if(synopGMAC_is_tx_payload_checksum_error(gmacdev, status)){
#if SYNOP_TX_DEBUG
			TR("Harware Failed to Insert Payload Checksum\n");
#else
			;
#endif
			}
			#endif
	
			plat_free_memory((void *)(data1));	//sw:	data1 = buffer1
			
			if(synopGMAC_is_desc_valid(status)){
				adapter->synopGMACNetStats.tx_bytes += length1;
				adapter->synopGMACNetStats.tx_packets++;
			}
			else {	
#if SYNOP_TX_DEBUG
				TR("Error in Status %08x\n",status);
#endif
				adapter->synopGMACNetStats.tx_errors++;
				adapter->synopGMACNetStats.tx_aborted_errors += synopGMAC_is_tx_aborted(status);
				adapter->synopGMACNetStats.tx_carrier_errors += synopGMAC_is_tx_carrier_error(status);
			}
		}	adapter->synopGMACNetStats.collisions += synopGMAC_get_tx_collision_count(status);
	} while(desc_index >= 0);
//	netif_wake_queue(netdev);
}




/**
 * Function to Receive a packet from the interface.
 * After Receiving a packet, DMA transfers the received packet to the system memory
 * and generates corresponding interrupt (if it is enabled). This function prepares
 * the sk_buff for received packet after removing the ethernet CRC, and hands it over
 * to linux networking stack.
 * 	- Updataes the networking interface statistics
 *	- Keeps track of the rx descriptors
 * @param[in] pointer to net_device structure. 
 * \return void.
 * \note This function runs in interrupt context.
 */

void synop_handle_received_data(struct synopGMACNetworkAdapter* tp)
{
	struct synopGMACNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct PmonInet * pinetdev;
	s32 desc_index;
	struct ifnet* ifp;
	struct ether_header * eh;
	int i;
	char * ptr;
	u32 bf1;
	u32 data1;
	u32 data2;
	u32 len;
	u32 status;
	u64 dma_addr1;
	u64 dma_addr2;
	struct mbuf *skb; //This is the pointer to hold the received data

//sw: dbg
//	printf("---------------------rx packet: %d--------------------\n",++rxcnt);
	
//#if SYNOP_RX_DEBUG
//	TR("%s\n",__FUNCTION__);	
//#endif

	adapter = tp;
	if(adapter == NULL){
#if SYNOP_RX_DEBUG
		TR("Unknown Device\n");
#endif
		return;
	}

	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
#if SYNOP_RX_DEBUG
		TR("GMAC device structure is missing\n");
#endif
		return;
	}	

	pinetdev = adapter->PInetdev;
	if(pinetdev == NULL){
#if SYNOP_RX_DEBUG
		TR("GMAC device structure is missing\n");
#endif
		return;
	}
	ifp = &(pinetdev->arpcom.ac_if);


	/*Handle the Receive Descriptors*/
	do{
		desc_index = synopGMAC_get_rx_qptr(gmacdev, &status,&dma_addr1,NULL, &data1,&dma_addr2,NULL,&data2);

		if(desc_index >= 0 && data1 != 0){
#if SYNOP_RX_DEBUG
			printf("Received Data at Rx Descriptor %d for skb 0x%08x whose status is %08x\n",desc_index,dma_addr1,status);
#endif

			if(synopGMAC_is_rx_desc_valid(status)||SYNOP_PHY_LOOPBACK){
				skb = getmbuf(adapter);

				if(skb == 0)
#if SYNOP_RX_DEBUG
					printf("===error in getmbuf\n");
#else						
				;
#endif

				len =  synopGMAC_get_rx_desc_frame_length(status) - 4; //Not interested in Ethernet CRC bytes
				pci_sync_cache(0, (vm_offset_t)data1, len, SYNC_R);
				bcopy((char *)data1, mtod(skb, char *), len); 

#if SYNOP_RX_DEBUG
				printf("==pkg len: %d",len);
#endif

				skb->m_pkthdr.rcvif = ifp;
				skb->m_pkthdr.len = skb->m_len = len - sizeof(struct ether_header);

				eh = mtod(skb, struct ether_header *);
#if SYNOP_RX_DEBUG
				dumppkghd(eh,1);
				{
					int k;
					char temp;
					for (k=0;k<len;k++)
					{
						temp = (char)(*(char *)(data1 + k));
						printf("%02x  ",temp);
					}
					printf("\n------------------------- rx --------------------\n\n");
				}

#endif

				skb->m_data += sizeof(struct ether_header);

				ether_input(ifp, eh, skb);
				memset((u32)data1,0,RX_BUF_SIZE);
				adapter->synopGMACNetStats.rx_packets++;
				adapter->synopGMACNetStats.rx_bytes += len;
			}
			else{
				adapter->synopGMACNetStats.rx_errors++;
				adapter->synopGMACNetStats.collisions       += synopGMAC_is_rx_frame_collision(status);
				adapter->synopGMACNetStats.rx_crc_errors    += synopGMAC_is_rx_crc(status);
				adapter->synopGMACNetStats.rx_frame_errors  += synopGMAC_is_frame_dribbling_errors(status);
				adapter->synopGMACNetStats.rx_length_errors += synopGMAC_is_rx_frame_length_errors(status);
			}

			dma_addr1  = (unsigned long)CACHED_TO_PHYS((unsigned long)(data1));
			desc_index = synopGMAC_set_rx_qptr(gmacdev,dma_addr1, RX_BUF_SIZE, (u32)data1,0,0,0);

			if(desc_index < 0){
#if SYNOP_RX_DEBUG
				TR("Cannot set Rx Descriptor for data1 %08x\n",(u32)data1);
#endif
				plat_free_memory((void *)data1);
			}

		}

	}while(desc_index >= 0);
}


/**
 * Interrupt service routing.
 * This is the function registered as ISR for device interrupts.
 * @param[in] interrupt number. 
 * @param[in] void pointer to device unique structure (Required for shared interrupts in Linux).
 * @param[in] pointer to pt_regs (not used).
 * \return Returns IRQ_NONE if not device interrupts IRQ_HANDLED for device interrupts.
 * \note This function runs in interrupt context
 *
 */

//irqreturn_t synopGMAC_intr_handler(s32 intr_num, void * dev_id, struct pt_regs *regs)
int synopGMAC_intr_handler_0(struct synopGMACNetworkAdapter * tp)
{       
	/*Kernels passes the netdev structure in the dev_id. So grab it*/
//        struct net_device *netdev;
        struct synopGMACNetworkAdapter *adapter;
        synopGMACdevice * gmacdev;
        u32 interrupt;
	u32 dma_status_reg;
	s32 status;
	u64 dma_addr;
	struct ifnet * ifp;
	int data;
	time_cnt++;
	
	//dumpphyreg();
        adapter  = tp;
        if(adapter == NULL){
                TR("Adapter Structure Missing\n");
                return -1;
        }

        gmacdev = adapter->synopGMACdev;
        if(gmacdev == NULL){
                TR("GMAC device structure Missing\n");
                return -1;
        }

	ifp = &(adapter->PInetdev->arpcom.ac_if);

	if(gmacdev->LinkState == LINKUP)
	ifp->if_flags = ifp->if_flags | IFF_RUNNING;


	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg((u64 *)gmacdev->DmaBase, DmaStatus, 0);
//       	TR("%s:Dma Status Reg: 0x%08x\n",__FUNCTION__,dma_status_reg);

	
	if(dma_status_reg == 0)
		return 0;

	//if(dma_status_reg & 0x04)	//sw: dbg
	//	printf("Tx Desc Unavailable! 0x%x \n",dma_status_reg);
	
	if(dma_status_reg == 0x660004 || dma_status_reg == 0x660000)	//sw: dbg
		return 0;
	
//sw: check phy status	
//	synopGMAC_linux_cable_unplug_function(tp);
	
        synopGMAC_disable_interrupt_all(gmacdev,0);
	
	if(dma_status_reg & GmacPmtIntr){
		TR("%s:: Interrupt due to PMT module\n",__FUNCTION__);
		synopGMAC_linux_powerup_mac(gmacdev,0);
	}
	
	if(dma_status_reg & GmacMmcIntr){
		TR("%s:: Interrupt due to MMC module\n",__FUNCTION__);
		TR("%s:: synopGMAC_rx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev,0));
		TR("%s:: synopGMAC_tx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev,0));
	}

	if(dma_status_reg & GmacLineIntfIntr){
		TR("%s:: Interrupt due to GMAC LINE module\n",__FUNCTION__);		
		data = synopGMACReadReg(5, 0xd8 ,0);
		printf("===mac reg54: %x\n",data);

	}


	/*Now lets handle the DMA interrupts*/  
        interrupt = synopGMAC_get_interrupt_type(gmacdev, 0);
//sw
	if(interrupt == 0)	
		return 0;
	
//	TR("%s:Interrupts to be handled: 0x%08x\n",__FUNCTION__,interrupt);

        if(interrupt & synopGMACDmaError){

		u8 mac_addr[6] = DEFAULT_MAC_ADDRESS_0;//after soft reset, configure the MAC address to default value
		TR("%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
		
		synopGMAC_disable_dma_tx(gmacdev, 0);
                synopGMAC_disable_dma_rx(gmacdev, 0);
                
		synopGMAC_take_desc_ownership_tx(gmacdev);
		synopGMAC_take_desc_ownership_rx(gmacdev);
		
		synopGMAC_init_tx_rx_desc_queue(gmacdev);
		
		synopGMAC_reset(gmacdev, 0);//reset the DMA engine and the GMAC ip
		
		synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr, 0); 
		synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 , 0);
	 	synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward, 0);	
		synopGMAC_init_rx_desc_base(gmacdev, 0);
		synopGMAC_init_tx_desc_base(gmacdev, 0);
		synopGMAC_mac_init(gmacdev, 0);
		synopGMAC_enable_dma_rx(gmacdev, 0);
		synopGMAC_enable_dma_tx(gmacdev, 0);

        }


	if(interrupt & synopGMACDmaRxNormal){
//sw: dbg
//		printf("<<< rx tcount = %lld\n",time_cnt);
		
//		printf("===dma status reg: %x\n",dma_status_reg);
#if (SYNOP_RX_DEBUG||SYNOP_LOOPBACK_DEBUG)
		TR("%s:: Rx Normal \n", __FUNCTION__);
#endif

#if SYNOP_RX_TEST
		rx_test_count += 1;
		printf("Rx: %d packets!\n",rx_test_count);
#endif
		synop_handle_received_data(adapter);
	}

        if(interrupt & synopGMACDmaRxAbnormal){
	
		TR("%s::Abnormal Rx Interrupt Seen\n",__FUNCTION__);
		#if 1
	
	       if(GMAC_Power_down == 0){	// If Mac is not in powerdown
                adapter->synopGMACNetStats.rx_over_errors++;
		/*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
		synopGMACWriteReg((u64 *)gmacdev->DmaBase, DmaStatus ,0x80, 0); 	//sw: clear the rxb ua bit
		synopGMAC_resume_dma_rx(gmacdev, 0);//To handle GBPS with 12 descriptors
		}
		#endif
	}



        if(interrupt & synopGMACDmaRxStopped){
        	TR("%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__); //Receiver gone in to stopped state
		#if 1
	        if(GMAC_Power_down == 0){	// If Mac is not in powerdown
		adapter->synopGMACNetStats.rx_over_errors++;
/*
		do{
			struct sk_buff *skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_ATOMIC);
			if(skb == NULL){
				TR("%s::ERROR in skb buffer allocation Better Luck Next time\n",__FUNCTION__);
				break;
				//			return -ESYNOPGMACNOMEM;
			}
			
			dma_addr = pci_map_single(pcidev,skb->data,skb_tailroom(skb),PCI_DMA_FROMDEVICE);
			status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, skb_tailroom(skb), (u32)skb,0,0,0);
			TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
			if(status < 0)
				dev_kfree_skb_irq(skb);//changed from dev_free_skb. If problem check this again--manju
		
		}while(status >= 0);
*/  
	     	do{
			u32 skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
			if(skb == NULL){
				TR0("ERROR in skb buffer allocation\n");
				break;
//				return -ESYNOPGMACNOMEM;
			}
			
			//dma_addr = (u32)skb;
			dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(skb));
			status = synopGMAC_set_rx_qptr(gmacdev,dma_addr,RX_BUF_SIZE, (u32)skb,0,0,0);
			TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
			if(status < 0)
			{	
				plat_free_memory((void *)skb);
			}
		}while(status >= 0);

			synopGMAC_enable_dma_rx(gmacdev, 0);
		}
		#endif
	}

	if(interrupt & synopGMACDmaTxNormal){
		//xmit function has done its job

#if SYNOP_TX_TEST
		tx_normal_test_count += 1;
		printf("Tx: %d normal packets!\n\n",tx_normal_test_count);
#endif
#if (SYNOP_TX_DEBUG||SYNOP_LOOPBACK_DEBUG)
		TR("%s::Finished Normal Transmission \n",__FUNCTION__);
		TR("---------------------- tx %d pkg done ----------------------\n",++txcnt);
#endif
#if SYNOP_TX_DEBUG
		printf("Gmac_intr: dma_status = 0x%08x\n",dma_status_reg);
#endif
		synop_handle_transmit_over(adapter);	//Do whatever you want after the transmission is over
	}

        if(interrupt & synopGMACDmaTxAbnormal){
#if SYNOP_TX_TEST
		tx_abnormal_test_count += 1;
		printf("Tx: %d abnormal packets!\n",tx_abnormal_test_count);
#endif
		printf("%s::Abnormal Tx Interrupt Seen\n",__FUNCTION__);
		#if 1
	       if(GMAC_Power_down == 0){	// If Mac is not in powerdown
                synop_handle_transmit_over(adapter);
		}
		#endif
	}



	if(interrupt & synopGMACDmaTxStopped){
#if SYNOP_TX_TEST
		tx_stopped_test_count += 1;
		printf("Tx: %d stopped packets!\n",tx_stopped_test_count);
#endif
		TR("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
		printf("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
		#if 1
	       if(GMAC_Power_down == 0){	// If Mac is not in powerdown
		synopGMAC_disable_dma_tx(gmacdev, 0);
                synopGMAC_take_desc_ownership_tx(gmacdev);
		
		synopGMAC_enable_dma_tx(gmacdev, 0);
//		netif_wake_queue(netdev);
		TR("%s::Transmission Resumed\n",__FUNCTION__);
		}
		#endif
	}
//	synopGMAC_clear_interrupt(gmacdev);

        /* Enable the interrrupt before returning from ISR*/
//        synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
        return 0;
}


int synopGMAC_intr_handler_1(struct synopGMACNetworkAdapter * tp)
{       
	/*Kernels passes the netdev structure in the dev_id. So grab it*/
//        struct net_device *netdev;
        struct synopGMACNetworkAdapter *adapter;
        synopGMACdevice * gmacdev;
        u32 interrupt;
	u32 dma_status_reg;
	s32 status;
	u64 dma_addr;
	struct ifnet * ifp;

        adapter  = tp;
        if(adapter == NULL){
                TR("Adapter Structure Missing\n");
                return -1;
        }

        gmacdev = adapter->synopGMACdev;
        if(gmacdev == NULL){
                TR("GMAC device structure Missing\n");
                return -1;
        }

	ifp = &(adapter->PInetdev->arpcom.ac_if);

	if(gmacdev->LinkState == LINKUP)
	ifp->if_flags = ifp->if_flags | IFF_RUNNING;


	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg((u64 *)gmacdev->DmaBase, DmaStatus, 1);
//       	TR("%s:Dma Status Reg: 0x%08x\n",__FUNCTION__,dma_status_reg);

	
	if(dma_status_reg == 0)
		return 0;

	//if(dma_status_reg & 0x04)	//sw: dbg
	//	printf("Tx Desc Unavailable! 0x%x \n",dma_status_reg);
	
	if(dma_status_reg == 0x660004)	//sw: dbg
		return 0;
	
//sw: check phy status	
//	synopGMAC_linux_cable_unplug_function(tp);
	
        synopGMAC_disable_interrupt_all(gmacdev,1);

	
	if(dma_status_reg & GmacPmtIntr){
		TR("%s:: Interrupt due to PMT module\n",__FUNCTION__);
		synopGMAC_linux_powerup_mac(gmacdev,1);
	}
	
	if(dma_status_reg & GmacMmcIntr){
		TR("%s:: Interrupt due to MMC module\n",__FUNCTION__);
		TR("%s:: synopGMAC_rx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev,1));
		TR("%s:: synopGMAC_tx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev,1));
	}

	if(dma_status_reg & GmacLineIntfIntr){
//		TR("%s:: Interrupt due to GMAC LINE module\n",__FUNCTION__);
	}

	/*Now lets handle the DMA interrupts*/  
        interrupt = synopGMAC_get_interrupt_type(gmacdev, 1);
//sw
	if(interrupt == 0)	
		return 0;
	
//	TR("%s:Interrupts to be handled: 0x%08x\n",__FUNCTION__,interrupt);

        if(interrupt & synopGMACDmaError){

		u8 mac_addr[6] = DEFAULT_MAC_ADDRESS_0;//after soft reset, configure the MAC address to default value
		TR("%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
		printf("====DMA error!!!\n");
		
		synopGMAC_disable_dma_tx(gmacdev, 1);
                synopGMAC_disable_dma_rx(gmacdev, 1);
                
		synopGMAC_take_desc_ownership_tx(gmacdev);
		synopGMAC_take_desc_ownership_rx(gmacdev);
		
		synopGMAC_init_tx_rx_desc_queue(gmacdev);
		
		synopGMAC_reset(gmacdev, 1);//reset the DMA engine and the GMAC ip
		
		synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr, 1); 
		synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 , 1);
	 	synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward, 1);	
		synopGMAC_init_rx_desc_base(gmacdev, 1);
		synopGMAC_init_tx_desc_base(gmacdev, 1);
		synopGMAC_mac_init(gmacdev, 1);
		synopGMAC_enable_dma_rx(gmacdev, 1);
		synopGMAC_enable_dma_tx(gmacdev, 1);

        }


	if(interrupt & synopGMACDmaRxNormal){
#if (SYNOP_RX_DEBUG||SYNOP_LOOPBACK_DEBUG)
		TR("%s:: Rx Normal \n", __FUNCTION__);
#endif

#if SYNOP_RX_TEST
		rx_test_count += 1;
		printf("Rx: %d packets!\n",rx_test_count);
#endif
		synop_handle_received_data(adapter);
	}

        if(interrupt & synopGMACDmaRxAbnormal){
	        TR("%s::Abnormal Rx Interrupt Seen\n",__FUNCTION__);
		#if 1
	
	       if(GMAC_Power_down == 0){	// If Mac is not in powerdown
                adapter->synopGMACNetStats.rx_over_errors++;
		/*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
		synopGMACWriteReg((u64 *)gmacdev->DmaBase, DmaStatus ,0x80, 1); 	//sw: clear the rxb ua bit
		synopGMAC_resume_dma_rx(gmacdev, 1);//To handle GBPS with 12 descriptors
		}
		#endif
	}



        if(interrupt & synopGMACDmaRxStopped){
        	TR("%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__); //Receiver gone in to stopped state
		#if 1
	        if(GMAC_Power_down == 0){	// If Mac is not in powerdown
		adapter->synopGMACNetStats.rx_over_errors++;
/*
		do{
			struct sk_buff *skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_ATOMIC);
			if(skb == NULL){
				TR("%s::ERROR in skb buffer allocation Better Luck Next time\n",__FUNCTION__);
				break;
				//			return -ESYNOPGMACNOMEM;
			}
			
			dma_addr = pci_map_single(pcidev,skb->data,skb_tailroom(skb),PCI_DMA_FROMDEVICE);
			status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, skb_tailroom(skb), (u32)skb,0,0,0);
			TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
			if(status < 0)
				dev_kfree_skb_irq(skb);//changed from dev_free_skb. If problem check this again--manju
		
		}while(status >= 0);
*/  
	     	do{
			u32 skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
			if(skb == NULL){
				TR0("ERROR in skb buffer allocation\n");
				break;
//				return -ESYNOPGMACNOMEM;
			}
			
			//dma_addr = (u32)skb;
			dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(skb));
			status = synopGMAC_set_rx_qptr(gmacdev,dma_addr,RX_BUF_SIZE, (u32)skb,0,0,0);
			TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
			if(status < 0)
			{	
				printf("==%s:no free\n",__FUNCTION__);
				plat_free_memory((void *)skb);
			}
		}while(status >= 0);

			synopGMAC_enable_dma_rx(gmacdev, 1);
		}
		#endif
	}

	if(interrupt & synopGMACDmaTxNormal){
		//xmit function has done its job

#if SYNOP_TX_TEST
		tx_normal_test_count += 1;
		printf("Tx: %d normal packets!\n",tx_normal_test_count);
#endif
#if (SYNOP_TX_DEBUG||SYNOP_LOOPBACK_DEBUG)
		TR("%s::Finished Normal Transmission \n",__FUNCTION__);
#endif
#if SYNOP_TX_DEBUG
		printf("Gmac_intr: dma_status = 0x%08x\n",dma_status_reg);
#endif
		synop_handle_transmit_over(adapter);	//Do whatever you want after the transmission is over
	}

        if(interrupt & synopGMACDmaTxAbnormal){
#if SYNOP_TX_TEST
		tx_abnormal_test_count += 1;
		printf("Tx: %d abnormal packets!\n",tx_abnormal_test_count);
#endif
		printf("%s::Abnormal Tx Interrupt Seen\n",__FUNCTION__);
//		dumpreg(regbase);
		#if 1
	       if(GMAC_Power_down == 0){	// If Mac is not in powerdown
                synop_handle_transmit_over(adapter);
		}
		#endif
	}



	if(interrupt & synopGMACDmaTxStopped){
#if SYNOP_TX_TEST
		tx_stopped_test_count += 1;
		printf("Tx: %d stopped packets!\n",tx_stopped_test_count);
#endif
		TR("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
		printf("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
		#if 1
	       if(GMAC_Power_down == 0){	// If Mac is not in powerdown
		synopGMAC_disable_dma_tx(gmacdev, 1);
                synopGMAC_take_desc_ownership_tx(gmacdev);
		
		synopGMAC_enable_dma_tx(gmacdev, 1);
//		netif_wake_queue(netdev);
		TR("%s::Transmission Resumed\n",__FUNCTION__);
		}
		#endif
	}
//	synopGMAC_clear_interrupt(gmacdev);

        /* Enable the interrrupt before returning from ISR*/
//        synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
        return 0;
}

/**
 * Function used when the interface is opened for use.
 * We register synopGMAC_linux_open function to linux open(). Basically this 
 * function prepares the the device for operation . This function is called whenever ifconfig (in Linux)
 * activates the device (for example "ifconfig eth0 up"). This function registers
 * system resources needed 
 * 	- Attaches device to device specific structure
 * 	- Programs the MDC clock for PHY configuration
 * 	- Check and initialize the PHY interface 
 *	- ISR registration
 * 	- Setup and initialize Tx and Rx descriptors
 *	- Initialize MAC and DMA
 *	- Allocate Memory for RX descriptors (The should be DMAable)
 * 	- Initialize one second timer to detect cable plug/unplug
 *	- Configure and Enable Interrupts
 *	- Enable Tx and Rx
 *	- start the Linux network queue interface
 * @param[in] pointer to net_device structure. 
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */

unsigned long synopGMAC_linux_open(struct synopGMACNetworkAdapter *tp, u32 sel)
{
	s32 status = 0;
	s32 retval = 0;
	int delay = 100;
//	DmaDesc * dbgdesc;	//sw: dbg
	
	//s32 reserve_len=2;
	u64 dma_addr;
	u32 skb;	//sw	we just use the name skb in pomn
	u32 skb1;
	struct synopGMACNetworkAdapter *adapter = tp;
        synopGMACdevice * gmacdev;
	struct PmonInet * PInetdev;

	TR0("%s called \n",__FUNCTION__);
	adapter = tp;
	gmacdev = (synopGMACdevice *)adapter->synopGMACdev;
	PInetdev = (struct PmonInet *)adapter->PInetdev;
	
#if 0	
//	sw	we add poll-interrupt in the end
	if(! sel)
	{
		PInetdev->sc_ih = pci_intr_establish(0, 0, IPL_NET, synopGMAC_intr_handler_0, adapter, 0);
		TR("register poll interrupt: gmac 0\n");
//		synopGMAC_rx_enable(gmacdev, sel);
	}
	else
	{
		PInetdev->sc_ih = pci_intr_establish(0, 0, IPL_NET, synopGMAC_intr_handler_1, adapter, 0);
		TR("register poll interrupt: gmac 1\n");
	}
#endif


	/*Now platform dependent initialization.*/

	/*Lets reset the IP*/
	synopGMAC_reset(gmacdev,sel);
	
	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	//	synopGMAC_attach(synopGMACadapter->synopGMACdev,(u32) synopGMACMappedAddr + MACBASE,(u32) synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE);
	synopGMAC_attach(adapter->synopGMACdev,(u64) synopGMACMappedAddr + MACBASE,(u64) synopGMACMappedAddr + DMABASE, 1,sel);
	
	/*Lets read the version of ip in to device structure*/	
	synopGMAC_read_version(gmacdev, sel);
	
	synopGMAC_get_mac_addr(adapter->synopGMACdev,GmacAddr0High,GmacAddr0Low, PInetdev->dev_addr,sel); 

	/*Check for Phy initialization*/
	synopGMAC_set_mdc_clk_div(gmacdev,GmiiCsrClk3,sel);
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev,sel);

	status = synopGMAC_check_phy_init(gmacdev,sel);

	printf("check phy init status = 0x%x\n",status);
	
	/*Set up the tx and rx descriptor queue/ring*/
//sw
	synopGMAC_setup_tx_desc_queue(gmacdev,TRANSMIT_DESC_SIZE, RINGMODE);
	synopGMAC_init_tx_desc_base(gmacdev,sel);	//Program the transmit descriptor base address in to DmaTxBase addr

	
	synopGMAC_setup_rx_desc_queue(gmacdev,RECEIVE_DESC_SIZE, RINGMODE);
	synopGMAC_init_rx_desc_base(gmacdev, sel);	//Program the transmit descriptor base address in to DmaTxBase addr


#ifdef ENH_DESC_8W
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2 | DmaDescriptor8Words ); //pbl32 incr with rxthreshold 128 and Desc is 8 Words
#else
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 | DmaDescriptorSkip1 ,sel);                      //pbl4 incr with rxthreshold 128 
#endif
	
	synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward |DmaTxSecondFrame|DmaRxThreshCtrl128 ,sel);	

#if SYNOP_PHY_LOOPBACK
	printf("===phyloopback\n");
	
	gmacdev->DuplexMode = FULLDUPLEX ;
//	gmacdev->Speed      =   SPEED100;
#endif

//sw:dbg
//	gmacdev->DuplexMode = FULLDUPLEX ;
//	gmacdev->Speed      =   SPEED100;

	/*Initialize the mac interface*/
	synopGMAC_mac_init(gmacdev, sel);
	
	synopGMAC_pause_control(gmacdev,sel); // This enables the pause control in Full duplex mode of operation

	do{
		skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
		if(skb == NULL){
			TR0("ERROR in skb buffer allocation\n");
			break;
//			return -ESYNOPGMACNOMEM;
		}
	
		memset((u32)skb,0,RX_BUF_SIZE);
		pci_sync_cache(0, (vm_offset_t)skb, RX_BUF_SIZE, SYNC_W);
		skb1 = (u32)CACHED_TO_UNCACHED((unsigned long)(skb));	
		dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(skb1));

		//status = synopGMAC_set_rx_qptr_init(gmacdev,dma_addr,RX_BUF_SIZE, (u32)skb,0,0,0);
		status = synopGMAC_set_rx_qptr(gmacdev,dma_addr,RX_BUF_SIZE, (u32)skb,0,0,0);
		
		if(status < 0)
		{
			plat_free_memory((void *)skb);
		}	
	}while(status >= 0 && status < RECEIVE_DESC_SIZE-1);

//	dumpdesc(gmacdev);

	synopGMAC_clear_interrupt(gmacdev,sel);
	/*
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
	*/	
	synopGMAC_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF,sel);
	synopGMAC_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF,sel);
	synopGMAC_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF,sel);

//sw	no interrupts in pmon	
//	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	synopGMAC_disable_interrupt_all(gmacdev, sel);
	
#if SYNOP_TOP_DEBUG
	dumpreg(regbase);
	dumpphyreg();
	dumpdesc(tp->synopGMACdev);
#endif

	synopGMAC_enable_dma_rx(gmacdev,sel);
	synopGMAC_enable_dma_tx(gmacdev,sel);

	
#if SYNOP_TOP_DEBUG
	dumpreg(regbase);
#endif


#if SYNOP_TOP_DEBUG
	dumpphyreg();
#endif
	
#if 1	
//	sw	we add poll-interrupt in the end
	if(! sel)
	{
		PInetdev->sc_ih = pci_intr_establish(0, 0, IPL_NET, synopGMAC_intr_handler_0, adapter, 0);
		TR("register poll interrupt: gmac 0\n");
//		synopGMAC_rx_enable(gmacdev, sel);
	}
	else
	{
		PInetdev->sc_ih = pci_intr_establish(0, 0, IPL_NET, synopGMAC_intr_handler_1, adapter, 0);
		TR("register poll interrupt: gmac 1\n");
	}
#endif

	return retval;

}

#if SYNOP_TX_TEST
s32 synopGMAC_test(synopGMACdevice * gmacdev_0, synopGMACdevice * gmacdev_1)
{
	s32 status = 0;
	u64 dma_addr;
	u32 offload_needed = 0;
	u32 bf1;
	u32 bf2;
	char *buffer;
	char *buffer_p;
	u32 index;
	DmaDesc * dpr;
	int len = 60;
	int i,loop;
	char * ptr;

	
	{
			buffer = plat_alloc_memory(len);

			/*
			buffer[0] = 0x00;
			buffer[1] = 0xd0;
			buffer[2] = 0xb7;
			buffer[3] = 0xb1;
			buffer[4] = 0x57;
			buffer[5] = 0xe2;

			buffer[0] = 0x00;
			buffer[1] = 0x11;
			buffer[2] = 0x09;
			buffer[3] = 0x03;
			buffer[4] = 0x3a;
			buffer[5] = 0x61;
*/

			buffer[0] = 0xff;
			buffer[1] = 0xff;
			buffer[2] = 0xff;
			buffer[3] = 0xff;
			buffer[4] = 0xff;
			buffer[5] = 0xff;

			buffer[6] = 0x00;
			buffer[7] = 0x55;
			buffer[8] = 0x7b;
			buffer[9] = 0xb5;
			buffer[10] = 0x7d;
			buffer[11] = 0xf7;
			/*
			buffer[6] = 0xff;
			buffer[7] = 0xff;
			buffer[8] = 0xff;
			buffer[9] = 0xff;
			buffer[10] = 0xff;
			buffer[11] = 0xff;
*/

			buffer[12] = 0x08;
			buffer[13] = 0x06;

			buffer[14] = 0x00;
			buffer[15] = 0x01;

			buffer[16] = 0x08;
			buffer[17] = 0x00;


			buffer[18] = 0x06;
			buffer[19] = 0x04;
			buffer[20] = 0x00;
			buffer[21] = 0x01;

			buffer[22] = 0x00;
			buffer[23] = 0x55;
			buffer[24] = 0x7b;
			buffer[25] = 0xb5;
			buffer[26] = 0x7d;
			buffer[27] = 0xf7;

			buffer[28] = 0xc0;
			buffer[29] = 0xa8;
			buffer[30] = 0x6f;
			buffer[31] = 0xeb;

			buffer[32] = 0x00;
			buffer[33] = 0x00;
			buffer[34] = 0x00;
			buffer[35] = 0x00;
			buffer[36] = 0x00;
			buffer[37] = 0x00;

			buffer[38] = 0xc0;
			buffer[39] = 0xa8;
			buffer[40] = 0x6f;
			buffer[41] = 0x00;

			buffer[42] = 0x00;
			buffer[43] = 0x00;
			buffer[44] = 0x00;
			buffer[45] = 0x00;
			buffer[46] = 0x00;
			buffer[47] = 0x00;
			buffer[48] = 0x00;
			buffer[49] = 0x00;
			buffer[50] = 0x00;
			buffer[51] = 0x00;
			buffer[52] = 0x00;
			buffer[53] = 0x00;
			buffer[54] = 0x00;
			buffer[55] = 0x00;
			buffer[56] = 0x00;
			buffer[57] = 0x00;
			buffer[58] = 0x00;
			buffer[59] = 0x00;
	}
	TR("\n\n\n\n===================== %s called ======================\n",__FUNCTION__);
	for(loop = 0;loop < 100000; loop ++)
	{
		printf("\n++++++++++++++++++++++++++ Packet: %d +++++++++++++++++++++++++++\n",loop);
#if SYNOP_GMAC0
		{
			printf("xmit: GMAC0 TxBusy = %d\tTxNext = %d\n",gmacdev_0->TxBusy,gmacdev_0->TxNext);
			printf("===desc addr: %s\n",gmacdev_0->TxNextDesc);
			buffer[41] = buffer[59] = ((loop +1)%256);
			buffer[58] = 0x00;
			pci_sync_cache(0, (vm_offset_t)gmacdev_0->TxNextDesc, 4*8, SYNC_R);
			
			if(!synopGMAC_is_desc_owned_by_dma(gmacdev_0->TxNextDesc))
			{

				bf1 = (u32)plat_alloc_memory(TX_BUF_SIZE);
				if(bf1 == 0)
				{
					printf("===error in alloc bf1\n");	
					return -1;
				}


				memset((char *)bf1,0,TX_BUF_SIZE);
				//len = 60;
				bcopy((char *)buffer,(char *)bf1,(len));
				printf("==tx pkg len: %d\n",len);

				for(i = 0;i < len;i++)
				{
					ptr = (u32)bf1;
					printf(" %02x",*(ptr+i));
				}
				printf("\n");

				pci_sync_cache(0, (vm_offset_t)bf1, len, SYNC_W);
				bf2  = (u32)CACHED_TO_UNCACHED((unsigned long)bf1);	
				dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(bf2));

				status = synopGMAC_set_tx_qptr(gmacdev_0, dma_addr, (len), bf1,0,0,0,offload_needed,&index,dpr);
				printf("status = %d \n",status);

				if(status < 0){
					TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
					return -EBUSY;
				}
			}
			else
				printf("===%x: GMAC0 next txDesc belongs to DMA don't set it\n",gmacdev_0->TxNextDesc);
			{
				u32 data;
				data = synopGMACReadReg(555, 0x48,0);
				printf("GMAC0: TX DMA DESC ADDR = 0x%x\n",data);
			}
			
			synopGMAC_resume_dma_tx(gmacdev_0, 0);
		}
#endif
#if SYNOP_GMAC1
		{
			printf("xmit: GMAC1 TxBusy = %d\tTxNext = %d\n",gmacdev_1->TxBusy,gmacdev_1->TxNext);
			buffer[41] = buffer[59] = ((loop +1)%256);
			buffer[58] = 0x01;
			if(!synopGMAC_is_desc_owned_by_dma(gmacdev_1->TxNextDesc))
			{

				bf1 = (u32)plat_alloc_memory(TX_BUF_SIZE);
				if(bf1 == 0)
				{
					printf("===error in alloc bf1\n");	
					return -1;
				}
				memset((char *)bf1,0,TX_BUF_SIZE);
				//len = 60;
				bcopy((char *)buffer,(char *)bf1,(len));
				printf("==tx pkg len: %d\n",len);

				for(i = 0;i < len;i++)
				{
					ptr = (u32)bf1;
					printf(" %02x",*(ptr+i));
				}
				printf("\n");

				bf2  = (u32)CACHED_TO_UNCACHED((unsigned long)bf1);	
				dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(bf2));

				status = synopGMAC_set_tx_qptr(gmacdev_1, dma_addr, (len), bf1,0,0,0,offload_needed,&index,dpr);
				printf("status = %d \n",status);

				if(status < 0){
					TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
					return -EBUSY;
				}
			}
			else
				printf("===%x: GMAC1 next txDesc belongs to DMA don't set it\n",gmacdev_1->TxNextDesc);
			{
				u32 data;
				data = synopGMACReadReg(555, 0x48,1);
				printf("GMAC1: TX DMA DESC ADDR = 0x%x\n",data);
			}


			synopGMAC_tx_enable(gmacdev, sel);	//according to Tang Dan's commitment
			synopGMAC_resume_dma_tx(gmacdev_1, 1);
		}
#endif
		delay(100);
		delay(100);
		delay(100);
		delay(100);
		delay(100);
		delay(100);
		delay(100);
		delay(100);
		delay(100);
		delay(100);
	}

	//printf("%02d %08x %08x %08x %08x %08x %08x %08x\n",index,(u32)dpr,dpr->status,dpr->length,dpr->buffer1,dpr->buffer2,dpr->data1,dpr->data2);
	return -ESYNOPGMACNOERR;
}
#endif
/**
 * Function to transmit a given packet on the wire.
 * Whenever Linux Kernel has a packet ready to be transmitted, this function is called.
 * The function prepares a packet and prepares the descriptor and 
 * enables/resumes the transmission.
 * @param[in] pointer to sk_buff structure. 
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and Error code on failure. 
 * \note structure sk_buff is used to hold packet in Linux networking stacks.
 */

//s32 synopGMAC_linux_xmit_frames(struct sk_buff *skb, struct net_device *netdev)
s32 synopGMAC_linux_xmit_frames_0(struct ifnet* ifp)
{
	s32 status = 0;
	u64 dma_addr;
	u32 offload_needed = 0;
	u32 bf1;
	u32 bf2;
	u32 index;
	DmaDesc * dpr;
	int len;
	int i;
	char * ptr;
	struct mbuf *skb;	//sw	we just use the name skb
	struct ether_header * eh;

	//u32 flags;
	struct synopGMACNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
#if SYNOP_TX_DEBUG
//	TR("%s called \n",__FUNCTION__);
#endif
	
	adapter = (struct synopGMACNetworkAdapter *) ifp->if_softc;
	if(adapter == NULL)
		return -1;

	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL)
		return -1;
#if SYNOP_TX_DEBUG
	printf("\n------------------- tx ------------------------\n");
	printf("======in xmit: TxBusy = %d\tTxNext = %d\n",gmacdev->TxBusy,gmacdev->TxNext);
#endif
		
		while(ifp->if_snd.ifq_head != NULL){
			pci_sync_cache(0, (vm_offset_t)gmacdev->TxNextDesc, 4*8, SYNC_R);
			if(!synopGMAC_is_desc_owned_by_dma(gmacdev->TxNextDesc))
			{

				bf1 = (u32)plat_alloc_memory(TX_BUF_SIZE);
				
				if(bf1 == 0)
				{
#if SYNOP_TX_DEBUG
					printf("===error in alloc bf1\n");	
#endif
					return -1;
				}
				memset((char *)bf1,0,TX_BUF_SIZE);

				IF_DEQUEUE(&ifp->if_snd, skb);

				/*Now we have skb ready and OS invoked this function. Lets make our DMA know about this*/



				len = skb->m_pkthdr.len;


				//sw: i don't know weather it's right
				m_copydata(skb, 0, len,(char *)bf1);

				/*
				   if(len < 64)
				   len = 64;
				 */

#if SYNOP_TX_DEBUG
				printf("==tx pkg len: %d",len);
#endif
				//sw: dbg
				eh = mtod(skb, struct ether_header *);
#if SYNOP_TX_DEBUG
				dumppkghd(eh,0);

				for(i = 0;i < len;i++)
				{
					ptr = (u32)bf1;
					printf(" %02x",*(ptr+i));
				}
				printf("---------------------------------\n");
#endif
				pci_sync_cache(0, (vm_offset_t)bf1, len, SYNC_W);
				plat_free_memory(skb);

				bf2  = (u32)CACHED_TO_UNCACHED((unsigned long)bf1);	
				//dma_addr  = (unsigned long)vtophys((unsigned long)(bf2));
				dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(bf2));

				//		status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, TX_BUF_SIZE, bf1,0,0,0,offload_needed);
				status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, len, bf1,0,0,0,offload_needed,&index,dpr);

				if(status < 0){
#if SYNOP_TX_DEBUG
					TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
#endif
					//			dev_kfree_skb (skb); //with this, system used to freeze.. ??
					return -EBUSY;
				}
			}
#if SYNOP_TX_DEBUG
			else
//				printf("===%x: next txDesc belongs to DMA don't set it\n",gmacdev->TxNextDesc);
				;
#endif
		}
	/*Now force the DMA to start transmission*/	
#if SYNOP_TX_DEBUG
		{
			u32 data;
			data = synopGMACReadReg(555, 0x48, 0);
			printf("TX DMA DESC ADDR = 0x%x\n",data);
		}
#endif
	synopGMAC_resume_dma_tx(gmacdev,0);
	//printf("%02d %08x %08x %08x %08x %08x %08x %08x\n",index,(u32)dpr,dpr->status,dpr->length,dpr->buffer1,dpr->buffer2,dpr->data1,dpr->data2);
	return -ESYNOPGMACNOERR;
}
s32 synopGMAC_linux_xmit_frames_1(struct ifnet* ifp)
{
	s32 status = 0;
	u64 dma_addr;
	u32 offload_needed = 0;
	u32 bf1;
	u32 bf2;
	u32 index;
	DmaDesc * dpr;
	int len;
	int i;
	char * ptr;
	struct mbuf *skb;	//sw	we just use the name skb
	struct ether_header * eh;

	//u32 flags;
	struct synopGMACNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
//	struct pci_dev * pcidev;
#if SYNOP_TX_DEBUG
	TR("%s called \n",__FUNCTION__);
	printf("===xmit  yeh!\n");
#endif
	
	adapter = (struct synopGMACNetworkAdapter *) ifp->if_softc;
	if(adapter == NULL)
		return -1;

	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL)
		return -1;
#if SYNOP_TX_DEBUG
	printf("xmit: TxBusy = %d\tTxNext = %d\n",gmacdev->TxBusy,gmacdev->TxNext);
#endif
		
		while(ifp->if_snd.ifq_head != NULL){
			if(!synopGMAC_is_desc_owned_by_dma(gmacdev->TxNextDesc))
			{

				bf1 = (u32)plat_alloc_memory(TX_BUF_SIZE);
				if(bf1 == 0)
				{
#if SYNOP_TX_DEBUG
					printf("===error in alloc bf1\n");	
#endif
					return -1;
				}
				memset((char *)bf1,0,TX_BUF_SIZE);

				IF_DEQUEUE(&ifp->if_snd, skb);

				/*Now we have skb ready and OS invoked this function. Lets make our DMA know about this*/



				len = skb->m_pkthdr.len;


				//sw: i don't know weather it's right
				m_copydata(skb, 0, len,(char *)bf1);

				/*
				   if(len < 64)
				   len = 64;
				 */

#if SYNOP_TX_DEBUG
				printf("==tx pkg len: %d",len);
#endif
				//sw: dbg
				eh = mtod(skb, struct ether_header *);
#if SYNOP_TX_DEBUG
				dumppkghd(eh,0);

				for(i = 0;i < len;i++)
				{
					ptr = (u32)bf1;
					printf(" %02x",*(ptr+i));
				}
				printf("\n");
#endif


				plat_free_memory(skb);

				bf2  = (u32)CACHED_TO_UNCACHED((unsigned long)bf1);	
				//dma_addr  = (unsigned long)vtophys((unsigned long)(bf2));
				dma_addr  = (unsigned long)UNCACHED_TO_PHYS((unsigned long)(bf2));

				//		status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, TX_BUF_SIZE, bf1,0,0,0,offload_needed);
				status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, len, bf1,0,0,0,offload_needed,&index,dpr);

#if SYNOP_TX_DEBUG
				printf("status = %d \n",status);
#endif

				if(status < 0){
#if SYNOP_TX_DEBUG
					TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
#endif
					//			dev_kfree_skb (skb); //with this, system used to freeze.. ??
					return -EBUSY;
				}
			}
#if SYNOP_TX_DEBUG
			else
				printf("===%x: next txDesc belongs to DMA don't set it\n",gmacdev->TxNextDesc);
#endif
		}
	
	/*Now force the DMA to start transmission*/	
#if SYNOP_TX_DEBUG
		{
			u32 data;
			data = synopGMACReadReg(555, 0x48, 1);
			printf("TX DMA DESC ADDR = 0x%x\n",data);
		}
#endif
/*
	synopGMAC_tx_enable(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);
	synopGMAC_resume_dma_tx(gmacdev);
*/
	synopGMAC_resume_dma_tx(gmacdev, 1);
	//printf("%02d %08x %08x %08x %08x %08x %08x %08x\n",index,(u32)dpr,dpr->status,dpr->length,dpr->buffer1,dpr->buffer2,dpr->data1,dpr->data2);
	return -ESYNOPGMACNOERR;
}

/**
 * Function provides the network interface statistics.
 * Function is registered to linux get_stats() function. This function is 
 * called whenever ifconfig (in Linux) asks for networkig statistics
 * (for example "ifconfig eth0").
 * @param[in] pointer to net_device structure. 
 * \return Returns pointer to net_device_stats structure.
 * \callgraph
 */
struct net_device_stats *  synopGMAC_linux_get_stats(struct synopGMACNetworkAdapter *tp)
{
TR("%s called \n",__FUNCTION__);
return( &(((struct synopGMACNetworkAdapter *)(tp))->synopGMACNetStats) );
}


/**
 * IOCTL interface.
 * This function is mainly for debugging purpose.
 * This provides hooks for Register read write, Retrieve descriptor status
 * and Retreiving Device structure information.
 * @param[in] pointer to net_device structure. 
 * @param[in] pointer to ifreq structure.
 * @param[in] ioctl command. 
 * \return Returns 0 on success Error code on failure.
 */


int init_phy(struct synopGMACdevice *gmacdev, u32 sel)
{
	int retval;
	
	//retval = bcm54xx_config_init(gmacdev);

	retval = rtl8211_config_init(gmacdev , sel);
//	synopGMAC_phy_loopback(gmacdev, SYNOP_PHY_LOOPBACK);
	//if(retval != 0)
		return retval;
}

void dumppkghd(struct ether_header *eh,int tp)
{
		int i;
//sw: dbg
		if(tp == 1)
	       		printf("\n===Rx:  pkg dst:  ");
		else
	       		printf("\n===Tx:  pkg dst:  ");
		
		for(i = 0;i < 6;i++)
	       		printf(" %02x",eh->ether_dhost[i]);
		
		if(tp == 1)
			printf("\n===Rx:  pkg sst:  ");
		else
			printf("\n===Tx:  pkg sst:  ");
	
		for(i = 0;i < 6;i++)
	       		printf(" %02x",eh->ether_shost[i]);
		
		if(tp == 1)
			printf("\n===Rx:  pkg type:  ");
		else
			printf("\n===Tx:  pkg type:  ");
		printf(" %12x",eh->ether_type);
		printf("\n");
		
//dbg
}




s32 synopGMAC_dummy_reset_0(struct ifnet *ifp)
{
	
	struct synopGMACNetworkAdapter * adapter; 
	synopGMACdevice	* gmacdev;
	
	adapter = (struct synopGMACNetworkAdapter *)ifp->if_softc;
	gmacdev = adapter->synopGMACdev;
	
	return synopGMAC_reset(gmacdev,0);
}

s32 synopGMAC_dummy_reset_1(struct ifnet *ifp)
{
	
	struct synopGMACNetworkAdapter * adapter; 
	synopGMACdevice	* gmacdev;
	
	adapter = (struct synopGMACNetworkAdapter *)ifp->if_softc;
	gmacdev = adapter->synopGMACdev;
	
	return synopGMAC_reset(gmacdev,1);
}

s32 synopGMAC_dummy_ioctl(struct ifnet *ifp)
{
//	printf("==dummy ioctl\n");
	return 0;
}
//sw:	i just copy this function from rtl8169.c

static int gmac_ether_ioctl_0(struct ifnet *ifp, unsigned long cmd, caddr_t data)
{
	struct ifaddr *ifa;
	struct synopGMACNetworkAdapter *adapter;
	int error = 0;
	int s;

	adapter = ifp->if_softc;
	ifa = (struct ifaddr *) data;

	s = splimp();
	switch (cmd) {
#ifdef PMON
	case SIOCPOLL:
	{
		break;
	}
#endif
	case SIOCSIFADDR:
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if (!(ifp->if_flags & IFF_UP))			
			{	
				error = synopGMAC_linux_open(adapter,0);
			}	
//			error = rtl8169_open(sc);
	
			if(error == -1){
				return(error);
			}	
			ifp->if_flags |= IFF_UP;
#ifdef __OpenBSD__
//			arp_ifinit(&sc->arpcom, ifa);

			arp_ifinit(&(adapter->PInetdev->arpcom), ifa);
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
			synopGMAC_linux_open(adapter,0);
			ifp->if_flags |= IFF_UP;
			break;
		}
		break;
	case SIOCSIFFLAGS:
		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */

		printf("===ioctl sifflags\n");
		if(ifp->if_flags & IFF_UP){
			synopGMAC_linux_open(adapter,0);
		}
		break;
/*
        case SIOCETHTOOL:
        {
        long *p=data;
        myRTL = sc;
        cmd_setmac(p[0],p[1]);
        }
        break;
       case SIOCRDEEPROM:
                {
                long *p=data;
                myRTL = sc;
                cmd_reprom(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
                myRTL = sc;
                cmd_wrprom(p[0],p[1]);
                }
                break;
*/
	default:
		printf("===ioctl default\n");
		dumpreg(regbase); 
		error = EINVAL;
	}

	splx(s);

	if(error)
		printf("===ioctl error: %d\n",error);
	return (error);
}
static int gmac_ether_ioctl_1(struct ifnet *ifp, unsigned long cmd, caddr_t data)
{
	struct ifaddr *ifa;
	struct synopGMACNetworkAdapter *adapter;
	int error = 0;
	int s;

	adapter = ifp->if_softc;
	ifa = (struct ifaddr *) data;

	s = splimp();
	switch (cmd) {
#ifdef PMON
	case SIOCPOLL:
	{
		break;
	}
#endif
	case SIOCSIFADDR:
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if (!(ifp->if_flags & IFF_UP))			
			{	
				error = synopGMAC_linux_open(adapter,1);
			}	
//			error = rtl8169_open(sc);
	
			if(error == -1){
				return(error);
			}	
			ifp->if_flags |= IFF_UP;
#ifdef __OpenBSD__

			arp_ifinit(&(adapter->PInetdev->arpcom), ifa);
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
			synopGMAC_linux_open(adapter,1);
			ifp->if_flags |= IFF_UP;
			break;
		}
		break;
	case SIOCSIFFLAGS:
		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */

		if(ifp->if_flags & IFF_UP){
			synopGMAC_linux_open(adapter,1);
		}
		break;
	default:
		dumpreg(regbase); 
		error = EINVAL;
	}

	splx(s);

	if(error)
		printf("===ioctl error: %d\n",error);
	return (error);
}

void dumpdesc(synopGMACdevice	* gmacdev)
{
	int i;
	
	printf("\n===dump %d Tx desc",gmacdev -> TxDescCount);
	for(i =0; i < gmacdev -> TxDescCount; i++){
//		pci_sync_cache(0, (vm_offset_t)(gmacdev->TxDesc + i),32, SYNC_R);
		printf("\n%02d %08x : ",i,(unsigned int)(gmacdev->TxDesc + i));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i))->status);
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->length));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->buffer1));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->buffer2));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->data1));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->data2));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->dummy1));
		printf("%08x ",(unsigned int)((gmacdev->TxDesc + i)->dummy2));
	}
	printf("\n===dump %d Rx desc",gmacdev -> RxDescCount);
	for(i =0; i < gmacdev -> RxDescCount; i++){
//		pci_sync_cache(0, (vm_offset_t)(gmacdev->RxDesc + i),32, SYNC_R);
		printf("\n%02d %08x : ",i,(unsigned int)(gmacdev->RxDesc + i));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i))->status);
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->length));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->buffer1));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->buffer2));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->data1));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->data2));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->dummy1));
		printf("%08x ",(unsigned int)((gmacdev->RxDesc + i)->dummy2));
	}

	printf("\n\n");
	
}

void dumpreg(u64 gbase)
{
	int i;
	int k;
	u32 data;
	
#if SYNOP_GMAC0
	printf("\n==== gmac:0 dumpreg\n");
	for (i = 0,k = 0; i < 0xbc; i = i+4,k++)
	{
		data = synopGMACReadReg(5, i ,0);
		printf("  reg:%2x value:%8x  ",i,data);
		if(k%4 == 3)
			printf("\n");
	}
	printf("\n");
	for (i = 0xc0,k = 0; i < 0xdc; i = i+4,k++){	
		data = synopGMACReadReg(5, i ,0);
		printf("  reg:%2x value:%8x  ",i,data);
		if(k%4 == 3)
			printf("\n");
	}
	printf("\n");

	for (i = 0,k = 0; i < 0x5c; i = i+4,k++){
		data = synopGMACReadReg(555, i ,0);
		printf("  reg:%2x value:%8x  ",i,data);
		if(k%4 == 3)
			printf("\n");

	}
	printf("\n\n");
#endif
#if SYNOP_GMAC1
	printf("\n==== gmac:1 dumpreg\n");
	for (i = 0,k = 0; i < 0xbc; i = i+4,k++)
	{
		data = synopGMACReadReg(5, i ,1);
		printf("  reg:%2x value:%8x  ",i,data);
		if(k%4 == 3)
			printf("\n");
	}
	printf("\n");
	for (i = 0xc0,k = 0; i < 0xdc; i = i+4,k++){	
		data = synopGMACReadReg(5, i ,1);
		printf("  reg:%2x value:%8x  ",i,data);
		if(k%4 == 3)
			printf("\n");
	}
	printf("\n");

	for (i = 0,k = 0; i < 0x5c; i = i+4,k++){
		data = synopGMACReadReg(555, i ,1);
		printf("  reg:%2x value:%8x  ",i,data);
		if(k%4 == 3)
			printf("\n");

	}
	printf("\n\n");
#endif
}

void dumpphyreg(void)
{
	u16 data;
	int i;
#if SYNOP_GMAC0
	printf("===dump mii phy regs of GMAC: 0\n");
	for(i = 0x0; i <= 0x1f; i++)
	{
		if ((i==11)||(i==12)||(i==13)||(i==14)||(i==20)||(i==22)||(i==23)||(i==25)||(i==26)||(i==27)||(i==28)||(i==29)||(i==30))
			continue;
		synopGMAC_read_phy_reg(0x5,1,i, &data,0);
		printf("  mii phy reg: 0x%x    value: %x  \n",i,data);
	}
	printf("\n");
#endif
#if SYNOP_GMAC1
	printf("===dump mii phy regs of GMAC: 1\n");
	for(i = 0x0; i <= 0x1f; i++)
	{
		if ((i==11)||(i==12)||(i==13)||(i==14)||(i==20)||(i==22)||(i==23)||(i==25)||(i==26)||(i==27)||(i==28)||(i==29)||(i==30))
			continue;
		synopGMAC_read_phy_reg(0x5,1,i, &data,1);
		printf("  mii phy reg: 0x%x    value: %x  \n",i,data);
	}
	printf("\n");
#endif
}

/**
 * Function to initialize the Linux network interface.
 * 
 * Linux dependent Network interface is setup here. This provides 
 * an example to handle the network dependent functionality.
 *
 * \return Returns 0 on success and Error code on failure.
 */
s32  synopGMAC_init_network_interface(void)
{
//varables added by sw
#if SYNOP_GMAC0
	struct ifnet* ifp_0;
	char* xname_0 = "syn0";
#endif
#if SYNOP_GMAC1
	struct ifnet* ifp_1;
	char* xname_1 = "syn1";
#endif
//	char mac_addr[6] = MAC_ADDR;
#if SYNOP_GMAC0
	u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS_0;
#endif
#if SYNOP_GMAC1
	u8 mac_addr1[6] = DEFAULT_MAC_ADDRESS_1;
#endif
	int i,errr;
	u16 data;
	
	
	TR("Now Going to Call register_netdev to register the network interface for GMAC core\n");
//gmac0 base
	
	synopGMACadapter_0 = (struct synopGMACNetworkAdapter * )plat_alloc_memory(sizeof (struct synopGMACNetworkAdapter)); 
	memset((char *)synopGMACadapter_0 ,0, sizeof (struct synopGMACNetworkAdapter));

	synopGMACadapter_0->synopGMACdev    = NULL;
	synopGMACadapter_0->PInetdev   = NULL;
	
	/*Allocate Memory for the the GMACip structure*/
	synopGMACadapter_0->synopGMACdev = (synopGMACdevice *) plat_alloc_memory(sizeof (synopGMACdevice));
	memset((char *)synopGMACadapter_0->synopGMACdev ,0, sizeof (synopGMACdevice));
	if(!synopGMACadapter_0->synopGMACdev){
		TR0("Error in Memory Allocataion \n");
	}
		
	/*Allocate Memory for the the GMAC-Pmon structure	sw*/
	synopGMACadapter_0->PInetdev = (struct PmonInet *) plat_alloc_memory(sizeof (struct PmonInet));
	memset((char *)synopGMACadapter_0->PInetdev ,0, sizeof (struct PmonInet));
	if(!synopGMACadapter_0->PInetdev){
		TR0("Error in Pdev-Memory Allocataion \n");
	}

	synopGMACadapter_1 = (struct synopGMACNetworkAdapter * )plat_alloc_memory(sizeof (struct synopGMACNetworkAdapter)); 
//sw:	should i put sync_cache here?
	memset((char *)synopGMACadapter_1 ,0, sizeof (struct synopGMACNetworkAdapter));

	synopGMACadapter_1->synopGMACdev    = NULL;
	synopGMACadapter_1->PInetdev   = NULL;
	
	/*Allocate Memory for the the GMACip structure*/
	synopGMACadapter_1->synopGMACdev = (synopGMACdevice *) plat_alloc_memory(sizeof (synopGMACdevice));
	memset((char *)synopGMACadapter_1->synopGMACdev ,0, sizeof (synopGMACdevice));
	if(!synopGMACadapter_1->synopGMACdev){
		TR0("Error in Memory Allocataion \n");
	}
		
	/*Allocate Memory for the the GMAC-Pmon structure	sw*/
	synopGMACadapter_1->PInetdev = (struct PmonInet *) plat_alloc_memory(sizeof (struct PmonInet));
	memset((char *)synopGMACadapter_1->PInetdev ,0, sizeof (struct PmonInet));
	if(!synopGMACadapter_1->PInetdev){
		TR0("Error in Pdev-Memory Allocataion \n");
	}
	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	//synopGMAC_attach(s,selynopGMACadapter->synopGMACdev,(u32) synopGMACMappedAddr + MACBASE,(u32) synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE);
#if SYNOP_GMAC0
	synopGMAC_attach(synopGMACadapter_0->synopGMACdev,(u64) synopGMACMappedAddr + MACBASE,(u64) synopGMACMappedAddr + DMABASE, 1,0);
#endif
#if SYNOP_GMAC1
	synopGMAC_attach(synopGMACadapter_1->synopGMACdev,(u64) synopGMACMappedAddr + MACBASE,(u64) synopGMACMappedAddr + DMABASE, 1,1);
#endif

#if SYNOP_TOP_DEBUG
	dumpphyreg();
#endif

//	testphyreg(synopGMACadapter->synopGMACdev);
#if SYNOP_GMAC0
	synopGMAC_check_phy_init(synopGMACadapter_0->synopGMACdev,0);
#endif
#if SYNOP_GMAC1
	synopGMAC_check_phy_init(synopGMACadapter_1->synopGMACdev,1);
#endif
#if SYNOP_GMAC0
	synopGMAC_reset(synopGMACadapter_0->synopGMACdev,0);
#endif
#if SYNOP_GMAC1
	synopGMAC_reset(synopGMACadapter_1->synopGMACdev,1);
#endif



//ifp init	sw
//	memset(PInetdev,0,sizeof(struct PmonInet));
#if SYNOP_GMAC0
	
	ifp_0 = &(synopGMACadapter_0->PInetdev->arpcom.ac_if);
	ifp_0->if_softc = synopGMACadapter_0;
	
	synopGMACadapter_0->PInetdev->dev_addr = mac_addr0;


//	bcopy(mac_addr, synopGMACadapter->PInetdev->arpcom.ac_enaddr, sizeof(synopGMACadapter->PInetdev->arpcom.ac_enaddr));		//sw: set mac addr manually
	bcopy(synopGMACadapter_0->PInetdev->dev_addr, synopGMACadapter_0->PInetdev->arpcom.ac_enaddr, sizeof(synopGMACadapter_0->PInetdev->arpcom.ac_enaddr));		//sw: set mac addr manually

/*	
	printf("\n===mac addr:");
	for(i = 0;i < 6;i++)
		printf(" %2x ",*(synopGMACadapter->PInetdev->arpcom.ac_enaddr+i));
*/

	bcopy(xname_0, ifp_0->if_xname, IFNAMSIZ);
	
	ifp_0->if_start = (void *)synopGMAC_linux_xmit_frames_0;
	ifp_0->if_ioctl = (int *)gmac_ether_ioctl_0;
//	ifp->if_ioctl = (int *)synopGMAC_dummy_ioctl;
	ifp_0->if_reset = (int *)synopGMAC_dummy_reset_0;
	
	ifp_0->if_snd.ifq_maxlen = TRANSMIT_DESC_SIZE - 1;	//defined in Dev.h value is 12, too small?

	errr =  init_phy(synopGMACadapter_0->synopGMACdev,0);

	printf("errr = %d \n",errr);
#endif
#if SYNOP_GMAC1
	
	ifp_1 = &(synopGMACadapter_1->PInetdev->arpcom.ac_if);
	ifp_1->if_softc = synopGMACadapter_1;
	
	synopGMACadapter_1->PInetdev->dev_addr = mac_addr1;


//	bcopy(mac_addr, synopGMACadapter->PInetdev->arpcom.ac_enaddr, sizeof(synopGMACadapter->PInetdev->arpcom.ac_enaddr));		//sw: set mac addr manually
	bcopy(synopGMACadapter_1->PInetdev->dev_addr, synopGMACadapter_1->PInetdev->arpcom.ac_enaddr, sizeof(synopGMACadapter_1->PInetdev->arpcom.ac_enaddr));		//sw: set mac addr manually

/*	
	printf("\n===mac addr:");
	for(i = 0;i < 6;i++)
		printf(" %2x ",*(synopGMACadapter->PInetdev->arpcom.ac_enaddr+i));
*/

	bcopy(xname_1, ifp_1->if_xname, IFNAMSIZ);
	
	ifp_1->if_start = (void *)synopGMAC_linux_xmit_frames_1;
	ifp_1->if_ioctl = (int *)gmac_ether_ioctl_1;
//	ifp->if_ioctl = (int *)synopGMAC_dummy_ioctl;
	ifp_1->if_reset = (int *)synopGMAC_dummy_reset_1;
	
	ifp_1->if_snd.ifq_maxlen = TRANSMIT_DESC_SIZE - 1;	//defined in Dev.h value is 12, too small?

	errr =  init_phy(synopGMACadapter_1->synopGMACdev,1);

	printf("errr = %d \n",errr);
#endif
	/*Now start the network interface*/
	TR("\nNow Registering the netdevice\n");
#if SYNOP_GMAC0
	synopGMAC_linux_open(synopGMACadapter_0,0); 
#endif
#if SYNOP_GMAC1
	synopGMAC_linux_open(synopGMACadapter_1,1); 
#endif
#if SYNOP_GMAC0
	if_attach(ifp_0);
	ether_ifattach(ifp_0);
	ifp_0->if_flags = ifp_0->if_flags | IFF_UP | IFF_RUNNING;
#endif
#if SYNOP_GMAC1
	if_attach(ifp_1);
	ether_ifattach(ifp_1);
	ifp_1->if_flags = ifp_1->if_flags | IFF_UP | IFF_RUNNING;
#endif
//sw: dbg
	

#if SYNOP_GMAC1
	dumpdesc(synopGMACadapter_1->synopGMACdev);
#endif
	
#if SYNOP_TX_TEST
	synopGMAC_test(synopGMACadapter_0->synopGMACdev, synopGMACadapter_1->synopGMACdev);
#endif
//	return 1;

//	dumpphyreg();
	
	return 0;

}


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"dumpphyreg","",0,"dumpphyreg",dumpphyreg,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void))  __attribute__ ((constructor));

static void init_cmd()
{
	cmdlist_expand(Cmds,1);
}
