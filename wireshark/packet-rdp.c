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

gint offset = 0;
int proto_rdp = -1;
static int hf_rdp_rdp = -1;
static int hf_rdp_tpkt = -1;
static int hf_rdp_x224 = -1;
static int hf_rdp_mcs = -1;
static gint ett_rdp = -1;

#define MCS_ERECT_DOMAIN_REQUEST		0x01
#define MCS_DISCONNECT_PROVIDER_ULTIMATUM	0x08
#define MCS_ATTACH_USER_REQUEST			0x0A
#define MCS_ATTACH_USER_CONFIRM			0x0B
#define MCS_CHANNEL_JOIN_REQUEST		0x0E
#define MCS_CHANNEL_JOIN_CONFIRM		0x0F
#define MCS_SEND_DATA_REQUEST			0x19
#define MCS_SEND_DATA_INDICATION		0x1A

static const value_string t125_mcs_tpdu_types[] = {
	{ MCS_ERECT_DOMAIN_REQUEST,		"Erect Domain Request" },
	{ MCS_DISCONNECT_PROVIDER_ULTIMATUM,	"Disconnect Provider Ultimatum" },
	{ MCS_ATTACH_USER_REQUEST,		"Attach User Request" },
	{ MCS_ATTACH_USER_CONFIRM,		"Attach User Confirm" },
	{ MCS_CHANNEL_JOIN_REQUEST,		"Channel Join Request" },
	{ MCS_CHANNEL_JOIN_CONFIRM,		"Channel Join Confirm" },
	{ MCS_SEND_DATA_REQUEST,		"Send Data Request" },
	{ MCS_SEND_DATA_INDICATION,		"Send Data Indication" },
	{ 0x0,	NULL }
};


#define X224_CONNECTION_REQUEST		0xE
#define X224_CONNECTION_CONFIRM		0xD
#define X224_DISCONNECT_REQUEST		0x8
#define X224_DISCONNECT_CONFIRM		0xC
#define X224_DATA			0xF	

static const value_string x224_tpdu_types[] = {
	{ X224_CONNECTION_REQUEST,	"Connection Request" },
	{ X224_CONNECTION_CONFIRM,	"Connection Confirm" },
	{ X224_DISCONNECT_REQUEST,	"Disconnect Request" },
	{ X224_DISCONNECT_CONFIRM,	"Disconnect Confirm" },
	{ X224_DATA,			"Data" },
	{ 0x0,	NULL }
};

void proto_reg_handoff_rdp(void);

static void
dissect_mcs(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	gint bytes;
	guint8 type;
	guint8 byte;
	guint8 flags;
	guint16 initiator;
	guint16 channelId;
	guint16 length;
	guint16 header_length;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes > 0)
		{
			proto_item *ti;

			type = tvb_get_bits8(tvb, offset * 8, 6);

			switch (type)
			{
				case MCS_SEND_DATA_REQUEST:
				case MCS_SEND_DATA_INDICATION:
					initiator = tvb_get_ntohs(tvb, offset + 1);
					channelId = tvb_get_ntohs(tvb, offset + 3);
					flags = tvb_get_guint8(tvb, offset + 4);

					byte = tvb_get_guint8(tvb, offset + 6);
					length = (guint16) byte;
					header_length = 7;

					if (byte & 0x80)
					{
						length &= ~0x80;
						length <<= 8;
						byte = tvb_get_guint8(tvb, offset + 7);
						length += (guint16) byte;
						header_length++;
					}

					ti = proto_tree_add_item(tree, hf_rdp_mcs, tvb, offset, header_length, FALSE);
					proto_item_set_text(ti, "T.125 MCS %s PDU, Length = %d", val_to_str(type, t125_mcs_tpdu_types, "Unknown %d"), length);
					break;

				default:
					ti = proto_tree_add_item(tree, hf_rdp_mcs, tvb, offset, -1, FALSE);
					proto_item_set_text(ti, "T.125 MCS %s PDU", val_to_str(type, t125_mcs_tpdu_types, "Unknown %d"));
					break;
			}
		}
	}
}

static void
dissect_x224(tvbuff_t *tvb, packet_info *pinfo _U_ , proto_tree *tree)
{
	gint bytes;
	guint8 type;
	guint8 length;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes > 0)
		{
			proto_item *ti;
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
	gint bytes;
	guint8 version;
	guint16 length;

	if (tree)
	{
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes >= 4)
		{
			proto_item *ti;
			version = tvb_get_guint8(tvb, 0);
			length = tvb_get_ntohs(tvb, 2);

			if (version == 3)
			{
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
	gint bytes;

	if (tree)
	{
		offset = 0;
		bytes = tvb_length_remaining(tvb, 0);

		if (bytes > 0)
		{
			proto_item *ti;
			proto_tree *rdp_tree;

			ti = proto_tree_add_item(tree, proto_rdp, tvb, 0, -1, FALSE);
			rdp_tree = proto_item_add_subtree(ti, ett_rdp);

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
		  { "MCS Header", "rdp.mcs", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } }
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

