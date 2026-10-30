#include "common.h"
#include <include/core/SkSurfaceProps.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/private/chromium/GrSurfaceCharacterization.h>
#include <include/gpu/GpuTypes.h>
#include <include/gpu/ganesh/GrTypes.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/MutableTextureState.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>

namespace {

void PyReadPixelsCallback (
    void* context,
    std::unique_ptr<const SkSurface::AsyncReadResult> result) {
    if (!context)
        throw std::runtime_error("Null pointer exception.");
    auto callback = py::reinterpret_steal<py::function>(
        reinterpret_cast<PyObject*>(context));
    callback(result.get());
}

}  // namespace


/*
const SkSurfaceProps::Flags SkSurfaceProps::kUseDistanceFieldFonts_Flag;
*/

void initSurface(py::module &m) {

/* m111: SkBackingFit is no longer part of the public API. */
/*
py::enum_<SkBackingFit>(m, "BackingFit", R"docstring(
    Indicates whether a backing store needs to be an exact match or can be
    larger than is strictly necessary.
    )docstring")
    .value("kApprox", SkBackingFit::kApprox)
    .value("kExact", SkBackingFit::kExact)
    .export_values();
*/

py::enum_<SkPixelGeometry>(m, "PixelGeometry", R"docstring(
    Description of how the LCD strips are arranged for each pixel.

    If this is unknown, or the pixels are meant to be "portable" and/or
    transformed before showing (e.g. rotated, scaled) then use
    :py:attr:`PixelGeometry.kUnknown`.
    )docstring")
    .value("kUnknown_PixelGeometry", SkPixelGeometry::kUnknown_SkPixelGeometry)
    .value("kRGB_H_PixelGeometry", SkPixelGeometry::kRGB_H_SkPixelGeometry)
    .value("kBGR_H_PixelGeometry", SkPixelGeometry::kBGR_H_SkPixelGeometry)
    .value("kRGB_V_PixelGeometry", SkPixelGeometry::kRGB_V_SkPixelGeometry)
    .value("kBGR_V_PixelGeometry", SkPixelGeometry::kBGR_V_SkPixelGeometry)
    .export_values();

py::class_<SkSurfaceProps> surfaceprops(m, "SurfaceProps", R"docstring(
    Describes properties and constraints of a given :py:class:`Surface`.

    The rendering engine can parse these during drawing, and can sometimes
    optimize its performance (e.g. disabling an expensive feature).
    )docstring");

py::enum_<SkSurfaceProps::Flags>(surfaceprops, "Flags", py::arithmetic())
    .value("kDefault_Flag",
        SkSurfaceProps::Flags::kDefault_Flag)
    .value("kUseDeviceIndependentFonts_Flag",
        SkSurfaceProps::Flags::kUseDeviceIndependentFonts_Flag)
    .value("kDynamicMSAA_Flag",
        SkSurfaceProps::Flags::kDynamicMSAA_Flag)
    .value("kAlwaysDither_Flag",
        SkSurfaceProps::Flags::kAlwaysDither_Flag)
    .export_values();

/* SkSurfaceProps::kLegacyFontHost_InitType was removed in m88.
   Its usage was replaced by:
       SkSurfaceProps(SkSurfaceProps::kLegacyFontHost_InitType)
           -> SkSurfaceProps() - private in m87 to public in m88
       SkSurfaceProps(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
                      SkSurfaceProps::kLegacyFontHost_InitType)
           -> SkSurfaceProps(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
                             SkSurfaceProps::kUnknown_SkPixelGeometry) - different constructor
 */

surfaceprops
    .def(py::init<>())
    .def(py::init<uint32_t, SkPixelGeometry>(),
        py::arg("flags"), py::arg("geometry"))
    .def(py::init<uint32_t, SkPixelGeometry, SkScalar, SkScalar>(),
        "Construct native surface properties with explicit text contrast/gamma.",
        py::arg("flags"), py::arg("geometry"), py::arg("textContrast"), py::arg("textGamma"))
/*
    .def(py::init<SkSurfaceProps::InitType>(),
        py::arg("initType"))
    .def(py::init<uint32_t, SkSurfaceProps::InitType>(),
        py::arg("flags"), py::arg("initType"))
*/
    .def(py::init<const SkSurfaceProps&>(),
        py::arg("props"))
    .def("flags", &SkSurfaceProps::flags)
    .def("pixelGeometry", &SkSurfaceProps::pixelGeometry)
    .def("textContrast", &SkSurfaceProps::textContrast,
        "Return the native text mask contrast.")
    .def("textGamma", &SkSurfaceProps::textGamma,
        "Return the native text mask gamma; zero selects the native sRGB response.")
    .def("isUseDeviceIndependentFonts",
        &SkSurfaceProps::isUseDeviceIndependentFonts)
    .def(py::self == py::self)
    .def(py::self != py::self)
/*
    .def_readonly_static("kUseDistanceFieldFonts_Flag",
        &SkSurfaceProps::kUseDistanceFieldFonts_Flag,
        R"docstring(
        Deprecated alias used by Chromium.

        Will be removed.
        )docstring")
*/
    ;

py::class_<GrSurfaceCharacterization>(m, "SurfaceCharacterization")
    .def(py::init())
    .def("createResized", &GrSurfaceCharacterization::createResized,
        py::arg("width"), py::arg("height"))
    .def("createColorSpace", &GrSurfaceCharacterization::createColorSpace,
        py::arg("cs"))
    .def("createBackendFormat", &GrSurfaceCharacterization::createBackendFormat,
        py::arg("colorType"), py::arg("backendFormat"))
    .def("createFBO0", &GrSurfaceCharacterization::createFBO0,
        py::arg("usesGLFBO0"))
    .def(py::self == py::self)
    .def(py::self != py::self)
    .def("cacheMaxResourceBytes",
        &GrSurfaceCharacterization::cacheMaxResourceBytes)
    .def("isValid", &GrSurfaceCharacterization::isValid)
    .def("width", &GrSurfaceCharacterization::width)
    .def("height", &GrSurfaceCharacterization::height)
/*
    #if !SK_SUPPORT_GPU
    .def("stencilCount", &GrSurfaceCharacterization::stencilCount)
    #endif
*/
    .def("isTextureable", &GrSurfaceCharacterization::isTextureable)
    .def("isMipMapped", &GrSurfaceCharacterization::isMipMapped)
    .def("usesGLFBO0", &GrSurfaceCharacterization::usesGLFBO0)
    .def("vulkanSecondaryCBCompatible",
        &GrSurfaceCharacterization::vulkanSecondaryCBCompatible)
    .def("colorSpace", &GrSurfaceCharacterization::colorSpace,
        py::return_value_policy::reference_internal)
    .def("refColorSpace", &GrSurfaceCharacterization::refColorSpace)
    .def("surfaceProps", &GrSurfaceCharacterization::surfaceProps)
    ;

py::class_<SkSurface, sk_sp<SkSurface>, SkRefCnt> surface(
    m, "Surface", py::buffer_protocol(), R"docstring(
    :py:class:`Surface` is responsible for managing the pixels that a canvas
    draws into.

    The pixels can be allocated either in CPU memory (a raster surface) or on
    the GPU (a GrRenderTarget surface). :py:class:`Surface` takes care of
    allocating a :py:class:`Canvas` that will draw into the surface. Call
    :py:meth:`getCanvas` to use that canvas (but don't delete it, it is owned
    by the surface). :py:class:`Surface` always has non-zero dimensions. If
    there is a request for a new surface, and either of the requested dimensions
    are zero, then nullptr will be returned.

    Example::

        surface = skia.Surface(640, 480)
        with surface as canvas:
            draw(canvas)
        image = surface.makeImageSnapshot()

    )docstring");

py::class_<SkSurface::AsyncReadResult>(surface, "AsyncReadResult", R"docstring(
    The result from :py:meth:`Surface.asyncRescaleAndReadPixels` or
    :py:meth:`Surface.asyncRescaleAndReadPixelsYUV420`.
    )docstring")
    .def("count", &SkSurface::AsyncReadResult::count)
    .def("data", &SkSurface::AsyncReadResult::data, py::arg("i"))
    .def("rowBytes", &SkSurface::AsyncReadResult::rowBytes, py::arg("i"))
    ;

py::enum_<SkSurface::ContentChangeMode>(surface, "ContentChangeMode")
    .value("kDiscard_ContentChangeMode",
        SkSurface::ContentChangeMode::kDiscard_ContentChangeMode,
        "discards surface on change")
    .value("kRetain_ContentChangeMode",
        SkSurface::ContentChangeMode::kRetain_ContentChangeMode,
        "preserves surface on change")
    .export_values();

py::enum_<SkSurfaces::BackendHandleAccess>(surface, "BackendHandleAccess")
    .value("kFlushRead_BackendHandleAccess",
        SkSurfaces::BackendHandleAccess::kFlushRead,
        "back-end object is readable")
    .value("kFlushWrite_BackendHandleAccess",
        SkSurfaces::BackendHandleAccess::kFlushWrite,
        "back-end object is writable")
    .value("kDiscardWrite_BackendHandleAccess",
        SkSurfaces::BackendHandleAccess::kDiscardWrite,
        "back-end object must be overwritten")
    .export_values();

py::enum_<SkSurface::RescaleGamma>(surface, "RescaleGamma", R"docstring(
    Controls the gamma that rescaling occurs in for
    :py:meth:`Surface.asyncRescaleAndReadPixels` and
    :py:meth:`Surface.asyncRescaleAndReadPixelsYUV420`.
    )docstring", py::arithmetic())
    .value("kSrc", SkSurface::RescaleGamma::kSrc)
    .value("kLinear", SkSurface::RescaleGamma::kLinear)
    .export_values();

py::enum_<SkSurfaces::BackendSurfaceAccess>(surface, "BackendSurfaceAccess")
    .value("kNoAccess", SkSurfaces::BackendSurfaceAccess::kNoAccess,
        "back-end object will not be used by client")
    .value("kPresent", SkSurfaces::BackendSurfaceAccess::kPresent,
        "back-end surface will be used for presenting to screen")
    .export_values();

surface
    .def("__repr__",
        [] (const SkSurface& surface) {
            return py::str("Surface({}, {})").format(
                surface.width(), surface.height());
        })
    .def("__enter__",
        [] (SkSurface& surface) { return surface.getCanvas(); },
        py::return_value_policy::reference_internal)
    .def("__exit__",
        [] (const SkSurface& surface, py::object exc_type, py::object exc_value,
            py::object traceback) { })
    .def("toarray", &ReadToNumpy<SkSurface>,
        R"docstring(
        Exports a ``numpy.ndarray``.

        :param srcX: offset into readable pixels on x-axis; may be negative
        :param srcY: offset into readable pixels on y-axis; may be negative
        :param colorType: target :py:class:`ColorType`
        :param alphaType: target :py:class:`AlphaType`
        :param colorSpace: target :py:class:`ColorSpace`
        :return: numpy.ndarray
        )docstring",
        py::arg("srcX") = 0, py::arg("srcY") = 0,
        py::arg_v("colorType", kUnknown_SkColorType, "skia.ColorType.kUnknown_ColorType"),
        py::arg_v("alphaType", kUnpremul_SkAlphaType, "skia.AlphaType.kUnpremul_AlphaType"),
        py::arg("colorSpace") = nullptr)
    .def(py::init(
        [] (int width, int height, const SkSurfaceProps* surfaceProps) {
            return SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height), surfaceProps);
        }),
        R"docstring(
        See :py:meth:`~MakeRasterN32Premul`
        )docstring",
        py::arg("width"), py::arg("height"), py::arg("surfaceProps") = nullptr)
    .def(py::init(
        [] (int width, int height, SkColorType colortype, const SkSurfaceProps* surfaceProps) {
            auto info = SkImageInfo::MakeN32Premul(width, height).makeColorType(colortype);
            return SkSurfaces::Raster(info, surfaceProps);
        }),
        R"docstring(
        See :py:meth:`~MakeRasterN32Premul` plus :py:class:`ImageInfo`'s :py:meth:`~makeColorType`
        )docstring",
        py::arg("width"), py::arg("height"), py::arg("colortype"), py::arg("surfaceProps") = nullptr)
    .def(py::init(
        [] (const SkImageInfo& imageinfo, size_t rowBytes, const SkSurfaceProps* surfaceProps) {
            return SkSurfaces::Raster(imageinfo, rowBytes, surfaceProps);
        }),
        R"docstring(
        See :py:meth:`~MakeRaster`
        )docstring",
        py::arg("imageInfo"), py::arg("rowBytes") = 0,
        py::arg("surfaceProps") = nullptr)
    .def(py::init(
        [] (py::array array, SkColorType ct, SkAlphaType at,
            const SkColorSpace* cs, const SkSurfaceProps *surfaceProps) {
            auto imageInfo = NumPyToImageInfo(array, ct, at, cs);
            auto surface = SkSurfaces::WrapPixels(
                imageInfo, array.mutable_data(), array.strides(0),
                surfaceProps);
            if (!surface)
                throw std::runtime_error("Failed to create Canvas");
            return surface;
        }),
        R"docstring(
        Create a raster surface on numpy array.

        Input array must have compatible shape with the given color type.

        Do not destroy numpy array while using this surface.

        :array: numpy ndarray of shape=(height, width, channels) and appropriate
            dtype. Must have non-zero width and height, and the valid number of
            channels for the specified color type.
        :colorType: color type of the array
        :alphaType: alpha type of the array
        :colorSpace: range of colors; may be nullptr
        )docstring",
        py::arg("array"),
        py::arg_v("colorType", kN32_SkColorType, "skia.ColorType.kN32_ColorType"),
        py::arg_v("alphaType", kUnpremul_SkAlphaType, "skia.AlphaType.kUnpremul_AlphaType"),
        py::arg("colorSpace") = nullptr, py::arg("surfaceProps") = nullptr)
    .def("isCompatible", &SkSurface::isCompatible,
        R"docstring(
        Is this surface compatible with the provided characterization?

        This method can be used to determine if an existing :py:class:`Surface`
        is a viable destination for an :py:class:`DeferredDisplayList`.

        :param skia.SurfaceCharacterization characterization: The
            characterization for which a compatibility check is desired
        :return: true if this surface is compatible with the characterization;
            false otherwise
        )docstring",
        py::arg("characterization"))
    .def("width", &SkSurface::width,
        R"docstring(
        Returns pixel count in each row; may be zero or greater.

        :return: number of pixel columns
        )docstring")
    .def("height", &SkSurface::height,
        R"docstring(
        Returns pixel row count; may be zero or greater.

        :return: number of pixel rows
        )docstring")
    .def("imageInfo", &SkSurface::imageInfo,
        R"docstring(
        Returns an :py:class:`ImageInfo` describing the surface.
        )docstring")
    .def("generationID", &SkSurface::generationID,
        R"docstring(
        Returns unique value identifying the content of :py:class:`Surface`.

        Returned value changes each time the content changes. Content is changed
        by drawing, or by calling :py:meth:`notifyContentWillChange`.

        :return: unique content identifier
        )docstring")
    .def("notifyContentWillChange", &SkSurface::notifyContentWillChange,
        R"docstring(
        Notifies that :py:class:`Surface` contents will be changed by code
        outside of Skia.

        Subsequent calls to :py:meth:`generationID` return a different value.
        )docstring",
        py::arg("mode"))
    .def("recordingContext", &SkSurface::recordingContext,
        R"docstring(
        Returns the recording context being used by the :py:class:`Surface`.

        :return: GPU context, if available; nullptr otherwise
        )docstring",
        py::return_value_policy::reference_internal)
    .def("getBackendTexture", &SkSurfaces::GetBackendTexture,
        R"docstring(
        Retrieves the back-end texture.

        If :py:class:`Surface` has no back-end texture, an invalid object is
        returned. Call :py:meth:`GrBackendTexture.isValid` to determine if the
        result is valid.

        The returned :py:class:`GrBackendTexture` should be discarded if the
        :py:class:`Surface` is drawn to or deleted.

        :return: GPU texture reference; invalid on failure
        )docstring",
        py::arg("backendHandleAccess"))
    .def("getBackendRenderTarget", &SkSurfaces::GetBackendRenderTarget,
        R"docstring(
        Retrieves the back-end render target.

        If :py:class:`Surface` has no back-end render target, an invalid object
        is returned. Call :py:meth:`GrBackendRenderTarget.isValid` to determine
        if the result is valid.

        The returned :py:class:`GrBackendRenderTarget` should be discarded if
        the :py:class:`Surface` is drawn to or deleted.

        :return: GPU render target reference; invalid on failure
        )docstring",
        py::arg("backendHandleAccess"))
    .def("replaceBackendTexture",
        [] (SkSurface& surface, const GrBackendTexture& backendTexture,
            GrSurfaceOrigin origin, SkSurface::ContentChangeMode mode) {
            return surface.replaceBackendTexture(backendTexture, origin, mode);
        },
        R"docstring(
        If the surface was made via :py:meth:`MakeFromBackendTexture` then it's
        backing texture may be substituted with a different texture.

        The contents of the previous backing texture are copied into the new
        texture. :py:class:`Canvas` state is preserved. The original sample
        count is used. The :py:class:`GrBackendFormat` and dimensions of
        replacement texture must match that of the original.

        :param backendTexture:  the new backing texture for the surface
        :param mode:    Retain or discard current Content
        )docstring",
        py::arg("backendTexture"), py::arg("origin"),
        py::arg_v("mode", SkSurface::kRetain_ContentChangeMode,
                  "skia.Surface.ContentChangeMode.kRetain_ContentChangeMode"))
    .def("getCanvas", &SkSurface::getCanvas,
        R"docstring(
        Returns :py:class:`Canvas` that draws into :py:class:`Surface`.

        Subsequent calls return the same :py:class:`Canvas`. :py:class:`Canvas`
        returned is managed and owned by :py:class:`Surface`, and is deleted
        when :py:class:`Surface` is deleted.

        :return: drawing :py:class:`Canvas` for :py:class:`Surface`
        )docstring",
        py::return_value_policy::reference_internal)
    .def("makeSurface",
        py::overload_cast<const SkImageInfo&>(&SkSurface::makeSurface),
        R"docstring(
        Returns a compatible :py:class:`Surface`, or nullptr.

        Returned :py:class:`Surface` contains the same raster, GPU, or null
        properties as the original. Returned :py:class:`Surface` does not share
        the same pixels.

        Returns nullptr if imageInfo width or height are zero, or if imageInfo
        is incompatible with :py:class:`Surface`.

        :param skia.ImageInfo imageInfo: width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`, of
            :py:class:`Surface`; width and height must be greater than zero
        :return: compatible :py:class:`Surface` or nullptr
        )docstring",
        py::arg("imageInfo"))
    .def("makeSurface",
        py::overload_cast<int, int>(&SkSurface::makeSurface),
        R"docstring(
        Calls :py:meth:`makeSurface()` with the same ImageInfo as this surface,
        but with the specified width and height.
        )docstring",
        py::arg("width"), py::arg("height"))
    .def("makeImageSnapshot",
        py::overload_cast<>(&SkSurface::makeImageSnapshot),
        R"docstring(
        Returns :py:class:`Image` capturing :py:class:`Surface` contents.

        Subsequent drawing to :py:class:`Surface` contents are not captured.
        :py:class:`Image` allocation is accounted for if :py:class:`Surface` was
        created with :py:attr:`Budgeted.kYes`.

        :return: :py:class:`Image` initialized with :py:class:`Surface` contents
        )docstring")
    .def("makeImageSnapshot",
        py::overload_cast<const SkIRect &>(&SkSurface::makeImageSnapshot),
        R"docstring(
        Like the no-parameter version, this returns an image of the current
        surface contents.

        This variant takes a rectangle specifying the subset of the surface that
        is of interest. These bounds will be sanitized before being used.

        - If bounds extends beyond the surface, it will be trimmed to just the
            intersection of it and the surface.
        - If bounds does not intersect the surface, then this returns nullptr.
        - If bounds == the surface, then this is the same as calling the
            no-parameter variant.
        )docstring",
        py::arg("bounds"))
    .def("draw",
        py::overload_cast<SkCanvas*, SkScalar, SkScalar, const SkPaint*>(
            &SkSurface::draw),
        R"docstring(
        Draws :py:class:`Surface` contents to canvas, with its top-left corner
        at (x, y).

        If :py:class:`Paint` paint is not nullptr, apply
        :py:class:`ColorFilter`, alpha, :py:class:`ImageFilter`,
        :py:class:`BlendMode`, and :py:class:`DrawLooper`.

        :canvas: :py:class:`Canvas` drawn into
        :x: horizontal offset in :py:class:`Canvas`
        :y: vertical offset in :py:class:`Canvas`
        :paint: :py:class:`Paint` containing :py:class:`BlendMode`,
            :py:class:`ColorFilter`, :py:class:`ImageFilter`, and so on; or
            nullptr
        )docstring",
        py::arg("canvas"), py::arg("x"), py::arg("y"),
        py::arg("paint") = nullptr)
    .def("peekPixels", &SkSurface::peekPixels,
        R"docstring(
        Copies :py:class:`Surface` pixel address, row bytes, and
        :py:class:`ImageInfo` to :py:class:`Pixmap`, if address is available,
        and returns true.

        If pixel address is not available, return false and leave
        :py:class:`Pixmap` unchanged.

        pixmap contents become invalid on any future change to
        :py:class:`Surface`.

        :param skia.Pixmap pixmap: storage for pixel state if pixels are
            readable; otherwise, ignored
        :return: true if :py:class:`Surface` has direct access to pixels
        )docstring",
        py::arg("pixmap"))
    .def("readPixels",
        py::overload_cast<const SkPixmap&, int, int>(&SkSurface::readPixels),
        R"docstring(
        Copies :py:class:`Rect` of pixels to dst.

        Source :py:class:`Rect` corners are (srcX, srcY) and :py:class:`Surface`
        (width(), height()). Destination :py:class:`Rect` corners are (0, 0) and
        (dst.width(), dst.height()). Copies each readable pixel intersecting
        both rectangles, without scaling, converting to dst.colorType() and
        dst.alphaType() if required.

        Pixels are readable when :py:class:`Surface` is raster, or backed by a
        GPU.

        The destination pixel storage must be allocated by the caller.

        Pixel values are converted only if :py:class:`ColorType` and
        :py:class:`AlphaType` do not match. Only pixels within both source and
        destination rectangles are copied. dst contents outside :py:class:`Rect`
        intersection are unchanged.

        Pass negative values for srcX or srcY to offset pixels across or down
        destination.

        Does not copy, and returns false if:

        - Source and destination rectangles do not intersect.
        - :py:class:`Pixmap` pixels could not be allocated.
        - dst.rowBytes() is too small to contain one row of pixels.

        :dst: storage for pixels copied from :py:class:`Surface`
        :srcX: offset into readable pixels on x-axis; may be negative
        :srcY: offset into readable pixels on y-axis; may be negative
        :return: true if pixels were copied
        )docstring",
        py::arg("dst"), py::arg("srcX") = 0, py::arg("srcY") = 0)
    .def("readPixels", &ReadPixels<SkSurface>,
        R"docstring(
        Copies :py:class:`Rect` of pixels from :py:class:`Canvas` into array.

        :py:class:`Matrix` and clip are ignored.

        Source :py:class:`Rect` corners are (srcX, srcY) and :py:class:`Surface`
        (width(), height()). Destination :py:class:`Rect` corners are (0, 0) and
        (array.shape[1], array.shape[0]). Copies each readable pixel
        intersecting both rectangles, without scaling, converting to
        :py:attr:`ColorType.kN32` and :py:attr:`AlphaType.kPremul` if required.

        Pixels are readable when :py:class:`BaseDevice` is raster, or backed by
        a GPU.

        The destination pixel storage must be allocated by the caller.

        Pixel values are converted only if :py:class:`ColorType` and
        :py:class:`AlphaType` do not match. Only pixels within both source and
        destination rectangles are copied. array contents outside
        :py:class:`Rect` intersection are unchanged.

        Pass negative values for srcX or srcY to offset pixels across or down
        destination.

        Does not copy, and returns false if:

        - Source and destination rectangles do not intersect.
        - :py:class:`Surface` pixels could not be converted to
            :py:attr:`ColorType.kN32` or :py:attr:`AlphaType.kPremul`.

        :dstInfo: width, height, :py:class:`ColorType`, and
            :py:class:`AlphaType` of dstPixels
        :dstPixels: storage for pixels; dstInfo.height() times dstRowBytes, or
            larger
        :dstRowBytes: size of one destination row; dstInfo.width() times pixel
            size, or larger. Ignored when dstPixels has more than one-dimension.
        :srcX: offset into readable pixels on x-axis; may be negative
        :srcY: offset into readable pixels on y-axis; may be negative
        :return: true if pixels were copied
        )docstring",
        py::arg("dstInfo"), py::arg("dstPixels"), py::arg("dstRowBytes") = 0,
        py::arg("srcX") = 0, py::arg("srcY") = 0)
    .def("readPixels",
        py::overload_cast<const SkBitmap&, int, int>(&SkSurface::readPixels),
        R"docstring(
        Copies :py:class:`Rect` of pixels from :py:class:`Surface` into bitmap.

        Source :py:class:`Rect` corners are (srcX, srcY) and :py:class:`Surface`
        (width(), height()). Destination :py:class:`Rect` corners are (0, 0) and
        (bitmap.width(), bitmap.height()). Copies each readable pixel
        intersecting both rectangles, without scaling, converting to
        bitmap.colorType() and bitmap.alphaType() if required.

        Pixels are readable when :py:class:`Surface` is raster, or backed by a
        GPU.

        The destination pixel storage must be allocated by the caller.

        Pixel values are converted only if :py:class:`ColorType` and
        :py:class:`AlphaType` do not match. Only pixels within both source and
        destination rectangles are copied. dst contents outside :py:class:`Rect`
        intersection are unchanged.

        Pass negative values for srcX or srcY to offset pixels across or down
        destination.

        Does not copy, and returns false if:

        - Source and destination rectangles do not intersect.
        - :py:class:`Surface` pixels could not be converted to dst.colorType()
            or dst.alphaType().
        - dst pixels could not be allocated.
        - dst.rowBytes() is too small to contain one row of pixels.

        :dst: storage for pixels copied from :py:class:`Surface`
        :srcX: offset into readable pixels on x-axis; may be negative
        :srcY: offset into readable pixels on y-axis; may be negative
        :return: true if pixels were copied
        )docstring",
        py::arg("dst"), py::arg("srcX"), py::arg("srcY"))
    .def("asyncRescaleAndReadPixels",
        [] (SkSurface& surface, const SkImageInfo& info, const SkIRect& srcRect,
            SkSurface::RescaleGamma rescaleGamma,
            py::function callback) {
            surface.asyncRescaleAndReadPixels(
                info, srcRect, rescaleGamma, SkSurface::RescaleMode(),
                &PyReadPixelsCallback, callback.release().ptr());
        },
        R"docstring(
        Makes surface pixel data available to caller, possibly asynchronously.

        It can also rescale the surface pixels.

        Currently asynchronous reads are only supported on the GPU backend and
        only when the underlying 3D API supports transfer buffers and CPU/GPU
        synchronization primitives. In all other cases this operates
        synchronously.

        Data is read from the source sub-rectangle, is optionally converted to a
        linear gamma, is rescaled to the size indicated by 'info', is then
        converted to the color space, color type, and alpha type of 'info'. A
        'srcRect' that is not contained by the bounds of the surface causes
        failure.

        When the pixel data is ready the caller's callback function is called
        with a AsyncReadResult containing pixel data in the requested color
        type, alpha type, and color space. The AsyncReadResult will have
        count() == 1. Upon failure the callback is called with nullptr for
        AsyncReadResult. For a GPU surface this flushes work but a submit must
        occur to guarantee a finite time before the callback is called.

        The data is valid for the lifetime of AsyncReadResult with the exception
        that if the :py:class:`Surface` is GPU-backed the data is immediately
        invalidated if the GrContext is abandoned or destroyed.

        :param info:    info of the requested pixels
        :param srcRect: subrectangle of surface to read
        :param rescaleGamma:    controls whether rescaling is done in the
            surface's gamma or whether the source data is transformed to a
            linear gamma before rescaling.
        :param rescaleQuality:  controls the quality (and cost) of the rescaling
        :param callback: function to call with result of the read.  The callback
            takes one argument of :py:class:`Surface.AsyncReadResult`
        )docstring",
        py::arg("info"), py::arg("srcRect"), py::arg("rescaleGamma"),
        py::arg("callback"))
/*
    .def("asyncRescaleAndReadPixelsYUV420",
        [] (SkSurface& surface, SkYUVColorSpace yuvColorSpace,
            const SkColorSpace* dstColorSpace, const SkIRect& srcRect,
            const SkISize& dstSize, SkSurface::RescaleGamma rescaleGamma,
            SkFilterQuality rescaleQuality, py::function callback) {
            surface.asyncRescaleAndReadPixelsYUV420(
                yuvColorSpace, CloneColorSpace(dstColorSpace), srcRect, dstSize,
                rescaleGamma, rescaleQuality, &PyReadPixelsCallback,
                callback.release().ptr());
        },
        R"docstring(
        Similar to :py:meth:`asyncRescaleAndReadPixels` but performs an
        additional conversion to YUV.

        The RGB->YUV conversion is controlled by 'yuvColorSpace'. The YUV data
        is returned as three planes ordered y, u, v. The u and v planes are half
        the width and height of the resized rectangle. The y, u, and v values
        are single bytes. Currently this fails if 'dstSize' width and height are
        not even. A 'srcRect' that is not contained by the bounds of the surface
        causes failure.

        When the pixel data is ready the caller's callback function is called
        with a AsyncReadResult containing the planar data. The AsyncReadResult
        will have count() == 3. Upon failure the callback is called with nullptr
        for AsyncReadResult. For a GPU surface this flushes work but a submit
        must occur to guarantee a finite time before the callback is called.

        The data is valid for the lifetime of AsyncReadResult with the exception
        that if the :py:class:`Surface` is GPU-backed the data is immediately
        invalidated if the GrContext is abandoned or destroyed.

        :param yuvColorSpace:   The transformation from RGB to YUV. Applied to
            the resized image after it is converted to dstColorSpace.
        :param dstColorSpace:   The color space to convert the resized image to,
            after rescaling.
        :param srcRect: The portion of the surface to rescale and convert to YUV
            planes.
        :param dstSize: The size to rescale srcRect to
        :param rescaleGamma:    controls whether rescaling is done in the
            surface's gamma or whether the source data is transformed to a
            linear gamma before rescaling.
        :param rescaleQuality:  controls the quality (and cost) of the rescaling
        :param callback:    function to call with the planar read result
        )docstring",
        py::arg("yuvColorSpace"), py::arg("dstColorSpace"), py::arg("srcRect"),
        py::arg("dstSize"), py::arg("rescaleGamma"), py::arg("rescaleQuality"),
        py::arg("callback"))
*/
    .def("writePixels",
        py::overload_cast<const SkPixmap&, int, int>(&SkSurface::writePixels),
        R"docstring(
        Copies :py:class:`Rect` of pixels from the src :py:class:`Pixmap` to the
        :py:class:`Surface`.

        Source :py:class:`Rect` corners are (0, 0) and (src.width(),
        src.height()). Destination :py:class:`Rect` corners are (dstX, dstY) and
        (dstX + Surface width(), dstY + Surface height()).

        Copies each readable pixel intersecting both rectangles, without
        scaling, converting to :py:class:`Surface` colorType() and
        :py:class:`Surface` alphaType() if required.

        :src: storage for pixels to copy to :py:class:`Surface`
        :dstX: x-axis position relative to :py:class:`Surface` to begin copy;
            may be negative
        :dstY: y-axis position relative to :py:class:`Surface` to begin copy;
            may be negative
        )docstring",
        py::arg("src"), py::arg("dstX") = 0, py::arg("dstY") = 0)
    .def("writePixels",
        py::overload_cast<const SkBitmap&, int, int>(&SkSurface::writePixels),
        R"docstring(
        Copies :py:class:`Rect` of pixels from the src :py:class:`Bitmap` to the
        :py:class:`Surface`.

        Source :py:class:`Rect` corners are (0, 0) and (src.width(),
        src.height()). Destination :py:class:`Rect` corners are (dstX, dstY) and
        (dstX + Surface width(), dstY + Surface height()).

        Copies each readable pixel intersecting both rectangles, without
        scaling, converting to :py:class:`Surface` colorType() and
        :py:class:`Surface` alphaType() if required.

        :src: storage for pixels to copy to :py:class:`Surface`
        :dstX: x-axis position relative to :py:class:`Surface` to begin copy;
            may be negative
        :dstY: y-axis position relative to :py:class:`Surface` to begin copy;
            may be negative
        )docstring",
        py::arg("src"), py::arg("dstX") = 0, py::arg("dstY") = 0)
    .def("props", &SkSurface::props,
        R"docstring(
        Returns :py:class:`SurfaceProps` for surface.

        :return: LCD striping orientation and setting for device independent
            fonts
        )docstring")
/* m117: Remove legacy SkImage and SkSurface methods */
    .def("flushAndSubmit",
        [] (SkSurface& surface, GrSyncCpu sync) {
            auto direct = GrAsDirectContext(surface.recordingContext());
            if (direct) {
                direct->flush(&surface, SkSurfaces::BackendSurfaceAccess::kNoAccess, GrFlushInfo());
                direct->submit(sync);
            }
        },
        R"docstring(
        Call to ensure all reads/writes of the surface have been issued to the
        underlying 3D API.

        Skia will correctly order its own draws and pixel operations. This must
        to be used to ensure correct ordering when the surface backing store is
        accessed outside Skia (e.g. direct use of the 3D API or a windowing
        system). :py:class:`GrContext` has additional flush and submit methods
        that apply to all surfaces and images created from a
        :py:class:`GrContext`. This is equivalent to calling :py:meth:`flush`
        with a default :py:class:`GrFlushInfo` followed by
        :py:meth:`GrContext.submit`.
        )docstring",
        py::arg_v("sync", GrSyncCpu::kNo, "skia.GrSyncCpu.kNo"))
    .def("flush",
        [] (SkSurface& surface, SkSurfaces::BackendSurfaceAccess access, const GrFlushInfo& info) {
            auto dContext = GrAsDirectContext(surface.recordingContext());
            if (!dContext) {
                return GrSemaphoresSubmitted::kNo;
            }
            return dContext->flush(&surface, access, info);
        },
        R"docstring(
        Issues pending :py:class:`Surface` commands to the GPU-backed API
        objects and resolves any :py:class:`Surface` MSAA. A call to
        :py:meth:`GrContext.submit` is always required to ensure work
        is actually sent to the gpu. Some specific API details:

        :GL: Commands are actually sent to the driver, but glFlush is never
            called. Thus some sync objects from the flush will not be valid
            until a submission occurs.

        :Vulkan/Metal/D3D/Dawn: Commands are recorded to the backend APIs
            corresponding command buffer or encoder objects. However, these
            objects are not sent to the gpu until a submission occurs.

        The work that is submitted to the GPU will be dependent on the
        :py:class:`BackendSurfaceAccess` that is passed in.

        If :py:attr:`BackendSurfaceAccess.kNoAccess` is passed in all commands
        will be issued to the GPU.

        If :py:attr:`BackendSurfaceAccess.kPresent` is passed in and the backend
        API is not Vulkan, it is treated the same as kNoAccess. If the backend
        API is Vulkan, the VkImage that backs the :py:class:`Surface` will be
        transferred back to its original queue. If the :py:class:`Surface` was
        created by wrapping a VkImage, the queue will be set to the queue which
        was originally passed in on the GrVkImageInfo. Additionally, if the
        original queue was not external or foreign the layout of the VkImage
        will be set to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.

        The :py:class:`GrFlushInfo` describes additional options to flush.
        Please see documentation at :py:class:`GrFlushInfo` for more info.

        If the return is :py:attr:`GrSemaphoresSubmitted.kYes`, only initialized
        :py:class:`GrBackendSemaphores` will be submitted to the gpu during the
        next submit call (it is possible Skia failed to create a subset of the
        semaphores). The client should not wait on these semaphores until after
        submit has been called, but must keep them alive until then. If a submit
        flag was passed in with the flush these valid semaphores can we waited
        on immediately. If this call returns
        :py:attr:`GrSemaphoresSubmitted.kNo`, the GPU backend will not submit
        any semaphores to be signaled on the GPU. Thus the client should not
        have the GPU wait on any of the semaphores passed in with the
        :py:class:`GrFlushInfo`. Regardless of whether semaphores were submitted
        to the GPU or not, the client is still responsible for deleting any
        initialized semaphores. Regardleess of semaphore submission the context
        will still be flushed. It should be emphasized that a return value of
        :py:attr:`GrSemaphoresSubmitted.kNo` does not mean the flush did not
        happen. It simply means there were no semaphores submitted to the GPU. A
        caller should only take this as a failure if they passed in semaphores
        to be submitted.

        Pending surface commands are flushed regardless of the return result.

        :param access:  type of access the call will do on the backend object
            after flush
        :param info:    flush options
        )docstring",
        py::arg("access"), py::arg("info"))
    .def("flush",
        [] (SkSurface& surface, const GrFlushInfo& info,
            const skgpu::MutableTextureState* newState) {
            auto dContext = GrAsDirectContext(surface.recordingContext());
            if (!dContext) {
                return GrSemaphoresSubmitted::kNo;
            }
            return dContext->flush(&surface, info, newState);
        },
        R"docstring(
        Issues pending :py:class:`Surface` commands to the GPU-backed API
        objects and resolves any :py:class:`Surface` MSAA.

        A call to :py:meth:`GrContext.submit` is always required to ensure work
        is ctually sent to the gpu. Some specific API details:

        :GL: Commands are actually sent to the driver, but glFlush is never
            called. Thus some sync objects from the flush will not be valid
            until a submission occurs.

        :Vulkan/Metal/D3D/Dawn: Commands are recorded to the backend APIs
            corresponding command buffer or encoder objects. However, these
            objects are not sent to the gpu until a submission occurs.

        The GrFlushInfo describes additional options to flush. Please see
        documentation at GrFlushInfo for more info.

        If a GrBackendSurfaceMutableState is passed in, at the end of the flush
        we will transition the surface to be in the state requested by the
        GrBackendSurfaceMutableState. If the surface (or :py:class:`Image` or
        GrBackendSurface wrapping the same backend object) is used again after
        this flush the state may be changed and no longer match what is
        requested here. This is often used if the surface will be used for
        presenting or external use and the client wants backend object to be
        prepped for that use. A finishedProc or semaphore on the GrFlushInfo
        will also include the work for any requested state change.

        If the return is GrSemaphoresSubmitted::kYes, only initialized
        GrBackendSemaphores will be submitted to the gpu during the next submit
        call (it is possible Skia failed to create a subset of the semaphores).
        The client should not wait on these semaphores until after submit has
        been called, but must keep them alive until then. If a submit flag was
        passed in with the flush these valid semaphores can we waited on
        immediately. If this call returns GrSemaphoresSubmitted::kNo, the GPU
        backend will not submit any semaphores to be signaled on the GPU. Thus
        the client should not have the GPU wait on any of the semaphores passed
        in with the GrFlushInfo. Regardless of whether semaphores were submitted
        to the GPU or not, the client is still responsible for deleting any
        initialized semaphores. Regardleess of semaphore submission the context
        will still be flushed. It should be emphasized that a return value of
        GrSemaphoresSubmitted::kNo does not mean the flush did not happen. It
        simply means there were no semaphores submitted to the GPU. A caller
        should only take this as a failure if they passed in semaphores to be
        submitted.

        Pending surface commands are flushed regardless of the return result.

        :param info: flush options
        :param newState: optional state change request after flush
        )docstring",
        py::arg("info"), py::arg("newState") = nullptr)
    .def("characterize", &SkSurface::characterize,
        R"docstring(
        Initializes :py:class:`SurfaceCharacterization` that can be used to
        perform GPU back-end processing in a separate thread.

        Typically this is used to divide drawing into multiple tiles.
        :py:class:`DeferredDisplayListRecorder` records the drawing commands for
        each tile.

        Return true if :py:class:`Surface` supports characterization. raster
        surface returns false.

        :param skia.SurfaceCharacterization characterization: properties for
            parallel drawing
        :return: true if supported
        )docstring",
        py::arg("characterization"))
/*
    .def("draw",
        py::overload_cast<sk_sp<const SkDeferredDisplayList>>(&SkSurface::draw),
        R"docstring(
        Draws deferred display list created using
        :py:class:`DeferredDisplayListRecorder`.

        Has no effect and returns false if :py:class:`SurfaceCharacterization`
        stored in deferredDisplayList is not compatible with
        :py:class:`Surface`.

        raster surface returns false.

        :deferredDisplayList: drawing commands
        :return: false if deferredDisplayList is not compatible
        )docstring",
        py::arg("deferredDisplayList"))
*/
    .def_static("MakeRasterDirect",
        [](const SkImageInfo& imageInfo, py::buffer b, size_t rowBytes,
            const SkSurfaceProps* surfaceProps) {
            py::buffer_info info = b.request();
            rowBytes = ValidateBufferToImageInfo(imageInfo, info, rowBytes);
            return SkSurfaces::WrapPixels(
                imageInfo, info.ptr, rowBytes, surfaceProps);
        },
        R"docstring(
        Allocates raster :py:class:`Surface`.

        :py:class:`Canvas` returned by :py:class:`Surface` draws directly into
        pixels.

        :py:class:`Surface` is returned if all parameters are valid. Valid
        parameters include: info dimensions are greater than zero; info contains
        :py:class:`ColorType` and :py:class:`AlphaType` supported by raster
        surface; pixels is not nullptr; rowBytes is large enough to contain info
        width pixels of :py:class:`ColorType`.

        Pixel buffer size should be info height times computed rowBytes. Pixels
        are not initialized. To access pixels after drawing, peekPixels() or
        readPixels().

        :param skia.ImageInfo imageInfo: width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`, of raster surface;
            width and height must be greater than zero
        :param Union[bytes,bytearray,memoryview] pixels: pointer to destination
            pixels buffer
        :param rowBytes: interval from one :py:class:`Surface` row to the next
        :param surfaceProps: LCD striping orientation and setting for device
            independent fonts; may be nullptr

        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("info"), py::arg("pixels"), py::arg("rowBytes") = 0,
        py::arg("surfaceProps") = nullptr)
    // .def_static("MakeRasterDirectReleaseProc",
    //     &SkSurfaces::WrapPixels,
    //     "Allocates raster SkSurface.")
    .def_static("MakeRaster",
        py::overload_cast<const SkImageInfo&, size_t, const SkSurfaceProps*>(
            &SkSurfaces::Raster),
        R"docstring(
        Allocates raster :py:class:`Surface`.

        :py:class:`Canvas` returned by :py:class:`Surface` draws directly into
        pixels. Allocates and zeroes pixel memory. Pixel memory size is
        imageInfo.height() times rowBytes, or times imageInfo.minRowBytes() if
        rowBytes is zero. Pixel memory is deleted when :py:class:`Surface` is
        deleted.

        :py:class:`Surface` is returned if all parameters are valid. Valid
        parameters include: info dimensions are greater than zero; info contains
        :py:class:`ColorType` and :py:class:`AlphaType` supported by raster
        surface; rowBytes is large enough to contain info width pixels of
        :py:class:`ColorType`, or is zero.

        If rowBytes is zero, a suitable value will be chosen internally.

        :imageInfo: width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`, of raster surface;
            width and height must be greater than zero
        :rowBytes: interval from one :py:class:`Surface` row to the
            next; may be zero
        :skia.SurfaceProps surfaceProps: LCD striping orientation and
            setting for device independent fonts; may be nullptr
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("imageInfo"), py::arg("rowBytes") = 0,
        py::arg("surfaceProps") = nullptr)
    .def_static("MakeRasterN32Premul",
        [] (int width, int height, const SkSurfaceProps* surfaceProps) {
            return SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height), surfaceProps);
        },
        R"docstring(
        Allocates raster :py:class:`Surface`.

        :py:class:`Canvas` returned by :py:class:`Surface` draws directly into
        pixels. Allocates and zeroes pixel memory. Pixel memory size is height
        times width times four. Pixel memory is deleted when :py:class:`Surface`
        is deleted.

        Internally, sets :py:class:`ImageInfo` to width, height, native color
        type, and :py:attr:`AlphaType.kPremul`.

        Note that the :py:class:`ImageInfo` native color type might not be what
        you desire, if you intent to interact with other software expecting
        specific color type order. e.g. package `wx`'s `wx.Bitmap.FromBufferRGBA`.
        For that purpose, you need one of the other :py:class:`Surface`
        constructors which takes an explicit appropriate matching
        :py:class:`ImageInfo` input, or one that takes an explicit :py:class:`ColorType`
        input. You should check the output of `surface.imageInfo().colorType()`
        afterwards, in that usage.

        :py:class:`Surface` is returned if width and height are greater than
        zero.

        Use to create :py:class:`Surface` that matches :py:class:`PMColor`, the
        native pixel arrangement on the platform. :py:class:`Surface` drawn to
        output device skips converting its pixel format.

        :param int width: pixel column count; must be greater than zero
        :param int height: pixel row count; must be greater than zero
        :param skia.SurfaceProps surfaceProps: LCD striping orientation and
            setting for device independent fonts; may be nullptr
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("width"), py::arg("height"), py::arg("surfaceProps") = nullptr)
    .def_static("MakeFromBackendTexture",
        [] (GrRecordingContext* context, const GrBackendTexture& backendTexture,
            GrSurfaceOrigin origin, int sampleCnt, SkColorType colorType,
            sk_sp<SkColorSpace> colorSpace,
            const SkSurfaceProps* surfaceProps) {
            return SkSurfaces::WrapBackendTexture(
                context, backendTexture, origin, sampleCnt, colorType,
                colorSpace, surfaceProps, NULL, NULL);
        },
        R"docstring(
        Wraps a GPU-backed texture into :py:class:`Surface`. Caller must ensure
        backendRenderTarget is valid for the lifetime of returned
        :py:class:`Surface`. If sampleCnt greater than zero, creates an
        intermediate MSAA :py:class:`Surface` which is used for drawing
        backendTexture.

        :py:class:`Surface` is returned if all parameters are valid.
        backendTexture is valid if its pixel configuration agrees with
        colorSpace and context; for instance, if backendTexture has an sRGB
        configuration, then context must support sRGB, and colorSpace must be
        present. Further, backendTexture width and height must not exceed
        context capabilities, and the context must be able to support back-end
        textures.

        Upon success textureReleaseProc is called when it is safe to delete the
        texture in the backend API (accounting only for use of the texture by
        this surface). If :py:class:`Surface` creation fails textureReleaseProc
        is called before this function returns.

        If SK_SUPPORT_GPU is defined as zero, has no effect and returns nullptr.

        :context: GPU context
        :backendTexture:  texture residing on GPU
        :sampleCnt:  samples per pixel, or 0 to disable full scene anti-aliasing
        :colorSpace:  range of colors; may be nullptr
        :surfaceProps:    LCD striping orientation and setting for device
            independent fonts; may be nullptr
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("context"), py::arg("backendTexture"), py::arg("origin"),
        py::arg("sampleCnt"), py::arg("colorType"), py::arg("colorSpace"),
        py::arg("surfaceProps") = nullptr)
    .def_static("MakeFromBackendRenderTarget",
        [] (GrRecordingContext* context, const GrBackendRenderTarget& target,
            GrSurfaceOrigin origin, SkColorType colorType,
            sk_sp<SkColorSpace> colorSpace,
            const SkSurfaceProps* surfaceProps) {
            return SkSurfaces::WrapBackendRenderTarget(
                context, target, origin, colorType, colorSpace, surfaceProps, NULL, NULL);
        },
        R"docstring(
        Wraps a GPU-backed buffer into :py:class:`Surface`.

        Caller must ensure backendRenderTarget is valid for the lifetime of
        returned :py:class:`Surface`.

        :py:class:`Surface` is returned if all parameters are valid.
        backendRenderTarget is valid if its pixel configuration agrees with
        colorSpace and context; for instance, if backendRenderTarget has an sRGB
        configuration, then context must support sRGB, and colorSpace must be
        present. Further, backendRenderTarget width and height must not exceed
        context capabilities, and the context must be able to support back-end
        render targets.

        Upon success releaseProc is called when it is safe to delete the render
        target in the backend API (accounting only for use of the render target
        by this surface). If :py:class:`Surface` creation fails releaseProc is
        called before this function returns.

        If SK_SUPPORT_GPU is defined as zero, has no effect and returns nullptr.

        :param context: GPU context
        :param backendRenderTarget: GPU intermediate memory buffer
        :param colorSpace:  range of colors
        :param surfaceProps:    LCD striping orientation and setting for device
            independent fonts; may be nullptr
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("context"), py::arg("backendRenderTarget"), py::arg("origin"),
        py::arg("colorType"), py::arg("colorSpace"),
        py::arg("surfaceProps") = nullptr)
    .def_static("WrapBackendRenderTarget", &SkSurfaces::WrapBackendRenderTarget,
        R"docstring(
        Wraps a GPU-backed buffer into :py:class:`Surface`.

        Caller must ensure backendRenderTarget is valid for the lifetime of
        returned :py:class:`Surface`.

        :py:class:`Surface` is returned if all parameters are valid.
        backendRenderTarget is valid if its pixel configuration agrees with
        colorSpace and context; for instance, if backendRenderTarget has an sRGB
        configuration, then context must support sRGB, and colorSpace must be
        present. Further, backendRenderTarget width and height must not exceed
        context capabilities, and the context must be able to support back-end
        render targets.

        Upon success releaseProc is called when it is safe to delete the render
        target in the backend API (accounting only for use of the render target
        by this surface). If :py:class:`Surface` creation fails releaseProc is
        called before this function returns.

        If SK_SUPPORT_GPU is defined as zero, has no effect and returns nullptr.

        :param context: GPU context
        :param backendRenderTarget: GPU intermediate memory buffer
        :param colorSpace:  range of colors
        :param surfaceProps:    LCD striping orientation and setting for device
            independent fonts; may be nullptr
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("context"), py::arg("backendRenderTarget"), py::arg("origin"),
        py::arg("colorType"), py::arg("colorSpace"),
        py::arg("surfaceProps") = nullptr, py::arg("releaseProc") = nullptr, py::arg("releaseContext") = nullptr)
    .def_static("MakeRenderTarget",
        py::overload_cast<GrRecordingContext*, skgpu::Budgeted, const SkImageInfo&, int,
        GrSurfaceOrigin, const SkSurfaceProps*, bool, bool>(
            &SkSurfaces::RenderTarget),
        R"docstring(
        Returns :py:class:`Surface` on GPU indicated by context.

        Allocates memory for pixels, based on the width, height, and
        :py:class:`ColorType` in :py:class:`ImageInfo`. budgeted selects whether
        allocation for pixels is tracked by context. imageInfo describes the
        pixel format in :py:class:`ColorType`, and transparency in
        :py:class:`AlphaType`, and color matching in :py:class:`ColorSpace`.

        sampleCount requests the number of samples per pixel. Pass zero to
        disable multi-sample anti-aliasing. The request is rounded up to the
        next supported count, or rounded down if it is larger than the maximum
        supported count.

        surfaceOrigin pins either the top-left or the bottom-left corner to the
        origin.

        shouldCreateWithMips hints that :py:class:`Image` returned by
        makeImageSnapshot() is mip map.

        If SK_SUPPORT_GPU is defined as zero, has no effect and returns nullptr.

        :param skia.GrContext context: GPU context
        :param skia.ImageInfo imageInfo: width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`; width, or height, or
            both, may be zero
        :param int sampleCount: samples per pixel, or 0 to disable full scene
            anti-aliasing
        :param skia.GrSurfaceOrigin surfaceOrigin: kTopLeft or kBottomLeft
        :param skia.SurfaceProps surfaceProps: LCD striping orientation and
            setting for device independent fonts; may be nullptr
        :param shouldCreateWithMips: hint that :py:class:`Surface` will host mip
            map images
        :param isProtected: protected-ness
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("context"), py::arg("budgeted"), py::arg("imageInfo"),
        py::arg("sampleCount") = 0,
        py::arg_v("surfaceOrigin", GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,
                  "skia.GrSurfaceOrigin.kBottomLeft_GrSurfaceOrigin"),
        py::arg("surfaceProps") = nullptr,
        py::arg("shouldCreateWithMips") = false,
        py::arg("isProtected") = false)
    .def_static("MakeRenderTarget",
        py::overload_cast<GrRecordingContext*, skgpu::Budgeted, const SkImageInfo&, int,
            const SkSurfaceProps*>(&SkSurfaces::RenderTarget),
        R"docstring(
        Returns :py:class:`Surface` on GPU indicated by context.

        Allocates memory for pixels, based on the width, height, and
        :py:class:`ColorType` in :py:class:`ImageInfo`. budgeted selects whether
        allocation for pixels is tracked by context. imageInfo describes the
        pixel format in :py:class:`ColorType`, and transparency in
        :py:class:`AlphaType`, and color matching in :py:class:`ColorSpace`.

        sampleCount requests the number of samples per pixel. Pass zero to
        disable multi-sample anti-aliasing. The request is rounded up to the
        next supported count, or rounded down if it is larger than the maximum
        supported count.

        :py:class:`Surface` bottom-left corner is pinned to the origin.

        :context: GPU context
        :imageInfo:   width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`; width, or height, or
            both, may be zero
        :sampleCount: samples per pixel, or 0 to disable full scene
            anti-aliasing
        :surfaceProps:    LCD striping orientation and setting for device
            independent fonts; may be nullptr
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("context"), py::arg("budgeted"), py::arg("imageInfo"),
        py::arg("sampleCount"), py::arg("surfaceProps") = nullptr)
    .def_static("MakeRenderTarget",
        py::overload_cast<GrRecordingContext*, skgpu::Budgeted, const SkImageInfo&>(
            &SkSurfaces::RenderTarget),
        R"docstring(
        Returns :py:class:`Surface` on GPU indicated by context.

        Allocates memory for pixels, based on the width, height, and
        :py:class:`ColorType` in :py:class:`ImageInfo`. budgeted selects whether
        allocation for pixels is tracked by context. imageInfo describes the
        pixel format in :py:class:`ColorType`, and transparency in
        :py:class:`AlphaType`, and color matching in :py:class:`ColorSpace`.

        :py:class:`Surface` bottom-left corner is pinned to the origin.

        :context: GPU context
        :imageInfo:   width, height, :py:class:`ColorType`,
            :py:class:`AlphaType`, :py:class:`ColorSpace`; width, or height, or
            both, may be zero
        )docstring",
        py::arg("context"), py::arg("budgeted"), py::arg("imageInfo"))
    .def_static("MakeRenderTarget",
        py::overload_cast<GrRecordingContext*,
        const GrSurfaceCharacterization&, skgpu::Budgeted>(
            &SkSurfaces::RenderTarget),
        R"docstring(
        Returns :py:class:`Surface` on GPU indicated by context that is
        compatible with the provided characterization.

        budgeted selects whether allocation for pixels is tracked by context.

        :context: GPU context
        :characterization: description of the desired :py:class:`Surface`
        :return: :py:class:`Surface` if all parameters are valid; otherwise,
            nullptr
        )docstring",
        py::arg("context"), py::arg("characterization"), py::arg("budgeted"))
    .def_static("MakeNull", &SkSurfaces::Null,
        R"docstring(
        Returns :py:class:`Surface` without backing pixels.

        Drawing to :py:class:`Canvas` returned from :py:class:`Surface` has no
        effect. Calling makeImageSnapshot() on returned :py:class:`Surface`
        returns nullptr.

        :param int width: one or greater
        :param int height: one or greater

        :return: :py:class:`Surface` if width and height are positive;
            otherwise, nullptr
        )docstring",
        py::arg("width"), py::arg("height"))
    ;

// Surfaces is a namespace
m.attr("Surfaces") = m.attr("Surface");
m.attr("Surface").attr("Raster") = m.attr("Surface").attr("MakeRaster");
}

