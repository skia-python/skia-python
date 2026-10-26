#include "common.h"
#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TypefaceFontProvider.h"
#include <pybind11/stl.h>

using namespace skia::textlayout;

namespace {
// SkParagraph visitor arrays are borrowed, and extendedVisit frees some of them
// immediately after the callback. Copy them while the callback is executing so
// Python can retain the result, including its font, after relayout/destruction.
struct ParagraphVisitorInfo {
    SkFont font;
    SkPoint origin;
    SkScalar advanceX;
    int count;
    std::vector<SkGlyphID> glyphs;
    std::vector<SkPoint> positions;
    std::vector<uint32_t> utf8Starts;
    unsigned flags;

    // Snapshot the native visitor's font, positions and UTF-8 cluster offsets.
    explicit ParagraphVisitorInfo(const Paragraph::VisitorInfo& info)
        : font(info.font), origin(info.origin), advanceX(info.advanceX),
          count(info.count), flags(info.flags) {
        if (count > 0) {
            glyphs.assign(info.glyphs, info.glyphs + count);
            positions.assign(info.positions, info.positions + count);
        }
        if (info.utf8Starts && count > 0) {
            utf8Starts.assign(info.utf8Starts, info.utf8Starts + count);
        }
    }
};

struct ParagraphExtendedVisitorInfo {
    SkFont font;
    SkPoint origin;
    SkSize advance;
    int count;
    std::vector<SkGlyphID> glyphs;
    std::vector<SkPoint> positions;
    std::vector<SkRect> bounds;
    std::vector<uint32_t> utf8Starts;
    unsigned flags;

    // Snapshot native glyph ink bounds as well as all ordinary visitor data.
    explicit ParagraphExtendedVisitorInfo(const Paragraph::ExtendedVisitorInfo& info)
        : font(info.font), origin(info.origin), advance(info.advance),
          count(info.count), flags(info.flags) {
        if (count > 0) {
            glyphs.assign(info.glyphs, info.glyphs + count);
            positions.assign(info.positions, info.positions + count);
            bounds.assign(info.bounds, info.bounds + count);
        }
        if (info.utf8Starts && count > 0) {
            utf8Starts.assign(info.utf8Starts, info.utf8Starts + count);
        }
    }
};
}  // namespace

