# Skia python binding

[![CI](https://github.com/skia-python/skia-python/actions/workflows/ci.yml/badge.svg)](https://github.com/skia-python/skia-python/actions/workflows/ci.yml)
[![PyPI version](https://badge.fury.io/py/skia-python.svg)](https://badge.fury.io/py/skia-python)

Python binding to [Skia Graphics Library](https://skia.org/).

- Binding based on [pybind11](https://github.com/pybind/pybind11).

## Install

Binary package is available on PyPI:

```bash
pip install skia-python
```

Supported platforms: Python 3.8-3.15 (CPython) on

- Linux x86_64, aarch64
- macOS x86_64, arm64
- Windows x86_64, arm64 ; arm64 for the windows 11 and Python 3.11-3.15 combination only.

**IMPORTANT:** Starting v87.9rc1 / v138.0rc1, `libEGL.so` (from mesa-libEGL, libglvnd, or your graphic
card's vendor e.g. NVidia) is required to be present on Linux hosts. This is associated
with the general change on Linux from X11 to Wayland, and from GTK3 to GTK4. Firefox started
to use it in 2021 (instead of the older `libGLX.so`), so it is likely newer Linux systems
already have it, but please check. skia-python v87.9 / v138.0+ supports hardware acceleration
via both GLX (still the default under X11) and EGL (newly added).

For Linux platforms, there must be OpenGL, libEGL and fontconfig installed.
Current Ubuntu needs `libegl1` (and `libegl-mesa0`); on older Ubuntu the package names are
`libglvnd0` and `libgl1-mesa-egl`:



```bash
apt-get install libfontconfig1 libgl1-mesa-glx libgl1-mesa-egl libegl1 libglvnd0 libgl1-mesa-dri
```

Or:

```bash
yum install fontconfig mesa-libGL mesa-libEGL libglvnd-egl mesa-dri-drivers
```

For unsupported environments, check the [build instructions](https://skia-python.github.io/skia-python/install.html).

## Examples

- [Showcase](https://github.com/skia-python/skia-python/blob/main/notebooks/Showcase.ipynb)
- [Canvas Overview](https://github.com/skia-python/skia-python/blob/main/notebooks/Canvas-Overview.ipynb)
- [Canvas Creation](https://github.com/skia-python/skia-python/blob/main/notebooks/Canvas-Creation.ipynb)
- [Path Overview](https://github.com/skia-python/skia-python/blob/main/notebooks/Path-Overview.ipynb)
- [Paint Overview](https://github.com/skia-python/skia-python/blob/main/notebooks/Paint-Overview.ipynb)
- [Python Image I/O](https://github.com/skia-python/skia-python/blob/main/notebooks/Python-Image-IO.ipynb)
- [Drawing Texts](https://github.com/skia-python/skia-python/blob/main/notebooks/Drawing-Texts.ipynb)

## Documentation

https://skia-python.github.io/skia-python

- [Tutorial](https://skia-python.github.io/skia-python/tutorial/)
- [Reference](https://skia-python.github.io/skia-python/reference.html)

- For breaking changes and tips on migration from `m87`: See [Migration Guide](Migration_from_v8x_to_v13x.md), also
  see the detailed changes below, especially [README.m116](relnotes/README.m116.md).

- For information about changes after `m116`: [README.m116](relnotes/README.m116.md),
  [README.m117](relnotes/README.m117.md), [README.m118](relnotes/README.m118.md), [README.m119](relnotes/README.m119.md),
  [README.m120](relnotes/README.m120.md), [README.m121](relnotes/README.m121.md), [README.m122](relnotes/README.m122.md),
  [README.m123](relnotes/README.m123.md), [README.m124](relnotes/README.m124.md), [README.m125](relnotes/README.m125.md),
  [README.m126](relnotes/README.m126.md), [README.m127](relnotes/README.m127.md), [README.m128](relnotes/README.m128.md),
  [README.m129](relnotes/README.m129.md), [README.m130](relnotes/README.m130.md), [README.m131](relnotes/README.m131.md),
  [README.m132](relnotes/README.m132.md), [README.m133](relnotes/README.m133.md), [README.m134](relnotes/README.m134.md),
  [README.m135](relnotes/README.m135.md), [README.m136](relnotes/README.m136.md), [README.m137](relnotes/README.m137.md),
  [README.m138](relnotes/README.m138.md), [README.m139](relnotes/README.m139.md), [README.m140](relnotes/README.m140.md),
  [README.m141](relnotes/README.m141.md), [README.m142](relnotes/README.m142.md), [README.m143](relnotes/README.m143.md),
  [README.m144](relnotes/README.m144.md).

## Contributing

Feel free to [post an issue](https://github.com/skia-python/skia-python/issues) or [PR](https://github.com/skia-python/skia-python/pulls).
