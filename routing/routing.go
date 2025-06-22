package routing

import (
	"fmt"
	"os/exec"
)

// GetRoute runs the routing CLI with (start, end, mode)
// and returns its JSON stdout or an error.
func GetRoute(start, end, mode string) ([]byte, error) {
	// adjust "./routing/routing" to the correct path of your CLI binary
	cmd := exec.Command("./cpp_binaries/routing", start, end, mode)
	out, err := cmd.CombinedOutput()
	if err != nil {
		// include the CLI’s stderr in your Go error
		return nil, fmt.Errorf("routing binary failed: %w\noutput:\n%s", err, out)
	}
	return out, nil
}