void initParagraph(py::module &m) {

py::class_<FontCollection, sk_sp<FontCollection>, SkRefCnt> font_collection(m, "textlayout_FontCollection");
py::class_<ParagraphBuilder> paragraph_builder(m, "textlayout_ParagraphBuilder");
py::class_<ParagraphStyle> paragraph_style(m, "textlayout_ParagraphStyle");
py::class_<StrutStyle> strut_style(m, "textlayout_StrutStyle");
py::class_<TextStyle> text_style(m, "textlayout_TextStyle");
py::class_<Paragraph> paragraph(m, "textlayout_Paragraph");
py::class_<TypefaceFontProvider, sk_sp<TypefaceFontProvider>, SkFontMgr> typeface_font_provider(m, "textlayout_TypefaceFontProvider");

py::enum_<RectHeightStyle>(m, "textlayout_RectHeightStyle")
    .value("kTight", RectHeightStyle::kTight)
    .value("kMax", RectHeightStyle::kMax)
    .value("kIncludeLineSpacingMiddle", RectHeightStyle::kIncludeLineSpacingMiddle)
    .value("kIncludeLineSpacingTop", RectHeightStyle::kIncludeLineSpacingTop)
    .value("kIncludeLineSpacingBottom", RectHeightStyle::kIncludeLineSpacingBottom)
    .value("kStrut", RectHeightStyle::kStrut);
py::enum_<RectWidthStyle>(m, "textlayout_RectWidthStyle")
    .value("kTight", RectWidthStyle::kTight)
    .value("kMax", RectWidthStyle::kMax);
py::enum_<TextDirection>(m, "textlayout_TextDirection")
    .value("kRtl", TextDirection::kRtl)
    .value("kLtr", TextDirection::kLtr);
py::class_<TextBox>(m, "textlayout_TextBox")
    .def_readonly("rect", &TextBox::rect)
    .def_readonly("direction", &TextBox::direction);
py::class_<TextRange>(m, "textlayout_TextRange")
    .def_readonly("start", &TextRange::start)
    .def_readonly("end", &TextRange::end);

// Numeric line metrics are value copies. The internal StyleMetrics map contains
// paragraph-owned TextStyle pointers and is intentionally not exposed here.
py::class_<LineMetrics>(m, "textlayout_LineMetrics")
    .def_readonly("fStartIndex", &LineMetrics::fStartIndex)
    .def_readonly("fEndIndex", &LineMetrics::fEndIndex)
    .def_readonly("fEndExcludingWhitespaces", &LineMetrics::fEndExcludingWhitespaces)
    .def_readonly("fEndIncludingNewline", &LineMetrics::fEndIncludingNewline)
    .def_readonly("fHardBreak", &LineMetrics::fHardBreak)
    .def_readonly("fAscent", &LineMetrics::fAscent)
    .def_readonly("fDescent", &LineMetrics::fDescent)
    .def_readonly("fUnscaledAscent", &LineMetrics::fUnscaledAscent)
    .def_readonly("fHeight", &LineMetrics::fHeight)
    .def_readonly("fWidth", &LineMetrics::fWidth)
    .def_readonly("fLeft", &LineMetrics::fLeft)
    .def_readonly("fBaseline", &LineMetrics::fBaseline)
    .def_readonly("fLineNumber", &LineMetrics::fLineNumber);
py::class_<Paragraph::FontInfo>(paragraph, "FontInfo")
    .def_readonly("fFont", &Paragraph::FontInfo::fFont)
    .def_readonly("fTextRange", &Paragraph::FontInfo::fTextRange);
py::class_<ParagraphVisitorInfo>(paragraph, "VisitorInfo")
    .def_readonly("font", &ParagraphVisitorInfo::font)
    .def_readonly("origin", &ParagraphVisitorInfo::origin)
    .def_readonly("advanceX", &ParagraphVisitorInfo::advanceX)
    .def_readonly("count", &ParagraphVisitorInfo::count)
    .def_readonly("glyphs", &ParagraphVisitorInfo::glyphs)
    .def_readonly("positions", &ParagraphVisitorInfo::positions)
    .def_readonly("utf8Starts", &ParagraphVisitorInfo::utf8Starts)
    .def_readonly("flags", &ParagraphVisitorInfo::flags);
py::class_<ParagraphExtendedVisitorInfo>(paragraph, "ExtendedVisitorInfo")
    .def_readonly("font", &ParagraphExtendedVisitorInfo::font)
    .def_readonly("origin", &ParagraphExtendedVisitorInfo::origin)
    .def_readonly("advance", &ParagraphExtendedVisitorInfo::advance)
    .def_readonly("count", &ParagraphExtendedVisitorInfo::count)
    .def_readonly("glyphs", &ParagraphExtendedVisitorInfo::glyphs)
    .def_readonly("positions", &ParagraphExtendedVisitorInfo::positions)
    .def_readonly("bounds", &ParagraphExtendedVisitorInfo::bounds)
    .def_readonly("utf8Starts", &ParagraphExtendedVisitorInfo::utf8Starts)
    .def_readonly("flags", &ParagraphExtendedVisitorInfo::flags);
paragraph.attr("kWhiteSpace_VisitorFlag") = py::int_(static_cast<unsigned>(Paragraph::kWhiteSpace_VisitorFlag));

py::enum_<TextAlign>(m, "textlayout_TextAlign", R"docstring(
    )docstring")
    .value("kLeft", TextAlign::kLeft)
    .value("kRight", TextAlign::kRight)
    .value("kCenter", TextAlign::kCenter)
    .value("kJustify", TextAlign::kJustify)
    .value("kStart", TextAlign::kStart)
    .value("kEnd", TextAlign::kEnd)
    .export_values();

py::enum_<TextDecoration>(m, "textlayout_TextDecoration", R"docstring(
    )docstring")
    .value("kNoDecoration", TextDecoration::kNoDecoration)
    .value("kUnderline", TextDecoration::kUnderline)
    .value("kOverline", TextDecoration::kOverline)
    .value("kLineThrough", TextDecoration::kLineThrough)
    .value("kUnderlineOverline", TextDecoration(TextDecoration::kUnderline | TextDecoration::kOverline))
    .value("kUnderlineLineThrough", TextDecoration(TextDecoration::kUnderline | TextDecoration::kLineThrough))
    .value("kOverlineLineThrough", TextDecoration(TextDecoration::kOverline | TextDecoration::kLineThrough))
    .value("kUnderlineOverlineLineThrough", TextDecoration(TextDecoration::kUnderline | TextDecoration::kOverline | TextDecoration::kLineThrough))
    .export_values();

py::enum_<TextDecorationStyle>(m, "textlayout_TextDecorationStyle", R"docstring(
    )docstring")
    .value("kSolid", TextDecorationStyle::kSolid)
    .value("kDouble", TextDecorationStyle::kDouble)
    .value("kDotted", TextDecorationStyle::kDotted)
    .value("kDashed", TextDecorationStyle::kDashed)
    .value("kWavy", TextDecorationStyle::kWavy)
    .export_values();

py::enum_<TextDecorationMode>(m, "textlayout_TextDecorationMode", R"docstring(
    )docstring")
    .value("kGaps", TextDecorationMode::kGaps)
    .value("kThrough", TextDecorationMode::kThrough)
    .export_values();

paragraph_builder
    .def(py::init(
        [] (const ParagraphStyle& style,
            sk_sp<FontCollection> fontCollection,
            sk_sp<SkUnicode> unicode) {
                return ParagraphBuilder::make(style, fontCollection, unicode);
        }),
        R"docstring(
        )docstring",
        py::arg("style"), py::arg("fontCollection"), py::arg("unicode"))
    .def_static("make",
        py::overload_cast<ParagraphStyle const&, sk_sp<FontCollection>, sk_sp<SkUnicode>>(&ParagraphBuilder::make),
        R"docstring(
        )docstring",
        py::arg("style"), py::arg("fontCollection"), py::arg("unicode"))
    .def("addText",
        py::overload_cast<const char*>(&ParagraphBuilder::addText),
        R"docstring(
        )docstring",
        py::arg("text"))
    .def("pop", &ParagraphBuilder::pop)
    .def("pushStyle",
        py::overload_cast<const TextStyle&>(&ParagraphBuilder::pushStyle),
        R"docstring(
        )docstring",
        py::arg("style"))
    .def("Build", &ParagraphBuilder::Build)
    ;

