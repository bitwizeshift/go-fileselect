package fileselect

import "github.com/bitwizeshift/go-fileselect/internal/native"

// WindowConfig configures a dialog. Every field is optional; a nil
// [*WindowConfig] selects platform defaults for all of them.
type WindowConfig struct {
	// Title sets the dialog window title. Empty uses the platform default.
	Title string

	// Dir is the directory the dialog initially displays. Empty uses the
	// platform default (typically the last-used or home directory).
	Dir string

	// Filename is the suggested file name. It is honored by [SaveFile] and
	// ignored by the open dialogs.
	Filename string

	// Filters restricts the selectable file types. An empty slice allows any
	// file. Filters are ignored when selecting a directory.
	Filters []Filter
}

// Filter describes a selectable file-type group identified by a human-readable
// name and the extensions it matches.
type Filter struct {
	// Name is the label shown for the group, e.g. "PSX images".
	Name string

	// Extensions are matched without a leading dot, e.g. {"bin", "cue"}.
	Extensions []string
}

// request builds the native request for the given mode. A nil receiver yields a
// request carrying only the mode.
func (cfg *WindowConfig) request(mode native.Mode) native.Request {
	if cfg == nil {
		return native.Request{Mode: mode}
	}
	filters := make([]native.Filter, len(cfg.Filters))
	for i, f := range cfg.Filters {
		filters[i] = native.Filter{Name: f.Name, Extensions: f.Extensions}
	}
	return native.Request{
		Mode:     mode,
		Title:    cfg.Title,
		Dir:      cfg.Dir,
		Filename: cfg.Filename,
		Filters:  filters,
	}
}
