// Name:		Ben Belden
// Class ID#:	bpb2v
// Section:		CSCI 6250-001
// Assignment:	Lab #5
// Due:			16:20:00, November 9, 2016
// Purpose:		Implement client/server connections with fun sockety goodness.		
// Input:	    From terminal.	
// Outut:		To terminal.
// 
// File:		p5.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <errno.h>

#define RECV_OK				0
#define RECV_EOF			-1
#define DEFAULT_BACKLOG		10
#define NON_RESERVED_PORT	5001  // not gonna use this, user defines port#
#define BUFFER_LEN			10000
#define CMD_LEN				64
#define SERVER_ACK			"ACK_FROM_SERVER"

#define i_to_n(n) (int)htonl((u_long)n)
#define n_to_i(n) (int)ntohl((u_long)n)


int main (int argc, char *argv[])
{
	if (argc == 2)
	{
		printf("############### starting the server ###############\n");
		server(atoi(argv[1]));
	}
	else if (argc >= 5)
	{
		printf("############### starting the client ###############\n");
		client(argv[1],atoi(argv[2]),atoi(argv[3]),argv[4],&(argv[4]));
	}
	else printf("if server: p5s port#\nif client: p5c server port# time cmd args\n");
	return 0;
}

server(int port)
{
	int rc,accept_socket,to_client_socket;

	accept_socket = setup_to_accept(port);
	for(;;)
	{
		to_client_socket = accept_connection(accept_socket);
		if ((rc = fork()) == -1) // error
		{
			printf("server: fork failed\n");
			exit(99);
		}
		else if (rc > 0) close(to_client_socket); // parent
		else  // child
		{
			close(accept_socket);
			serve_one_connection(to_client_socket);
			exit(0);
		}
	}
}

setup_to_accept(int port)
{
	int rc,accept_socket,optval=1;
	struct sockaddr_in sin, from;
	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	accept_socket = socket(AF_INET,SOCK_STREAM,0);
	error_check(accept_socket,"setup_to_accept socket");

	setsockopt(accept_socket,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval));

	rc = bind(accept_socket, (struct sockaddr *)&sin, sizeof(sin));
	error_check(rc,"setup_to_accept bind");

	rc = listen(accept_socket, DEFAULT_BACKLOG);
	error_check(rc,"setup_to_accept listen");

	return (accept_socket);
}

int accept_connection(int accept_socket)
{
	struct sockaddr_in from;
	int fromlen,to_client_socket,gotit,optval=1;

	fromlen = sizeof(from);
	gotit = 0;
	while (!gotit)
	{
		to_client_socket = accept(accept_socket, (struct sockaddr *)&from,&fromlen);
		if (to_client_socket == -1)
		{ // did we get interrupted? if so, try again!!
			if (errno == EINTR) continue;
			else error_check(to_client_socket,"accept_connection accept");
		}
		else gotit = 1;
	}
	setsockopt(to_client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
	return (to_client_socket);
}

error_check(int val, char *str)
{
	if (val < 0)
	{
		printf("%s :%d: %s\n",str,val,strerror(errno));
		exit(1);
	}
}

serve_one_connection(int to_client_socket)
{
	int rc, ack_length,cpuLim,i,pfd[2];
	char buf[BUFFER_LEN],*ptr,cpuLimBuf[5],*cmd,*args[50],*buf2,*arg,msg[BUFFER_LEN];
	ack_length = strlen(SERVER_ACK)+1;
	pid_t pid;
	struct rlimit r;
	rc = recv_msg(to_client_socket,buf);

	for (i=0;i<4;i++) cpuLimBuf[i] = buf[i];

	cpuLim = atoi(cpuLimBuf);
	buf2 = strdup(&(buf[4]));
	arg = strtok_r(buf2," \t\n",&ptr);
	cmd = strdup(arg);
	for (i=0;i<20 && arg;i++)
	{
		args[i] = strdup(arg);
		arg = strtok_r(NULL," \t\n",&ptr);
	}
	if ((pipe(pfd))==-1)
	{
		send_msg(to_client_socket,"failed to open pipe",19);
		exit(-1);
	}
	if ((pid=fork())<0)
	{
		send_msg(to_client_socket,"failed to fork",14);
		exit(-1);
	}
	else if (pid==0)
	{
		r.rlim_cur = cpuLim;
		r.rlim_max = cpuLim;
		setrlimit(RLIMIT_CPU,&r);
		close(pfd[0]);
		dup2(pfd[1],1);
		dup2(pfd[1],2);
		close(pfd[1]);
		close(pfd[2]);
		execvp(cmd,args);
		send_msg(to_client_socket,"failed to exec",14);
		exit(-1);
	}
	close(pfd[1]);
	close(pfd[2]);
	bzero(buf,sizeof(buf));
	while(read(pfd[0],buf,sizeof(buf))>0)
	{
		send_msg(to_client_socket,buf,strlen(buf));
		bzero(buf,strlen(buf));
	}
	wait(NULL);
	send_msg(to_client_socket,msg,strlen(msg));
	close(to_client_socket);
}

client(char *server_host, int port, int cpuLim, char *cmd, char *args[])
{
	int to_server_socket,i;
	char buf[BUFFER_LEN],cpuLimArr[5];

	sprintf(cpuLimArr,"%.4d",cpuLim);
	strcat(buf,cpuLimArr);

	for (i=0;i<10 && args[i];i++)
	{
		if (args[i][0] != '\n')
		{
			if (i>0) strcat(buf," ");
			strcat(buf,args[i]);
		}
	}
	to_server_socket = connect_to_server(server_host,port);
	send_msg(to_server_socket,buf,strlen(buf)+1);
	bzero(buf,sizeof(buf));
	while(recv_msg(to_server_socket,buf)>0)
	{
		printf("%s",buf);
		bzero(buf,strlen(buf));
	}
}

connect_to_server(char *hostname, int port)
{
	int rc, to_server_socket,optval=-1;
	struct sockaddr_in listener;
	struct hostent *hp;

	hp = gethostbyname(hostname);
	if (hp == NULL)
	{
		printf("connect_to_server: gethostbyname %s: %s -- exiting\n",hostname,strerror(errno));
		exit(99);
	}
	bzero((void*)&listener,sizeof(listener));
	bcopy((void*)hp->h_addr,(void*)&listener.sin_addr,hp->h_length);
	listener.sin_family = hp->h_addrtype;
	listener.sin_port = htons(port);
	
	to_server_socket = socket(AF_INET,SOCK_STREAM,0);
	error_check(to_server_socket,"net_connect_to_server socket");

	setsockopt(to_server_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));

	rc = connect(to_server_socket,(struct sockaddr *)&listener,sizeof(listener));
	error_check(rc,"net_connect_to_server connect");

	return (to_server_socket);
}

