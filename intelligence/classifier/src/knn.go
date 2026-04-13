package src

import (
	"container/heap"
	"math"
	"math/bits"
)

const (
	BitsWords = (Pixels + 63) / 64 // = 13; pack 784 bits into 13 uint64 words
)

// Classifier is the common interface for all classifiers.
type Classifier interface {
	Fit(images [][Pixels]float32, labels []uint8) error
	Predict(image [Pixels]float32) uint8
}

// KNNClassifier implements k-nearest neighbours with L1 distance.
// When BitQuantEnabled is true, each pixel is binarized at threshold 128/255
// and packed into [BitsWords]uint64; Predict uses Hamming distance (XOR+popcount).
type KNNClassifier struct {
	K               int
	BitQuantEnabled bool

	trainImages [][Pixels]float32 // nil when BitQuantEnabled
	trainLabels []uint8

	// 1-bit quant fields — non-zero only when BitQuantEnabled.
	bitCodes [][BitsWords]uint64
}

// Fit stores the training data (plain) or binarizes it (bquant).
func (c *KNNClassifier) Fit(images [][Pixels]float32, labels []uint8) error {
	c.trainLabels = make([]uint8, len(labels))
	copy(c.trainLabels, labels)

	if c.BitQuantEnabled {
		c.fitBitQuant(images)
		c.trainImages = nil
	} else {
		c.trainImages = make([][Pixels]float32, len(images))
		copy(c.trainImages, images)
	}
	return nil
}

// Predict returns the majority label among the k nearest neighbours.
func (c *KNNClassifier) Predict(image [Pixels]float32) uint8 {
	if c.BitQuantEnabled {
		return c.predictBitQuant(image)
	}

	h := make(knnMaxHeap, 0, c.K)
	heap.Init(&h)

	for t, train := range c.trainImages {
		d := l1Distance(image, train)
		dBits := math.Float32bits(d)
		e := knnEntry{distBits: dBits, label: c.trainLabels[t]}
		if h.Len() < c.K {
			heap.Push(&h, e)
		} else if dBits < h[0].distBits {
			heap.Pop(&h)
			heap.Push(&h, e)
		}
	}

	return knnMajority(&h)
}

// binarize converts a float32 image to a packed bit array.
// Each pixel is thresholded at 128/255 ≈ 0.502: bit 1 if >= threshold, else 0.
func binarize(image [Pixels]float32) [BitsWords]uint64 {
	const threshold = 128.0 / 255.0
	var w [BitsWords]uint64
	for i, v := range image {
		if v >= threshold {
			w[i/64] |= 1 << uint(i%64)
		}
	}
	return w
}

// fitBitQuant binarizes and packs all training images into c.bitCodes.
// No training loop — O(n) pass over images.
func (c *KNNClassifier) fitBitQuant(images [][Pixels]float32) {
	c.bitCodes = make([][BitsWords]uint64, len(images))
	for i, img := range images {
		c.bitCodes[i] = binarize(img)
	}
}

// predictBitQuant computes Hamming distance from a binarized query to every
// training image and returns the majority label among the K nearest.
func (c *KNNClassifier) predictBitQuant(image [Pixels]float32) uint8 {
	q := binarize(image)

	h := make(knnMaxHeap, 0, c.K)
	heap.Init(&h)

	for t, code := range c.bitCodes {
		var dist uint32
		for w := 0; w < BitsWords; w++ {
			dist += uint32(bits.OnesCount64(q[w] ^ code[w]))
		}
		e := knnEntry{distBits: dist, label: c.trainLabels[t]}
		if h.Len() < c.K {
			heap.Push(&h, e)
		} else if dist < h[0].distBits {
			heap.Pop(&h)
			heap.Push(&h, e)
		}
	}

	return knnMajority(&h)
}

// knnMajority returns the most common label in the heap.
func knnMajority(h *knnMaxHeap) uint8 {
	var counts [Classes]int
	for _, e := range *h {
		counts[e.label]++
	}
	best := uint8(0)
	for i := uint8(1); i < Classes; i++ {
		if counts[i] > counts[best] {
			best = i
		}
	}
	return best
}

// l1Distance computes the Manhattan distance between two images.
func l1Distance(a, b [Pixels]float32) float32 {
	var s float32
	for i := 0; i < Pixels; i++ {
		d := a[i] - b[i]
		if d < 0 {
			s -= d
		} else {
			s += d
		}
	}
	return s
}

// knnEntry packs a distance and a label.
type knnEntry struct {
	distBits uint32
	label    uint8
}

// knnMaxHeap is a max-heap of knnEntry values, ordered by distance.
type knnMaxHeap []knnEntry

func (h knnMaxHeap) Len() int           { return len(h) }
func (h knnMaxHeap) Less(i, j int) bool { return h[i].distBits > h[j].distBits }
func (h knnMaxHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }

func (h *knnMaxHeap) Push(x any) { *h = append(*h, x.(knnEntry)) }
func (h *knnMaxHeap) Pop() any {
	old := *h
	n := len(old)
	x := old[n-1]
	*h = old[:n-1]
	return x
}
