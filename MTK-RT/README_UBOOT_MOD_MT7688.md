# u-boot-mt7688
#### uboot for Widora V1.0.7(with simple web)
***
# How to use
* 1.make menuconfig
* 2.select MT7628 board
* 3.make clean;make

### Note
* note:compile need java such as 1.7.0_79

# update list 1.0.7
* Press 'WPS' button when power on, then enter the failsafe web mode.
* change the sysfrq from 575Mhz to 580Mhz
# update list
* change bps to 115200,fix gpio39,40,41,42 low when startup
* add all gpio test,just press 'WPS' button with more than 7 seconds at power on
* web failsafe update mode,just press 'WPS' button with 2 to 7 seconds at power on
* web failsafe IP is 192.168.1.111
* DDR2 can be 64MB or 128MB,just select 512Mbit or 1024Mbit in menuconfig
***
* QQ:771992497
* mail:widora@qq.com
* twitter:https://twitter.com/widora_io
## Thanks to Manfeel、cleanwrt、Piotr Dymac、Adam Dunkels。
