TARGET=radio_interface

ifndef BUILD_DIR
	BUILD_DIR=$(CUR_DIR)
endif

CPP=g++

LDFLAGS=

.PHONY: all
all: $(BUILD_DIR)/$(TARGET).o

$(TARGET): $(TARGET).cpp transmitter_rule
	$(CPP) $(CFLAGS) -c $(TARGET).cpp -o $(BUILD_DIR)/$(TARGET).o

transmitter_rule: transmitter/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C transmitter

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o

