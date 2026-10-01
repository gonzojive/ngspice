# Bazel Build for ngspice

This repository contains a pure Bazel build system for **ngspice** and **libngspice**, designed for hermeticity, reproducibility, and seamless consumption by downstream projects such as [kicad-bazel](https://github.com/gonzojive/kicad-bazel).

---

## 1. Architecture Overview

### Target Structure

The Bazel build graph is organized into layered libraries and binaries in `//:BUILD.bazel`:

- **`//:ngspice_headers`**: A `cc_library` exporting all public and internal C/C++ header files, including generated Bison parser headers and the Bazel configuration header `src/include/ngspice/config.h`. Exposes standard include directories:
  - `src/include`
  - `src/spicelib/devices`
  - `src/spicelib/parser`
  - `src/frontend`
  - `src/maths/poly`
  - `src/include/cppduals`
- **`//:inpptree-parser` & `//:parse-bison`**: Bison targets generating C parsers and headers using BCR `@rules_bison`:
  - `src/spicelib/parser/inpptree-parser.y` $\rightarrow$ `inpptree-parser.c`, `inpptree-parser.h`
  - `src/frontend/parse-bison.y` $\rightarrow$ `parse-bison.c`, `parse-bison.h`
- **`//:klu_complex`**: Compiles the KLU sparse solver routines with `-DCOMPLEX` to provide `klu_z_*` complex matrix support required by the simulator.
- **`//:ngspice_core`**: The main simulator engine library containing all common sources from `sources.bzl:LIBNGSPICE_COMMON_SRCS`, linked with OpenMP (`-fopenmp`), POSIX math, and dynamic loader libraries.
- **`//:libngspice`**: The static/archive `cc_library` incorporating `src/sharedspice.c` and exporting public shared-library APIs (`ngSpice_Init`, `ngSpice_Circ`, `ngSpice_Command`, etc.).
- **`//:libngspice.so`**: The standalone shared object binary (`linkshared = True`) for dynamic loading.
- **`//:ngspice`**: The standalone CLI executable.

### Configuration (`config.h`)

Rather than relying on Autotools `configure` or CMake generation at build time, Bazel uses a predefined POSIX/MSVC platform header in `src/include/ngspice/config_bazel.h`. The `genrule(name = "config_h")` copies this to `src/include/ngspice/config.h` in the Bazel output tree.

Features enabled by default:
- OpenMP multi-threading (`USE_OMP`, `-fopenmp`)
- XSPICE code model support (`#define XSPICE 1`)
- CIDER numerical device simulator (`#define CIDER 1`)
- KLU sparse matrix solver (`#define KLU 1`)
- S-parameter RF analysis (`#define RFSPICE 1`)
- Periodic Steady-State analysis (`#define WITH_PSS 1`)

---

## 2. Source Generation Toolchain (`//tools/gen_bazel`)

### Motivation

Upstream ngspice maintains curated Visual Studio project files (`visualc/*.vcxproj`) for Windows developers. These project files enumerate the exact list of non-Autotools source files needed to build `libngspice`, `ngspice`, `cmpp`, and `KLU_COMPLEX`.

To eliminate manual maintenance of `sources.bzl`, we provide an automated Go tool:

- **`//tools/internal/vcxproj`**: A Go package with complete XML deserialization for MSVC project files. It handles XML namespaces, Windows backslash normalization, and directory tree resolution. Tested via `//tools/internal/vcxproj:vcxproj_test`.
- **`//tools/gen_bazel`**: A Go CLI command that parses:
  - `visualc/sharedspice.vcxproj` $\rightarrow$ `LIBNGSPICE_COMMON_SRCS`, `LIBNGSPICE_WINDOWS_SRCS`
  - `visualc/vngspice.vcxproj` $\rightarrow$ `NGSPICE_CLI_UNIQUE_SRCS`
  - `visualc/xspice/cmpp/cmpp.vcxproj` $\rightarrow$ `CMPP_SRCS`
  - `visualc/KLU/KLU_COMPLEX.vcxproj` $\rightarrow$ `KLU_COMPLEX_SRCS`

### Usage

To regenerate `sources.bzl`:
```bash
bazel run //tools/gen_bazel
```

To verify in CI that `sources.bzl` is synchronized with upstream MSVC project files:
```bash
bazel run //tools/gen_bazel -- --check
```

---

## 3. Downstream Consumption (e.g. KiCad)

To use `libngspice` in a downstream Bazel project:

### 1. Configure `MODULE.bazel`

Add a dependency via `git_override` or BCR (when published):

```starlark
bazel_dep(name = "ngspice", version = "47.0.0")

git_override(
    module_name = "ngspice",
    remote = "https://github.com/gonzojive/ngspice.git",
    commit = "<commit-sha>",
)
```

### 2. Link in `BUILD.bazel`

Depend on `@ngspice//:libngspice` and `@ngspice//:ngspice_headers`:

```starlark
cc_library(
    name = "kicad_sim",
    srcs = ["sim_engine.cpp"],
    deps = [
        "@ngspice//:libngspice",
        "@ngspice//:ngspice_headers",
    ],
)
```

In your C/C++ code:
```c
#include <ngspice/sharedspice.h>

int init() {
    return ngSpice_Init(SendCharCb, SendStatCb, ExitCb,
                        DataCb, InitDataCb, ThreadCb, NULL);
}
```

---

## 4. Upstream Sync Runbook

When synchronizing with a new upstream release of ngspice:

1. **Fetch & Rebase/Merge**:
   ```bash
   git remote add upstream git://git.code.sf.net/p/ngspice/ngspice
   git fetch upstream
   git merge upstream/master
   ```

2. **Regenerate Sources**:
   ```bash
   bazel run //tools/gen_bazel
   ```

3. **Check Version Headers**:
   If upstream version bumped (e.g., from 47 to 48):
   - Update `version` in `MODULE.bazel`.
   - Update `VERSION`, `PACKAGE_VERSION`, and `PACKAGE_STRING` in `src/include/ngspice/config_bazel.h`.

4. **Run Verification**:
   ```bash
   bazel run //tools/gen_bazel -- --check
   bazel test //...
   bazel run //:ngspice -- --version
   ```

5. **Commit and Push**:
   ```bash
   git add sources.bzl src/include/ngspice/config_bazel.h MODULE.bazel
   git commit -m "Update ngspice to upstream <version>"
   git push origin bazel
   ```
