#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) \
	-boot d \
	-cdrom yaos2_multiboot.iso \
	-drive file=$HD_IMAGE,format=raw,media=disk,if=ide,bus=0,unit=0 \
	-serial file:com.log \
	-monitor stdio \
	-smp 2 \
	-D qemu.log \
#	-d int \
