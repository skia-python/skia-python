#include "common.h"
#include <include/core/SkFontMetrics.h>
#include <include/ports/SkFontMgr_directory.h>
#include <include/ports/SkFontMgr_empty.h>
#include <include/ports/SkFontScanner_FreeType.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/iostream.h>
#ifdef __linux__
#include <freetype/freetype.h>
#include <freetype/ftcolor.h>
#include <src/ports/SkFontHost_FreeType_common.h>
#endif

#ifdef __APPLE__
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

#ifdef __linux__
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif

#ifdef _WIN32
#include "include/ports/SkTypeface_win.h"
#endif

#include <mutex>

namespace {

bool g_factory_called = false;

}  // namespace

static sk_sp<SkFontMgr> fontmgr_factory() {
#if defined(__APPLE__)
  return SkFontMgr_New_CoreText(nullptr);
#elif defined(__linux__)
  return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#elif defined(_WIN32)
  return SkFontMgr_New_DirectWrite();
#else
  return SkFontMgr_New_Custom_Empty(); /* last resort: SkFontMgr::RefEmpty(); */
#endif
}

sk_sp<SkFontMgr> SkFontMgr_RefDefault() {
  static std::once_flag flag;
  static sk_sp<SkFontMgr> mgr;
  std::call_once(flag, [] {
    mgr = fontmgr_factory();
    g_factory_called = true;
  });
  return mgr;
}

using Axis = SkFontParameters::Variation::Axis;
using Coordinate = SkFontArguments::VariationPosition::Coordinate;
PYBIND11_MAKE_OPAQUE(std::vector<Coordinate>);

namespace {

void SetVariationPositionCoordinates(
    SkFontArguments::VariationPosition& vp,
    const std::vector<Coordinate>& coords) {
    vp.coordinates = &coords[0];
    vp.coordinateCount = coords.size();
}


py::tuple SkFontStyleSet_getStyle(SkFontStyleSet* self, int index) {
    SkFontStyle style;
    SkString name;
    if (index < 0 || index >= self->count())
        throw py::index_error();
    self->getStyle(index, &style, &name);
    return py::make_tuple(style, py::str(name.c_str(), name.size()));
}

py::str SkFontMgr_getFamilyName(const SkFontMgr& fontmgr, int index) {
    SkString familyName;
    if (index < 0 || index >= fontmgr.countFamilies())
        throw py::index_error();
    fontmgr.getFamilyName(index, &familyName);
    return py::str(familyName.c_str(), familyName.size());
}

sk_sp<SkTypeface> SkTypeface_MakeFromName(
    py::object familyName,
    const SkFontStyle* fontStyle) {
    return SkFontMgr_RefDefault()->legacyMakeTypeface(
        (familyName.is_none()) ?
            nullptr : familyName.cast<std::string>().c_str(),
        (fontStyle) ? *fontStyle : SkFontStyle());
}

}  // namespace

namespace {

/* class OneFontStyleSet and OneFontMgr verbatim from
   skia's chrome/m128:example/external_client/src/shape_text.cpp
 */
class OneFontStyleSet : public SkFontStyleSet {
 public:
  explicit OneFontStyleSet(sk_sp<SkTypeface> face) : face_(face) {}

 protected:
  int count() override { return 1; }
  void getStyle(int, SkFontStyle* out_style, SkString*) override {
    *out_style = SkFontStyle();
  }
  sk_sp<SkTypeface> createTypeface(int index) override { return face_; }
  sk_sp<SkTypeface> matchStyle(const SkFontStyle&) override { return face_; }

 private:
  sk_sp<SkTypeface> face_;
};

class OneFontMgr : public SkFontMgr {
 public:
  explicit OneFontMgr(sk_sp<SkTypeface> face)
      : face_(face), style_set_(sk_make_sp<OneFontStyleSet>(face)) {}

 protected:
  int onCountFamilies() const override { return 1; }
  void onGetFamilyName(int index, SkString* familyName) const override {
    *familyName = SkString("the-only-font-I-have");
  }
  sk_sp<SkFontStyleSet> onCreateStyleSet(int index) const override {
    return style_set_;
  }
  sk_sp<SkFontStyleSet> onMatchFamily(const char[]) const override {
    return style_set_;
  }

  sk_sp<SkTypeface> onMatchFamilyStyle(const char[],
                                       const SkFontStyle&) const override {
    return face_;
  }
  sk_sp<SkTypeface> onMatchFamilyStyleCharacter(
      const char familyName[], const SkFontStyle& style, const char* bcp47[],
      int bcp47Count, SkUnichar character) const override {
    return face_;
  }
  sk_sp<SkTypeface> onLegacyMakeTypeface(const char[],
                                         SkFontStyle) const override {
    return face_;
  }

  sk_sp<SkTypeface> onMakeFromData(sk_sp<SkData>, int) const override {
    std::abort();
    return nullptr;
  }
  sk_sp<SkTypeface> onMakeFromStreamIndex(std::unique_ptr<SkStreamAsset>,
                                          int) const override {
    std::abort();
    return nullptr;
  }
  sk_sp<SkTypeface> onMakeFromStreamArgs(
      std::unique_ptr<SkStreamAsset>, const SkFontArguments&) const override {
    std::abort();
    return nullptr;
  }
  sk_sp<SkTypeface> onMakeFromFile(const char[], int) const override {
    std::abort();
    return nullptr;
  }

 private:
  sk_sp<SkTypeface> face_;
  sk_sp<SkFontStyleSet> style_set_;
};

/* Adapted from skia's chrome/m128:example/external_client/src/shape_text.cpp */

/* Forward declaration */
sk_sp<SkFontMgr> OneFontMgr_New_Custom_Empty(sk_sp<SkData> font_data);

sk_sp<SkFontMgr> OneFontMgr_New_Custom_Empty(char* argv1) {
  SkFILEStream input(argv1);
  if (!input.isValid()) {
    printf("Cannot open input file %s\n", argv1);
    return nullptr;
  }
  sk_sp<SkData> font_data = SkData::MakeFromStream(&input, input.getLength());
  return OneFontMgr_New_Custom_Empty(font_data);
}

sk_sp<SkFontMgr> OneFontMgr_New_Custom_Empty(sk_sp<SkData> font_data) {
  sk_sp<SkFontMgr> mgr = SkFontMgr_New_Custom_Empty();
  sk_sp<SkTypeface> face = mgr->makeFromData(font_data);
  if (!face) {
    printf("input font stream was not parsable by Freetype\n");
    return nullptr;
  }
  return sk_make_sp<OneFontMgr>(face);
}

}  // namespace