strut_style
    .def(py::init())
    .def("setStrutEnabled",
        py::overload_cast<const bool>(&StrutStyle::setStrutEnabled),
        R"docstring(
        )docstring",
        py::arg("strutenabled"))
    .def("setLeading",
        py::overload_cast<const SkScalar>(&StrutStyle::setLeading),
        R"docstring(
        )docstring",
        py::arg("leading"))
    .def("setHeight",
        py::overload_cast<const SkScalar>(&StrutStyle::setHeight),
        R"docstring(
        )docstring",
        py::arg("height"))
    .def("setHeightOverride",
        py::overload_cast<const bool>(&StrutStyle::setHeightOverride),
        R"docstring(
        )docstring",
        py::arg("heightoverride"))
    .def("setForceStrutHeight",
        py::overload_cast<const bool>(&StrutStyle::setForceStrutHeight),
        R"docstring(
        )docstring",
        py::arg("forcestrutheight"))
    .def("setFontFamilies",
        py::overload_cast<std::vector<SkString>>(&StrutStyle::setFontFamilies),
        R"docstring(
        )docstring",
        py::arg("families"))
    .def("setFontSize",
        py::overload_cast<SkScalar>(&StrutStyle::setFontSize),
        R"docstring(
        )docstring",
        py::arg("size"))
    .def("setFontStyle",
        py::overload_cast<SkFontStyle>(&StrutStyle::setFontStyle),
        R"docstring(
        )docstring",
        py::arg("fontstyle"))
    .def("setHalfLeading",
        py::overload_cast<const bool>(&StrutStyle::setHalfLeading),
        R"docstring(
        )docstring",
        py::arg("halfleading"))
    ;

