
OBJS = rdpsnd_main.o

CFLAGS = -O2 -Wall -I../../include/freerdp

LDFLAGS = -lpthread

librdpsnd.so: $(OBJS)
	$(CC) -shared -o librdpsnd.so $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f librdpsnd.so