void initFont(py::module &m) {
// FontStyle
py::class_<SkFontStyle> fontstyle(m, "FontStyle");

py::enum_<SkFontStyle::Weight>(fontstyle, "Weight", py::arithmetic())
    .value("kInvisible_Weight", SkFontStyle::Weight::kInvisible_Weight)
    .value("kThin_Weight", SkFontStyle::Weight::kThin_Weight)
    .value("kExtraLight_Weight", SkFontStyle::Weight::kExtraLight_Weight)
    .value("kLight_Weight", SkFontStyle::Weight::kLight_Weight)
    .value("kNormal_Weight", SkFontStyle::Weight::kNormal_Weight)
    .value("kMedium_Weight", SkFontStyle::Weight::kMedium_Weight)
    .value("kSemiBold_Weight", SkFontStyle::Weight::kSemiBold_Weight)
    .value("kBold_Weight", SkFontStyle::Weight::kBold_Weight)
    .value("kExtraBold_Weight", SkFontStyle::Weight::kExtraBold_Weight)
    .value("kBlack_Weight", SkFontStyle::Weight::kBlack_Weight)
    .value("kExtraBlack_Weight", SkFontStyle::Weight::kExtraBlack_Weight)
    .export_values();

py::enum_<SkFontStyle::Width>(fontstyle, "Width", py::arithmetic())
    .value("kUltraCondensed_Width", SkFontStyle::Width::kUltraCondensed_Width)
    .value("kExtraCondensed_Width", SkFontStyle::Width::kExtraCondensed_Width)
    .value("kCondensed_Width", SkFontStyle::Width::kCondensed_Width)
    .value("kSemiCondensed_Width", SkFontStyle::Width::kSemiCondensed_Width)
    .value("kNormal_Width", SkFontStyle::Width::kNormal_Width)
    .value("kSemiExpanded_Width", SkFontStyle::Width::kSemiExpanded_Width)
    .value("kExpanded_Width", SkFontStyle::Width::kExpanded_Width)
    .value("kExtraExpanded_Width", SkFontStyle::Width::kExtraExpanded_Width)
    .value("kUltraExpanded_Width", SkFontStyle::Width::kUltraExpanded_Width)
    .export_values();

py::enum_<SkFontStyle::Slant>(fontstyle, "Slant")
    .value("kUpright_Slant", SkFontStyle::Slant::kUpright_Slant)
    .value("kItalic_Slant", SkFontStyle::Slant::kItalic_Slant)
    .value("kOblique_Slant", SkFontStyle::Slant::kOblique_Slant)
    .export_values();

fontstyle
    .def(py::init<int, int, SkFontStyle::Slant>(),
        py::arg("weight"), py::arg("width"), py::arg("slant"))
    .def(py::init<>())
    .def("__repr__",
        [] (const SkFontStyle& self) {
            return py::str("FontStyle({}, {}, {})").format(
                self.weight(), self.width(), self.slant());
        })
    .def("weight", &SkFontStyle::weight)
    .def("width", &SkFontStyle::width)
    .def("slant", &SkFontStyle::slant)
    .def_static("Normal", &SkFontStyle::Normal)
    .def_static("Bold", &SkFontStyle::Bold)
    .def_static("Italic", &SkFontStyle::Italic)
    .def_static("BoldItalic", &SkFontStyle::BoldItalic)
    ;

// Typeface
py::class_<SkFontParameters> fontparameters(m, "FontParameters");

py::class_<SkFontParameters::Variation> variation(
    fontparameters, "Variation",
    R"docstring(
    Parameters in a variation font axis.
    )docstring");

py::class_<Axis>(variation, "Axis")
    .def(py::init<>())
    .def("__repr__",
        [] (const Axis& self) {
            return py::str("Axis(tag={:x}, min={}, def={}, max={})").format(
                self.tag, self.min, self.def, self.max);
        })
    .def_readwrite("tag", &Axis::tag,
        R"docstring(
        Four character identifier of the font axis (weight, width, slant,
        italic...).
        )docstring")
    .def_readwrite("min", &Axis::min,
        R"docstring(
        Minimum value supported by this axis.
        )docstring")
    .def_readwrite("def", &Axis::def,
        R"docstring(
        Default value set by this axis.
        )docstring")
    .def_readwrite("max", &Axis::max,
        R"docstring(
        Maximum value supported by this axis. The maximum can equal the minimum.
        )docstring")
    .def("isHidden", &Axis::isHidden,
        R"docstring(
        Return whether this axis is recommended to be remain hidden in user
        interfaces.
        )docstring")
    .def("setHidden", &Axis::setHidden,
        R"docstring(
        Set this axis to be remain hidden in user interfaces.
        )docstring",
        py::arg("hidden"))
    ;

py::class_<SkFontArguments> fontarguments(m, "FontArguments", R"docstring(
    Represents a set of actual arguments for a font.
    )docstring");

py::class_<SkFontArguments::VariationPosition> variationposition(
    fontarguments, "VariationPosition",
    R"docstring(
    Container of coordinates.
    )docstring");

py::class_<Coordinate>(
    variationposition, "Coordinate")
    .def(py::init(
        [] (SkFourByteTag axis, float value) {
            return Coordinate({axis, value});
        }),
        py::arg("axis"), py::arg("value"))
    .def("__repr__",
        [] (const Coordinate& self) {
            return py::str("Coordinate(axis={:x}, value={})").format(
                self.axis, self.value);
        })
    .def_readwrite("axis", &Coordinate::axis)
    .def_readwrite("value", &Coordinate::value)
    ;

py::bind_vector<std::vector<Coordinate>>(variationposition, "Coordinates");

variationposition
    .def(py::init(
        [] (const std::vector<Coordinate>& coordinates) {
            SkFontArguments::VariationPosition vp;
            SetVariationPositionCoordinates(vp, coordinates);
            return vp;
        }),
        py::arg("coordinates"))
    .def_property("coordinates",
        [] (const SkFontArguments::VariationPosition& vp) {
            return std::vector<Coordinate>(
                vp.coordinates, vp.coordinates + vp.coordinateCount);
        },
        &SetVariationPositionCoordinates)
    .def_readonly("coordinateCount",
        &SkFontArguments::VariationPosition::coordinateCount)
    ;

fontarguments
    .def(py::init<>())
    .def("setCollectionIndex", &SkFontArguments::setCollectionIndex,
        R"docstring(
        Specify the index of the desired font.

        Font formats like ttc, dfont, cff, cid, pfr, t42, t1, and fon may
        actually be indexed collections of fonts.
        )docstring",
        py::arg("collectionIndex"))
    .def("setVariationDesignPosition",
        &SkFontArguments::setVariationDesignPosition,
        R"docstring(
        Specify a position in the variation design space.

        Any axis not specified will use the default value. Any specified axis
        not actually present in the font will be ignored.

        :param position: not copied. The value must remain valid for life of
            :py:class:`FontArguments`.
        )docstring",
        py::arg("position"))
    .def("getCollectionIndex", &SkFontArguments::getCollectionIndex)
    .def("getVariationDesignPosition",
        &SkFontArguments::getVariationDesignPosition)
    ;

py::class_<SkFontScanner> font_scanner(m, "FontScanner");

font_scanner
    .def(py::init(&SkFontScanner_Make_FreeType))
    .def("scanFile", &SkFontScanner::scanFile,
        py::arg("stream"), py::arg("numFaces"))
    .def("scanFace", &SkFontScanner::scanFace,
        py::arg("stream"), py::arg("faceIndex"), py::arg("numInstances"))
    .def("MakeFromData",
        [] (const SkFontScanner& self, sk_sp<SkData> data, const SkFontArguments& args) {
            std::unique_ptr<SkStreamAsset> stream(new SkMemoryStream(data));
            return self.MakeFromStream(std::move(stream), args);
        },
        py::arg("data"), py::arg("args"))
    ;

py::class_<SkTypeface, sk_sp<SkTypeface>, SkRefCnt> typeface(
    m, "Typeface", R"docstring(
    The :py:class:`Typeface` class specifies the typeface and intrinsic style of
    a font.

    This is used in the paint, along with optionally algorithmic settings like
    textSize, textSkewX, textScaleX, kFakeBoldText_Mask, to specify how text
    appears when drawn (and measured).

    Typeface objects are immutable, and so they can be shared between threads.


    Example::

        typeface = skia.Typeface()
        typeface = skia.Typeface('Arial')
        typeface = skia.Typeface('Arial', skia.FontStyle.Bold())

    )docstring");

py::enum_<SkTypeface::SerializeBehavior>(typeface, "SerializeBehavior",
    R"docstring(
    A typeface can serialize just a descriptor (names, etc.), or it can also
    include the actual font data (which can be large).

    This enum controls how serialize() decides what to serialize.
    )docstring")
    .value("kDoIncludeData", SkTypeface::SerializeBehavior::kDoIncludeData)
    .value("kDontIncludeData", SkTypeface::SerializeBehavior::kDontIncludeData)
    .value("kIncludeDataIfLocal",
        SkTypeface::SerializeBehavior::kIncludeDataIfLocal)
    .export_values();

