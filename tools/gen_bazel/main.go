// Command gen_bazel generates sources.bzl from upstream Visual Studio project files.
package main

import (
	"bytes"
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/gonzojive/ngspice/tools/internal/vcxproj"
)

var (
	workspaceRoot = flag.String("workspace-root", ".", "Path to ngspice workspace root")
	outputFile    = flag.String("out", "sources.bzl", "Output Starlark file path relative to workspace root")
	checkOnly     = flag.Bool("check", false, "Check if sources.bzl is up to date instead of writing")
)

func main() {
	flag.Parse()

	root := *workspaceRoot
	if root == "." {
		if ws := os.Getenv("BUILD_WORKSPACE_DIRECTORY"); ws != "" {
			root = ws
		}
	}
	absRoot, err := filepath.Abs(root)
	if err != nil {
		log.Fatalf("failed to resolve workspace root: %v", err)
	}

	var buf bytes.Buffer
	buf.WriteString(`"""Auto-generated source lists for ngspice Bazel build.

DO NOT EDIT DIRECTLY.
Regenerate using:
    bazel run //tools/gen_bazel
"""

`)

	// 1. Process libngspice (visualc/sharedspice.vcxproj)
	sharedSpicePath := filepath.Join(absRoot, "visualc", "sharedspice.vcxproj")
	sharedProj, err := vcxproj.Parse(sharedSpicePath)
	if err != nil {
		log.Fatalf("failed to parse %s: %v", sharedSpicePath, err)
	}

	sharedSrcs, err := sharedProj.NormalizedSourceFiles(absRoot)
	if err != nil {
		log.Fatalf("failed to normalize sharedspice sources: %v", err)
	}

	var libCommonSrcs, libWinSrcs []string
	for _, f := range sharedSrcs {
		// Generated bison files will be supplied by rules_bison targets
		if strings.Contains(f, "tmp-bison") {
			continue
		}
		// Windows-specific compatibility shim
		if strings.Contains(f, "msvc-compat.c") || strings.Contains(f, "winmain.c") {
			libWinSrcs = append(libWinSrcs, f)
			continue
		}
		libCommonSrcs = append(libCommonSrcs, f)
	}

	writeStarlarkList(&buf, "LIBNGSPICE_COMMON_SRCS", libCommonSrcs)
	writeStarlarkList(&buf, "LIBNGSPICE_WINDOWS_SRCS", libWinSrcs)

	// 2. Process ngspice standalone CLI (visualc/vngspice.vcxproj)
	vngspicePath := filepath.Join(absRoot, "visualc", "vngspice.vcxproj")
	vngProj, err := vcxproj.Parse(vngspicePath)
	if err != nil {
		log.Fatalf("failed to parse %s: %v", vngspicePath, err)
	}

	vngSrcs, err := vngProj.NormalizedSourceFiles(absRoot)
	if err != nil {
		log.Fatalf("failed to normalize vngspice sources: %v", err)
	}

	// Identify sources unique to the CLI executable
	sharedSet := make(map[string]bool)
	for _, f := range sharedSrcs {
		sharedSet[f] = true
	}

	var cliUniqueSrcs []string
	for _, f := range vngSrcs {
		if strings.Contains(f, "tmp-bison") {
			continue
		}
		if !sharedSet[f] {
			if strings.Contains(f, "winmain.c") || strings.Contains(f, "hist_info.c") {
				continue
			}
			cliUniqueSrcs = append(cliUniqueSrcs, f)
		}
	}
	sort.Strings(cliUniqueSrcs)
	writeStarlarkList(&buf, "NGSPICE_CLI_UNIQUE_SRCS", cliUniqueSrcs)

	// 3. Process cmpp host tool (visualc/xspice/cmpp/cmpp.vcxproj)
	cmppPath := filepath.Join(absRoot, "visualc", "xspice", "cmpp", "cmpp.vcxproj")
	cmppProj, err := vcxproj.Parse(cmppPath)
	if err != nil {
		log.Fatalf("failed to parse %s: %v", cmppPath, err)
	}

	cmppSrcs, err := cmppProj.NormalizedSourceFiles(absRoot)
	if err != nil {
		log.Fatalf("failed to normalize cmpp sources: %v", err)
	}

	var cmppPureCSrcs []string
	for _, f := range cmppSrcs {
		if strings.Contains(f, "tmp-bison") {
			continue
		}
		cmppPureCSrcs = append(cmppPureCSrcs, f)
	}
	sort.Strings(cmppPureCSrcs)
	writeStarlarkList(&buf, "CMPP_SRCS", cmppPureCSrcs)

	// 4. Process KLU_COMPLEX (visualc/KLU/KLU_COMPLEX.vcxproj)
	kluPath := filepath.Join(absRoot, "visualc", "KLU", "KLU_COMPLEX.vcxproj")
	kluProj, err := vcxproj.Parse(kluPath)
	if err != nil {
		log.Fatalf("failed to parse %s: %v", kluPath, err)
	}
	kluSrcs, err := kluProj.NormalizedSourceFiles(absRoot)
	if err != nil {
		log.Fatalf("failed to normalize klu sources: %v", err)
	}
	sort.Strings(kluSrcs)
	writeStarlarkList(&buf, "KLU_COMPLEX_SRCS", kluSrcs)

	outPath := filepath.Join(absRoot, *outputFile)
	newContent := buf.Bytes()

	if *checkOnly {
		existingContent, err := os.ReadFile(outPath)
		if err != nil {
			log.Fatalf("failed to read existing %s: %v", outPath, err)
		}
		if !bytes.Equal(existingContent, newContent) {
			fmt.Fprintf(os.Stderr, "ERROR: %s is out of date with upstream project files.\n", *outputFile)
			fmt.Fprintf(os.Stderr, "Run 'bazel run //tools/gen_bazel' to update it.\n")
			os.Exit(1)
		}
		fmt.Printf("OK: %s is up to date.\n", *outputFile)
		return
	}

	if err := os.WriteFile(outPath, newContent, 0644); err != nil {
		log.Fatalf("failed to write %s: %v", outPath, err)
	}
	fmt.Printf("Generated %s successfully (%d common libngspice sources).\n", outPath, len(libCommonSrcs))
}

func writeStarlarkList(buf *bytes.Buffer, varName string, items []string) {
	fmt.Fprintf(buf, "%s = [\n", varName)
	for _, item := range items {
		fmt.Fprintf(buf, "    %q,\n", item)
	}
	buf.WriteString("]\n\n")
}
