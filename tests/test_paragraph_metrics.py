"""Native paragraph introspection, UTF offsets and owned visitor lifetimes."""
import gc

import pytest
import skia


@pytest.fixture
def metrics_paragraph():
    """Build real paragraphs; segment pairs specify text and pixel font size."""
    def build(segments=(("Ag e\u0301", 24),), width=1000):
        fonts = skia.textlayout.FontCollection()
        fonts.setDefaultFontManager(skia.FontMgr.RefDefault())
        builder = skia.textlayout.ParagraphBuilder.make(
            skia.textlayout.ParagraphStyle(), fonts, skia.Unicodes.ICU.Make())
        for text, size in segments:
            style = skia.textlayout.TextStyle()
            style.setFontSize(size)
            builder.pushStyle(style)
            builder.addText(text)
            builder.pop()
        paragraph = builder.Build()
        paragraph.layout(width)
        return paragraph
    return build


def collect_visits(paragraph, extended=False):
    """Retain each native run and line-end signal for assertions after return."""
    visits = []
    method = paragraph.extendedVisit if extended else paragraph.visit
    method(lambda line, info: visits.append((line, info)))
    return visits


def run_signature(info, extended=False):
    """Read owned font/array fields, including bounds in an extended snapshot."""
    value = (info.font.getSize(), info.font.refTypeface().getFamilyName(),
             tuple(info.glyphs), tuple((p.x(), p.y()) for p in info.positions),
             tuple(info.utf8Starts), info.flags)
    if extended:
        value += (tuple((r.left(), r.top(), r.right(), r.bottom()) for r in info.bounds),)
    return value


def test_numeric_line_metrics(metrics_paragraph):
    """Each line has baseline/ascent/descent and agrees with indexed lookup."""
    paragraph = metrics_paragraph((("Ag\nAg", 24),))
    metrics = paragraph.getLineMetrics()
    assert paragraph.lineNumber() == len(metrics) == 2
    assert metrics[1].fBaseline > metrics[0].fBaseline
    for index, line in enumerate(metrics):
        assert line.fLineNumber == index
        assert line.fAscent > 0 and line.fDescent >= 0 and line.fWidth > 0
        assert paragraph.getLineMetricsAt(index).fBaseline == line.fBaseline
    assert paragraph.getLineMetricsAt(-1) is None
    assert paragraph.getLineMetricsAt(2) is None


def test_range_indices_and_trailing_spaces(metrics_paragraph):
    """Actual line ranges are UTF-8 bytes, whereas rectangle queries use UTF-16."""
    text = "A\u00e9\U0001f600  "
    paragraph = metrics_paragraph(((text, 24),))
    full = paragraph.getActualTextRange(0, True)
    trimmed = paragraph.getActualTextRange(0, False)
    assert (full.start, full.end) == (0, len(text.encode("utf-8")))
    assert (trimmed.start, trimmed.end) == (0, len(text.rstrip().encode("utf-8")))
    boxes = paragraph.getRectsForRange(
        4, 6, skia.textlayout.RectHeightStyle.kTight,
        skia.textlayout.RectWidthStyle.kTight)
    assert boxes and sum(box.rect.width() for box in boxes) > 0
    assert paragraph.getRectsForRange(0, 0, skia.textlayout.RectHeightStyle.kTight,
                                      skia.textlayout.RectWidthStyle.kTight) == []


@pytest.mark.parametrize("extended", [False, True])
def test_visitor_arrays_and_line_end_signals(metrics_paragraph, extended):
    """Borrowed native arrays become owned values without changing run order."""
    paragraph = metrics_paragraph((("Ag e\u0301\nNext", 24),))
    visits = collect_visits(paragraph, extended)
    assert [line for line, info in visits if info is None] == [0, 1]
    runs = [info for _, info in visits if info is not None]
    assert runs
    for info in runs:
        assert info.count > 0
        assert len(info.glyphs) == len(info.positions) == len(info.utf8Starts) == info.count
        assert all(0 <= offset < len("Ag e\u0301\nNext".encode("utf-8")) for offset in info.utf8Starts)
        assert isinstance(info.font, skia.Font)
        assert info.font.getSize() == 24
        if extended:
            assert len(info.bounds) == info.count
    if extended:
        assert any(not rect.isEmpty() for info in runs for rect in info.bounds)


@pytest.mark.parametrize("extended", [False, True])
def test_visitor_survives_relayout_and_gc(metrics_paragraph, extended):
    """Retained fonts, glyphs and bounds must not borrow paragraph storage."""
    paragraph = metrics_paragraph((("A long Ag e\u0301 line", 24),))
    infos = [info for _, info in collect_visits(paragraph, extended) if info is not None]
    before = [run_signature(info, extended) for info in infos]
    paragraph.layout(40)
    del paragraph
    gc.collect()
    assert [run_signature(info, extended) for info in infos] == before


@pytest.mark.parametrize("extended", [False, True])
def test_visitor_exception_propagates(metrics_paragraph, extended):
    """Callback exceptions are neither swallowed nor converted into success."""
    paragraph = metrics_paragraph()
    def fail(line, info):
        raise ValueError("visitor failure")
    with pytest.raises(ValueError, match="visitor failure"):
        (paragraph.extendedVisit if extended else paragraph.visit)(fail)
    assert paragraph.getLineMetricsAt(0).fWidth > 0


