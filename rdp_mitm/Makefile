
ASN1_PATH = ../asn1
CFLAGS = -O2 -w -I$(ASN1_PATH)
LDFLAGS = -lssl
OBJS = rdp_mitm.o $(ASN1_PATH)/*.o

all: rdp_mitm

rdp_mitm:
	$(CC) -c rdp_mitm.c $(CFLAGS) $(LDFLAGS)
	$(CC) -o rdp_mitm $(OBJS) $(LDFLAGS)

clean:
	rm -f rdp_mitm
	rm -f *.o


