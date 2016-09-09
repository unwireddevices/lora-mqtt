#!/bin/bash

INCLUDE=/home/openwrt/openwrt/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include
LIBM=/home/openwrt/openwrt/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib
#LIB=/home/openwrt/openwrt/build_dir/target-mips_34kc_uClibc-0.9.33.2/root-ar71xx/usr/lib/
#LIB=/home/openwrt/openwrt/build_dir/target-mips_34kc_uClibc-0.9.33.2/mosquitto-ssl/mosquitto-1.3.5/lib/
#LIBM=/home/openwrt/openwrt/build_dir/target-mips_34kc_uClibc-0.9.33.2/OpenWrt-SDK-ar71xx-for-redhat-i686-gcc-4.8-linaro_uClibc-0.9.33.2/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib
#LIBS=staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib
#$INCLUDE=$LIBM/../include

mips-openwrt-linux-gcc -L $LIBM -I $INCLUDE -o mqtt -lcares -lcrypto -lssl -lmosquitto -lpthread -pthread *.c
