#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "usage: $0 <file> <volume name>"
    exit 1
fi

random-string() {
    cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w ${1:-16} | head -n 1
}

# Create the temporary directory.

TMP=/tmp/createdisk-$(random-string)
mkdir -p $TMP

# Create a disk image with a partition table.

dd if=/dev/zero of=$1 bs=1M count=128
echo "n
p
1


t
c
a
w
" | fdisk $1

LOOP_DEVICE=`sudo losetup --show -P -f $1`

# Format the first and only parition to VFAT. Then mount it.

sudo mkfs.fat -F 32 -n $2 $LOOP_DEVICE"p1"
mkdir -p $TMP/mounted
sudo mount $LOOP_DEVICE"p1" $TMP/mounted

# Install GRUB to the image.

echo "(hd0) $LOOP_DEVICE" > $TMP/device.map
sudo grub-install --no-floppy           \
    --grub-mkdevicemap=$TMP/device.map  \
    --root-directory=$TMP/mounted       \
    --target i386-pc                    \
    $LOOP_DEVICE

# Unmount the partition, then detach the loop device. Clean up.

sudo umount $LOOP_DEVICE"p1"
sudo losetup -d $LOOP_DEVICE
sudo rm -r $TMP