paragraph_style
    .def(py::init())
    .def("setTextStyle",
        py::overload_cast<const TextStyle&>(&ParagraphStyle::setTextStyle),
        R"docstring(
        )docstring",
        py::arg("textstyle"))
    .def("setTextAlign",
        py::overload_cast<TextAlign>(&ParagraphStyle::setTextAlign),
        R"docstring(
        )docstring",
        py::arg("align"))
    .def("setStrutStyle",
        py::overload_cast<StrutStyle>(&ParagraphStyle::setStrutStyle),
        R"docstring(
        )docstring",
        py::arg("strutstyle"))
    ;

font_collection
    .def(py::init())
    .def("setAssetFontManager", &FontCollection::setAssetFontManager,
        "Set the asset font manager; retain the existing native fallback policy.",
        py::arg("fontManager"))
    .def("setDefaultFontManager",
        py::overload_cast<sk_sp<SkFontMgr>>(&FontCollection::setDefaultFontManager),
        R"docstring(
        )docstring",
        py::arg("fontManager"))
    .def("setDefaultFontManager",
        py::overload_cast<sk_sp<SkFontMgr>, const char[]>(&FontCollection::setDefaultFontManager),
        R"docstring(
        )docstring",
        py::arg("fontManager"), py::arg("defaultFamilyName"))
    .def("setDefaultFontManager",
        py::overload_cast<sk_sp<SkFontMgr>, const std::vector<SkString>&>(&FontCollection::setDefaultFontManager),
        R"docstring(
        )docstring",
        py::arg("fontManager"), py::arg("defaultFamilyNames"))
    ;

text_style
    .def(py::init())
    .def("cloneForPlaceholder", &TextStyle::cloneForPlaceholder)
    .def("setColor",
        py::overload_cast<SkColor>(&TextStyle::setColor),
        R"docstring(
        )docstring",
        py::arg("color"))
    .def("setForegroundColor",
        py::overload_cast<SkPaint>(&TextStyle::setForegroundColor),
        R"docstring(
        DEPRECATED: prefer `setForegroundPaint`
        )docstring",
        py::arg("paint"))
    .def("setForegroundPaint",
        py::overload_cast<SkPaint>(&TextStyle::setForegroundPaint),
        R"docstring(
        )docstring",
        py::arg("paint"))
    .def("setFontFamilies",
        py::overload_cast<std::vector<SkString>>(&TextStyle::setFontFamilies),
        R"docstring(
        )docstring",
        py::arg("families"))
    .def("setFontSize",
        py::overload_cast<SkScalar>(&TextStyle::setFontSize),
        R"docstring(
        )docstring",
        py::arg("size"))
    .def("setFontStyle",
        py::overload_cast<SkFontStyle>(&TextStyle::setFontStyle),
        R"docstring(
        )docstring",
        py::arg("fontStyle"))
    .def("setLocale",
        py::overload_cast<const SkString&>(&TextStyle::setLocale),
        R"docstring(
        )docstring",
        py::arg("locale"))
    .def("setLetterSpacing",
        py::overload_cast<SkScalar>(&TextStyle::setLetterSpacing),
        R"docstring(
        )docstring",
        py::arg("letterspacing"))
    .def("setWordSpacing",
        py::overload_cast<SkScalar>(&TextStyle::setWordSpacing),
        R"docstring(
        )docstring",
        py::arg("wordspacing"))
    .def("setDecoration",
        py::overload_cast<TextDecoration>(&TextStyle::setDecoration),
        R"docstring(
        )docstring",
        py::arg("decoration"))
    .def("setDecorationMode",
        py::overload_cast<TextDecorationMode>(&TextStyle::setDecorationMode),
        R"docstring(
        )docstring",
        py::arg("mode"))
    .def("setDecorationStyle",
        py::overload_cast<TextDecorationStyle>(&TextStyle::setDecorationStyle),
        R"docstring(
        )docstring",
        py::arg("style"))
    .def("setDecorationColor",
        py::overload_cast<SkColor>(&TextStyle::setDecorationColor),
        R"docstring(
        )docstring",
        py::arg("color"))
    .def("setDecorationThicknessMultiplier",
        py::overload_cast<SkScalar>(&TextStyle::setDecorationThicknessMultiplier),
        R"docstring(
        )docstring",
        py::arg("m"))
    .def("setHeight",
        py::overload_cast<SkScalar>(&TextStyle::setHeight),
        R"docstring(
        )docstring",
        py::arg("height"))
    .def("setHeightOverride",
        py::overload_cast<bool>(&TextStyle::setHeightOverride),
        R"docstring(
        )docstring",
        py::arg("heightoverride"))
    .def("setHalfLeading",
        py::overload_cast<bool>(&TextStyle::setHalfLeading),
        R"docstring(
        )docstring",
        py::arg("halfleading"))
    .def("setBaselineShift",
        py::overload_cast<SkScalar>(&TextStyle::setBaselineShift),
        R"docstring(
        )docstring",
        py::arg("baselineshift"))
    ;

