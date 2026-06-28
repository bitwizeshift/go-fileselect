// Package fileselect provides a small, cross-platform API for presenting native
// modal file dialogs and returning the user's selection. This is a thin wrapper
// around the native-OS dialog APIs.
//
// It exposes four entry points, each blocking until the user responds:
//
//   - [OpenFile]  selects a single existing file
//   - [OpenFiles] selects one or more existing files
//   - [SaveFile]  chooses a destination path for saving
//   - [OpenDir]   selects a single existing directory
package fileselect
