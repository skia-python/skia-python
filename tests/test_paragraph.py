import skia
import pytest
import operator


@pytest.fixture(scope='module')
def non_text_typeface():
    import sys
    import os
    if sys.platform.startswith("linux"):
        if os.path.exists("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf"): # Ubuntu CI
            # Ubuntu is weird - the font is on disk but not accessible to fontconfig
            # - Possibly https://bugs.launchpad.net/bugs/2054924
            return skia.Typeface.MakeFromFile("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf")
        elif os.path.exists("/usr/share/fonts/google-noto-color-emoji-fonts/NotoColorEmoji.ttf"): # Fedora
            return skia.Typeface.MakeFromFile("/usr/share/fonts/google-noto-color-emoji-fonts/NotoColorEmoji.ttf")
        else:
            pytest.skip("Not in Ubuntu CI")
    if sys.platform.startswith("darwin"):
        return skia.Typeface("Apple Color Emoji")
    if sys.platform.startswith("win"):
        return skia.Typeface("Segoe UI Emoji") # COLRv0


@pytest.fixture(scope='session')
def textlayout_font_collection():
    return skia.textlayout.FontCollection()

def test_FontCollection_init0(textlayout_font_collection):
    assert isinstance(textlayout_font_collection, skia.textlayout_FontCollection)


@pytest.fixture(scope='session')
def paragraph_style():
    return skia.textlayout.ParagraphStyle()

def test_ParagraphStyle_init0(paragraph_style):
    assert isinstance(paragraph_style, skia.textlayout_ParagraphStyle)


@pytest.fixture(scope='session')
def strut_style():
    return skia.textlayout.StrutStyle()

def test_StrutStyle_init0(strut_style):
    assert isinstance(strut_style, skia.textlayout_StrutStyle)


@pytest.fixture(scope='session')
def textlayout_text_style():
    return skia.textlayout.TextStyle()

def test_TextStyle_init0(textlayout_text_style):
    assert isinstance(textlayout_text_style, skia.textlayout_TextStyle)


@pytest.fixture(scope='session')
def paragraph_builder():
    return skia.textlayout.ParagraphBuilder.make(skia.textlayout.ParagraphStyle(),
                                                 skia.textlayout.FontCollection(),
                                                 skia.Unicodes.ICU.Make())

def test_ParagraphBuilder_init0(paragraph_builder):
    assert isinstance(paragraph_builder, skia.textlayout_ParagraphBuilder)

def test_Paragraph_init0(paragraph_builder):
    paragraph_builder.addText("")
    paragraph = paragraph_builder.Build()
    assert isinstance(paragraph, skia.textlayout_Paragraph)


# Adapted from #278, to make sure that "\n" results in a line break  (rather that .notdef).
# Height is larger than twice width, if a break happens.
def test_Paragraph_linebreak(paragraph_builder, textlayout_text_style, textlayout_font_collection, paragraph_style):
    paint = skia.Paint()
    paint.setColor(skia.ColorBLACK)
    paint.setAntiAlias(True)

    textlayout_text_style.setFontSize(50)
    textlayout_text_style.setForegroundPaint(paint)

    textlayout_font_collection.setDefaultFontManager(skia.FontMgr())

    builder = skia.textlayout.ParagraphBuilder.make(
        paragraph_style, textlayout_font_collection, skia.Unicodes.ICU.Make()
    )
    builder.pushStyle(textlayout_text_style)

    builder.addText("o\no")
    paragraph = builder.Build()
    paragraph.layout(300)
    assert (paragraph.Height > 0) and (paragraph.Height > paragraph.LongestLine * 2)


@pytest.fixture(scope='session')
def typeface_font_provider():
    return skia.textlayout.TypefaceFontProvider()

def test_textlayout_TypefaceFontProvider_init(typeface_font_provider):
    assert isinstance(typeface_font_provider, skia.textlayout_TypefaceFontProvider)
    assert typeface_font_provider.countFamilies() == 0

def test_textlayout_TypefaceFontProvider_registerTypeface0(typeface_font_provider, non_text_typeface):
    typeface_any = skia.Typeface("Text")
    assert typeface_font_provider.registerTypeface(typeface_any) == 1
    assert typeface_font_provider.countFamilies() == 1
    assert typeface_font_provider.registerTypeface(non_text_typeface) == 1
    assert typeface_font_provider.countFamilies() == 2
    # TypefaceFontProvider can detect duplicates.
    typeface_any_two = skia.Typeface("Text")
    assert typeface_font_provider.registerTypeface(typeface_any_two) == 1
    assert typeface_font_provider.countFamilies() == 2

def test_textlayout_TypefaceFontProvider_registerTypeface1(typeface_font_provider, non_text_typeface):
    typeface_any = skia.Typeface("Text")
    assert typeface_font_provider.registerTypeface(typeface_any, "Not Text") == 1
    assert typeface_font_provider.countFamilies() == 3
    assert typeface_font_provider.registerTypeface(non_text_typeface, "Not Emoji") == 1
    assert typeface_font_provider.countFamilies() == 4


