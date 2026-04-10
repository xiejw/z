package src

import (
	"fmt"
	"math"
)

// NNClassifier implements a two-layer MLP: 784 → hidden (ReLU) → 10 (softmax).
// Training uses mini-batch SGD with cross-entropy loss.
type NNClassifier struct {
	Hidden int
	Lr     float64
	Epochs int
	Batch  int
	w1, b1 []float32 // first layer: w1[hidden×784], b1[hidden]
	w2, b2 []float32 // second layer: w2[10×hidden], b2[10]
}

// NewNNClassifier allocates an NNClassifier with the given hyperparameters.
func NewNNClassifier(hidden int, lr float64, epochs, batch int) *NNClassifier {
	return &NNClassifier{
		Hidden: hidden,
		Lr:     lr,
		Epochs: epochs,
		Batch:  batch,
		w1:     make([]float32, hidden*Pixels),
		b1:     make([]float32, hidden),
		w2:     make([]float32, Classes*hidden),
		b2:     make([]float32, Classes),
	}
}

// Fit trains the network on the provided data using mini-batch SGD.
func (c *NNClassifier) Fit(images [][Pixels]float32, labels []uint8) error {
	n := len(images)
	h := c.Hidden

	c.initWeights()

	idx := make([]int, n)
	for i := range idx {
		idx[i] = i
	}

	var r rng = 0xcafebabe

	for epoch := 0; epoch < c.Epochs; epoch++ {
		// Fisher-Yates shuffle.
		for i := n - 1; i > 0; i-- {
			j := int(r.next()) % (i + 1)
			idx[i], idx[j] = idx[j], idx[i]
		}

		var totalLoss float64

		for start := 0; start < n; start += c.Batch {
			b := c.Batch
			if start+b > n {
				b = n - start
			}
			chunk := idx[start : start+b]

			// Build batch matrices.
			// xBatch[b×784]: normalised pixel values.
			// yBatch[b×10]:  one-hot labels.
			xBatch := make([]float32, b*Pixels)
			yBatch := make([]float32, b*Classes)
			for bi, si := range chunk {
				copy(xBatch[bi*Pixels:], images[si][:])
				yBatch[bi*Classes+int(labels[si])] = 1.0
			}

			// Forward pass.
			// z1[b×h] = x[b×784] · W1^T[784×h] + b1
			z1 := matMulABt(xBatch, b, Pixels, c.w1, h)
			for i := 0; i < b; i++ {
				for j := 0; j < h; j++ {
					z1[i*h+j] += c.b1[j]
				}
			}

			// a1[b×h] = ReLU(z1)
			a1 := make([]float32, b*h)
			copy(a1, z1)
			for i, v := range a1 {
				if v < 0 {
					a1[i] = 0
				}
			}

			// z2[b×10] = a1[b×h] · W2^T[h×10] + b2
			z2 := matMulABt(a1, b, h, c.w2, Classes)
			for i := 0; i < b; i++ {
				for j := 0; j < Classes; j++ {
					z2[i*Classes+j] += c.b2[j]
				}
			}

			// a2[b×10] = softmax(z2) row-wise
			a2 := make([]float32, b*Classes)
			copy(a2, z2)
			for i := 0; i < b; i++ {
				row := a2[i*Classes : (i+1)*Classes]
				maxVal := row[0]
				for _, v := range row[1:] {
					if v > maxVal {
						maxVal = v
					}
				}
				var s float32
				for j := range row {
					row[j] = float32(math.Exp(float64(row[j] - maxVal)))
					s += row[j]
				}
				for j := range row {
					row[j] /= s
				}
			}

			// Accumulate cross-entropy loss.
			for i := 0; i < b; i++ {
				for j := 0; j < Classes; j++ {
					if yBatch[i*Classes+j] > 0.5 {
						p := float64(a2[i*Classes+j])
						if p < 1e-7 {
							p = 1e-7
						}
						totalLoss -= math.Log(p)
					}
				}
			}

			// Backward pass.
			// dz2[b×10] = (a2 − y) / b  (fused softmax + cross-entropy gradient)
			scale := float32(1.0 / float64(b))
			dz2 := make([]float32, b*Classes)
			for i := range dz2 {
				dz2[i] = (a2[i] - yBatch[i]) * scale
			}

			// dw2[10×h] = dz2^T[10×b] · a1[b×h]
			dw2 := matMulAtB(dz2, Classes, b, a1, h)

			// db2[10] = Σ_rows dz2
			db2 := make([]float32, Classes)
			for i := 0; i < b; i++ {
				for j := 0; j < Classes; j++ {
					db2[j] += dz2[i*Classes+j]
				}
			}

			// da1[b×h] = dz2[b×10] · W2[10×h]
			da1 := matMulAB(dz2, b, Classes, c.w2, h)

			// dz1[b×h] = da1 ⊙ ReLU'(z1)  (in-place; da1 becomes dz1)
			for i, v := range z1 {
				if v <= 0 {
					da1[i] = 0
				}
			}

			// dw1[h×784] = dz1^T[h×b] · x[b×784]
			dw1 := matMulAtB(da1, h, b, xBatch, Pixels)

			// db1[h] = Σ_rows dz1
			db1 := make([]float32, h)
			for i := 0; i < b; i++ {
				for j := 0; j < h; j++ {
					db1[j] += da1[i*h+j]
				}
			}

			// SGD update.
			lr := float32(c.Lr)
			for i := range c.w1 {
				c.w1[i] -= lr * dw1[i]
			}
			for i := range c.b1 {
				c.b1[i] -= lr * db1[i]
			}
			for i := range c.w2 {
				c.w2[i] -= lr * dw2[i]
			}
			for i := range c.b2 {
				c.b2[i] -= lr * db2[i]
			}
		}

		fmt.Printf("  epoch %d/%d: loss = %.4f\n", epoch+1, c.Epochs, totalLoss/float64(n))
	}
	return nil
}