typeface
    .def(py::init(
        [] (void) {
            auto warnings = pybind11::module::import("warnings");
            auto builtins = pybind11::module::import("builtins");
            warnings.attr("warn")(
                "\"Default typeface\" is deprecated upstream. Please specify name/file/style choices.",
                builtins.attr("DeprecationWarning"));
            return SkFontMgr_RefDefault()->legacyMakeTypeface("", SkFontStyle());
        }),
        R"docstring(
        Returns the default normal typeface.
        )docstring")
    .def(py::init(&SkTypeface_MakeFromName),
        R"docstring(
        Creates a new reference to the typeface that most closely matches the
        requested familyName and fontStyle.

        This method allows extended font face specifiers as in the
        :py:class:`FontStyle` type. Will never return null.

        :param str familyName: May be NULL. The name of the font family.
        :param skia.FontStyle fontStyle: The style of the typeface.
        :return: reference to the closest-matching typeface.
        )docstring",
        py::arg("familyName"), py::arg("fontStyle") = nullptr)
    .def("__repr__",
        [] (const SkTypeface& self) {
            SkString name;
            self.getFamilyName(&name);
            return py::str("Typeface('{}', {})").format(
                name.c_str(), self.fontStyle());
        })
    .def("fontStyle", &SkTypeface::fontStyle,
        R"docstring(
        Returns the typeface's intrinsic style attributes.
        )docstring")
    .def("isBold", &SkTypeface::isBold,
        R"docstring(
        Returns true if style() has the kBold bit set.
        )docstring")
    .def("isItalic", &SkTypeface::isItalic,
        R"docstring(
        Returns true if style() has the kItalic bit set.
        )docstring")
    .def("isFixedPitch", &SkTypeface::isFixedPitch,
        R"docstring(
        Returns true if the typeface claims to be fixed-pitch.
        )docstring")
    .def("getVariationDesignParameters",
        [] (const SkTypeface& typeface) {
            auto count = typeface.getVariationDesignParameters({nullptr, 0});
            if (count == -1)
                throw std::runtime_error("Failed to get; Likely no parameter");
            std::vector<Axis> params(count);
            if (count == 0)
                return params;
            auto actualCount = typeface.getVariationDesignParameters(
                {params.data(), params.size()});
            if (actualCount == -1)
                throw std::runtime_error("Failed to get");
            return params;
        },
        R"docstring(
        Returns the design variation parameters.

        :return: List of axes.
        :rtype: List[skia.FontParameters.Variation.Axis]
        )docstring")
    .def("getVariationDesignPosition",
        [] (const SkTypeface& typeface) {
            auto count = typeface.getVariationDesignPosition({nullptr, 0});
            if (count == -1)
                throw std::runtime_error("Failed to get; Likely no position");
            std::vector<Coordinate> coords(count);
            auto actualCount = typeface.getVariationDesignPosition(
                {coords.data(), coords.size()});
            if (actualCount == -1)
                throw std::runtime_error("Failed to get");
            return coords;
        },
        R"docstring(
        Returns the design variation coordinates.

        :param int coordinateCount: the maximum number of entries to get.
        :return: list of coordinates
        :rtype: List[skia.FontArguments.VariationPosition.Coordinate]
        )docstring")
    .def("uniqueID", &SkTypeface::uniqueID,
        R"docstring(
        Return a 32bit value for this typeface, unique for the underlying font
        data.

        Will never return 0.
        )docstring")
    .def("makeClone", &SkTypeface::makeClone,
        R"docstring(
        Return a new typeface based on this typeface but parameterized as
        specified in the :py:class:`FontArguments`.

        If the :py:class:`FontArguments` does not supply an argument for a
        parameter in the font then the value from this typeface will be used as
        the value for that argument. If the cloned typeface would be exaclty the
        same as this typeface then this typeface may be ref'ed and returned. May
        return nullptr on failure.
        )docstring",
        py::arg("fontArguments"))
    // .def("serialize", &SkTypeface::serialize)
    .def("serialize",
        py::overload_cast<SkTypeface::SerializeBehavior>(
            &SkTypeface::serialize, py::const_),
        R"docstring(
        Returns a unique signature to a stream, sufficient to reconstruct a
        typeface referencing the same font when Deserialize is called.
        )docstring",
        py::arg_v("behavior", SkTypeface::SerializeBehavior::kIncludeDataIfLocal,
                  "skia.Typeface.SerializeBehavior.kIncludeDataIfLocal"))
    .def("unicharsToGlyphs",
        [] (const SkTypeface& typeface, const std::vector<SkUnichar>& chars) {
            std::vector<SkGlyphID> glyphs(chars.size());
            typeface.unicharsToGlyphs({chars.data(), chars.size()}, {glyphs.data(), glyphs.size()});
            return glyphs;
        },
        R"docstring(
        Given an array of UTF32 character codes, return their corresponding
        glyph IDs.

        :param List[int] chars: the array of UTF32 chars.
        :return: the corresponding glyph IDs for each character.
        )docstring",
        py::arg("chars"))
    .def("unicharToGlyph", &SkTypeface::unicharToGlyph,
        R"docstring(
        Return the glyphID that corresponds to the specified unicode code-point
        (in UTF32 encoding).

        If the unichar is not supported, returns 0.

        This is a short-cut for calling :py:meth:`unicharsToGlyphs`.
        )docstring",
        py::arg("unichar"))
    .def("countGlyphs", &SkTypeface::countGlyphs,
        R"docstring(
        Return the number of glyphs in the typeface.
        )docstring")
    .def("countTables", &SkTypeface::countTables,
        R"docstring(
        Return the number of tables in the font.
        )docstring")
    .def("getTableTags",
        [] (const SkTypeface& typeface) {
            std::vector<SkFontTableTag> tags(typeface.countTables());
            size_t size = typeface.readTableTags({tags.data(), tags.size()});
            if (size < tags.size())
                throw std::runtime_error("Failed to get table tags.");
            return tags;
        },
        R"docstring(
        Returns the list of table tags in the font.
        )docstring")
    .def("getTableSize", &SkTypeface::getTableSize,
        R"docstring(
        Given a table tag, return the size of its contents, or 0 if not present.
        )docstring",
        py::arg("tag"))
    .def("getTableData",
        [] (const SkTypeface& typeface, SkFontTableTag tag) {
            size_t size = typeface.getTableSize(tag);
            std::vector<char> data(size);
            auto written = typeface.getTableData(tag, 0, data.size(), &data[0]);
            if (written == 0 && size > 0)
                throw std::runtime_error("Failed to get table data.");
            return py::bytes(&data[0], data.size());
        },
        R"docstring(
        Returns the contents of a table.

        Note that the contents of the table will be in their native endian order
        (which for most truetype tables is big endian). If the table tag is not
        found, or there is an error copying the data, then 0 is returned. If
        this happens, it is possible that some or all of the memory pointed to
        by data may have been written to, even though an error has occured.

        :param int tag: The table tag whose contents are to be copied.
        :return: table contents
        )docstring",
        py::arg("tag"))
    .def("copyTableData", &SkTypeface::copyTableData,
        R"docstring(
        Return an immutable copy of the requested font table, or nullptr if that
        table was not found.

        This can sometimes be faster than calling :py:meth:`getTableData` twice:
        once to find the length, and then again to copy the data.

        :param tag: The table tag whose contents are to be copied
        :return: an immutable copy of the table's data, or nullptr.
        )docstring",
        py::arg("tag"))
    .def("getUnitsPerEm", &SkTypeface::getUnitsPerEm,
        R"docstring(
        Return the units-per-em value for this typeface, or zero if there is an
        error.
        )docstring")
    .def("getKerningPairAdjustments",
        [] (const SkTypeface& typeface,
            const std::vector<SkGlyphID>& glyphs) -> py::object {
            std::vector<int32_t> adjustments(glyphs.size() - 1);
            auto result = typeface.getKerningPairAdjustments(
                {glyphs.data(), glyphs.size()}, {(glyphs.size() > 1) ? adjustments.data() : nullptr, adjustments.size()});
            if (!result) {
                // Kerning is not supported for this typeface.
                return py::none();
            }
            return py::cast(adjustments);
        },
        R"docstring(
        Given a run of glyphs, return the associated horizontal adjustments.

        Adjustments are in "design units", which are integers relative to the
        typeface's units per em (see getUnitsPerEm).

        Some typefaces are known to never support kerning. If the typeface does
        not support kerning, then this method will return None (no kerning) for
        all possible glyph runs.
        )docstring",
        py::arg("glyphs"))
    .def("getFamilyNames",
        [] (const SkTypeface& typeface) {
            SkTypeface::LocalizedString name;
            py::list results;
            auto it = typeface.createFamilyNameIterator();
            if (!it)
                throw std::runtime_error("Null pointer exception");
            while (it->next(&name)) {
                auto fString = py::str(
                    name.fString.c_str(), name.fString.size());
                auto fLanguage = py::str(
                    name.fLanguage.c_str(), name.fLanguage.size());
                results.append(py::make_tuple(fString, fLanguage));
            }
            it->unref();
            return results;
        },
        R"docstring(
        Returns a list of (family name, language) pairs specified by the font.
        )docstring")
    .def("getFamilyName",
        [] (const SkTypeface& typeface) {
            SkString name;
            typeface.getFamilyName(&name);
            return py::str(name.c_str(), name.size());
        },
        R"docstring(
        Return the family name for this typeface.

        It will always be returned encoded as UTF8, but the language of the name
        is whatever the host platform chooses.
        )docstring")
    .def("getPostScriptName",
        [] (const SkTypeface& typeface) {
            SkString name;
            typeface.getPostScriptName(&name);
            return py::str(name.c_str(), name.size());
        },
        R"docstring(
        Return the PostScript name for this typeface.
        Value may change based on variation parameters.
        Returns false if no PostScript name is available.
        )docstring")
    // .def("openStream", &SkTypeface::openStream,
    //     "Return a stream for the contents of the font data, or NULL on "
    //     "failure.")
    // .def("makeFontData", &SkTypeface::makeFontData,
    //     R"docstring(
    //     Return the font data, or nullptr on failure.
    //     )docstring")
    // .def("createScalerContext", &SkTypeface::createScalerContext,
    //     "Return a scalercontext for the given descriptor.")
    .def("getBounds", &SkTypeface::getBounds,
        R"docstring(
        Return a rectangle (scaled to 1-pt) that represents the union of the
        bounds of all of the glyphs, but each one positioned at (0,).

        This may be conservatively large, and will not take into account any
        hinting or other size-specific adjustments.
        )docstring")
    // .def("filterRec", &SkTypeface::filterRec)
    // .def("getFontDescriptor", &SkTypeface::getFontDescriptor)
