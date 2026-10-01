// Package vcxproj provides generic parsing and extraction for Visual Studio C/C++ (.vcxproj) project files.
package vcxproj

import (
	"encoding/xml"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

// Project represents the root MSBuild <Project> element.
type Project struct {
	XMLName              xml.Name              `xml:"Project"`
	ItemGroups           []ItemGroup           `xml:"ItemGroup"`
	ItemDefinitionGroups []ItemDefinitionGroup `xml:"ItemDefinitionGroup"`
	PropertyGroups       []PropertyGroup       `xml:"PropertyGroup"`

	// Dir is the directory containing the .vcxproj file on disk.
	Dir string `xml:"-"`
}

// ItemGroup contains build items such as ClCompile, ClInclude, CustomBuild, etc.
type ItemGroup struct {
	ClCompiles       []ClCompile       `xml:"ClCompile"`
	ClHeaders        []ClInclude        `xml:"ClInclude"`
	CustomBuilds     []CustomBuild     `xml:"CustomBuild"`
	Nones            []None            `xml:"None"`
	ResourceCompiles []ResourceCompile `xml:"ResourceCompile"`
}

// ClCompile represents a <ClCompile Include="..." /> compilation unit.
type ClCompile struct {
	Include string `xml:"Include,attr"`
}

// ClInclude represents a <ClInclude Include="..." /> header file.
type ClInclude struct {
	Include string `xml:"Include,attr"`
}

// ClHeader is an alias for ClInclude.
type ClHeader = ClInclude

// CustomBuild represents a <CustomBuild Include="..."> rule.
type CustomBuild struct {
	Include string `xml:"Include,attr"`
	Command string `xml:"Command"`
	Outputs string `xml:"Outputs"`
	Message string `xml:"Message"`
}

// None represents a <None Include="..." /> item.
type None struct {
	Include string `xml:"Include,attr"`
}

// ResourceCompile represents a Windows resource file (.rc).
type ResourceCompile struct {
	Include string `xml:"Include,attr"`
}

// ItemDefinitionGroup specifies compilation/link definitions per configuration.
type ItemDefinitionGroup struct {
	Condition string       `xml:"Condition,attr"`
	ClCompile ClCompileDef `xml:"ClCompile"`
}

// ClCompileDef contains compiler options in an ItemDefinitionGroup.
type ClCompileDef struct {
	AdditionalIncludeDirectories string `xml:"AdditionalIncludeDirectories"`
	PreprocessorDefinitions      string `xml:"PreprocessorDefinitions"`
}

// PropertyGroup represents MSBuild property definitions.
type PropertyGroup struct {
	Condition string `xml:"Condition,attr"`
}

// Parse reads a .vcxproj file from the local filesystem.
func Parse(path string) (*Project, error) {
	absPath, err := filepath.Abs(path)
	if err != nil {
		return nil, fmt.Errorf("failed to get absolute path for %s: %w", path, err)
	}

	f, err := os.Open(absPath)
	if err != nil {
		return nil, fmt.Errorf("failed to open %s: %w", path, err)
	}
	defer f.Close()

	return ParseReader(f, filepath.Dir(absPath))
}

// ParseReader parses .vcxproj XML content from an io.Reader.
func ParseReader(r io.Reader, dir string) (*Project, error) {
	var proj Project
	decoder := xml.NewDecoder(r)
	if err := decoder.Decode(&proj); err != nil {
		return nil, fmt.Errorf("failed to unmarshal vcxproj XML: %w", err)
	}
	proj.Dir = dir
	return &proj, nil
}

// RawSourceFiles returns all raw Include paths from ClCompile elements.
func (p *Project) RawSourceFiles() []string {
	var files []string
	for _, g := range p.ItemGroups {
		for _, c := range g.ClCompiles {
			if strings.TrimSpace(c.Include) != "" {
				files = append(files, c.Include)
			}
		}
	}
	return files
}

// RawHeaderFiles returns all raw Include paths from ClInclude elements.
func (p *Project) RawHeaderFiles() []string {
	var files []string
	for _, g := range p.ItemGroups {
		for _, h := range g.ClHeaders {
			if strings.TrimSpace(h.Include) != "" {
				files = append(files, h.Include)
			}
		}
	}
	return files
}

// CustomBuildRules returns all CustomBuild entries in the project.
func (p *Project) CustomBuildRules() []CustomBuild {
	var rules []CustomBuild
	for _, g := range p.ItemGroups {
		for _, cb := range g.CustomBuilds {
			if strings.TrimSpace(cb.Include) != "" {
				rules = append(rules, cb)
			}
		}
	}
	return rules
}

// NormalizePath resolves a Windows-style path relative to p.Dir against workspaceRoot
// and returns a clean, forward-slash path relative to workspaceRoot.
func (p *Project) NormalizePath(winPath, workspaceRoot string) (string, error) {
	cleaned := strings.ReplaceAll(winPath, "\\", "/")
	var absPath string
	if filepath.IsAbs(cleaned) {
		absPath = filepath.Clean(cleaned)
	} else {
		absPath = filepath.Clean(filepath.Join(p.Dir, cleaned))
	}

	rel, err := filepath.Rel(workspaceRoot, absPath)
	if err != nil {
		return "", fmt.Errorf("failed to compute relative path: %w", err)
	}
	return filepath.ToSlash(rel), nil
}

// NormalizedSourceFiles returns all ClCompile files resolved relative to workspaceRoot.
func (p *Project) NormalizedSourceFiles(workspaceRoot string) ([]string, error) {
	var result []string
	seen := make(map[string]bool)

	for _, raw := range p.RawSourceFiles() {
		norm, err := p.NormalizePath(raw, workspaceRoot)
		if err != nil {
			return nil, err
		}
		if !seen[norm] {
			seen[norm] = true
			result = append(result, norm)
		}
	}
	sort.Strings(result)
	return result, nil
}

// NormalizedHeaderFiles returns all ClInclude files resolved relative to workspaceRoot.
func (p *Project) NormalizedHeaderFiles(workspaceRoot string) ([]string, error) {
	var result []string
	seen := make(map[string]bool)

	for _, raw := range p.RawHeaderFiles() {
		norm, err := p.NormalizePath(raw, workspaceRoot)
		if err != nil {
			return nil, err
		}
		if !seen[norm] {
			seen[norm] = true
			result = append(result, norm)
		}
	}
	sort.Strings(result)
	return result, nil
}

// IncludeDirectories returns unique include directories specified in ItemDefinitionGroups.
func (p *Project) IncludeDirectories(workspaceRoot string) []string {
	var dirs []string
	seen := make(map[string]bool)

	for _, idg := range p.ItemDefinitionGroups {
		rawDirs := strings.Split(idg.ClCompile.AdditionalIncludeDirectories, ";")
		for _, d := range rawDirs {
			d = strings.TrimSpace(d)
			if d == "" || strings.HasPrefix(d, "%") {
				continue
			}
			norm, err := p.NormalizePath(d, workspaceRoot)
			if err == nil && !seen[norm] {
				seen[norm] = true
				dirs = append(dirs, norm)
			}
		}
	}
	sort.Strings(dirs)
	return dirs
}

// PreprocessorDefinitions returns unique preprocessor definitions found in the project.
func (p *Project) PreprocessorDefinitions() []string {
	var defs []string
	seen := make(map[string]bool)

	for _, idg := range p.ItemDefinitionGroups {
		rawDefs := strings.Split(idg.ClCompile.PreprocessorDefinitions, ";")
		for _, d := range rawDefs {
			d = strings.TrimSpace(d)
			if d == "" || strings.HasPrefix(d, "%") || strings.HasPrefix(d, "$") {
				continue
			}
			if !seen[d] {
				seen[d] = true
				defs = append(defs, d)
			}
		}
	}
	sort.Strings(defs)
	return defs
}
