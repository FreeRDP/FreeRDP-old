/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - TCP layer
   Copyright (C) Matthew Chapman 1999-2008

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include "tcp.h"
#include "iso.h"
#include "mcs.h"
#include "secure.h"
#include "rdp.h"
#include "mem.h"

#ifdef _WIN32
#define socklen_t int
#define TCP_CLOSE(_sck) closesocket(_sck)
#define TCP_STRERROR "tcp error"
#define TCP_BLOCKS (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#define TCP_CLOSE(_sck) close(_sck)
#define TCP_STRERROR strerror(errno)
#define TCP_BLOCKS (errno == EWOULDBLOCK)
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
	if (getsockopt(sck, SOL_SOCKET, SO_ERROR, (char*)(&opt), &opt_len) == 0)
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

/* Initialise TCP transport data packet */
STREAM
tcp_init(rdpTcp * tcp, uint32 maxlen)
{
	STREAM result;

	result = &(tcp->out);

	if (maxlen > result->size)
	{
		result->data = (uint8 *) xrealloc(result->data, maxlen);
		result->size = maxlen;
	}

	result->p = result->data;
	result->end = result->data + result->size;
	return result;
}

/* Send TCP transport data packet */
void
tcp_send(rdpTcp * tcp, STREAM s)
{
	int length = s->end - s->data;
	int sent, total = 0;

	while (total < length)
	{
		sent = send(tcp->sock, s->data + total, length - total, 0);
		if (sent <= 0)
		{
			if (sent == -1 && TCP_BLOCKS)
			{
				tcp_can_send(tcp->sock, 100);
				sent = 0;
			}
			else
			{
				ui_error(tcp->iso->mcs->sec->rdp->inst, "send: %s\n", TCP_STRERROR);
				return;
			}
		}
		total += sent;
	}
}

/* Receive a message on the TCP layer */
STREAM
tcp_recv(rdpTcp * tcp, STREAM s, uint32 length)
{
	uint32 new_length, end_offset, p_offset;
	int rcvd = 0;

	if (s == NULL)
	{
		/* read into "new" stream */
		if (length > tcp->in.size)
		{
			tcp->in.data = (uint8 *) xrealloc(tcp->in.data, length);
			tcp->in.size = length;
		}
		tcp->in.end = tcp->in.p = tcp->in.data;
		s = &(tcp->in);
	}
	else
	{
		/* append to existing stream */
		new_length = (s->end - s->data) + length;
		if (new_length > s->size)
		{
			p_offset = s->p - s->data;
			end_offset = s->end - s->data;
			s->data = (uint8 *) xrealloc(s->data, new_length);
			s->size = new_length;
			s->p = s->data + p_offset;
			s->end = s->data + end_offset;
		}
	}

	while (length > 0)
	{
		if (!ui_select(tcp->iso->mcs->sec->rdp->inst, tcp->sock))
			/* User quit */
			return NULL;

		rcvd = recv(tcp->sock, s->end, length, 0);
		if (rcvd < 0)
		{
			if (rcvd == -1 && TCP_BLOCKS)
			{
				tcp_can_recv(tcp->sock, 1);
				rcvd = 0;
			}
			else
			{
				ui_error(tcp->iso->mcs->sec->rdp->inst, "recv: %s\n", TCP_STRERROR);
				return NULL;
			}
		}
		else if (rcvd == 0)
		{
			if (tcp->iso->mcs->sec->negotiation_state < 2)
			{
				/* Disconnection is due to an encryption negotiation failure */
				tcp->iso->mcs->sec->negotiation_state = -1; /* failure */
				return NULL;
			}
			else
			{
				ui_error(tcp->iso->mcs->sec->rdp->inst, "Connection closed\n");
				return NULL;
			}
		}

		s->end += rcvd;
		length -= rcvd;
	}

	return s;
}

