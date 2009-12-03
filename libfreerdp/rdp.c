/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RDP layer
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

#include <time.h>
#ifndef _WIN32
#include <errno.h>
#include <unistd.h>
#endif
#include "rdesktop.h"
#include "iso.h"
#include "tcp.h"
#include "mcs.h"
#include "secure.h"
#include "rdp.h"
#include "rail.h"
#include "capabilities.h"
#include "rdpset.h"
#include "orders.h"
#include "pstcache.h"
#include "cache.h"
#include "bitmap.h"
#include "mem.h"

#ifdef HAVE_ICONV
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#ifndef ICONV_CONST
#define ICONV_CONST ""
#endif
#endif

#define RDP5_FLAG 0x0030


/* Receive an RDP packet */
static STREAM
rdp_recv(rdpRdp * rdp, uint8 * type)
{
	uint16 length, pdu_type;
	uint8 rdpver;

	if ((rdp->rdp_s == NULL) || (rdp->next_packet >= rdp->rdp_s->end) ||
	    (rdp->next_packet == NULL))
	{
		rdp->rdp_s = sec_recv(rdp->sec, &rdpver);
		if (rdp->rdp_s == NULL)
			return NULL;
		if (rdpver == 0xff)
		{
			rdp->next_packet = rdp->rdp_s->end;
			*type = 0;
			return rdp->rdp_s;
		}
		else if (rdpver != 3)
		{
			/* rdp5_process should move rdp->next_packet ok */
			rdp5_process(rdp, rdp->rdp_s);
			*type = 0;
			return rdp->rdp_s;
		}

		rdp->next_packet = rdp->rdp_s->p;
	}
	else
	{
		rdp->rdp_s->p = rdp->next_packet;
	}

	in_uint16_le(rdp->rdp_s, length);
	/* 32k packets are really 8, keepalive fix */
	if (length == 0x8000)
	{
		rdp->next_packet += 8;
		*type = 0;
		return rdp->rdp_s;
	}
	in_uint16_le(rdp->rdp_s, pdu_type);
	in_uint8s(rdp->rdp_s, 2);	/* userid */
	*type = pdu_type & 0xf;

#if WITH_DEBUG
	DEBUG("RDP packet #%d, (type %x)\n", ++(rdp->packetno), *type);
	hexdump(rdp->next_packet, length);
#endif /*  */

	rdp->next_packet += length;
	return rdp->rdp_s;
}

/* Initialise an RDP data packet */
static STREAM
rdp_init_data(rdpRdp * rdp, int maxlen)
{
	STREAM s;

	s = sec_init(rdp->sec, rdp->settings->encryption ? SEC_ENCRYPT : 0, maxlen + 18);
	s_push_layer(s, rdp_hdr, 18);

	return s;
}

/* Send an RDP data packet */
static void
rdp_send_data(rdpRdp * rdp, STREAM s, uint8 data_pdu_type)
{
	uint16 length;

	s_pop_layer(s, rdp_hdr);
	length = s->end - s->p;

	out_uint16_le(s, length);
	out_uint16_le(s, (RDP_PDU_DATA | 0x10));
	out_uint16_le(s, (rdp->sec->mcs->mcs_userid + 1001));

	out_uint32_le(s, rdp->rdp_shareid);
	out_uint8(s, 0);	/* pad */
	out_uint8(s, 1);	/* streamid */
	out_uint16_le(s, (length - 14));
	out_uint8(s, data_pdu_type);
	out_uint8(s, 0);	/* compress_type */
	out_uint16(s, 0);	/* compress_len */

	sec_send(rdp->sec, s, rdp->settings->encryption ? SEC_ENCRYPT : 0);
}