/*
    .def_static("UniqueID", &SkTypeface::UniqueID,
        R"docstring(
        Return the uniqueID for the specified typeface.

        If the face is null, resolve it to the default font and return its
        uniqueID. Will never return 0.
        )docstring",
        py::arg("typeface"))
*/
    .def_static("Equal", &SkTypeface::Equal,
        R"docstring(
        Returns true if the two typefaces reference the same underlying font,
        handling either being null (treating null as the default font)
        )docstring",
        py::arg("self"), py::arg("other"))
    .def("__eq__", &SkTypeface::Equal, py::is_operator())
    .def_static("MakeDefault",
        [] (void) {
            auto warnings = pybind11::module::import("warnings");
            auto builtins = pybind11::module::import("builtins");
            warnings.attr("warn")(
                "\"Default typeface\" is deprecated upstream. Please specify name/file/style choices.",
                builtins.attr("DeprecationWarning"));
            return SkFontMgr_RefDefault()->legacyMakeTypeface("", SkFontStyle());
        },
        R"docstring(
        Returns the default normal typeface, which is never nullptr.
        )docstring")
    .def_static("MakeEmpty", &SkTypeface::MakeEmpty,
        R"docstring(
        Returns a non-null typeface which contains no glyphs.
        )docstring")
    .def_static("MakeFromName", &SkTypeface_MakeFromName,
        R"docstring(
        Creates a new reference to the typeface that most closely matches the
        requested familyName and fontStyle.

        This method allows extended font face specifiers as in the
        :py:class:`FontStyle` type. Will never return null.

        :param str familyName: May be NULL. The name of the font family.
        :param skia.FontStyle fontStyle: The style of the typeface.
        :return: reference to the closest-matching typeface.
        )docstring",
        py::arg("familyName"), py::arg("fontStyle") = nullptr)
    .def_static("MakeFromFile",
        [] (const std::string& path, int index) {
            return SkFontMgr_RefDefault()->makeFromFile(&path[0], index);
        },
        R"docstring(
        Return a new typeface given a file.

        If the file does not exist, or is not a valid font file, returns
        nullptr.
        )docstring",
        py::arg("path"), py::arg("index") = 0)
    // .def_static("MakeFromStream", &SkTypeface::MakeFromStream,
    //     "Return a new typeface given a stream.")
    .def_static("MakeFromData",
        [] (sk_sp<SkData> data, int index) {
            return SkFontMgr_RefDefault()->makeFromData(data, index);
        },
        R"docstring(
        Return a new typeface given a :py:class:`Data`.

        If the data is null, or is not a valid font file, returns nullptr.

        Use :py:meth:`MakeDeserialize` for the result of :py:meth:`serialize`.
        )docstring",
        py::arg("data"), py::arg("index") = 0)
    // .def_static("MakeFromFontData", &SkTypeface::MakeFromFontData,
    //     "Return a new typeface given font data and configuration.")
    .def_static("MakeDeserialize",
        [] (const sk_sp<SkData>& data, sk_sp<SkFontMgr> lastResortMgr) {
            SkMemoryStream stream(data);
            return SkTypeface::MakeDeserialize(&stream, lastResortMgr);
        },
        R"docstring(
        Given the data previously written by :py:meth:`serialize`, return a new
        instance of a typeface referring to the same font.

        If that font is not available, return nullptr.
        )docstring",
        py::arg("dats"), py::arg("lastResortMgr"))
    ;

// FontMgr
py::class_<SkFontStyleSet, sk_sp<SkFontStyleSet>, SkRefCnt>(m, "FontStyleSet")
    .def("__getitem__", &SkFontStyleSet_getStyle, py::arg("index"))
    .def("__len__", &SkFontStyleSet::count)
    .def("__repr__",
        [] (SkFontStyleSet& self) {
            return py::str("FontStyleSet({})").format(self.count());
        })
    .def("count", &SkFontStyleSet::count)
    .def("getStyle", &SkFontStyleSet_getStyle, py::arg("index"))
    .def("createTypeface", &SkFontStyleSet::createTypeface, py::arg("index"))
    .def("matchStyle", &SkFontStyleSet::matchStyle, py::arg("pattern"))
    .def_static("CreateEmpty", &SkFontStyleSet::CreateEmpty)
    ;

py::class_<SkFontMgr, sk_sp<SkFontMgr>, SkRefCnt>(m, "FontMgr",
    R"docstring(
    Font manager that knows system fonts.

    Example::

        fontmgr = skia.FontMgr()
        print(list(fontmgr))
        familyName = fontmgr[0]

        typeface = fontmgr.matchFamilyStyle(familyName, skia.FontStyle.Normal())
        assert typeface is not None

    )docstring")
    .def(py::init([] () { return SkFontMgr_RefDefault(); }))
    .def_static("New_Custom_Directory", &SkFontMgr_New_Custom_Directory,
        R"docstring(
        Create a custom font manager which scans a given directory for font files.
        This font manager uses FreeType for rendering.
        )docstring",
        py::arg("dir"))
    .def_static("New_Custom_Empty", &SkFontMgr_New_Custom_Empty)
    .def_static("New_Custom_Empty", py::overload_cast<char*>(&OneFontMgr_New_Custom_Empty),
        py::arg("filename"))
    .def_static("New_Custom_Empty", py::overload_cast<sk_sp<SkData>>(&OneFontMgr_New_Custom_Empty),
        py::arg("data"))
    .def("__getitem__", &SkFontMgr_getFamilyName, py::arg("index"))
    .def("__len__", &SkFontMgr::countFamilies)
    .def("countFamilies", &SkFontMgr::countFamilies)
    .def("getFamilyName", &SkFontMgr_getFamilyName, py::arg("index"))
    .def("createStyleSet",
        [] (const SkFontMgr& fontmgr, int index) {
            return sk_sp<SkFontStyleSet>(fontmgr.createStyleSet(index));
        },
        py::arg("index"))
    .def("matchFamily",
        [] (const SkFontMgr& fontmgr, const std::string* familyName) {
            return sk_sp<SkFontStyleSet>(
                fontmgr.matchFamily(
                    (familyName) ? familyName->c_str() : nullptr));
        },
        R"docstring(
        Never returns NULL; will return an empty set if the name is not found.

        Passing nullptr as the parameter will return the default system family.
        Note that most systems don't have a default system family, so passing
        nullptr will often result in the empty set.

        It is possible that this will return a style set not accessible from
        createStyleSet(int) due to hidden or auto-activated fonts.
        )docstring",
        py::arg("familyName"))
    .def("matchFamilyStyle",
        [] (const SkFontMgr& fontmgr, const py::object& familyName,
            const SkFontStyle& style) {
            return sk_sp<SkTypeface>(
                fontmgr.matchFamilyStyle(
                    (familyName.is_none()) ? nullptr :
                    familyName.cast<std::string>().c_str(), style));
        },
        R"docstring(
        Find the closest matching typeface to the specified familyName and style
        and return a ref to it.

        Will return nullptr if no 'good' match is found.

        Passing nullptr as the parameter for familyName will return the
        default system font.

        It is possible that this will return a style set not accessible from
        createStyleSet(int) or matchFamily(const char[]) due to hidden or
        auto-activated fonts.
        )docstring",
        py::arg("familyName"), py::arg("style"))
    .def("matchFamilyStyleCharacter",
        [] (const SkFontMgr& fontmgr, const std::string& familyName,
            const SkFontStyle& style, const std::vector<std::string>& bcp47,
            SkUnichar character) {
            std::vector<const char*> bcp47_(bcp47.size());
            std::transform(bcp47.begin(), bcp47.end(), bcp47_.begin(),
                [] (const std::string& c) { return c.c_str(); });
            return sk_sp<SkTypeface>(
                fontmgr.matchFamilyStyleCharacter(
                    familyName.c_str(), style, &bcp47_[0], bcp47_.size(),
                    character));
        },
        R"docstring(
        Use the system fallback to find a typeface for the given character.

        Note that bcp47 is a combination of ISO 639, 15924, and 3166-1 codes, so
        it is fine to just pass a ISO 639 here.

        Will return NULL if no family can be found for the character in the
        system fallback.

        Passing nullptr as the parameter for familyName will return the
        default system font.

        bcp47[0] is the least significant fallback, bcp47[bcp47Count-1] is the
        most significant. If no specified bcp47 codes match, any font with the
        requested character will be matched.
        )docstring",
        py::arg("familyName"), py::arg("style"), py::arg("bcp47"),
        py::arg("character"))
    .def("makeFromData", &SkFontMgr::makeFromData,
        R"docstring(
        Create a typeface for the specified data and TTC index (pass 0 for none)
        or NULL if the data is not recognized.
        )docstring",
        py::arg("data"), py::arg("ttcIndex") = 0)
    // .def("makeFromStream", &SkFontMgr::makeFromStream)
    // .def("makeFromStream", &SkFontMgr::makeFromStream)
    // .def("makeFromFontData", &SkFontMgr::makeFromFontData)
    .def("makeFromFile",
        [] (const SkFontMgr& fontmgr, const std::string& path, int ttcIndex) {
            return fontmgr.makeFromFile(path.c_str(), ttcIndex);
        },
        R"docstring(
        Create a typeface for the specified fileName and TTC index (pass 0 for
        none) or NULL if the file is not found, or its contents are not
        recognized.
        )docstring",
        py::arg("path"), py::arg("ttcIndex") = 0)
    .def("legacyMakeTypeface",
        [] (const SkFontMgr& fontmgr, const std::string& familyName,
            const SkFontStyle& style) {
            return fontmgr.legacyMakeTypeface(familyName.c_str(), style);
        },
        py::arg("familyName"), py::arg("style"))
    .def_static("RefDefault", &SkFontMgr_RefDefault,
        R"docstring(
        Return the default fontmgr.
        )docstring")
    ;

