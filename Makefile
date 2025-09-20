# The compiler
CPP = g++
CFLAGS = -g -Wall

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = main

.PHONY: all
all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CPP) $(CFLAGS) -o $(TARGET) $(TARGET.c)

clean:
	$(RM) $(TARGET)

.PHONY: edf_scheduler
edf_scheduler: earliest_deadline_first/Makefile
	$(MAKE) BUILD_DIR=$(BUILD_DIR) -C can
