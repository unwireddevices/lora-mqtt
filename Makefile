#CFLAGS = -Wall -Werror -pg
CFLAGS = -Wall -pg

LIBS = -lcares -lcrypto -lssl -lmosquitto -lpthread -pthread

SOURCES = unwds-mqtt.c utils.c
BUILD_DIR = build/
CC = gcc
CXX = $(CC) $(CFLAGS)

OBJ :=${SOURCES:%.c=$(BUILD_DIR)%.o}

all: lora-mqtt

build/%.o: %.c 
	$(CXX) $< -c -o $@ 

lora-mqtt: $(OBJ) create_dirs $(SOURCES)
	$(CXX) -c mqtt.c -o $(BUILD_DIR)mqtt.o
	$(CXX) $(OBJ) $(BUILD_DIR)mqtt.o $(LIBS) -o lora-mqtt

create_dirs:
	mkdir -p $(BUILD_DIR)
