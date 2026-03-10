BUILD      = .build
DATA_DIR  := data
BASE_URL  := https://storage.googleapis.com/cvdf-datasets/mnist

IMAGES_GZ := $(DATA_DIR)/train-images-idx3-ubyte.gz
LABELS_GZ := $(DATA_DIR)/train-labels-idx1-ubyte.gz
IMAGES    := $(DATA_DIR)/train-images-idx3-ubyte
LABELS    := $(DATA_DIR)/train-labels-idx1-ubyte

.PHONY: download run knn nn clean

.DEFAULT_GOAL = run

download: $(IMAGES) $(LABELS)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(IMAGES_GZ): | $(DATA_DIR)
	curl -L -o $@ $(BASE_URL)/train-images-idx3-ubyte.gz

$(LABELS_GZ): | $(DATA_DIR)
	curl -L -o $@ $(BASE_URL)/train-labels-idx1-ubyte.gz

$(IMAGES): $(IMAGES_GZ)
	gunzip -k $<

$(LABELS): $(LABELS_GZ)
	gunzip -k $<

run: download
	cargo run -- view 0

knn: download
	cargo run --release -- knn

nn: download
	cargo run --release -- nn

clean:
	rm -rf $(DATA_DIR) ${BUILD}

fmt:
	cargo fmt
