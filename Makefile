CFLAGS = -v -Wall -Werror -pg

LIBS = -lcares -lcrypto -lssl -lmosquitto -lpthread -pthread

SOURCES = unwds-mqtt.c utils.c
BUILD_DIR = build/

ifeq ($(ARCH),mips)
	#echo "Building for mips"
	STAGING_DIR=/home/openwrt/openwrt/staging_dir/
	CC = mips-openwrt-linux-gcc
	INCLUDE=/home/openwrt/openwrt/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include \
-I/home/openwrt/openwrt/build_dir/target-mips_34kc_uClibc-0.9.33.2/OpenWrt-Toolchain-ar71xx-generic_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-i686/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/include/
	LIBM=/home/openwrt/openwrt/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib
else
	CC = gcc
endif

ifndef V
	V = 0
endif

CC_0 = @$(CC)
CC_1 = $(CC)
CXX = $(CC_$(V))

OBJ :=${SOURCES:%.c=$(BUILD_DIR)%.o}

all: lora-mqtt-mips

$(BUILD_DIR)%.o: %.c 
	@echo -e '\tCompiling $<'
	$(CXX) $(CFLAGS) -L$(LIBM) -I$(INCLUDE) $< -c -o $@ 

lora-mqtt: create_dirs $(OBJ) $(SOURCES)
	@echo -e '\tCompiling mqtt.c'
	$(CXX) $(CFLAGS) -c mqtt.c -o $(BUILD_DIR)mqtt.o
	@echo -e '\tLinking lora-mqtt'
	$(CXX) $(CFLAGS) $(OBJ) $(BUILD_DIR)mqtt.o $(LIBS) -o lora-mqtt

lora-mqtt-mips: create_dirs $(OBJ) $(SOURCES)
	@echo -e '\tCompiling mqtt.c'
	$(CXX) $(CFLAGS) -L$(LIBM) -I$(INCLUDE) -c mqtt.c -o $(BUILD_DIR)mqtt.o
	@echo -e '\tLinking'
	$(CXX) $(CFLAGS) $(OBJ) $(BUILD_DIR)mqtt.o $(LIBS) -o lora-mqtt

create_dirs:
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
