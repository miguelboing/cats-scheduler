# The compiler
CPP = g++
CFLAGS =-Wall -I$(CURDIR)

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = main

.PHONY: all
all: $(TARGET)

$(TARGET): $(TARGET).cpp edf_scheduler
	$(CPP) $(CFLAGS) $(TARGET).cpp edf_scheduler.o -o $(TARGET)

clean:
	$(RM) $(TARGET)

.PHONY: edf_scheduler
edf_scheduler: schedulers/earliest_deadline_first/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C schedulers/earliest_deadline_first

