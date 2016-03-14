#include <getopt.h>
#include <stdlib.h>
#include  <unistd.h>
extern char **environ;
/*
 *  Calculate checksum. If 'set' checksum is calculated and set.
 */
static int
cksum(void *p, size_t s, int set)
{
	u_int16_t sum = 0;
	u_int8_t *sp = p;
	int sz = s / 2;

	if(set) {
		*sp = 0;	/* Clear checksum */
		*(sp+1) = 0;	/* Clear checksum */
	}
	while(sz--) {
		sum += (*sp++) << 8;
		sum += *sp++;
	}
	if(set) {
		sum = -sum;
		*(u_int8_t *)p = sum >> 8;
		*((u_int8_t *)p+1) = sum;
	}
	return(sum);
}

int tgt_unsetenv()
{
        ep = nvrambuf + 2;

	status = 0;
        while((*ep != '\0') && (ep < nvrambuf + NVRAM_SIZE)) {
                np = name;
                sp = ep;

                while((*ep == *np) && (*ep != '=') && (*np != '\0')) {
                        ep++;
                        np++;
                }
                if((*np == '\0') && ((*ep == '\0') || (*ep == '='))) {
                        while(*ep++);
                        while(ep < nvrambuf + NVRAM_SIZE) {
                                *sp++ = *ep++;
                        }
                        if(nvrambuf[2] == '\0') {
                                nvrambuf[3] = '\0';
                        }
                        cksum(nvrambuf, NVRAM_SIZE, 1);
#ifdef NVRAM_IN_FLASH
                        if(fl_erase_device(nvram, NVRAM_SECSIZE, FALSE)) {
                                status = -1;
				break;
                        }

                        if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, FALSE)) {
                                status = -1;
				break;
                        }
#else
			nvram_put(nvram);
#endif
			status = 1;
			break;
                }
                else if(*ep != '\0') {
                        while(*ep++ != '\0');
                }
        }
}

int tgt_setenv()
{
	if(nvram_invalid)
	{
                memset(nvram, -1, fsize);
                nvrambuf[2] = '\0';
                nvrambuf[3] = '\0';
                cksum((void *)nvram, fsize, 1);
	 }
	else {
		ep = nvram+2;
		if(*ep != '\0') {
			do {
				while(*ep++ != '\0');
			} while(*ep++ != '\0');
			ep--;
		}
		if(((int)ep + fsize - (int)ep) < (envlen + 1)) {
			free(nvram);
			return(0);      /* Bummer! */
		}

		/*
		 *  Special case heaptop must always be first since it
		 *  can change how memory allocation works.
		 */
		if(strcmp("heaptop", name) == 0) {

			bcopy(nvrambuf+2, nvrambuf+2 + envlen,
				 ep - nvrambuf+1);

			ep = nvrambuf+2;
			while(*name != '\0') {
				*ep++ = *name++;
			}
			if(value != NULL) {
				*ep++ = '=';
				while((*ep++ = *value++) != '\0');
			}
			else {
				*ep++ = '\0';
			}
		}
		else {
			while(*name != '\0') {
				*ep++ = *name++;
			}
			if(value != NULL) {
				*ep++ = '=';
				while((*ep++ = *value++) != '\0');
			}
			else {
				*ep++ = '\0';
			}
			*ep++ = '\0';   /* End of env strings */
		}
	}

        cksum(nvrambuf, NVRAM_SIZE, 1);
}

int main(int argc, char **argv)
{
int opt;
char *fname = "gzrom.bin", *ep;
int foff = 0x70000;
int fsize = 512;
int fd;
char *ep;
char env[512] = "ENV_";
int nvram_invalid = 0;
while((opt=getopt(argc,argv,"f:o:s:"))!=-1)
{
 switch(opt)
 {
   case 'f':
    fname = optarg;
    break;
   case 'o':
     foff = strtoul(optarg,0,0);
    break;
   case 's':
     fsize = strtoul(optarg,0,0);
    break;
 }
}

printf("%d\n", optind);
nvram = zalloc(fsize);

fd = open(fname, O_RDONLY);
lseek(fd, foff, 0);
read(td, nvram, fsize);
close(fd);

if(cksum((void *)nvram, fsize, 1))
{
	printf("Warning! NVRAM checksum fail. Reset!\n");
        nvram_invalid = 1;
}
else
{

	ep = nvram+2;;

	while(*ep != 0) {
		char *val = 0, *p = env+4;
		i = 0;
		while((*p++ = *ep++) && (ep <= nvram + fsize - 1) && i++ < 255) {
			if((*(p - 1) == '=') && (val == NULL)) {
				*(p - 1) = '\0';
				val = p;
			}
		}
		if(ep <= nvram + fsize - 1 && i < 255) {
			setenv(env, val, 1);
		}
		else {
			nvram_invalid = 2;
			break;
		}
	}
}

	for(ret=0,i=0;environ[i];i++)
		if(!strncmp(environ[i],"ENV_",4)) ret+=4;

return 0;
}

