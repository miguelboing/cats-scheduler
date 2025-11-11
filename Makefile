CPP = g++
CFLAGS =-Wall -I$(CURDIR)

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = main

.PHONY: all
all: $(TARGET)

$(TARGET): $(TARGET).cpp buffer_packet fixed_rate edf_scheduler transmitter physical_channel receiver
	$(CPP) $(CFLAGS) $(TARGET).cpp buffer_packet.o fixed_rate.o physical_channel.o edf_scheduler.o transmitter.o receiver.o -o $(TARGET).o

.PHONY: fixed_rate
fixed_rate: packet_generators/fixed_rate/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C packet_generators/fixed_rate

.PHONY: edf_scheduler
edf_scheduler: schedulers/earliest_deadline_first/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C schedulers/earliest_deadline_first

.PHONY: physical_channel
physical_channel: physical_channel/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C physical_channel

.PHONY: buffer_packet
buffer_packet: system_model/buffer_packet/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/buffer_packet

.PHONY: transmitter
transmitter: system_model/transmitter/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/transmitter

.PHONY: receiver
receiver: system_model/receiver/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/receiver

clean:
	$(RM) *.o

