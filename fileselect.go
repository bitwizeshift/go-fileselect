package fileselect

import "github.com/bitwizeshift/go-fileselect/internal/native"

// ErrCancelled is returned when the user dismisses a dialog without confirming a
// selection. Callers distinguish cancellation from failure with
// [errors.Is](err, ErrCancelled).
var ErrCancelled = native.ErrCancelled

// ErrUnsupported is returned on platforms that have no native dialog
// implementation.
var ErrUnsupported = native.ErrUnsupported

// OpenFile presents a modal dialog for selecting a single existing file and
// returns its absolute path. It returns [ErrCancelled] if the user dismisses
// the dialog. A nil cfg uses platform defaults.
func OpenFile(cfg *WindowConfig) (string, error) {
	return single(cfg, native.ModeOpenFile)
}

// OpenFiles presents a modal dialog for selecting one or more existing files
// and returns their absolute paths. It returns [ErrCancelled] if the user
// dismisses the dialog. A nil cfg uses platform defaults.
func OpenFiles(cfg *WindowConfig) ([]string, error) {
	return native.Open(cfg.request(native.ModeOpenFiles))
}

// SaveFile presents a modal dialog for choosing a save destination and returns
// the absolute path the user selected. The path may not yet exist. It returns
// [ErrCancelled] if the user dismisses the dialog. A nil cfg uses platform
// defaults.
func SaveFile(cfg *WindowConfig) (string, error) {
	return single(cfg, native.ModeSaveFile)
}

// OpenDir presents a modal dialog for selecting a single existing directory and
// returns its absolute path. It returns [ErrCancelled] if the user dismisses
// the dialog. A nil cfg uses platform defaults.
func OpenDir(cfg *WindowConfig) (string, error) {
	return single(cfg, native.ModeOpenDir)
}

// single runs a dialog mode that yields at most one path and adapts the result
// to a single string.
func single(cfg *WindowConfig, mode native.Mode) (string, error) {
	paths, err := native.Open(cfg.request(mode))
	if err != nil {
		return "", err
	}
	if len(paths) == 0 {
		return "", ErrCancelled
	}
	return paths[0], nil
}
