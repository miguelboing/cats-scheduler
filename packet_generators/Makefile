ifndef BUILD_DIR
	BUILD_DIR=$(CURDIR)
endif

.PHONY: all
all: fixed_rate_rule

fixed_rate_rule: fixed_rate/Makefile
	$(MAKE) BUILD_DIR=$(BUILD_DIR) -C fixed_rate

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o
