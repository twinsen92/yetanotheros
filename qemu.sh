#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom yaos2_multiboot.iso \
	-monitor stdio \
#	-d int \
#	-D qemu.log
