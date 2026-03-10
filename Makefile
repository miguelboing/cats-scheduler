CPP = g++
CFLAGS =-Wall -I$(CURDIR) -I$(CURDIR)/libs

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = main

.PHONY: all
all: $(TARGET)

$(TARGET): $(TARGET).cpp system_model_rule packet_gens_rule schedulers_rule physical_channels_rule
	$(CPP) $(CFLAGS) $(TARGET).cpp $(BUILD_DIR)/*.o -o $(TARGET).o

.PHONY: system_model_rule
system_model_rule: system_model/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C system_model

.PHONY: packet_gens_rule
packet_gens_rule: packet_generators/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C packet_generators

.PHONY: schedulers_rule
schedulers_rule: schedulers/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C schedulers

.PHONY: physical_channels_rule
physical_channels_rule: physical_channels/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C physical_channels

clean:
	$(RM) *.o