paragraph
    .def_property_readonly("Width", &Paragraph::getMaxWidth)
    .def_property_readonly("Height", &Paragraph::getHeight)
    .def_property_readonly("MinIntrinsicWidth", &Paragraph::getMinIntrinsicWidth)
    .def_property_readonly("MaxIntrinsicWidth", &Paragraph::getMaxIntrinsicWidth)
    .def_property_readonly("AlphabeticBaseline", &Paragraph::getAlphabeticBaseline)
    .def_property_readonly("IdeographicBaseline", &Paragraph::getIdeographicBaseline)
    .def_property_readonly("LongestLine", &Paragraph::getLongestLine)
    .def_property_readonly("ExceedMaxLines", &Paragraph::didExceedMaxLines)
    .def("layout",
        py::overload_cast<SkScalar>(&Paragraph::layout),
        R"docstring(
        )docstring",
        py::arg("width"))
    .def("paint",
        py::overload_cast<SkCanvas*, SkScalar, SkScalar>(&Paragraph::paint),
        R"docstring(
        )docstring",
        py::arg("canvas"), py::arg("x"), py::arg("y"))
    .def("getRectsForRange", &Paragraph::getRectsForRange,
        "Return native text boxes for the half-open UTF-16 range [start, end).",
        py::arg("start"), py::arg("end"), py::arg("rectHeightStyle"), py::arg("rectWidthStyle"))
    .def("getLineMetrics", [](Paragraph& self) {
        std::vector<LineMetrics> metrics;
        self.getLineMetrics(metrics);
        return metrics;
    }, "Return copies of the numeric line metrics after layout.")
    .def("getLineMetricsAt", [](const Paragraph& self, int lineNumber) -> py::object {
        LineMetrics metrics;
        if (!self.getLineMetricsAt(lineNumber, &metrics)) {
            return py::none();
        }
        return py::cast(std::move(metrics));
    }, "Return a line's numeric metrics, or None for an invalid zero-based line.",
       py::arg("lineNumber"))
    .def("lineNumber", &Paragraph::lineNumber, "Return the number of laid-out lines.")
    .def("getActualTextRange", &Paragraph::getActualTextRange,
        "Return the native UTF-8 byte range; includeSpaces controls trailing whitespace.",
        py::arg("lineNumber"), py::arg("includeSpaces"))
    .def("unresolvedGlyphs", &Paragraph::unresolvedGlyphs,
        "Return the native unresolved glyph count (-1 when not yet shaped).")
    .def("unresolvedCodepoints", &Paragraph::unresolvedCodepoints,
        "Return the native set of unresolved Unicode code points.")
    .def("getFonts", &Paragraph::getFonts,
        "Return actual shaping fonts and their native UTF-8 text ranges as value copies.")
    .def("getFontAtUTF16Offset", &Paragraph::getFontAtUTF16Offset,
        "Return the actual shaping font at a UTF-16 offset.", py::arg("codeUnitIndex"))
    .def("visit", [](Paragraph& self, const py::function& visitor) {
        self.visit([&visitor](int lineNumber, const Paragraph::VisitorInfo* info) {
            visitor(lineNumber, info ? py::cast(ParagraphVisitorInfo(*info)) : py::none());
        });
    }, R"docstring(
        Call visitor(lineNumber, info) for every native glyph run.

        info is an owning VisitorInfo snapshot, or None at a native line-end
        callback. Arrays retain native order; utf8Starts contains the first
        count cluster offsets, without an inferred terminal offset. Python
        exceptions propagate to the caller.
    )docstring", py::arg("visitor"))
    .def("extendedVisit", [](Paragraph& self, const py::function& visitor) {
        self.extendedVisit([&visitor](int lineNumber, const Paragraph::ExtendedVisitorInfo* info) {
            visitor(lineNumber, info ? py::cast(ParagraphExtendedVisitorInfo(*info)) : py::none());
        });
    }, R"docstring(
        Call visitor(lineNumber, info) with native glyph positions and ink bounds.

        info is an owning ExtendedVisitorInfo snapshot, or None at a native
        line-end callback. Glyph bounds are not joined, shifted or estimated.
        The snapshot remains valid after callback return or paragraph relayout.
    )docstring", py::arg("visitor"))
    ;

