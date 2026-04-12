package src

import (
	"container/heap"
	"math"
)

const (
	QuantChunks    = 16
	QuantCodeSize  = 256                    // fits in uint8
	QuantChunkSize = Pixels / QuantChunks   // = 49
)

// Classifier is the common interface for all classifiers.
type Classifier interface {
	Fit(images [][Pixels]float32, labels []uint8) error
	Predict(image [Pixels]float32) uint8
}

// KNNClassifier implements k-nearest neighbours with L1 distance.
// When QuantEnabled is true, Fit learns a product-quantization codebook
// (16 chunks × 256 centroids) via k-means and encodes every training image
// as [16]uint8.  Predict then uses Asymmetric Distance Computation (ADC),
// reducing the per-image scan cost from 784 float ops to 16 table lookups.
type KNNClassifier struct {
	K            int
	QuantEnabled bool

	trainImages [][Pixels]float32 // nil when QuantEnabled
	trainLabels []uint8

	// PQ fields — non-zero only when QuantEnabled.
	quantCodebook [QuantChunks][QuantCodeSize][QuantChunkSize]float32
	quantCodes    [][QuantChunks]uint8
}

// Fit stores the training data (plain) or learns PQ codebooks (quant).
func (c *KNNClassifier) Fit(images [][Pixels]float32, labels []uint8) error {
	c.trainLabels = make([]uint8, len(labels))
	copy(c.trainLabels, labels)

	if c.QuantEnabled {
		tmp := make([][Pixels]float32, len(images))
		copy(tmp, images)
		c.fitQuant(tmp)
		c.trainImages = nil // release ~180 MB; codebook + codes are kept
	} else {
		c.trainImages = make([][Pixels]float32, len(images))
		copy(c.trainImages, images)
	}
	return nil
}

// Predict returns the majority label among the k nearest neighbours.
func (c *KNNClassifier) Predict(image [Pixels]float32) uint8 {
	if c.QuantEnabled {
		return c.predictQuant(image)
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

// fitQuant learns PQ codebooks via k-means (15 iterations) and encodes
// all training images into c.quantCodes.
func (c *KNNClassifier) fitQuant(images [][Pixels]float32) {
	n := len(images)
	var r rng = 0x12345678

	// Initialise each codebook by random sampling from training sub-vectors.
	for m := 0; m < QuantChunks; m++ {
		for j := 0; j < QuantCodeSize; j++ {
			idx := int(r.next()) % n
			copy(c.quantCodebook[m][j][:], images[idx][m*QuantChunkSize:(m+1)*QuantChunkSize])
		}
	}

	type accumEntry struct {
		sum   [QuantChunkSize]float32
		count int
	}
	var accum [QuantChunks][QuantCodeSize]accumEntry

	for range 15 {
		// Reset accumulators.
		for m := range accum {
			for j := range accum[m] {
				accum[m][j] = accumEntry{}
			}
		}
		// Assignment + accumulation — single pass over images to keep each
		// 784-float row warm in cache while processing all 16 chunks.
		for _, img := range images {
			for m := 0; m < QuantChunks; m++ {
				sv := img[m*QuantChunkSize : (m+1)*QuantChunkSize]
				j := int(findNearestU8(sv, &c.quantCodebook[m]))
				accum[m][j].count++
				for d := 0; d < QuantChunkSize; d++ {
					accum[m][j].sum[d] += sv[d]
				}
			}
		}
		// Update centroids; re-seed any empty cluster.
		for m := 0; m < QuantChunks; m++ {
			for j := 0; j < QuantCodeSize; j++ {
				if accum[m][j].count == 0 {
					idx := int(r.next()) % n
					copy(c.quantCodebook[m][j][:], images[idx][m*QuantChunkSize:(m+1)*QuantChunkSize])
					continue
				}
				cnt := float32(accum[m][j].count)
				for d := 0; d < QuantChunkSize; d++ {
					c.quantCodebook[m][j][d] = accum[m][j].sum[d] / cnt
				}
			}
		}
	}

	// Encode all training images against the final codebook.
	c.quantCodes = make([][QuantChunks]uint8, n)
	for i, img := range images {
		for m := 0; m < QuantChunks; m++ {
			sv := img[m*QuantChunkSize : (m+1)*QuantChunkSize]
			c.quantCodes[i][m] = findNearestU8(sv, &c.quantCodebook[m])
		}
	}
}

// predictQuant uses Asymmetric Distance Computation (ADC).
// It precomputes distTable[chunk][centroid] once per query, then the scan
// over 60 K training images costs only 16 table lookups each.
// Safe for concurrent calls: only reads immutable post-Fit fields.
func (c *KNNClassifier) predictQuant(image [Pixels]float32) uint8 {
	// Build ADC table (~16 KB on stack).
	var distTable [QuantChunks][QuantCodeSize]float32
	for m := 0; m < QuantChunks; m++ {
		sv := image[m*QuantChunkSize : (m+1)*QuantChunkSize]
		for j := 0; j < QuantCodeSize; j++ {
			distTable[m][j] = l1SubVec(sv, c.quantCodebook[m][j][:])
		}
	}

	h := make(knnMaxHeap, 0, c.K)
	heap.Init(&h)
	for t, code := range c.quantCodes {
		var approxDist float32
		for m := 0; m < QuantChunks; m++ {
			approxDist += distTable[m][code[m]]
		}
		dBits := math.Float32bits(approxDist)
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

// l1SubVec computes the Manhattan distance between two equal-length slices.
func l1SubVec(a, b []float32) float32 {
	var s float32
	for i := range a {
		if d := a[i] - b[i]; d < 0 {
			s -= d
		} else {
			s += d
		}
	}
	return s
}

// findNearestU8 returns the index of the nearest centroid in codebook (L1).
func findNearestU8(sv []float32, codebook *[QuantCodeSize][QuantChunkSize]float32) uint8 {
	best := uint8(0)
	bestDist := l1SubVec(sv, codebook[0][:])
	for j := 1; j < QuantCodeSize; j++ {
		if d := l1SubVec(sv, codebook[j][:]); d < bestDist {
			bestDist = d
			best = uint8(j)
		}
	}
	return best
}

// knnEntry packs a float32 distance (as raw bits) and a label.
// Using IEEE 754 bit patterns allows integer comparison to stand in for
// float comparison for non-negative distances.
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
