
CFLAGS = -O2 -w -I.
ASN1_MODULES = credssp.asn1 spnego.asn1

all: asn1_modules

asn1_modules:
	asn1c -fnative-types -fskeletons-copy $(ASN1_MODULES)
	rm converter-sample.c Makefile.am.sample
	$(CC) -c *.c $(CFLAGS)

clean:
	rm -f *.o *.c *.h