/* Output a string in Unicode */
void
rdp_out_unistr(rdpRdp * rdp, STREAM s, char *string, int len)
{
#ifdef HAVE_ICONV
	size_t ibl = strlen(string), obl = len + 2;
	char *pin = string, *pout = (char *) s->p;
	iconv_t iconv_h = (iconv_t) (rdp->out_iconv_h);

	memset(pout, 0, len + 4);

	if (rdp->iconv_works)
	{
		if (iconv_h == (iconv_t) - 1)
		{
			size_t i = 1, o = 4;
			if ((iconv_h = iconv_open(WINDOWS_CODEPAGE, rdp->settings->codepage)) ==
			    (iconv_t) - 1)
			{
				ui_warning(rdp->inst, "rdp_out_unistr: iconv_open[%s -> %s] fail %p\n",
					   rdp->settings->codepage, WINDOWS_CODEPAGE, iconv_h);

				rdp->iconv_works = False;
				rdp_out_unistr(rdp, s, string, len);
				return;
			}
			if (iconv(iconv_h, (ICONV_CONST char **) &pin, &i, &pout, &o) ==
			    (size_t) - 1)
			{
				iconv_close(iconv_h);
				rdp->out_iconv_h = (void *) (-1);
				ui_warning(rdp->inst, "rdp_out_unistr: iconv(1) fail, errno %d\n",
					   errno);

				rdp->iconv_works = False;
				rdp_out_unistr(rdp, s, string, len);
				return;
			}
			pin = string;
			pout = (char *) s->p;
		}

		if (iconv(iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
		{
			iconv_close(iconv_h);
			rdp->out_iconv_h = (void *) (-1);
			ui_warning(rdp->inst, "rdp_out_unistr: iconv(2) fail, errno %d\n", errno);

			rdp->iconv_works = False;
			rdp_out_unistr(rdp, s, string, len);
			return;
		}

		s->p += len + 2;

	}
	else
#endif
	{
		int i = 0, j = 0;

		len += 2;

		while (i < len)
		{
			s->p[i++] = string[j++];
			s->p[i++] = 0;
		}

		s->p += len;
	}
}

/* Input a string in Unicode
 *
 * Returns str_len of string
 */
int
rdp_in_unistr(rdpRdp * rdp, STREAM s, char *string, int str_size, int in_len)
{
#ifdef HAVE_ICONV
	size_t ibl = in_len, obl = str_size - 1;
	char *pin = (char *) s->p, *pout = string;
	iconv_t iconv_h = (iconv_t) (rdp->in_iconv_h);

	if (rdp->iconv_works)
	{
		if (iconv_h == (iconv_t) - 1)
		{
			if ((iconv_h = iconv_open(rdp->settings->codepage, WINDOWS_CODEPAGE)) ==
			    (iconv_t) - 1)
			{
				ui_warning(rdp->inst, "rdp_in_unistr: iconv_open[%s -> %s] fail %p\n",
					   WINDOWS_CODEPAGE, rdp->settings->codepage, iconv_h);

				rdp->iconv_works = False;
				return rdp_in_unistr(rdp, s, string, str_size, in_len);
			}
		}

		if (iconv(iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
		{
			if (errno == E2BIG)
			{
				ui_warning(rdp->inst, "server sent an unexpectedly long string, truncating\n");
			}
			else
			{
				iconv_close(iconv_h);
				rdp->in_iconv_h = (void *) (-1);
				ui_warning(rdp->inst, "rdp_in_unistr: iconv fail, errno %d\n", errno);

				rdp->iconv_works = False;
				return rdp_in_unistr(rdp, s, string, str_size, in_len);
			}
		}

		/* we must update the location of the current STREAM for future reads of s->p */
		s->p += in_len;

		*pout = 0;
		return pout - string;
	}
	else
#endif
	{
		int i = 0;
		int len = in_len / 2;
		int rem = 0;

		if (len > str_size - 1)
		{
			ui_warning(rdp->inst, "server sent an unexpectedly long string, truncating\n");
			len = str_size - 1;
			rem = in_len - 2 * len;
		}

		while (i < len)
		{
			in_uint8a(s, &string[i++], 1);
			in_uint8s(s, 1);
		}

		in_uint8s(s, rem);
		string[len] = 0;
		return len;
	}
}

/* Output system time structure */
void
rdp_out_systemtime(STREAM s, systemTime sysTime)
{
	out_uint16_le(s, sysTime.wYear); // wYear, must be set to zero
	out_uint16_le(s, sysTime.wMonth); // wMonth
	out_uint16_le(s, sysTime.wDayOfWeek); // wDayOfWeek
	out_uint16_le(s, sysTime.wDay); // wDay
	out_uint16_le(s, sysTime.wHour); // wHour
	out_uint16_le(s, sysTime.wMinute); // wMinute
	out_uint16_le(s, sysTime.wSecond); // wSecond
	out_uint16_le(s, sysTime.wMilliseconds); // wMilliseconds
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
		strftime(daylightName, 32, "%Z, Summer Time", localTime);

		memset((void*)&standardDate, '\0', sizeof(systemTime));
		memset((void*)&daylightDate, '\0', sizeof(systemTime));

		out_uint32_le(s, bias); // bias

		rdp_out_unistr(rdp, s, standardName, 2 * strlen(standardName));
		out_uint8s(s, 62 - 2 * strlen(standardName)); // standardName (64 bytes)

		rdp_out_systemtime(s, standardDate); // standardDate
		out_uint32_le(s, standardBias); // standardBias

		rdp_out_unistr(rdp, s, daylightName, 2 * strlen(daylightName));
		out_uint8s(s, 62 - 2 * strlen(daylightName)); // daylightName (64 bytes)

		rdp_out_systemtime(s, daylightDate); // daylightDate
		out_uint32_le(s, daylightBias); // daylightBias
}

/* Parse a logon info packet */
static void
rdp_send_logon_info(rdpRdp * rdp, uint32 flags, char *domain, char *user,
		    char *password, char *program, char *directory)
{
	char *ipaddr = tcp_get_address(rdp->sec->mcs->iso->tcp);
	int len_domain = 2 * strlen(domain);
	int len_user = 2 * strlen(user);
	int len_password = 2 * strlen(password);
	int len_program = 2 * strlen(program);
	int len_directory = 2 * strlen(directory);
	int len_ip = 2 * strlen(ipaddr);
	int len_dll = 2 * strlen("C:\\Windows\\System32\\mstscax.dll");
	int packetlen = 0;
	uint32 sec_flags =
		rdp->settings->encryption ? (SEC_LOGON_INFO | SEC_ENCRYPT) : SEC_LOGON_INFO;
	STREAM s;

	if ((rdp->settings->rdp_version < 5))
	{
		DEBUG_RDP5("Sending RDP4-style Logon packet\n");

		s = sec_init(rdp->sec, sec_flags, 18 + len_domain + len_user + len_password
			     + len_program + len_directory + 10);

		out_uint32(s, 0);
		out_uint32_le(s, flags);
		out_uint16_le(s, len_domain);
		out_uint16_le(s, len_user);
		out_uint16_le(s, len_password);
		out_uint16_le(s, len_program);
		out_uint16_le(s, len_directory);
		rdp_out_unistr(rdp, s, domain, len_domain);
		rdp_out_unistr(rdp, s, user, len_user);
		rdp_out_unistr(rdp, s, password, len_password);
		rdp_out_unistr(rdp, s, program, len_program);
		rdp_out_unistr(rdp, s, directory, len_directory);
	}
	else
	{

		flags |= RDP_LOGON_BLOB;
		DEBUG_RDP5("Sending RDP5-style Logon packet\n");
		packetlen = 4 +	/* Codepage */
			4 +	/* flags */
			2 +	/* length of Domain field */
			2 +	/* length of UserName field */
			(flags & RDP_LOGON_AUTO ? 2 : 0) +	/* length of Password field */
			(flags & RDP_LOGON_BLOB ? 2 : 0) +	/* Length of BLOB */
			2 +	/* len_program */
			2 +	/* len_directory */
			(0 < len_domain ? len_domain : 2) +	/* domain */
			len_user + (flags & RDP_LOGON_AUTO ? len_password : 0) + 0 +	/* We have no 512 byte BLOB. Perhaps we must? */
			(flags & RDP_LOGON_BLOB && !(flags & RDP_LOGON_AUTO) ? 2 : 0) +	/* After the BLOB is a unknown int16. If there is a BLOB, that is. */
			(0 < len_program ? len_program : 2) + (0 < len_directory ? len_directory : 2) + 2 +	/* Unknown (2) */
			2 +	/* Client ip length */
			len_ip +	/* Client ip */
			2 +	/* DLL string length */
			len_dll +	/* DLL string */
			172 + // clientTimeZone
			10; // performanceFlags and othersftrg

		s = sec_init(rdp->sec, sec_flags, packetlen);
		DEBUG_RDP5("Called sec_init with packetlen %d\n", packetlen);

		out_uint32(s, 0);	// Codepage, see http://go.microsoft.com/fwlink/?LinkId=89981

		if(rdp->settings->autologin)
			flags |= INFO_AUTOLOGON;

		if(rdp->settings->bulk_compression)
		{
			flags |= INFO_COMPRESSION;
			flags |= PACKET_COMPR_TYPE_64K;
		}

		out_uint32_le(s, flags);	// See constants.h for Info Packet Flags

		out_uint16_le(s, len_domain);	// cbDomain, length of Domain field
		out_uint16_le(s, len_user);	// cbUserName, length of UserName field
		if (flags & RDP_LOGON_AUTO)
		{
			out_uint16_le(s, len_password);	// cbPassword, length of Password field

		}
		if (flags & RDP_LOGON_BLOB && !(flags & RDP_LOGON_AUTO))
		{
			out_uint16_le(s, 0);
		}
		out_uint16_le(s, len_program);	// cbAlternateShell, length of AlternateShell field
		out_uint16_le(s, len_directory);	// cbWorkingDir, length of WorkingDir field
		if (0 < len_domain)
			// Maximum Domain length of 52 bytes in RDP 4.0 and 5.0, and 512 bytes in RDP 5.1 and later
			rdp_out_unistr(rdp, s, domain, len_domain);	// Domain (length specified by cbDomain)
		else
			out_uint16_le(s, 0);
		// Maximum UserName length of 44 bytes in RDP 4.0 and 5.0, and 512 bytes in RDP 5.1 and later
		rdp_out_unistr(rdp, s, user, len_user);	// UserName (length specified by cbUserName)
		if (flags & RDP_LOGON_AUTO)
		{
			// Maximum Password length of 32 bytes in RDP 4.0 and 5.0, and 512 bytes in RDP 5.1 and later
			rdp_out_unistr(rdp, s, password, len_password);	// Password (length specified by cbPassword)
		}
		if (flags & RDP_LOGON_BLOB && !(flags & RDP_LOGON_AUTO))
		{
			out_uint16_le(s, 0);
		}
		if (0 < len_program)
		{
			// Maximum AlternateShell length of 512 bytes
			rdp_out_unistr(rdp, s, program, len_program);	// AlternateShell (length specified by cbAlternateShell)
		}
		else
		{
			out_uint16_le(s, 0);
		}
		if (0 < len_directory)
		{
			// Maximum WorkingDir length of 512 bytes
			rdp_out_unistr(rdp, s, directory, len_directory);	// WorkingDir (length specified by cbWorkingDir)
		}
		else
		{
			out_uint16_le(s, 0);
		}

		/* Extended Info (Optional) RDP 5.0 and later */
		out_uint16_le(s, 0x0002);	// clientAddressFamily (AF_INET 0x0002)
		out_uint16_le(s, len_ip + 2);	// cbClientAddress (IP length)
		// clientAddress maximum length of 64 bytes prior to RDP 6.1, and 80 bytes for RDP 6.1 and later
		rdp_out_unistr(rdp, s, ipaddr, len_ip);	// clientAddress (Textual IP representation)
		out_uint16_le(s, len_dll + 2);	// cbClientDir
		rdp_out_unistr(rdp, s, "C:\\Windows\\System32\\mstscax.dll", len_dll); // clientDir

		rdp_out_client_timezone_info(rdp, s); // clientTimeZone

		out_uint32_le(s, 0); // clientSessionId, should be set to zero
		out_uint32_le(s, rdp->settings->rdp5_performanceflags); // performanceFlags
		out_uint16(s, 0); // cbAutoReconnectLen
		// autoReconnectCookie (length specified by cbAutoReconnectLen)
		
	}
	s_mark_end(s);
	sec_send(rdp->sec, s, sec_flags);
}

/* Send a control PDU */
static void
rdp_send_control(rdpRdp * rdp, uint16 action)
{
	STREAM s;

	s = rdp_init_data(rdp, 8);

	out_uint16_le(s, action); // action
	out_uint16(s, 0); // grantID
	out_uint32(s, 0); // controlID

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_CONTROL);
}

/* Send a synchronisation PDU */
static void
rdp_send_synchronize(rdpRdp * rdp)
{
	STREAM s;

	s = rdp_init_data(rdp, 4);

	out_uint16_le(s, 1); // messageType
	out_uint16_le(s, 1002); // targetUser, the MCS channel ID of the target user

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_SYNCHRONIZE);
}

/* Send a single input event */
void
rdp_send_input(rdpRdp * rdp, uint32 time, uint16 message_type, uint16 device_flags, uint16 param1,
	       uint16 param2)
{
	STREAM s;

	s = rdp_init_data(rdp, 16);

	out_uint16_le(s, 1);	/* number of events */
	out_uint16(s, 0);	/* pad */

	out_uint32_le(s, time);
	out_uint16_le(s, message_type);
	out_uint16_le(s, device_flags);
	out_uint16_le(s, param1);
	out_uint16_le(s, param2);

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_INPUT);
}

/* Send a single keyboard synchronize event */
void
rdp_sync_input(rdpRdp * rdp, uint32 time, uint32 toggle_keys_state)
{
	STREAM s;

	s = rdp_init_data(rdp, 16);

	out_uint16_le(s, 1);	/* number of events */
	out_uint16(s, 0);	/* pad */

	out_uint32_le(s, time);	// eventTime
	out_uint16_le(s, RDP_INPUT_SYNC);	// messageType
	out_uint16_le(s, 0);	// pad
	out_uint32_le(s, toggle_keys_state);	// toggleFlags

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_INPUT);
}

