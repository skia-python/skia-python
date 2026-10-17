#include "common.h"
#include <include/core/SkRSXform.h>
#include <include/core/SkPoint3.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/iostream.h>

// Static variables must be declared.
constexpr int SkMatrix::kMScaleX;
constexpr int SkMatrix::kMSkewX;
constexpr int SkMatrix::kMTransX;
constexpr int SkMatrix::kMSkewY;
constexpr int SkMatrix::kMScaleY;
constexpr int SkMatrix::kMTransY;
constexpr int SkMatrix::kMPersp0;
constexpr int SkMatrix::kMPersp1;
constexpr int SkMatrix::kMPersp2;
constexpr int SkMatrix::kAScaleX;
constexpr int SkMatrix::kASkewY;
constexpr int SkMatrix::kASkewX;
constexpr int SkMatrix::kAScaleY;
constexpr int SkMatrix::kATransX;
constexpr int SkMatrix::kATransY;

typedef py::array_t<SkScalar, py::array::c_style | py::array::forcecast> NumPy;


SkScalar GetItem(const SkMatrix& m, int index) {
    if (index < 0 || 9 <= index)
        throw std::out_of_range("Invalid index");
    return m[index];
}

void SetItem(SkMatrix& m, int index, SkScalar value) {
    if (index < 0 || 9 <= index)
        throw std::out_of_range("Invalid index");
    m[index] = value;
}

