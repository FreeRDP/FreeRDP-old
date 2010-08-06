/* packet-rdp.c
 * Routines for Remote Desktop Protocol dissection
 * Copyright 2010, Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include "packet-rdp.h"

gint bytes = 0;
gint offset = 0;
gint rdp_offset = 0;
gint tpkt_offset = 0;
gint x224_offset = 0;
gint mcs_offset = 0;
gint ts_security_header_offset = 0;
gint ts_share_control_header_offset = 0;
gint ts_share_data_header_offset = 0;

int proto_rdp = -1;
static int hf_rdp_rdp = -1;
static int hf_rdp_tpkt = -1;
static int hf_rdp_x224 = -1;
static int hf_rdp_mcs = -1;
static int hf_ts_security_header = -1;
static int hf_ts_share_control_header = -1;
static int hf_ts_share_data_header = -1;
static gint ett_rdp = -1;

#define SEC_EXCHANGE_PKT			0x0001
#define SEC_ENCRYPT				0x0008
#define SEC_RESET_SEQNO				0x0010
#define SEC_IGNORE_SEQNO			0x0020
#define SEC_INFO_PKT				0x0040
#define SEC_LICENSE_PKT				0x0080
#define SEC_LICENSE_ENCRYPT_CS			0x0200
#define SEC_LICENSE_ENCRYPT_SC			0x0200
#define SEC_REDIRECTION_PKT			0x0400
#define SEC_SECURE_CHECKSUM			0x0800
#define SEC_FLAGSHI_VALID			0x8000

#define PDUTYPE_DEMAND_ACTIVE_PDU		0x1
#define PDUTYPE_CONFIRM_ACTIVE_PDU		0x3
#define PDUTYPE_DEACTIVATE_ALL_PDU		0x6
#define PDUTYPE_DATA_PDU			0x7
#define PDUTYPE_SERVER_REDIR_PKT		0xA

#define	PDUTYPE2_UPDATE				2
#define	PDUTYPE2_CONTROL			20
#define	PDUTYPE2_POINTER			27
#define	PDUTYPE2_INPUT				28
#define	PDUTYPE2_SYNCHRONIZE			31
#define	PDUTYPE2_REFRESH_RECT			33
#define	PDUTYPE2_PLAY_SOUND			34
#define	PDUTYPE2_SUPPRESS_OUTPUT		35
#define	PDUTYPE2_SHUTDOWN_REQUEST		36
#define	PDUTYPE2_SHUTDOWN_DENIED		37
#define	PDUTYPE2_SAVE_SESSION_INFO		38
#define	PDUTYPE2_FONTLIST			39
#define	PDUTYPE2_FONTMAP			40
#define	PDUTYPE2_SET_KEYBOARD_INDICATORS	41
#define	PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST	43
#define	PDUTYPE2_BITMAPCACHE_ERROR_PDU		44
#define	PDUTYPE2_SET_KEYBOARD_IME_STATUS	45
#define	PDUTYPE2_OFFSCRCACHE_ERROR_PDU		46
#define	PDUTYPE2_SET_ERROR_INFO_PDU		47
#define	PDUTYPE2_DRAWNINEGRID_ERROR_PDU		48
#define	PDUTYPE2_DRAWGDIPLUS_ERROR_PDU		49
#define	PDUTYPE2_ARC_STATUS_PDU			50
#define	PDUTYPE2_STATUS_INFO_PDU		54
#define	PDUTYPE2_MONITOR_LAYOUT_PDU		55

#define MCS_ERECT_DOMAIN_REQUEST		0x01
#define MCS_DISCONNECT_PROVIDER_ULTIMATUM	0x08
#define MCS_ATTACH_USER_REQUEST			0x0A
#define MCS_ATTACH_USER_CONFIRM			0x0B
#define MCS_CHANNEL_JOIN_REQUEST		0x0E
#define MCS_CHANNEL_JOIN_CONFIRM		0x0F
#define MCS_SEND_DATA_REQUEST			0x19
#define MCS_SEND_DATA_INDICATION		0x1A
#define MCS_CONNECT_INITIAL			0x65
#define MCS_CONNECT_RESPONSE			0x66

#define X224_CONNECTION_REQUEST			0xE
#define X224_CONNECTION_CONFIRM			0xD
#define X224_DISCONNECT_REQUEST			0x8
#define X224_DISCONNECT_CONFIRM			0xC
#define X224_DATA				0xF

static const value_string pdu_types[] = {
	{ PDUTYPE_DEMAND_ACTIVE_PDU,		"Demand Active" },
	{ PDUTYPE_CONFIRM_ACTIVE_PDU,		"Confirm Active" },
	{ PDUTYPE_DEACTIVATE_ALL_PDU,		"Deactivate All" },
	{ PDUTYPE_DATA_PDU,			"Data" },
	{ PDUTYPE_SERVER_REDIR_PKT,		"Server Redirection Packet" },
	{ 0x0,	NULL }
};

static const value_string pdu2_types[] = {
	{ PDUTYPE2_UPDATE,			"Update" },
	{ PDUTYPE2_CONTROL,			"Control" },
	{ PDUTYPE2_POINTER,			"Pointer" },
	{ PDUTYPE2_INPUT,			"Input" },
	{ PDUTYPE2_SYNCHRONIZE,			"Synchronize" },
	{ PDUTYPE2_REFRESH_RECT,		"Refresh Rect" },
	{ PDUTYPE2_PLAY_SOUND,			"Play Sound" },
	{ PDUTYPE2_SUPPRESS_OUTPUT,		"Suppress Output" },
	{ PDUTYPE2_SHUTDOWN_REQUEST,		"Shutdown Request" },
	{ PDUTYPE2_SHUTDOWN_DENIED,		"Shutdown Denied" },
	{ PDUTYPE2_SAVE_SESSION_INFO,		"Save Session Info" },
	{ PDUTYPE2_FONTLIST,			"Font List" },
	{ PDUTYPE2_FONTMAP,			"Font Map" },
	{ PDUTYPE2_SET_KEYBOARD_INDICATORS,	"Set Keyboard Indicator" },
	{ PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST,	"Bitmap Cache Persistent List" },
	{ PDUTYPE2_BITMAPCACHE_ERROR_PDU,	"Bitmap Cache Error" },
	{ PDUTYPE2_SET_KEYBOARD_IME_STATUS,	"Set Keyboard IME Status" },
	{ PDUTYPE2_OFFSCRCACHE_ERROR_PDU,	"Offscreen Cache Error" },
	{ PDUTYPE2_SET_ERROR_INFO_PDU,		"Set Error Info" },
	{ PDUTYPE2_DRAWNINEGRID_ERROR_PDU,	"Draw Nine Grid Error" },
	{ PDUTYPE2_DRAWGDIPLUS_ERROR_PDU,	"Draw GDI+ Error" },
	{ PDUTYPE2_ARC_STATUS_PDU,		"Arc Status" },
	{ PDUTYPE2_STATUS_INFO_PDU,		"Status Info" },
	{ PDUTYPE2_MONITOR_LAYOUT_PDU,		"Monitor Layout" },
	{ 0x0,	NULL }
};

static const value_string t125_mcs_tpdu_types[] = {
	{ MCS_ERECT_DOMAIN_REQUEST,		"Erect Domain Request" },
	{ MCS_DISCONNECT_PROVIDER_ULTIMATUM,	"Disconnect Provider Ultimatum" },
	{ MCS_ATTACH_USER_REQUEST,		"Attach User Request" },
	{ MCS_ATTACH_USER_CONFIRM,		"Attach User Confirm" },
	{ MCS_CHANNEL_JOIN_REQUEST,		"Channel Join Request" },
	{ MCS_CHANNEL_JOIN_CONFIRM,		"Channel Join Confirm" },
	{ MCS_SEND_DATA_REQUEST,		"Send Data Request" },
	{ MCS_SEND_DATA_INDICATION,		"Send Data Indication" },
	{ MCS_CONNECT_INITIAL,			"Connect Initial" },
	{ MCS_CONNECT_RESPONSE,			"Connect Response" },
	{ 0x0,	NULL }
};	

static const value_string x224_tpdu_types[] = {
	{ X224_CONNECTION_REQUEST,		"Connection Request" },
	{ X224_CONNECTION_CONFIRM,		"Connection Confirm" },
	{ X224_DISCONNECT_REQUEST,		"Disconnect Request" },
	{ X224_DISCONNECT_CONFIRM,		"Disconnect Confirm" },
	{ X224_DATA,				"Data" },
	{ 0x0,	NULL }
};

void proto_reg_handoff_rdp(void);
void dissect_ts_info_packet(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree);
void dissect_ts_share_control_header(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree);
void dissect_ts_share_data_header(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree);
void dissect_ts_security_header(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree);

void
dissect_ts_info_packet(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint32 codePage;
	guint32 flags;
	guint16 cbDomain;
	guint16 cbUserName;
	guint16 cbPassword;
	guint16 cbAlternateShell;
	guint16 cbWorkingDir;

	if (tree)
	{
		codePage = tvb_get_letohl(tvb, offset);
		flags = tvb_get_letohl(tvb, offset + 4);
		cbDomain = tvb_get_letohs(tvb, offset + 6);
		cbUserName = tvb_get_letohs(tvb, offset + 8);
		cbPassword = tvb_get_letohs(tvb, offset + 10);
		cbAlternateShell = tvb_get_letohs(tvb, offset + 12);
		cbWorkingDir = tvb_get_letohs(tvb, offset + 14);
	}
}

void
dissect_ts_share_data_header(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint32 shareId;
	guint8 streamId;
	guint16 uncompressedLength;
	guint8 pduType2;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes >= 4)
		{
			proto_item *ti;
			ts_share_control_header_offset = offset;
			shareId = tvb_get_letohl(tvb, offset);	
			streamId = tvb_get_guint8(tvb, offset + 5);
			uncompressedLength = tvb_get_letohs(tvb, offset + 6);
			pduType2 = tvb_get_guint8(tvb, offset + 8);
			offset += 9;

			ti = proto_tree_add_item(tree, hf_ts_share_data_header, tvb, ts_share_data_header_offset, offset - ts_share_data_header_offset, FALSE);
			proto_item_set_text(ti, "TS_SHARE_DATA_HEADER: %s", val_to_str(pduType2, pdu2_types, "Unknown %d"));

			col_clear(pinfo->cinfo, COL_INFO);
			col_add_fstr(pinfo->cinfo, COL_INFO, "%s PDU", val_to_str(pduType2, pdu2_types, "Data %d PDU"));
		}
	}
}

void
dissect_ts_share_control_header(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint16 pduType;
	guint16 PDUSource;
	guint16 totalLength;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);
		totalLength = tvb_get_letohs(tvb, offset);

		if (bytes >= 4)
		{
			proto_item *ti;
			ts_share_control_header_offset = offset;
			pduType = tvb_get_letohs(tvb, offset + 2) & 0xF;
			PDUSource = tvb_get_letohs(tvb, offset + 4);

			if (totalLength == 128)
				return;

			offset += 6;
			ti = proto_tree_add_item(tree, hf_ts_share_control_header, tvb, ts_share_control_header_offset, offset - ts_share_control_header_offset, FALSE);
			proto_item_set_text(ti, "TS_SHARE_CONTROL_HEADER: %s", val_to_str(pduType, pdu_types, "Unknown %d"));

			switch (pduType)
			{
				case PDUTYPE_DEMAND_ACTIVE_PDU:
					col_set_str(pinfo->cinfo, COL_INFO, "Demand Active PDU");
					break;

				case PDUTYPE_CONFIRM_ACTIVE_PDU:
					col_set_str(pinfo->cinfo, COL_INFO, "Confirm Active PDU");
					break;

				case PDUTYPE_DATA_PDU:
					dissect_ts_share_data_header(tvb, pinfo, tree);
					break;
			}
		}
	}
}

void
dissect_ts_security_header(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint16 flags;
	guint16 flagsHi;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes >= 4)
		{
			proto_item *ti;
			ts_security_header_offset = offset;
			flags = tvb_get_letohs(tvb, offset);
			flagsHi = tvb_get_letohs(tvb, offset + 2);
			offset += 4;

			ti = proto_tree_add_item(tree, hf_ts_security_header, tvb, ts_security_header_offset, offset - ts_security_header_offset, FALSE);
			proto_item_set_text(ti, "TS_SECURITY_HEADER, Flags = 0x%04X", flags);

			if (flags & SEC_INFO_PKT)
			{
				dissect_ts_info_packet(tvb, pinfo, tree);
				col_clear(pinfo->cinfo, COL_INFO);
				col_add_str(pinfo->cinfo, COL_INFO, "Client Info PDU");
			}
		}
	}
}

static void
dissect_mcs(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint8 type;
	guint8 byte;
	guint8 flags;
	guint16 initiator;
	guint16 channelId;
	guint16 length;
	guint16 real_length;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes > 0)
		{
			proto_item *ti;
			type = tvb_get_bits8(tvb, offset * 8, 6);
			mcs_offset = offset++;

			/* Connect Initial and Connect Response */
			if (type == 31)
				type = tvb_get_guint8(tvb, offset++);

			col_clear(pinfo->cinfo, COL_INFO);
			col_add_fstr(pinfo->cinfo, COL_INFO, "MCS %s PDU", val_to_str(type, t125_mcs_tpdu_types, "Unknown %d"));

			switch (type)
			{
				case MCS_SEND_DATA_INDICATION:
				case MCS_SEND_DATA_REQUEST:
					initiator = tvb_get_ntohs(tvb, offset + 2);
					channelId = tvb_get_ntohs(tvb, offset + 4);
					offset += 4;
					flags = tvb_get_guint8(tvb, offset++);

					byte = tvb_get_guint8(tvb, offset++);
					length = (guint16) byte;

					if (byte & 0x80)
					{
						length &= ~0x80;
						length <<= 8;
						byte = tvb_get_guint8(tvb, offset++);
						length += (guint16) byte;
					}

					ti = proto_tree_add_item(tree, hf_rdp_mcs, tvb, mcs_offset, offset - mcs_offset, FALSE);
					proto_item_set_text(ti, "T.125 MCS %s PDU, Length = %d", val_to_str(type, t125_mcs_tpdu_types, "Unknown %d"), length);

					real_length = tvb_length(tvb) - rdp_offset;
					if ((offset - rdp_offset) + length != real_length)
						proto_item_append_text(ti, " [Length Mismatch: %d]", real_length);

					dissect_ts_share_control_header(tvb, pinfo, tree);
					break;

				default:
					ti = proto_tree_add_item(tree, hf_rdp_mcs, tvb, mcs_offset, -1, FALSE);
					proto_item_set_text(ti, "T.125 MCS %s PDU", val_to_str(type, t125_mcs_tpdu_types, "Unknown %d"));
					break;
			}
		}
	}
}

