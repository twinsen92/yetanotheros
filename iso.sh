#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/yaos2_multiboot.kernel isodir/boot/yaos2_multiboot.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "yaos2_multiboot" {
	multiboot /boot/yaos2_multiboot.kernel
}
EOF
grub-mkrescue -o yaos2_multiboot.iso isodir

cp -rf sysroot/* $HD_MOUNT
