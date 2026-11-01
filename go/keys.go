package main

import (
	"os"
	"strconv"
	"strings"
	"sync"
)

// ---- keyboard ---------------------------------------------------------------
//  getch() semantics: char code; extended keys return 0 and then the DOS scan code.

var (
	headless bool
	kmu      sync.Mutex
	kcond    = sync.NewCond(&kmu)
	kq       []int // key queue (filled by the window thread, or by DRUGLORD_KEYS when headless)
)

type headlessStop string

func initHeadlessKeys() {
	for _, t := range strings.Fields(os.Getenv("DRUGLORD_KEYS")) {
		if n, err := strconv.Atoi(t); err == nil {
			kq = append(kq, n)
		}
	}
}

func queueCount() int {
	kmu.Lock()
	defer kmu.Unlock()
	return len(kq)
}

func testKey() bool { return queueCount() > 0 }

// look at the next key without taking it (-1 if none)
func peekKey() int {
	kmu.Lock()
	defer kmu.Unlock()
	if len(kq) == 0 {
		return -1
	}
	return kq[0]
}

func readKey() int {
	flushScreen()
	kmu.Lock()
	defer kmu.Unlock()
	for len(kq) == 0 {
		if headless {
			panic(headlessStop("HEADLESS_OUT_OF_KEYS"))
		}
		kcond.Wait()
	}
	k := kq[0]
	kq = kq[1:]
	return k
}

func flushKeys() {
	if headless {
		return
	}
	kmu.Lock()
	kq = kq[:0]
	kmu.Unlock()
}

// pushKey is called by the window thread
func pushKey(keys ...int) {
	kmu.Lock()
	if len(kq) < 1024 {
		kq = append(kq, keys...)
	}
	kmu.Unlock()
	kcond.Broadcast()
}
