TARGET=schedulers

ifndef BUILD_DIR
	BUILD_DIR=$(CUR_DIR)
endif

CPP=g++

LDFLAGS=

.PHONY: all
all: $(TARGET)

$(TARGET): earliest_deadline_first_rule

.PHONY: earliest_deadline_first_rule
earliest_deadline_first_rule: earliest_deadline_first/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C earliest_deadline_first

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o

