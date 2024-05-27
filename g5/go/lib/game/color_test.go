package game

import (
	"fmt"
	"testing"
)

func TestColor(t *testing.T) {
	if "b" != fmt.Sprintf("%v", CLR_BLACK) {
		t.Errorf("color string error")
	}
	if "w" != fmt.Sprintf("%v", CLR_WHITE) {
		t.Errorf("color string error")
	}
	if "_" != fmt.Sprintf("%v", CLR_NA) {
		t.Errorf("color string error")
	}
}
