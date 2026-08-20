PKGS = gdk-pixbuf-2.0 x11 xscrnsaver
CFLAGS = $(shell pkg-config $(PKGS) --cflags)
LDFLAGS = $(shell pkg-config $(PKGS) --libs) -ldl
MYFLAGS = -fPIC -O2 -Wall

all: gdk-screenshotter.so

clean:
	rm -f *.o *.so test

%.so: %.o
	gcc -shared -fPIC -O2 -Wall $(LDFLAGS) $< -o $@

%.o: %.c
	gcc -c $(MYFLAGS) $(CFLAGS) $< -o $@

test: gdk-screenshotter.c
	gcc -DTEST -g $(CFLAGS) $(LDFLAGS) $< -o $@
