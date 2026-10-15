#include "common.h"
#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TypefaceFontProvider.h"
#include <pybind11/stl.h>

using namespace skia::textlayout;

void initParagraph(py::module &m) {

py::class_<FontCollection, sk_sp<FontCollection>, SkRefCnt> font_collection(m, "textlayout_FontCollection");
py::class_<ParagraphBuilder> paragraph_builder(m, "textlayout_ParagraphBuilder");
py::class_<ParagraphStyle> paragraph_style(m, "textlayout_ParagraphStyle");
py::class_<StrutStyle> strut_style(m, "textlayout_StrutStyle");
py::class_<TextStyle> text_style(m, "textlayout_TextStyle");
py::class_<Paragraph> paragraph(m, "textlayout_Paragraph");
py::class_<TypefaceFontProvider, sk_sp<TypefaceFontProvider>, SkFontMgr> typeface_font_provider(m, "textlayout_TypefaceFontProvider");

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
}
