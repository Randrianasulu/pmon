cmd :
ubi part nand0,1
ubi create ubi0
ubi info
initrd tftp://192.168.0.10/ubifs.img
ubi write 0x84000000 ubi0 0x985600
ubifsmount ubi0
ubifsumount
ubi remove ubi0