void initMatrix(py::module &m) {
/* SkApplyPerspectiveClip removed in m143 */
/*
py::enum_<SkApplyPerspectiveClip>(m, "ApplyPerspectiveClip")
    .value("kNo", SkApplyPerspectiveClip::kNo,
        "Don't pre-clip the geometry before applying the (perspective) matrix.")
    .value("kYes", SkApplyPerspectiveClip::kYes,
        "Do pre-clip the geometry before applying the (perspective) matrix.")
    .export_values();
*/

// Matrix
py::class_<SkMatrix> matrix(m, "Matrix", R"docstring(
    :py:class:`Matrix` holds a 3x3 matrix for transforming coordinates.

    This allows mapping :py:class:`Point` and vectors with translation, scaling,
    skewing, rotation, and perspective.

    :py:class:`Matrix` elements are in row major order. :py:class:`Matrix` does
    not have a constructor, so it must be explicitly initialized.
    :py:meth:`setIdentity` initializes :py:class:`Matrix` so it has no effect.
    :py:meth:`setTranslate`, :py:meth:`setScale`, :py:meth:`setSkew`,
    :py:meth:`setRotate`, :py:meth:`set9` and :py:meth:`setAll` initializes all
    :py:class:`Matrix` elements with the corresponding mapping.

    :py:class:`Matrix` includes a hidden variable that classifies the type of
    matrix to improve performance. :py:class:`Matrix` is not thread safe unless
    getType() is called first.
    )docstring");

py::enum_<SkMatrix::ScaleToFit>(matrix, "ScaleToFit")
    .value("kFill_ScaleToFit", SkMatrix::ScaleToFit::kFill_ScaleToFit,
        "scales in x and y to fill destination SkRect")
    .value("kStart_ScaleToFit", SkMatrix::ScaleToFit::kStart_ScaleToFit,
        "scales and aligns to left and top")
    .value("kCenter_ScaleToFit", SkMatrix::ScaleToFit::kCenter_ScaleToFit,
        "scales and aligns to center")
    .value("kEnd_ScaleToFit", SkMatrix::ScaleToFit::kEnd_ScaleToFit,
        "scales and aligns to right and bottom")
    .export_values();

py::enum_<SkMatrix::TypeMask>(matrix, "TypeMask")
    .value("kIdentity_Mask", SkMatrix::TypeMask::kIdentity_Mask,
        "identity SkMatrix; all bits clear")
    .value("kTranslate_Mask", SkMatrix::TypeMask::kTranslate_Mask,
        "translation SkMatrix")
    .value("kScale_Mask", SkMatrix::TypeMask::kScale_Mask,
        "scale SkMatrix")
    .value("kAffine_Mask", SkMatrix::TypeMask::kAffine_Mask,
        "skew or rotate SkMatrix")
    .value("kPerspective_Mask", SkMatrix::TypeMask::kPerspective_Mask,
        "perspective SkMatrix")
    .export_values();

matrix
    .def(py::init<>(),
        R"docstring(
        Creates an identity :py:class:`Matrix`::

            | 1 0 0 |
            | 0 1 0 |
            | 0 0 1 |
        )docstring")
    .def(py::init(
        [] (NumPy array) {
            auto info = array.request();
            size_t size = (info.ndim) ? info.shape[0] * info.strides[0] : 0;
            if (size != (sizeof(SkScalar) * 9))
                throw std::runtime_error("Incompatible array size");
            SkMatrix matrix;
            matrix.set9(reinterpret_cast<SkScalar*>(info.ptr));
            return matrix;
        }),
        R"docstring(
        Creates a :py:class:`Matrix` from 3x3 float32 NumPy array.
        )docstring",
        py::arg("array"))
    .def("getType", &SkMatrix::getType,
        R"docstring(
        Returns a bit field describing the transformations the matrix may
        perform.

        The bit field is computed conservatively, so it may include false
        positives. For example, when kPerspective_Mask is set, all other bits
        are set.

        :return: kIdentity_Mask, or combinations of: kTranslate_Mask,
            kScale_Mask, kAffine_Mask, kPerspective_Mask
        )docstring")
    .def("isIdentity", &SkMatrix::isIdentity,
        R"docstring(
        Returns true if :py:class:`Matrix` is identity.

        Identity matrix is::

            | 1 0 0 |
            | 0 1 0 |
            | 0 0 1 |

        :return: true if :py:class:`Matrix` has no effect
        )docstring")
    .def("isScaleTranslate", &SkMatrix::isScaleTranslate,
        R"docstring(
        Returns true if :py:class:`Matrix` at most scales and translates.

        :py:class:`Matrix` may be identity, contain only scale elements, only
        translate elements, or both. :py:class:`Matrix` form is::

            | scale-x    0    translate-x |
            |    0    scale-y translate-y |
            |    0       0         1      |

        :return: true if :py:class:`Matrix` is identity; or scales, translates,
            or both
        )docstring")
    .def("isTranslate", &SkMatrix::isTranslate,
        R"docstring(
        Returns true if :py:class:`Matrix` is identity, or translates.

        :py:class:`Matrix` form is::

            | 1 0 translate-x | | 0 1 translate-y | | 0 0 1 |

        :return: true if :py:class:`Matrix` is identity, or translates
        )docstring")
    .def("rectStaysRect", &SkMatrix::rectStaysRect,
        R"docstring(
        Returns true SkMatrix maps SkRect to another SkRect.

        If true, :py:class:`Matrix` is identity, or scales, or rotates a
        multiple of 90 degrees, or mirrors on axes. In all cases,
        :py:class:`Matrix` may also have translation. :py:class:`Matrix` form is
        either::

            | scale-x    0    translate-x |
            |    0    scale-y translate-y |
            |    0       0         1      |

        or::

            |    0     rotate-x translate-x |
            | rotate-y    0     translate-y |
            |    0        0          1      |
            for non-zero values of scale-x, scale-y, rotate-x, and rotate-y.

        Also called :py:meth:`preservesAxisAlignment`; use the one that provides
        better inline documentation.

        :return: true if :py:class:`Matrix` maps one :py:class:`Rect` into
            another
        )docstring")
    .def("preservesAxisAlignment", &SkMatrix::preservesAxisAlignment,
        R"docstring(
        Returns true :py:class:`Matrix` maps :py:class:`Rect` to another
        :py:class:`Rect`.

        If true, :py:class:`Matrix` is identity, or scales, or rotates a
        multiple of 90 degrees, or mirrors on axes. In all cases,
        :py:class:`Matrix` may also have translation. :py:class:`Matrix` form is
        either::

            | scale-x    0    translate-x |
            |    0    scale-y translate-y |
            |    0       0         1      |

        or::

            |    0     rotate-x translate-x |
            | rotate-y    0     translate-y |
            |    0        0          1      |

        for non-zero values of scale-x, scale-y, rotate-x, and rotate-y.

        Also called :py:meth:`rectStaysRect`; use the one that provides better
        inline documentation.

        :return: true if :py:class:`Matrix` maps one :py:class:`Rect` into
            another
        )docstring")
    .def("hasPerspective", &SkMatrix::hasPerspective,
        R"docstring(
        Returns true if the matrix contains perspective elements.

        :py:class:`Matrix` form is::

            | – – – |
            | – – – |
            | perspective-x perspective-y perspective-scale |

        where perspective-x or perspective-y is non-zero, or perspective-scale
        is not one. All other elements may have any value.

        :return: true if :py:class:`Matrix` is in most general form
        )docstring")
    .def("isSimilarity", &SkMatrix::isSimilarity,
        R"docstring(
        Returns true if :py:class:`Matrix` contains only translation, rotation,
        reflection, and uniform scale.

        Returns false if :py:class:`Matrix` contains different scales, skewing,
        perspective, or degenerate forms that collapse to a line or point.

        Describes that the :py:class:`Matrix` makes rendering with and without
        the matrix are visually alike; a transformed circle remains a circle.
        Mathematically, this is referred to as similarity of a Euclidean space,
        or a similarity transformation.

        Preserves right angles, keeping the arms of the angle equal lengths.

        :param float tol: to be deprecated
        :return: true if :py:class:`Matrix` only rotates, uniformly scales,
            translates
        )docstring",
        py::arg("tol") = SK_ScalarNearlyZero)
    .def("preservesRightAngles", &SkMatrix::preservesRightAngles,
        R"docstring(
        Returns true if :py:class:`Matrix` contains only translation, rotation,
        reflection, and scale.

        Scale may differ along rotated axes. Returns false if :py:class:`Matrix`
        skewing, perspective, or degenerate forms that collapse to a line or
        point.

        Preserves right angles, but not requiring that the arms of the angle
        retain equal lengths.

        :param float tol: to be deprecated
        :return: true if :py:class:`Matrix` only rotates, scales, translates
        )docstring",
        py::arg("tol") = SK_ScalarNearlyZero)
    .def("__getitem__", &GetItem,
        R"docstring(
        Returns one matrix value.

        Asserts if index is out of range and SK_DEBUG is defined.

        :param int index: one of: kMScaleX, kMSkewX, kMTransX, kMSkewY,
            kMScaleY, kMTransY, kMPersp0, kMPersp1, kMPersp2
        :return: value corresponding to index
        )docstring",
        py::arg("index"), py::is_operator())
    .def("get", &GetItem,
        R"docstring(
        Returns one matrix value.

        Asserts if index is out of range and SK_DEBUG is defined.

        :param int index: one of: kMScaleX, kMSkewX, kMTransX, kMSkewY,
            kMScaleY, kMTransY, kMPersp0, kMPersp1, kMPersp2
        :return: value corresponding to index
        )docstring",
        py::arg("index"))
    .def("rc", &SkMatrix::rc,
        R"docstring(
        Returns one matrix value from a particular row/column. Asserts if index
        is out of range and SK_DEBUG is defined.

        :param r:  matrix row to fetch
        :param c:  matrix column to fetch
        :return:   value at the given matrix position
        )docstring",
        py::arg("r"), py::arg("c"))
    .def("getScaleX", &SkMatrix::getScaleX,
        R"docstring(
        Returns scale factor multiplied by x-axis input, contributing to x-axis
        output.

        With :py:meth:`mapPoints`, scales :py:class:`Point` along the x-axis.

        :return: horizontal scale factor
        )docstring")
    .def("getScaleY", &SkMatrix::getScaleY,
        R"docstring(
        Returns scale factor multiplied by y-axis input, contributing to y-axis
        output.

        With :py:meth:`mapPoints`, scales :py:class:`Point` along the y-axis.

        :return: vertical scale factor
        )docstring")
    .def("getSkewY", &SkMatrix::getSkewY,
        R"docstring(
        Returns scale factor multiplied by x-axis input, contributing to y-axis
        output.

        With :py:meth:`mapPoints`, skews :py:class:`Point` along the y-axis.
        Skewing both axes can rotate :py:class:`Point`.

        :return: vertical skew factor
        )docstring")
    .def("getSkewX", &SkMatrix::getSkewX,
        R"docstring(
        Returns scale factor multiplied by y-axis input, contributing to x-axis
        output.

        With :py:meth:`mapPoints`, skews :py:class:`Point` along the x-axis.
        Skewing both axes can rotate :py:class:`Point`.

        :return: horizontal scale factor
        )docstring")
    .def("getTranslateX", &SkMatrix::getTranslateX,
        R"docstring(
        Returns translation contributing to x-axis output.

        With :py:meth:`mapPoints`, moves :py:class:`Point` along the x-axis.

        :return: horizontal translation factor
        )docstring")
    .def("getTranslateY", &SkMatrix::getTranslateY,
        R"docstring(
        Returns translation contributing to y-axis output.

        With :py:meth:`mapPoints`, moves :py:class:`Point` along the y-axis.

        :return: vertical translation factor
        )docstring")
    .def("getPerspX", &SkMatrix::getPerspX,
        R"docstring(
        Returns factor scaling input x-axis relative to input y-axis.

        :return: input x-axis perspective factor
        )docstring")
    .def("getPerspY", &SkMatrix::getPerspY,
        R"docstring(
        Returns factor scaling input y-axis relative to input x-axis.

        :return: input y-axis perspective factor
        )docstring")
    .def("__setitem__", &SetItem,
        R"docstring(
        Returns writable :py:class:`Matrix` value.

        Asserts if index is out of range and SK_DEBUG is defined. Clears
        internal cache anticipating that caller will change :py:class:`Matrix`
        value.

        Next call to read :py:class:`Matrix` state may recompute cache;
        subsequent writes to :py:class:`Matrix` value must be followed by
        :py:meth:`dirtyMatrixTypeCache`.

        :param int index: one of: kMScaleX, kMSkewX, kMTransX, kMSkewY,
            kMScaleY, kMTransY, kMPersp0, kMPersp1, kMPersp2
        :param value: value to set
        )docstring",
        py::arg("index"), py::arg("value"), py::is_operator())
    .def("set", &SetItem,
        R"docstring(
        Returns writable :py:class:`Matrix` value.

        Asserts if index is out of range and SK_DEBUG is defined. Clears
        internal cache anticipating that caller will change :py:class:`Matrix`
        value.

        Next call to read :py:class:`Matrix` state may recompute cache;
        subsequent writes to :py:class:`Matrix` value must be followed by
        :py:meth:`dirtyMatrixTypeCache`.

        :param int index: one of: kMScaleX, kMSkewX, kMTransX, kMSkewY,
            kMScaleY, kMTransY, kMPersp0, kMPersp1, kMPersp2
        :param value: value to set
        )docstring",
        py::arg("index"), py::arg("value"))
    .def("setScaleX", &SkMatrix::setScaleX,
        R"docstring(
        Sets horizontal scale factor.

        :param float v: horizontal scale factor to store
        )docstring",
        py::arg("v"))
    .def("setScaleY", &SkMatrix::setScaleY,
        R"docstring(
        Sets vertical scale factor.

        :param float v: vertical scale factor to store
        )docstring",
        py::arg("v"))
    .def("setSkewY", &SkMatrix::setSkewY,
        R"docstring(
        Sets vertical skew factor.

        :param v: vertical skew factor to store
        )docstring",
        py::arg("v"))
    .def("setSkewX", &SkMatrix::setSkewX,
        R"docstring(
        Sets horizontal skew factor.

        :param float v: horizontal skew factor to store
        )docstring",
        py::arg("v"))
    .def("setTranslateX", &SkMatrix::setTranslateX,
        R"docstring(
        Sets horizontal translation.

        :param float v: horizontal translation to store
        )docstring",
        py::arg("v"))
    .def("setTranslateY", &SkMatrix::setTranslateY,
        R"docstring(
        Sets vertical translation.

        :param float v: vertical translation to store
        )docstring",
        py::arg("v"))
    .def("setPerspX", &SkMatrix::setPerspX,
        R"docstring(
        Sets input x-axis perspective factor, which causes mapXY() to vary input
        x-axis values inversely proportional to input y-axis values.

        :param float v: perspective factor
        )docstring",
        py::arg("v"))
    .def("setPerspY", &SkMatrix::setPerspY,
        R"docstring(
        Sets input y-axis perspective factor, which causes mapXY() to vary input
        y-axis values inversely proportional to input x-axis values.

        :param float v: perspective factor
        )docstring",
        py::arg("v"))
    .def("setAll", &SkMatrix::setAll,
        R"docstring(
        Sets all values from parameters.

        Sets matrix to::

            | scaleX skewX transX |
            | skewY scaleY transY |
            | persp0 persp1 persp2 |

        :param float scaleX:  horizontal scale factor to store
        :param float skewX:   horizontal skew factor to store
        :param float transX:  horizontal translation to store
        :param float skewY:   vertical skew factor to store
        :param float scaleY:  vertical scale factor to store
        :param float transY:  vertical translation to store
        :param float persp0:  input x-axis values perspective factor to store
        :param float persp1:  input y-axis values perspective factor to store
        :param float persp2:  perspective scale factor to store
        )docstring",
        py::arg("scaleX"), py::arg("skewX"), py::arg("transX"),
        py::arg("skewY"), py::arg("scaleY"), py::arg("transY"),
        py::arg("persp0"), py::arg("persp1"), py::arg("persp2"))
    .def("get9",
        [] (const SkMatrix& matrix) {
            std::vector<SkScalar> buffer(9);
            matrix.get9(&buffer[0]);
            return buffer;
        },
        R"docstring(
        Returns nine scalar values contained by :py:class:`Matrix` into list,
        in member value ascending order: kMScaleX, kMSkewX, kMTransX, kMSkewY,
        kMScaleY, kMTransY, kMPersp0, kMPersp1, kMPersp2.
        )docstring")
    .def("set9",
        [] (SkMatrix& matrix, const std::vector<SkScalar> values) {
            if (values.size() != 9)
                throw std::runtime_error("values must have 9 elements");
            return matrix.set9(&values[0]);
        },
        R"docstring(
        Sets :py:class:`Matrix` to nine scalar values in values, in member value
        ascending order: kMScaleX, kMSkewX, kMTransX, kMSkewY, kMScaleY,
        kMTransY, kMPersp0, kMPersp1, kMPersp2.

        Sets matrix to::

            | values[0] values[1] values[2] |
            | values[3] values[4] values[5] |
            | values[6] values[7] values[8] |

        In the future, set9 followed by get9 may not return the same values.
        Since :py:class:`Matrix` maps non-homogeneous coordinates, scaling all
        nine values produces an equivalent transformation, possibly improving
        precision.

        :param values: nine scalar values
        )docstring",
        py::arg("buffer"))
    .def("reset", &SkMatrix::reset,
        R"docstring(
        Sets :py:class:`Matrix` to identity; which has no effect on mapped
        :py:class:`Point`.

        Sets :py:class:`Matrix` to::

            | 1 0 0 |
            | 0 1 0 |
            | 0 0 1 |

        Also called :py:meth:`setIdentity`; use the one that provides better
        inline documentation.
        )docstring")
    .def("setIdentity", &SkMatrix::setIdentity,
        R"docstring(
        Sets :py:class:`Matrix` to identity; which has no effect on mapped
        :py:class:`Point`.

        Sets :py:class:`Matrix` to::

            | 1 0 0 |
            | 0 1 0 |
            | 0 0 1 |

        Also called :py:meth:`reset`; use the one that provides better inline
        documentation.
        )docstring")
    .def("setTranslate",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::setTranslate),
        R"docstring(
        Sets :py:class:`Matrix` to translate by (dx, dy).

        :dx: horizontal translation
        :dy: vertical translation
        )docstring",
        py::arg("dx"), py::arg("dy"))
    .def("setTranslate",
        py::overload_cast<const SkVector&>(&SkMatrix::setTranslate),
        R"docstring(
        Sets :py:class:`Matrix` to translate by (v.fX, v.fY).

        :v: vector containing horizontal and vertical
            translation
        )docstring",
        py::arg("v"))
    .def("setScale",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::setScale),
        R"docstring(
        Sets :py:class:`Matrix` to scale by sx and sy, about a pivot point at
        (px, py).

        The pivot point is unchanged when mapped with :py:class:`Matrix`.

        :sx: horizontal scale factor
        :sy: vertical scale factor
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("sx"), py::arg("sy"), py::arg("px"), py::arg("py"))
    .def("setScale",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::setScale),
        R"docstring(
        Sets :py:class:`Matrix` to scale by sx and sy about at pivot point at
        (0, 0).

        :sx: horizontal scale factor
        :sy: vertical scale factor
        )docstring",
        py::arg("sx"), py::arg("sy"))
    .def("setRotate",
        py::overload_cast<SkScalar, SkScalar, SkScalar>(&SkMatrix::setRotate),
        R"docstring(
        Sets :py:class:`Matrix` to rotate by degrees about a pivot point at (px,
        py).

        The pivot point is unchanged when mapped with :py:class:`Matrix`.

        Positive degrees rotates clockwise.

        :degrees: angle of axes relative to upright axes
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("degrees"), py::arg("px"), py::arg("py"))
    .def("setRotate", py::overload_cast<SkScalar>(&SkMatrix::setRotate),
        R"docstring(
        Sets :py:class:`Matrix` to rotate by degrees about a pivot point at (0,
        0).

        Positive degrees rotates clockwise.

        :degrees: angle of axes relative to upright axes
        )docstring",
        py::arg("degrees"))
    .def("setSinCos",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::setSinCos),
        R"docstring(
        Sets :py:class:`Matrix` to rotate by sinValue and cosValue, about a
        pivot point at (px, py).

        The pivot point is unchanged when mapped with :py:class:`Matrix`.

        Vector (sinValue, cosValue) describes the angle of rotation relative to
        (0, 1). Vector length specifies scale.

        :sinValue: rotation vector x-axis component
        :cosValue: rotation vector y-axis component
        :px:  pivot on x-axis
        :py:  pivot on y-axis
        )docstring",
        py::arg("sinValue"), py::arg("cosValue"), py::arg("px"), py::arg("py"))
    .def("setSinCos",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::setSinCos),
        R"docstring(
        Sets :py:class:`Matrix` to rotate by sinValue and cosValue, about a
        pivot point at (0, 0).

        Vector (sinValue, cosValue) describes the angle of rotation relative to
        (0, 1). Vector length specifies scale.

        :sinValue: rotation vector x-axis component
        :cosValue: rotation vector y-axis component
        )docstring",
        py::arg("sinValue"), py::arg("cosValue"))
    .def("setRSXform", &SkMatrix::setRSXform,
        R"docstring(
        Sets :py:class:`Matrix` to rotate, scale, and translate using a
        compressed matrix form.

        Vector (rsxForm.fSSin, rsxForm.fSCos) describes the angle of rotation
        relative to (0, 1). Vector length specifies scale. Mapped point is
        rotated and scaled by vector, then translated by (rsxForm.fTx,
        rsxForm.fTy).

        :param skia.RSXform rsxForm: compressed :py:class:`RSXform` matrix
        :return: reference to :py:class:`Matrix`
        )docstring",
        py::arg("rsxForm"))
    .def("setSkew",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::setSkew),
        R"docstring(
        Sets :py:class:`Matrix` to skew by kx and ky, about a pivot point at
        (px, py).

        The pivot point is unchanged when mapped with :py:class:`Matrix`.

        :kx: horizontal skew factor
        :ky: vertical skew factor
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("kx"), py::arg("ky"), py::arg("px"), py::arg("py"))
    .def("setSkew", py::overload_cast<SkScalar, SkScalar>(&SkMatrix::setSkew),
        R"docstring(
        Sets :py:class:`Matrix` to skew by kx and ky, about a pivot point at
        (0, 0).

        :kx: horizontal skew factor
        :ky: vertical skew factor
        )docstring",
        py::arg("kx"), py::arg("ky"))
    .def("setConcat", &SkMatrix::setConcat,
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` a multiplied by
        :py:class:`Matrix` b.

        Either a or b may be this.

        Given::

                | A B C |      | J K L |
            a = | D E F |, b = | M N O |
                | G H I |      | P Q R |

        sets :py:class:`Matrix` to::

                    | A B C |   | J K L |   | AJ+BM+CP AK+BN+CQ AL+BO+CR |
            a * b = | D E F | * | M N O | = | DJ+EM+FP DK+EN+FQ DL+EO+FR |
                    | G H I |   | P Q R |   | GJ+HM+IP GK+HN+IQ GL+HO+IR |

        :param skia.Matrix a: :py:class:`Matrix` on left side of multiply
            expression
        :param skia.Matrix b: :py:class:`Matrix` on right side of multiply
            expression
        )docstring",
        py::arg("a"), py::arg("b"))
    .def("preTranslate", &SkMatrix::preTranslate,
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from translation (dx, dy).

        This can be thought of as moving the point to be mapped before applying
        :py:class:`Matrix`.

        Given::

                     | A B C |               | 1 0 dx |
            Matrix = | D E F |,  T(dx, dy) = | 0 1 dy |
                     | G H I |               | 0 0  1 |

        sets :py:class:`Matrix` to::

                                 | A B C | | 1 0 dx |   | A B A*dx+B*dy+C |
            Matrix * T(dx, dy) = | D E F | | 0 1 dy | = | D E D*dx+E*dy+F |
                                 | G H I | | 0 0  1 |   | G H G*dx+H*dy+I |

        :param dx: x-axis translation before applying :py:class:`Matrix`
        :param dy: y-axis translation before applying :py:class:`Matrix`
        )docstring",
        py::arg("dx"), py::arg("dy"))
    .def("preScale",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::preScale),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from scaling by (sx, sy) about pivot
        point (px, py).

        This can be thought of as scaling about a pivot point before applying
        :py:class:`Matrix`.

        Given::

                     | A B C |                       | sx  0 dx |
            Matrix = | D E F |,  S(sx, sy, px, py) = |  0 sy dy |
                     | G H I |                       |  0  0  1 |

        where::

            dx = px - sx * px
            dy = py - sy * py

        sets :py:class:`Matrix` to::

                                         | A B C | | sx  0 dx |   | A*sx B*sy A*dx+B*dy+C |
            Matrix * S(sx, sy, px, py) = | D E F | |  0 sy dy | = | D*sx E*sy D*dx+E*dy+F |
                                         | G H I | |  0  0  1 |   | G*sx H*sy G*dx+H*dy+I |

        :sx: horizontal scale factor
        :sy: vertical scale factor
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("sx"), py::arg("sy"), py::arg("px"), py::arg("py"))
    .def("preScale",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::preScale),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from scaling by (sx, sy) about pivot
        point (0, 0).

        This can be thought of as scaling about the origin before applying
        :py:class:`Matrix`.

        Given::

                     | A B C |               | sx  0  0 |
            Matrix = | D E F |,  S(sx, sy) = |  0 sy  0 |
                     | G H I |               |  0  0  1 |

        sets :py:class:`Matrix` to::

                                 | A B C | | sx  0  0 |   | A*sx B*sy C |
            Matrix * S(sx, sy) = | D E F | |  0 sy  0 | = | D*sx E*sy F |
                                 | G H I | |  0  0  1 |   | G*sx H*sy I |

        :sx: horizontal scale factor
        :sy: vertical scale factor
        )docstring",
        py::arg("sx"), py::arg("sy"))
    .def("preRotate",
        py::overload_cast<SkScalar, SkScalar, SkScalar>(&SkMatrix::preRotate),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from rotating by degrees about pivot
        point (px, py).

        This can be thought of as rotating about a pivot point before applying
        :py:class:`Matrix`.

        Positive degrees rotates clockwise.

        Given::

                     | A B C |                        | c -s dx |
            Matrix = | D E F |,  R(degrees, px, py) = | s  c dy |
                     | G H I |                        | 0  0  1 |

        where::

            c  = cos(degrees)
            s  = sin(degrees)
            dx =  s * py + (1 - c) * px
            dy = -s * px + (1 - c) * py

        sets :py:class:`Matrix` to::

                                          | A B C | | c -s dx |   | Ac+Bs -As+Bc A*dx+B*dy+C |
            Matrix * R(degrees, px, py) = | D E F | | s  c dy | = | Dc+Es -Ds+Ec D*dx+E*dy+F |
                                          | G H I | | 0  0  1 |   | Gc+Hs -Gs+Hc G*dx+H*dy+I |

        :degrees: angle of axes relative to upright axes
        :px:  pivot on x-axis
        :py:  pivot on y-axis
        )docstring",
        py::arg("degrees"), py::arg("px"), py::arg("py"))
    .def("preRotate",
        py::overload_cast<SkScalar>(&SkMatrix::preRotate),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from rotating by degrees about pivot
        point (0, 0).

        This can be thought of as rotating about the origin before applying
        :py:class:`Matrix`.

        Positive degrees rotates clockwise.

        Given::

                     | A B C |                        | c -s 0 |
            Matrix = | D E F |,  R(degrees, px, py) = | s  c 0 |
                     | G H I |                        | 0  0 1 |

        where::

            c  = cos(degrees)
            s  = sin(degrees)

        sets :py:class:`Matrix` to::

                                          | A B C | | c -s 0 |   | Ac+Bs -As+Bc C |
            Matrix * R(degrees, px, py) = | D E F | | s  c 0 | = | Dc+Es -Ds+Ec F |
                                          | G H I | | 0  0 1 |   | Gc+Hs -Gs+Hc I |

        :degrees: angle of axes relative to upright axes
        )docstring",
        py::arg("degrees"))
    .def("preSkew",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::preSkew),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from skewing by (kx, ky) about pivot
        point (px, py).

        This can be thought of as skewing about a pivot point before applying
        :py:class:`Matrix`.

        Given::

                     | A B C |                       |  1 kx dx |
            Matrix = | D E F |,  K(kx, ky, px, py) = | ky  1 dy |
                     | G H I |                       |  0  0  1 |

        where::

            dx = -kx * py
            dy = -ky * px

        sets :py:class:`Matrix` to::

                                         | A B C | |  1 kx dx |   | A+B*ky A*kx+B A*dx+B*dy+C |
            Matrix * K(kx, ky, px, py) = | D E F | | ky  1 dy | = | D+E*ky D*kx+E D*dx+E*dy+F |
                                         | G H I | |  0  0  1 |   | G+H*ky G*kx+H G*dx+H*dy+I |

        :kx: horizontal skew factor
        :ky: vertical skew factor
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("kx"), py::arg("ky"), py::arg("px"), py::arg("py"))
    .def("preSkew",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::preSkew),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` constructed from skewing by (kx, ky) about pivot
        point (0, 0).

        This can be thought of as skewing about the origin before applying
        :py:class:`Matrix`.

        Given::

                     | A B C |               |  1 kx 0 |
            Matrix = | D E F |,  K(kx, ky) = | ky  1 0 |
                     | G H I |               |  0  0 1 |

        sets :py:class:`Matrix` to::

                                 | A B C | |  1 kx 0 |   | A+B*ky A*kx+B C |
            Matrix * K(kx, ky) = | D E F | | ky  1 0 | = | D+E*ky D*kx+E F |
                                 | G H I | |  0  0 1 |   | G+H*ky G*kx+H I |

        :kx: horizontal skew factor
        :ky: vertical skew factor
        )docstring",
        py::arg("kx"), py::arg("ky"))
    .def("preConcat", &SkMatrix::preConcat,
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` multiplied by
        :py:class:`Matrix` other.

        This can be thought of mapping by other before applying
        :py:class:`Matrix`.

        Given::

                     | A B C |          | J K L |
            Matrix = | D E F |, other = | M N O |
                     | G H I |          | P Q R |

        sets :py:class:`Matrix` to::

                             | A B C |   | J K L |   | AJ+BM+CP AK+BN+CQ AL+BO+CR |
            Matrix * other = | D E F | * | M N O | = | DJ+EM+FP DK+EN+FQ DL+EO+FR |
                             | G H I |   | P Q R |   | GJ+HM+IP GK+HN+IQ GL+HO+IR |

        :param skia.Matrix other: :py:class:`Matrix` on right side of multiply
            expression
        )docstring",
        py::arg("other"))
    .def("postTranslate", &SkMatrix::postTranslate,
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from
        translation (dx, dy) multiplied by :py:class:`Matrix`.

        This can be thought of as moving the point to be mapped after applying
        :py:class:`Matrix`.

        Given::

                     | J K L |               | 1 0 dx |
            Matrix = | M N O |,  T(dx, dy) = | 0 1 dy |
                     | P Q R |               | 0 0  1 |

        sets :py:class:`Matrix` to::

                                 | 1 0 dx | | J K L |   | J+dx*P K+dx*Q L+dx*R |
            T(dx, dy) * Matrix = | 0 1 dy | | M N O | = | M+dy*P N+dy*Q O+dy*R |
                                 | 0 0  1 | | P Q R |   |      P      Q      R |

        :param float dx: x-axis translation after applying :py:class:`Matrix`
        :param float dy: y-axis translation after applying :py:class:`Matrix`
        )docstring",
        py::arg("dx"), py::arg("dy"))
    .def("postScale",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::postScale),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from scaling
        by (sx, sy) about pivot point (px, py), multiplied by
        :py:class:`Matrix`.

        This can be thought of as scaling about a pivot point after applying :py:class:`Matrix`.

        Given::

                     | J K L |                       | sx  0 dx |
            Matrix = | M N O |,  S(sx, sy, px, py) = |  0 sy dy |
                     | P Q R |                       |  0  0  1 |

        where::

            dx = px - sx * px
            dy = py - sy * py

        sets :py:class:`Matrix` to::

                                         | sx  0 dx | | J K L |   | sx*J+dx*P sx*K+dx*Q sx*L+dx+R |
            S(sx, sy, px, py) * Matrix = |  0 sy dy | | M N O | = | sy*M+dy*P sy*N+dy*Q sy*O+dy*R |
                                         |  0  0  1 | | P Q R |   |         P         Q         R |

        :sx: horizontal scale factor
        :sy: vertical scale factor
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("sx"), py::arg("sy"), py::arg("px"), py::arg("py"))
    .def("postScale",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::postScale),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from scaling
        by (sx, sy) about pivot point (0, 0), multiplied by :py:class:`Matrix`.

        This can be thought of as scaling about the origin after applying
        :py:class:`Matrix`.

        Given::

                     | J K L |               | sx  0  0 |
            Matrix = | M N O |,  S(sx, sy) = |  0 sy  0 |
                     | P Q R |               |  0  0  1 |

        sets :py:class:`Matrix` to::

                                 | sx  0  0 | | J K L |   | sx*J sx*K sx*L |
            S(sx, sy) * Matrix = |  0 sy  0 | | M N O | = | sy*M sy*N sy*O |
                                 |  0  0  1 | | P Q R |   |    P    Q    R |

        :sx: horizontal scale factor
        :sy: vertical scale factor
        )docstring",
        py::arg("sx"), py::arg("sy"))
    .def("postRotate",
        py::overload_cast<SkScalar, SkScalar, SkScalar>(&SkMatrix::postRotate),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from rotating
        by degrees about pivot point (px, py), multiplied by :py:class:`Matrix`.

        This can be thought of as rotating about a pivot point after applying
        :py:class:`Matrix`.

        Positive degrees rotates clockwise.

        Given::

                     | J K L |                        | c -s dx |
            Matrix = | M N O |,  R(degrees, px, py) = | s  c dy |
                     | P Q R |                        | 0  0  1 |

        where::

            c  = cos(degrees)
            s  = sin(degrees)
            dx =  s * py + (1 - c) * px
            dy = -s * px + (1 - c) * py

        sets :py:class:`Matrix` to::

                                          |c -s dx| |J K L|   |cJ-sM+dx*P cK-sN+dx*Q cL-sO+dx+R|
            R(degrees, px, py) * Matrix = |s  c dy| |M N O| = |sJ+cM+dy*P sK+cN+dy*Q sL+cO+dy*R|
                                          |0  0  1| |P Q R|   |         P          Q          R|

        :degrees: angle of axes relative to upright axes
        :px:  pivot on x-axis
        :py:  pivot on y-axis
        )docstring",
        py::arg("degrees"), py::arg("px"), py::arg("py"))
    .def("postRotate",
        py::overload_cast<SkScalar>(&SkMatrix::postRotate),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from rotating
        by degrees about pivot point (0, 0), multiplied by :py:class:`Matrix`.

        This can be thought of as rotating about the origin after applying
        :py:class:`Matrix`.

        Positive degrees rotates clockwise.

        Given::

                     | J K L |                        | c -s 0 |
            Matrix = | M N O |,  R(degrees, px, py) = | s  c 0 |
                     | P Q R |                        | 0  0 1 |

        where::

            c  = cos(degrees)
            s  = sin(degrees)

        sets :py:class:`Matrix` to::

                                          | c -s dx | | J K L |   | cJ-sM cK-sN cL-sO |
            R(degrees, px, py) * Matrix = | s  c dy | | M N O | = | sJ+cM sK+cN sL+cO |
                                          | 0  0  1 | | P Q R |   |     P     Q     R |

        :degrees: angle of axes relative to upright axes
        )docstring",
        py::arg("degrees"))
    .def("postSkew",
        py::overload_cast<SkScalar, SkScalar, SkScalar, SkScalar>(
            &SkMatrix::postSkew),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from skewing
        by (kx, ky) about pivot point (px, py), multiplied by
        :py:class:`Matrix`.

        This can be thought of as skewing about a pivot point after applying
        :py:class:`Matrix`.

        Given::

                     | J K L |                       |  1 kx dx |
            Matrix = | M N O |,  K(kx, ky, px, py) = | ky  1 dy |
                     | P Q R |                       |  0  0  1 |

        where::

            dx = -kx * py
            dy = -ky * px

        sets :py:class:`Matrix` to::

                                         | 1 kx dx| |J K L|   |J+kx*M+dx*P K+kx*N+dx*Q L+kx*O+dx+R|
            K(kx, ky, px, py) * Matrix = |ky  1 dy| |M N O| = |ky*J+M+dy*P ky*K+N+dy*Q ky*L+O+dy*R|
                                         | 0  0  1| |P Q R|   |          P           Q           R|

        :kx: horizontal skew factor
        :ky: vertical skew factor
        :px: pivot on x-axis
        :py: pivot on y-axis
        )docstring",
        py::arg("kx"), py::arg("ky"), py::arg("px"), py::arg("py"))
    .def("postSkew",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::postSkew),
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` constructed from skewing
        by (kx, ky) about pivot point (0, 0), multiplied by :py:class:`Matrix`.

        This can be thought of as skewing about the origin after applying
        :py:class:`Matrix`.

        Given::

                     | J K L |               |  1 kx 0 |
            Matrix = | M N O |,  K(kx, ky) = | ky  1 0 |
                     | P Q R |               |  0  0 1 |

        sets :py:class:`Matrix` to::

                                 |  1 kx 0 | | J K L |   | J+kx*M K+kx*N L+kx*O |
            K(kx, ky) * Matrix = | ky  1 0 | | M N O | = | ky*J+M ky*K+N ky*L+O |
                                 |  0  0 1 | | P Q R |   |      P      Q      R |

        :kx: horizontal skew factor
        :ky: vertical skew factor
        )docstring",
        py::arg("kx"), py::arg("ky"))
    .def("postConcat", &SkMatrix::postConcat,
        R"docstring(
        Sets :py:class:`Matrix` to :py:class:`Matrix` other multiplied by
        :py:class:`Matrix`.

        This can be thought of mapping by other after applying
        :py:class:`Matrix`.

        Given::

                     | J K L |           | A B C |
            Matrix = | M N O |,  other = | D E F |
                     | P Q R |           | G H I |

        sets :py:class:`Matrix` to::

                             | A B C |   | J K L |   | AJ+BM+CP AK+BN+CQ AL+BO+CR |
            other * Matrix = | D E F | * | M N O | = | DJ+EM+FP DK+EN+FQ DL+EO+FR |
                             | G H I |   | P Q R |   | GJ+HM+IP GK+HN+IQ GL+HO+IR |

        :param skia.Matrix other: :py:class:`Matrix` on left side of multiply
            expression
        )docstring",
        py::arg("other"))
    .def("setRectToRect", &SkMatrix::setRectToRect,
        R"docstring(
        Sets :py:class:`Matrix` to scale and translate src :py:class:`Rect` to
        dst :py:class:`Rect`.

        stf selects whether mapping completely fills dst or preserves the aspect
        ratio, and how to align src within dst. Returns false if src is empty,
        and sets :py:class:`Matrix` to identity. Returns true if dst is empty,
        and sets :py:class:`Matrix` to:

            | 0 0 0 |
            | 0 0 0 |
            | 0 0 1 |

        :param skia.Rect src: :py:class:`Rect` to map from
        :param skia.Rect dst: :py:class:`Rect` to map to
        :param skia.Matrix.ScaleToFit stf: mapping method
        :return: true if :py:class:`Matrix` can represent :py:class:`Rect`
            mapping
        )docstring",
        py::arg("src"), py::arg("dst"), py::arg("stf"))
    .def("setPolyToPoly",
        [] (SkMatrix& matrix, const std::vector<SkPoint>& src,
            const std::vector<SkPoint>& dst) {
            if (src.size() != dst.size())
                throw std::runtime_error("src and dst must have the same size");
            if (src.empty())
                return matrix.setPolyToPoly({nullptr, 0}, {nullptr, 0});
            return matrix.setPolyToPoly({src.data(), src.size()}, {dst.data(), dst.size()});
        },
        R"docstring(
        Sets :py:class:`Matrix` to map src to dst.

        count must be zero or greater, and four or less.

        If count is zero, sets :py:class:`Matrix` to identity and returns true.
        If count is one, sets :py:class:`Matrix` to translate and returns true.
        If count is two or more, sets :py:class:`Matrix` to map
        :py:class:`Point` if possible; returns false if :py:class:`Matrix`
        cannot be constructed. If count is four, :py:class:`Matrix` may include
        perspective.

        :param List[skia.Point] src: :py:class:`Point` to map from
        :param List[skia.Point] dst: :py:class:`Point` to map to
        :return: true if :py:class:`Matrix` was constructed successfully
        )docstring",
        py::arg("src"), py::arg("dst"))
    .def("invert", py::overload_cast<SkMatrix*>(&SkMatrix::invert, py::const_),
        R"docstring(
        Sets inverse to reciprocal matrix, returning true if :py:class:`Matrix`
        can be inverted.

        Geometrically, if :py:class:`Matrix` maps from source to destination,
        inverse :py:class:`Matrix` maps from destination to source. If
        :py:class:`Matrix` can not be inverted, inverse is unchanged.

        :param skia.Matrix inverse: storage for inverted :py:class:`Matrix`; may
            be nullptr
        :return: true if :py:class:`Matrix` can be inverted
        )docstring",
        py::arg("inverse"))
    .def("asAffine",
        [] (const SkMatrix& matrix) -> py::object {
            std::vector<SkScalar> affine(6);
            if (matrix.asAffine(&affine[0]))
                return py::cast(affine);
            return py::none();
        },
        R"docstring(
        Fills affine in column major order.

        Sets affine to::

            | scale-x skew-x translate-x | | skew-y scale-y translate-y |

        If :py:class:`Matrix` contains perspective, returns None.

        :return: list of float if :py:class:`Matrix` does not contain
            perspective, else None
        )docstring")
    .def("setAffine",
        [] (SkMatrix& matrix, const std::vector<SkScalar>& affine) {
            if (affine.size() != 6)
                throw std::runtime_error("affine must have 6 elements");
            return matrix.setAffine(&affine[0]);
        },
        R"docstring(
        Sets :py:class:`Matrix` to affine values, passed in column major order.

        Given affine, column, then row, as::

            | scale-x  skew-x translate-x |
            |  skew-y scale-y translate-y |

        :py:class:`Matrix` is set, row, then column, to::

            | scale-x  skew-x translate-x |
            |  skew-y scale-y translate-y |
            |       0       0           1 |

        :param affine: 3 by 2 affine matrix
        )docstring",
        py::arg("affine"))
    .def("normalizePerspective", &SkMatrix::normalizePerspective,
        R"docstring(
        A matrix is categorized as 'perspective' if the bottom row is not
        [0, 0, 1].

        However, for most uses (e.g. mapPoints) a bottom row of [0, 0, X]
        behaves like a non-perspective matrix, though it will be categorized as
        perspective. Calling :py:meth:`normalizePerspective` will change the
        matrix such that, if its bottom row was [0, 0, X], it will be changed to
        [0, 0, 1] by scaling the rest of the matrix by 1/X::

            | A B C | | A/X B/X C/X | | D E F | -> \
            | D/X E/X F/X | for X != 0 | 0 0 X | | 0 0 1 |
        )docstring")
    .def("mapPoints",
        [] (const SkMatrix& matrix, std::vector<SkPoint>& pts) {
            if (pts.empty())
                return pts;
            matrix.mapPoints({pts.data(), pts.size()}, {pts.data(), pts.size()});
            return pts;
        },
        R"docstring(
        Takes src :py:class:`Point` array and returns mapped :py:class:`Point`
        array.

        :py:class:`Point` are mapped by multiplying each :py:class:`Point` by
        :py:class:`Matrix`. Given::

                     | A B C |       | x |
            Matrix = | D E F |, pt = | y |
                     | G H I |       | 1 |

        where::

            for (i = 0; i < count; ++i) {
                x = src[i].fX
                y = src[i].fY
            }

        each dst :py:class:`Point` is computed as::

                          |A B C| |x|                               Ax+By+C   Dx+Ey+F
            Matrix * pt = |D E F| |y| = |Ax+By+C Dx+Ey+F Gx+Hy+I| = ------- , -------
                          |G H I| |1|                               Gx+Hy+I   Gx+Hy+I

        :param List[skia.Point] src: list of :py:class:`Point` to transform
        )docstring",
        py::arg("pts"))
    .def("mapHomogeneousPoints",
        [] (const SkMatrix& matrix, std::vector<SkPoint3>& pts) -> py::object {
            if (pts.empty())
                return py::cast(pts);
            matrix.mapHomogeneousPoints({pts.data(), pts.size()}, {pts.data(), pts.size()});
            return py::cast(pts);
        },
        R"docstring(
        Takes src :py:class:`Point3` array and returns mapped :py:class:`Point3`
        array.

        :py:class:`Point3` array is mapped by multiplying each
        :py:class:`Point3` by :py:class:`Matrix`. Given::

                     | A B C |        | x |
            Matrix = | D E F |, src = | y |
                     | G H I |        | z |

        each resulting dst :py:class:`Point` is computed as::

                           |A B C| |x|
            Matrix * src = |D E F| |y| = |Ax+By+Cz Dx+Ey+Fz Gx+Hy+Iz|
                           |G H I| |z|

        :pts: :py:class:`Point3` array to transform
        )docstring",
        py::arg("pts"))
    .def("mapHomogeneousPoints",
        [] (const SkMatrix& matrix, const std::vector<SkPoint>& pts
            ) -> py::object {
            if (pts.empty())
                return py::cast(pts);
            std::vector<SkPoint3> dst(pts.size());
            matrix.mapPointsToHomogeneous({dst.data(), dst.size()}, {pts.data(), pts.size()});
            return py::cast(dst);
        },
        R"docstring(
        Returns homogeneous points, starting with 2D src points (with implied w
        = 1).

        :pts: :py:class:`Point` array to transform
        )docstring",
        py::arg("pts"))
    .def("mapXY",
        [] (const SkMatrix& matrix, SkScalar x, SkScalar y) {
            return matrix.mapPoint({x, y});
        },
        R"docstring(
        Returns :py:class:`Point` (x, y) multiplied by :py:class:`Matrix`.

        Given::

                     | A B C |       | x |
            Matrix = | D E F |, pt = | y |
                     | G H I |       | 1 |

        result is computed as::

                          |A B C| |x|                               Ax+By+C   Dx+Ey+F
            Matrix * pt = |D E F| |y| = |Ax+By+C Dx+Ey+F Gx+Hy+I| = ------- , -------
                          |G H I| |1|                               Gx+Hy+I   Gx+Hy+I

        :param float x: x-axis value of :py:class:`Point` to map
        :param float y: y-axis value of :py:class:`Point` to map
        :return: mapped :py:class:`Point`
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("mapVectors",
        [] (const SkMatrix& matrix, std::vector<SkVector>& src) {
            if (src.empty())
                return src;
            matrix.mapVectors({src.data(), src.size()}, {src.data(), src.size()});
            return src;
        },
        R"docstring(
        Maps src vector array to vector :py:class:`Point` array.

        Vectors are mapped by multiplying each vector by :py:class:`Matrix`,
        treating :py:class:`Matrix` translation as zero. Given::

                     | A B 0 |         | x |
            Matrix = | D E 0 |,  src = | y |
                     | G H I |         | 1 |

        where::

            for (i = 0; i < count; ++i) {
                x = src[i].fX
                y = src[i].fY
            }

        each dst vector is computed as::

                           |A B 0| |x|                            Ax+By     Dx+Ey
            Matrix * src = |D E 0| |y| = |Ax+By Dx+Ey Gx+Hy+I| = ------- , -------
                           |G H I| |1|                           Gx+Hy+I   Gx+Hy+I

        :param List[skia.Point] src: vectors to transform
        )docstring",
        py::arg("src"))
    .def("mapVector",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::mapVector, py::const_),
        R"docstring(
        Returns vector (dx, dy) multiplied by :py:class:`Matrix`, treating
        :py:class:`Matrix` translation as zero.

        Given::

                     | A B 0 |         | dx |
            Matrix = | D E 0 |,  vec = | dy |
                     | G H I |         |  1 |

        each result vector is computed as::

                           |A B 0| |dx|                                       A*dx+B*dy    D*dx+E*dy
            Matrix * vec = |D E 0| |dy| = |A*dx+B*dy D*dx+E*dy G*dx+H*dy+I| = --------—--, --------—--
                           |G H I| | 1|                                       G*dx+H*dy+I  G*dx+*dHy+I

        :param float dx: x-axis value of vector to map
        :param float dy: y-axis value of vector to map
        :return: mapped vector
        )docstring",
        py::arg("dx"), py::arg("dy"))
    .def("mapRect",
        py::overload_cast<const SkRect&>(
            &SkMatrix::mapRect, py::const_),
        R"docstring(
        Returns bounds of src corners mapped by :py:class:`Matrix`.

        :param skia.Rect src: rectangle to map
        :return: mapped bounds
        )docstring",
        py::arg("src"))
    .def("mapRectToQuad",
        [] (const SkMatrix& matrix, const SkRect& rect) {
            std::vector<SkPoint> dst(4);
            matrix.mapRectToQuad(&dst[0], rect);
            return dst;
        },
        R"docstring(
        Maps four corners of rect to dst.

        :py:class:`Point` are mapped by multiplying each rect corner by
        :py:class:`Matrix`. rect corner is processed in this order: (rect.fLeft,
        rect.fTop), (rect.fRight, rect.fTop), (rect.fRight, rect.fBottom),
        (rect.fLeft, rect.fBottom).

        rect may be empty: rect.fLeft may be greater than or equal to
        rect.fRight; rect.fTop may be greater than or equal to rect.fBottom.

        Given::

                     | A B C |        | x |
            Matrix = | D E F |,  pt = | y |
                     | G H I |        | 1 |

        where pt is initialized from each of (rect.fLeft, rect.fTop),
        (rect.fRight, rect.fTop), (rect.fRight, rect.fBottom), (rect.fLeft,
        rect.fBottom), each dst :py:class:`Point` is computed as::

                          |A B C| |x|                               Ax+By+C   Dx+Ey+F
            Matrix * pt = |D E F| |y| = |Ax+By+C Dx+Ey+F Gx+Hy+I| = ------- , -------
                          |G H I| |1|                               Gx+Hy+I   Gx+Hy+I

        :param skia.Rect rect: :py:class:`Rect` to map

        Note: this does not perform perspective clipping (as that might result
        in more than 4 points, so results are suspect if the matrix contains
        perspective.
        )docstring",
        py::arg("rect"))
    .def("mapRectScaleTranslate",
        [] (const SkMatrix& matrix, const SkRect& src) {
            SkRect dst;
            matrix.mapRectScaleTranslate(&dst, src);
            return dst;
        },
        R"docstring(
        Returns bounds of src corners mapped by :py:class:`Matrix`.

        If matrix contains elements other than scale or translate: asserts if
        SK_DEBUG is defined; otherwise, results are undefined.

        :param skia.Rect src: :py:class:`Rect` to map
        )docstring",
        py::arg("src"))
    .def("mapRadius", &SkMatrix::mapRadius,
        R"docstring(
        Returns geometric mean radius of ellipse formed by constructing circle
        of size radius, and mapping constructed circle with :py:class:`Matrix`.

        The result squared is equal to the major axis length times the minor
        axis length. Result is not meaningful if :py:class:`Matrix` contains
        perspective elements.

        :param radius: circle size to map
        :return: average mapped radius
        )docstring",
        py::arg("radius"))
    .def("dump",
        [] (const SkMatrix& matrix) {
            py::scoped_ostream_redirect stream;
            matrix.dump();
        },
        R"docstring(
        Writes text representation of :py:class:`Matrix` to standard output.

        Floating point values are written with limited precision; it may not be
        possible to reconstruct original :py:class:`Matrix` from output.
        )docstring")
    .def("getMinScale", &SkMatrix::getMinScale,
        R"docstring(
        Returns the minimum scaling factor of :py:class:`Matrix` by decomposing
        the scaling and skewing elements.

        Returns -1 if scale factor overflows or :py:class:`Matrix` contains
        perspective.

        :return: minimum scale factor
        )docstring")
    .def("getMaxScale", &SkMatrix::getMaxScale,
        R"docstring(
        Returns the maximum scaling factor of :py:class:`Matrix` by decomposing
        the scaling and skewing elements.

        Returns -1 if scale factor overflows or :py:class:`Matrix` contains
        perspective.

        :return: maximum scale factor
        )docstring")
    .def("getMinMaxScales",
        [] (const SkMatrix& matrix) -> py::object {
            std::vector<SkScalar> factors(2);
            if (matrix.getMinMaxScales(&factors[0]))
                return py::make_tuple(factors[0], factors[1]);
            return py::make_tuple(-1, -1);
        },
        R"docstring(
        Returns the minimum scaling factor and the maximum scaling factor.

        Scaling factors are computed by decomposing the :py:class:`Matrix`
        scaling and skewing elements.

        Returns (min, max) if scale factors are found; otherwise, returns
        (-1, -1).

        :return: tuple of min and max factors
        )docstring")
    .def("decomposeScale", &SkMatrix::decomposeScale,
        R"docstring(
        Decomposes :py:class:`Matrix` into scale components and whatever
        remains.

        Returns false if :py:class:`Matrix` could not be decomposed.

        Sets scale to portion of :py:class:`Matrix` that scale axes. Sets
        remaining to :py:class:`Matrix` with scaling factored out. remaining may
        be passed as nullptr to determine if :py:class:`Matrix` can be
        decomposed without computing remainder.

        Returns true if scale components are found. scale and remaining are
        unchanged if :py:class:`Matrix` contains perspective; scale factors are
        not finite, or are nearly zero.

        On success: Matrix = Remaining * scale.

        :param skia.Size scale: axes scaling factors; may be nullptr
        :param skia.Matrix remaining: :py:class:`Matrix` without scaling; may be
            nullptr
        :return: true if scale can be computed
        )docstring",
        py::arg("scale"), py::arg("remaining"))
    .def("dirtyMatrixTypeCache", &SkMatrix::dirtyMatrixTypeCache,
        R"docstring(
        Sets internal cache to unknown state.

        Use to force update after repeated modifications to :py:class:`Matrix`
        element reference returned by operator[](int index).
        )docstring")
    .def("setScaleTranslate", &SkMatrix::setScaleTranslate,
        R"docstring(
        Initializes :py:class:`Matrix` with scale and translate elements.

            | sx 0 tx |
            | 0 sy ty |
            | 0  0  1 |

        :param float sx: horizontal scale factor to store
        :param float sy: vertical scale factor to store
        :param float tx: horizontal translation to store
        :param float ty: vertical translation to store
        )docstring",
        py::arg("sx"), py::arg("sy"), py::arg("tx"), py::arg("ty"))
    .def("isFinite", &SkMatrix::isFinite,
        R"docstring(
        Returns true if all elements of the matrix are finite.

        Returns false if any element is infinity, or NaN.

        :return: true if matrix has only finite elements
        )docstring")
    .def_static("Scale",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::Scale),
        R"docstring(
        Sets :py:class:`Matrix` to scale by (sx, sy).

        Returned matrix is:

            | sx 0 0 |
            | 0 sy 0 |
            | 0  0 1 |

        :param float sx: horizontal scale factor
        :param float sy: vertical scale factor
        :return: :py:class:`Matrix` with scale
        )docstring",
        py::arg("sx"), py::arg("sy"))
    .def_static("Translate",
        py::overload_cast<SkScalar, SkScalar>(&SkMatrix::Translate),
        R"docstring(
        Sets :py:class:`Matrix` to translate by (dx, dy).

        Returned matrix is:

            | 1 0 dx |
            | 0 1 dy |
            | 0 0  1 |

        :dx: horizontal translation
        :dy: vertical translation
        :return: :py:class:`Matrix` with translation
        )docstring",
        py::arg("dx"), py::arg("dy"))
    .def_static("Translate",
        py::overload_cast<SkVector>(&SkMatrix::Translate),
        py::arg("t"))
    .def_static("Translate",
        py::overload_cast<SkIVector>(&SkMatrix::Translate),
        py::arg("t"))
    .def_static("RotateDeg",
        py::overload_cast<SkScalar>(&SkMatrix::RotateDeg),
        R"docstring(
        Sets :py:class:`Matrix` to rotate by ``|deg|`` about a pivot point at
        (0, 0).

        :param deg: rotation angle in degrees (positive rotates clockwise)
        :return: :py:class:`Matrix` with rotation
        )docstring",
        py::arg("deg"))
    .def_static("RotateDeg",
        py::overload_cast<SkScalar, SkPoint>(&SkMatrix::RotateDeg),
        py::arg("deg"), py::arg("pt"))
    .def_static("RotateRad", &SkMatrix::RotateRad,
        py::arg("rad"))
    .def_static("MakeAll", &SkMatrix::MakeAll,
        R"docstring(
        Sets :py:class:`Matrix` to::

            | scaleX skewX transX |
            | skewY scaleY transY |
            | pers0  pers1  pers2 |

        :param float scaleX:  horizontal scale factor
        :param float skewX:   horizontal skew factor
        :param float transX:  horizontal translation
        :param float skewY:   vertical skew factor
        :param float scaleY:  vertical scale factor
        :param float transY:  vertical translation
        :param float pers0:   input x-axis perspective factor
        :param float pers1:   input y-axis perspective factor
        :param float pers2:   perspective scale factor
        :return: :py:class:`Matrix` constructed from parameters
        )docstring",
        py::arg("scaleX"), py::arg("skewX"), py::arg("transX"),
        py::arg("skewY"), py::arg("scaleY"), py::arg("transY"),
        py::arg("pers0"), py::arg("pers1"), py::arg("pers2"))
    .def_static("MakeRectToRect", &SkMatrix::MakeRectToRect,
        R"docstring(
        Returns :py:class:`Matrix` set to scale and translate src
        :py:class:`Rect` to dst :py:class:`Rect`.

        stf selects whether mapping completely fills dst or preserves the aspect
        ratio, and how to align src within dst. Returns the identity
        :py:class:`Matrix` if src is empty. If dst is empty, returns
        :py:class:`Matrix` set to::

            | 0 0 0 |
            | 0 0 0 |
            | 0 0 1 |

        :param skia.Rect src: :py:class:`Rect` to map from
        :param skia.Rect dst: :py:class:`Rect` to map to
        :param skia.Matrix.ScaleToFit stf: fill option
        :return: :py:class:`Matrix` mapping src to dst
        )docstring",
        py::arg("src"), py::arg("dst"), py::arg("stf"))
    .def_static("SetAffineIdentity",
        [] () {
            std::vector<SkScalar> affine(6);
            SkMatrix::SetAffineIdentity(&affine[0]);
            return affine;
        },
        R"docstring(
        Fills affine with identity values in column major order.

        Sets affine to:

            | 1 0 0 |
            | 0 1 0 |

        Affine 3 by 2 matrices in column major order are used by OpenGL and XPS.

        :return: List of float
        )docstring")
    .def_static("I", &SkMatrix::I,
        R"docstring(
        Returns reference to const identity :py:class:`Matrix`.

        Returned :py:class:`Matrix` is set to:

            | 1 0 0 |
            | 0 1 0 |
            | 0 0 1 |

        :return: const identity :py:class:`Matrix`
        )docstring")
    .def_static("InvalidMatrix", &SkMatrix::InvalidMatrix,
        R"docstring(
        Returns reference to a const :py:class:`Matrix` with invalid values.

        Returned :py:class:`Matrix` is set to:

            | SK_ScalarMax SK_ScalarMax SK_ScalarMax |
            | SK_ScalarMax SK_ScalarMax SK_ScalarMax |
            | SK_ScalarMax SK_ScalarMax SK_ScalarMax |

        :return: const invalid :py:class:`Matrix`
        )docstring")
    .def_static("Concat", &SkMatrix::Concat,
        R"docstring(
        Returns :py:class:`Matrix` a multiplied by :py:class:`Matrix` b.

        Given::

                | A B C |      | J K L |
            a = | D E F |, b = | M N O |
                | G H I |      | P Q R |

        sets :py:class:`Matrix` to::

                    | A B C |   | J K L |   | AJ+BM+CP AK+BN+CQ AL+BO+CR |
            a * b = | D E F | * | M N O | = | DJ+EM+FP DK+EN+FQ DL+EO+FR |
                    | G H I |   | P Q R |   | GJ+HM+IP GK+HN+IQ GL+HO+IR |

        :param skia.Matrix a: :py:class:`Matrix` on left side of multiply
            expression
        :param skia.Matrix b: :py:class:`Matrix` on right side of multiply
            expression
        :return: :py:class:`Matrix` computed from a times b
        )docstring",
        py::arg("a"), py::arg("b"))
    .def_readonly_static("kMScaleX", &SkMatrix::kMScaleX)
    .def_readonly_static("kMSkewX", &SkMatrix::kMSkewX)
    .def_readonly_static("kMTransX", &SkMatrix::kMTransX)
    .def_readonly_static("kMSkewY", &SkMatrix::kMSkewY)
    .def_readonly_static("kMScaleY", &SkMatrix::kMScaleY)
    .def_readonly_static("kMTransY", &SkMatrix::kMTransY)
    .def_readonly_static("kMPersp0", &SkMatrix::kMPersp0)
    .def_readonly_static("kMPersp1", &SkMatrix::kMPersp1)
    .def_readonly_static("kMPersp2", &SkMatrix::kMPersp2)
    .def_readonly_static("kAScaleX", &SkMatrix::kAScaleX)
    .def_readonly_static("kASkewY", &SkMatrix::kASkewY)
    .def_readonly_static("kASkewX", &SkMatrix::kASkewX)
    .def_readonly_static("kAScaleY", &SkMatrix::kAScaleY)
    .def_readonly_static("kATransX", &SkMatrix::kATransX)
    .def_readonly_static("kATransY", &SkMatrix::kATransY)
    ;