// Predict returns the argmax class for a single image.
func (c *NNClassifier) Predict(image [Pixels]float32) uint8 {
	h := c.Hidden

	// a1[h] = ReLU(W1[h×784] · x[784] + b1)
	a1 := make([]float32, h)
	for i := 0; i < h; i++ {
		s := c.b1[i]
		for j := 0; j < Pixels; j++ {
			s += c.w1[i*Pixels+j] * image[j]
		}
		if s > 0 {
			a1[i] = s
		}
	}

	// z2[10] = W2[10×h] · a1[h] + b2; return argmax (no softmax needed)
	best := uint8(0)
	bestVal := c.b2[0]
	for j := 0; j < h; j++ {
		bestVal += c.w2[j] * a1[j]
	}
	for i := uint8(1); i < Classes; i++ {
		v := c.b2[i]
		for j := 0; j < h; j++ {
			v += c.w2[int(i)*h+j] * a1[j]
		}
		if v > bestVal {
			bestVal = v
			best = i
		}
	}
	return best
}

// initWeights applies He initialisation to w1 and w2; biases are left at zero.
func (c *NNClassifier) initWeights() {
	var r rng = 0xdeadbeef
	std1 := float32(math.Sqrt(2.0 / float64(Pixels)))
	std2 := float32(math.Sqrt(2.0 / float64(c.Hidden)))
	for i := range c.w1 {
		c.w1[i] = r.normal() * std1
	}
	for i := range c.w2 {
		c.w2[i] = r.normal() * std2
	}
}

// rng is a simple xorshift32 pseudo-random number generator.
type rng uint32

func (r *rng) next() uint32 {
	*r ^= *r << 13
	*r ^= *r >> 17
	*r ^= *r << 5
	return uint32(*r)
}

func (r *rng) uniform() float32 {
	return float32(r.next()>>8) / float32(1<<24)
}

// normal returns a standard-normal sample via Box-Muller.
func (r *rng) normal() float32 {
	u1 := r.uniform()
	if u1 < 1e-7 {
		u1 = 1e-7
	}
	u2 := r.uniform()
	return float32(math.Sqrt(float64(-2*math.Log(float64(u1))))) *
		float32(math.Cos(2*math.Pi*float64(u2)))
}

// matMulABt computes C = A * B^T where A is m×k, B is n×k, C is m×n.
func matMulABt(a []float32, m, k int, b []float32, n int) []float32 {
	c := make([]float32, m*n)
	for i := 0; i < m; i++ {
		for l := 0; l < k; l++ {
			ail := a[i*k+l]
			if ail == 0 {
				continue
			}
			for j := 0; j < n; j++ {
				c[i*n+j] += ail * b[j*k+l]
			}
		}
	}
	return c
}

// matMulAtB computes C = A^T * B where A is stored as k×m, B is k×n, C is m×n.
func matMulAtB(a []float32, m, k int, b []float32, n int) []float32 {
	c := make([]float32, m*n)
	for l := 0; l < k; l++ {
		for i := 0; i < m; i++ {
			ail := a[l*m+i]
			if ail == 0 {
				continue
			}
			for j := 0; j < n; j++ {
				c[i*n+j] += ail * b[l*n+j]
			}
		}
	}
	return c
}

// matMulAB computes C = A * B where A is m×k, B is k×n, C is m×n.
func matMulAB(a []float32, m, k int, b []float32, n int) []float32 {
	c := make([]float32, m*n)
	for i := 0; i < m; i++ {
		for l := 0; l < k; l++ {
			ail := a[i*k+l]
			if ail == 0 {
				continue
			}
			for j := 0; j < n; j++ {
				c[i*n+j] += ail * b[l*n+j]
			}
		}
	}
	return c
}
