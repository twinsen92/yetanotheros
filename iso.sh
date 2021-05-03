#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/yaos2.kernel isodir/boot/yaos2.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "yaos2" {
	multiboot /boot/yaos2.kernel
}
EOF
grub-mkrescue -o yaos2.iso isodir
