#include <sys/types.h>
#include <sys/socket.h>

#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <openssl/crypto.h>
#include <openssl/ssl.h>

#define CA_CRT "cacert.pem"

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif  /* INADDR_NONE */

extern int	errno;

int	TCPecho(const char *host, const char *portnum, SSL_CTX *ctx);
int	errexit(const char *format, ...);
int	connectsock(const char *host, const char *portnum);

#define	LINELEN		128


SSL_CTX *create_CTX_to_authenticate(); 
SSL *authenticate_server(const int s, SSL_CTX *ctx); 

/*------------------------------------------------------------------------
 * main - TCP client for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*portnum = "5004";	/* default server port number	*/

	/* SSL initializations */
	SSL_CTX *ctx;

	/* Load encryption & hashing algorithms for the SSL program */
	SSL_library_init();

	/* Create and load ctx struct to authenticate server certificate */
	ctx = create_CTX_to_authenticate(); 

	switch (argc) {
	case 1:
		host = "localhost";
		break;
	case 3:
		host = argv[2];
		/* FALL THROUGH */
	case 2:
		portnum = argv[1];
		break;
	default:
		fprintf(stderr, "usage: TCPecho [host [port]]\n");
		exit(1);
	}
	TCPecho(host, portnum, ctx);
	exit(0);
}

/*------------------------------------------------------------------------
 * TCPecho - send input to ECHO service on specified host and print reply
 *------------------------------------------------------------------------
 */
int
TCPecho(const char *host, const char *portnum, SSL_CTX *ctx)
{
	char	buf[LINELEN];		/* buffer for one line of text	*/
	memset(buf, '\0', sizeof(buf));
	
	int	s;			/* socket descriptor, read count*/

	SSL *ssl;

	s = connectsock(host, portnum);
	
	/* Create an SSL structure and authenticate the server certificate */
	ssl = authenticate_server(s, ctx);

	while (fgets(buf, sizeof(buf), stdin)) {

		if(SSL_write(ssl, buf, strlen(buf)) <= 0) {
			errexit("SSL_write failed.\n");
		}
		
		memset(buf, '\0', sizeof(buf));

		if(SSL_read(ssl, buf, sizeof(buf)) <= 0) {
				errexit("SSL_read failed.\n");
			}

		fputs(buf, stdout);

		memset(buf, '\0', sizeof(buf));
	}
	
	SSL_shutdown(ssl);
	(void) close(s);

	SSL_free(ssl);
	SSL_CTX_free(ctx);

	return EXIT_SUCCESS;
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
 * connectsock - allocate & connect a socket using TCP 
 *------------------------------------------------------------------------
 */
int
connectsock(const char *host, const char *portnum)
/*
 * Arguments:
 *      host      - name of host to which connection is desired
 *      portnum   - server port number
 */
{
        struct hostent  *phe;   /* pointer to host information entry    */
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int     s;              /* socket descriptor                    */


        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

    /* Map port number (char string) to port number (int)*/
        if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
                errexit("can't get \"%s\" port number\n", portnum);

    /* Map host name to IP address, allowing for dotted decimal */
        if ( (phe = gethostbyname(host)) )
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
                errexit("can't get \"%s\" host entry\n", host);

    /* Allocate a socket */
        s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0)
                errexit("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
                errexit("can't connect to %s.%s: %s\n", host, portnum,
                        strerror(errno));
        return s;
}

SSL_CTX *create_CTX_to_authenticate() {

	SSL_CTX *ctx;

	/* Create an SSL_CTX structure */
	ctx = SSL_CTX_new(SSLv3_method());
	if(ctx == NULL) {
		errexit("could not create SSL_CTX structure\n");
	}

	/* Load CA certificate into SSL_CTX structure in */
	/* order to authenticate the server's certificate */
	if((SSL_CTX_load_verify_locations(ctx, CA_CRT, NULL)) == 0) {
		errexit("could not load CA certificate: %s\n", strerror(errno));
	}
	
	/* Certificate must be signed by a CA */
	SSL_CTX_set_verify_depth(ctx, 1);
	
	return ctx;
}

SSL *authenticate_server(const int s, SSL_CTX *ctx) {

	SSL *ssl;

	/* Create an SSL structue */
	ssl = SSL_new(ctx);

	/* Assign newly created socket into the SSL structure */
	SSL_set_fd(ssl, s);

	/* Perform SSL handshake by the client */
	if(SSL_connect(ssl) < 0) {
		errexit("client SSL handshake failed.\n");
	}
	

	if(SSL_get_peer_certificate(ssl) != NULL) {
		if(SSL_get_verify_result(ssl) == X509_V_OK) {
			printf("client verification with SSL_get_verify_result() succeeded.\n");
		}
		else {
			errexit("client verification with SSL_get_verify_result() failed.\n");
		}
	}
	else {
		errexit("no server certificate was presented.\n");
	}

	return ssl;
}
