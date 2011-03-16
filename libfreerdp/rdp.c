/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP layer

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

#include <time.h>
#ifndef _WIN32
#include <errno.h>
#include <unistd.h>
#endif
#include <freerdp/utils.h>
#include <freerdp/freerdp.h>
#include "frdp.h"
#include "iso.h"
#include "tcp.h"
#include "mcs.h"
#include "secure.h"
#include "rdp.h"
#include "rail.h"
#include "capabilities.h"
#include "orders.h"
#include "pstcache.h"
#include "cache.h"
#include "bitmap.h"
#include "debug.h"
#include "ext.h"
#include "surface.h"

RD_BOOL
rdp_global_init(void)
{
	return sec_global_init();
}

void
rdp_global_finish(void)
{
	sec_global_finish();
}

static void
process_redirect_pdu(rdpRdp * rdp, STREAM s);
static RD_BOOL
process_data_pdu(rdpRdp * rdp, STREAM s);
static void
process_fp(rdpRdp * rdp, STREAM s);


/* Receive an RDP packet */
static STREAM
rdp_recv(rdpRdp * rdp, enum RDP_PDU_TYPE * type, uint16 * source)
{
	uint16 totalLength;
	uint16 pduType;
	secRecvType sec_type;

	*type = RDP_PDU_NULL;
	*source = 0;
	if ((rdp->rdp_s == NULL) || (rdp->next_packet >= rdp->rdp_s->end))
	{
		rdp->rdp_s = sec_recv(rdp->sec, &sec_type);

		if (rdp->rdp_s == NULL)
			return NULL;

		if (sec_type == SEC_RECV_IOCHANNEL)
		{
			rdp->next_packet = rdp->rdp_s->end;
			return rdp->rdp_s;
		}
		else if (sec_type == SEC_RECV_FAST_PATH)
		{
			/* process_fp should move rdp->next_packet ok */
			process_fp(rdp, rdp->rdp_s);
			return rdp->rdp_s;
		}
		else if (sec_type == SEC_RECV_REDIRECT)
		{
			process_redirect_pdu(rdp, rdp->rdp_s);
			rdp->next_packet = rdp->rdp_s->p;
			return rdp->rdp_s;
		}
		ASSERT(sec_type == SEC_RECV_SHARE_CONTROL);

		rdp->next_packet = rdp->rdp_s->p;
	}
	else
	{
		ASSERT(rdp->next_packet < rdp->rdp_s->end);
		rdp->rdp_s->p = rdp->next_packet;
	}

	/* Share Control Header: */
	in_uint16_le(rdp->rdp_s, totalLength); /* totalLength */

	/* Undocumented!(?): 32k packets are keepalive packages of length 8: */
	if (totalLength == 0x8000)
	{
		DEBUG("keepalive\n");
		rdp->next_packet += 8;
		return rdp->rdp_s;
	}

	rdp->next_packet += totalLength;

	in_uint16_le(rdp->rdp_s, pduType); /* pduType */
	in_uint16_le(rdp->rdp_s, *source);	/* PDUSource */

	if (pduType == RDP_PDU_SERVER_REDIR_PKT)
	{
		DEBUG("Enhanced Security Server Redirection PDU\n");
		in_uint8s(rdp->rdp_s, 2);
		process_redirect_pdu(rdp, rdp->rdp_s);
		if (rdp->rdp_s->p == rdp->next_packet - 1)
			in_uint8s(rdp->rdp_s, 1);
		ASSERT(rdp->rdp_s->p == rdp->next_packet);
		return rdp->rdp_s;
	}

	if ((pduType >> 8 != 0) || (((pduType >> 4) & 0xF) != 1))
	{
		ui_error(rdp->inst, "pduType version must be 0 and 1 but is %d and %d\n", pduType >> 8, (pduType >> 4) & 0xF);
	}
	*type = pduType & 0xF; /* type is in 4 lower bits, version in high bits */

#if WITH_DEBUG
	DEBUG("Share Control Data PDU #%d, (type %x)\n", ++(rdp->packetno), *type);
	hexdump(rdp->next_packet, totalLength);
#endif

	return rdp->rdp_s;
}

/* Initialise an RDP data packet */
static STREAM
rdp_init_data(rdpRdp * rdp, int maxlen)
{
	STREAM s;

	uint32 sec_flags;

	if (rdp->sec->tls_connected)
		sec_flags = 0;
	else
		sec_flags = rdp->settings->encryption ? SEC_ENCRYPT : 0;

	s = sec_init(rdp->sec, sec_flags, maxlen + 18);
	s_push_layer(s, rdp_hdr, 18);
	return s;
}

/* Initialise a fast path RDP data packet */
static STREAM
rdp_fp_init(rdpRdp * rdp, int maxlen)
{
	STREAM s;
	s = sec_fp_init(rdp->sec, rdp->settings->encryption ? SEC_ENCRYPT : 0, maxlen);
	return s;
}

/* Send an RDP data packet */
static void
rdp_send_data(rdpRdp * rdp, STREAM s, uint8 data_pdu_type)
{
	uint16 length;
	uint16 uncompressedLength;

	s_pop_layer(s, rdp_hdr);
	length = (int) (s->end - s->p);
	uncompressedLength = length - 14;

	/* Share Control Header */
	out_uint16_le(s, length); /* totalLength */
	out_uint16_le(s, (RDP_PDU_DATA | 0x10)); /* pduType */
	out_uint16_le(s, (rdp->sec->mcs->mcs_userid + 1001)); /* PDUSource */

	/* Share Data Header */
	out_uint32_le(s, rdp->rdp_shareid);	/* shareId */
	out_uint8(s, 0);			/* pad1 */
	out_uint8(s, STREAM_LOW);		/* streamId */
	out_uint16_le(s, uncompressedLength);	/* uncompressedLength */
	out_uint8(s, data_pdu_type);		/* pduType2 */
	out_uint8(s, 0);			/* compressedType */
	out_uint16_le(s, 0);			/* compressedLength */

	sec_send(rdp->sec, s, rdp->settings->encryption ? SEC_ENCRYPT : 0);
}

/* Send a fast path RDP data packet */
static void
rdp_fp_send(rdpRdp * rdp, STREAM s)
{
	sec_fp_send(rdp->sec, s, rdp->settings->encryption ? SEC_ENCRYPT : 0);
}

int
rdp_send_frame_ack(rdpRdp * rdp, int frame_id)
{
	STREAM s;

	if (rdp->send_frame_ack == 0)
	{
		return 0;
	}
	printf("send_frame_ack: frame_id %d\n", frame_id);
	s = rdp_init_data(rdp, 4);
	out_uint32_le(s, frame_id);
	s_mark_end(s);
	rdp_send_data(rdp, s, 56);
	return 0;
}

/* Output system time structure */
void
rdp_out_systemtime(STREAM s, systemTime sysTime)
{
	out_uint16_le(s, sysTime.wYear);		/* wYear, must be set to zero */
	out_uint16_le(s, sysTime.wMonth);		/* wMonth */
	out_uint16_le(s, sysTime.wDayOfWeek);		/* wDayOfWeek */
	out_uint16_le(s, sysTime.wDay);			/* wDay */
	out_uint16_le(s, sysTime.wHour);		/* wHour */
	out_uint16_le(s, sysTime.wMinute);		/* wMinute */
	out_uint16_le(s, sysTime.wSecond);		/* wSecond */
	out_uint16_le(s, sysTime.wMilliseconds);	/* wMilliseconds */
}

