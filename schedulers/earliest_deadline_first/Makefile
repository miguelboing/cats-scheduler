TARGET=edf

ifndef BUILD_DIR
	BUILD_DIR=$(CUR_DIR)
endif

CPP=g++

CFLAGS=

LDFLAGS=

.PHONY: all
all:
	$(CPP) $(CFLAGS) $(TARGET).c -o $(BUILD_DIR)/$(TARGET).o

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o
