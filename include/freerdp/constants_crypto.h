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

/* Negotation message types*/
#define TYPE_RDP_NEG_REQ	0x1
#define TYPE_RDP_NEG_RSP	0x2
#define TYPE_RDP_NEG_FAILURE	0x3

/* Protocol security flags */
#define PROTOCOL_RDP		0x00000000
#define PROTOCOL_SSL		0x00000001
#define PROTOCOL_HYBRID		0x00000002

/* Protocol security negotation failure reason flags */
#define SSL_REQUIRED_BY_SERVER		0x00000001
#define SSL_NOT_ALLOWED_BY_SERVER	0x00000002
#define SSL_CERT_NOT_ON_SERVER		0x00000003
#define INCONSISTENT_FLAGS		0x00000004
#define HYBRID_REQUIRED_BY_SERVER	0x00000005

/* RDP secure transport constants */
#define SEC_RANDOM_SIZE		32
#define SEC_MODULUS_SIZE	64
#define SEC_MAX_MODULUS_SIZE	256
#define SEC_PADDING_SIZE	8
#define SEC_EXPONENT_SIZE	4

#define SEC_CLIENT_RANDOM	0x0001
#define SEC_ENCRYPT		0x0008
#define SEC_LOGON_INFO		0x0040
#define SEC_LICENCE_NEG		0x0080
#define SEC_REDIRECTION_PKT	0x0400

#define SEC_TAG_SRV_INFO	0x0c01
#define SEC_TAG_SRV_CRYPT	0x0c02
#define SEC_TAG_SRV_CHANNELS	0x0c03

#define SEC_TAG_CLI_INFO	0xc001
#define SEC_TAG_CLI_CRYPT	0xc002
#define SEC_TAG_CLI_CHANNELS    0xc003
#define SEC_TAG_CLI_4           0xc004

#define SEC_TAG_PUBKEY		0x0006
#define SEC_TAG_KEYSIG		0x0008

#define SEC_RSA_MAGIC		0x31415352	/* RSA1 */

#endif /* __CONSTANTS_CRYPTO_H */
