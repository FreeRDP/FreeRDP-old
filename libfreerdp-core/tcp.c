/*
   FreeRDP: A Remote Desktop Protocol client.
   TCP Layer

   Copyright (C) Matthew Chapman 1999-2008

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef _WIN32
#include <unistd.h>		/* select read write close */
#include <sys/socket.h>		/* socket connect setsockopt */
#include <sys/time.h>		/* timeval */
#include <netdb.h>		/* gethostbyname */
#include <netinet/in.h>		/* sockaddr_in */
#include <netinet/tcp.h>	/* TCP_NODELAY */
#include <arpa/inet.h>		/* inet_addr */
#include <errno.h>		/* errno */
#include <fcntl.h>		/* fcntl F_GETFL F_SETFL O_NONBLOCK */
#endif

#include "frdp.h"
#include "iso.h"
#include "mcs.h"
#include "rdp.h"
#include <freerdp/utils/memory.h>
#include <freerdp/utils/hexdump.h>

#include "tcp.h"

#ifdef _WIN32
#define socklen_t int
#define TCP_CLOSE(_sck) closesocket(_sck)
#define TCP_STRERROR "tcp error"
#define TCP_BLOCKS (WSAGetLastError() == WSAEWOULDBLOCK)
#define MSG_NOSIGNAL 0
#else
#define TCP_CLOSE(_sck) close(_sck)
#define TCP_STRERROR strerror(errno)
#define TCP_BLOCKS (errno == EWOULDBLOCK)
#endif

#ifdef __APPLE__
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

/*****************************************************************************/
/* returns boolean */
static RD_BOOL
tcp_socket_ok(int sck)
{
#if defined(_WIN32)
	int opt;
	int opt_len;
#else
	int opt;
	unsigned int opt_len;
#endif

	opt_len = sizeof(opt);
	if (getsockopt(sck, SOL_SOCKET, SO_ERROR, (char *) (&opt), &opt_len) == 0)
	{
		if (opt == 0)
		{
			return True;
		}
	}

	return False;
}

/* wait till socket is ready to write or timeout */
RD_BOOL
tcp_can_send(int sck, int millis)
{
	fd_set wfds;
	struct timeval time;
	int sel_count;

	time.tv_sec = millis / 1000;
	time.tv_usec = (millis * 1000) % 1000000;

	FD_ZERO(&wfds);
	FD_SET(sck, &wfds);
	sel_count = select(sck + 1, 0, &wfds, 0, &time);

	if (sel_count > 0)
	{
		return tcp_socket_ok(sck);
	}

	return False;
}

/* wait till socket is ready to read or timeout */
RD_BOOL
tcp_can_recv(int sck, int millis)
{
	fd_set rfds;
	struct timeval time;
	int sel_count;

	time.tv_sec = millis / 1000;
	time.tv_usec = (millis * 1000) % 1000000;

	FD_ZERO(&rfds);
	FD_SET(sck, &rfds);
	sel_count = select(sck + 1, &rfds, 0, 0, &time);

	if (sel_count > 0)
	{
		return tcp_socket_ok(sck);
	}

	return False;
}

void
tcp_write(rdpTcp * tcp, char* b, int length)
{
	int sent = 0;
	int total = 0;

	while (total < length)
	{
		while (total < length)
		{
			sent = send(tcp->sockfd, b + total, length - total, MSG_NOSIGNAL);
			if (sent <= 0)
			{
				if (sent == -1 && TCP_BLOCKS)
				{
					tcp_can_send(tcp->sockfd, 100);
					sent = 0;
				}
				else
				{
					ui_error(tcp->net->rdp->inst, "send: %s\n", TCP_STRERROR);
					return;
				}
			}
			total += sent;
		}
	}
}

int
tcp_read(rdpTcp * tcp, char* b, int length)
{
	int rcvd = 0;

	if (!ui_select(tcp->net->sec->rdp->inst, tcp->sockfd))
		return -1; /* user quit */

	rcvd = recv(tcp->sockfd, b, length, 0);

	if (rcvd < 0)
	{
		if (rcvd == -1 && TCP_BLOCKS)
		{
			tcp_can_recv(tcp->sockfd, 1);
			rcvd = 0;
		}
		else
		{
			ui_error(tcp->net->rdp->inst, "recv: %s\n", TCP_STRERROR);
			return -1;
		}
	}
	else if (rcvd == 0)
	{
		ui_error(tcp->net->rdp->inst, "Connection closed\n");
			return -1;
	}

	return rcvd;
}