/* Send a single unicode character input event */
void
rdp_unicode_input(rdpRdp * rdp, uint32 time, uint16 unicode_character)
{
	STREAM s;

	s = rdp_init_data(rdp, 16);

	out_uint16_le(s, 1);	/* number of events */
	out_uint16(s, 0);	/* pad */

	out_uint32_le(s, time);	// eventTime
	out_uint16_le(s, RDP_INPUT_UNICODE);	// messageType
	out_uint16_le(s, 0);	// pad
	out_uint32_le(s, unicode_character);	// Unicode character
	out_uint16_le(s, 0);	// pad

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_INPUT);
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
	rdp_send_data(rdp, s, RDP_DATA_PDU_CLIENT_WINDOW_STATUS);
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
			flags |= PDU_FLAG_FIRST;
		if (num_keys - offset <= 169)
			flags |= PDU_FLAG_LAST;

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

	out_uint16(s, 0); // numberFonts, should be set to zero
	out_uint16_le(s, 0); // totalNumFonts, should be set to zero
	out_uint16_le(s, seq); // listFlags
	out_uint16_le(s, 0x0032); // entrySize, should be set to 0x0032

	s_mark_end(s);
	rdp_send_data(rdp, s, RDP_DATA_PDU_FONT2);
}

/* Send a confirm active PDU */
static void
rdp_send_confirm_active(rdpRdp * rdp)
{
	STREAM caps;
	STREAM s;
	uint32 sec_flags = rdp->settings->encryption ? (RDP5_FLAG | SEC_ENCRYPT) : RDP5_FLAG;
	int caplen;
	uint16 numberCapabilities = 14;

	caps = (STREAM) xmalloc(sizeof(struct stream));
	memset(caps, 0, sizeof(struct stream));
	caps->size = 8192;
	caps->data = (uint8 *) xmalloc(caps->size);
	caps->p = caps->data;
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
	s_mark_end(caps);
	caplen = (int) (caps->end - caps->data);

	s = sec_init(rdp->sec, sec_flags, 6 + 14 + caplen + 4 + sizeof(RDP_SOURCE));

	out_uint16_le(s, 2 + 14 + caplen + sizeof(RDP_SOURCE));
	out_uint16_le(s, (RDP_PDU_CONFIRM_ACTIVE | 0x10));	/* Version 1 */
	out_uint16_le(s, (rdp->sec->mcs->mcs_userid + 1001));

	out_uint32_le(s, rdp->rdp_shareid);
        /* originatorID must be set to the server channel ID
            This value is always 0x3EA for Microsoft RDP server implementations */
	out_uint16_le(s, 0x3EA); // originatorID
	out_uint16_le(s, sizeof(RDP_SOURCE)); // lengthSourceDescriptor
        /* lengthCombinedCapabilities is the combined size of
            numberCapabilities, pad2Octets and capabilitySets */
	out_uint16_le(s, caplen + 4); // lengthCombinedCapabilities

        /* sourceDescriptor is "MSTSC" for Microsoft RDP clients */
	out_uint8p(s, RDP_SOURCE, sizeof(RDP_SOURCE)); // sourceDescriptor
	out_uint16_le(s, numberCapabilities); // numberCapabilities
	out_uint8s(s, 2); // pad
	out_uint8p(s, caps->data, caplen);
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

        in_uint16_le(s, numberCapabilities);
        in_uint8s(s, 2); // pad

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

                        case CAPSET_TYPE_COMPDESK:
                                rdp_process_compdesk_capset(rdp, s);
                                break;

                        case CAPSET_TYPE_MULTIFRAGMENTUPDATE:
                                rdp_process_multifragmentupdate_capset(rdp, s);
                                break;

                        default:
				ui_unimpl(rdp->inst, "Capability set 0x%02X processing\n", capabilitySetType);
				break;
		}

		s->p = next;
	}
}