@pytest.mark.parametrize('test_operator, spec_a, spec_b', [
    (operator.eq, (False, 1.0), (True, 1.0)),
    (operator.eq, (True, 0), (True, 1.0)),
    (operator.eq, (True, 0.5), (True, 1.0)),
    (operator.lt, (True, 1.0), (True, 2.0)),
    (operator.lt, (True, 2.0), (True, 3.0)),
])
def test_Paragraph_strutHeight(paragraph_builder, textlayout_text_style, textlayout_font_collection, paragraph_style, test_operator, strut_style, spec_a, spec_b):
    paint = skia.Paint()
    paint.setColor(skia.ColorBLACK)
    paint.setAntiAlias(True)

    textlayout_text_style.setFontSize(50)
    textlayout_text_style.setForegroundPaint(paint)

    textlayout_font_collection.setDefaultFontManager(skia.FontMgr())

    def graf_with_strut(enabled, leading_factor):
        strut_style.setStrutEnabled(enabled)
        strut_style.setLeading(leading_factor)
        paragraph_style.setStrutStyle(strut_style)

        builder = skia.textlayout.ParagraphBuilder.make(
            paragraph_style, textlayout_font_collection, skia.Unicodes.ICU.Make()
        )
        builder.pushStyle(textlayout_text_style)

        builder.addText("o\no")
        paragraph = builder.Build()
        paragraph.layout(300)

        return paragraph

    paragraph_a_height = graf_with_strut(*spec_a).Height
    paragraph_b_height = graf_with_strut(*spec_b).Height

    assert test_operator(paragraph_a_height, paragraph_b_height)


@pytest.mark.parametrize('spacing_a, spacing_b', [
    (-1, 0),
    (0, 1),
    (1, 2),
    (2, 3),
    (-1, 1),
])
def test_Paragraph_letterSpacing(paragraph_builder, textlayout_text_style, textlayout_font_collection, paragraph_style, strut_style, spacing_a, spacing_b):
    paint = skia.Paint()
    paint.setColor(skia.ColorBLACK)
    paint.setAntiAlias(True)

    textlayout_font_collection.setDefaultFontManager(skia.FontMgr())

    def graf_with_letterspacing(letterspacing):
        textlayout_text_style.setFontSize(50)
        textlayout_text_style.setForegroundPaint(paint)
        textlayout_text_style.setLetterSpacing(letterspacing)

        builder = skia.textlayout.ParagraphBuilder.make(
            paragraph_style, textlayout_font_collection, skia.Unicodes.ICU.Make()
        )
        builder.pushStyle(textlayout_text_style)

        builder.addText("ooo")
        paragraph = builder.Build()
        paragraph.layout(300)

        return paragraph

    assert graf_with_letterspacing(spacing_a).LongestLine < graf_with_letterspacing(spacing_b).LongestLine


@pytest.mark.parametrize('spacing_a, spacing_b', [
    (-1, 0),
    (0, 1),
    (1, 2),
    (2, 3),
    (-1, 1),
])
def test_Paragraph_wordSpacing(paragraph_builder, textlayout_text_style, textlayout_font_collection, paragraph_style, strut_style, spacing_a, spacing_b):
    paint = skia.Paint()
    paint.setColor(skia.ColorBLACK)
    paint.setAntiAlias(True)

    textlayout_font_collection.setDefaultFontManager(skia.FontMgr())

    def graf_with_word_spacing(letterspacing):
        textlayout_text_style.setFontSize(50)
        textlayout_text_style.setForegroundPaint(paint)
        textlayout_text_style.setWordSpacing(letterspacing)

        builder = skia.textlayout.ParagraphBuilder.make(
            paragraph_style, textlayout_font_collection, skia.Unicodes.ICU.Make()
        )
        builder.pushStyle(textlayout_text_style)

        builder.addText("word word word")
        paragraph = builder.Build()
        paragraph.layout(300)

        return paragraph

    assert graf_with_word_spacing(spacing_a).LongestLine < graf_with_word_spacing(spacing_b).LongestLine


def graf_with_text_height(font_collection, height_override, height, font_size=20.0):
    font_collection.setDefaultFontManager(skia.FontMgr())

    text_style = skia.textlayout.TextStyle()
    text_style.setFontSize(font_size)
    text_style.setHeightOverride(height_override)
    text_style.setHalfLeading(True)
    text_style.setHeight(height)

    paragraph_style = skia.textlayout.ParagraphStyle()

    builder = skia.textlayout.ParagraphBuilder.make(
        paragraph_style, font_collection, skia.Unicodes.ICU.Make()
    )
    builder.pushStyle(text_style)

    builder.addText("o\no")
    paragraph = builder.Build()
    paragraph.layout(300)

    return paragraph