/* Output client time zone information structure */
void
rdp_out_client_timezone_info(rdpRdp * rdp, STREAM s)
{
		uint32 bias;
		uint32 standardBias;
		uint32 daylightBias;
		char standardName[32];
		char daylightName[32];
		systemTime standardDate;
		systemTime daylightDate;
		void * p;
		size_t len;

		time_t t;
		struct tm* localTime;

		time(&t);
		localTime = localtime(&t);

#if defined(sun)
		if(localTime->tm_isdst > 0)
			bias = (uint32)(altzone / 3600);
		else
			bias = (uint32)(timezone / 3600);
#elif defined(HAVE_TM_GMTOFF)
		if(localTime->tm_gmtoff >= 0)
			bias = (uint32)(localTime->tm_gmtoff / 60);
		else
			bias = (uint32)((-1 * localTime->tm_gmtoff) / 60 + 720);
#else
		/* TODO: Implement another way of finding the bias */
		bias = 0;
#endif

		if(localTime->tm_isdst > 0)
		{
			standardBias = bias - 60;
			daylightBias = bias;
		}
		else
		{
			standardBias = bias;
			daylightBias = bias + 60;
		}

		strftime(standardName, 32, "%Z, Standard Time", localTime);
		standardName[31] = 0;
		strftime(daylightName, 32, "%Z, Summer Time", localTime);
		daylightName[31] = 0;

		memset((void*)&standardDate, '\0', sizeof(systemTime));
		memset((void*)&daylightDate, '\0', sizeof(systemTime));

		out_uint32_le(s, bias);			/* bias */

		p = freerdp_uniconv_out(rdp->uniconv, standardName, &len);
		ASSERT(len <= 64 - 2);
		out_uint8a(s, p, len + 2);
		out_uint8s(s, 64 - len - 2);		/* standardName (64 bytes) */
		xfree(p);

		rdp_out_systemtime(s, standardDate);	/* standardDate */
		out_uint32_le(s, standardBias);		/* standardBias */

		p = freerdp_uniconv_out(rdp->uniconv, daylightName, &len);
		ASSERT(len <= 64 - 2);
		out_uint8a(s, p, len + 2);
		out_uint8s(s, 64 - len - 2);		/* daylightName (64 bytes) */
		xfree(p);

		rdp_out_systemtime(s, daylightDate);	/* daylightDate */
		out_uint32_le(s, daylightBias);		/* daylightBias */
}

static char dll[] = "C:\\Windows\\System32\\mstscax.dll";

/* Send Client Info PDU Data */
static void
rdp_send_client_info(rdpRdp * rdp, uint32 flags, char *domain_name,
                     char *username, char *password, size_t cbPassword, char *shell, char *dir)
{
	STREAM s;
	int length;
	char* domain;
	char* userName;
	char* alternateShell;
	char* workingDir;
	char* clientAddress;
	char* clientDir;
	size_t cbDomain;
	size_t cbUserName;
	size_t cbAlternateShell;
	size_t cbWorkingDir;
	size_t cbClientAddress;
	size_t cbClientDir;
	uint32 sec_flags;

	flags |= INFO_UNICODE;
	flags |= INFO_LOGONERRORS;
	flags |= INFO_LOGONNOTIFY;
	flags |= INFO_ENABLEWINDOWSKEY;
	flags |= RNS_INFO_AUDIOCAPTURE;

	if (rdp->settings->autologin)
		flags |= INFO_AUTOLOGON;

	if (rdp->settings->bulk_compression)
		flags |= INFO_COMPRESSION | PACKET_COMPR_TYPE_64K;

	domain = freerdp_uniconv_out(rdp->uniconv, domain_name, &cbDomain);
	userName = freerdp_uniconv_out(rdp->uniconv, username, &cbUserName);
	alternateShell = freerdp_uniconv_out(rdp->uniconv, shell, &cbAlternateShell);
	workingDir = freerdp_uniconv_out(rdp->uniconv, dir, &cbWorkingDir);
	clientAddress = freerdp_uniconv_out(rdp->uniconv, tcp_get_address(rdp->sec->mcs->iso->tcp), &cbClientAddress);
	clientDir = freerdp_uniconv_out(rdp->uniconv, dll, &cbClientDir); /* client working directory OR binary name */

	length = 8 + (5 * 4) + cbDomain + cbUserName + cbPassword + cbAlternateShell + cbWorkingDir;

	if (rdp->settings->rdp_version >= 5)
		length += 180 + (2 * 4) + cbClientAddress + cbClientDir;

	sec_flags = SEC_INFO_PKT | (rdp->settings->encryption ? SEC_ENCRYPT : 0);
	s = sec_init(rdp->sec, sec_flags, length);

	/* INFO_UNICODE is set, so codePage contains the active locale identifier in the low word (see MSDN-MUI) */

	/*
	 * When codePage is non-zero, the server seems to be using it to determine the keyboard layout ID,
	 * with the side effect of ignoring the keyboard layout ID sent in the Client Core Data
	 */
	out_uint32_le(s, 0);		/* codePage */
	out_uint32_le(s, flags);	/* flags */

	out_uint16_le(s, cbDomain);		/* cbDomain */
	out_uint16_le(s, cbUserName);		/* cbUserName */
	out_uint16_le(s, cbPassword);		/* cbPassword */
	out_uint16_le(s, cbAlternateShell);	/* cbAlternateShell */
	out_uint16_le(s, cbWorkingDir);		/* cbWorkingDir */

	if (cbDomain > 0)
		out_uint8p(s, domain, cbDomain);			/* domain */
	out_uint16_le(s, 0);

	out_uint8a(s, userName, cbUserName);				/* userName */
	out_uint16_le(s, 0);

	if (cbPassword > 0)
		out_uint8p(s, password, cbPassword);			/* password */
	out_uint16_le(s, 0);

	if (cbAlternateShell > 0)
		out_uint8p(s, alternateShell, cbAlternateShell);	/* alternateShell */
	out_uint16_le(s, 0);

	if (cbWorkingDir > 0)
		out_uint8p(s, workingDir, cbWorkingDir);		/* workingDir */
	out_uint16_le(s, 0);

	/* extraInfo: optional, used in RDP 5.0, 5.1, 5.2, 6.0, 6.1 and 7.0 */
	if (rdp->settings->rdp_version >= 5)
	{
		out_uint16_le(s, CLIENT_INFO_AF_INET);			/* clientAddressFamily (IPv4) */
		out_uint16_le(s, cbClientAddress + 2);			/* cbClientAddress */
		out_uint8a(s, clientAddress, cbClientAddress + 2);	/* clientAddress */
		out_uint16_le(s, cbClientDir + 2);			/* cbClientDir */
		out_uint8a(s, clientDir, cbClientDir + 2);		/* clientDir */
		rdp_out_client_timezone_info(rdp, s);			/* clientTimeZone (172 bytes) */
		out_uint32_le(s, 0);					/* clientSessionId, should be set to zero */
		if (rdp->settings->performanceflags == PERF_FLAG_NONE)
		{
			rdp->settings->performanceflags |= PERF_ENABLE_DESKTOP_COMPOSITION;
		}
		out_uint32_le(s, rdp->settings->performanceflags);	/* performanceFlags */
		out_uint16_le(s, 0);					/* cbAutoReconnectLen */
		/* autoReconnectCookie */ /* FIXME: populate this field */
		/* reserved1 (2 bytes) */
		/* reserved2 (2 bytes) */
	}

	s_mark_end(s);
	sec_send(rdp->sec, s, sec_flags);

	xfree(domain);
	xfree(userName);
	xfree(alternateShell);
	xfree(workingDir);
	xfree(clientAddress);
	xfree(clientDir);
}

