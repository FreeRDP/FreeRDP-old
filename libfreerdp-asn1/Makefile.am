## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libfreerdp-asn1.la

shared_sources = \
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
	der_encoder.c \
	der_encoder.h \
	GeneralString.c \
	GeneralString.h \
	INTEGER.c \
	INTEGER.h \
	NativeEnumerated.c \
	NativeEnumerated.h \
	NativeInteger.c \
	NativeInteger.h \
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
	xer_decoder.c \
	xer_decoder.h \
	xer_encoder.c \
	xer_encoder.h \
	xer_support.c \
	xer_support.h

credssp_sources = \
	NegoData.c \
	NegoData.h \
	NegoDataItem.c \
	NegoDataItem.h \
	TSCredentials.c \
	TSCredentials.h \
	TSCspDataDetail.c \
	TSCspDataDetail.h \
	TSPasswordCreds.c \
	TSPasswordCreds.h \
	TSRequest.c \
	TSRequest.h \
	TSSmartCardCreds.c \
	TSSmartCardCreds.h

spnego_sources = \
	ContextFlags.c \
	ContextFlags.h \
	MechType.c \
	MechType.h \
	MechTypeList.c \
	MechTypeList.h \
	NegHints.c \
	NegHints.h \
	NegotiationToken.c \
	NegotiationToken.h \
	NegTokenInit2.c \
	NegTokenInit2.h \
	NegTokenInit.c \
	NegTokenInit.h \
	NegTokenResp.c \
	NegTokenResp.h

