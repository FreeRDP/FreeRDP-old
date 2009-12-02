/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   SPNEGO - Simple and Protected GSS-API Negotiation Mechanism

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

/*
 * The Simple and Protected Generic Security Service Application Program Interface (GSS-API)
 * Negotiation Mechanism is specified in RFC4178 (http://www.ietf.org/rfc/rfc4178.txt)
 *
 * Refer to [MS-SPNG].pdf for implementation details and protocol examples.
 */

/* NegTokenInit2 is sent by the client to the server after the TLS handshake is complete */
#if 0
void
spnego_send_neg_token_init2(STREAM s)
{
	/*
		SPNEGOASNOneSpec
		{
			iso(1) identified-organization(3) dod(6) internet(1)
			security(5) mechanism(5) snego (2) modules(4) spec2(2)
		}
		DEFINITIONS EXPLICIT TAGS ::= BEGIN
	 */

	/*
		NegHints ::= SEQUENCE {
			hintName[0] GeneralString OPTIONAL,
			hintAddress[1] OCTET STRING OPTIONAL
		}
		NegTokenInit2 ::= SEQUENCE {
			mechTypes[0] MechTypeList OPTIONAL,
			reqFlags [1] ContextFlags OPTIONAL,
			mechToken [2] OCTET STRING OPTIONAL,
			negHints [3] NegHints OPTIONAL,
			mechListMIC [4] OCTET STRING OPTIONAL,
			...
		}
	*/


}

/* NegTokenRsp is sent by the server as a response to NegTokenInit2 */

void
spnego_recv_neg_token_rsp(STREAM s)
{
	/*
	      NegTokenResp ::= SEQUENCE {
		  negState       [0] ENUMERATED {
		      accept-completed    (0),
		      accept-incomplete   (1),
		      reject              (2),
		      request-mic         (3)
		  }                                 OPTIONAL,
		    -- REQUIRED in the first reply from the target
		  supportedMech   [1] MechType      OPTIONAL,
		    -- present only in the first reply from the target
		  responseToken   [2] OCTET STRING  OPTIONAL,
		  mechListMIC     [3] OCTET STRING  OPTIONAL,
		  ...
	      }
	*/

}
#endif