/* Establish a connection on the TCP layer */
RD_BOOL
tcp_connect(rdpTcp * tcp, char * server, int port)
{
	socklen_t option_len;
	uint32 option_value;

#ifdef IPv6

	int n;
	struct addrinfo hints, *res, *ressave;
	char tcp_port_rdp_s[10];

	snprintf(tcp_port_rdp_s, 10, "%d", port);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((n = getaddrinfo(server, tcp_port_rdp_s, &hints, &res)))
	{
		ui_error(tcp->iso->mcs->sec->rdp->inst, "getaddrinfo: %s\n", gai_strerror(n));
		return False;
	}

	ressave = res;
	tcp->sock = -1;
	while (res)
	{
		tcp->sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (!(tcp->sock < 0))
		{
			if (connect(tcp->sock, res->ai_addr, res->ai_addrlen) == 0)
				break;
			TCP_CLOSE(tcp->sock);
			tcp->sock = -1;
		}
		res = res->ai_next;
	}
	freeaddrinfo(ressave);

	if (tcp->sock == -1)
	{
		ui_error(tcp->iso->mcs->sec->rdp->inst, "%s: unable to connect\n", server);
		return False;
	}

#else /* no IPv6 support */

	struct hostent *nslookup;
	struct sockaddr_in servaddr;

	if ((nslookup = gethostbyname(server)) != NULL)
	{
		memcpy(&servaddr.sin_addr, nslookup->h_addr, sizeof(servaddr.sin_addr));
	}
	else if ((servaddr.sin_addr.s_addr = inet_addr(server)) == INADDR_NONE)
	{
		ui_error(tcp->iso->mcs->sec->rdp->inst, "%s: unable to resolve host\n", server);
		return False;
	}

	if ((tcp->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		ui_error(tcp->iso->mcs->sec->rdp->inst, "socket: %s\n", TCP_STRERROR);
		return False;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons((uint16) port);

	if (connect(tcp->sock, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0)
	{
		ui_error(tcp->iso->mcs->sec->rdp->inst, "connect: %s\n", TCP_STRERROR);
		TCP_CLOSE(tcp->sock);
		return False;
	}

#endif /* IPv6 */

	/* set socket as non blocking */
#ifdef _WIN32
	{
		u_long arg = 1;
		ioctlsocket(tcp->sock, FIONBIO, &arg);
		tcp->wsa_event = WSACreateEvent();
		WSAEventSelect(tcp->sock, tcp->wsa_event, FD_READ);
	}
#else
	option_value = fcntl(tcp->sock, F_GETFL);
	option_value = option_value | O_NONBLOCK;
	fcntl(tcp->sock, F_SETFL, option_value);
#endif

	option_value = 1;
	option_len = sizeof(option_value);
	setsockopt(tcp->sock, IPPROTO_TCP, TCP_NODELAY, (void *) &option_value, option_len);
	/* receive buffer must be a least 16 K */
	if (getsockopt(tcp->sock, SOL_SOCKET, SO_RCVBUF, (void *) &option_value, &option_len) == 0)
	{
		if (option_value < (1024 * 16))
		{
			option_value = 1024 * 16;
			option_len = sizeof(option_value);
			setsockopt(tcp->sock, SOL_SOCKET, SO_RCVBUF, (void *) &option_value,
				   option_len);
		}
	}

	return True;
}

/* Disconnect on the TCP layer */
void
tcp_disconnect(rdpTcp * tcp)
{
	TCP_CLOSE(tcp->sock);
#ifdef _WIN32
	if (tcp->wsa_event)
	{
		WSACloseEvent(tcp->wsa_event);
		tcp->wsa_event = 0;
	}
#endif
}

char *
tcp_get_address(rdpTcp * tcp)
{
	struct sockaddr_in sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (getsockname(tcp->sock, (struct sockaddr *) &sockaddr, &len) == 0)
	{
		uint8 *ip = (uint8 *) & sockaddr.sin_addr;
		sprintf(tcp->ipaddr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	}
	else
		strcpy(tcp->ipaddr, "127.0.0.1");
	return tcp->ipaddr;
}

/* reset the state of the tcp layer */
/* Support for Session Directory */
void
tcp_reset_state(rdpTcp * tcp)
{
	tcp->sock = -1;		/* reset socket */

	/* Clear the incoming stream - put preserve its data store */
	tcp->in.p = NULL;
	tcp->in.end = NULL;
	tcp->in.iso_hdr = NULL;
	tcp->in.mcs_hdr = NULL;
	tcp->in.sec_hdr = NULL;
	tcp->in.rdp_hdr = NULL;
	tcp->in.channel_hdr = NULL;

	/* Clear the outgoing stream - put preserve its data store */
	tcp->out.p = NULL;
	tcp->out.end = NULL;
	tcp->out.iso_hdr = NULL;
	tcp->out.mcs_hdr = NULL;
	tcp->out.sec_hdr = NULL;
	tcp->out.rdp_hdr = NULL;
	tcp->out.channel_hdr = NULL;
}

rdpTcp *
tcp_new(struct rdp_iso * iso)
{
	rdpTcp * self;

	self = (rdpTcp *) xmalloc(sizeof(rdpTcp));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpTcp));
		self->iso = iso;

		self->in.size = 4096;
		self->in.data = (uint8 *) xmalloc(self->in.size);

		self->out.size = 4096;
		self->out.data = (uint8 *) xmalloc(self->out.size);
	}
	return self;
}

void
tcp_free(rdpTcp * tcp)
{
	if (tcp != NULL)
	{
		xfree(tcp->in.data);
		xfree(tcp->out.data);
		xfree(tcp);
	}
}
