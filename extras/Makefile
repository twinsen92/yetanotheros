DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

PREFIX=../sysroot
DESTDIR=$(PREFIX)/usr/bin
EXEC_PREFIX=$(PREFIX)
BOOTDIR=$(EXEC_PREFIX)/boot
INCLUDEDIR=$(PREFIX)/usr/include
LIBDIR=$(PREFIX)/usr/lib

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra --sysroot=$(PREFIX) -I$(INCLUDEDIR)
CPPFLAGS:=$(CPPFLAGS)
LDFLAGS:=$(LDFLAGS) -L$(LIBDIR)
LIBS:=$(LIBS) -nostdlib -lc -lgcc

SHARED_OBJS=\
crt0.o \

HELLO_OBJS=\
$(SHARED_OBJS) \
hello.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: hello

hello: $(HELLO_OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(HELLO_OBJS) $(LIBS)

.c.o:
	$(CC) -fPIC -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -fPIC -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f hello
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-sysroot

install-sysroot: hello
	mkdir -p $(DESTDIR)
	cp hello $(DESTDIR)

-include $(OBJS:.o=.d)