// This has the side-effect of letting "skia.FontMgr.OneFontMgr()" work.
m.attr("FontMgr").attr("OneFontMgr") = m.attr("FontMgr").attr("New_Custom_Empty");

// Font
py::enum_<SkFontHinting>(m, "FontHinting")
    .value("kNone", SkFontHinting::kNone)
    .value("kSlight", SkFontHinting::kSlight)
    .value("kNormal", SkFontHinting::kNormal)
    .value("kFull", SkFontHinting::kFull)
    .export_values();

py::enum_<SkTextEncoding>(m, "TextEncoding")
    .value("kUTF8", SkTextEncoding::kUTF8)
    .value("kUTF16", SkTextEncoding::kUTF16)
    .value("kUTF32", SkTextEncoding::kUTF32)
    .value("kGlyphID", SkTextEncoding::kGlyphID)
    .export_values();

py::class_<SkFont> font(m, "Font", R"docstring(
    :py:class:`Font` controls options applied when drawing and measuring text.
    )docstring");

py::enum_<SkFont::Edging>(font, "Edging", R"docstring(
    Whether edge pixels draw opaque or with partial transparency.
    )docstring")
    .value("kAlias", SkFont::Edging::kAlias)
    .value("kAntiAlias", SkFont::Edging::kAntiAlias)
    .value("kSubpixelAntiAlias", SkFont::Edging::kSubpixelAntiAlias)
    .export_values();

font
/*
    .def("__repr__",
        [] (const SkFont& self) {
            return py::str("Font({}, {}, {}, {})").format(
                self.getTypefaceOrDefault(), self.getSize(), self.getScaleX(),
                self.getSkewX());
        })
*/
    .def(py::init(
        [] (void) {
            auto warnings = pybind11::module::import("warnings");
            auto builtins = pybind11::module::import("builtins");
            warnings.attr("warn")(
                "\"Default font\" is deprecated upstream. Please specify name/file/style choices.",
                builtins.attr("DeprecationWarning"));
            return SkFont(SkFontMgr_RefDefault()->legacyMakeTypeface("", SkFontStyle()));
        }),
        R"docstring(
        Constructs :py:class:`Font` with default values.
        )docstring")
    .def(py::init(
        [] (py::object typeface, SkScalar size) {
            if (typeface.is_none()) {
                auto warnings = pybind11::module::import("warnings");
                auto builtins = pybind11::module::import("builtins");
                warnings.attr("warn")(
                    "\"Default font\" is deprecated upstream. Please specify name/file/style choices.",
                    builtins.attr("DeprecationWarning"));
                return SkFont(SkFontMgr_RefDefault()->legacyMakeTypeface("", SkFontStyle()), size);
            } else {
                return SkFont(typeface.cast<sk_sp<SkTypeface>>(), size);
            }
        }),
        R"docstring(
        Constructs :py:class:`Font` with default values with
        :py:class:`Typeface` and size in points.

        :typeface: font and style used to draw and measure
            text
        :size: typographic height of text
        )docstring",
        py::arg("typeface"), py::arg("size"))
    .def(py::init(
        [] (py::object typeface) {
            if (typeface.is_none()) {
                auto warnings = pybind11::module::import("warnings");
                auto builtins = pybind11::module::import("builtins");
                warnings.attr("warn")(
                    "\"Default font\" is deprecated upstream. Please specify name/file/style choices.",
                    builtins.attr("DeprecationWarning"));
                return SkFont(SkFontMgr_RefDefault()->legacyMakeTypeface("", SkFontStyle()));
            } else {
                return SkFont(typeface.cast<sk_sp<SkTypeface>>());
            }
        }),
        R"docstring(
        Constructs :py:class:`Font` with default values with
        :py:class:`Typeface`.

        :typeface: font and style used to draw and measure
            text
        )docstring",
        py::arg("typeface"))
    .def(py::init(
        [] (py::object typeface, SkScalar size, SkScalar scaleX, SkScalar skewX) {
            if (typeface.is_none()) {
                auto warnings = pybind11::module::import("warnings");
                auto builtins = pybind11::module::import("builtins");
                warnings.attr("warn")(
                    "\"Default font\" is deprecated upstream. Please specify name/file/style choices.",
                    builtins.attr("DeprecationWarning"));
                return SkFont(SkFontMgr_RefDefault()->legacyMakeTypeface("", SkFontStyle()),
                                      size, scaleX, skewX);
            } else {
                return SkFont(typeface.cast<sk_sp<SkTypeface>>(),
                                      size, scaleX, skewX);
            }
        }),
        R"docstring(
        Constructs :py:class:`Font` with default values with
        :py:class:`Typeface` and size in points, horizontal scale, and
        horizontal skew.

        Horizontal scale emulates condensed and expanded fonts. Horizontal skew
        emulates oblique fonts.

        :typeface: font and style used to draw and measure
            text
        :size: typographic height of text
        :scaleX: text horizontal scale
        :skewX: additional shear on x-axis relative to y-axis
        )docstring",
        py::arg("typeface"), py::arg("size"), py::arg("scaleX"),
        py::arg("skewX"))
#ifdef __linux___
    .def_static("COLRv1Bound",
        py::overload_cast<FT_Face, SkGlyphID, SkRect*>(&SkScalerContext_FreeType_Base::computeColrV1GlyphBoundingBox),
        R"docstring(
        )docstring",
        py::arg("face"), py::arg("id"), py::arg("bound"))
    .def_static("COLRV1DrawCanvas",
        py::overload_cast<SkCanvas*, FT_Face, uint16_t, FT_UShort, FT_Color_Root_Transform>(&SkScalerContext_FreeType_Base::skia_colrv1_start_glyph),
        R"docstring(
        )docstring",
        py::arg("canvas"), py::arg("face"), py::arg("glyphId"), py::arg("palette_index"), py::arg("rootTransform"))