/* Respond to a demand active PDU */
static void
process_demand_active(rdpRdp * rdp, STREAM s)
{
	uint8 type;
	uint16 lengthSourceDescriptor;
	uint16 lengthCombinedCapabilities;

	in_uint32_le(s, rdp->rdp_shareid); // shareId
	in_uint16_le(s, lengthSourceDescriptor); // lengthSourceDescriptor
	in_uint16_le(s, lengthCombinedCapabilities); // lengthCombinedCapabilities
	in_uint8s(s, lengthSourceDescriptor); // sourceDescriptor, should be "RDP"

	DEBUG("DEMAND_ACTIVE(id=0x%x)\n", rdp->rdp_shareid);
	rdp_process_server_caps(rdp, s, lengthCombinedCapabilities);
	in_uint8s(s, 4); // sessionID, ignored by the client

	rdp_send_confirm_active(rdp);
	rdp_send_synchronize(rdp);
	rdp_send_control(rdp, RDP_CTL_COOPERATE);
	rdp_send_control(rdp, RDP_CTL_REQUEST_CONTROL);
	rdp_recv(rdp, &type);	/* RDP_PDU_SYNCHRONIZE */
	rdp_recv(rdp, &type);	/* RDP_CTL_COOPERATE */
	rdp_recv(rdp, &type);	/* RDP_CTL_GRANT_CONTROL */

	// Synchronize toggle keys
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

	rdp_recv(rdp, &type);	/* RDP_PDU_UNKNOWN 0x28 (Fonts?) */
	reset_order_state(rdp->orders);
}

