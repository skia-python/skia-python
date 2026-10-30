"""Native collection selection/ownership; no alternate font matching implementation."""
import gc
import pytest
import skia


def face_and_manager(style=None):
    """Select an installed native Latin face and retain its default manager."""
    manager=skia.FontMgr.RefDefault()
    style=skia.FontStyle.Normal() if style is None else style
    for name in ('Arial','DejaVu Sans','Liberation Sans','Ubuntu','Noto Sans'):
        face=manager.matchFamilyStyle(name,style)
        if face is not None:
            return face,manager
    raise AssertionError('A native Latin font is required; do not skip binding tests')


@pytest.mark.parametrize('bold',[False,True])
def test_legacy_default_accepts_none(bold):
    """None reaches native nullptr, retaining normal/bold default face selection."""
    face,manager=face_and_manager()
    chosen=manager.legacyMakeTypeface(None,skia.FontStyle.Bold() if bold else skia.FontStyle.Normal())
    assert chosen is not None and chosen.getFamilyName()


def test_legacy_explicit_family_keeps_string_contract():
    """An existing string still selects the same native family/style."""
    face,manager=face_and_manager()
    chosen=manager.legacyMakeTypeface(face.getFamilyName(),skia.FontStyle.Normal())
    assert chosen.uniqueID()==face.uniqueID()


def test_find_typefaces_preserves_order_duplicates_and_owning_results():
    """Return real native matches; missing families omitted, no deduplication."""
    face,manager=face_and_manager()
    provider=skia.textlayout.TypefaceFontProvider()
    provider.registerTypeface(face,skia.String('Collection Alias'))
    collection=skia.textlayout.FontCollection()
    collection.setDefaultFontManager(manager)
    collection.setAssetFontManager(provider)
    matches=collection.findTypefaces(['Collection Alias','Missing Native Family 93820',face.getFamilyName(),'Collection Alias'],skia.FontStyle.Normal())
    assert [item.uniqueID() for item in matches]==[face.uniqueID()]*3
    expected=face.getFamilyName()
    del collection,provider,face,manager
    gc.collect()
    assert all(item.getFamilyName()==expected for item in matches)


def test_dynamic_precedes_assets_and_native_clear_caches_releases_old_lookup():
    """Managers are natively retained; cache invalidation remains explicit."""
    regular,manager=face_and_manager()
    bold=manager.matchFamilyStyle(regular.getFamilyName(),skia.FontStyle.Bold())
    assert bold is not None and bold.uniqueID()!=regular.uniqueID()
    asset=skia.textlayout.TypefaceFontProvider();asset.registerTypeface(regular,skia.String('Shared Alias'))
    dynamic=skia.textlayout.TypefaceFontProvider();dynamic.registerTypeface(bold,skia.String('Shared Alias'))
    collection=skia.textlayout.FontCollection();collection.setDefaultFontManager(manager);collection.setAssetFontManager(asset)
    assert collection.findTypefaces(['Shared Alias'],skia.FontStyle.Normal())[0].uniqueID()==regular.uniqueID()
    collection.setDynamicFontManager(dynamic);collection.clearCaches()
    expected=bold.uniqueID()
    del dynamic,bold,asset
    gc.collect()
    assert collection.findTypefaces(['Shared Alias'],skia.FontStyle.Normal())[0].uniqueID()==expected
    collection.setDynamicFontManager(None);collection.clearCaches()
    assert collection.findTypefaces(['Shared Alias'],skia.FontStyle.Normal())[0].uniqueID()==regular.uniqueID()


def test_empty_family_list_uses_native_configured_defaults():
    """An empty request uses the existing native default-family list, not Python guesses."""
    face,manager=face_and_manager()
    provider=skia.textlayout.TypefaceFontProvider();provider.registerTypeface(face,skia.String('Default Alias'))
    collection=skia.textlayout.FontCollection()
    collection.setDefaultFontManager(manager,[skia.String('Default Alias')]);collection.setAssetFontManager(provider)
    assert collection.findTypefaces([],skia.FontStyle.Normal())[0].uniqueID()==face.uniqueID()
