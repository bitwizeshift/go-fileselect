package native

import "errors"

// ErrCancelled is returned by [Open] when the user dismisses the dialog without
// making a selection.
var ErrCancelled = errors.New("dialog cancelled by user")

// ErrUnsupported is returned by [Open] on platforms that have no native dialog
// implementation.
var ErrUnsupported = errors.New("platform not supported")

// Mode selects which kind of dialog [Open] presents.
type Mode int

const (
	// ModeOpenFile selects a single existing file.
	ModeOpenFile Mode = iota
	// ModeOpenFiles selects one or more existing files.
	ModeOpenFiles
	// ModeSaveFile chooses a destination path for saving.
	ModeSaveFile
	// ModeOpenDir selects a single existing directory.
	ModeOpenDir
)

// Filter describes a single selectable file-type group, identified by a
// human-readable name and the set of extensions it matches.
type Filter struct {
	// Name is the label shown for the group, e.g. "PSX images".
	Name string
	// Extensions are matched without a leading dot, e.g. {"bin", "cue"}.
	Extensions []string
}

// Request describes a dialog to present. The zero value is a valid request for
// a single-file open dialog with platform default title and directory.
type Request struct {
	// Mode is the kind of dialog to present.
	Mode Mode
	// Title overrides the dialog window title when non-empty.
	Title string
	// Dir is the initial directory; when empty the platform default is used.
	Dir string
	// Filename is the suggested name, used by [ModeSaveFile].
	Filename string
	// Filters restricts the selectable file types; empty means all files.
	Filters []Filter
}
