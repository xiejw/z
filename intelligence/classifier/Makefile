BUILD    := .build
BASE_URL := https://storage.googleapis.com/cvdf-datasets/mnist

K       ?= 5
HIDDEN  ?= 128
LR      ?= 0.1
EPOCHS  ?= 10
BATCH   ?= 64

TRAIN_IMAGES_GZ := $(BUILD)/train-images-idx3-ubyte.gz
TRAIN_LABELS_GZ := $(BUILD)/train-labels-idx1-ubyte.gz
TEST_IMAGES_GZ  := $(BUILD)/t10k-images-idx3-ubyte.gz
TEST_LABELS_GZ  := $(BUILD)/t10k-labels-idx1-ubyte.gz

TRAIN_IMAGES := $(BUILD)/train-images-idx3-ubyte
TRAIN_LABELS := $(BUILD)/train-labels-idx1-ubyte
TEST_IMAGES  := $(BUILD)/t10k-images-idx3-ubyte
TEST_LABELS  := $(BUILD)/t10k-labels-idx1-ubyte

.PHONY: all compile release test fmt download view knn nn clean

.DEFAULT_GOAL = compile

all: compile

compile: $(BUILD)/classifier
$(BUILD)/classifier: cmd/main.go src/*.go go.mod | $(BUILD)
	go build -o $@ ./cmd

$(BUILD):
	mkdir -p $(BUILD)

download: $(TRAIN_IMAGES) $(TRAIN_LABELS) $(TEST_IMAGES) $(TEST_LABELS)

$(TRAIN_IMAGES_GZ): | $(BUILD)
	curl -L -o $@ $(BASE_URL)/train-images-idx3-ubyte.gz

$(TRAIN_LABELS_GZ): | $(BUILD)
	curl -L -o $@ $(BASE_URL)/train-labels-idx1-ubyte.gz

$(TEST_IMAGES_GZ): | $(BUILD)
	curl -L -o $@ $(BASE_URL)/t10k-images-idx3-ubyte.gz

$(TEST_LABELS_GZ): | $(BUILD)
	curl -L -o $@ $(BASE_URL)/t10k-labels-idx1-ubyte.gz

$(TRAIN_IMAGES): $(TRAIN_IMAGES_GZ)
	gunzip -k $<

$(TRAIN_LABELS): $(TRAIN_LABELS_GZ)
	gunzip -k $<

$(TEST_IMAGES): $(TEST_IMAGES_GZ)
	gunzip -k $<

$(TEST_LABELS): $(TEST_LABELS_GZ)
	gunzip -k $<

view: compile download
	$(BUILD)/classifier view

knn: compile download
	$(BUILD)/classifier knn -k $(K)

nn: compile download
	$(BUILD)/classifier nn -hidden $(HIDDEN) -lr $(LR) -epochs $(EPOCHS) -batch $(BATCH)

release: compile

test: compile

fmt:
	go fmt classifier/...

clean:
	rm -rf $(BUILD)