static void
dissect_x224(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint8 type;
	guint8 length;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes > 0)
		{
			proto_item *ti;
			x224_offset = offset;
			length = tvb_get_guint8(tvb, offset);
			type = tvb_get_bits8(tvb, (offset + 1) * 8, 4);

			if (length > 1)
			{
				ti = proto_tree_add_item(tree, hf_rdp_x224, tvb, offset, length + 1, FALSE);
				proto_item_set_text(ti, "X.224 %s TPDU", val_to_str(type, x224_tpdu_types, "Unknown %d"));
				offset += (length + 1);
				dissect_mcs(tvb, pinfo, tree);
			}
		}
	}
}

static void
dissect_tpkt(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	guint8 version;
	guint16 length;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes >= 4)
		{
			proto_item *ti;
			version = tvb_get_guint8(tvb, offset);
			length = tvb_get_ntohs(tvb, offset + 2);

			if (version == 3)
			{
				tpkt_offset = offset;
				ti = proto_tree_add_item(tree, hf_rdp_tpkt, tvb, 0, 4, FALSE);
				proto_item_set_text(ti, "TPKT Header, Length = %d", length);
				offset += 4;
				dissect_x224(tvb, pinfo, tree);
			}
		}
	}
}

static void
dissect_rdp(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	if (tree)
	{
		offset = 0;
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes > 0)
		{
			proto_item *ti;
			proto_tree *rdp_tree;

			rdp_offset = offset;
			ti = proto_tree_add_item(tree, proto_rdp, tvb, 0, -1, FALSE);
			rdp_tree = proto_item_add_subtree(ti, ett_rdp);

			col_set_str(pinfo->cinfo, COL_PROTOCOL, "RDP");

			dissect_tpkt(tvb, pinfo, rdp_tree);
		}
	}
}