#endif
    .def("__eq__", &SkFont::operator==,
        R"docstring(
        Compares :py:class:`Font` and font, and returns true if they are
        equivalent.

        May return false if :py:class:`Typeface` has identical contents but
        different pointers.
        )docstring",
        py::is_operator())
    .def("__ne__", &SkFont::operator!=,
        R"docstring(
        Compares :py:class:`Font` and font, and returns true if they are not
        equivalent.

        May return true if :py:class:`Typeface` has identical contents but
        different pointers.
        )docstring",
        py::is_operator())
    .def("isForceAutoHinting", &SkFont::isForceAutoHinting,
        R"docstring(
        If true, instructs the font manager to always hint glyphs.

        Returned value is only meaningful if platform uses FreeType as the font
        manager.

        :return: true if all glyphs are hinted
        )docstring")
    .def("isEmbeddedBitmaps", &SkFont::isEmbeddedBitmaps,
        R"docstring(
        Returns true if font engine may return glyphs from font bitmaps instead
        of from outlines.

        :return: true if glyphs may be font bitmaps
        )docstring")
    .def("isSubpixel", &SkFont::isSubpixel,
        R"docstring(
        Returns true if glyphs may be drawn at sub-pixel offsets.

        :return: true if glyphs may be drawn at sub-pixel offsets.
        )docstring")
    .def("isLinearMetrics", &SkFont::isLinearMetrics,
        R"docstring(
        Returns true if font and glyph metrics are requested to be linearly
        scalable.

        :return: true if font and glyph metrics are requested to be linearly
            scalable.
        )docstring")
    .def("isEmbolden", &SkFont::isEmbolden,
        R"docstring(
        Returns true if bold is approximated by increasing the stroke width when
        creating glyph bitmaps from outlines.

        :return: bold is approximated through stroke width
        )docstring")
    .def("isBaselineSnap", &SkFont::isBaselineSnap,
        R"docstring(
        Returns true if baselines will be snapped to pixel positions when the
        current transformation matrix is axis aligned.

        :return: baselines may be snapped to pixels
        )docstring")
    .def("setForceAutoHinting", &SkFont::setForceAutoHinting,
        R"docstring(
        Sets whether to always hint glyphs.

        If forceAutoHinting is set, instructs the font manager to always hint
        glyphs.

        Only affects platforms that use FreeType as the font manager.

        :param bool forceAutoHinting: setting to always hint glyphs
        )docstring",
        py::arg("forceAutoHinting"))
    .def("setEmbeddedBitmaps", &SkFont::setEmbeddedBitmaps,
        R"docstring(
        Requests, but does not require, to use bitmaps in fonts instead of
        outlines.

        :param bool embeddedBitmaps setting to use bitmaps in fonts
        )docstring",
        py::arg("embeddedBitmaps"))
    .def("setSubpixel", &SkFont::setSubpixel,
        R"docstring(
        Requests, but does not require, that glyphs respect sub-pixel
        positioning.

        :param bool subpixel: setting for sub-pixel positioning
        )docstring",
        py::arg("subpixel"))
    .def("setLinearMetrics", &SkFont::setLinearMetrics,
        R"docstring(
        Requests, but does not require, linearly scalable font and glyph
        metrics.

        For outline fonts 'true' means font and glyph metrics should ignore
        hinting and rounding. Note that some bitmap formats may not be able to
        scale linearly and will ignore this flag.

        :param bool linearMetrics: setting for linearly scalable font and glyph
            metrics.
        )docstring",
        py::arg("linearMetrics"))
    .def("setEmbolden", &SkFont::setEmbolden,
        R"docstring(
        Increases stroke width when creating glyph bitmaps to approximate a bold
        typeface.

        :param bool embolden: setting for bold approximation
        )docstring",
        py::arg("embolden"))
    .def("setBaselineSnap", &SkFont::setBaselineSnap,
        R"docstring(
        Requests that baselines be snapped to pixels when the current
        transformation matrix is axis aligned.

        :param bool baselineSnap: setting for baseline snapping to pixels
        )docstring",
        py::arg("baselineSnap"))
    .def("getEdging", &SkFont::getEdging,
        R"docstring(
        Whether edge pixels draw opaque or with partial transparency.
        )docstring")
    .def("setEdging", &SkFont::setEdging,
        R"docstring(
        Requests, but does not require, that edge pixels draw opaque or with
        partial transparency.
        )docstring",
        py::arg("edging"))
    .def("setHinting", &SkFont::setHinting,
        R"docstring(
        Sets level of glyph outline adjustment.

        Does not check for valid values of hintingLevel.
        )docstring",
        py::arg("hintingLevel"))
    .def("getHinting", &SkFont::getHinting,
        R"docstring(
        Returns level of glyph outline adjustment.
        )docstring")
    .def("makeWithSize", &SkFont::makeWithSize,
        R"docstring(
        Returns a font with the same attributes of this font, but with the
        specified size.

        Returns nullptr if size is less than zero, infinite, or NaN.

        :param float size: typographic height of text
        )docstring",
        py::arg("size"))
    .def("getTypeface", &SkFont::getTypeface,
        R"docstring(
        Returns :py:class:`Typeface` if set, or nullptr.

        :return: :py:class:`Typeface` if previously set, nullptr otherwise
        )docstring",
        py::return_value_policy::reference)
/*
    .def("getTypefaceOrDefault", &SkFont::getTypefaceOrDefault,
        R"docstring(
        Returns :py:class:`Typeface` if set, or the default typeface.

        :return: :py:class:`Typeface` if previously set or, a pointer to the
            default typeface if not previously set.
        )docstring",
        py::return_value_policy::reference)
*/
    .def("getSize", &SkFont::getSize,
        R"docstring(
        Returns text size in points.

        :return: typographic height of text
        )docstring")
    .def("getScaleX", &SkFont::getScaleX,
        R"docstring(
        Returns text scale on x-axis.

        Default value is 1.

        :return: text horizontal scale
        )docstring")
    .def("getSkewX", &SkFont::getSkewX,
        R"docstring(
        Returns text skew on x-axis.

        Default value is zero.

        :return: additional shear on x-axis relative to y-axis
        )docstring")
    .def("refTypeface", &SkFont::refTypeface,
        R"docstring(
        Increases :py:class:`Typeface` :py:class:`RefCnt` by one.

        :return: :py:class:`Typeface` if previously set, nullptr otherwise
        )docstring")
