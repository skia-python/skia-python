Install
=======

PyPI distribution
-----------------

Binary package is available on PyPI:

.. code-block:: bash

    pip install skia-python

Supported platforms:

- Linux x86_64, aarch64
- macOS x86_64, arm64
- Windows x86_64

For Linux platforms, there must be OpenGL and fontconfig installed:

.. code-block:: bash

    apt-get install libfontconfig1 libgl1-mesa-glx libgl1-mesa-dri

Or:

.. code-block:: bash

    yum install fontconfig mesa-libGL mesa-dri-drivers

For unsupported platforms, check the `Build instruction`_.

The Linux wheels include their own copy of fontconfig in ``skia_python.libs``.
If your system fontconfig is strictly newer, you can make skia-python use it
instead, so that it reads the same configuration and font cache as the rest of
your system. Install ``patchelf`` (for example with ``pip install patchelf``),
then run:

.. code-block:: bash

    SKIA_SO=$(python -c 'import skia; print(skia.__file__)')
    # Prints the name of the included library, such as libfontconfig-dcb2ce6c.so.1.12.0
    patchelf --print-needed "$SKIA_SO" | grep fontconfig
    # Replace that name with the system library
    patchelf --replace-needed libfontconfig-dcb2ce6c.so.1.12.0 libfontconfig.so.1 "$SKIA_SO"
    # Check that fontconfig now loads from a system directory
    ldd "$SKIA_SO" | grep fontconfig

Upgrading skia-python undoes this change.


Build instruction
-----------------

First, clone the repo.

.. code-block:: bash

    git clone --recursive https://github.com/skia-python/skia-python.git
    cd skia-python

The repository bundles `skia` and its build tools (`depot_tools`) as submodules.
`skia` must be built first to build Python binding.

The actual CI build procedures are implemented in `scripts/build_${ os }.sh`.

For building skia, there are a few compile-time requirements:

- Run-time type information (RTTI) must be enabled.

  - ``-frtti`` flag for ``clang`` or ``gcc``
  - ``/GR`` flag for ``cl.exe`` (Visual C++)

- ``/MD`` flag must be set for ``cl.exe`` (Visual C++)
- ``gn`` args should include ``skia_enable_tools = true`` to build ``skia.h`` header.

For detailed Skia build instructions, check `the official page`_.

.. _the official page: https://skia.org/

- `Linux`_
- `macOS`_
- `Windows`_

Linux
^^^^^

Prerequisites:

- Python 3 (build time only)
- GLIBC >= 2.17
- fontconfig >= 2.10.93
- OpenGL


Install dependencies:

.. code-block:: bash

    apt-get install -y libfontconfig1-dev libgl-dev libgl1-mesa-dri

or:

.. code-block:: bash

    yum install -y fontconfig-devel mesa-libGL-devel mesa-dri-drivers


Set up ``PATH`` to the ``depot_tools``. build skia library. At this point,
``python`` executable should be python 3. Note the build tools require
relatively new glibc and python 3.

.. code-block:: bash

    export PATH="$PWD/depot_tools:$PATH"
    cd skia
    python tools/git-sync-deps
    bin/gn gen out/Release --args='is_official_build=true skia_enable_tools=true skia_use_system_libjpeg_turbo=false skia_use_system_libwebp=false skia_use_system_libpng=false skia_use_system_icu=false skia_use_system_harfbuzz=false extra_cflags_cc=["-frtti"] extra_ldflags=["-lrt"]'
    ninja -C out/Release skia skia.h
    cd ..

Then, build the skia python binding. At this point, ``python`` should be set to
the desired version.

.. code-block:: bash

    export SKIA_PATH=$PWD/skia
    export SKIA_OUT_PATH=$SKIA_PATH/out/Release
    python -m pip install pybind11 numpy wheel
    python setup.py bdist_wheel

macOS
^^^^^

Prerequisites:

- Xcode Command Line Tools

Set up ``PATH`` to the ``depot_tools``, and build skia library.

.. code-block:: bash

    export PATH="$PWD/depot_tools:$PATH"
    cd skia
    python3 tools/git-sync-deps
    bin/gn gen out/Release --args='is_official_build=true skia_enable_tools=true skia_use_system_libjpeg_turbo=false skia_use_system_libwebp=false skia_use_system_libpng=false skia_use_system_icu=false skia_use_system_harfbuzz=false extra_cflags_cc=["-frtti"]'
    ninja -C out/Release skia skia.h
    cd ..

Then, build the skia-python binding. Here, ``python`` should be set to
the desired version.

.. code-block:: bash

    export SKIA_PATH=$PWD/skia
    export SKIA_OUT_PATH=$SKIA_PATH/out/Release
    python -m pip install pybind11 numpy wheel
    python setup.py bdist_wheel

Windows
^^^^^^^

Prerequisites:

- Python 3 (build time only)
- Visual C++ version that supports C++17

Windows binary can be built using the generic steps above.

.. code-block:: powershell

    $env:Path += ";$pwd\depot_tools"

    cd skia
    python tools\git-sync-deps
    bin\gn gen out\Release --args="is_official_build=true skia_enable_tools=true skia_use_system_libjpeg_turbo=false skia_use_system_libwebp=false skia_use_system_libpng=false skia_use_system_icu=false skia_use_system_harfbuzz=false skia_use_system_expat=false skia_use_system_zlib=false extra_cflags_cc=[\"/GR\", \"/EHsc\", \"/MD\"] target_cpu=\"x86_64\""
    ninja -C out\Release skia skia.h
    cd ..

    python -m pip install pybind11 numpy wheel
    python setup.py bdist_wheel

Testing
-------

.. code-block:: bash

    python -m pip install pytest numpy glfw
    python -m pytest tests

Alternatively, use ``tox`` to run tests under various python versions.

.. code-block:: bash

    export SKIA_PATH=$PWD/skia
    export SKIA_OUT_PATH=$SKIA_PATH/out/Release
    tox


Documentation
-------------

Once skia-python is installed, sphinx documentation can be built:

.. code-block:: bash

    python -m pip install sphinx sphinx-rtd-theme
    python setup.py build_sphinx