@pytest.mark.parametrize('height', [1.0, 2.0, 3.0])
def test_Paragraph_textStyleHeightIsMultiple(textlayout_font_collection, height):
    font_size = 20.0
    paragraph = graf_with_text_height(textlayout_font_collection, True, height, font_size)

    assert paragraph.Height == pytest.approx(2 * font_size * height, abs=1)


def graf_with_baseline_shift(font_collection, shift, font_size=20.0):
    font_collection.setDefaultFontManager(skia.FontMgr())

    def style(baseline_shift):
        text_style = skia.textlayout.TextStyle()
        text_style.setFontSize(font_size)
        text_style.setBaselineShift(baseline_shift)
        return text_style

    builder = skia.textlayout.ParagraphBuilder.make(
        skia.textlayout.ParagraphStyle(), font_collection, skia.Unicodes.ICU.Make()
    )
    builder.pushStyle(style(0.0))
    builder.addText("Base")
    builder.pop()
    builder.pushStyle(style(shift))
    builder.addText("X")

    paragraph = builder.Build()
    paragraph.layout(300)

    return paragraph


@pytest.mark.parametrize('test_operator, shift_a, shift_b', [
    (operator.eq, -8.0, 8.0),
    (operator.lt, 0.0, 8.0),
    (operator.lt, 0.0, -8.0),
    (operator.lt, 8.0, 15.0),
])
def test_Paragraph_baselineShiftGrowsLine(textlayout_font_collection, test_operator, shift_a, shift_b):
    paragraph_a_height = graf_with_baseline_shift(textlayout_font_collection, shift_a).Height
    paragraph_b_height = graf_with_baseline_shift(textlayout_font_collection, shift_b).Height

    assert test_operator(paragraph_a_height, paragraph_b_height)


def graf_with_strut_font(font_collection, strut_size, strut_height, force_strut_height=False):
    font_collection.setDefaultFontManager(skia.FontMgr())

    strut_style = skia.textlayout.StrutStyle()
    strut_style.setStrutEnabled(True)
    strut_style.setFontSize(strut_size)
    strut_style.setHeightOverride(True)
    strut_style.setHeight(strut_height)
    strut_style.setForceStrutHeight(force_strut_height)

    paragraph_style = skia.textlayout.ParagraphStyle()
    paragraph_style.setStrutStyle(strut_style)

    small = skia.textlayout.TextStyle()
    small.setFontSize(12.0)
    large = skia.textlayout.TextStyle()
    large.setFontSize(36.0)

    builder = skia.textlayout.ParagraphBuilder.make(
        paragraph_style, font_collection, skia.Unicodes.ICU.Make()
    )
    builder.pushStyle(small)
    builder.addText("small\n")
    builder.pop()
    builder.pushStyle(large)
    builder.addText("LARGE")

    paragraph = builder.Build()
    paragraph.layout(600)

    return paragraph


@pytest.mark.parametrize('strut_size, strut_height', [
    (14.0, 1.0),
    (20.0, 1.0),
    (20.0, 2.0),
])
def test_Paragraph_strutStyleForceStrutHeight(textlayout_font_collection, strut_size, strut_height):
    forced = graf_with_strut_font(textlayout_font_collection, strut_size, strut_height, True)
    unforced = graf_with_strut_font(textlayout_font_collection, strut_size, strut_height, False)

    assert forced.Height == pytest.approx(2 * strut_size * strut_height, abs=1)
    assert forced.Height <= unforced.Height


# Each paragraph gets its own FontCollection: the shaping cache is not keyed on half
# leading, so sharing one would make this test pass without testing anything.

def graf_with_half_leading(half_leading, height=1.0, font_size=20.0, height_override=True):
    font_collection = skia.textlayout.FontCollection()
    font_collection.setDefaultFontManager(skia.FontMgr())

    text_style = skia.textlayout.TextStyle()
    text_style.setFontSize(font_size)
    text_style.setHeightOverride(height_override)
    text_style.setHeight(height)
    text_style.setHalfLeading(half_leading)

    paragraph_style = skia.textlayout.ParagraphStyle()
    paragraph_style.setTextStyle(text_style)

    builder = skia.textlayout.ParagraphBuilder.make(
        paragraph_style, font_collection, skia.Unicodes.ICU.Make()
    )
    builder.pushStyle(text_style)
    builder.addText("Ag")

    paragraph = builder.Build()
    paragraph.layout(300)

    return paragraph


def test_Paragraph_halfLeading():
    font_size = 20.0
    natural = graf_with_half_leading(False, 1.0, font_size, height_override=False).Height
    height = 2.0 * natural / font_size

    half = graf_with_half_leading(True, height, font_size)
    scaled = graf_with_half_leading(False, height, font_size)

    assert half.Height == pytest.approx(scaled.Height, abs=0.01)
    assert half.AlphabeticBaseline < scaled.AlphabeticBaseline