/*
    .def("refTypefaceOrDefault", &SkFont::refTypefaceOrDefault,
        R"docstring(
        Increases :py:class:`Typeface` :py:class:`RefCnt` by one.

        :return: :py:class:`Typeface` if previously set or, a pointer to the
            default typeface if not previously set.
        )docstring")
*/
    .def("setTypeface", &SkFont::setTypeface,
        R"docstring(
        Sets :py:class:`Typeface` to typeface, decreasing :py:class:`RefCnt` of
        the previous :py:class:`Typeface`.

        Pass nullptr to clear :py:class:`Typeface` and use the default typeface.
        Increments tf :py:class:`RefCnt` by one.

        :param typeface: font and style used to draw text
        )docstring",
        py::arg("typeface"))
    .def("setSize", &SkFont::setSize,
        R"docstring(
        Sets text size in points.

        Has no effect if textSize is not greater than or equal to zero.

        :param float textSize: typographic height of text
        )docstring",
        py::arg("textSize"))
    .def("setScaleX", &SkFont::setScaleX,
        R"docstring(
        Sets text scale on x-axis.

        Default value is 1.

        :param float scaleX: text horizontal scale
        )docstring",
        py::arg("scaleX"))
    .def("setSkewX", &SkFont::setSkewX,
        R"docstring(
        Sets text skew on x-axis.

        Default value is zero.

        :param float skewX: additional shear on x-axis relative to y-axis
        )docstring",
        py::arg("skewX"))
    .def("textToGlyphs",
        [] (const SkFont& font, const std::string& text,
            SkTextEncoding encoding) {
            int count = font.countText(text.data(), text.size(), encoding);
            std::vector<SkGlyphID> glyphs(count);
            font.textToGlyphs(
                text.data(), text.size(), encoding, {glyphs.data(), glyphs.size()});
            return glyphs;
        },
        R"docstring(
        Converts text into glyph indices.

        Returns the number of glyph indices represented by text.
        :py:class:`TextEncoding` specifies how text represents characters or
        glyphs.

        Does not check text for valid character codes or valid glyph indices.

        If encoding is :py:attr:`TextEncoding.kUTF8` and text contains an
        invalid UTF-8 sequence, empty array is returned.

        When encoding is :py:attr:`TextEncoding.kUTF8`,
        :py:attr:`TextEncoding.kUTF16`, or :py:attr:`TextEncoding.kUTF32`; then
        each Unicode codepoint is mapped to a single glyph. This function uses
        the default character-to-glyph mapping from the :py:class:`Typeface` and
        maps characters not found in the :py:class:`Typeface` to zero.

        :param str text: character storage encoded with :py:class:`TextEncoding`
        :param skia.TextEncoding encoding: text encoding
        :return: glyphs represented by text
        )docstring",
        py::arg("text"),
        py::arg_v("encoding", SkTextEncoding::kUTF8, "skia.TextEncoding.kUTF8"))
    .def("unicharToGlyph", &SkFont::unicharToGlyph,
        R"docstring(
        Returns glyph index for Unicode character.

        If the character is not supported by the :py:class:`Typeface`, returns
        0.

        :param int uni: Unicode character
        )docstring",
        py::arg("uni"))
    .def("unicharsToGlyphs",
        [] (const SkFont& font, const std::vector<SkUnichar>& uni) {
            std::vector<SkGlyphID> glyphs(uni.size());
            font.unicharsToGlyphs({uni.data(), uni.size()}, {glyphs.data(), glyphs.size()});
            return glyphs;
        },
        py::arg("uni"))
    .def("countText",
        [] (const SkFont& font, const std::string& text,
            SkTextEncoding encoding) {
            return font.countText(text.data(), text.size(), encoding);
        },
        R"docstring(
        Returns number of glyphs represented by text.

        If encoding is :py:attr:`TextEncoding.kUTF8`,
        :py:attr:`TextEncoding.kUTF16`, or :py:attr:`TextEncoding.kUTF32`; then
        each Unicode codepoint is mapped to a single glyph.

        :param str text: character storage encoded with :py:class:`TextEncoding`
        :param skia.TextEncoding encoding: text encoding
        )docstring",
        py::arg("text"),
        py::arg_v("encoding", SkTextEncoding::kUTF8, "skia.TextEncoding.kUTF8"))
    .def("measureText",
        [] (const SkFont& font, const std::string& text,
            SkTextEncoding encoding, SkRect* bounds, const SkPaint* paint) {
            return font.measureText(
                text.data(), text.size(), encoding, bounds, paint);
        },
        R"docstring(
        Returns the advance width of text.

        The advance is the normal distance to move before drawing additional
        text. Returns the bounding box of text if bounds is not nullptr. paint
        stroke width or :py:class:`PathEffect` may modify the advance with.

        :param str text: character storage encoded with :py:class:`TextEncoding`
        :param skia.TextEncoding encoding: text encoding
        :param skia.Rect bounds: returns bounding box relative to (0, 0) if not
            nullptr
        :param skia.Paint paint: optional; may be nullptr
        :return: the advance width of text
        )docstring",
        py::arg("text"),
        py::arg_v("encoding", SkTextEncoding::kUTF8, "skia.TextEncoding.kUTF8"),
        py::arg("bounds") = nullptr, py::arg("paint") = nullptr)
    .def("getWidths",
        [] (const SkFont& font, const std::vector<SkGlyphID>& glyphs) {
            std::vector<SkScalar> width(glyphs.size());
            font.getWidths({glyphs.data(), glyphs.size()}, {width.data(), width.size()});
            return width;
        },
        R"docstring(
        Retrieves the advance and bounds for each glyph in glyphs.

        :param List[int] glyphs: array of glyph indices to be measured
        :rtype: List[float]
        )docstring",
        py::arg("glyphs"))
    .def("getWidthsBounds",
        [] (const SkFont& font, const std::vector<SkGlyphID>& glyphs,
            const SkPaint* paint) {
            std::vector<SkScalar> width(glyphs.size());
            std::vector<SkRect> bounds(glyphs.size());
            font.getWidthsBounds(
                {glyphs.data(), glyphs.size()}, {width.data(), width.size()}, {bounds.data(), bounds.size()}, paint);
            return py::make_tuple(width, bounds);
        },
        R"docstring(
        Retrieves the advance and bounds for each glyph in glyphs.

        :param List[int] glyphs: array of glyph indices to be measured
        :param skia.Paint paint: optional, specifies stroking,
            :py:class:`PathEffect` and :py:class:`MaskFilter`
        :return: Widths and bounds array
        :rtype: Tuple[List[float],List[skia.Rect]]
        )docstring",
        py::arg("glyphs"), py::arg("paint") = nullptr)
    .def("getBounds",
        [] (const SkFont& font, const std::vector<SkGlyphID>& glyphs,
            const SkPaint* paint) {
            std::vector<SkRect> bounds(glyphs.size());
            font.getBounds({glyphs.data(), glyphs.size()}, {bounds.data(), bounds.size()}, paint);
            return bounds;
        },
        R"docstring(
        Retrieves the bounds for each glyph in glyphs.

        If paint is not nullptr, its stroking, :py:class:`PathEffect`, and
        :py:class:`MaskFilter` fields are respected.

        :param List[int] glyphs: array of glyph indices to be measured
        :param skia.Paint paint: optional, specifies stroking,
            :py:class:`PathEffect`, and :py:class:`MaskFilter`
        :return: bounds for each glyph relative to (0, 0)
        :rtype: List[skia.Rect]
        )docstring",
        py::arg("glyphs"), py::arg("paint") = nullptr)
    .def("getPos",
        [] (const SkFont& font, const std::vector<SkGlyphID>& glyphs,
            const SkPoint& origin) {
            std::vector<SkPoint> pos(glyphs.size());
            font.getPos({glyphs.data(), glyphs.size()}, {pos.data(), pos.size()}, origin);
            return pos;
        },
        R"docstring(
        Retrieves the positions for each glyph, beginning at the specified
        origin.

        :param List[int] glyphs: array of glyph indices to be positioned
        :param skia.Point origin: location of the first glyph. Defaults to
            (0, 0).
        :return: glyphs positions
        )docstring",
        py::arg("glyphs"),
        py::arg_v("origin", SkPoint::Make(0, 0), "skia.Point(0, 0)"))
    .def("getXPos",
        [] (const SkFont& font, const std::vector<SkGlyphID>& glyphs,
            const SkScalar& origin) {
            std::vector<SkScalar> xpos(glyphs.size());
            font.getXPos({glyphs.data(), glyphs.size()}, {xpos.data(), xpos.size()}, origin);
            return xpos;
        },
        R"docstring(
        Retrieves the x-positions for each glyph, beginning at the specified
        origin.

        :param List[int] glyphs: array of glyph indices to be positioned
        :param skia.Point origin: location of the first glyph. Defaults to 0.
        :return: glyphs x-positions
        )docstring",
        py::arg("glyphs"), py::arg("origin") = 0)
    .def("getPath",
        [] (const SkFont& font, SkGlyphID glyphID) -> py::object {
            SkPath path;
            if (font.getPath(glyphID, &path))
                return py::cast(path);
            return py::none();
        },
        R"docstring(
        Modifies path to be the outline of the glyph.

        If the glyph has an outline, modifies path to be the glyph's outline and
        returns true. The glyph outline may be empty. Degenerate contours in the
        glyph outline will be skipped. If glyph is described by a bitmap,
        returns false and ignores path parameter.

        :param int glyphID: index of glyph
        :return: true if glyphID is described by path
        )docstring",
        py::arg("glyphID"))
    .def("getPaths",
        [] (const SkFont& font,
            const std::vector<SkGlyphID>& glyphIDs) -> py::object {
            std::vector<SkPath> paths;
            paths.reserve(glyphIDs.size());
            font.getPaths(
                {glyphIDs.data(),
                glyphIDs.size()},
                [] (const SkPath* pathOrNull, const SkMatrix& mx, void* ctx) {
                    auto paths_ = static_cast<std::vector<SkPath>*>(ctx);
                    if (pathOrNull) {
                        SkPath path;
                        pathOrNull->transform(mx, &path);
                        paths_->push_back(path);
                    }
                },
                static_cast<void*>(&paths));
            if (paths.empty())
                return py::none();
            return py::cast(paths);
        },
        R"docstring(
        Returns path corresponding to glyph array.

        :param glyphIDs: array of glyph indices
        :return: list of :py:class:`Path`
        )docstring",
        py::arg("glyphIDs"))
    .def("getMetrics",
        [] (const SkFont& font) {
            SkFontMetrics metrics;
            font.getMetrics(&metrics);
            return metrics;
        },
        R"docstring(
        Returns :py:class:`FontMetrics` associated with :py:class:`Typeface`.

        Results are scaled by text size but does not take into account
        dimensions required by text scale, text skew, fake bold, style stroke,
        and :py:class:`PathEffect`.

        :rtype: :py:class:`FontMetrics`
        )docstring")
    .def("getSpacing", &SkFont::getSpacing,
        R"docstring(
        Returns the recommended spacing between lines: the sum of metrics
        descent, ascent, and leading.

        Result is scaled by text size but does not take into account dimensions
        required by stroking and :py:class:`PathEffect`. Returns the same result
        as :py:meth:`getMetrics`.

        :return: recommended spacing between lines
        )docstring")
    .def("dump",
        [] (const SkFont& font) {
            py::scoped_ostream_redirect stream;
            font.dump();
        },
        R"docstring(
        Dumps fields of the font to SkDebugf.

        May change its output over time, so clients should not rely on this for
        anything specific. Used to aid in debugging.
        )docstring")
    ;

