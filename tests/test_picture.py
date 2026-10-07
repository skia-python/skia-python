import skia
import pytest


@pytest.fixture
def recorder():
    return skia.PictureRecorder()


def test_Picture_init():
    assert isinstance(skia.Picture(skia.Rect(100, 100)), skia.Picture)


def test_Picture_playback(picture, canvas):
    picture.playback(canvas)


def test_Picture_cullRect(picture):
    assert isinstance(picture.cullRect(), skia.Rect)


def test_Picture_uniqueID(picture):
    assert isinstance(picture.uniqueID(), int)


def test_Picture_serialize(picture):
    assert isinstance(picture.serialize(), skia.Data)


def test_Picture_approximateOpCount(picture):
    assert isinstance(picture.approximateOpCount(), int)


def test_Picture_approximateBytesUsed(picture):
    assert isinstance(picture.approximateBytesUsed(), int)


@pytest.mark.parametrize('args', [
    (skia.TileMode.kClamp, skia.TileMode.kClamp, skia.FilterMode.kNearest),
    (skia.TileMode.kClamp, skia.TileMode.kClamp, skia.FilterMode.kLinear),
])
def test_Picture_makeShader(picture, args):
    assert isinstance(
        picture.makeShader(*args),
        skia.Shader)


def test_Picture_MakeFromStream(picture):
    data = picture.serialize()
    stream = skia.MemoryStream(data)
    assert isinstance(
        skia.Picture.MakeFromStream(stream), skia.Picture)


def test_Picture_MakeFromData(picture):
    assert isinstance(
        skia.Picture.MakeFromData(picture.serialize()), skia.Picture)


def test_Picture_MakePlaceholder():
    assert isinstance(
        skia.Picture.MakePlaceholder(skia.Rect(100, 100)), skia.Picture)


def test_RTreeFactory_init():
    assert isinstance(skia.RTreeFactory(), skia.RTreeFactory)


@pytest.fixture
def factory():
    return skia.RTreeFactory()


def test_RTreeFactory_call(factory):
    bbh = factory()
    assert isinstance(bbh, skia.BBoxHierarchy)


def test_BBoxHierarchy_init():
    assert isinstance(skia.BBoxHierarchy(), skia.BBoxHierarchy)


def test_BBoxHierarchy_insert(factory):
    bbh = factory()
    bbh.insert([skia.Rect(100, 100)])


def test_BBoxHierarchy_search(factory):
    bbh = factory()
    bbh.insert([skia.Rect(100, 100)])
    results = bbh.search(skia.Rect(100, 100))
    assert results[0] == 0


def test_inherit_BBoxHierarchy():
    class TestBBH(skia.BBoxHierarchy):
        def __init__(self):
            self.rects = []
            super().__init__()

        def search(self, query):
            return [i for i, rect in enumerate(self.rects) if rect.intersects(query)]

        def insert(self, rects, metadata=[]):
            self.rects.extend(rects)

        def bytesUsed(self):
            return 0

    bbh = TestBBH()
    bbh.insert([skia.Rect(100, 100)])
    assert len(bbh.rects) == 1
    results = bbh.search(skia.Rect(100, 100))
    assert results[0] == 0


def test_PictureRecorder_init():
    assert isinstance(skia.PictureRecorder(), skia.PictureRecorder)


@pytest.mark.parametrize('args', [
    (skia.Rect(100, 100),),
    (100, 100),
    (skia.Rect(100, 100), skia.RTreeFactory()()),
])
def test_PictureRecorder_beginRecording(recorder, args):
    canvas = recorder.beginRecording(*args)
    assert isinstance(canvas, skia.Canvas)


def test_PictureRecorder_getRecordingCanvas(recorder):
    assert isinstance(recorder.getRecordingCanvas(), (skia.Canvas, type(None)))


def test_PictureRecorder_finishRecordingAsPicture(recorder):
    canvas = recorder.beginRecording(skia.Rect(100, 100))
    canvas.clear(0xFFFFFFFF)
    assert isinstance(recorder.finishRecordingAsPicture(), skia.Picture)


