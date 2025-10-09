CPP = g++
CFLAGS =-Wall -I$(CURDIR)

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = main

.PHONY: all
all: $(TARGET)

$(TARGET): $(TARGET).cpp edf_scheduler physical_channel receiver
	$(CPP) $(CFLAGS) $(TARGET).cpp physical_channel.o edf_scheduler.o receiver.o -o $(TARGET).o

.PHONY: edf_scheduler
edf_scheduler: schedulers/earliest_deadline_first/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C schedulers/earliest_deadline_first

.PHONY: physical_channel
physical_channel: system_model/physical_channel/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/physical_channel

.PHONY: receiver
receiver: system_model/receiver/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model/receiver

clean:
	$(RM) *.o

