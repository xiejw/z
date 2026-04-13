package src

import (
	"fmt"
	"runtime"
	"sync"
	"sync/atomic"
	"time"
)

// RunEval predicts labels for all test images in parallel and prints
// overall accuracy and a per-class breakdown.
func RunEval(clf Classifier, images [][Pixels]float32, labels []uint8) {
	n := len(images)
	preds := make([]uint8, n)

	fmt.Printf("Evaluating on %d test samples...\n", n)
	t0 := time.Now()

	var done atomic.Int64
	stopCh := make(chan struct{})

	// Progress goroutine: refresh the terminal line every 200 ms.
	go func() {
		ticker := time.NewTicker(200 * time.Millisecond)
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				fmt.Printf("\r  [%d/%d]", done.Load(), n)
			case <-stopCh:
				return
			}
		}
	}()

	nCPU := runtime.NumCPU()
	chunkSize := (n + nCPU - 1) / nCPU
	var wg sync.WaitGroup

	for w := 0; w < nCPU; w++ {
		lo := w * chunkSize
		hi := lo + chunkSize
		if hi > n {
			hi = n
		}
		if lo >= n {
			break
		}
		wg.Add(1)
		go func(lo, hi int) {
			defer wg.Done()
			for i := lo; i < hi; i++ {
				preds[i] = clf.Predict(images[i])
				done.Add(1)
			}
		}(lo, hi)
	}

	wg.Wait()
	close(stopCh)

	elapsed := time.Since(t0)
	fmt.Printf("\r  prediction time: %.3f s (%.0f ms)\n",
		elapsed.Seconds(), float64(elapsed.Milliseconds()))

	// Compute per-class and overall accuracy.
	var correct int
	var classCorrect [Classes]int
	var classTotal [Classes]int
	for i, lbl := range labels {
		classTotal[lbl]++
		if preds[i] == lbl {
			correct++
			classCorrect[lbl]++
		}
	}

	fmt.Printf("\nAccuracy: %d/%d (%.2f%%)\n",
		correct, n, float64(correct)/float64(n)*100)
	fmt.Printf("\nPer-class breakdown:\n")
	for d := 0; d < Classes; d++ {
		fmt.Printf("  digit %d: %4d/%4d (%6.2f%%)\n",
			d, classCorrect[d], classTotal[d],
			float64(classCorrect[d])/float64(classTotal[d])*100)
	}
}
