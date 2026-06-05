# CadmIA Experiments [![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/sdavtaker/cadmia-experiments/badge)](https://scorecard.dev/viewer/?uri=github.com/sdavtaker/cadmia-experiments)

A collection of experiments using the CadmIA C++23 IA-DEVS simulator,
where time and values are represented as intervals rather than scalars.

## What this repository is about

CadmIA implements Interval-Approximated DEVS (IA-DEVS), a formalism in which every
quantity carries an interval uncertainty.  The simulator is the C++23 successor to
Cadmium, built on C++ Concepts for type-safe interval-time models.

The underlying formalism is defined in:

> Vicino, Wainer, Dalle. *Uncertainty on Discrete-Event System Simulation.*
> ACM TOMACS, Vol. 32, No. 1, Article 2, 2021. DOI: 10.1145/3466169.

## Structure

```
cadmia-impl-notes.tex    Shared implementation notes (IA-DEVS conventions,
                         interval representation, log format) included in the paper.
docs/
  main.tex               Root LaTeX document — compiles to main.pdf.
  main.pdf               Built paper.
vdw14/
  spec.tex               Model definition and expected observations.
  main.cpp               Simulation driver.
  k_counter.hpp          Interval-arithmetic counter model.
  tick_gen.hpp           Tick generator (period [1/10, 1/10] s).
  reset_gen.hpp          Reset generator (period [1, 1] s).
  test_models.cpp        Unit tests.
  CMakeLists.txt
vcpkg.json               vcpkg dependency manifest.
CMakeLists.txt           Root build configuration.
```

## Building

Requires CMake 3.28+, vcpkg, and g++-14 (C++23 Concepts).
The `vcpkg-configuration.json` in this repository pre-configures the private registry that
provides the `cadmia` and `cdcommons` packages — no manual registry setup is needed.

```sh
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_CXX_COMPILER=g++-14
cmake --build build -j2
ctest --test-dir build
```

Simulation executables write NDJSON logs to stdout.
See [`doc/log-format.md`](https://github.com/sdavtaker/cadmia/blob/main/doc/log-format.md) in the CadmIA repository for the log schema.
