// Package native provides the platform-specific, CGO-backed implementation of
// the modal file-selection dialogs exposed by the parent fileselect package.
//
// It defines a single, OS-agnostic entry point, [Open], that accepts a
// [Request] describing the kind of dialog to display and its configuration, and
// returns the absolute paths the user selected. Each supported operating system
// supplies its own build-tagged implementation of [Open]:
//
//   - darwin  uses Cocoa NSOpenPanel / NSSavePanel
//   - windows uses the COM Common Item Dialog (IFileOpenDialog / IFileSaveDialog)
//   - linux   uses the GTK3 GtkFileChooser
//
// Platforms without an implementation return [ErrUnsupported] so the module
// always builds. Callers that dismiss a dialog yield [ErrCancelled].
//
// This package is internal: its API is an implementation detail of fileselect
// and offers no compatibility guarantees.
package native
