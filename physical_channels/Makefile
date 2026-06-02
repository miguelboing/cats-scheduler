ifndef BUILD_DIR
	BUILD_DIR=$(CURDIR)
endif

.PHONY: all
all:
	$(MAKE) BUILD_DIR=$(BUILD_DIR) -C sigmoid_channel
	$(MAKE) BUILD_DIR=$(BUILD_DIR) -C replay_channel

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o