def test_fonts_use_native_utf8_ranges_and_utf16_lookup(metrics_paragraph):
    """A second style after multi-byte and non-BMP text keeps both index units."""
    paragraph = metrics_paragraph((("\u00e9\U0001f600", 18), ("B", 36)))
    font = paragraph.getFontAtUTF16Offset(3)
    assert font.getSize() == 36
    fonts = paragraph.getFonts()
    assert any(item.fTextRange.start <= 6 < item.fTextRange.end
               and item.fFont.getSize() == 36 for item in fonts)
    assert all(0 <= item.fTextRange.start <= item.fTextRange.end <= 7 for item in fonts)
    del paragraph
    gc.collect()
    assert font.refTypeface() is not None
    assert all(item.fFont.refTypeface() is not None for item in fonts)


def test_empty_and_ascii_diagnostics(metrics_paragraph):
    """Empty text is valid; supported ASCII must not be reported unresolved."""
    empty = metrics_paragraph((("", 24),))
    assert empty.getRectsForRange(0, 0, skia.textlayout.RectHeightStyle.kTight,
                                  skia.textlayout.RectWidthStyle.kTight) == []
    assert empty.lineNumber() == 0
    assert collect_visits(empty, True) == []
    ascii_text = metrics_paragraph((("ABC", 24),))
    assert ascii_text.unresolvedGlyphs() == 0
    assert ascii_text.unresolvedCodepoints() == set()


def test_asset_font_manager_retains_registered_typeface():
    """Asset-manager binding preserves native ownership after provider GC."""
    face = skia.Typeface.MakeDefault()
    fonts = skia.textlayout.FontCollection()
    fonts.setDefaultFontManager(skia.FontMgr.RefDefault())
    provider = skia.textlayout.TypefaceFontProvider()
    provider.registerTypeface(face, skia.String("ParagraphMetricsAsset"))
    fonts.setAssetFontManager(provider)
    del provider
    gc.collect()
    style = skia.textlayout.TextStyle()
    style.setFontSize(24)
    style.setFontFamilies(["ParagraphMetricsAsset"])
    paragraph_style = skia.textlayout.ParagraphStyle()
    paragraph_style.setTextStyle(style)
    builder = skia.textlayout.ParagraphBuilder.make(
        paragraph_style, fonts, skia.Unicodes.ICU.Make())
    builder.addText("Ag")
    paragraph = builder.Build()
    paragraph.layout(500)
    assert paragraph.getFontAtUTF16Offset(0).refTypeface().uniqueID() == face.uniqueID()


def test_inspection_does_not_change_pixels(metrics_paragraph):
    """Querying native data is observational and leaves paragraph paint intact."""
    paragraph = metrics_paragraph((("Ag e\u0301", 24),))
    surface = skia.Surface(400, 100)
    def paint():
        canvas = surface.getCanvas()
        canvas.clear(skia.ColorWHITE)
        paragraph.paint(canvas, 10, 10)
        return surface.makeImageSnapshot().tobytes()
    before = paint()
    paragraph.getLineMetrics()
    paragraph.getActualTextRange(0, True)
    paragraph.getRectsForRange(0, 6, skia.textlayout.RectHeightStyle.kTight,
                               skia.textlayout.RectWidthStyle.kTight)
    collect_visits(paragraph)
    collect_visits(paragraph, True)
    paragraph.getFonts()
    assert paint() == before


def test_text_style_metrics_keep_typeface_and_snapshot_lifetime():
    """Style metrics retain the selected face and return independent value copies."""
    face = skia.Typeface.MakeDefault()
    assert face is not None
    style = skia.textlayout.TextStyle()
    style.setTypeface(face)
    style.setFontSize(24)
    raw = skia.Font(face, 24).getMetrics()
    snapshot = style.getFontMetrics()
    assert snapshot.fAscent == pytest.approx(raw.fAscent - raw.fLeading / 2)
    assert snapshot.fDescent == pytest.approx(raw.fDescent + raw.fLeading / 2)
    before = (snapshot.fAscent, snapshot.fDescent, snapshot.fLeading)
    style.setFontSize(48)
    assert style.getFontMetrics().fAscent != snapshot.fAscent
    del face, style
    gc.collect()
    assert (snapshot.fAscent, snapshot.fDescent, snapshot.fLeading) == before


def test_text_style_metrics_apply_native_height_and_baseline_shift():
    """Height override and baseline shift are calculated by Skia, not Python glue."""
    style = skia.textlayout.TextStyle()
    style.setTypeface(skia.Typeface.MakeDefault())
    style.setFontSize(24)
    style.setHeight(2)
    style.setHeightOverride(True)
    before = style.getFontMetrics()
    assert before.fDescent - before.fAscent == pytest.approx(48)
    style.setBaselineShift(3)
    after = style.getFontMetrics()
    assert after.fAscent == pytest.approx(before.fAscent + 3)
    assert after.fDescent == pytest.approx(before.fDescent + 3)


def test_paragraph_style_hinting_and_single_line_controls():
    """Bind native flag/line limit setters without reconstructing layout in Python."""
    style = skia.textlayout.ParagraphStyle()
    assert style.hintingIsOn() is True
    style.turnHintingOff()
    assert style.hintingIsOn() is False
    style.setMaxLines(1)
    fonts = skia.textlayout.FontCollection()
    fonts.setDefaultFontManager(skia.FontMgr.RefDefault())
    builder = skia.textlayout.ParagraphBuilder.make(style, fonts, skia.Unicodes.ICU.Make())
    builder.addText("Ag\nNext")
    paragraph = builder.Build()
    paragraph.layout(1000)
    assert paragraph.lineNumber() == 1
    assert paragraph.ExceedMaxLines