py::class_<SkFontMetrics> fontmetrics(m, "FontMetrics", R"docstring(
    The metrics of an :py:class:`Font`.

    The metric values are consistent with the Skia y-down coordinate system.
    )docstring");

py::enum_<SkFontMetrics::FontMetricsFlags>(fontmetrics, "FontMetricsFlags",
    R"docstring(
    FontMetricsFlags indicate when certain metrics are valid; the underline or
    strikeout metrics may be valid and zero.

    Fonts with embedded bitmaps may not have valid underline or strikeout
    metrics.
    )docstring", py::arithmetic())
    .value("kUnderlineThicknessIsValid_Flag",
        SkFontMetrics::FontMetricsFlags::kUnderlineThicknessIsValid_Flag,
        "set if fUnderlineThickness is valid")
    .value("kUnderlinePositionIsValid_Flag",
        SkFontMetrics::FontMetricsFlags::kUnderlinePositionIsValid_Flag,
        "set if fUnderlinePosition is valid")
    .value("kStrikeoutThicknessIsValid_Flag",
        SkFontMetrics::FontMetricsFlags::kStrikeoutThicknessIsValid_Flag,
        "set if fStrikeoutThickness is valid")
    .value("kStrikeoutPositionIsValid_Flag",
        SkFontMetrics::FontMetricsFlags::kStrikeoutPositionIsValid_Flag,
        "set if fStrikeoutPosition is valid")
    .export_values();

fontmetrics
    .def(py::init<>())
    .def("__repr__",
        [] (const SkFontMetrics& self) {
            std::stringstream stream;
            stream << "FontMetrics("
                << "Flags=" << self.fFlags << ", "
                << "Top=" << self.fTop << ", "
                << "Ascent=" << self.fAscent << ", "
                << "Descent=" << self.fDescent << ", "
                << "Bottom=" << self.fBottom << ", "
                << "Leading=" << self.fLeading << ", "
                << "AvgCharWidth=" << self.fAvgCharWidth << ", "
                << "MaxCharWidth=" << self.fMaxCharWidth << ", "
                << "XMin=" << self.fXMin << ", "
                << "XMax=" << self.fXMax << ", "
                << "XHeight=" << self.fXHeight << ", "
                << "CapHeight=" << self.fCapHeight << ", "
                << "UnderlineThickness=" << self.fUnderlineThickness << ", "
                << "UnderlinePosition=" << self.fUnderlinePosition << ", "
                << "StrikeoutThickness=" << self.fStrikeoutThickness << ", "
                << "StrikeoutPosition=" << self.fStrikeoutPosition << ")";
            return stream.str();
        })
    .def("hasUnderlineThickness", &SkFontMetrics::hasUnderlineThickness,
        R"docstring(
        Returns true if :py:class:`FontMetrics` has a valid underline thickness,
        and sets thickness to that value.

        If the underline thickness is not valid, return false, and ignore
        thickness.

        :param thickness: storage for underline width
        :return: true if font specifies underline width
        )docstring",
        py::arg("thickness"))
    .def("hasUnderlinePosition", &SkFontMetrics::hasUnderlinePosition,
        R"docstring(
        Returns true if :py:class:`FontMetrics` has a valid underline position,
        and sets position to that value.

        If the underline position is not valid, return false, and ignore
        position.

        :param float position: storage for underline position
        :return: true if font specifies underline position
        )docstring",
        py::arg("position"))
    .def("hasStrikeoutThickness", &SkFontMetrics::hasStrikeoutThickness,
        R"docstring(
        Returns true if :py:class:`FontMetrics` has a valid strikeout thickness,
        and sets thickness to that value.

        If the underline thickness is not valid, return false, and ignore
        thickness.

        :param float thickness: storage for strikeout width
        :return: true if font specifies strikeout width
        )docstring",
        py::arg("thickness"))
    .def("hasStrikeoutPosition", &SkFontMetrics::hasStrikeoutPosition,
        R"docstring(
        Returns true if :py:class:`FontMetrics` has a valid strikeout position,
        and sets position to that value.

        If the underline position is not valid, return false, and ignore
        position.

        :param float position: storage for strikeout position
        :return: true if font specifies strikeout position
        )docstring",
        py::arg("position"))
    .def_readwrite("fFlags", &SkFontMetrics::fFlags,
        R"docstring(
        FontMetricsFlags indicating which metrics are valid.
        )docstring")
    .def_readwrite("fTop", &SkFontMetrics::fTop,
        R"docstring(
        greatest extent above origin of any glyph bounding box, typically
        negative; deprecated with variable fonts
        )docstring")
    .def_readwrite("fAscent", &SkFontMetrics::fAscent,
        R"docstring(
        distance to reserve above baseline, typically negative
        )docstring")
    .def_readwrite("fDescent", &SkFontMetrics::fDescent,
        R"docstring(
        distance to reserve below baseline, typically positive
        )docstring")
    .def_readwrite("fBottom", &SkFontMetrics::fBottom,
        R"docstring(
        greatest extent below origin of any glyph bounding box, typically
        positive; deprecated with variable fonts
        )docstring")
    .def_readwrite("fLeading", &SkFontMetrics::fLeading,
        R"docstring(
        distance to add between lines, typically positive or zero
        )docstring")
    .def_readwrite("fAvgCharWidth", &SkFontMetrics::fAvgCharWidth,
        R"docstring(
        average character width, zero if unknown
        )docstring")
    .def_readwrite("fMaxCharWidth", &SkFontMetrics::fMaxCharWidth,
        R"docstring(
        maximum character width, zero if unknown
        )docstring")
    .def_readwrite("fXMin", &SkFontMetrics::fXMin,
        R"docstring(
        greatest extent to left of origin of any glyph bounding box, typically
        negative; deprecated with variable fonts
        )docstring")
    .def_readwrite("fXMax", &SkFontMetrics::fXMax,
        R"docstring(
        greatest extent to right of origin of any glyph bounding box, typically
        positive; deprecated with variable fonts
        )docstring")
    .def_readwrite("fXHeight", &SkFontMetrics::fXHeight,
        R"docstring(
        height of lower-case 'x', zero if unknown, typically negative
        )docstring")
    .def_readwrite("fCapHeight", &SkFontMetrics::fCapHeight,
        R"docstring(
        height of an upper-case letter, zero if unknown, typically negative
        )docstring")
    .def_readwrite("fUnderlineThickness", &SkFontMetrics::fUnderlineThickness,
        R"docstring(
        underline thickness
        )docstring")
    .def_readwrite("fUnderlinePosition", &SkFontMetrics::fUnderlinePosition,
        R"docstring(
        distance from baseline to top of stroke, typically positive
        )docstring")
    .def_readwrite("fStrikeoutThickness", &SkFontMetrics::fStrikeoutThickness,
        R"docstring(
        strikeout thickness
        )docstring")
    .def_readwrite("fStrikeoutPosition", &SkFontMetrics::fStrikeoutPosition,
        R"docstring(
        distance from baseline to bottom of stroke, typically negative
        )docstring")
    ;
}
