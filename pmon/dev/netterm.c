#include <pmon.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <termio.h>
#include <sys/proc.h>
#include <sys/file.h>

static int hasdata;
void	so_upcall(struct socket *so, caddr_t arg, int waitf)
{
hasdata = 1;
}

int server_sock;
int netserial (char *host, int port)
{
	int sock;
	struct sockaddr_in sin;
        struct hostent *hp;
        int ret;
        char buf[512];
	struct proc *p = curproc;
	struct file *fp;
	struct socket *so;
	sock = socket(AF_INET, SOCK_STREAM, 0);

	//sin_family = AF_INET,and bind
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);


	//get the message of host
	hp = gethostbyname(host);
	if(hp)
	{
		sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (void *)&sin.sin_addr, hp->h_length);
	}
	else
		goto error;
/*
 *
 * connect the httpd
 *
 */
	ret = connect(sock,&sin, sizeof(sin));
	if(ret < 0)
		goto error;

/*
 *
 * my_write			->add head of http,send filename address and port of host
 * my_read			->del head of http,receive file
 *
 */
#if 0
	while(1)
	{
	ret = recv(sock, buf, 511, 0);
	if(ret<=0) break;
        buf[ret] = 0;
	send(sock, buf, ret,0);
	}
#else
server_sock = sock;
send(sock, "server started\n", 15,0);
	if ((ret = getsock(p->p_fd, sock, &fp)) != 0)
	goto error;
	so = fp->f_data;
	so->so_upcall = so_upcall;
#endif
	return 0;
error:
	printf("error\n");
	return -1;
}

int
netterm (int op, struct DevEntry *dev, unsigned long param, int data)
{

	static char c;
	switch (op) {
		case OP_INIT:

		case OP_XBAUD:
		case OP_BAUD:
		return 0;

		case OP_TXRDY:
		return 1;

		case OP_TX:
			if(server_sock)
			  send(server_sock, &data,1,0);
			break;

		case OP_RXRDY:
		if(hasdata)
		{
                 int ret;
                 ret = recv(server_sock, &c, 1, MSG_DONTWAIT);
                hasdata = ret==1;
		}
		return hasdata;

		case OP_RX:
			return c;

		case OP_RXSTOP:
			return 0;
	}
	return 0;
}

static int cmd_netserial (int argc, char **argv)
{
char *host=argc>1?argv[1]:"10.20.0.1";
int port = argc>2?strtoul(argv[2],0,0):8888;
netserial(host, port);
return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"netserial","", 0, "netserial", cmd_netserial, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

