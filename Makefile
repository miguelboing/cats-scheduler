CPP = g++
CFLAGS =-Wall -I$(CURDIR) -I$(CURDIR)/libs

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = main

.PHONY: all
all: $(TARGET)

$(TARGET): $(TARGET).cpp buffer_packet packet_gens schedulers transmitter physical_channels target_receiver
	$(CPP) $(CFLAGS) $(TARGET).cpp buffer_packet.o fixed_rate.o sigmoid_channel.o edf_scheduler.o transmitter.o target_receiver.o -o $(TARGET).o

.PHONY: packet_gens
packet_gens: packet_generators/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C packet_generators

.PHONY: schedulers
schedulers: schedulers/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C schedulers

.PHONY: physical_channels
physical_channels: physical_channels/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C physical_channels

.PHONY: buffer_packet
buffer_packet: system_model/buffer_packet/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/buffer_packet

.PHONY: transmitter
transmitter: system_model/transmitter/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/transmitter

.PHONY: target_receiver
target_receiver: system_model/target_receiver/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/target_receiver

clean:
	$(RM) *.o

