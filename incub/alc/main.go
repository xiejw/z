package main

import (
	"fmt"
)

type Module struct {
}

func NewModule() *Module {
	return &Module{}
}

func (m *Module) NewArray(shape Shape, v []float32) *OpArray {
	return &OpArray{
		Shape: shape,
		Array: v,
	}
}

type Shape struct {
	Rank int
	Dims []int
}

type Value struct {
	Shape Shape
	Array []float32
}

type Op interface {
	String() string
	OutputShape() Shape
}

type OpArray struct {
	Shape Shape
	Array []float32
}

func (op *OpArray) String() string     { return "OpArray" }
func (op *OpArray) OutputShape() Shape { return op.Shape }

type OpType int

const (
	OpAdd OpType = iota
)

type OpBinary struct {
	Type   OpType
	Output Op
	LHS    Op
	RHS    Op
}

func (op *OpBinary) String() string     { return "OpBinary" }
func (op *OpBinary) OutputShape() Shape { return op.Output.OutputShape() }

type OpPlaceHolder struct {
	Name  string
	Shape Shape
}

func (op *OpPlaceHolder) String() string     { return "OpPlaceHolder" }
func (op *OpPlaceHolder) OutputShape() Shape { return op.Shape }

func main() {
	fmt.Printf("Hello alc\n")
}
