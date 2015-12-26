#
#  Makefile to build guess.c
#

CROSS_COMPILE 	= mipsel-linux-

#
# Include the make variables (CC, etc...)
#

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
SIZE		= $(CROSS_COMPILE)size

guess:	guess.c
	${CC} -G 0 -O2 -mips2  -Ttext=0x80200000 -N -Wl,-T ld.script -o guess guess.c -nostdlib -fno-pic -mno-abicalls -e main
clean:
	rm -f guess
