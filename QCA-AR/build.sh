#bin/sh
export BUILD_TOPDIR=$PWD
export STAGING_DIR=$BUILD_TOPDIR/tmp
export MAKECMD="make --silent ARCH=mips CROSS_COMPILE=mips-openwrt-linux-"
export PATH=$BUILD_TOPDIR/../../openwrt-sdk-18.06.1-ar71xx-generic_gcc-7.3.0_musl.Linux-x86_64/staging_dir/toolchain-mips_24kc_gcc-7.3.0_musl/bin:$PATH

make clean
#make tp-link_tl-wr810n_v2
make clean