/* Send a control PDU */
static void
rdp_send_control(rdpRdp * rdp, uint16 action)
{
	STREAM s;

	s = rdp_init_data(rdp, 8);

	out_uint16_le(s, action);	/* action */
	out_uint16_le(s, 0);		/* grantID */
	out_uint32_le(s, 0);		/* controlID */

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_CONTROL);
}

/* Send a synchronisation PDU */
static void
rdp_send_synchronize(rdpRdp * rdp)
{
	STREAM s;

	s = rdp_init_data(rdp, 4);

	out_uint16_le(s, 1);		/* messageType */
	out_uint16_le(s, rdp->rdp_serverid);		/* targetUser, the MCS channel ID of the target user */

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_SYNCHRONIZE);
}

/* Send a single input event */
void
rdp_send_input(rdpRdp * rdp, time_t time, uint16 message_type, uint16 device_flags, uint16 param1, uint16 param2)
{
	STREAM s;
	int fp_flags;

	if (rdp->use_input_fast_path)
	{
		switch (message_type)
		{
			case RDP_INPUT_SCANCODE:
				fp_flags = 0 << 5; /* FASTPATH_INPUT_EVENT_SCANCODE */
				if (device_flags & KBD_FLAG_UP)
				{
					fp_flags |= 1; /* FASTPATH_INPUT_KBDFLAGS_RELEASE */
				}
				if (device_flags & KBD_FLAG_EXT)
				{
					fp_flags |= 2; /* FASTPATH_INPUT_KBDFLAGS_EXTENDED */
				}
				s = rdp_fp_init(rdp, 2);
				out_uint8(s, fp_flags);
				out_uint8(s, (uint8)param1);	/* Scan code */
				s_mark_end(s);
				rdp_fp_send(rdp, s);
				break;
			case RDP_INPUT_MOUSE:
				fp_flags = 1 << 5; /* FASTPATH_INPUT_EVENT_MOUSE */
				s = rdp_fp_init(rdp, 7);
				out_uint8(s, fp_flags);
				out_uint16_le(s, device_flags);
				out_uint16_le(s, param1);
				out_uint16_le(s, param2);
				s_mark_end(s);
				rdp_fp_send(rdp, s);
				break;
			case RDP_INPUT_MOUSEX:
				fp_flags = 2 << 5; /* FASTPATH_INPUT_EVENT_MOUSEX */
				ui_unimpl(rdp->inst, "rdp_send_input: TS_FP_INPUT_EVENT "
					"FASTPATH_INPUT_EVENT_MOUSEX\n");
				break;
			default:
				ui_unimpl(rdp->inst, "rdp_send_input: TS_FP_INPUT_EVENT %x\n",
					message_type);
				break;
		}
	}
	else
	{
		s = rdp_init_data(rdp, 16);
		out_uint16_le(s, 1); /* number of events */
		out_uint16_le(s, 0); /* pad */
		out_uint32_le(s, (uint32)time);
		out_uint16_le(s, message_type);
		out_uint16_le(s, device_flags);
		out_uint16_le(s, param1);
		out_uint16_le(s, param2);
		s_mark_end(s);
		rdp_send_data(rdp, s, RDP_DATA_PDU_INPUT);
	}
}

/* Send a single keyboard synchronize event */
void
rdp_sync_input(rdpRdp * rdp, time_t time, uint32 toggle_keys_state)
{
	STREAM s;
	int fp_flags;

	if (rdp->use_input_fast_path)
	{
		fp_flags = 3 << 5; /* FASTPATH_INPUT_EVENT_SYNC */
		fp_flags |= toggle_keys_state & 0xf;
		/* FASTPATH_INPUT_SYNC_SCROLL_LOCK = KBD_SYNC_SCROLL_LOCK = 1
		   FASTPATH_INPUT_SYNC_NUM_LOCK    = KBD_SYNC_NUM_LOCK    = 2
		   FASTPATH_INPUT_SYNC_CAPS_LOCK   = KBD_SYNC_CAPS_LOCK   = 4
		   FASTPATH_INPUT_SYNC_KANA_LOCK   = KBD_SYNC_KANA_LOCK   = 8 */
		s = rdp_fp_init(rdp, 1);
		out_uint8(s, fp_flags);
		s_mark_end(s);
		rdp_fp_send(rdp, s);
	}
	else
	{
		s = rdp_init_data(rdp, 16);
		out_uint16_le(s, 1); /* number of events */
		out_uint16_le(s, 0); /* pad */
		out_uint32_le(s, (uint32)time); /* eventTime */
		out_uint16_le(s, RDP_INPUT_SYNC); /* messageType */
		out_uint16_le(s, 0); /* pad */
		out_uint32_le(s, toggle_keys_state); /* toggleFlags */
		s_mark_end(s);
		rdp_send_data(rdp, s, RDP_DATA_PDU_INPUT);
	}
}

/* Send a single unicode character input event */
void
rdp_unicode_input(rdpRdp * rdp, time_t time, uint16 unicode_character)
{
	STREAM s;
	int fp_flags;

	if (rdp->use_input_fast_path)
	{
		fp_flags = 4 << 5; /* FASTPATH_INPUT_EVENT_UNICODE */
		s = rdp_fp_init(rdp, 3);
		out_uint8(s, fp_flags);
		out_uint16_le(s, unicode_character);
		s_mark_end(s);
		rdp_fp_send(rdp, s);
	}
	else
	{
		s = rdp_init_data(rdp, 16);
		out_uint16_le(s, 1); /* number of events */
		out_uint16_le(s, 0); /* pad */
		out_uint32_le(s, (uint32)time); /* eventTime */
		out_uint16_le(s, RDP_INPUT_UNICODE); /* messageType */
		out_uint16_le(s, 0); /* pad */
		out_uint16_le(s, unicode_character); /* Unicode character */
		out_uint16_le(s, 0); /* pad */
		s_mark_end(s);
		rdp_send_data(rdp, s, RDP_DATA_PDU_INPUT);
	}
}

/* Send a client window information PDU */
void
rdp_send_client_window_status(rdpRdp * rdp, int status)
{
	STREAM s;

	if (rdp->current_status == status)
		return;

	s = rdp_init_data(rdp, 12);

	out_uint32_le(s, status);

	switch (status)
	{
		case 0:	/* shut the server up */
			break;

		case 1:	/* receive data again */
			out_uint32_le(s, 0);	/* unknown */
			out_uint16_le(s, rdp->settings->width);
			out_uint16_le(s, rdp->settings->height);
			break;
	}

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_SUPPRESS_OUTPUT);
	rdp->current_status = status;
}

/* Send persistent bitmap cache enumeration PDU's */
static void
rdp_enum_bmpcache2(rdpRdp * rdp)
{
	STREAM s;
	HASH_KEY keylist[BMPCACHE2_NUM_PSTCELLS];
	uint32 num_keys, offset, count, flags;

	offset = 0;
	num_keys = pstcache_enumerate(rdp->pcache, 2, keylist);

	while (offset < num_keys)
	{
		count = MIN(num_keys - offset, 169);

		s = rdp_init_data(rdp, 24 + count * sizeof(HASH_KEY));

		flags = 0;
		if (offset == 0)
			flags |= BITMAP_PERSIST_FLAG_FIRST;
		if (num_keys - offset <= 169)
			flags |= BITMAP_PERSIST_FLAG_LAST;

		/* header */
		out_uint32_le(s, 0);
		out_uint16_le(s, count);
		out_uint16_le(s, 0);
		out_uint16_le(s, 0);
		out_uint16_le(s, 0);
		out_uint16_le(s, 0);
		out_uint16_le(s, num_keys);
		out_uint32_le(s, 0);
		out_uint32_le(s, flags);

		/* list */
		out_uint8a(s, keylist[offset], count * sizeof(HASH_KEY));

		s_mark_end(s);
		rdp_send_data(rdp, s, 0x2b);

		offset += 169;
	}
}

