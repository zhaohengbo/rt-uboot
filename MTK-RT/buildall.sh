#bin/sh
make clean
make
rm -rf /root/ihex-master/uboot.bin
cp /root/u-boot-mt7688/uboot.img /root/ihex-master/
/root/ihex-master/bin2ihex -i /root/ihex-master/uboot.img -o /root/ihex-master/uboot.hex
rm -rf /media/sf_Shared/uboot.hex
cp /root/ihex-master/uboot.hex /media/sf_Shared/