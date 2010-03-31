## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libasn1.la

libasn1_la_SOURCES = \
	asn_application.h \
	asn_codecs.h \
	asn_codecs_prim.c \
	asn_codecs_prim.h \
	asn_internal.h \
	asn_SEQUENCE_OF.c \
	asn_SEQUENCE_OF.h \
	asn_SET_OF.c \
	asn_SET_OF.h \
	asn_system.h \
	ber_decoder.c \
	ber_decoder.h \
	ber_tlv_length.c \
	ber_tlv_length.h \
	ber_tlv_tag.c \
	ber_tlv_tag.h \
	BIT_STRING.c \
	BIT_STRING.h \
	constraints.c \
	constraints.h \
	constr_CHOICE.c \
	constr_CHOICE.h \
	constr_SEQUENCE.c \
	constr_SEQUENCE.h \
	constr_SEQUENCE_OF.c \
	constr_SEQUENCE_OF.h \
	constr_SET_OF.c \
	constr_SET_OF.h \
	constr_TYPE.c \
	constr_TYPE.h \
	ContextFlags.c \
	ContextFlags.h \
	der_encoder.c \
	der_encoder.h \
	GeneralString.c \
	GeneralString.h \
	INTEGER.c \
	INTEGER.h \
	MechType.c \
	MechType.h \
	MechTypeList.c \
	MechTypeList.h \
	NativeEnumerated.c \
	NativeEnumerated.h \
	NativeInteger.c \
	NativeInteger.h \
	NegHints.c \
	NegHints.h \
	NegoData.c \
	NegoData.h \
	NegotiationToken.c \
	NegotiationToken.h \
	NegTokenInit2.c \
	NegTokenInit2.h \
	NegTokenInit.c \
	NegTokenInit.h \
	NegTokenResp.c \
	NegTokenResp.h \
	OBJECT_IDENTIFIER.c \
	OBJECT_IDENTIFIER.h \
	OCTET_STRING.c \
	OCTET_STRING.h \
	per_decoder.c \
	per_decoder.h \
	per_encoder.c \
	per_encoder.h \
	per_opentype.c \
	per_opentype.h \
	per_support.c \
	per_support.h \
	TSCredentials.c \
	TSCredentials.h \
	TSCspDataDetail.c \
	TSCspDataDetail.h \
	TSPasswordCreds.c \
	TSPasswordCreds.h \
	TSRequest.c \
	TSRequest.h \
	TSSmartCardCreds.c \
	TSSmartCardCreds.h \
	xer_decoder.c \
	xer_decoder.h \
	xer_encoder.c \
	xer_encoder.h \
	xer_support.c \
	xer_support.h

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