/* Establish a connection on the TCP layer */
RD_BOOL
tcp_connect(rdpTcp * tcp, char * server, int port)
{
	int sockfd;
	uint32 option_value;
	socklen_t option_len;

#ifdef IPv6

	int n;
	struct addrinfo hints, *res, *ressave;
	char tcp_port_rdp_s[10];

	printf("connecting to %s:%d\n", server, port);

	snprintf(tcp_port_rdp_s, 10, "%d", port);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((n = getaddrinfo(server, tcp_port_rdp_s, &hints, &res)))
	{
		ui_error(tcp->net->rdp->inst, "getaddrinfo: %s\n", gai_strerror(n));
		return False;
	}

	ressave = res;
	sockfd = -1;
	while (res)
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (!(sockfd < 0))
		{
			if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
				break;
			TCP_CLOSE(sockfd);
			sockfd = -1;
		}
		res = res->ai_next;
	}
	freeaddrinfo(ressave);

	if (sockfd == -1)
	{
		ui_error(tcp->net->rdp->inst, "%s: unable to connect\n", server);
		return False;
	}

#else /* no IPv6 support */

	struct hostent *nslookup;
	struct sockaddr_in servaddr;

	printf("connecting to %s:%d\n", server, port);

	if ((nslookup = gethostbyname(server)) != NULL)
	{
		memcpy(&servaddr.sin_addr, nslookup->h_addr, sizeof(servaddr.sin_addr));
	}
	else if ((servaddr.sin_addr.s_addr = inet_addr(server)) == INADDR_NONE)
	{
		ui_error(tcp->net->rdp->inst, "%s: unable to resolve host\n", server);
		return False;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		ui_error(tcp->net->rdp->inst, "socket: %s\n", TCP_STRERROR);
		return False;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons((uint16) port);

	if (connect(sock, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0)
	{
		ui_error(tcp->net->rdp->inst, "connect: %s\n", TCP_STRERROR);
		TCP_CLOSE(sock);
		return False;
	}

#endif /* IPv6 */

	tcp->sockfd = sockfd;

	/* set socket as non blocking */
#ifdef _WIN32
	{
		u_long arg = 1;
		ioctlsocket(tcp->sockfd, FIONBIO, &arg);
		tcp->wsa_event = WSACreateEvent();
		WSAEventSelect(tcp->sockfd, tcp->wsa_event, FD_READ);
	}
#else
	option_value = fcntl(tcp->sockfd, F_GETFL);
	option_value = option_value | O_NONBLOCK;
	fcntl(tcp->sockfd, F_SETFL, option_value);
#endif

	option_value = 1;
	option_len = sizeof(option_value);
	setsockopt(tcp->sockfd, IPPROTO_TCP, TCP_NODELAY, (void *) &option_value, option_len);
	/* receive buffer must be a least 16 K */
	if (getsockopt(tcp->sockfd, SOL_SOCKET, SO_RCVBUF, (void *) &option_value, &option_len) == 0)
	{
		if (option_value < (1024 * 16))
		{
			option_value = 1024 * 16;
			option_len = sizeof(option_value);
			setsockopt(tcp->sockfd, SOL_SOCKET, SO_RCVBUF, (void *) &option_value,
				   option_len);
		}
	}

	return True;
}

/* Disconnect on the TCP layer */
void
tcp_disconnect(rdpTcp * tcp)
{
	if (tcp->sockfd != -1)
	{
		TCP_CLOSE(tcp->sockfd);
		tcp->sockfd = -1;
	}
#ifdef _WIN32
	if (tcp->wsa_event)
	{
		WSACloseEvent(tcp->wsa_event);
		tcp->wsa_event = 0;
	}
#endif
}

/* Returns pointer to internal buffer with lifetime as tcp */
char *
tcp_get_address(rdpTcp * tcp)
{
	struct sockaddr_in sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (getsockname(tcp->sockfd, (struct sockaddr *) &sockaddr, &len) == 0)
	{
		uint8 *ip = (uint8 *) & sockaddr.sin_addr;
		snprintf(tcp->ipaddr, sizeof(tcp->ipaddr), "%d.%d.%d.%d", ip[0], ip[1], ip[2],
			 ip[3]);
	}
	else
		strncpy(tcp->ipaddr, "127.0.0.1", sizeof(tcp->ipaddr));
	tcp->ipaddr[sizeof(tcp->ipaddr) - 1] = 0;
	return tcp->ipaddr;
}

rdpTcp *
tcp_new(struct rdp_network * net)
{
	rdpTcp * self;

	self = (rdpTcp *) xmalloc(sizeof(rdpTcp));

	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpTcp));
		self->net = net;
		self->sockfd = -1;
	}

	return self;
}

void
tcp_free(rdpTcp * tcp)
{
	if (tcp != NULL)
	{
		xfree(tcp);
	}
}
