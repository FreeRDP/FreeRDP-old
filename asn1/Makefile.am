## Process this file with automake to produce Makefile.in

# libasn1
libasn1dir = $(PLUGIN_PATH)

libasn1_LTLIBRARIES = libasn1.la

libasn1_la_SOURCES = \
	asn_application.h asn_codecs.h asn_internal.h asn_system.h \
	asn_codecs_prim.h asn_codecs_prim.c \
	asn_SEQUENCE_OF.h asn_SEQUENCE_OF.c \
	asn_SET_OF.h asn_SET_OF.c \
	ber_decoder.h ber_decoder.c \
	ber_tlv_length.h ber_tlv_length.c \
	ber_tlv_tag.h ber_tlv_tag.c \
	BIT_STRING.h BIT_STRING.c \
	constraints.h constraints.c \
	constr_CHOICE.h constr_CHOICE.c \
	constr_SEQUENCE.h constr_SEQUENCE.c \
	constr_SEQUENCE_OF.h constr_SEQUENCE_OF.c \
	constr_SET_OF.h constr_SET_OF.c \
	constr_TYPE.h constr_TYPE.c \
	ContextFlags.h ContextFlags.c \
	der_encoder.h der_encoder.c \
	GeneralString.h GeneralString.c \
	INTEGER.h INTEGER.c \
	MechType.h MechType.c \
	MechTypeList.h MechTypeList.c \
	NativeEnumerated.h NativeEnumerated.c \
	NativeInteger.h NativeInteger.c \
	NegHints.h NegHints.c \
	NegoData.h NegoData.c \
	NegotiationToken.h NegotiationToken.c \
	NegTokenInit2.h NegTokenInit2.c \
	NegTokenInit.h NegTokenInit.c \
	NegTokenResp.h NegTokenResp.c \
	OBJECT_IDENTIFIER.h OBJECT_IDENTIFIER.c \
	OCTET_STRING.h OCTET_STRING.c \
	per_decoder.h per_decoder.c \
	per_encoder.h per_encoder.c \
	per_support.h per_support.c \
	TSCredentials.h TSCredentials.c \
	TSCspDataDetail.h TSCspDataDetail.c \
	TSPasswordCreds.h TSPasswordCreds.c \
	TSRequest.h TSRequest.c \
	TSSmartCardCreds.h TSSmartCardCreds.c \
	xer_decoder.h xer_decoder.c \
	xer_encoder.h xer_encoder.c \
	xer_support.h xer_support.c

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
