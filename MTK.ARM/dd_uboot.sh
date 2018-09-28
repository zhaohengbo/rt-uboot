UBOOT=u-boot.bin
DEVICE=/dev/sdg

sudo dd if=${UBOOT}  of=${DEVICE}  bs=1024 seek=320
sync

eject ${DEVICE}
