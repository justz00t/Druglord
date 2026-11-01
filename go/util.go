package main

import (
	"math"
	"regexp"
	"strconv"
	"strings"
	"time"
)

// Borland rand(): 32-bit LCG, returns bits 16..30
var seed int64 = time.Now().UnixNano() & 0xFFFF

func rnd() int {
	seed = (seed*22695477 + 1) & 0xFFFFFFFF
	return int((seed >> 16) & 0x7FFF)
}

func idiv(a, b int64) int64 {
	if b == 0 {
		return 0
	}
	return int64(math.Trunc(float64(a) / float64(b)))
}

func dice(sides, count int) int {
	s := 0
	for i := 0; i < count; i++ {
		s += rnd()%sides + 1
	}
	return s
}

func ftol(x float64) int64 {
	if math.IsNaN(x) || math.IsInf(x, 0) {
		return 0
	}
	return int64(math.Trunc(x))
}

var atolRe = regexp.MustCompile(`^\s*([+-]?\d+)`)

func atol(s string) int64 {
	m := atolRe.FindStringSubmatch(s)
	if m == nil {
		return 0
	}
	v, err := strconv.ParseInt(m[1], 10, 64)
	if err != nil {
		return 0
	}
	return v
}

func f32(x float64) float64 { return float64(float32(x)) }

func padR(s string, w int) string {
	if len(s) >= w {
		return s
	}
	return s + strings.Repeat(" ", w-len(s))
}
func padL(s string, w int) string {
	if len(s) >= w {
		return s
	}
	return strings.Repeat(" ", w-len(s)) + s
}
func itoa(n int) string     { return strconv.Itoa(n) }
func ltoa(n int64) string   { return strconv.FormatInt(n, 10) }
func b2i(b bool) int {
	if b {
		return 1
	}
	return 0
}