void
proto_register_rdp(void)
{
	module_t *module_rdp;

	static hf_register_info hf[] =
	{
		{ &hf_rdp_rdp,
		  { "rdp", "rdp", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
		{ &hf_rdp_tpkt,
		  { "TPKT Header", "rdp.tpkt", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL } },
		{ &hf_rdp_x224,
		  { "X.224 Header", "rdp.x224", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL } },
		{ &hf_rdp_mcs,
		  { "MCS Header", "rdp.mcs", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
		{ &hf_ts_security_header,
		  { "TS_SECURITY_HEADER", "rdp.sec", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
		{ &hf_ts_share_control_header,
		  { "TS_SHARE_CONTROL_HEADER", "rdp.share_ctrl", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
		{ &hf_ts_share_data_header,
		  { "TS_SHARE_DATA_HEADER", "rdp.share_data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } }
	};

	static gint *ett[] = {
		&ett_rdp
	};
	
	proto_rdp = proto_register_protocol("Remote Desktop Protocol", "RDP", "rdp");
	register_dissector("rdp", dissect_rdp, proto_rdp);

	proto_register_field_array(proto_rdp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));	
	module_rdp = prefs_register_protocol( proto_rdp, proto_reg_handoff_rdp);
}

void
proto_reg_handoff_rdp(void)
{

}