/* Process a colour pointer PDU */
static void
process_colour_pointer_common(rdpRdp * rdp, STREAM s, int bpp)
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
	if ((width != 32) || (height != 32))
	{
		ui_error(rdp->inst, "process_colour_pointer_common: error "
			"width %d height %d\n", width, height);
		return;
	}
	x = MAX(x, 0);
	x = MIN(x, width - 1);
	y = MAX(y, 0);
	y = MIN(y, height - 1);
	cursor = ui_create_cursor(rdp->inst, x, y, width, height, mask, data, bpp);
	ui_set_cursor(rdp->inst, cursor);
	cache_put_cursor(rdp->cache, cache_idx, cursor);
}

/* Process a colour pointer PDU */
void
process_colour_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	process_colour_pointer_common(rdp, s, 24);
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
void
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
			ui_unimpl(rdp->inst, "System pointer message 0x%x\n", system_pointer_type);
	}
}

/* Process a new pointer PDU */
void
process_new_pointer_pdu(rdpRdp * rdp, STREAM s)
{
	int xor_bpp;

	in_uint16_le(s, xor_bpp);
	process_colour_pointer_common(rdp, s, xor_bpp);
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
			if (s_check(s))
				ui_move_pointer(rdp->inst, x, y);
			break;

		case RDP_PTRMSGTYPE_COLOR:
			process_colour_pointer_pdu(rdp, s);
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
			ui_unimpl(rdp->inst, "Pointer message 0x%x\n", message_type);
	}
}

