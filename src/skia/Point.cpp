#include "common.h"
#include <include/core/SkPoint3.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

void initPoint(py::module &m) {
// IPoint
py::class_<SkIPoint>(m, "IPoint", R"docstring(
    :py:class:`IPoint` holds two 32-bit integer coordinates.

    Example::

        point = skia.IPoint(0, 0)
        point = skia.IPoint((0, 0))  # Convert from tuple
        print(point.fX, point.fY)
        point + (1, 1)   # Add offset
        point - (0, 0)   # Subtract offset
        x, y = tuple(point)  # Convert to tuple
    )docstring")
    .def(py::init(&SkIPoint::Make),
        R"docstring(
        Sets fX to x, fY to y.

        :x: integer x-axis value of constructed :py:class:`IPoint`
        :y: integer y-axis value of constructed :py:class:`IPoint`
        )docstring",
        py::arg("x"), py::arg("y"))
    .def(py::init(
        [] (py::tuple t) {
            if (t.size() != 2)
                throw py::value_error("IPoint must have exactly two elements.");
            return SkIPoint::Make(t[0].cast<int32_t>(), t[1].cast<int32_t>());
        }),
        py::arg("t"))
    .def("x", &SkIPoint::x,
        R"docstring(
        Returns x-axis value of :py:class:`IPoint`.
        )docstring")
    .def("y", &SkIPoint::y,
        R"docstring(
        Returns y-axis value of :py:class:`IPoint`.
        )docstring")
    .def("isZero", &SkIPoint::isZero,
        R"docstring(
        Returns true if fX and fY are both zero.
        )docstring")
    .def("set", &SkIPoint::set,
        R"docstring(
        Sets fX to x and fY to y.
        )docstring",
        py::arg("x"), py::arg("y"))
    .def(-py::self,
        R"docstring(
        Returns :py:class:`IPoint` changing the signs of fX and fY.
        )docstring")
    .def("__iadd__", [] (SkIPoint& p, const SkIPoint& v) { p += v; return p; },
        R"docstring(
        Offsets :py:class:`IPoint` by ivector v.

        Sets :py:class:`IPoint` to ``(fX + v.fX, fY + v.fY)``.
        )docstring",
        py::arg("v"), py::is_operator())
    .def("__isub__", [] (SkIPoint& p, const SkIPoint& v) { p -= v; return p; },
        R"docstring(
        Returns :py:class:`IPoint` changing the signs of fX and fY.
        )docstring",
        py::arg("v"), py::is_operator())
    .def("equals", &SkIPoint::equals,
        R"docstring(
        Returns true if :py:class:`IPoint` is equivalent to :py:class:`IPoint`
        constructed from (x, y).

        :param x: value compared with fX
        :param y: value compared with fY
        )docstring",
        py::arg("x"), py::arg("y"))
    .def_static("Make", &SkIPoint::Make,
        R"docstring(
        Sets fX to x, fY to y.

        :param x: integer x-axis value of constructed :py:class:`IPoint`
        :param y: integer y-axis value of constructed :py:class:`IPoint`
        :return: :py:class:`IPoint` (x, y)
        )docstring",
        py::arg("x"), py::arg("y"))
    .def_readwrite("fX", &SkIPoint::fX, "x-axis value")
    .def_readwrite("fY", &SkIPoint::fY, "y-axis value")
    .def(py::self == py::self,
        R"docstring(
        Returns true if a is equivalent to b.

        :param other: :py:class:`IPoint` to compare
        :return: true if self.fX == other.fX and self.fY == other.fY
        )docstring",
        py::arg("other"))
    .def(py::self != py::self,
        R"docstring(
        Returns true if a is not equivalent to b.

        :param other: :py:class:`IPoint` to compare
        :return: true if a.fX != b.fX or a.fY != b.fY
        )docstring",
        py::arg("other"))
    .def(py::self - py::self,
        R"docstring(
        Returns ivector from other to self; computed as (self.fX - other.fX,
        self.fY - other.fY).

        Can also be used to subtract ivector from ivector, returning ivector.

        :param other: ivector to subtract
        :return: ivector from other to self
        )docstring",
        py::arg("other"))
    .def(py::self + py::self,
        R"docstring(
        Returns :py:class:`IPoint` resulting from :py:class:`IPoint` self offset
        by ivector other, computed as: (self.fX + other.fX, self.fY + other.fY).

        Can also be used to offset :py:class:`IPoint` other by ivector self,
        returning :py:class:`IPoint`. Can also be used to add ivector to
        ivector, returning ivector.

        :param other: :py:class:`IPoint` or ivector to add
        :return: :py:class:`IPoint` equal to self offset by other
        )docstring",
        py::arg("other"))
    .def("__iter__",
        [] (const SkIPoint& p) {
            return py::make_iterator(&p.fX, &p.fX + 2);
        }, py::keep_alive<0, 1>())
    .def("__len__", [] (const SkIPoint& p) { return 2; })
    .def("__repr__",
        [] (const SkIPoint& p) {
            std::stringstream s;
            s << "IPoint(" << p.fX << ", " << p.fY << ")";
            return s.str();
        })
    ;