/* Send an (empty) font information PDU */
static void
rdp_send_fonts(rdpRdp * rdp, uint16 seq)
{
	STREAM s;

	s = rdp_init_data(rdp, 8);

	out_uint16_le(s, 0);		/* numberFonts, should be set to zero */
	out_uint16_le(s, 0);		/* totalNumFonts, should be set to zero */
	out_uint16_le(s, seq);		/* listFlags */
	out_uint16_le(s, 0x0032);	/* entrySize, should be set to 0x0032 */

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_FONTLIST);
}

/* Send a confirm active PDU */
static void
rdp_send_confirm_active(rdpRdp * rdp)
{
	STREAM s;
	STREAM caps;
	int caplen;
	int length;
	uint32 sec_flags;
	uint16 numberCapabilities = 14;

	caps = (STREAM) xmalloc(sizeof(struct stream));
	memset(caps, 0, sizeof(struct stream));
	caps->size = 8192;
	caps->p = caps->data = (uint8 *) xmalloc(caps->size);
	caps->end = caps->data + caps->size;

	rdp_out_general_capset(rdp, caps);
	rdp_out_bitmap_capset(rdp, caps);
	rdp_out_order_capset(rdp, caps);
	if (rdp->settings->rdp_version >= 5)
	{
		rdp_out_bitmapcache_rev2_capset(rdp, caps);
	}
	else
	{
		rdp_out_bitmapcache_capset(rdp, caps);
	}
	rdp_out_colorcache_capset(caps);
	rdp_out_window_activation_capset(caps);
	rdp_out_control_capset(caps);
	rdp_out_pointer_capset(rdp, caps);
	rdp_out_share_capset(caps);
	rdp_out_input_capset(rdp, caps);
	rdp_out_brush_capset(caps);
	rdp_out_sound_capset(caps);
	rdp_out_font_capset(caps);
	if (rdp->settings->off_screen_bitmaps)
	{
		numberCapabilities++;
		rdp_out_offscreenscache_capset(caps);
	}
	rdp_out_glyphcache_capset(caps);
	if (rdp->settings->remote_app)
	{
		numberCapabilities += 2;
		rdp_out_rail_capset(caps);
		rdp_out_window_capset(caps);
	}
	if (rdp->got_large_pointer_caps)
	{
		numberCapabilities++;
		rdp_out_large_pointer_capset(rdp, caps);
	}
	if (rdp->got_frame_ack_caps)
	{
		if (rdp->settings->use_frame_ack)
		{
			numberCapabilities++;
			rdp_out_frame_ack_capset(rdp, caps);
		}
	}
	if (rdp->got_surface_commands_caps)
	{
		numberCapabilities++;
		rdp_out_surface_commands_capset(rdp, caps);
	}
	if (rdp->got_multifragmentupdate_caps)
	{
		numberCapabilities++;
		rdp_out_multifragmentupdate_capset(rdp, caps);
	}
	if (rdp->got_bitmap_codecs_caps)
	{
		numberCapabilities++;
		rdp_out_bitmap_codecs_capset(rdp, caps);
	}
	s_mark_end(caps);
	caplen = (int) (caps->end - caps->data);

	if (rdp->sec->tls_connected)
		sec_flags = 0;
	else
		sec_flags = rdp->settings->encryption ? SEC_ENCRYPT : 0;

	length = 6 + 14 + caplen + sizeof(RDP_SOURCE);
	s = sec_init(rdp->sec, sec_flags, length);

	/* share control header */
	out_uint16_le(s, length);
	out_uint16_le(s, (RDP_PDU_CONFIRM_ACTIVE | 0x10));	/* Version 1 */
	out_uint16_le(s, (rdp->sec->mcs->mcs_userid + 1001));

	out_uint32_le(s, rdp->rdp_shareid); /* sharedId */
	/* originatorId must be set to the server channel ID
	   This value is always 0x3EA for Microsoft RDP server implementations */
	out_uint16_le(s, rdp->rdp_serverid); /* originatorId */
	out_uint16_le(s, sizeof(RDP_SOURCE)); /* lengthSourceDescriptor */
	/* lengthCombinedCapabilities is the combined size of
	   numberCapabilities, pad2Octets and capabilitySets */
	out_uint16_le(s, caplen + 4); /* lengthCombinedCapabilities */

	/* sourceDescriptor is "MSTSC" for Microsoft RDP clients */
	out_uint8p(s, RDP_SOURCE, sizeof(RDP_SOURCE)); /* sourceDescriptor */
	out_uint16_le(s, numberCapabilities); /* numberCapabilities */
	out_uint8s(s, 2); /* pad2Octets */
	out_uint8p(s, caps->data, caplen); /* capabilitySets */
	xfree(caps->data);
	xfree(caps);
	s_mark_end(s);
	sec_send(rdp->sec, s, sec_flags);
}

/* Process a general capability set */
static void
rdp_process_general_caps(rdpRdp * rdp, STREAM s)
{
	uint16 pad2octetsB;	/* rdp5 flags? */

	in_uint8s(s, 10);
	in_uint16_le(s, pad2octetsB);

	if (!pad2octetsB)
		rdp->settings->rdp_version = 4;
}

/* Process server capabilities */
static void
rdp_process_server_caps(rdpRdp * rdp, STREAM s, uint16 length)
{
	int n;
	uint8 *next, *start;
	uint16 numberCapabilities;
	uint16 capabilitySetType;
	uint16 lengthCapability;

	start = s->p;

	in_uint16_le(s, numberCapabilities); /* numberCapabilities */
	in_uint8s(s, 2); /* pad */

	for (n = 0; n < numberCapabilities; n++)
	{
		if (s->p > start + length)
			return;

		in_uint16_le(s, capabilitySetType);
		in_uint16_le(s, lengthCapability);

		next = s->p + lengthCapability - 4;

		switch (capabilitySetType)
		{
			case CAPSET_TYPE_GENERAL:
				rdp_process_general_caps(rdp, s);
				break;

			case CAPSET_TYPE_ORDER:
				rdp_process_order_capset(rdp, s);
				break;

			case CAPSET_TYPE_BITMAP:
				rdp_process_bitmap_capset(rdp, s);
				break;

			case CAPSET_TYPE_POINTER:
				rdp_process_pointer_capset(rdp, s);
				break;

			case CAPSET_TYPE_SHARE:
				rdp_process_share_capset(rdp, s);
				break;

			case CAPSET_TYPE_COLORCACHE:
				rdp_process_colorcache_capset(rdp, s);
				break;

			case CAPSET_TYPE_INPUT:
				rdp_process_input_capset(rdp, s);
				break;

			case CAPSET_TYPE_FONT:
				rdp_process_font_capset(rdp, s);
				break;

			case CAPSET_TYPE_BITMAPCACHE_HOSTSUPPORT:
				rdp_process_bitmapcache_hostsupport_capset(rdp, s);
				break;

			case CAPSET_TYPE_VIRTUALCHANNEL:
				rdp_process_virtualchannel_capset(rdp, s);
				break;

			case CAPSET_TYPE_DRAWGDIPLUS:
				rdp_process_draw_gdiplus_capset(rdp, s);
				break;

			case CAPSET_TYPE_RAIL:
				rdp_process_rail_capset(rdp, s);
				break;

			case CAPSET_TYPE_WINDOW:
				rdp_process_window_capset(rdp, s);
				break;

			case CAPSET_TYPE_LARGE_POINTER:
				rdp_process_large_pointer_capset(rdp, s);
				break;

			case CAPSET_TYPE_SURFACE_COMMANDS:
				rdp_process_surface_commands_capset(rdp, s);
				break;

			case CAPSET_TYPE_COMPDESK:
				rdp_process_compdesk_capset(rdp, s);
				break;

			case CAPSET_TYPE_MULTIFRAGMENTUPDATE:
				rdp_process_multifragmentupdate_capset(rdp, s);
				break;

			case CAPSET_TYPE_BITMAP_CODECS:
				rdp_process_bitmap_codecs_capset(rdp, s, lengthCapability);
				break;

			case CAPSET_TYPE_FRAME_ACKNOWLEDGE:
				rdp_process_frame_ack_capset(rdp, s);
				break;

			default:
				ui_unimpl(rdp->inst, "Unknown Capability Set 0x%02X", capabilitySetType);
				break;
		}

		s->p = next;
	}
}

