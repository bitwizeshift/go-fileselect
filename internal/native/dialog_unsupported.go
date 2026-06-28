//go:build !darwin && !windows && !linux

package native

// Open reports that the current platform has no native dialog implementation.
// It always returns [ErrUnsupported].
func Open(Request) ([]string, error) {
	return nil, ErrUnsupported
}
