DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O0 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

ARCHDIR=arch/$(HOSTARCH)

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -DKERNEL_DEBUG -Iinclude -I$(ARCHDIR)/include
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lgcc

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
kernel/block/mbr.o \
kernel/block/registry.o \
kernel/cpu/checkpoint.o \
kernel/devices/ata_pio.o \
kernel/devices/pci.o \
kernel/drivers/ata/generic.o \
kernel/fs/fat_core.o \
kernel/fs/fat_vfs.o \
kernel/fs/fat.o \
kernel/test/kalloc_test.o \
kernel/vfs/core.o \
kernel/vfs/file.o \
kernel/exclusive_buffer.o \
kernel/kernel.o \
kernel/printf.o \

OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: yaos2_multiboot.kernel

yaos2_multiboot.kernel: $(OBJS) $(ARCHDIR)/multiboot.ld
	$(CC) -T $(ARCHDIR)/multiboot.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot yaos2_multiboot.kernel

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f yaos2_multiboot.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.
	cp -R --preserve=timestamps $(ARCHDIR)/include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: yaos2_multiboot.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp yaos2_multiboot.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
