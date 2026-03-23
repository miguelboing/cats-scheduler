TARGET=schedulers

ifndef BUILD_DIR
	BUILD_DIR=$(CUR_DIR)
endif

CPP=g++

LDFLAGS=

.PHONY: all
all: $(TARGET)

$(TARGET): earliest_deadline_first_rule smaller_period_first_rule channel_aware_smaller_period_first_rule

.PHONY: earliest_deadline_first_rule
earliest_deadline_first_rule: earliest_deadline_first/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C earliest_deadline_first

.PHONY: smaller_period_first_rule
smaller_period_first_rule: smaller_period_first/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C smaller_period_first

.PHONY: channel_aware_smaller_period_first_rule
channel_aware_smaller_period_first_rule: channel_aware_spf/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C channel_aware_spf

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o