typeface_font_provider
    .def(py::init())
    .def("registerTypeface",
        py::overload_cast<sk_sp<SkTypeface>>(&TypefaceFontProvider::registerTypeface),
        py::arg("typeface"))
    .def("registerTypeface",
        py::overload_cast<sk_sp<SkTypeface>, const SkString&>(&TypefaceFontProvider::registerTypeface),
        py::arg("typeface"), py::arg("alias"))
    ;

py::object SimpleNamespace = py::module_::import("types").attr("SimpleNamespace");
m.attr("textlayout") = SimpleNamespace();
m.attr("textlayout").attr("FontCollection") = m.attr("textlayout_FontCollection");
m.attr("textlayout").attr("ParagraphBuilder") = m.attr("textlayout_ParagraphBuilder");
m.attr("textlayout").attr("ParagraphStyle") = m.attr("textlayout_ParagraphStyle");
m.attr("textlayout").attr("Paragraph") = m.attr("textlayout_Paragraph");
m.attr("textlayout").attr("TypefaceFontProvider") = m.attr("textlayout_TypefaceFontProvider");
m.attr("textlayout").attr("StrutStyle") = m.attr("textlayout_StrutStyle");
m.attr("textlayout").attr("TextStyle") = m.attr("textlayout_TextStyle");
m.attr("textlayout").attr("TextAlign") = m.attr("textlayout_TextAlign");
m.attr("textlayout").attr("TextDecoration") = m.attr("textlayout_TextDecoration");
m.attr("textlayout").attr("TextDecorationStyle") = m.attr("textlayout_TextDecorationStyle");
m.attr("textlayout").attr("TextDecorationMode") = m.attr("textlayout_TextDecorationMode");
for (const char* name : {"RectHeightStyle", "RectWidthStyle", "TextDirection", "TextBox", "TextRange", "LineMetrics"}) {
    m.attr("textlayout").attr(name) = m.attr((std::string("textlayout_") + name).c_str());
}
}
