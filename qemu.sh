#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom yaos2_multiboot.iso \
	-monitor stdio \
	-smp 2 \
	-D qemu.log \
#	-d int \