py::implicitly_convertible<NumPy, SkMatrix>();

// M44
py::class_<SkM44>(m, "M44", R"docstring(
    4x4 matrix used by :py:class:`Canvas` and other parts of Skia.

    Skia assumes a right-handed coordinate system: +X goes to the right +Y goes
    down +Z goes into the screen (away from the viewer)
    )docstring")
    .def(py::init<>())
    .def(py::init<const SkMatrix&>(), py::arg("src"))
    .def(py::init<const SkM44&, const SkM44&>(), py::arg("a"), py::arg("b"))
    .def_static("Rows", &SkM44::Rows,
        py::arg("r0"), py::arg("r1"), py::arg("r2"), py::arg("r3"))
    .def_static("Cols", &SkM44::Cols,
        py::arg("c0"), py::arg("c1"), py::arg("c2"), py::arg("c3"))
    .def_static("RowMajor", &SkM44::RowMajor, py::arg("r"))
    .def_static("ColMajor", &SkM44::ColMajor, py::arg("c"))
    .def_static("Translate", &SkM44::Translate,
        py::arg("x"), py::arg("y"), py::arg("z") = 0)
    .def_static("Scale", &SkM44::Scale,
        py::arg("x"), py::arg("y"), py::arg("z") = 1)
    .def_static("Rotate", &SkM44::Rotate,
        py::arg("axis"), py::arg("radians"))
    .def_static("RectToRect", &SkM44::RectToRect,
        py::arg("src"), py::arg("dst"))
    .def_static("LookAt", &SkM44::LookAt,
        py::arg("eye"), py::arg("center"), py::arg("up"))
    .def_static("Perspective", &SkM44::Perspective,
        py::arg("near"), py::arg("far"), py::arg("angle"))
    .def("rc",
        [] (const SkM44& self, int r, int c) {
            if ((r < 0) || (r > 3) || (c < 0) || (c > 3))
                throw py::value_error("Indices must be between 0 to 3.");
            return self.rc(r, c);
        },
        R"docstring(
        Returns one matrix value from a particular row/column.

        :param r:  matrix row to fetch
        :param c:  matrix column to fetch
        :return:   value at the given matrix position
        )docstring",
        py::arg("r"), py::arg("c"))
    .def("setRC",
        [] (SkM44& self, int r, int c, SkScalar value) {
            if ((r < 0) || (r > 3) || (c < 0) || (c > 3))
                throw py::value_error("Indices must be between 0 to 3.");
            self.setRC(r, c, value);
        },
        R"docstring(
        Set one matrix value at a particular row/column.

        :param r:  matrix row to set
        :param c:  matrix column to set
        :param value:  value to set
        )docstring",
        py::arg("r"), py::arg("c"), py::arg("value"))
    .def("setIdentity", &SkM44::setIdentity)
    .def("setTranslate", &SkM44::setTranslate,
        py::arg("x"), py::arg("y"), py::arg("z") = 0)
    .def("setScale", &SkM44::setScale,
        py::arg("x"), py::arg("y"), py::arg("z") = 1)
    .def("setRotateUnitSinCos", &SkM44::setRotateUnitSinCos,
        py::arg("axis"), py::arg("sinAngle"), py::arg("cosAngle"))
    .def("setRotateUnit", &SkM44::setRotateUnit,
        py::arg("axis"), py::arg("radians"))
    .def("setRotate", &SkM44::setRotate,
        py::arg("axis"), py::arg("radians"))
    .def("setConcat", &SkM44::setConcat,
        py::arg("a"), py::arg("b"))
    .def("preConcat", py::overload_cast<const SkM44&>(&SkM44::preConcat),
        py::arg("m"))
    .def("postConcat", &SkM44::postConcat,
        py::arg("m"))
    .def("normalizePerspective", &SkM44::normalizePerspective)
    .def("isFinite", &SkM44::isFinite)
    .def("asM33", &SkM44::asM33)
    .def("preTranslate", &SkM44::preTranslate,
        py::arg("x"), py::arg("y"), py::arg("z") = 0)
    .def("postTranslate", &SkM44::postTranslate,
        py::arg("x"), py::arg("y"), py::arg("z") = 0)
    .def("preScale", py::overload_cast<SkScalar, SkScalar>(&SkM44::preScale),
        py::arg("x"), py::arg("y"))
    .def("preScale", py::overload_cast<SkScalar, SkScalar, SkScalar>(&SkM44::preScale),
        py::arg("x"), py::arg("y"), py::arg("z"))
    .def("preConcat", py::overload_cast<const SkMatrix&>(&SkM44::preConcat),
        py::arg("m"))
    ;

