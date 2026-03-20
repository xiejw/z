BUILD     := .build
DATA_DIR  := ${BUILD}

BASE_URL  := https://storage.googleapis.com/cvdf-datasets/mnist
TRAIN_IMAGES_GZ := $(DATA_DIR)/train-images-idx3-ubyte.gz
TRAIN_LABELS_GZ := $(DATA_DIR)/train-labels-idx1-ubyte.gz
TEST_IMAGES_GZ  := $(DATA_DIR)/t10k-images-idx3-ubyte.gz
TEST_LABELS_GZ  := $(DATA_DIR)/t10k-labels-idx1-ubyte.gz

TRAIN_IMAGES := $(DATA_DIR)/train-images-idx3-ubyte
TRAIN_LABELS := $(DATA_DIR)/train-labels-idx1-ubyte
TEST_IMAGES  := $(DATA_DIR)/t10k-images-idx3-ubyte
TEST_LABELS  := $(DATA_DIR)/t10k-labels-idx1-ubyte

CC     := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -DNDEBUG

.PHONY: download view knn nn compile clean fmt

.DEFAULT_GOAL = compile

download: $(TRAIN_IMAGES) $(TRAIN_LABELS) $(TEST_IMAGES) $(TEST_LABELS)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(TRAIN_IMAGES_GZ): | $(DATA_DIR)
	curl -L -o $@ $(BASE_URL)/train-images-idx3-ubyte.gz

$(TRAIN_LABELS_GZ): | $(DATA_DIR)
	curl -L -o $@ $(BASE_URL)/train-labels-idx1-ubyte.gz

$(TEST_IMAGES_GZ): | $(DATA_DIR)
	curl -L -o $@ $(BASE_URL)/t10k-images-idx3-ubyte.gz

$(TEST_LABELS_GZ): | $(DATA_DIR)
	curl -L -o $@ $(BASE_URL)/t10k-labels-idx1-ubyte.gz

$(TRAIN_IMAGES): $(TRAIN_IMAGES_GZ)
	gunzip -k $<

$(TRAIN_LABELS): $(TRAIN_LABELS_GZ)
	gunzip -k $<

$(TEST_IMAGES): $(TEST_IMAGES_GZ)
	gunzip -k $<

$(TEST_LABELS): $(TEST_LABELS_GZ)
	gunzip -k $<

clean:
	rm -rf ${BUILD} $(DATA_DIR)

fmt:
	~/Workspace/y/tools/scripts/clang_format_all.sh .

$(BUILD)/error.o: error.c error.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/hermes.o: hermes.c hermes.h error.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/par.o: par.c par.h error.h | $(BUILD)
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -c $< -o $@

$(BUILD)/main.o: main.c hermes.h par.h error.h | $(BUILD)
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -c $< -o $@

$(BUILD)/mnist: $(BUILD)/main.o $(BUILD)/hermes.o $(BUILD)/par.o $(BUILD)/error.o
	$(CC) $(CFLAGS) $^ -lm -lpthread -o $@

compile: $(BUILD)/mnist

view: $(BUILD)/mnist download
	$(BUILD)/mnist view

knn: $(BUILD)/mnist download
	$(BUILD)/mnist knn

nn: $(BUILD)/mnist download
	$(BUILD)/mnist nn
