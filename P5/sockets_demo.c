#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
    
#define RECV_OK    		0
#define RECV_EOF  		-1
#define DEFAULT_BACKLOG		5
#define NON_RESERVED_PORT 	5001
#define BUFFER_LEN	 	1024	
#define SERVER_ACK		"ACK_FROM_SERVER"

/* extern char *sys_errlist[]; */

/* Macros to convert from integer to net byte order and vice versa */
#define i_to_n(n)  (int) htonl( (u_long) n)
#define n_to_i(n)  (int) ntohl( (u_long) n)

main(int argc,char *argv[])
{
    /* second argument, if supplied, is host where server is running */
    if (argc == 2)
    {
		printf("calling client\n");
		client(argv[1]);
		printf("back from client\n");
    }
    else  /* if no arguments then running as the server */
    {
		printf("calling server\n");
		server();
		printf("back from server\n");
    }
}

server()
{
    int rc, accept_socket, to_client_socket;

    accept_socket = setup_to_accept(NON_RESERVED_PORT);
    for (;;)
    {
		to_client_socket = accept_connection(accept_socket);
		if ((rc = fork()) == -1) /* Error Condition */
		{
			printf("server: fork failed\n");
			exit(99);
		}
		else if (rc > 0)  /* Parent Process, i.e. rc = child's pid */
		{
			close(to_client_socket);
		}
		else          /* Child Process, rc = 0 */
		{
			close(accept_socket);
			serve_one_connection(to_client_socket);
			exit(0);
		}
    }
}

serve_one_connection(int to_client_socket)
{
    int rc, ack_length;
    char buf[BUFFER_LEN];

    ack_length = strlen(SERVER_ACK)+1;
    rc = recv_msg(to_client_socket, buf);	
    while (rc != RECV_EOF)
    {
		printf("server received: %s \n",buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);	
    }
    close(to_client_socket);
}

client(char *server_host)
{
    int to_server_socket;
    char buf[BUFFER_LEN];

    to_server_socket = connect_to_server(server_host,NON_RESERVED_PORT);
    printf("\nEnter a line of text to send to the server or EOF to exit\n");
    while (fgets(buf,BUFFER_LEN,stdin) != NULL)
    {
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);	
		printf("client received: %s \n",buf);
        printf("\nEnter a line of text to send to the server or EOF to exit\n");
    }
}

setup_to_accept(int port)	
{
    int rc, accept_socket;
    int optval = 1;
    struct sockaddr_in sin, from;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    accept_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(accept_socket,"setup_to_accept socket");

    setsockopt(accept_socket,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval));

    rc = bind(accept_socket, (struct sockaddr *)&sin ,sizeof(sin));
    error_check(rc,"setup_to_accept bind");

    rc = listen(accept_socket, DEFAULT_BACKLOG);
    error_check(rc,"setup_to_accept listen");

    return(accept_socket);
}

int accept_connection(int accept_socket)	
{
    struct sockaddr_in from;
    int fromlen, to_client_socket, gotit;
    int optval = 1;

    fromlen = sizeof(from);
    gotit = 0;
    while (!gotit)
    {
		to_client_socket = accept(accept_socket, (struct sockaddr *)&from, &fromlen);
		if (to_client_socket == -1)
		{
			/* Did we get interrupted? If so, try again */
			if (errno == EINTR)
			continue;
			else
			error_check(to_client_socket, "accept_connection accept");
		}
		else
			gotit = 1;
    }

    setsockopt(to_client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
    return(to_client_socket);
}

connect_to_server(char *hostname, int port)	
{
    int rc, to_server_socket;
    int optval = 1;
    struct sockaddr_in listener;
    struct hostent *hp;

    hp = gethostbyname(hostname);
    if (hp == NULL)
    {
		printf("connect_to_server: gethostbyname %s: %s -- exiting\n",
		hostname, sys_errlist[errno]);
		exit(99);
    }

    bzero((void *)&listener, sizeof(listener));
    bcopy((void *)hp->h_addr, (void *)&listener.sin_addr, hp->h_length);
    listener.sin_family = hp->h_addrtype;
    listener.sin_port = htons(port);

    to_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(to_server_socket, "net_connect_to_server socket");

    setsockopt(to_server_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));

    rc = connect(to_server_socket,(struct sockaddr *) &listener, sizeof(listener));
    error_check(rc, "net_connect_to_server connect");

    return(to_server_socket);
}

int recv_msg(int fd, char *buf)
{
    int bytes_read;
	// loop here too. if header, loop through until all header is read
	// then loop to get data.
    bytes_read = read(fd, buf, BUFFER_LEN);
    error_check( bytes_read, "recv_msg read");
    if (bytes_read == 0)
		return(RECV_EOF);
    return( bytes_read );
}

send_msg(int fd, char *buf, int size)	
{
    int n;
	// loop on the write. look at n to see how many bytes were written
	// may not start at the front of buffer on successive writes
    n = write(fd, buf, size);
    error_check(n, "send_msg write");
}

error_check(int val, char *str)	
{
    if (val < 0)
    {
		printf("%s :%d: %s\n", str, val, sys_errlist[errno]);
		exit(1);
    }
}