/* Process demand active PDU and respond */
static void
process_demand_active(rdpRdp * rdp, STREAM s, uint16 serverChannelId)
{
	enum RDP_PDU_TYPE type;
	uint16 source;
	uint16 lengthSourceDescriptor;
	uint16 lengthCombinedCapabilities;

	rdp->rdp_serverid = serverChannelId;
	in_uint32_le(s, rdp->rdp_shareid);		/* shareId */
	in_uint16_le(s, lengthSourceDescriptor);	/* lengthSourceDescriptor */
	in_uint16_le(s, lengthCombinedCapabilities);	/* lengthCombinedCapabilities */
	in_uint8s(s, lengthSourceDescriptor);		/* sourceDescriptor, should be "RDP" */

	DEBUG("DEMAND_ACTIVE(id=0x%x)\n", rdp->rdp_shareid);
	rdp_process_server_caps(rdp, s, lengthCombinedCapabilities);
	in_uint8s(s, 4); /* sessionID, ignored by the client */

	rdp_send_confirm_active(rdp);
	rdp_send_synchronize(rdp);
	rdp_send_control(rdp, RDP_CTL_COOPERATE);
	rdp_send_control(rdp, RDP_CTL_REQUEST_CONTROL);
	s = rdp_recv(rdp, &type, &source);	/* RDP_PDU_SYNCHRONIZE */
	ASSERT(type == RDP_PDU_DATA);
	process_data_pdu(rdp, s);
	s = rdp_recv(rdp, &type, &source);	/* RDP_CTL_COOPERATE */
	ASSERT(type == RDP_PDU_DATA);
	process_data_pdu(rdp, s);
	s = rdp_recv(rdp, &type, &source);	/* RDP_CTL_GRANTED_CONTROL */
	ASSERT(type == RDP_PDU_DATA);
	process_data_pdu(rdp, s);

	/* Synchronize toggle keys */
	rdp_sync_input(rdp, time(NULL), ui_get_toggle_keys_state(rdp->inst));

	if (rdp->settings->rdp_version >= 5)
	{
		rdp_enum_bmpcache2(rdp);
		rdp_send_fonts(rdp, 3);
	}
	else
	{
		rdp_send_fonts(rdp, 1);
		rdp_send_fonts(rdp, 2);
	}

	s = rdp_recv(rdp, &type, &source);	/* RDP_DATA_PDU_FONTMAP */
	ASSERT(type == RDP_PDU_DATA);
	process_data_pdu(rdp, s);
	reset_order_state(rdp->orders);
}

/* Process a color pointer PDU */
static void
process_color_pointer_common(rdpRdp * rdp, STREAM s, int bpp)
{
	uint16 width, height, cache_idx, masklen, datalen;
	sint16 x, y;
	uint8 * mask;
	uint8 * data;
	RD_HCURSOR cursor;

	in_uint16_le(s, cache_idx);
	in_uint16_le(s, x);
	in_uint16_le(s, y);
	in_uint16_le(s, width);
	in_uint16_le(s, height);
	in_uint16_le(s, masklen);
	in_uint16_le(s, datalen);
	in_uint8p(s, data, datalen);
	in_uint8p(s, mask, masklen);
	x = MAX(x, 0);
	x = MIN(x, width - 1);
	y = MAX(y, 0);
	y = MIN(y, height - 1);
	cursor = ui_create_cursor(rdp->inst, x, y, width, height, mask, data, bpp);
	ui_set_cursor(rdp->inst, cursor);
	cache_put_cursor(rdp->cache, cache_idx, cursor);
}

/* Process a color pointer PDU */
void
process_color_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	process_color_pointer_common(rdp, s, 24);
}

/* Process a cached pointer PDU */
void
process_cached_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	uint16 cache_idx;

	in_uint16_le(s, cache_idx);
	ui_set_cursor(rdp->inst, cache_get_cursor(rdp->cache, cache_idx));
}

/* Process a system pointer PDU */
static void
process_system_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	uint16 system_pointer_type;

	in_uint16_le(s, system_pointer_type);
	switch (system_pointer_type)
	{
		case RDP_SYSPTR_NULL:
			ui_set_null_cursor(rdp->inst);
			break;

		case RDP_SYSPTR_DEFAULT:
			ui_set_default_cursor(rdp->inst);
			break;

		default:
			ui_unimpl(rdp->inst, "Unknown System Pointer message 0x%x\n", system_pointer_type);
	}
}

/* Process a new pointer PDU */
void
process_new_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	int xor_bpp;

	in_uint16_le(s, xor_bpp);
	process_color_pointer_common(rdp, s, xor_bpp);
}

/* Process a pointer PDU */
static void
process_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	uint16 message_type;
	uint16 x, y;

	in_uint16_le(s, message_type);
	in_uint8s(s, 2);	/* pad */

	switch (message_type)
	{
		case RDP_PTRMSGTYPE_POSITION:
			in_uint16_le(s, x);
			in_uint16_le(s, y);
			ui_move_pointer(rdp->inst, x, y);
			break;

		case RDP_PTRMSGTYPE_COLOR:
			process_color_pointer_pdu(rdp, s);
			break;

		case RDP_PTRMSGTYPE_CACHED:
			process_cached_pointer_pdu(rdp, s);
			break;

		case RDP_PTRMSGTYPE_SYSTEM:
			process_system_pointer_pdu(rdp, s);
			break;

		case RDP_PTRMSGTYPE_POINTER:
			process_new_pointer_pdu(rdp, s);
			break;

		default:
			ui_unimpl(rdp->inst, "Unknown Pointer message 0x%x\n", message_type);
	}
}

static void /* see [MS-RDPBCGR] 2.2.9.1.1.5.1 */
process_play_sound_pdu(rdpRdp * rdp, STREAM s)
{
	uint16 duration, frequency;

	in_uint16_le(s, duration);
	in_uint8s(s, 2); /* pad */
	in_uint16_le(s, frequency);
	in_uint8s(s, 2);

	DEBUG("(beep not implemented) duration %d frequency %d\n", duration, frequency);
}

