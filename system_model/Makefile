CPP = g++
CFLAGS =-Wall -I$(CURDIR) -I$(CURDIR)/libs

ifndef $(BUILD_DIR)
	BUILD_DIR=$(CURDIR)
endif

TARGET = system_model

.PHONY: $(TARGET)
$(TARGET): buffer_packet_rule radio_interface_rule target_receiver_rule ml_predictor_rule

.PHONY: buffer_packet_rule
buffer_packet_rule: buffer_packet/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C buffer_packet

.PHONY: radio_interface_rule
radio_interface_rule: radio_interface/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C radio_interface

.PHONY: target_receiver_rule
target_receiver_rule: target_receiver/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C target_receiver

.PHONY: ml_predictor_rule
ml_predictor_rule: ml_predictor/Makefile
	$(MAKE) CFLAGS="$(CFLAGS)" BUILD_DIR=$(BUILD_DIR) -C ml_predictor

clean:
	$(RM) *.o

