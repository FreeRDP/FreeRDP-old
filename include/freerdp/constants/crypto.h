/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP Cryptographic Constants

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __CONSTANTS_CRYPTO_H
#define __CONSTANTS_CRYPTO_H

/* RDP Negotiation Protocols */
enum RDP_NEG_PROTOCOLS
{
	PROTOCOL_RDP = 0x00000000,
	PROTOCOL_TLS = 0x00000001,
	PROTOCOL_NLA = 0x00000002
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
