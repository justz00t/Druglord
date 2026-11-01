//go:build !windows

package main

import (
	"fmt"
	"os"
)

// The window version only exists on Windows; elsewhere only the headless test mode runs.
func runWindowed(body func()) {
	fmt.Fprintln(os.Stderr, "DRUGLORD's window needs Windows. (Set DRUGLORD_HEADLESS=1 for the test mode.)")
	os.Exit(1)
}
