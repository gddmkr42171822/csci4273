
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <openssl/crypto.h>
#include <openssl/ssl.h>

#define SERV_CRT "server.crt"
#define SERV_KEY "server_priv.key"

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		4096

#define SSL_MAX 10

extern int	errno;
int		errexit(const char *format, ...);
int		passivesock(const char *portnum, int qlen);
int		echo(int fd, SSL *ssl[]);


SSL_CTX *create_serverCTX(); 
SSL *ssl_handshake(const int s, SSL_CTX *ctx);

/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*portnum = "5004";	/* Standard server port number	*/
	struct sockaddr_in fsin;	/* the from address of a client	*/
	int	msock;			/* master server socket		*/
	fd_set	rfds;			/* read file descriptor set	*/
	fd_set	afds;			/* active file descriptor set	*/
	unsigned int	alen;		/* from-address length		*/
	int	fd, nfds;

	/* SSL initializations */
	SSL_CTX *ctx;

	/* Array of pointers to ssl structures; limited to 10 structures */
	SSL *ssl[SSL_MAX];
	fd_set seen_sockets;
	FD_ZERO(&seen_sockets);

	SSL_library_init();

	ctx = create_serverCTX(); 
	
	switch (argc) {
	case	1:
		break;
	case	2:
		portnum = argv[1];
		break;
	default:
		errexit("usage: TCPmechod [port]\n");
	}

	msock = passivesock(portnum, QLEN);

	nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(msock, &afds);

	while (1) {
		memcpy(&rfds, &afds, sizeof(rfds));

		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0) {
			errexit("select: %s\n", strerror(errno));
		}

		if (FD_ISSET(msock, &rfds)) {
			int	ssock;

			alen = sizeof(fsin);
			ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
			if (ssock < 0) {
				errexit("accept: %s\n", strerror(errno));
			}

			ssl[ssock] = ssl_handshake(ssock, ctx);
			printf("Adding socket |%d| to an ssl struct.\n", ssock);

			FD_SET(ssock, &afds);
		}
		for (fd=0; fd<nfds; ++fd) {
			if (fd != msock && FD_ISSET(fd, &rfds)) {
				if (echo(fd, ssl) == 0) {
					
					if(SSL_shutdown(ssl[fd]) < 0) {
						errexit("problem shutting down ssl connection");
					}

					SSL_free(ssl[fd]);

					(void) close(fd);
					printf("Socket |%d| closed and ssl struct freed.\n", fd);

					FD_CLR(fd, &afds);

				}
			}
		}
	}
	
	/* Free the SSL_CTX structure */
	SSL_CTX_free(ctx);
}

/*------------------------------------------------------------------------
 * echo - echo one buffer of data, returning byte count
 *------------------------------------------------------------------------
 */
int
echo(int fd, SSL *ssl[])
{
	char	buf[BUFSIZ];
	memset(buf, '\0', sizeof(buf));
	int	cc;

	if(SSL_read(ssl[fd], buf, sizeof(buf)) <= 0) {
		fprintf(stderr, "ssl_read on socket |%d| was not successful.\n", fd);
		return 0;
	}
	
	cc = strlen(buf);

	if(SSL_write(ssl[fd], buf, strlen(buf)) <= 0) {
		fprintf(stderr, "ssl_write on socket |%d| was not successful.\n", fd);
		return 0;
	}

	return cc;
	
}

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int
errexit(const char *format, ...)
{
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

/*------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP
 *------------------------------------------------------------------------
 */
int
passivesock(const char *portnum, int qlen)
/*
 * Arguments:
 *      portnum   - port number of the server
 *      qlen      - maximum server request queue length
 */
{
        struct sockaddr_in sin; /* an Internet endpoint address  */
        int     s;              /* socket descriptor             */

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;

    /* Map port number (char string) to port number (int) */
        if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
                errexit("can't get \"%s\" port number\n", portnum);

    /* Allocate a socket */
        s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0)
            errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            fprintf(stderr, "can't bind to %s port: %s; Trying other port\n",
                portnum, strerror(errno));
            sin.sin_port=htons(0); /* request a port number to be allocated
                                   by bind */
            if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
                errexit("can't bind: %s\n", strerror(errno));
            else {
                socklen_t socklen = sizeof(sin);

                if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                        errexit("getsockname: %s\n", strerror(errno));
                printf("New server port number is %d\n", ntohs(sin.sin_port));
            }
        }

        if (listen(s, qlen) < 0)
            errexit("can't listen on %s port: %s\n", portnum, strerror(errno));
        return s;
}

SSL_CTX *create_serverCTX() {
	SSL_CTX *ctx;

	ctx = SSL_CTX_new(SSLv3_method());
	if(ctx == NULL) {
		errexit("could not create server SSL_CTX structure");
	}

	/* Load the server certificate into the SSL_CTX structure */
	if(SSL_CTX_use_certificate_file(ctx, SERV_CRT, SSL_FILETYPE_PEM) <= 0) {
		errexit("could not load server certificate: %s\n", strerror(errno));
	}

	/* Load the private-key corresponding to the server certificate */
	if(SSL_CTX_use_PrivateKey_file(ctx, SERV_KEY, SSL_FILETYPE_PEM) <= 0) {
		errexit("could not load server private-key: %s\n", strerror(errno));
	}

	/* Check if server certificate and private-key matches */
	if(SSL_CTX_check_private_key(ctx) <= 0) {
		errexit("private-key and server certificate do not match: %s\n", strerror(errno));
	}

	return ctx;

}

SSL *ssl_handshake(const int s, SSL_CTX *ctx) {
	SSL *ssl;

	ssl = SSL_new(ctx);
	if(ssl == NULL) {
		errexit("server could not create SSL structure");
	}

	SSL_set_fd(ssl,s);

	if(SSL_accept(ssl) < 0) {
		errexit("server SSL handshake failed.\n");
	}

	return ssl;
}

