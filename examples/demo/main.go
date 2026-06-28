// Command demo is a manual harness for exercising the fileselect dialogs.
//
// Run it with the name of the dialog to present:
//
//	go run ./examples/demo open    # single file
//	go run ./examples/demo files   # multiple files
//	go run ./examples/demo save    # save destination
//	go run ./examples/demo dir     # directory
//
// It prints the selection, or notes that the dialog was cancelled.
package main

import (
	"errors"
	"fmt"
	"os"
	"runtime"

	fileselect "github.com/bitwizeshift/go-fileselect"
)

// init locks the main goroutine to the main OS thread so the macOS Cocoa
// backend can present panels from a command-line program.
func init() { runtime.LockOSThread() }

func main() {
	which := "open"
	if len(os.Args) > 1 {
		which = os.Args[1]
	}

	cfg := &fileselect.WindowConfig{
		Title: "fileselect demo",
		Filters: []fileselect.Filter{
			{Name: "PSX images", Extensions: []string{"bin", "cue"}},
			{Name: "All files", Extensions: []string{"*"}},
		},
	}

	if err := run(which, cfg); err != nil {
		if errors.Is(err, fileselect.ErrCancelled) {
			fmt.Println("cancelled")
			return
		}
		fmt.Fprintln(os.Stderr, "error:", err)
		os.Exit(1)
	}
}

func run(which string, cfg *fileselect.WindowConfig) error {
	switch which {
	case "open":
		path, err := fileselect.OpenFile(cfg)
		if err != nil {
			return err
		}
		fmt.Println("selected:", path)
	case "files":
		paths, err := fileselect.OpenFiles(cfg)
		if err != nil {
			return err
		}
		for _, path := range paths {
			fmt.Println("selected:", path)
		}
	case "save":
		cfg.Filename = "untitled.bin"
		path, err := fileselect.SaveFile(cfg)
		if err != nil {
			return err
		}
		fmt.Println("save to:", path)
	case "dir":
		path, err := fileselect.OpenDir(cfg)
		if err != nil {
			return err
		}
		fmt.Println("selected:", path)
	default:
		return fmt.Errorf("unknown dialog %q: want open, files, save, or dir", which)
	}
	return nil
}
