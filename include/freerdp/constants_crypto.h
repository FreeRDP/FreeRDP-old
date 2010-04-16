/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Cryptographic constants

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

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

#ifndef __CONSTANTS_CRYPTO_H
#define __CONSTANTS_CRYPTO_H

/* RDP Negotiation Protocols */
enum RDP_NEG_PROTOCOLS
{
	PROTOCOL_RDP = 0x00000000,
	PROTOCOL_SSL = 0x00000001,
	PROTOCOL_HYBRID = 0x00000002
};

/* RDP Negotiation Failure failureCode */
enum RDP_NEG_FAILURE_FAILURECODES
{
	SSL_REQUIRED_BY_SERVER = 0x00000001,
	SSL_NOT_ALLOWED_BY_SERVER = 0x00000002,
	SSL_CERT_NOT_ON_SERVER = 0x00000003,
	INCONSISTENT_FLAGS = 0x00000004,
	HYBRID_REQUIRED_BY_SERVER = 0x00000005
};

/* RDP secure transport constants */
#define SEC_RANDOM_SIZE		32
#define SEC_MODULUS_SIZE	64
#define SEC_MAX_MODULUS_SIZE	256
#define SEC_PADDING_SIZE	8
#define SEC_EXPONENT_SIZE	4

/* RDP Basic Security Header flags */
enum RDP_BASIC_SEC_FLAGS
{
	SEC_EXCHANGE_PKT = 0x0001,
	SEC_ENCRYPT = 0x0008,
	SEC_RESET_SEQNO = 0x0010,	/* ignore */
	SEC_IGNORE_SEQNO = 0x0020,	/* ignore */
	SEC_INFO_PKT = 0x0040,
	SEC_LICENSE_PKT = 0x0080,
	SEC_REDIRECTION_PKT = 0x0400
};

/* User Data Header types */
enum RDP_MCS_CONNECT_BLOCKS
{
	UDH_CS_CORE = 0xc001,
	UDH_CS_SECURITY = 0xc002,
	UDH_CS_NET = 0xc003,
	UDH_CS_CLUSTER = 0xc004,
	UDH_CS_MONITOR = 0xc005,
	UDH_SC_CORE = 0x0c01,
	UDH_SC_SECURITY = 0x0c02,
	UDH_SC_NET = 0x0c03
};

/* Server Proprietary Certificate constants */
#define SIGNATURE_ALG_RSA	1
#define KEY_EXCHANGE_ALG_RSA	1
#define BB_RSA_KEY_BLOB		0x0006
#define BB_RSA_SIGNATURE_BLOB		0x0008
#define SEC_RSA_MAGIC		0x31415352	/* RSA1 */

#endif /* __CONSTANTS_CRYPTO_H */
