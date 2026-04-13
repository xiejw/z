// classifier — MNIST digit classifier
//
// Usage:
//
//	classifier view [<index>]
//	classifier knn  [-k <int>]
//	classifier nn   [-hidden <int>] [-lr <float>] [-epochs <int>] [-batch <int>]
//
// Run `make download` once to fetch the MNIST data files into .build/.
package main

import (
	classifier "classifier/src"
	"flag"
	"fmt"
	"log"
	"os"
	"time"
)

const (
	trainImages = ".build/train-images-idx3-ubyte"
	trainLabels = ".build/train-labels-idx1-ubyte"
	testImages  = ".build/t10k-images-idx3-ubyte"
	testLabels  = ".build/t10k-labels-idx1-ubyte"
)

func main() {
	if len(os.Args) < 2 {
		usage()
		os.Exit(1)
	}
	sub := os.Args[1]
	args := os.Args[2:]

	switch sub {
	case "view":
		runView(args)
	case "knn":
		runKNN(args)
	case "nn":
		runNN(args)
	default:
		fmt.Fprintf(os.Stderr, "unknown subcommand: %s\n", sub)
		usage()
		os.Exit(1)
	}
}

func usage() {
	fmt.Fprintln(os.Stderr, "Usage:")
	fmt.Fprintln(os.Stderr, "  classifier view [<index>]")
	fmt.Fprintln(os.Stderr, "  classifier knn  [-k <int>] [-bquant]")
	fmt.Fprintln(os.Stderr, "  classifier nn   [-hidden <int>] [-lr <float>] [-epochs <int>] [-batch <int>]")
}

func runView(args []string) {
	fs := flag.NewFlagSet("view", flag.ExitOnError)
	fs.Parse(args)

	index := 0
	if fs.NArg() > 0 {
		fmt.Sscan(fs.Arg(0), &index)
	}

	label, err := classifier.LoadOneLabel(trainLabels, index)
	if err != nil {
		log.Fatal(err)
	}
	img, err := classifier.LoadOneImage(trainImages, index)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("Label: %d  (sample index %d)\n", label, index)
	classifier.RenderImage(img)
}

func runKNN(args []string) {
	fs := flag.NewFlagSet("knn", flag.ExitOnError)
	k      := fs.Int("k", 5, "number of neighbours")
	bquant := fs.Bool("bquant", false, "enable 1-bit quantization (threshold 128, Hamming distance)")
	fs.Parse(args)

	t0 := timerStart("Loading data...")
	trainImgs, trainLbls, testImgs, testLbls := loadData()
	timerStopAndReport(t0, fmt.Sprintf("load time (%d train, %d test)", len(trainImgs), len(testImgs)))

	clf := &classifier.KNNClassifier{K: *k, BitQuantEnabled: *bquant}

	t1 := timerStart(fmt.Sprintf("Fitting KNN (k=%d, bquant=%v) on %d training samples...", *k, *bquant, len(trainImgs)))
	if err := clf.Fit(trainImgs, trainLbls); err != nil {
		log.Fatal(err)
	}
	timerStopAndReport(t1, "fit time")

	classifier.RunEval(clf, testImgs, testLbls)
}

func runNN(args []string) {
	fs := flag.NewFlagSet("nn", flag.ExitOnError)
	hidden := fs.Int("hidden", 128, "hidden layer size")
	lr := fs.Float64("lr", 0.1, "learning rate")
	epochs := fs.Int("epochs", 10, "number of training epochs")
	batch := fs.Int("batch", 64, "mini-batch size")
	fs.Parse(args)

	t0 := timerStart("Loading training set...")
	trainImgs, trainLbls, testImgs, testLbls := loadData()
	timerStopAndReport(t0, fmt.Sprintf("load time (%d train, %d test)", len(trainImgs), len(testImgs)))

	clf := classifier.NewNNClassifier(*hidden, *lr, *epochs, *batch)

	t1 := timerStart(fmt.Sprintf("Training MLP (hidden=%d, lr=%.4f, epochs=%d, batch=%d) on %d samples...",
		*hidden, *lr, *epochs, *batch, len(trainImgs)))
	if err := clf.Fit(trainImgs, trainLbls); err != nil {
		log.Fatal(err)
	}
	timerStopAndReport(t1, "train time")

	classifier.RunEval(clf, testImgs, testLbls)
}

func timerStart(msg string) time.Time {
	fmt.Println(msg)
	return time.Now()
}

func timerStopAndReport(t0 time.Time, label string) {
	fmt.Printf("  %s: %.3f s\n", label, time.Since(t0).Seconds())
}

func loadData() (trainImgs [][classifier.Pixels]float32, trainLbls []uint8,
	testImgs [][classifier.Pixels]float32, testLbls []uint8) {
	var err error
	if trainImgs, err = classifier.LoadImages(trainImages); err != nil {
		log.Fatal(err)
	}
	if trainLbls, err = classifier.LoadLabels(trainLabels); err != nil {
		log.Fatal(err)
	}
	if testImgs, err = classifier.LoadImages(testImages); err != nil {
		log.Fatal(err)
	}
	if testLbls, err = classifier.LoadLabels(testLabels); err != nil {
		log.Fatal(err)
	}
	return
}
