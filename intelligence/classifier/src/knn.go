package src

import (
	"container/heap"
	"math"
)

// Classifier is the common interface for all classifiers.
type Classifier interface {
	Fit(images [][Pixels]float32, labels []uint8) error
	Predict(image [Pixels]float32) uint8
}

// KNNClassifier implements k-nearest neighbours with L1 distance.
type KNNClassifier struct {
	K           int
	trainImages [][Pixels]float32
	trainLabels []uint8
}

// Fit stores a copy of the training data.
func (c *KNNClassifier) Fit(images [][Pixels]float32, labels []uint8) error {
	c.trainImages = make([][Pixels]float32, len(images))
	c.trainLabels = make([]uint8, len(labels))
	copy(c.trainImages, images)
	copy(c.trainLabels, labels)
	return nil
}

// Predict returns the majority label among the k nearest neighbours.
func (c *KNNClassifier) Predict(image [Pixels]float32) uint8 {
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

	var counts [Classes]int
	for _, e := range h {
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