mcs_sources = \
	AssignedChannelId.c \
	AssignedChannelId.h \
	AttachUserConfirm.c \
	AttachUserConfirm.h \
	AttachUserRequest.c \
	AttachUserRequest.h \
	BOOLEAN.c \
	BOOLEAN.h \
	CapabilitiesNotificationIndication.c \
	CapabilitiesNotificationIndication.h \
	CapabilitiesNotificationRequest.c \
	CapabilitiesNotificationRequest.h \
	CapabilityClass.c \
	CapabilityClass.h \
	CapabilityID.c \
	CapabilityID.h \
	ChannelAdmitIndication.c \
	ChannelAdmitIndication.h \
	ChannelAdmitRequest.c \
	ChannelAdmitRequest.h \
	ChannelAttributes.c \
	ChannelAttributes.h \
	ChannelConveneConfirm.c \
	ChannelConveneConfirm.h \
	ChannelConveneRequest.c \
	ChannelConveneRequest.h \
	ChannelDisbandIndication.c \
	ChannelDisbandIndication.h \
	ChannelDisbandRequest.c \
	ChannelDisbandRequest.h \
	ChannelExpelIndication.c \
	ChannelExpelIndication.h \
	ChannelExpelRequest.c \
	ChannelExpelRequest.h \
	ChannelId.c \
	ChannelId.h \
	ChannelJoinConfirm.c \
	ChannelJoinConfirm.h \
	ChannelJoinRequest.c \
	ChannelJoinRequest.h \
	ChannelLeaveRequest.c \
	ChannelLeaveRequest.h \
	DataPriority.c \
	DataPriority.h \
	DetachUserIndication.c \
	DetachUserIndication.h \
	DetachUserRequest.c \
	DetachUserRequest.h \
	Diagnostic.c \
	Diagnostic.h \
	DisconnectProviderUltimatum.c \
	DisconnectProviderUltimatum.h \
	DomainMCSPDU.c \
	DomainMCSPDU.h \
	DynamicChannelId.c \
	DynamicChannelId.h \
	ErectDomainRequest.c \
	ErectDomainRequest.h \
	ExtendedParameterAccept.c \
	ExtendedParameterAccept.h \
	ExtendedParameterMCSPDU.c \
	ExtendedParameterMCSPDU.h \
	ExtendedParameterPropose.c \
	ExtendedParameterPropose.h \
	ExtendedParameters.c \
	ExtendedParameters.h \
	H221NonStandardIdentifier.c \
	H221NonStandardIdentifier.h \
	IndicationCapability.c \
	IndicationCapability.h \
	Key.c \
	Key.h \
	MergeChannelsConfirm.c \
	MergeChannelsConfirm.h \
	MergeChannelsRequest.c \
	MergeChannelsRequest.h \
	MergeTokensConfirm.c \
	MergeTokensConfirm.h \
	MergeTokensRequest.c \
	MergeTokensRequest.h \
	NonStandardParameter.c \
	NonStandardParameter.h \
	NonStandardPDU.c \
	NonStandardPDU.h \
	NULL.c \
	NULL.h \
	ParticipationIndicator.c \
	ParticipationIndicator.h \
	PlumbDomainIndication.c \
	PlumbDomainIndication.h \
	PrivateChannelId.c \
	PrivateChannelId.h \
	PurgeChannelsIndication.c \
	PurgeChannelsIndication.h \
	PurgeTokensIndication.c \
	PurgeTokensIndication.h \
	Reason.c \
	Reason.h \
	RejectMCSPDUUltimatum.c \
	RejectMCSPDUUltimatum.h \
	RequestCapability.c \
	RequestCapability.h \
	Result.c \
	Result.h \
	Segmentation.c \
	Segmentation.h \
	SendDataIndication.c \
	SendDataIndication.h \
	SendDataRequest.c \
	SendDataRequest.h \
	SeqOfIndicationCapabilities.c \
	SeqOfIndicationCapabilities.h \
	SeqOfRequestCapabilities.c \
	SeqOfRequestCapabilities.h \
	StaticChannelId.c \
	StaticChannelId.h \
	TokenAttributes.c \
	TokenAttributes.h \
	TokenGiveConfirm.c \
	TokenGiveConfirm.h \
	TokenGiveIndication.c \
	TokenGiveIndication.h \
	TokenGiveRequest.c \
	TokenGiveRequest.h \
	TokenGiveResponse.c \
	TokenGiveResponse.h \
	TokenGrabConfirm.c \
	TokenGrabConfirm.h \
	TokenGrabRequest.c \
	TokenGrabRequest.h \
	TokenId.c \
	TokenId.h \
	TokenInhibitConfirm.c \
	TokenInhibitConfirm.h \
	TokenInhibitRequest.c \
	TokenInhibitRequest.h \
	TokenPleaseIndication.c \
	TokenPleaseIndication.h \
	TokenPleaseRequest.c \
	TokenPleaseRequest.h \
	TokenReleaseConfirm.c \
	TokenReleaseConfirm.h \
	TokenReleaseRequest.c \
	TokenReleaseRequest.h \
	TokenStatus.c \
	TokenStatus.h \
	TokenTestConfirm.c \
	TokenTestConfirm.h \
	TokenTestRequest.c \
	TokenTestRequest.h \
	UniformSendDataIndication.c \
	UniformSendDataIndication.h \
	UniformSendDataRequest.c \
	UniformSendDataRequest.h \
	UserId.c \
	UserId.h

libfreerdp_asn1_la_SOURCES = \
	$(shared_sources) \
	$(credssp_sources) \
	$(spnego_sources)
#	$(mcs_sources)

libfreerdp_asn1_la_CFLAGS = -O2 -w

libfreerdp_asn1_la_LDFLAGS = -avoid-version -module

libfreerdp_asn1_la_LIBADD =

ASN1_MODULES_BER = credssp.asn1 spnego.asn1
ASN1_MODULES_PER = mcs.asn1
ASN1_MODULES = $(ASN1_MODULES_BER) $(ASN1_MODULES_PER)

asn1_modules:
	rm -f *.c *.h
# Note: extra redundant -fskeletons-copy parameter given to asn1c because it forgets one
	asn1c -fnative-types -fskeletons-copy -fcompound-names -fskeletons-copy $(ASN1_MODULES_BER)
#	asn1c -fnative-types -fskeletons-copy -fcompound-names -gen-PER -fskeletons-copy $(ASN1_MODULES_PER)
	for h in *.h; do sed -i "s,#include <$$h>,#include \"$$h\",g" *.[ch]; done
	rm converter-sample.c Makefile.am.sample

EXTRA_DIST = $(ASN1_MODULES)

DISTCLEANFILES =