/* Process bitmap updates */
void
process_bitmap_updates(rdpRdp * rdp, STREAM s)
{
	int i;
	size_t buffer_size;
	uint16 num_updates;
	uint16 left, top, right, bottom, width, height;
	uint16 cx, cy, bpp, Bpp, compress, bufsize, size;
	uint8 *data, *bmpdata;

	in_uint16_le(s, num_updates);

	for (i = 0; i < num_updates; i++)
	{
		in_uint16_le(s, left);
		in_uint16_le(s, top);
		in_uint16_le(s, right);
		in_uint16_le(s, bottom);
		in_uint16_le(s, width);
		in_uint16_le(s, height);
		in_uint16_le(s, bpp);
		Bpp = (bpp + 7) / 8;
		in_uint16_le(s, compress);
		in_uint16_le(s, bufsize);

		cx = right - left + 1;
		cy = bottom - top + 1;

		DEBUG_DRAW("BITMAP_UPDATE(l=%d,t=%d,r=%d,b=%d,w=%d,h=%d,Bpp=%d,cmp=%d)\n",
		       left, top, right, bottom, width, height, Bpp, compress);

		buffer_size = width * height * Bpp;

		if (buffer_size > rdp->buffer_size)
		{
			rdp->buffer = xrealloc(rdp->buffer, buffer_size);
			rdp->buffer_size = buffer_size;
		}

		if (!compress)
		{
			int y;
			bmpdata = (uint8 *) rdp->buffer;
			for (y = 0; y < height; y++)
			{
				in_uint8a(s, &bmpdata[(height - y - 1) * (width * Bpp)],
					  width * Bpp);
			}
			ui_paint_bitmap(rdp->inst, left, top, cx, cy, width, height, bmpdata);
			continue;
		}

		if (compress & 0x400)
		{
			size = bufsize;
		}
		else
		{
			in_uint8s(s, 2);	/* pad */
			in_uint16_le(s, size);
			in_uint8s(s, 4);	/* line_size, final_size */
		}
		in_uint8p(s, data, size);

		buffer_size = width * height * Bpp;

		if (buffer_size > rdp->buffer_size)
		{
			rdp->buffer = xrealloc(rdp->buffer, buffer_size);
			rdp->buffer_size = buffer_size;
		}

		bmpdata = (uint8 *) rdp->buffer;

		if (bitmap_decompress(rdp->inst, bmpdata, width, height, data, size, Bpp))
		{
			ui_paint_bitmap(rdp->inst, left, top, cx, cy, width, height, bmpdata);
		}
		else
		{
			DEBUG_DRAW("Failed to decompress data\n");
		}
	}
}

/* Process a palette update */
void
process_palette(rdpRdp * rdp, STREAM s)
{
	int i;
	size_t size;
	RD_COLORENTRY *entry;
	RD_PALETTE map;
	RD_HPALETTE hmap;

	in_uint8s(s, 2);	/* pad */
	in_uint16_le(s, map.ncolors);
	in_uint8s(s, 2);	/* pad */

	size = sizeof(RD_COLORENTRY) * map.ncolors;

	if (size > rdp->buffer_size)
	{
		rdp->buffer = xrealloc(rdp->buffer, size);
		rdp->buffer_size = size;
	}

	map.colors = (RD_COLORENTRY *) rdp->buffer;

	DEBUG_DRAW("PALETTE(c=%d)\n", map.ncolors);

	for (i = 0; i < map.ncolors; i++)
	{
		entry = &map.colors[i];
		in_uint8(s, entry->red);
		in_uint8(s, entry->green);
		in_uint8(s, entry->blue);
	}

	hmap = ui_create_colormap(rdp->inst, &map);
	ui_set_colormap(rdp->inst, hmap);
}

/* Process an update PDU */
static void
process_update_pdu(rdpRdp * rdp, STREAM s)
{
	uint16 update_type, count;

	in_uint16_le(s, update_type);

	ui_begin_update(rdp->inst);
	switch (update_type)
	{
		case RDP_UPDATE_ORDERS:
			in_uint8s(s, 2);	/* pad */
			in_uint16_le(s, count);
			in_uint8s(s, 2);	/* pad */
			process_orders(rdp->orders, s, count);
			break;

		case RDP_UPDATE_BITMAP:
			process_bitmap_updates(rdp, s);
			break;

		case RDP_UPDATE_PALETTE:
			process_palette(rdp, s);
			break;

		case RDP_UPDATE_SYNCHRONIZE:
			in_uint8s(s, 2);	/* pad */
			break;

		default:
			ui_unimpl(rdp->inst, "Unknown update pdu type 0x%x\n", update_type);
	}
	ui_end_update(rdp->inst);
}

/* Process a Set Error Information PDU */
static void
process_set_error_info_pdu(STREAM s, struct rdp_inst *inst)
{
	in_uint32_le(s, inst->disc_reason); /* Note: reason 0 is _not_ an error */
	printf("Received Set Error Information PDU with reason %x\n", inst->disc_reason);
}

/* Process Data PDU */
static RD_BOOL
process_data_pdu(rdpRdp * rdp, STREAM s)
{
	uint32 uncompressedLength;
	uint8 pduType2;
	uint8 compressedType;
	uint16 compressedLength;
	uint32 roff, rlen;
	STREAM data_s;
	uint8 * data_s_end;

	/* rest of Share Data Header */
	in_uint8s(s, 6);	/* shareid, pad, streamid */
	in_uint16_le(s, uncompressedLength);
	in_uint8(s, pduType2);
	in_uint8(s, compressedType);
	in_uint16_le(s, compressedLength);

	if (compressedType & RDP_MPPC_COMPRESSED)
	{
		data_s = &(rdp->mppc_dict.ns);
		compressedLength -= 18;
		if (uncompressedLength > RDP_MPPC_DICT_SIZE)
			ui_error(rdp->inst, "error decompressed packet size exceeds max\n");
		if (mppc_expand(rdp, s->p, compressedLength, compressedType, &roff, &rlen) == -1)
			ui_error(rdp->inst, "error while decompressing packet\n");
		/* allocate memory and copy the uncompressed data into the temporary stream */
		data_s->data = (uint8 *) xrealloc(data_s->data, rlen);
		memcpy(data_s->data, rdp->mppc_dict.hist + roff, rlen);
		ASSERT(rlen == uncompressedLength);
		data_s->size = rlen;
		data_s->end = data_s->data + data_s->size;
		data_s->p = data_s->data;
		data_s->rdp_hdr = data_s->p;
		data_s_end = data_s->p + rlen;
	}
	else
	{
		data_s = s;
		data_s_end = rdp->next_packet;
	}

	switch (pduType2)
	{
		case RDP_DATA_PDU_UPDATE:
			process_update_pdu(rdp, data_s);
			ASSERT(data_s->p == data_s_end);
			break;

		case RDP_DATA_PDU_CONTROL:
			DEBUG("Received Control PDU\n");
			break;

		case RDP_DATA_PDU_SYNCHRONIZE:
			DEBUG("Received Sync PDU\n");
			break;

		case RDP_DATA_PDU_POINTER:
			process_pointer_pdu(rdp, data_s);
			ASSERT(data_s->p == data_s_end);
			break;

		case RDP_DATA_PDU_PLAY_SOUND:
			process_play_sound_pdu(rdp, data_s);
			ASSERT(data_s->p == data_s_end);
			break;

		case RDP_DATA_PDU_SAVE_SESSION_INFO:
			DEBUG("Received Logon PDU\n");
			/* User logged on */
			break;

		case RDP_DATA_PDU_FONTMAP:
			DEBUG("Received Font Map PDU\n");
			break;

		case RDP_DATA_PDU_SET_ERROR_INFO:
			/* A FYI message - don't give up yet */
			process_set_error_info_pdu(data_s, rdp->inst);
			ASSERT(data_s->p == data_s_end);
			break;

		default:
			ui_unimpl(rdp->inst, "Unknown data PDU type 0x%x\n", pduType2);
	}
	return False;
}

