
LIBOBJS = tcp.o iso.o mcs.o channels.o secure.o orders.o cache.o pstcache.o \
licence.o ssl.o mppc.o rdp.o rdp5.o bitmap.o freerdp.o capabilities.o rail.o spnego.o \
mem.o asn1.o

OBJS = xfreerdp.o xf_win.o xf_event.o xf_colour.o xf_keyboard.o

CFLAGS = -O2 -Wall -fPIC -I..
LDFLAGS = -L. -Wl,-rpath,. -lX11 -lXcursor -lfreerdp
#LDFLAGS = -L. -Wl,-rpath,. -lX11 -lfreerdp

all: libfreerdp.so xfreerdp

xfreerdp: $(OBJS)
	$(CC) -o xfreerdp $(OBJS) $(LDFLAGS)

clean:
	rm -f xfreerdp
	rm -f *.o
	rm -f libfreerdp.so

libfreerdp.so: $(LIBOBJS)
	$(CC) -shared -o libfreerdp.so $(LIBOBJS) -lcrypto -lssl

rdp.o: ../rdp.c
	$(CC) $(CFLAGS) -c ../rdp.c

tcp.o: ../tcp.c
	$(CC) $(CFLAGS) -c ../tcp.c

mcs.o: ../mcs.c
	$(CC) $(CFLAGS) -c ../mcs.c

secure.o: ../secure.c
	$(CC) $(CFLAGS) -c ../secure.c

iso.o: ../iso.c
	$(CC) $(CFLAGS) -c ../iso.c

orders.o: ../orders.c
	$(CC) $(CFLAGS) -c ../orders.c

cache.o: ../cache.c
	$(CC) $(CFLAGS) -c ../cache.c

pstcache.o: ../pstcache.c
	$(CC) $(CFLAGS) -c ../pstcache.c

licence.o: ../licence.c
	$(CC) $(CFLAGS) -c ../licence.c

ssl.o: ../ssl.c
	$(CC) $(CFLAGS) -c ../ssl.c

mppc.o: ../mppc.c
	$(CC) $(CFLAGS) -c ../mppc.c

rdp5.o: ../rdp5.c
	$(CC) $(CFLAGS) -c ../rdp5.c

bitmap.o: ../bitmap.c
	$(CC) $(CFLAGS) -c ../bitmap.c

channels.o: ../channels.c
	$(CC) $(CFLAGS) -c ../channels.c

freerdp.o: ../freerdp.c
	$(CC) $(CFLAGS) -c ../freerdp.c

capabilities.o: ../capabilities.c
	$(CC) $(CFLAGS) -c ../capabilities.c

rail.o: ../rail.c
	$(CC) $(CFLAGS) -c ../rail.c

mem.o: ../mem.c
	$(CC) $(CFLAGS) -c ../mem.c

spnego.o: ../spnego.c
	$(CC) $(CFLAGS) -c ../spnego.c

asn1.o: ../asn1.c
	$(CC) $(CFLAGS) -c ../asn1.c
