package policy

import (
	"log"
	"time"
)

// DummyPolicy is designed based on a super dummy algorithrm which puts stones
// in the first possible place. And even before that, it tries to sleep.
//
// To create one, consider
//
//	&DummyPolicy{
//	    SleepSeconds: 1,
//	    AIFirst: true,
//	}
type DummyPolicy struct {
	SleepSeconds int
	AIFirst      bool
}

func (p *DummyPolicy) IsAIFirst() bool {
	return p.AIFirst
}

func (p *DummyPolicy) Query(w, h int, v []int) (row, col int) {
	log.Printf("sleep for %v seconds", p.SleepSeconds)
	time.Sleep(time.Duration(p.SleepSeconds) * time.Second)
	for row := 0; row < h; row++ {
		for col := 0; col < w; col++ {
			index := row*w + col
			if v[index] == 0 {
				log.Printf("place on (%v,%v)", row, col)
				return row, col
			}
		}
	}
	panic("should never reach")
}
