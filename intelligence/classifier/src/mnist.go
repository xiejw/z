package src

import (
	"encoding/binary"
	"fmt"
	"io"
	"os"
)

const (
	Pixels  = 784
	Rows    = 28
	Cols    = 28
	Classes = 10
)

var palette = [16]byte{
	' ', '.', ',', ':', ';', 'i', '1', 't',
	'f', 'L', 'C', 'G', '0', '8', '@', '#',
}

// LoadImages reads all images from an IDX3-ubyte file and returns them
// as float32 slices normalised to [0, 1].
func LoadImages(path string) ([][Pixels]float32, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, fmt.Errorf("open %s: %w", path, err)
	}
	defer f.Close()

	var magic, n, rows, cols uint32
	for _, p := range []*uint32{&magic, &n, &rows, &cols} {
		if err := binary.Read(f, binary.BigEndian, p); err != nil {
			return nil, fmt.Errorf("read header from %s: %w", path, err)
		}
	}
	if magic != 0x00000803 {
		return nil, fmt.Errorf("invalid magic 0x%08x in %s", magic, path)
	}
	if rows != Rows || cols != Cols {
		return nil, fmt.Errorf("unexpected dims %dx%d in %s", rows, cols, path)
	}

	raw := make([]byte, int(n)*Pixels)
	if _, err := io.ReadFull(f, raw); err != nil {
		return nil, fmt.Errorf("read pixels from %s: %w", path, err)
	}

	images := make([][Pixels]float32, n)
	for i := range images {
		for p := 0; p < Pixels; p++ {
			images[i][p] = float32(raw[i*Pixels+p]) / 255.0
		}
	}
	return images, nil
}

// LoadLabels reads all labels from an IDX1-ubyte file.
func LoadLabels(path string) ([]uint8, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, fmt.Errorf("open %s: %w", path, err)
	}
	defer f.Close()

	var magic, n uint32
	for _, p := range []*uint32{&magic, &n} {
		if err := binary.Read(f, binary.BigEndian, p); err != nil {
			return nil, fmt.Errorf("read header from %s: %w", path, err)
		}
	}
	if magic != 0x00000801 {
		return nil, fmt.Errorf("invalid magic 0x%08x in %s", magic, path)
	}

	labels := make([]uint8, n)
	if _, err := io.ReadFull(f, labels); err != nil {
		return nil, fmt.Errorf("read labels from %s: %w", path, err)
	}
	return labels, nil
}

// LoadOneImage reads a single image at the given index from an IDX3-ubyte file.
func LoadOneImage(path string, index int) ([Pixels]float32, error) {
	f, err := os.Open(path)
	if err != nil {
		return [Pixels]float32{}, fmt.Errorf("open %s: %w", path, err)
	}
	defer f.Close()

	var magic, n, rows, cols uint32
	for _, p := range []*uint32{&magic, &n, &rows, &cols} {
		if err := binary.Read(f, binary.BigEndian, p); err != nil {
			return [Pixels]float32{}, fmt.Errorf("read header from %s: %w", path, err)
		}
	}
	if magic != 0x00000803 {
		return [Pixels]float32{}, fmt.Errorf("invalid magic 0x%08x in %s", magic, path)
	}
	if index >= int(n) {
		return [Pixels]float32{}, fmt.Errorf("index %d out of range (max %d) in %s", index, int(n)-1, path)
	}

	if _, err := f.Seek(int64(16+index*Pixels), io.SeekStart); err != nil {
		return [Pixels]float32{}, fmt.Errorf("seek in %s: %w", path, err)
	}

	raw := make([]byte, Pixels)
	if _, err := io.ReadFull(f, raw); err != nil {
		return [Pixels]float32{}, fmt.Errorf("read from %s: %w", path, err)
	}

	var img [Pixels]float32
	for i, b := range raw {
		img[i] = float32(b) / 255.0
	}
	return img, nil
}

// LoadOneLabel reads a single label at the given index from an IDX1-ubyte file.
func LoadOneLabel(path string, index int) (uint8, error) {
	f, err := os.Open(path)
	if err != nil {
		return 0, fmt.Errorf("open %s: %w", path, err)
	}
	defer f.Close()

	var magic, n uint32
	for _, p := range []*uint32{&magic, &n} {
		if err := binary.Read(f, binary.BigEndian, p); err != nil {
			return 0, fmt.Errorf("read header from %s: %w", path, err)
		}
	}
	if magic != 0x00000801 {
		return 0, fmt.Errorf("invalid magic 0x%08x in %s", magic, path)
	}
	if index >= int(n) {
		return 0, fmt.Errorf("index %d out of range in %s", index, path)
	}

	if _, err := f.Seek(int64(8+index), io.SeekStart); err != nil {
		return 0, fmt.Errorf("seek in %s: %w", path, err)
	}

	var buf [1]byte
	if _, err := io.ReadFull(f, buf[:]); err != nil {
		return 0, fmt.Errorf("read from %s: %w", path, err)
	}
	return buf[0], nil
}

// RenderImage prints a 28×28 image as ASCII art to stdout.
// Each pixel is doubled horizontally to compensate for terminal aspect ratio.
func RenderImage(pixels [Pixels]float32) {
	for row := 0; row < Rows; row++ {
		for col := 0; col < Cols; col++ {
			p := pixels[row*Cols+col]
			idx := int(p*15.0 + 0.5)
			if idx < 0 {
				idx = 0
			}
			if idx > 15 {
				idx = 15
			}
			c := palette[idx]
			fmt.Printf("%c%c", c, c)
		}
		fmt.Println()
	}
}