/* Read 32 bit length field followed by binary data, returns xmalloc'ed memory and length */
static char*
xmalloc_in_len32_data(rdpRdp * rdp, STREAM s, uint32 *plen)
{
	unsigned char *sp, *p;
	in_uint32_le(s, *plen);
	in_uint8p(s, sp, *plen);
	p = xmalloc(*plen);
	memcpy(p, sp, *plen);
	return (char*) p;
}

/* Read 32 bit length field followed by WINDOWS_CODEPAGE string - return like xstrdup, 0-terminated */
static char*
xstrdup_in_len32_unistr(rdpRdp * rdp, STREAM s)
{
	uint32 len;
	unsigned char *p;
	in_uint32_le(s, len);
	in_uint8p(s, p, len);
	return freerdp_uniconv_in(rdp->uniconv, p, len);
}

/* Process Server Redirection Packet */
static void
process_redirect_pdu(rdpRdp * rdp, STREAM s)
{
	uint16 length;
	unsigned char * s_start = s->p;
	uint32 redirFlags;

	in_uint8s(s, 2);				/* flags, MUST be set to SEC_REDIRECTION_PKT = 0x0400 */
	in_uint16_le(s, length);			/* length */
	in_uint32_le(s, rdp->redirect_session_id);	/* sessionID, must be used in Client Cluster Data as RedirectedSessionID */
	in_uint32_le(s, redirFlags);			/* redirFlags */
	printf("redirect flags: %x\n", redirFlags);

	if (redirFlags & LB_TARGET_NET_ADDRESS)
	{
		rdp->redirect_server = xstrdup_in_len32_unistr(rdp, s);
		printf("redirect_server: %s\n", rdp->redirect_server);
	}
	if (redirFlags & LB_LOAD_BALANCE_INFO)
	{
		rdp->redirect_routingtoken = xmalloc_in_len32_data(rdp, s,
			&rdp->redirect_routingtoken_len);
		printf("redirect_cookie_len: %d\n", rdp->redirect_routingtoken_len);
		hexdump((void*)rdp->redirect_routingtoken, rdp->redirect_routingtoken_len);
	}
	if (redirFlags & LB_USERNAME)
	{
		rdp->redirect_username = xstrdup_in_len32_unistr(rdp, s);
		printf("redirect_username: %s\n", rdp->redirect_username);
	}
	if (redirFlags & LB_DOMAIN)
	{
		rdp->redirect_domain = xstrdup_in_len32_unistr(rdp, s);
		printf("redirect_domain: %s\n", rdp->redirect_domain);
	}
	if (redirFlags & LB_PASSWORD)
	{
		/* LB_DONTSTOREUSERNAME apparently means that we shouldn't store the password we got. It makes no difference for us. */
		rdp->redirect_password = xmalloc_in_len32_data(rdp, s,
			&rdp->redirect_password_len);
		printf("redirect_password_len: %d\n", rdp->redirect_password_len);
	}
	if (redirFlags & LB_TARGET_FQDN)
	{
		rdp->redirect_target_fqdn = xstrdup_in_len32_unistr(rdp, s);
		printf("redirect_target_fqdn: %s\n", rdp->redirect_target_fqdn);
	}
	if (redirFlags & LB_TARGET_NETBIOS_NAME)
	{
		rdp->redirect_target_netbios_name = xstrdup_in_len32_unistr(rdp, s);
		printf("redirect_target_netbios_name: %s\n", rdp->redirect_target_netbios_name);
	}
	if (redirFlags & LB_TARGET_NET_ADDRESSES)
	{
		rdp->redirect_target_net_addresses = xmalloc_in_len32_data(rdp, s,
			&rdp->redirect_target_net_addresses_len);
		printf("redirect_target_net_addresses_len: %d\n", rdp->redirect_target_net_addresses_len);
		hexdump((void*)rdp->redirect_target_net_addresses, rdp->redirect_target_net_addresses_len);
	}
	if (redirFlags & LB_NOREDIRECT)
	{
		printf("no redirect\n");
	}
	else
	{
		printf("Redirecting to %s as %s@%s\n", rdp->redirect_server, rdp->redirect_username, rdp->redirect_domain);
		rdp->redirect = True;
	}
	/* TODO: LB_SMARTCARD_LOGON */

	/* Skip optional padding up to length */
	ASSERT(s->p <= s_start + length);
	in_uint8s(s, length - (s->p - s_start));
}

/* process fast path */
static void
process_fp(rdpRdp * rdp, STREAM s)
{
	int numberOrders;
	int x;
	int y;
	int type;
	int ctype;
	int frag_bits;
	int comp_bits;
	int length;
	int skip_switch;
	uint8 *next;
	uint32 roff, rlen;
	STREAM ns = &(rdp->mppc_dict.ns);
	STREAM ts;
	STREAM fd_s;

	ui_begin_update(rdp->inst);
	while (s->p < s->end)
	{
		skip_switch = 0;
		in_uint8(s, type);
		frag_bits = (type & 0x30) >> 4;
		comp_bits = (type & 0xc0) >> 6;
		type = type & 0xf;
		if (comp_bits & FASTPATH_OUTPUT_COMPRESSION_USED)
		{
			in_uint8(s, ctype);
			in_uint16_le(s, length);
		}
		else
		{
			ctype = 0;
			in_uint16_le(s, length);
		}
		rdp->next_packet = next = s->p + length;
		if (ctype & RDP_MPPC_COMPRESSED)
		{
			if (mppc_expand(rdp, s->p, length, ctype, &roff, &rlen) == -1)
				ui_error(rdp->inst, "error while decompressing packet");
			ns->data = (uint8 *) xrealloc(ns->data, rlen);
			memcpy(ns->data, rdp->mppc_dict.hist + roff, rlen);
			ns->size = rlen;
			ns->end = ns->data + ns->size;
			ns->p = ns->data;
			ns->rdp_hdr = ns->p;
			length = rlen;
			ts = ns;
		}
		else
		{
			ts = s;
		}
		if ((frag_bits != 0) && (type == 4) &&
			(rdp->settings->ui_decode_flags & 2))
		{
			ui_decode(rdp->inst, ts->p, (int) (ts->end - ts->p));
			skip_switch = 1;
		}
		else if (frag_bits != 0)
		{
			fd_s = rdp->fragment_data;
			switch (frag_bits)
			{
				case FASTPATH_FRAGMENT_LAST:
					out_uint8a(fd_s, ts->p, length);
					fd_s->end = fd_s->p;
					fd_s->p = fd_s->data;
					ts = fd_s;
					break;
				case FASTPATH_FRAGMENT_FIRST:
					skip_switch = 1;
					fd_s->p = fd_s->data;
					out_uint8a(fd_s, ts->p, length);
					break;
				case FASTPATH_FRAGMENT_NEXT:
					skip_switch = 1;
					out_uint8a(fd_s, ts->p, length);
					break;
			}
		}
		if (skip_switch)
		{
			s->p = next;
			continue;
		}
		switch (type)
		{
			case FASTPATH_UPDATETYPE_ORDERS:
				in_uint16_le(ts, numberOrders);
				process_orders(rdp->orders, ts, numberOrders);
				break;
			case FASTPATH_UPDATETYPE_BITMAP:
				in_uint8s(ts, 2);
				process_bitmap_updates(rdp, ts);
				break;
			case FASTPATH_UPDATETYPE_PALETTE:
				in_uint8s(ts, 2);
				process_palette(rdp, ts);
				break;
			case FASTPATH_UPDATETYPE_SYNCHRONIZE:
				break;
			case FASTPATH_UPDATETYPE_SURFCMDS:
				if (rdp->settings->ui_decode_flags & 3)
				{
					length = (int) (ts->end - ts->p);
					if (ui_decode(rdp->inst, ts->p, length) == 0)
					{
						break;
					}
				}
				surface_cmd(rdp, ts);
				break;
			case FASTPATH_UPDATETYPE_PTR_NULL:
				ui_set_null_cursor(rdp->inst);
				break;
			case FASTPATH_UPDATETYPE_PTR_DEFAULT:
				break;
			case FASTPATH_UPDATETYPE_PTR_POSITION:
				in_uint16_le(ts, x);
				in_uint16_le(ts, y);
				ui_move_pointer(rdp->inst, x, y);
				break;
			case FASTPATH_UPDATETYPE_COLOR:
				process_color_pointer_pdu(rdp, ts);
				break;
			case FASTPATH_UPDATETYPE_CACHED:
				process_cached_pointer_pdu(rdp, ts);
				break;
			case FASTPATH_UPDATETYPE_POINTER:
				process_new_pointer_pdu(rdp, ts);
				break;
			default:
				ui_unimpl(rdp->inst, "fastpath opcode %d\n", type);
				break;
		}
		s->p = next;
	}
	ui_end_update(rdp->inst);
}

