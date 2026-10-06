#include "common.h"
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace {

template <typename T, bool readonly = true>
py::memoryview AddrN(const SkPixmap& pixmap) {
    if (pixmap.info().bytesPerPixel() != sizeof(T))
        throw std::runtime_error("Incompatible byte size.");
    return py::memoryview::from_buffer(
        reinterpret_cast<T*>(pixmap.writable_addr()),
        sizeof(T),
        py::format_descriptor<T>::value,
        { pixmap.rowBytesAsPixels(), pixmap.height() },
        { pixmap.rowBytes(), sizeof(T) },
        readonly
    );
}

template <bool readonly = true>
py::memoryview Addr(const SkPixmap& pixmap) {
    CHECK_NOTNULL(pixmap.addr());
    return py::memoryview::from_memory(
        pixmap.writable_addr(), pixmap.computeByteSize(), readonly);
}

}  // namespace

const int SkYUVAPixmapInfo::kMaxPlanes;
const int SkYUVAPixmapInfo::kDataTypeCnt;
const int SkYUVAPixmaps::kMaxPlanes;
/*
const int SkYUVAIndex::kIndexCount;
const int SkYUVASizeInfo::kMaxCount;
*/

void initPixmap(py::module &m) {
py::class_<SkPixmap>(m, "Pixmap",
    R"docstring(
    :py:class:`Pixmap` provides a utility to pair :py:class:`ImageInfo` with
    pixels and row bytes.

    :py:class:`Pixmap` is a low level class which provides convenience functions
    to access raster destinations. :py:class:`Canvas` can not draw
    :py:class:`Pixmap`, nor does :py:class:`Pixmap` provide a direct drawing
    destination.

    Use :py:class:`Bitmap` to draw pixels referenced by :py:class:`Pixmap`; use
    :py:class:`Surface` to draw into pixels referenced by :py:class:`Pixmap`.

    :py:class:`Pixmap` does not try to manage the lifetime of the pixel memory.
    Use :py:class:`PixelRef` to manage pixel memory; :py:class:`PixelRef` is
    safe across threads.

    :py:class:`Pixmap` supports buffer protocol. It is possible to mount
    :py:class:`Pixmap` as array::

        array = np.array(pixmap, copy=False)

    Or mount array as :py:class:`Pixmap` with :py:class:`ImageInfo`::

        buffer = np.zeros((100, 100, 4), np.uint8)
        array = skia.Pixmap(skia.ImageInfo.MakeN32Premul(100, 100), buffer)

    )docstring",
    py::buffer_protocol())
    .def_buffer([] (SkPixmap& pixmap) {
        CHECK_NOTNULL(pixmap.addr());
        return ImageInfoToBufferInfo(
            pixmap.info(), pixmap.writable_addr(), pixmap.rowBytes(), false);
    })
    .def("__repr__",
        [] (const SkPixmap& pixmap) {
            return py::str("Pixmap({}, {}, {}, {})").format(
                pixmap.width(), pixmap.height(), pixmap.colorType(),
                pixmap.alphaType());
        })
    .def("__len__",
        [] (const SkPixmap& pixmap) {
            return pixmap.width() * pixmap.height();
        })
    .def("__getitem__",
        [] (const SkPixmap& pixmap, py::object object) {
            int x = 0;
            int y = 0;
            if (py::isinstance<py::tuple>(object)) {
                auto t = object.cast<py::tuple>();
                if (t.size() != 2)
                    throw py::index_error("Index must be two dimension.");
                x = t[0].cast<int>();
                y = t[1].cast<int>();
            }
            else {
                int offset = object.cast<int>();
                x = offset % pixmap.width();
                y = offset / pixmap.height();
            }
            if (x < 0 || pixmap.width() <= x ||
                y < 0 || pixmap.height() <= y)
                throw std::out_of_range("Index out of range.");
            return pixmap.getColor(x, y);
        })
    .def_property_readonly("__array_interface__",
        [] (const SkPixmap& pixmap) {
            return ImageInfoToArrayInterface(pixmap.info(), pixmap.rowBytes());
        })
    .def("tobytes",
        [] (const SkPixmap& pixmap) -> py::object {
            return py::module::import("builtins").attr("bytes")(pixmap);
        })
    .def(py::init<>(),
        R"docstring(
        Creates an empty :py:class:`Pixmap` without pixels, with
        :py:attr:`~ColorType.kUnknown_ColorType`, with
        :py:attr:`~AlphaType.kUnknown_AlphaType`, and with a width and height of
        zero.

        Use :py:meth:`reset` to associate pixels, :py:class:`ColorType`,
        :py:class:`AlphaType`, width, and height after :py:class:`Pixmap` has
        been created.

        :return: empty :py:class:`Pixmap`
        )docstring")
    .def(py::init(
        [] (const SkImageInfo& imageInfo, py::object data, size_t rowBytes) {
            if (data.is_none())
                return SkPixmap(imageInfo, nullptr, rowBytes);
            auto info = data.cast<py::buffer>().request();
            rowBytes = ValidateBufferToImageInfo(imageInfo, info, rowBytes);
            return SkPixmap(imageInfo, info.ptr, rowBytes);
        }),
        R"docstring(
        Creates :py:class:`Pixmap` from info width, height,
        :py:class:`AlphaType`, and :py:class:`ColorType`.

        ``data`` points to pixels, or nullptr. rowBytes should be
        ``info.width()`` times ``info.bytesPerPixel()``, or larger.

        No parameter checking is performed; it is up to the caller to ensure
        that data and rowBytes agree with info.

        The memory lifetime of pixels is managed by the caller. When
        :py:class:`Pixmap` goes out of scope, data is unaffected.

        :py:class:`Pixmap` may be later modified by :py:meth:`reset` to change
        its size, pixel type, or storage.

        :info: width, height, :py:class:`AlphaType`,
            :py:class:`ColorType` of :py:class:`ImageInfo`
        :data: pointer to pixels
            allocated by caller; may be nullptr
        :rowBytes: size of one row of data; width times pixel size, or
            larger
        )docstring",
        py::arg("info").none(false), py::arg("data"), py::arg("rowBytes"))
    .def(py::init(
        [] (py::array array, SkColorType ct, SkAlphaType at,
            const SkColorSpace* cs) {
            auto imageInfo = NumPyToImageInfo(array, ct, at, cs);
            return SkPixmap(imageInfo, array.mutable_data(), array.strides(0));
        }),
        R"docstring(
        Creates :py:class:`Pixmap` backed by numpy array.

        The memory lifetime of pixels is managed by the caller. When
        :py:class:`Pixmap` goes out of scope, data is unaffected.

        :array: numpy ndarray of shape=(height, width, channels). Must have
            non-zero width and height, and the valid number of channels for the
            specified color type.
        :colorType: color type of the array
        :alphaType: alpha type of the array
        :colorSpace: range of colors; may be nullptr
        )docstring",
        py::arg("array"),
        py::arg_v("colorType", kN32_SkColorType, "skia.ColorType.kN32_ColorType"),
        py::arg_v("alphaType", kUnpremul_SkAlphaType, "skia.AlphaType.kUnpremul_AlphaType"),
        py::arg("colorSpace") = nullptr)
    .def("reset",
        py::overload_cast<>(&SkPixmap::reset),
        R"docstring(
        Sets width, height, row bytes to zero; pixel address to nullptr;
        :py:class:`ColorType` to :py:attr:`~ColorType.kUnknown_ColorType`; and
        :py:class:`AlphaType` to :py:attr:`~Type.kUnknown_AlphaType`.

        The prior pixels are unaffected; it is up to the caller to release
        pixels memory if desired.
        )docstring")
    .def("reset",
        [] (SkPixmap& pixmap, const SkImageInfo& imageInfo, py::object data,
            size_t rowBytes) {
            if (data.is_none()) {
                pixmap.reset(imageInfo, nullptr, rowBytes);
                return;
            }
            auto info = data.cast<py::buffer>().request();
            rowBytes = ValidateBufferToImageInfo(imageInfo, info, rowBytes);
            pixmap.reset(imageInfo, info.ptr, rowBytes);
        },
        R"docstring(
        Sets width, height, :py:class:`AlphaType`, and :py:class:`ColorType`
        from info.

        Sets pixel address from data, which may be nullptr. Sets row bytes from
        rowBytes, which should be ``info.width()`` times
        ``info.bytesPerPixel()``, or larger.

        Does not check data. Asserts if built with SK_DEBUG defined and if
        rowBytes is too small to hold one row of pixels.

        The memory lifetime pixels are managed by the caller. When
        :py:class:`Pixmap` goes out of scope, data is unaffected.

        :param skia.ImageInfo info: width, height, :py:class:`AlphaType`,
            :py:class:`ColorType` of :py:class:`ImageInfo`
        :param Union[bytes,bytearray,memoryview] data: pointer to pixels
            allocated by caller; may be nullptr
        :param int rowBytes: size of one row of data; width times pixel size, or
            larger
        )docstring",
        py::arg("info"), py::arg("data"), py::arg("rowBytes"))
    .def("setColorSpace",
        [] (SkPixmap& pixmap, const SkColorSpace* colorSpace) {
            pixmap.setColorSpace(CloneColorSpace(colorSpace));
        },
        R"docstring(
        Changes :py:class:`ColorSpace` in :py:class:`ImageInfo`; preserves
        width, height, :py:class:`AlphaType`, and :py:class:`ColorType` in
        :py:class:`Image`, and leaves pixel address and row bytes unchanged.

        :py:class:`ColorSpace` reference count is incremented.

        :param skia.ColorSpace colorSpace: :py:class:`ColorSpace` moved to
            :py:class:`ImageInfo`
        )docstring",
        py::arg("colorSpace"))
    .def("extractSubset", &SkPixmap::extractSubset,
        R"docstring(
        Sets subset width, height, pixel address to intersection of
        :py:class:`Pixmap` with area, if intersection is not empty; and return
        true.

        Otherwise, leave subset unchanged and return false.

        Failing to read the return value generates a compile time warning.

        :param skia.Pixmap subset: storage for width, height, pixel address of
            intersection
        :param skia.IRect area: bounds to intersect with :py:class:`Pixmap`
        :return: true if intersection of :py:class:`Pixmap` and area is not
            empty
        )docstring",
        py::arg("subset").none(false), py::arg("area"))
    .def("info", &SkPixmap::info,
        R"docstring(
        Returns width, height, :py:class:`AlphaType`, :py:class:`ColorType`, and
        :py:class:`ColorSpace`.

        :return: reference to :py:class:`ImageInfo`
        )docstring")
    .def("rowBytes", &SkPixmap::rowBytes,
        R"docstring(
        Returns row bytes, the interval from one pixel row to the next.

        Row bytes is at least as large as: ``width() * info().bytesPerPixel()``.

        Returns zero if :py:meth:`colorType` is
        :py:attr:`~ColorType.kUnknown_ColorType`. It is up to the
        :py:class:`Bitmap` creator to ensure that row bytes is a useful value.

        :return: byte length of pixel row
        )docstring")
    .def("addr", &Addr<true>,
        R"docstring(
        Returns pixel address, the base address corresponding to the pixel
        origin.

        It is up to the :py:class:`Pixmap` creator to ensure that pixel address
        is a useful value.

        :return: pixel address
        :rtype: memoryview
        )docstring")
    .def("width", &SkPixmap::width,
        R"docstring(
        Returns pixel count in each pixel row.

        Should be equal or less than:
        :py:meth:`rowBytes` / ``info().bytesPerPixel()``.

        :return: pixel width in :py:class:`ImageInfo`
        )docstring")
    .def("height", &SkPixmap::height,
        R"docstring(
        Returns pixel row count.

        :return: pixel height in SkImageInfo
        )docstring")
    .def("dimensions", &SkPixmap::dimensions,
        R"docstring(
        Return the dimensions of the pixmap (from its ImageInfo)
        )docstring")
    .def("colorType", &SkPixmap::colorType)
    .def("alphaType", &SkPixmap::alphaType)
    .def("colorSpace", &SkPixmap::colorSpace,
        R"docstring(
        Returns :py:class:`ColorSpace`, the range of colors, associated with
        :py:class:`ImageInfo`.

        The reference count of :py:class:`ColorSpace` is unchanged. The returned
        :py:class:`ColorSpace` is immutable.

        :return: :py:class:`ColorSpace` in :py:class:`ImageInfo`, or nullptr
        )docstring",
        py::return_value_policy::reference_internal)
    .def("refColorSpace", &SkPixmap::refColorSpace,
        R"docstring(
        Returns smart pointer to :py:class:`ColorSpace`, the range of colors,
        associated with :py:class:`ImageInfo`.

        The smart pointer tracks the number of objects sharing this
        :py:class:`ColorSpace` reference so the memory is released when the
        owners destruct.

        The returned :py:class:`ColorSpace` is immutable.

        :return: :py:class:`ColorSpace` in :py:class:`ImageInfo` wrapped in a
            smart pointer
        )docstring")
    .def("isOpaque", &SkPixmap::isOpaque,
        R"docstring(
        Returns true if :py:class:`AlphaType` is :py:attr:`kOpaque_AlphaType`.

        Does not check if :py:class:`ColorType` allows alpha, or if any pixel
        value has transparency.

        :return: true if :py:class:`ImageInfo` has opaque :py:class:`AlphaType`
        )docstring")
    .def("bounds", &SkPixmap::bounds,
        R"docstring(
        Returns :py:class:`IRect` ( 0, 0, :py:meth:`width`, :py:meth:`height` ).

        :return: integral rectangle from origin to :py:meth:`width` and
            :py:meth:`height`
        )docstring")
    .def("rowBytesAsPixels", &SkPixmap::rowBytesAsPixels,
        R"docstring(
        Returns number of pixels that fit on row.

        Should be greater than or equal to :py:meth:`width`.

        :return: maximum pixels per row
        )docstring")
    .def("shiftPerPixel", &SkPixmap::shiftPerPixel,
        R"docstring(
        Returns bit shift converting row bytes to row pixels.

        Returns zero for :py:attr:`~ColorType.kUnknown_ColorType`.

        :return: one of: 0, 1, 2, 3; left shift to convert pixels to bytes
        )docstring")
    .def("computeByteSize", &SkPixmap::computeByteSize,
        R"docstring(
        Returns minimum memory required for pixel storage.

        Does not include unused memory on last row when
        :py:meth:`rowBytesAsPixels` exceeds width(). Returns SIZE_MAX if result
        does not fit in size_t. Returns zero if :py:meth:`height` or
        :py:meth:`width` is 0. Returns :py:meth:`height` times
        :py:meth:`rowBytes` if :py:meth:`colorType` is
        :py:attr:`~ColorType.kUnknown_ColorType`.

        :return: size in bytes of image buffer
        )docstring")
    .def("computeIsOpaque", &SkPixmap::computeIsOpaque,
        R"docstring(
        Returns true if all pixels are opaque.

        :py:class:`ColorType` determines how pixels are encoded, and whether
        pixel describes alpha. Returns true for :py:class:`ColorType` without
        alpha in each pixel; for other :py:class:`ColorType`, returns true if
        all pixels have alpha values equivalent to 1.0 or greater.

        For :py:class:`ColorType` :py:attr:`~ColorType.kRGB_565_ColorType` or
        :py:attr:`~ColorType.kGray_8_ColorType`: always returns true. For
        :py:class:`ColorType` :py:attr:`~ColorType.kAlpha_8_ColorType`,
        :py:attr:`~ColorType.kBGRA_8888_ColorType`,
        :py:attr:`~ColorType.kRGBA_8888_ColorType`: returns true if all pixel
        alpha values are 255. For :py:class:`ColorType`
        :py:attr:`~ColorType.kARGB_4444_ColorType`: returns true if all pixel
        alpha values are 15. For :py:attr:`~ColorType.kRGBA_F16_ColorType`:
        returns true if all pixel alpha values are 1.0 or greater.

        Returns false for :py:attr:`~ColorType.kUnknown_ColorType`.

        :return: true if all pixels have opaque values or :py:class:`ColorType`
            is opaque
        )docstring")
    .def("getColor", &SkPixmap::getColor,
        R"docstring(
        Returns pixel at (x, y) as unpremultiplied color.

        Returns black with alpha if :py:class:`ColorType` is
        :py:attr:`~ColorType.kAlpha_8_ColorType`.

        Input is not validated: out of bounds values of x or y trigger an
        assert() if built with SK_DEBUG defined; and returns undefined values or
        may crash if SK_RELEASE is defined. Fails if :py:class:`ColorType` is
        :py:attr:`~ColorType.kUnknown_ColorType` or pixel address is nullptr.

        :py:class:`ColorSpace` in :py:class:`ImageInfo` is ignored. Some color
        precision may be lost in the conversion to unpremultiplied color;
        original pixel data may have additional precision.

        :param int x: column index, zero or greater, and less than width()
        :param int y: row index, zero or greater, and less than height()
        :return: pixel converted to unpremultiplied color
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("getAlphaf", &SkPixmap::getAlphaf,
        R"docstring(
        Look up the pixel at (x,y) and return its alpha component, normalized to
        [0..1].

        This is roughly equivalent to :py:meth:`~skia.GetColorA`(
        :py:meth:`getColor` ), but can be more efficent (and more precise if the
        pixels store more than 8 bits per component).

        :param int x: column index, zero or greater, and less than width()
        :param int y: row index, zero or greater, and less than height()
        :return: alpha converted to normalized float
        )docstring",
        py::arg("x"), py::arg("y"))
    .def("addr8", &AddrN<uint8_t>,
        R"docstring(
        Returns readable base pixel address.

        Result is addressable as unsigned 8-bit bytes. Will trigger an assert()
        if :py:class:`ColorType` is not :py:attr:`~ColorType.kAlpha_8_ColorType`
        or :py:attr:`~ColorType.kGray_8_ColorType`, and is built with SK_DEBUG
        defined.

        One byte corresponds to one pixel.

        :return: readable unsigned 8-bit pointer to pixels
        :rtype: memoryview
        )docstring")
    .def("addr16",
        &AddrN<uint16_t>,
        R"docstring(
        Returns readable base pixel address.

        Result is addressable as unsigned 16-bit words. Will trigger an assert()
        if :py:class:`ColorType` is not :py:attr:`kRGB_565_ColorType` or
        :py:attr:`kARGB_4444_ColorType`, and is built with SK_DEBUG defined.

        One word corresponds to one pixel.

        :return: readable unsigned 16-bit pointer to pixels
        :rtype: memoryview
        )docstring")
    .def("addr32",
        &AddrN<uint32_t>,
        R"docstring(
        Returns readable base pixel address.

        Result is addressable as unsigned 32-bit words. Will trigger an assert()
        if :py:class:`ColorType` is not :py:attr:`kRGBA_8888_ColorType` or
        :py:attr:`kBGRA_8888_ColorType`, and is built with SK_DEBUG defined.

        One word corresponds to one pixel.

        :return: readable unsigned 32-bit pointer to pixels
        :rtype: memoryview
        )docstring")
    .def("addr64",
        &AddrN<uint64_t>,
        R"docstring(
        Returns readable base pixel address.

        Result is addressable as unsigned 64-bit words. Will trigger an assert()
        if :py:class:`ColorType` is not :py:attr:`kRGBA_F16_ColorType` and is
        built with SK_DEBUG defined.

        One word corresponds to one pixel.

        :return: readable unsigned 64-bit pointer to pixels
        :rtype: memoryview
        )docstring")
    .def("writable_addr",
        &Addr<false>,
        R"docstring(
        Returns writable base pixel address.

        :return: writable generic base pointer to pixels
        :rtype: memoryview
        )docstring")
    .def("readPixels", &ReadPixels<SkPixmap>,
        R"docstring(
        Copies :py:class:`Rect` of pixels to dstPixels.

        Copy starts at offset (srcX, srcY), and does not exceed
        :py:class:`Pixmap` (:py:meth:`width`, :py:meth:`height`).

        dstInfo specifies width, height, :py:class:`ColorType`,
        :py:class:`AlphaType`, and :py:class:`ColorSpace` of destination.
        dstRowBytes specifies the gap from one destination row to the next.
        Returns true if pixels are copied. Returns false if dstInfo address
        equals nullptr, or dstRowBytes is less than ``dstInfo.minRowBytes()``.

        Pixels are copied only if pixel conversion is possible. If
        :py:class:`Pixmap` :py:class:`ColorType` is
        :py:attr:`~ColorType.kGray_8_ColorType`, or
        :py:attr:`~ColorType.kAlpha_8_ColorType`; ``dstInfo.colorType``
        must match. If :py:class:`Pixmap` :py:class:`ColorType` is
        :py:attr:`~ColorType.kGray_8_ColorType`, ``dstInfo.colorSpace`` must
        match. If :py:class:`Pixmap` :py:class:`AlphaType` is
        :py:attr:`~AlphaType.kOpaque_AlphaType`, ``dstInfo.alphaType`` must
        match. If :py:class:`Pixmap` :py:class:`ColorSpace` is nullptr,
        ``dstInfo.colorSpace`` must match. Returns false if pixel conversion is
        not possible.

        srcX and srcY may be negative to copy only top or left of source.
        Returns false if :py:meth:`width` or :py:meth:`height` is zero or
        negative. Returns false if abs(srcX) >= Pixmap :py:meth:`width`, or if
        abs(srcY) >= Pixmap :py:meth:`height`.

        :dstInfo: destination width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`
        :dstPixels:   destination pixel storage
        :dstRowBytes: destination row length
        :srcX: column index whose absolute value is less than
            :py:meth:`width`
        :srcY: row index whose absolute value is less than
            :py:meth:`height`
        :return: true if pixels are copied to dstPixels
        )docstring",
        py::arg("dstInfo"), py::arg("dstPixels"), py::arg("dstRowBytes") = 0,
        py::arg("srcX") = 0, py::arg("srcY") = 0)
    .def("readPixels",
        py::overload_cast<const SkPixmap&, int, int>(
            &SkPixmap::readPixels, py::const_),
        R"docstring(
        Copies :py:class:`Rect` of pixels to dst.

        Copy starts at offset (srcX, srcY), and does not exceed
        :py:class:`Pixmap` (:py:meth:`width`, :py:meth:`height`).

        dstInfo specifies width, height, :py:class:`ColorType`,
        :py:class:`AlphaType`, and :py:class:`ColorSpace` of destination.
        dstRowBytes specifies the gap from one destination row to the next.
        Returns true if pixels are copied. Returns false if dstInfo address
        equals nullptr, or dstRowBytes is less than ``dstInfo.minRowBytes()``.

        Pixels are copied only if pixel conversion is possible. If
        :py:class:`Pixmap` :py:class:`ColorType` is
        :py:attr:`~ColorType.kGray_8_ColorType`, or
        :py:attr:`~ColorType.kAlpha_8_ColorType`; ``dstInfo.colorType``
        must match. If :py:class:`Pixmap` :py:class:`ColorType` is
        :py:attr:`~ColorType.kGray_8_ColorType`, ``dstInfo.colorSpace`` must
        match. If :py:class:`Pixmap` :py:class:`AlphaType` is
        :py:attr:`~AlphaType.kOpaque_AlphaType`, ``dstInfo.alphaType`` must
        match. If :py:class:`Pixmap` :py:class:`ColorSpace` is nullptr,
        ``dstInfo.colorSpace`` must match. Returns false if pixel conversion is
        not possible.

        srcX and srcY may be negative to copy only top or left of source.
        Returns false if :py:meth:`width` or :py:meth:`height` is zero or
        negative. Returns false if abs(srcX) >= Pixmap :py:meth:`width`, or if
        abs(srcY) >= Pixmap :py:meth:`height`.

        :dst: :py:class:`ImageInfo` and pixel address to write to
        :srcX: column index whose absolute value is less than
            :py:meth:`width`
        :srcY: row index whose absolute value is less than
            :py:meth:`height`
        :return: true if pixels are copied to dst
        )docstring",
        py::arg("dst"), py::arg("srcX") = 0, py::arg("srcY") = 0)
    .def("scalePixels", &SkPixmap::scalePixels,
        R"docstring(
        Copies :py:class:`Pixmap` to dst, scaling pixels to fit ``dst.width()``
        and ``dst.height()``, and converting pixels to match ``dst.colorType()``
        and ``dst.alphaType()``.

        Returns true if pixels are copied. Returns false if ``dst.addr()`` is
        nullptr, or ``dst.rowBytes()`` is less than dst
        :py:meth:`ImageInfo.minRowBytes`.

        Pixels are copied only if pixel conversion is possible. If
        :py:class:`Pixmap` :py:class:`ColorType` is
        :py:attr:`~ColorType.kGray_8_ColorType`, or
        :py:attr:`~ColorType.kAlpha_8_ColorType`; ``dst.colorType()`` must
        match. If :py:class:`Pixmap` :py:class:`ColorType` is
        :py:attr:`~ColorType.kGray_8_ColorType`, ``dst.colorSpace()`` must
        match. If :py:class:`Pixmap` :py:class:`AlphaType` is
        :py:attr:`~AlphaType.kOpaque_AlphaType`, ``dst.alphaType()`` must match.
        If :py:class:`Pixmap` :py:class:`ColorSpace` is nullptr,
        ``dst.colorSpace()`` must match. Returns false if pixel conversion is
        not possible.

        Returns false if :py:class:`Pixmap` :py:meth:`width` or
        :py:meth:`height` is zero or negative.

        Scales the image, with filterQuality, to match ``dst.width()`` and
        ``dst.height()``. filterQuality
        :py:attr:`~FilterQuality.None_FilterQuality` is fastest, typically
        implemented with nearest neighbor filter.
        :py:attr:`~FilterQuality.kLow_FilterQuality` is typically implemented
        with bilerp filter. :py:attr:`~FilterQuality.kMedium_FilterQuality` is
        typically implemented with bilerp filter, and mip-map filter when size
        is reduced. :py:attr:`~FilterQuality.kHigh_FilterQuality` is slowest,
        typically implemented with bicubic filter.

        :param skia.Pixmap dst: destination :py:class:`Pixmap`:
            :py:class:`ImageInfo`, pixels, row bytes
        :param skia.SamplingOptions options: sampling options
        :return: true if pixels are scaled to fit dst
        )docstring",
        py::arg("dst"),
        py::arg_v("samplingOptions", SkSamplingOptions(), "skia.SamplingOptions()"))
    .def("erase",
        py::overload_cast<const SkColor4f&, const SkIRect*>(
            &SkPixmap::erase, py::const_),
        R"docstring(
        Writes color to pixels bounded by subset; returns true on success.

        if subset is nullptr, writes colors pixels inside :py:meth:`bounds`.
        Returns false if :py:meth:`colorType` is
        :py:attr:`~ColorType.kUnknown_ColorType`, if subset is not nullptr and
        does not intersect :py:meth:`bounds`, or if subset is nullptr and
        :py:meth:`bounds` is empty.

        :param color: sRGB unpremultiplied color to write
        :param subset: bounding integer :py:class:`Rect` of pixels to write; may
            be nullptr
        :return: true if pixels are changed
        )docstring",
        py::arg("color"), py::arg("subset") = nullptr)
    ;

py::class_<SkYUVAPixmapInfo> yuvapixmapinfo(m, "YUVAPixmapInfo",
    R"docstring(
    :py:class:`~skia.YUVAInfo` combined with per-plane
    :py:class:`~skia.ColorType` and row bytes. Fully specifies the
    :py:class:`~skia.Pixmaps` for a YUVA image without the actual pixel memory
    and data.
    )docstring");

py::enum_<SkYUVAPixmapInfo::DataType>(yuvapixmapinfo, "DataType",
    R"docstring(
    Data type for Y, U, V, and possibly A channels independent of how values are
    packed into planes.
    )docstring")
    .value("kUnorm8", SkYUVAPixmapInfo::DataType::kUnorm8,
        "8 bit unsigned normalized")
    .value("kUnorm16", SkYUVAPixmapInfo::DataType::kUnorm16,
        "16 bit unsigned normalized")
    .value("kFloat16", SkYUVAPixmapInfo::DataType::kFloat16,
        "16 bit (half) floating point")
    .value("kUnorm10_Unorm2", SkYUVAPixmapInfo::DataType::kUnorm10_Unorm2,
        "10 bit unorm for Y, U, and V. 2 bit unorm for alpha (if present).")
    .value("kLast", SkYUVAPixmapInfo::DataType::kLast)
    .export_values();

py::class_<SkYUVAPixmapInfo::SupportedDataTypes>(
    yuvapixmapinfo, "SupportedDataTypes")
    .def(py::init<>(),
        R"docstring(
        Defaults to nothing supported.
        )docstring")
/* m116: The `SkYUVAPixmapInfo::SupportedDataTypes(const GrImageContext&)` constructor has been removed from
   the public API. */
/*
    .def(py::init<const GrImageContext&>(),
        R"docstring(
        Init based on texture formats supported by the context.
        )docstring",
        py::arg("context"))
*/
    .def_static("All", &SkYUVAPixmapInfo::SupportedDataTypes::All,
        R"docstring(
        All legal combinations of PlanarConfig and DataType are supported.
        )docstring")
    .def("supported", &SkYUVAPixmapInfo::SupportedDataTypes::supported,
        R"docstring(
        Checks whether there is a supported combination of color types for
        planes structured as indicated by PlanarConfig with channel data types
        as indicated by DataType.
        )docstring",
        py::arg("planarConfig"), py::arg("dataType"))
    .def("enableDataType",
        &SkYUVAPixmapInfo::SupportedDataTypes::enableDataType,
        R"docstring(
        Update to add support for pixmaps with numChannel channels where each
        channel is represented as DataType.
        )docstring",
        py::arg("dataType"), py::arg("numChannels"))
    ;

yuvapixmapinfo
    .def_readonly_static("kMaxPlanes", &SkYUVAPixmapInfo::kMaxPlanes)
    .def_static("DefaultColorTypeForDataType",
        &SkYUVAPixmapInfo::DefaultColorTypeForDataType,
        R"docstring(
        Gets the default SkColorType to use with numChannels channels, each
        represented as DataType. Returns kUnknown_SkColorType if no such color
        type.
        )docstring",
        py::arg("dataType"), py::arg("numChannels"))
    .def_static("NumChannelsAndDataType",
        &SkYUVAPixmapInfo::NumChannelsAndDataType,
        R"docstring(
        If the :py:class:`ColorType` is supported for YUVA pixmaps this will
        return the number of YUVA channels that can be stored in a plane of
        this color type and what the DataType is of those channels. If the
        :py:class:`ColorType` is not supported as a YUVA plane the number of
        channels is reported as 0 and the DataType returned should be
        ignored.
        )docstring",
        py::arg("colorType"))
    .def(py::init<>(),
        R"docstring(
        Default SkYUVAPixmapInfo is invalid.
        )docstring")
    .def(py::init(
        [] (const SkYUVAInfo& info, const std::vector<SkColorType>& colorType,
            py::object rowBytesOrNone) {
            std::vector<size_t> rowBytes;
            if (!rowBytesOrNone.is_none())
                rowBytes = rowBytesOrNone.cast<std::vector<size_t>>();
            if (colorType.size() < SkYUVAPixmapInfo::kMaxPlanes)
                throw py::value_error(py::str(
                    "colorType must have {} elements").format(
                    SkYUVAPixmapInfo::kMaxPlanes));
            if (!rowBytes.empty() &&
                (rowBytes.size() < SkYUVAPixmapInfo::kMaxPlanes))
                throw py::value_error(py::str(
                    "rowBytes must have {} elements").format(
                    SkYUVAPixmapInfo::kMaxPlanes));
            return SkYUVAPixmapInfo(info, colorType.data(),
                (rowBytes.empty()) ? nullptr : rowBytes.data());
        }),
        R"docstring(
        Initializes the :py:class:`YUVAPixmapInfo` from a :py:class:`YUVAInfo`
        with per-plane color types and row bytes. This will be invalid if the
        colorTypes aren't compatible with the :py:class:`YUVAInfo` or if a
        rowBytes entry is not valid for the plane dimensions and color type.
        Color type and row byte values beyond the number of planes in
        :py:class:`YUVAInfo` are ignored. All :py:class:`ColorTypes` must have
        the same DataType or this will be invalid.

        If rowBytes is nullptr then ``bpp*width`` is assumed for each plane.
        )docstring",
        py::arg("info"), py::arg("colorType"), py::arg("rowBytes") = nullptr)
    .def(py::init(
        [] (const SkYUVAInfo& info, SkYUVAPixmapInfo::DataType dataType,
            py::object rowBytesOrNone) {
            std::vector<size_t> rowBytes;
            if (!rowBytesOrNone.is_none())
                rowBytes = rowBytesOrNone.cast<std::vector<size_t>>();
            if (!rowBytes.empty() &&
                (rowBytes.size() < SkYUVAPixmapInfo::kMaxPlanes))
                throw py::value_error(py::str(
                    "rowBytes must have {} elements").format(
                    SkYUVAPixmapInfo::kMaxPlanes));
            return SkYUVAPixmapInfo(
                info, dataType, (rowBytes.empty()) ? nullptr : rowBytes.data());
        }),
        R"docstring(
        Like above but uses DefaultColorTypeForDataType to determine each
        plane's :py:class:`ColorType`. If rowBytes is nullptr then bpp*width is
        assumed for each plane.
        )docstring",
        py::arg("info"), py::arg("dataType"), py::arg("rowBytes") = nullptr)
    .def("__eq__",
        [] (const SkYUVAPixmapInfo& self, const SkYUVAPixmapInfo& that) {
            return self == that;
        },
        py::is_operator())
    .def("__ne__",
        [] (const SkYUVAPixmapInfo& self, const SkYUVAPixmapInfo& that) {
            return self != that;
        },
        py::is_operator())
    .def("yuvaInfo", &SkYUVAPixmapInfo::yuvaInfo)
    .def("yuvColorSpace", &SkYUVAPixmapInfo::yuvColorSpace)
    .def("numPlanes", &SkYUVAPixmapInfo::numPlanes,
        R"docstring(
        The number of :py:class:`Pixmap` planes, 0 if this
        :py:class:`YUVAPixmapInfo` is invalid.
        )docstring")
    .def("dataType", &SkYUVAPixmapInfo::dataType,
        R"docstring(
        The per-YUV[A] channel data type.
        )docstring")
    .def("rowBytes", &SkYUVAPixmapInfo::rowBytes,
        R"docstring(
        Row bytes for the ith plane. Returns zero if ``i >= numPlanes()`` or
        this :py:class:`YUVAPixmapInfo` is invalid.
        )docstring",
        py::arg("i"))
    .def("planeInfo", &SkYUVAPixmapInfo::planeInfo,
        R"docstring(
        Image info for the ith plane, or default :py:class:`ImageInfo` if
        ``i >= numPlanes()``
        )docstring",
        py::arg("i"))
    .def("computeTotalBytes",
        [] (const SkYUVAPixmapInfo& self, bool returnPlaneSizes) -> py::object {
            std::vector<size_t> planeSizes(SkYUVAPixmapInfo::kMaxPlanes);
            auto size = self.computeTotalBytes(planeSizes.data());
            if (returnPlaneSizes)
                return py::make_tuple(size, planeSizes);
            return py::cast(size);
        },
        R"docstring(
        Determine size to allocate for all planes. Optionally returns the
        per-plane sizes if returnPlaneSizes is True. If total size overflows
        will return SIZE_MAX and set all planeSizes to SIZE_MAX. Returns 0 and
        fills planesSizes with 0 if this :py:class:`YUVAPixmapInfo` is not
        valid.
        )docstring",
        py::arg("returnPlaneSizes") = false)
    .def("initPixmapsFromSingleAllocation",
        [] (const SkYUVAPixmapInfo& self, py::buffer b) {
            auto buffer = b.request();
            size_t size = (buffer.ndim) ? buffer.strides[0] * buffer.shape[0] : 0;
            if (self.computeTotalBytes() > size) {
                throw py::value_error(py::str(
                    "Allocation too small ({} < {})").format(
                        size, self.computeTotalBytes()));
            }
            std::vector<SkPixmap> pixmaps(SkYUVAPixmapInfo::kMaxPlanes);
            auto result = self.initPixmapsFromSingleAllocation(
                buffer.ptr, pixmaps.data());
            if (!result)
                throw std::runtime_error("Failed to initialize pixmaps.");
            return pixmaps;
        },
        R"docstring(
        Takes an allocation that is assumed to be at least
        :py:meth:`computeTotalBytes` in size and configures the first
        :py:meth:`numPlanes` entries in pixmaps array to point into that
        memory. The remaining entries of pixmaps are default initialized.
        Fails if this :py:class:`YUVAPixmapInfo` not valid.

        :param memory: Buffer that is at least  :py:meth:`computeTotalBytes` in
            size.
        :rtype: List[skia.Pixmap]
        )docstring",
        py::arg("memory"))
    .def("isValid", &SkYUVAPixmapInfo::isValid,
        R"docstring(
        Returns true if this has been configured with a non-empty dimensioned
        :py:class:`YUVAInfo` with compatible color types and row bytes.
        )docstring")
    .def("isSupported", &SkYUVAPixmapInfo::isSupported,
        R"docstring(
        Is this valid and does it use color types allowed by the passed
        :py:class:`SupportedDataTypes`?
        )docstring",
        py::arg("supportedDataTypes"))
    ;


py::class_<SkYUVAPixmaps>(m, "YUVAPixmaps",
    R"docstring(
    Helper to store :py:class:`Pixmap` planes as described by a
    :py:class:`YUVAPixmapInfo`. Can be responsible for
    allocating/freeing memory for pixmaps or use external memory.
    )docstring")
    .def_static("Allocate", &SkYUVAPixmaps::Allocate,
        R"docstring(
        Allocate space for pixmaps' pixels in the :py:class:`YUVAPixmaps`.
        )docstring",
        py::arg("yuvaPixmapInfo"))
    .def_static("FromData", &SkYUVAPixmaps::FromData,
        R"docstring(
        Use storage in :py:class:`Data` as backing store for pixmaps' pixels.
        :py:class:`Data` is retained by the :py:class:`YUVAPixmaps`.
        )docstring",
        py::arg("yuvaPixmapInfo"), py::arg("data"))
    .def_static("FromExternalMemory",
        [] (const SkYUVAPixmapInfo& info, py::buffer b) {
            auto buffer = b.request();
            size_t size = (buffer.ndim) ? buffer.strides[0] * buffer.shape[0] : 0;
            if (info.computeTotalBytes() > size) {
                throw py::value_error(py::str(
                    "Memory backstore too small ({} < {})").format(
                        size, info.computeTotalBytes()));
            }
            return SkYUVAPixmaps::FromExternalMemory(info, buffer.ptr);
        },
        R"docstring(
        Use passed in memory as backing store for pixmaps' pixels. Caller must
        ensure memory remains allocated while pixmaps are in use. There must be
        at least :py:meth:`YUVAPixmapInfo.computeTotalBytes` allocated starting
        at memory.
        )docstring",
        py::arg("yuvaPixmapInfo"), py::arg("memory"))
    .def_static("FromExternalPixmaps",
        [] (const SkYUVAInfo& info,
            const std::vector<SkPixmap>& pixmaps) {
            if (pixmaps.size() < SkYUVAPixmaps::kMaxPlanes)
                throw py::value_error(py::str(
                    "pixmaps must have {} elements").format(
                    SkYUVAPixmaps::kMaxPlanes));
            return SkYUVAPixmaps::FromExternalPixmaps(info, pixmaps.data());
        },
        R"docstring(
        Wraps existing :py:class:`Pixmap`. The :py:class:`YUVAPixmaps` will
        have no ownership of the :py:class:`Pixmap`' pixel memory so the
        caller must ensure it remains valid. Will return an invalid
        :py:class:`YUVAPixmaps` if the :py:class:`YUVAInfo` isn't compatible
        with the :py:class:`Pixmap` array (number of planes, plane dimensions,
        sufficient color channels in planes, ...).
        )docstring",
        py::arg("yuvaInfo"), py::arg("pixmaps"))
    .def(py::init<>())
    .def("isValid", &SkYUVAPixmaps::isValid,
        R"docstring(
        Does have initialized pixmaps compatible with its :py:class:`YUVAInfo`.
        )docstring")
    .def("yuvaInfo", &SkYUVAPixmaps::yuvaInfo)
    .def("numPlanes", &SkYUVAPixmaps::numPlanes,
        R"docstring(
        Number of pixmap planes or 0 if this :py:class:`YUVAPixmaps` is invalid.
        )docstring")
    .def("planes", &SkYUVAPixmaps::planes,
        R"docstring(
        Access the :py:class:`Pixmap` planes. They are default initialized if
        this is not a valid :py:class:`YUVAPixmaps`.
        )docstring")
    .def("plane", &SkYUVAPixmaps::plane,
        R"docstring(
        Get the ith :py:class:`Pixmap` plane. :py:class:`Pixmap` will be default
        initialized if i >= numPlanes or this :py:class:`YUVAPixmaps` is
        invalid.
        )docstring",
        py::arg("i"))
/*
    .def("toLegacy",
        [] (const SkYUVAPixmaps& self) {
            SkYUVASizeInfo info;
            std::vector<SkYUVAIndex> indices(SkYUVAPixmaps::kMaxPlanes);
            self.toLegacy(&info, indices.data());
            return py::make_tuple(info, indices);
        },
        R"docstring(
        Conversion to legacy SkYUVA data structures.
        )docstring")
*/
    ;

/*
py::class_<SkYUVAIndex> yuvaindex(m, "YUVAIndex");

py::enum_<SkYUVAIndex::Index>(yuvaindex, "Index")
    .value("kY_Index", SkYUVAIndex::kY_Index)
    .value("kU_Index", SkYUVAIndex::kU_Index)
    .value("kV_Index", SkYUVAIndex::kV_Index)
    .value("kA_Index", SkYUVAIndex::kA_Index)
    .value("kLast_Index", SkYUVAIndex::kLast_Index)
    .export_values();

yuvaindex
    .def(py::init<>())
    .def("__eq__",
        [] (const SkYUVAIndex& self, const SkYUVAIndex& that) {
            return self == that;
        },
        py::is_operator())
    .def("__ne__",
        [] (const SkYUVAIndex& self, const SkYUVAIndex& that) {
            return self == that;
        },
        py::is_operator())
    .def_readonly_static("kIndexCount", &SkYUVAIndex::kIndexCount)
    .def_readwrite("fIndex", &SkYUVAIndex::fIndex,
        R"docstring(
        The index is a number between -1..3 which defines which image source
        to read from, where -1 means the image source doesn't exist. The
        assumption is we will always have image sources for each of YUV
        planes, but optionally have image source for A plane.
        )docstring")
    .def_readwrite("fChannel", &SkYUVAIndex::fChannel,
        R"docstring(
        The channel describes from which channel to read the info from.
        Currently we only deal with YUV and NV12 and channel info is ignored.
        )docstring")
    .def_static("AreValidIndices",
        [] (const std::vector<SkYUVAIndex>& yuvaIndices, bool returnNumPlanes)
        -> py::object {
            int numPlanes = 0;
            if (yuvaIndices.size() < 4)
                throw py::value_error("yuvaIndices must have 4 elements.");
            auto result = SkYUVAIndex::AreValidIndices(
                yuvaIndices.data(), &numPlanes);
            if (returnNumPlanes)
                return py::make_tuple(result, numPlanes);
            return py::cast(result);
        },
        py::arg("yuvaIndices"), py::arg("returnNumPlanes") = false)
    ;

py::class_<SkYUVASizeInfo>(m, "YUVASizeInfo")
    .def(py::init<>())
    .def_property("fSizes",
        [] (const SkYUVASizeInfo& self) {
            return std::vector<SkISize>(
                self.fSizes, self.fSizes + SkYUVASizeInfo::kMaxCount);
        },
        [] ( SkYUVASizeInfo& self, const std::vector<SkISize>& value) {
            if (value.size() != SkYUVASizeInfo::kMaxCount)
                throw py::value_error(py::str(
                    "value must have {} elements.").format(
                    SkYUVASizeInfo::kMaxCount));
            std::copy(value.begin(), value.end(), self.fSizes);
        })
    .def_property("fWidthBytes",
        [] (const SkYUVASizeInfo& self) {
            return std::vector<size_t>(
                self.fWidthBytes, self.fWidthBytes + SkYUVASizeInfo::kMaxCount);
        },
        [] ( SkYUVASizeInfo& self, const std::vector<size_t>& value) {
            if (value.size() != SkYUVASizeInfo::kMaxCount)
                throw py::value_error(py::str(
                    "value must have {} elements.").format(
                    SkYUVASizeInfo::kMaxCount));
            std::copy(value.begin(), value.end(), self.fWidthBytes);
        },
        R"docstring(
        While the widths of the Y, U, V and A planes are not restricted, the
        implementation often requires that the width of the memory allocated
        for each plane be a multiple of 8.

        This struct allows us to inform the client how many "widthBytes"
        that we need.  Note that we use the new idea of "widthBytes"
        because this idea is distinct from "rowBytes" (used elsewhere in
        Skia).  "rowBytes" allow the last row of the allocation to not
        include any extra padding, while, in this case, every single row of
        the allocation must be at least "widthBytes".
        )docstring")
    .def_readwrite("fOrigin", &SkYUVASizeInfo::fOrigin,
        R"docstring(
        YUVA data often comes from formats like JPEG that support EXIF
        orientation. Code that operates on the raw YUV data often needs to
        know that orientation.
        )docstring")
    .def("__eq__",
        [] (const SkYUVASizeInfo& self, const SkYUVASizeInfo& that) {
            return self == that;
        },
        py::is_operator())
    .def("computeTotalBytes", &SkYUVASizeInfo::computeTotalBytes)
    // .def("computePlanes", &SkYUVASizeInfo)
    ;
*/

}
