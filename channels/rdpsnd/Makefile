
OBJS = rdpsnd_main.o

CFLAGS = -O2 -Wall -I../../include/freerdp

LDFLAGS =

librdpsnd.so: $(OBJS)
	$(CC) -shared -o librdpsnd.so $(OBJS)

clean:
	rm -f *.o
	rm -f librdpsnd.so