py::implicitly_convertible<py::tuple, SkIPoint>();

// Point
py::class_<SkPoint>(m, "Point", R"docstring(
    :py:class:`Point` holds two 32-bit floating point coordinates.

    Example::

        point = skia.Point(0.0, 0.0)
        point = skia.Point((0.0, 0.0))  # Convert from tuple
        print(point.fX, point.fY)
        point + (1, 1)   # Add offset
        point - (0, 0)   # Subtract offset
        point * 2.0      # Scale
        x, y = tuple(point)  # Convert to tuple
    )docstring")
    .def(py::init(&SkPoint::Make),
        R"docstring(
        Sets fX to x, fY to y.

        Used both to set :py:class:`Point` and vector.

        :param float x: :py:class:`Scalar` x-axis value of constructed
            :py:class:`Point` or vector
        :param float y: :py:class:`Scalar` y-axis value of constructed
            :py:class:`Point` or vector
        :return: :py:class:`Point` (x, y)
        )docstring",
        py::arg("x"), py::arg("y"))
    .def(py::init(
        [] (const SkIPoint& p) { return SkPoint::Make(p.fX, p.fY); }),
        py::arg("ipoint"))
    .def(py::init(
        [] (py::tuple t) {
            if (t.size() != 2)
                throw py::value_error("Point must have exactly two elements.");
            return SkPoint::Make(t[0].cast<SkScalar>(), t[1].cast<SkScalar>());
        }),
        py::arg("t"))
    .def("x", &SkPoint::x,
        R"docstring(
        Returns x-axis value of :py:class:`Point` or vector.
        )docstring")
    .def("y", &SkPoint::y,
        R"docstring(
        Returns y-axis value of :py:class:`Point` or vector.
        )docstring")
    .def("isZero", &SkPoint::isZero,
        R"docstring(
        Returns true if fX and fY are both zero.

        :return: true if fX is zero and fY is zero
        )docstring")
    .def("set", &SkPoint::set,
        R"docstring(
        Sets fX to x and fY to y.

        :param x: new value for fX
        :param y: new value for fY
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("iset", py::overload_cast<int32_t, int32_t>(&SkPoint::iset),
        R"docstring(
        Sets fX to x and fY to y, promoting integers to SkScalar values.

        Assigning a large integer value directly to fX or fY may cause a
        compiler error, triggered by narrowing conversion of int to SkScalar.
        This safely casts x and y to avoid the error.

        :x: new value for fX
        :y: new value for fY
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("iset", py::overload_cast<const SkIPoint&>(&SkPoint::iset),
        R"docstring(
        Sets fX to p.fX and fY to p.fY, promoting integers to SkScalar values.

        Assigning an :py:class:`IPoint` containing a large integer value
        directly to fX or fY may cause a compiler error, triggered by narrowing
        conversion of int to SkScalar. This safely casts p.fX and p.fY to avoid
        the error.

        :p: :py:class:`IPoint` members promoted to SkScalar
        )docstring",
        py::arg("p"))
    .def("setAbs", &SkPoint::setAbs,
        R"docstring(
        Sets fX to absolute value of pt.fX; and fY to absolute value of pt.fY.

        :param pt: members providing magnitude for fX and fY
        )docstring",
        py::arg("pt"))
    .def("offset", &SkPoint::offset,
        R"docstring(
        Adds offset (dx, dy) to :py:class:`Point`.

        :param dx: added to fX
        :param dy: added to fY
        )docstring",
        py::arg("dx"), py::arg("dy"))
    .def("length", &SkPoint::length,
        R"docstring(
        Returns the Euclidean distance from origin, computed as:

            sqrt(fX * fX + fY * fY)

        :return: straight-line distance to origin
        )docstring")
    .def("distanceToOrigin", &SkPoint::distanceToOrigin,
        R"docstring(
        Returns the Euclidean distance from origin, computed as:

            sqrt(fX * fX + fY * fY)

        :return: straight-line distance to origin
        )docstring")
    .def("normalize", &SkPoint::normalize,
        R"docstring(
        Scales (fX, fY) so that :py:meth:`length` returns one, while preserving
        ratio of fX to fY, if possible.

        If prior length is nearly zero, sets vector to (0, 0) and returns false;
        otherwise returns true.

        :return: true if former length is not zero or nearly zero
        )docstring")
    .def("setNormalize", &SkPoint::setNormalize,
        R"docstring(
        Sets vector to (x, y) scaled so :py:meth:`length` returns one, and so
        that (fX, fY) is proportional to (x, y).

        If (x, y) length is nearly zero, sets vector to (0, 0) and returns
        false; otherwise returns true.

        :param x: proportional value for fX
        :param y: proportional value for fY
        :return: true if (x, y) length is not zero or nearly zero
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("setLength", py::overload_cast<SkScalar>(&SkPoint::setLength),
        R"docstring(
        Scales vector so that :py:meth:`distanceToOrigin` returns length, if
        possible.

        If former length is nearly zero, sets vector to (0, 0) and return false;
        otherwise returns true.

        :length: straight-line distance to origin
        :return: true if former length is not zero or nearly zero
        )docstring",
        py::arg("length"))
    .def("setLength",
        py::overload_cast<SkScalar, SkScalar, SkScalar>(&SkPoint::setLength),
        R"docstring(
        Sets vector to (x, y) scaled to length, if possible.

        If former length is nearly zero, sets vector to (0, 0) and return false;
        otherwise returns true.

        :x: proportional value for fX
        :y: proportional value for fY
        :length: straight-line distance to origin
        :return: true if (x, y) length is not zero or nearly zero
        )docstring",
        py::arg("x"), py::arg("y"), py::arg("length"))
    .def("scale",
        [] (const SkPoint& p, SkScalar scale) {
            SkPoint dst;
            p.scale(scale, &dst);
            return dst;
        },
        R"docstring(
        Sets dst to :py:class:`Point` times scale.

        dst may be :py:class:`Point` to modify :py:class:`Point` in place.

        It is also possible to use operators::

            p * scale
            p *= scale  # in-place

        :param scale: factor to multiply :py:class:`Point` by
        )docstring",
        py::arg("scale"))
    .def("negate", &SkPoint::negate,
        R"docstring(
        Changes the sign of fX and fY.
        )docstring")
    .def(-py::self,
        R"docstring(
        Returns :py:class:`Point` changing the signs of fX and fY.
        )docstring")
    .def("__iadd__", [](SkPoint& p, const SkPoint& v) { p += v; return p; },
        R"docstring(
        Adds vector v to :py:class:`Point`.

        Sets :py:class:`Point` to: (fX + v.fX, fY + v.fY).

        :param v: vector to add
        )docstring",
        py::arg("v"))
    .def("__isub__", [](SkPoint& p, const SkPoint& v) { p -= v; return p; },
        R"docstring(
        Subtracts vector v from :py:class:`Point`.

        Sets :py:class:`Point` to: (fX - v.fX, fY - v.fY).

        :param v: vector to subtract
        )docstring",
        py::arg("v"))
    .def(py::self * SkScalar(),
        R"docstring(
        Returns :py:class:`Point` multiplied by scale.
        )docstring",
        py::arg("scale"))
    .def(py::self *= SkScalar(),
        R"docstring(
        Multiplies :py:class:`Point` by scale.

        Sets :py:class:`Point` to: (fX * scale, fY * scale).

        :param scale: scalar to multiply by
        :return: reference to :py:class:`Point`
        )docstring",
        py::arg("scale"))
    .def("isFinite", &SkPoint::isFinite,
        R"docstring(
        Returns true if both fX and fY are measurable values.

        :return: true for values other than infinities and NaN
        )docstring")
    .def("equals", &SkPoint::equals,
        R"docstring(
        Returns true if :py:class:`Point` is equivalent to :py:class:`Point`
        constructed from (x, y).

        :param x: value compared with fX
        :param y: value compared with fY
        :return: true if :py:class:`Point` equals (x, y)
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("cross", &SkPoint::cross,
        R"docstring(
        Returns the cross product of vector and vec.

        Vector and vec form three-dimensional vectors with z-axis value equal to
        zero. The cross product is a three-dimensional vector with x-axis and
        y-axis values equal to zero. The cross product z-axis component is
        returned.

        :param vec: right side of cross product
        :return: area spanned by vectors signed by angle direction
        )docstring",
        py::arg("vec"))
    .def("dot", &SkPoint::dot,
        R"docstring(
        Returns the dot product of vector and vector vec.

        :param vec: right side of dot product
        :return: product of input magnitudes and cosine of the angle between
            them
        )docstring",
        py::arg("vec"))
    .def_static("Make", &SkPoint::Make,
        R"docstring(
        Sets fX to x, fY to y.

        Used both to set :py:class:`Point` and vector.

        :param x: Scalar x-axis value of constructed :py:class:`Point` or vector
        :param y: Scalar y-axis value of constructed :py:class:`Point` or vector
        :return: :py:class:`Point` (x, y)
        )docstring",
        py::arg("x"), py::arg("y"))
    .def_static("Offset",
        [] (std::vector<SkPoint>& points, const SkVector& offset) {
            SkPoint::Offset(&points[0], points.size(), offset);
            return points;
        },
        R"docstring(
        Adds offset to each :py:class:`Point` in points array with count
        entries.

        :points: :py:class:`Point` array
        :offset: vector added to points
        :return: :py:class:`Point` array applied offset
        )docstring",
        py::arg("points"), py::arg("offset"))
    .def_static("Offset",
        [] (std::vector<SkPoint>& points, SkScalar dx, SkScalar dy) {
            SkPoint::Offset(&points[0], points.size(), dx, dy);
            return points;
        },
        R"docstring(
        Adds offset (dx, dy) to each :py:class:`Point` in points array of length
        count.

        :points: :py:class:`Point` array
        :dx: added to fX in points
        :dy: added to fY in points
        )docstring",
        py::arg("points"), py::arg("dx"), py::arg("dy"))
    .def_static("Length", &SkPoint::Length,
        R"docstring(
        Returns the Euclidean distance from origin, computed as:

            sqrt(x * x + y * y)

        :param x: component of length
        :param y: component of length
        :return: straight-line distance to origin
        )docstring",
        py::arg("x"), py::arg("y"))
    .def_static("Normalize", &SkPoint::Normalize,
        R"docstring(
        Scales (vec->fX, vec->fY) so that length() returns one, while preserving
        ratio of vec->fX to vec->fY, if possible.

        If original length is nearly zero, sets vec to (0, 0) and returns zero;
        otherwise, returns length of vec before vec is scaled.

        Returned prior length may be SK_ScalarInfinity if it can not be
        represented by SkScalar.

        Note that :py:meth:`normalize` is faster if prior length is not
        required.

        :param vec: normalized to unit length
        :return: original vec length
        )docstring",
        py::arg("vec"))
    .def_static("Distance", &SkPoint::Distance,
        R"docstring(
        Returns the Euclidean distance between a and b.

        :param a: line end point
        :param b: line end point
        :return: straight-line distance from a to b
        )docstring",
        py::arg("a"), py::arg("b"))
    .def_static("DotProduct", &SkPoint::DotProduct,
        R"docstring(
        Returns the dot product of vector a and vector b.

        :param a: left side of dot product
        :param b: right side of dot product
        :return: product of input magnitudes and cosine of the angle between
            them
        )docstring",
        py::arg("a"), py::arg("b"))
    .def_static("CrossProduct", &SkPoint::CrossProduct,
        R"docstring(
        Returns the cross product of vector a and vector b.

        a and b form three-dimensional vectors with z-axis value equal to zero.
        The cross product is a three-dimensional vector with x-axis and y-axis
        values equal to zero. The cross product z-axis component is returned.

        :param a: left side of cross product
        :param b: right side of cross product
        :return: area spanned by vectors signed by angle direction
        )docstring",
        py::arg("a"), py::arg("b"))
    .def_readwrite("fX", &SkPoint::fX, "x-axis value")
    .def_readwrite("fY", &SkPoint::fY, "y-axis value")
    .def(py::self == py::self,
        R"docstring(
        Returns true if self is not equivalent to other.

        :param other: :py:class:`Point` to compare
        :return: true if self.fX == other.fX and self.fY == other.fY
        )docstring",
        py::arg("other"))
    .def(py::self != py::self,
        R"docstring(
        Returns true if self is equivalent to other.

        :param other: :py:class:`Point` to compare
        :return: true if self.fX != other.fX or self.fY != other.fY
        )docstring",
        py::arg("other"))
    .def(py::self - py::self,
        R"docstring(
        Returns vector from other to self, computed as (self.fX - other.fX,
        self.fY - other.fY).

        Can also be used to subtract vector from :py:class:`Point`, returning
        :py:class:`Point`. Can also be used to subtract vector from vector,
        returning vector.

        :param other: :py:class:`Point` to subtract
        :return: vector from other to self
        )docstring",
        py::arg("other"))
    .def(py::self + py::self,
        R"docstring(
        Returns :py:class:`Point` resulting from :py:class:`Point` self offset
        by vector other, computed as: (self.fX + other.fX, self.fY + other.fY).

        Can also be used to offset :py:class:`Point` other by vector self,
        returning :py:class:`Point`. Can also be used to add vector to vector,
        returning vector.

        :param other: :py:class:`Point` or vector to add
        :return: :py:class:`Point` equal to self offset by other
        )docstring",
        py::arg("other"))
    .def("__iter__",
        [] (const SkPoint& p) {
            return py::make_iterator(&p.fX, &p.fX + 2);
        }, py::keep_alive<0, 1>())
    .def("__len__", [] (const SkPoint& p) { return 2; })
    .def("__repr__",
        [] (const SkPoint& p) {
            std::stringstream s;
            s << "Point(" << p.fX << ", " << p.fY << ")";
            return s.str();
        })
    ;

