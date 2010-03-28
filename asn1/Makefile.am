## Process this file with automake to produce Makefile.in

# libasn1
libasn1dir = $(PLUGIN_PATH)

libasn1_LTLIBRARIES = libasn1.la

libasn1_la_SOURCES = \
	asn_codecs_prim.c \
	asn_SEQUENCE_OF.c \
	asn_SET_OF.c \
	ber_decoder.c \
	ber_tlv_length.c \
	ber_tlv_tag.c \
	BIT_STRING.c \
	constraints.c \
	constr_CHOICE.c \
	constr_SEQUENCE.c \
	constr_SEQUENCE_OF.c \
	constr_SET_OF.c \
	constr_TYPE.c \
	ContextFlags.c \
	der_encoder.c \
	GeneralString.c \
	INTEGER.c \
	MechType.c \
	MechTypeList.c \
	NativeEnumerated.c \
	NativeInteger.c \
	NegHints.c \
	NegoData.c \
	NegotiationToken.c \
	NegTokenInit2.c \
	NegTokenInit.c \
	NegTokenResp.c \
	OBJECT_IDENTIFIER.c \
	OCTET_STRING.c \
	per_decoder.c \
	per_encoder.c \
	per_support.c \
	TSCredentials.c \
	TSCspDataDetail.c \
	TSPasswordCreds.c \
	TSRequest.c \
	TSSmartCardCreds.c \
	xer_decoder.c \
	xer_encoder.c \
	xer_support.c

libasn1_la_CFLAGS = -O2 -w -I.

libasn1_la_LDFLAGS = -avoid-version -module

libasn1_la_LIBADD =

# extra
EXTRA_DIST =

DISTCLEANFILES =

# CFLAGS = -O2 -w -I.
# ASN1_MODULES = credssp.asn1 spnego.asn1
#
# all: asn1_modules
#
# asn1_modules:
#	asn1c -fnative-types -fskeletons-copy -gen-PER $(ASN1_MODULES)
#	rm converter-sample.c Makefile.am.sample
#	$(CC) -c *.c $(CFLAGS)
#
# clean:
# 	rm -f *.o *.c *.h
