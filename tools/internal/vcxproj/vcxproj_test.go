package vcxproj_test

import (
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/gonzojive/ngspice/tools/internal/vcxproj"
)

const sampleProjectXML = `<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="14.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>..\src\include;..\src\frontend;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PreprocessorDefinitions>SIMULATOR;XSPICE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemGroup>
    <CustomBuild Include="..\src\frontend\parse-bison.y">
      <Command>win_bison.exe %(Identity)</Command>
      <Outputs>parse-bison.c;parse-bison.h</Outputs>
    </CustomBuild>
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="..\src\conf.c" />
    <ClCompile Include="..\src\ngspice.c" />
    <ClInclude Include="..\src\conf.h" />
  </ItemGroup>
</Project>`

func TestParseReader(t *testing.T) {
	proj, err := vcxproj.ParseReader(strings.NewReader(sampleProjectXML), "/mock/visualc")
	if err != nil {
		t.Fatalf("ParseReader failed: %v", err)
	}

	srcs := proj.RawSourceFiles()
	if len(srcs) != 2 {
		t.Fatalf("expected 2 raw source files, got %d: %v", len(srcs), srcs)
	}

	headers := proj.RawHeaderFiles()
	if len(headers) != 1 {
		t.Fatalf("expected 1 header, got %d: %v", len(headers), headers)
	}

	rules := proj.CustomBuildRules()
	if len(rules) != 1 {
		t.Fatalf("expected 1 custom build rule, got %d", len(rules))
	}
	if !strings.HasSuffix(rules[0].Include, "parse-bison.y") {
		t.Errorf("expected parse-bison.y rule, got %s", rules[0].Include)
	}

	normSrcs, err := proj.NormalizedSourceFiles("/mock")
	if err != nil {
		t.Fatalf("NormalizedSourceFiles failed: %v", err)
	}
	expected := []string{"src/conf.c", "src/ngspice.c"}
	if len(normSrcs) != len(expected) {
		t.Fatalf("expected %v, got %v", expected, normSrcs)
	}
	for i := range expected {
		if normSrcs[i] != expected[i] {
			t.Errorf("src[%d]: expected %s, got %s", i, expected[i], normSrcs[i])
		}
	}
}

func TestParseSharedSpiceVcxproj(t *testing.T) {
	// Find repo root relative to test
	wd, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}
	repoRoot := filepath.Clean(filepath.Join(wd, "..", "..", ".."))
	vcxprojPath := filepath.Join(repoRoot, "visualc", "sharedspice.vcxproj")

	if _, err := os.Stat(vcxprojPath); os.IsNotExist(err) {
		t.Skipf("sharedspice.vcxproj not found at %s", vcxprojPath)
	}

	proj, err := vcxproj.Parse(vcxprojPath)
	if err != nil {
		t.Fatalf("failed to parse sharedspice.vcxproj: %v", err)
	}

	srcs, err := proj.NormalizedSourceFiles(repoRoot)
	if err != nil {
		t.Fatalf("NormalizedSourceFiles failed: %v", err)
	}

	if len(srcs) < 1400 {
		t.Errorf("expected at least 1400 source files in sharedspice.vcxproj, got %d", len(srcs))
	}

	// Verify custom builds contain parse-bison.y and inpptree-parser.y
	rules := proj.CustomBuildRules()
	var customNames []string
	for _, r := range rules {
		norm, _ := proj.NormalizePath(r.Include, repoRoot)
		customNames = append(customNames, norm)
	}

	hasBison := false
	hasInpTree := false
	for _, name := range customNames {
		if strings.HasSuffix(name, "parse-bison.y") {
			hasBison = true
		}
		if strings.HasSuffix(name, "inpptree-parser.y") {
			hasInpTree = true
		}
	}

	if !hasBison || !hasInpTree {
		t.Errorf("expected both parse-bison.y and inpptree-parser.y in custom builds, got: %v", customNames)
	}
}