py::implicitly_convertible<py::tuple, SkPoint>();
py::implicitly_convertible<SkIPoint, SkPoint>();

// Point3
py::class_<SkPoint3>(m, "Point3",
    R"docstring(
    :py:class:`Point` holds three 32-bit floating point coordinates.

    Example::

        point = skia.Point3(0.0, 0.0, 0.0)
        point = skia.Point3((0.0, 0.0, 0.0))  # Convert from tuple
        print(point.fX, point.fY, point.fZ)
        x, y, z = tuple(point)  # Convert to tuple
    )docstring")
    .def(py::init(&SkPoint3::Make),
        py::arg("x"), py::arg("y"), py::arg("z"))
    .def(py::init(
        [] (py::tuple t) {
            if (t.size() != 3)
                throw py::value_error("Point3 must have exactly 3 elements.");
            return SkPoint3::Make(
                t[0].cast<SkScalar>(),
                t[1].cast<SkScalar>(),
                t[2].cast<SkScalar>());
        }),
        py::arg("t"))
    .def("x", &SkPoint3::x)
    .def("y", &SkPoint3::y)
    .def("z", &SkPoint3::z)
    .def("set", &SkPoint3::set, py::arg("x"), py::arg("y"), py::arg("z"))
    .def("length", &SkPoint3::length,
        R"docstring(
        Return the Euclidian distance from (0,0,0) to the point.
        )docstring")
    .def("normalize", &SkPoint3::normalize,
        R"docstring(
        Set the point (vector) to be unit-length in the same direction as it
        already points.

        If the point has a degenerate length (i.e., nearly 0) then set it to
        (0,0,0) and return false; otherwise return true.
        )docstring")
    .def("makeScale", &SkPoint3::makeScale,
        R"docstring(
        Return a new point whose X, Y and Z coordinates are scaled.
        )docstring",
        py::arg("scale"))
    .def("scale", &SkPoint3::scale,
        R"docstring(
        Scale the point's coordinates by scale.
        )docstring",
        py::arg("scale"))
    .def(-py::self,
        R"docstring(
        Return a new point whose X, Y and Z coordinates are the negative of the
        original point's.
        )docstring")
    .def("__iadd__", [] (SkPoint3& p, const SkPoint3& v) { p += v; return p; },
        R"docstring(
        Add v's coordinates to the point's.
        )docstring",
        py::arg("v"), py::is_operator())
    .def("__isub__", [] (SkPoint3& p, const SkPoint3& v) { p -= v; return p; },
        R"docstring(
        Subtract v's coordinates from the point's.
        )docstring",
        py::arg("v"), py::is_operator())
    .def("isFinite", &SkPoint3::isFinite,
        "Returns true if fX, fY, and fZ are measurable values.")
    .def("dot", &SkPoint3::dot, py::arg("vec"))
    .def("cross", &SkPoint3::cross, py::arg("vec"))
    .def_static("Make", &SkPoint3::Make,
        py::arg("x"), py::arg("y"), py::arg("z"))
    .def_static("Length", &SkPoint3::Length,
        R"docstring(
        Returns the Euclidian distance from (0,0,0) to (x,y,z)
        )docstring",
        py::arg("x"), py::arg("y"), py::arg("z"))
    .def_static("DotProduct", &SkPoint3::DotProduct,
        R"docstring(
        Returns the dot product of a and b, treating them as 3D vectors.
        )docstring",
        py::arg("a"), py::arg("b"))
    .def_static("CrossProduct", &SkPoint3::CrossProduct,
        R"docstring(
        Returns the cross product of a and b, treating them as 3D vectors.
        )docstring",
        py::arg("a"), py::arg("b"))
    .def_readwrite("fX", &SkPoint3::fX, "x-axis value")
    .def_readwrite("fY", &SkPoint3::fY, "y-axis value")
    .def_readwrite("fZ", &SkPoint3::fZ, "z-axis value")
    .def(py::self == py::self, py::arg("other"))
    .def(py::self != py::self, py::arg("other"))
    .def(py::self - py::self,
        R"docstring(
        Returns a new point whose coordinates are the difference between a and b
        (i.e., a - b)
        )docstring",
        py::arg("other"))
    .def(py::self + py::self,
        R"docstring(
        Returns a new point whose coordinates are the sum of a and b (a + b)
        )docstring",
        py::arg("other"))
    .def(SkScalar() * py::self, py::arg("scale"))
    .def("__iter__",
        [] (const SkPoint3& p) {
            return py::make_iterator(&p.fX, &p.fX + 3);
        }, py::keep_alive<0, 1>())
    .def("__len__", [] (const SkPoint3& p) { return 3; })
    .def("__repr__",
        [] (const SkPoint3& p) {
            std::stringstream s;
            s << "Point3(" << p.fX << ", " << p.fY << ", " << p.fZ << ")";
            return s.str();
        })
    ;

py::implicitly_convertible<py::tuple, SkPoint3>();
}
