//go:build darwin || windows || linux

package native

/*
#cgo darwin  LDFLAGS: -framework Cocoa
#cgo linux   pkg-config: gtk+-3.0
#cgo windows LDFLAGS: -lole32 -loleaut32 -luuid -lshell32

#include <stdlib.h>
#include "dialog.h"
*/
import "C"

import (
	"errors"
	"strings"
	"unsafe"
)

// Open presents the native modal dialog described by req, blocks until the user
// responds, and returns the selected absolute paths. It returns [ErrCancelled]
// when the user dismisses the dialog, or a descriptive error when the platform
// dialog cannot be presented.
func Open(req Request) ([]string, error) {
	title := C.CString(req.Title)
	defer C.free(unsafe.Pointer(title))
	dir := C.CString(req.Dir)
	defer C.free(unsafe.Pointer(dir))
	filename := C.CString(req.Filename)
	defer C.free(unsafe.Pointer(filename))

	names, specs := splitFilters(req.Filters)
	cNames, freeNames := cStrings(names)
	defer freeNames()
	cSpecs, freeSpecs := cStrings(specs)
	defer freeSpecs()

	result := C.fileselect_run(
		C.int(req.Mode),
		title, dir, filename,
		cNames, cSpecs, C.int(len(names)),
	)
	defer C.fileselect_free(result)

	switch result.status {
	case C.FILESELECT_CANCELLED:
		return nil, ErrCancelled
	case C.FILESELECT_ERROR:
		return nil, errors.New(goStringOr(result.error, "dialog could not be presented"))
	default:
		return goStrings(result.paths, int(result.count)), nil
	}
}

// splitFilters flattens filters into parallel name and spec slices, where each
// spec is a ';'-separated list of bare extensions matching its name.
func splitFilters(filters []Filter) (names, specs []string) {
	for _, f := range filters {
		names = append(names, f.Name)
		specs = append(specs, strings.Join(f.Extensions, ";"))
	}
	return names, specs
}

// cStrings allocates a C array of C strings from ss and returns it alongside a
// release function. The array is nil when ss is empty.
func cStrings(ss []string) (**C.char, func()) {
	if len(ss) == 0 {
		return nil, func() {}
	}
	arr := C.malloc(C.size_t(len(ss)) * C.size_t(unsafe.Sizeof(uintptr(0))))
	view := unsafe.Slice((**C.char)(arr), len(ss))
	for i, s := range ss {
		view[i] = C.CString(s)
	}
	return (**C.char)(arr), func() {
		for i := range view {
			C.free(unsafe.Pointer(view[i]))
		}
		C.free(arr)
	}
}

// goStrings copies n C strings beginning at p into a Go slice.
func goStrings(p **C.char, n int) []string {
	if p == nil || n <= 0 {
		return nil
	}
	view := unsafe.Slice(p, n)
	out := make([]string, n)
	for i := range out {
		out[i] = C.GoString(view[i])
	}
	return out
}

// goStringOr returns the Go form of p, or fallback when p is nil or empty.
func goStringOr(p *C.char, fallback string) string {
	if p == nil {
		return fallback
	}
	if s := C.GoString(p); s != "" {
		return s
	}
	return fallback
}
