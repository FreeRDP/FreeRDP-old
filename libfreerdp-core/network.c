/*
   FreeRDP: A Remote Desktop Protocol client.
   Network Transport Abstraction Layer

   Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <freerdp/utils/memory.h>

#include "network.h"

/* Initialize and return STREAM.
 * The stream will have room for at least min_size.
 * The tcp layers out stream will be used. */

STREAM
network_stream_init(rdpNetwork * net, uint32 min_size)
{
	STREAM result = &(net->out);

	if (min_size > result->size)
	{
		result->data = (uint8 *) xrealloc(result->data, min_size);
		result->size = min_size;
	}

	result->p = result->data;
	result->end = result->data + result->size;

	return result;
}

#ifndef DISABLE_TLS

/* verify SSL/TLS connection integrity. 2 checks are carried out. First make sure that the
 * certificate is assigned to the server we're connected to, and second make sure that the
 * certificate is signed by a trusted certification authority
 */

RD_BOOL
network_verify_tls(rdpNetwork * net)
{
	char * issuer;
	char * subject;
	char * fingerprint;
	CryptoCert cert;
	RD_BOOL verified = False;

#ifdef _WIN32
	/*
	 * TODO: FIX ME! This is really bad, I know...
	 * There appears to be a buffer overflow only
	 * on Windows that affects this part of the code.
	 * Skipping it is a workaround, but it's obviously
	 * not a permanent "solution".
	 */
	return True;
#endif

	cert = tls_get_certificate(net->tls);

	if (!cert)
	{
		goto exit;
	}

	subject = crypto_cert_get_subject(cert);
	issuer = crypto_cert_get_issuer(cert);
	fingerprint = crypto_cert_get_fingerprint(cert);

	verified = tls_verify(net->tls, net->server);

	if (verified != False)
		verified = crypto_cert_verify_peer_identity(cert, net->server);

	verified = ui_check_certificate(net->rdp->inst, fingerprint, subject, issuer, verified);

	xfree(fingerprint);
	xfree(subject);
	xfree(issuer);

exit:
	if (cert)
	{
		crypto_cert_free(cert);
		cert = NULL;
	}

	return verified;
}
#endif

RD_BOOL
network_connect(rdpNetwork * net, char* server, char* username, int port)
{
	NEGO *nego = net->iso->nego;

	net->port = port;
	net->server = server;
	net->username = username;
	net->license->license_issued = 0;

	if (net->rdp->settings->nla_security)
		nego->enabled_protocols[PROTOCOL_NLA] = 1;
	if (net->rdp->settings->tls_security)
		nego->enabled_protocols[PROTOCOL_TLS] = 1;
	if (net->rdp->settings->rdp_security)
		nego->enabled_protocols[PROTOCOL_RDP] = 1;

	if (!iso_connect(net->iso, net->server, net->username, net->port))
		return False;

#ifndef DISABLE_TLS
	if(nego->selected_protocol & PROTOCOL_NLA)
	{
		/* TLS with NLA was successfully negotiated */

		RD_BOOL status = 1;
		printf("TLS encryption with NLA negotiated\n");
		net->tls = tls_new();

		if (!tls_connect(net->tls, net->tcp->sock))
			return False;

		if (!network_verify_tls(net))
			return False;

		net->sec->tls_connected = 1;
		net->rdp->settings->encryption = 0;

		if (!net->rdp->settings->autologin)
			if (!ui_authenticate(net->rdp->inst))
				return False;

		net->credssp = credssp_new(net);

		if (credssp_authenticate(net->credssp) < 0)
		{
			printf("Authentication failure, check credentials.\n"
					"If credentials are valid, the NTLMSSP implementation may be to blame.\n");
			credssp_free(net->credssp);
			return 0;
		}

		credssp_free(net->credssp);

		status = mcs_connect(net->mcs);
		return status;
	}
	else if(nego->selected_protocol & PROTOCOL_TLS)
	{
		/* TLS without NLA was successfully negotiated */
		RD_BOOL success;
		printf("TLS encryption negotiated\n");
		net->tls = tls_new();

		if (!tls_connect(net->tls, net->tcp->sock))
			return False;

		if (!network_verify_tls(net))
			return False;

		net->sec->tls_connected = 1;
		net->rdp->settings->encryption = 0;

		success = mcs_connect(net->mcs);

		return success;
	}
	else
#endif
	{
		RD_BOOL success;

		printf("Standard RDP encryption negotiated\n");

		success = mcs_connect(net->mcs);

		if (success && net->rdp->settings->encryption)
			sec_establish_key(net->sec);

		return success;
	}

	return 0;
}

void
network_disconnect(rdpNetwork * net)
{
#ifndef DISABLE_TLS
	if (net->tls)
		tls_disconnect(net->tls);
#endif
	tcp_disconnect(net->tcp);
}

void
network_send(rdpNetwork * net, STREAM s)
{
#ifndef DISABLE_TLS
	if (net->sec->tls_connected)
	{
		tls_write(net->tls, (char*) s->data, s->end - s->data);
	}
	else
#endif
	{
		tcp_write(net->tcp, s);
	}
}

STREAM
network_recv(rdpNetwork * net, STREAM s, uint32 length)
{
	int rcvd = 0;
	uint32 p_offset;
	uint32 new_length;
	uint32 end_offset;

	if (s == NULL)
	{
		/* read into "new" stream */
		if (length > net->in.size)
		{
			net->in.data = (uint8 *) xrealloc(net->in.data, length);
			net->in.size = length;
		}

		net->in.end = net->in.p = net->in.data;
		s = &(net->in);
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
#ifndef DISABLE_TLS
		if (net->sec->tls_connected)
		{
			rcvd = tls_read(net->tls, (char*) s->end, length);

			if (rcvd < 0)
				return NULL;
		}
		else
#endif
		{
			rcvd = tcp_read(net->tcp, (char*) s->end, length);
		}

		s->end += rcvd;
		length -= rcvd;
	}

	return s;
}

rdpNetwork*
network_new(rdpRdp * rdp)
{
	rdpNetwork * self;

	self = (rdpNetwork *) xmalloc(sizeof(rdpNetwork));

	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpNetwork));
		self->rdp = rdp;
		self->sec = rdp->sec;
		self->mcs = mcs_new(self);
		self->tcp = tcp_new(self);
		self->nego = nego_new(self);
		self->iso = iso_new(self);
		self->license = license_new(self);
		self->sec->net = self;

		self->in.size = 4096;
		self->in.data = (uint8 *) xmalloc(self->in.size);

		self->out.size = 4096;
		self->out.data = (uint8 *) xmalloc(self->out.size);
	}

	return self;
}

void
network_free(rdpNetwork * net)
{
	if (net != NULL)
	{
		xfree(net->in.data);
		xfree(net->out.data);

		if (net->tcp != NULL)
			tcp_free(net->tcp);

		if (net->iso != NULL)
			iso_free(net->iso);

		if (net->mcs != NULL)
			mcs_free(net->mcs);

		if (net->nego != NULL)
			nego_free(net->nego);

		if (net->license != NULL)
			license_free(net->license);

		xfree(net);
	}
}