/* Process bitmap updates */
void
process_bitmap_updates(rdpRdp * rdp, STREAM s)
{
	uint16 num_updates;
	uint16 left, top, right, bottom, width, height;
	uint16 cx, cy, bpp, Bpp, compress, bufsize, size;
	uint8 *data, *bmpdata;
	int i;

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

		DEBUG("BITMAP_UPDATE(l=%d,t=%d,r=%d,b=%d,w=%d,h=%d,Bpp=%d,cmp=%d)\n",
		       left, top, right, bottom, width, height, Bpp, compress);

		if (!compress)
		{
			int y;
			bmpdata = (uint8 *) xmalloc(width * height * Bpp);
			for (y = 0; y < height; y++)
			{
				in_uint8a(s, &bmpdata[(height - y - 1) * (width * Bpp)],
					  width * Bpp);
			}
			ui_paint_bitmap(rdp->inst, left, top, cx, cy, width, height, bmpdata);
			xfree(bmpdata);
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
		bmpdata = (uint8 *) xmalloc(width * height * Bpp);
		if (bitmap_decompress(rdp->inst, bmpdata, width, height, data, size, Bpp))
		{
			ui_paint_bitmap(rdp->inst, left, top, cx, cy, width, height, bmpdata);
		}
		else
		{
			DEBUG_RDP5("Failed to decompress data\n");
		}

		xfree(bmpdata);
	}
}