def test_PictureRecorder_finishRecordingAsPictureWithCull(recorder):
    canvas = recorder.beginRecording(skia.Rect(100, 100))
    canvas.clear(0xFFFFFFFF)
    assert isinstance(
        recorder.finishRecordingAsPictureWithCull(skia.Rect(80, 80)),
        skia.Picture)


def test_PictureRecorder_finishRecordingAsDrawable(drawable):
    assert isinstance(drawable, skia.Drawable)


@pytest.fixture
def drawable(recorder):
    canvas = recorder.beginRecording(skia.Rect(100, 100))
    canvas.clear(0xFFFFFFFF)
    return recorder.finishRecordingAsDrawable()


@pytest.mark.parametrize('args', [
    tuple(),
    (5, 5),
])
def test_Drawable_draw(drawable, canvas, args):
    drawable.draw(canvas, *args)


def test_Drawable_newPictureSnapshot(drawable):
    assert isinstance(drawable.newPictureSnapshot(), skia.Picture)


def test_Drawable_getGenerationID(drawable):
    assert isinstance(drawable.getGenerationID(), int)


def test_Drawable_getBounds(drawable):
    assert isinstance(drawable.getBounds(), skia.Rect)


def test_Drawable_notifyDrawingChanged(drawable):
    drawable.notifyDrawingChanged()


# https://github.com/google/skia/blob/ad08229fd0163a784c60a8bac2c0c5a6a13877c9/tests/PictureTest.cpp#L869-L893
def test_Picture_fillsBBH(factory, recorder):
    rects = [
        skia.Rect(0, 0, 20, 20),
        skia.Rect(20, 20, 40, 40),
    ]

    for n in range(len(rects) + 1):
        bbh = factory()

        canvas = recorder.beginRecording(skia.Rect(0, 0, 100, 100), bbh)
        for i in range(n):
            canvas.drawRect(rects[i], skia.Paint())
        recorder.finishRecordingAsPicture()

        results = bbh.search(skia.Rect(0, 0, 100, 100))
        assert len(results) == n


# https://github.com/google/skia/blob/64148dd7cfe0a3f104d93f58ec42592a0252d378/tests/PictureBBHTest.cpp#L99-L127
def test_PictureNegativeSpace(factory, recorder):
    cull = skia.Rect(-200, -200, 200, 200)

    bbh = factory()
    canvas = recorder.beginRecording(cull, bbh)
    canvas.save()
    canvas.clipRect(cull)
    canvas.drawRect(skia.Rect(-20, -20, -10, -10), skia.Paint())
    canvas.drawRect(skia.Rect(-20, -20, -10, -10), skia.Paint())
    canvas.restore()
    picture = recorder.finishRecordingAsPicture()
    assert picture.approximateOpCount() == 5
    assert picture.cullRect() == skia.Rect(-20, -20, -10, -10)

    bbh = factory()
    canvas = recorder.beginRecording(cull, bbh)
    canvas.clipRect(cull)
    canvas.drawRect(skia.Rect(-20, -20, -10, -10), skia.Paint())
    canvas.drawRect(skia.Rect(-20, -20, -10, -10), skia.Paint())
    picture = recorder.finishRecordingAsPicture()
    assert picture.approximateOpCount() == 3
    assert picture.cullRect() == skia.Rect(-20, -20, -10, -10)


# https://github.com/google/skia/blob/64148dd7cfe0a3f104d93f58ec42592a0252d378/tests/PictureTest.cpp#L613-L657
def test_Picture_SkipBBH(recorder):
    class CountingBBH(skia.BBoxHierarchy):
        def __init__(self):
            self.search_calls: int = 0
            super().__init__()

        def search(self, query):
            self.search_calls += 1

        def insert(self, rects, metadata=[]):
            pass

        def bytesUsed(self):
            return 0

    bound = skia.Rect(320, 240)

    bbh = CountingBBH()

    canvas = recorder.beginRecording(bound, bbh)
    canvas.drawRect(bound, skia.Paint())
    canvas.drawRect(bound, skia.Paint())
    picture = recorder.finishRecordingAsPicture()

    big = skia.Canvas(640, 480)
    small = skia.Canvas(300, 200)

    picture.playback(big)
    assert bbh.search_calls == 0

    picture.playback(small)
    assert bbh.search_calls == 1