recv_msg(int fd, char *buf)
{
	int bytes_read,size,br;
	char msg[BUFFER_LEN];

	bzero(buf,sizeof(buf));
	bzero(msg,sizeof(msg));
	bytes_read = read(fd,msg,4);
	strcat(buf,msg);
	bzero(msg,bytes_read);
	while (bytes_read<4)
	{
		br = read(fd,msg,4-bytes_read);
		error_check(br,"recv_msg loop1 read");
		bytes_read+=br;
		strcat(buf,msg);
		bzero(msg,br);
	}
	size = atoi(buf);
	bzero(buf,4);
	bytes_read = read(fd,msg,size);
	error_check(bytes_read,"recv_msg read");
	strcat(buf,msg);
	bzero(msg,bytes_read);
	while (bytes_read<size)
	{
		br = read(fd,msg,size-bytes_read);
		error_check(br,"recv_msg loop2 read");
		bytes_read+=br;
		strcat(buf,msg);
		bzero(msg,br);
	}
	return (bytes_read);
}

send_msg(int fd, char *buf, int size)
{
	int n;
	char csize[5],msg[BUFFER_LEN];

	bzero(msg,sizeof(char) * BUFFER_LEN);
	sprintf(csize,"%.4d",size);
	strcat(msg,csize);
	strcat(msg,buf);
	size+=4;
	n = write(fd,msg,size);
	error_check(n,"send_msg write");
}



/*  PROGRAM EXAMPLES

	sockets_demo.c
	send_msg and recv_msg are bad bad bad, for the sockets_demo only

	fixed length vs headers on send/rcv messages
	
	
	
*/
/*

				Advanced Operating Systems
						Fall 2016
	 
turnin code: aos_p5


Write a C/C++ program which implements a client/server pair
of programs that provide the functionality described below.

The client and server should support functionality similar to that
provided by ssh/sshd or rsh/rhsd.  However, we are not requiring
that support be provided for full-screen applications such as vi.

The server should accept connections on a port specified as the
only cmd-line arg.  (It should use setsockopt with the SO_REUSEADDR
option to make the port reusable.)  The server should then accept
connections from one or more clients (concurrent server, not
iterative) and fork and exec each application that is requested.
Each application should have its time usage restricted based on
the value of cpu args passed to the server from the client (see
client description below).  The stdin and stderr of the application
process does not have to be handled; you may close them if desired.
The stdout of the application process should be captured by the
server and delivered to the client for printing.

The client requires a minimum of 4 cmd-line args.
It should accept these args in the order shown:
	host on which to connect to server
	port on which to connect to server
	max cpu time in seconds usable by remote app (e.g. 6)
	appname to run via remote server (full path name)
	args to appname


Submitting the project for grading:

The project's two programs should compile and link as p5s and p5c.

You should turnin a tar file containing all of the required files.

To build the project, I will cd to my directory containing your files
and simply type:

	rm -rf p5s p5c
	rm -rf *.o
	make    ## this one command should cause BOTH programs to be built

*/