/* Process a palette update */
void
process_palette(rdpRdp * rdp, STREAM s)
{
	RD_COLOURENTRY *entry;
	RD_COLOURMAP map;
	RD_HCOLOURMAP hmap;
	int i;

	in_uint8s(s, 2);	/* pad */
	in_uint16_le(s, map.ncolours);
	in_uint8s(s, 2);	/* pad */

	map.colours = (RD_COLOURENTRY *) xmalloc(sizeof(RD_COLOURENTRY) * map.ncolours);

	DEBUG("PALETTE(c=%d)\n", map.ncolours);

	for (i = 0; i < map.ncolours; i++)
	{
		entry = &map.colours[i];
		in_uint8(s, entry->red);
		in_uint8(s, entry->green);
		in_uint8(s, entry->blue);
	}

	hmap = ui_create_colourmap(rdp->inst, &map);
	ui_set_colourmap(rdp->inst, hmap);

	xfree(map.colours);
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
			break;

		default:
			ui_unimpl(rdp->inst, "update %d\n", update_type);
	}
	ui_end_update(rdp->inst);
}

/* Process a disconnect PDU */
void
process_disconnect_pdu(STREAM s, uint32 * ext_disc_reason)
{
	in_uint32_le(s, *ext_disc_reason);

	DEBUG("Received disconnect PDU\n");
}

/* Process data PDU */
static RD_BOOL
process_data_pdu(rdpRdp * rdp, STREAM s, uint32 * ext_disc_reason)
{
	uint8 data_pdu_type;
	uint8 ctype;
	uint16 clen;
	uint32 len;
	uint32 roff, rlen;
	struct stream * ns;

	in_uint8s(s, 6);	/* shareid, pad, streamid */
	in_uint16_le(s, len);
	in_uint8(s, data_pdu_type);
	in_uint8(s, ctype);
	in_uint16_le(s, clen);

	if (ctype & RDP_MPPC_COMPRESSED)
	{
		ns = &(rdp->mppc_dict.ns);
		clen -= 18;
		if (len > RDP_MPPC_DICT_SIZE)
			ui_error(rdp->inst, "error decompressed packet size exceeds max\n");
		if (mppc_expand(rdp, s->p, clen, ctype, &roff, &rlen) == -1)
			ui_error(rdp->inst, "error while decompressing packet\n");
		/* allocate memory and copy the uncompressed data into the temporary stream */
		ns->data = (uint8 *) xrealloc(ns->data, rlen);
		memcpy(ns->data, rdp->mppc_dict.hist + roff, rlen);
		ns->size = rlen;
		ns->end = (ns->data + ns->size);
		ns->p = ns->data;
		ns->rdp_hdr = ns->p;
		s = ns;
	}

	switch (data_pdu_type)
	{
		case RDP_DATA_PDU_UPDATE:
			process_update_pdu(rdp, s);
			break;

		case RDP_DATA_PDU_CONTROL:
			DEBUG("Received Control PDU\n");
			break;

		case RDP_DATA_PDU_SYNCHRONIZE:
			DEBUG("Received Sync PDU\n");
			break;

		case RDP_DATA_PDU_POINTER:
			process_pointer_pdu(rdp, s);
			break;

		case RDP_DATA_PDU_BELL:
			ui_bell(rdp->inst);
			break;

		case RDP_DATA_PDU_LOGON:
			DEBUG("Received Logon PDU\n");
			/* User logged on */
			break;

		case RDP_DATA_PDU_DISCONNECT:
			process_disconnect_pdu(s, ext_disc_reason);

			/* We used to return true and disconnect immediately here, but
			 * Windows Vista sends a disconnect PDU with reason 0 when
			 * reconnecting to a disconnected session, and MSTSC doesn't
			 * drop the connection.  I think we should just save the status.
			 */
			break;

		default:
			ui_unimpl(rdp->inst, "data PDU %d\n", data_pdu_type);
	}
	return False;
}

/* Process redirect PDU from Session Directory */
static RD_BOOL
process_redirect_pdu(rdpRdp * rdp, STREAM s /*, uint32 * ext_disc_reason */ )
{
	uint32 len;

	/* these 2 bytes are unknown, seem to be zeros */
	in_uint8s(s, 2);

	/* read connection flags */
	in_uint32_le(s, rdp->redirect_flags);

	/* read length of ip string */
	in_uint32_le(s, len);

	/* read ip string */
	rdp_in_unistr(rdp, s, rdp->redirect_server, sizeof(rdp->redirect_server), len);

	/* read length of cookie string */
	in_uint32_le(s, len);

	/* read cookie string (plain ASCII) */
	if (len > sizeof(rdp->redirect_cookie) - 1)
	{
		uint32 rem = len - (sizeof(rdp->redirect_cookie) - 1);
		len = sizeof(rdp->redirect_cookie) - 1;

		ui_warning(rdp->inst, "Unexpectedly large redirection cookie\n");
		in_uint8a(s, rdp->redirect_cookie, len);
		in_uint8s(s, rem);
	}
	else
	{
		in_uint8a(s, rdp->redirect_cookie, len);
	}
	rdp->redirect_cookie[len] = 0;

	/* read length of username string */
	in_uint32_le(s, len);

	/* read username string */
	rdp_in_unistr(rdp, s, rdp->redirect_username, sizeof(rdp->redirect_username), len);

	/* read length of domain string */
	in_uint32_le(s, len);

	/* read domain string */
	rdp_in_unistr(rdp, s, rdp->redirect_domain, sizeof(rdp->redirect_domain), len);

	/* read length of password string */
	in_uint32_le(s, len);

	/* read password string */
	rdp_in_unistr(rdp, s, rdp->redirect_password, sizeof(rdp->redirect_password), len);

	rdp->redirect = True;

	return True;
}

