CFLAGS = -v -Wall -Werror

LIBS = -lcares -lcrypto -lssl -lmosquitto -lpthread -pthread

SOURCES = unwds-mqtt.c utils.c
BUILD_DIR = build/
CC = mips-openwrt-linux-gcc

ifndef V
	V = 0
endif

CC_0 = @$(CC)
CC_1 = $(CC)
CXX = $(CC_$(V))

OBJ :=${SOURCES:%.c=$(BUILD_DIR)%.o}

all: lora-mqtt

$(BUILD_DIR)%.o: %.c 
	@echo -e '\tCompiling $<'
	$(CXX) $(CFLAGS) $< -c -o $@ 

lora-mqtt: create_dirs $(OBJ) $(SOURCES)
	@echo -e '\tCompiling mqtt.c'
	$(CXX) $(CFLAGS) -c mqtt.c -o $(BUILD_DIR)mqtt.o
	@echo -e '\tLinking lora-mqtt'
	$(CXX) $(CFLAGS) $(OBJ) $(BUILD_DIR)mqtt.o $(LIBS) -o mqtt

create_dirs:
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