/* used in uiports and rdp_main_loop, processes the rdp packets waiting */
RD_BOOL
rdp_loop(rdpRdp * rdp, RD_BOOL * deactivated)
{
	enum RDP_PDU_TYPE type;
	uint16 source;
	RD_BOOL disc = False;	/* True when a disconnect PDU was received */
	RD_BOOL cont = True;
	STREAM s;

	while (cont)
	{
		s = rdp_recv(rdp, &type, &source);

		if (s == NULL)
			return False;

		switch (type)
		{
			case RDP_PDU_DEMAND_ACTIVE:
				process_demand_active(rdp, s, source);
				*deactivated = False;
				break;
			case RDP_PDU_DEACTIVATE_ALL:
				DEBUG("RDP_PDU_DEACTIVATE_ALL\n");
				*deactivated = True;
				s->p = rdp->next_packet;	/* FIXME: This is cheating */
				break;
			case RDP_PDU_DATA:
				disc = process_data_pdu(rdp, s);
				break;
			case RDP_PDU_NULL:
				/* DEBUG("PDU already processed\n"); */
				break;
			default:
				ui_unimpl(rdp->inst, "Unknown PDU type 0x%x", type);
		}
		if (disc)
			return False;
		cont = rdp->next_packet < s->end;
	}
	return True;
}

/* Establish a connection up to the RDP layer */
RD_BOOL
rdp_connect(rdpRdp * rdp)
{
	char* password_encoded;
	size_t password_encoded_len = 0;
	uint32 connect_flags = INFO_NORMALLOGON;

	if (rdp->settings->bulk_compression)
	{
		connect_flags |= INFO_COMPRESSION | PACKET_COMPR_TYPE_64K;
	}
	if (rdp->settings->autologin)
	{
		connect_flags |= INFO_AUTOLOGON;
	}
	if (rdp->settings->console_audio)
	{
		connect_flags |= INFO_REMOTECONSOLEAUDIO;
	}

	if (!sec_connect(rdp->sec, rdp->settings->server, rdp->settings->username, rdp->settings->tcp_port_rdp))
		return False;

	password_encoded = freerdp_uniconv_out(rdp->uniconv, rdp->settings->password, &password_encoded_len);
	rdp_send_client_info(rdp, connect_flags, rdp->settings->domain, rdp->settings->username, password_encoded, password_encoded_len, rdp->settings->shell, rdp->settings->directory);
	xfree(password_encoded);

	/* by setting encryption to False here, we have an encrypted login packet but unencrypted transfer of other packets */
	if (rdp->sec->tls_connected)
		rdp->settings->encryption = 0;

	return True;
}

/* Establish a reconnection up to the RDP layer */
RD_BOOL
rdp_reconnect(rdpRdp * rdp)
{
	char *server;
	char *username;
	char *domain;
	char *password;
	size_t password_len;

	server = rdp->redirect_server ? rdp->redirect_server : rdp->settings->server;
	username = rdp->redirect_username ? rdp->redirect_username : rdp->settings->username;

	sec_disconnect(rdp->sec);
	sec_free(rdp->sec);
	rdp->sec = sec_new(rdp);
	if (!sec_connect(rdp->sec, server, username, rdp->settings->tcp_port_rdp))
		return False;

	domain = rdp->redirect_domain ? rdp->redirect_domain : rdp->settings->domain;

	if (rdp->redirect_password)
	{
		password = rdp->redirect_password;
		password_len = rdp->redirect_password_len;
	}
	else
		password = freerdp_uniconv_out(rdp->uniconv, rdp->settings->password, &password_len);
	rdp_send_client_info(rdp, INFO_NORMALLOGON | INFO_AUTOLOGON,
			domain, username, password, password_len,
			rdp->settings->shell, rdp->settings->directory);
	if (!rdp->redirect_password)
		xfree(password);
	return True;
}

/* Disconnect from the RDP layer */
void
rdp_disconnect(rdpRdp * rdp)
{
	sec_disconnect(rdp->sec);
}

rdpRdp *
rdp_new(struct rdp_set *settings, struct rdp_inst *inst)
{
	rdpRdp *self;

	self = (rdpRdp *) xmalloc(sizeof(rdpRdp));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpRdp));
		self->settings = settings;
		self->inst = inst;
		self->current_status = 1;
		self->uniconv = freerdp_uniconv_new();
		self->buffer_size = 2048;
		self->buffer = xmalloc(self->buffer_size);
		memset(self->buffer, 0, self->buffer_size);
		self->sec = sec_new(self);
		self->orders = orders_new(self);
		self->pcache = pcache_new(self);
		self->cache = cache_new(self);
		self->ext = ext_new(self);
	}
	return self;
}

void
rdp_free(rdpRdp * rdp)
{
	int index;

	if (rdp != NULL)
	{
		freerdp_uniconv_free(rdp->uniconv);
		cache_free(rdp->cache);
		pcache_free(rdp->pcache);
		orders_free(rdp->orders);
		xfree(rdp->buffer);
		sec_free(rdp->sec);
		ext_free(rdp->ext);
		xfree(rdp->redirect_server);
		xfree(rdp->redirect_routingtoken);
		xfree(rdp->redirect_username);
		xfree(rdp->redirect_domain);
		xfree(rdp->redirect_password);
		xfree(rdp->redirect_target_fqdn);
		xfree(rdp->redirect_target_netbios_name);
		xfree(rdp->redirect_target_net_addresses);
		for (index = 0; index < MAX_BITMAP_CODECS; index++)
		{
			stream_delete(rdp->out_codec_caps[index]);
		}
		stream_delete(rdp->fragment_data);
		xfree(rdp);
	}
}