/* Process incoming packets */
/* nevers gets out of here till app is done */
void
rdp_main_loop(rdpRdp * rdp, RD_BOOL * deactivated, uint32 * ext_disc_reason)
{
	while (rdp_loop(rdp, deactivated, ext_disc_reason))
		;
}

/* used in uiports and rdp_main_loop, processes the rdp packets waiting */
RD_BOOL
rdp_loop(rdpRdp * rdp, RD_BOOL * deactivated, uint32 * ext_disc_reason)
{
	uint8 type;
	RD_BOOL disc = False;	/* True when a disconnect PDU was received */
	RD_BOOL cont = True;
	STREAM s;

	while (cont)
	{
		s = rdp_recv(rdp, &type);
		if (s == NULL)
			return False;
		switch (type)
		{
			case RDP_PDU_DEMAND_ACTIVE:
				process_demand_active(rdp, s);
				*deactivated = False;
				break;
			case RDP_PDU_DEACTIVATE:
				DEBUG("RDP_PDU_DEACTIVATE\n");
				*deactivated = True;
				break;
			case RDP_PDU_REDIRECT:
				return process_redirect_pdu(rdp, s);
				break;
			case RDP_PDU_DATA:
				disc = process_data_pdu(rdp, s, ext_disc_reason);
				break;
			case 0:
				break;
			default:
				ui_unimpl(rdp->inst, "PDU %d\n", type);
		}
		if (disc)
			return False;
		cont = rdp->next_packet < s->end;
	}
	return True;
}

/* Establish a connection up to the RDP layer */
RD_BOOL
rdp_connect(rdpRdp * rdp, char *server, uint32 flags, char *domain, char *password,
	    char *command, char *directory, int port, char *username)
{
	if (!sec_connect(rdp->sec, server, username, port))
		return False;

	rdp_send_logon_info(rdp, flags, domain, username, password, command, directory);
	return True;
}

/* Establish a reconnection up to the RDP layer */
RD_BOOL
rdp_reconnect(rdpRdp * rdp, char *server, uint32 flags, char *domain, char *password,
	      char *command, char *directory, char *cookie, int port, char *username)
{
	if (!sec_reconnect(rdp->sec, server, port))
		return False;

	rdp_send_logon_info(rdp, flags, domain, username, password, command, directory);
	return True;
}

/* Called during redirection to reset the state to support redirection */
void
rdp_reset_state(rdpRdp * rdp)
{
	rdp->next_packet = NULL;	/* reset the packet information */
	rdp->rdp_shareid = 0;
	sec_reset_state(rdp->sec);
}

/* Disconnect from the RDP layer */
void
rdp_disconnect(rdpRdp * rdp)
{
	sec_disconnect(rdp->sec);
}

rdpRdp *
rdp_setup(struct rdp_set *settings)
{
	rdpRdp *self;

	self = (rdpRdp *) xmalloc(sizeof(rdpRdp));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpRdp));
		self->settings = settings;
		self->current_status = 1;
		self->iconv_works = True;
		self->in_iconv_h = (void *) (-1);
		self->out_iconv_h = (void *) (-1);
		self->sec = sec_setup(self);
		self->orders = orders_setup(self);
		self->pcache = pcache_setup(self);
		self->cache = cache_setup(self);
	}
	return self;
}

void
rdp_cleanup(rdpRdp * rdp)
{
	if (rdp != NULL)
	{
		cache_cleanup(rdp->cache);
		pcache_cleanup(rdp->pcache);
		orders_cleanup(rdp->orders);
		sec_cleanup(rdp->sec);
		xfree(rdp);
	}
}