// RSXform
py::class_<SkRSXform>(m, "RSXform", R"docstring(
    A compressed form of a rotation+scale matrix.

        [ fSCos -fSSin fTx ]
        [ fSSin fSCos fTy ]
        [ 0 0 1 ]
    )docstring")
    .def(py::init(&SkRSXform::Make),
        py::arg("scos"), py::arg("ssin"), py::arg("tx"), py::arg("ty"))
    .def_static("Make",
        &SkRSXform::Make,
        py::arg("scos"), py::arg("ssin"), py::arg("tx"), py::arg("ty"))
    .def_static("MakeFromRadians", &SkRSXform::MakeFromRadians,
        py::arg("scale"), py::arg("radians"), py::arg("tx"), py::arg("ty"),
        py::arg("ax"), py::arg("ay"))
    .def("rectStaysRect", &SkRSXform::rectStaysRect)
    .def("setIdentity", &SkRSXform::setIdentity)
    .def("set", &SkRSXform::set,
        py::arg("scos"), py::arg("ssin"), py::arg("tx"), py::arg("ty"))
    .def("toQuad", py::overload_cast<SkScalar, SkScalar, SkPoint[4]>(
        &SkRSXform::toQuad, py::const_),
        py::arg("width"), py::arg("height"), py::arg("quqd"))
    .def("toQuad", py::overload_cast<const SkSize&, SkPoint[4]>(
        &SkRSXform::toQuad, py::const_),
        py::arg("size"), py::arg("quqd"))
    .def("toTriStrip", &SkRSXform::toTriStrip,
        py::arg("width"), py::arg("height"), py::arg("strip"))
    .def_readwrite("fSCos", &SkRSXform::fSCos)
    .def_readwrite("fSSin", &SkRSXform::fSSin)
    .def_readwrite("fTx", &SkRSXform::fTx)
    .def_readwrite("fTy", &SkRSXform::fTy)
    ;
}
