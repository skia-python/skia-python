"""Native SurfaceProps text controls; no raster or color compensation in Python."""
import gc

import pytest
import skia


@pytest.mark.parametrize("contrast,gamma", [(0, 1.4), (.5, 0), (.3, 2.2)])
def test_explicit_text_surface_properties_and_owned_copy(contrast, gamma):
    """Explicit values survive copying and actual raster surface construction."""
    original = skia.SurfaceProps(0, skia.PixelGeometry.kUnknown_PixelGeometry, contrast, gamma)
    assert original.textContrast() == pytest.approx(contrast)
    assert original.textGamma() == pytest.approx(gamma)
    copied = skia.SurfaceProps(original)
    assert copied == original
    del original
    gc.collect()
    assert copied.textContrast() == pytest.approx(contrast)
    assert copied.textGamma() == pytest.approx(gamma)
    surface = skia.Surface(32, 32, surfaceProps=copied)
    assert surface.props().textContrast() == pytest.approx(contrast)
    assert surface.props().textGamma() == pytest.approx(gamma)
    surface.getCanvas().clear(skia.ColorWHITE)
    assert surface.width() == surface.height() == 32
    assert surface.makeImageSnapshot() is not None

