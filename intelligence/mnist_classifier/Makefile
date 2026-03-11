DATA_DIR  := .build
BASE_URL  := https://storage.googleapis.com/cvdf-datasets/mnist

TRAIN_IMAGES_GZ := $(DATA_DIR)/train-images-idx3-ubyte.gz
TRAIN_LABELS_GZ := $(DATA_DIR)/train-labels-idx1-ubyte.gz
TEST_IMAGES_GZ  := $(DATA_DIR)/t10k-images-idx3-ubyte.gz
TEST_LABELS_GZ  := $(DATA_DIR)/t10k-labels-idx1-ubyte.gz

TRAIN_IMAGES := $(DATA_DIR)/train-images-idx3-ubyte
TRAIN_LABELS := $(DATA_DIR)/train-labels-idx1-ubyte
TEST_IMAGES  := $(DATA_DIR)/t10k-images-idx3-ubyte
TEST_LABELS  := $(DATA_DIR)/t10k-labels-idx1-ubyte

.PHONY: download run knn nn clean fmt

.DEFAULT_GOAL = run

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

run: download
	cargo run -- view 0

knn: download
	cargo run --release -- knn

nn: download
	cargo run --release -- nn

clean:
	rm -rf $(DATA_DIR)

fmt:
	cargo fmt
