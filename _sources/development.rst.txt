Development
===========

Python binding design
---------------------

`skia-python` is built upon `pybind11`_. Python bindings are mostly plain
conversion from `Skia C++ API`_. Function names remain the same.

.. _pybind11: https://github.com/pybind/pybind11
.. _Skia C++ API: https://api.skia.org/

A few differences are:

- All bindings reside in ``skia`` module.
- Removes class name prefix ``Sk``; e.g., ``SkCanvas`` is ``skia.Canvas``.
- Some method signatures adapt to Python style; e.g.,

  - :py:meth:`skia.Surface.__init__` accepts additional args.
  - :py:class:`skia.Paint` is implicitly convertible from ``dict``.
  - Args taking `void* ptr` use buffer protocol.
  - Methods returning a raw memory address wrap in ``memoryview``.

- NumPy export method is added to ``Surface``, ``Canvas``, ``Image``.
- Buffer protocol support in ``Bitmap``, ``Pixmap``.


Contributing
------------

Development happens at `GitHub`_. Feel free to post to `issues`_ or make `PR`_.

.. _GitHub: https://github.com/skia-python/skia-python
.. _issues: https://github.com/skia-python/skia-python/issues
.. _PR: https://github.com/skia-python/skia-python/pulls
