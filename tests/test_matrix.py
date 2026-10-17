import skia
import pytest
import numpy as np


@pytest.fixture
def matrix():
    return skia.Matrix()


@pytest.mark.parametrize('args', [
    tuple(),
    (np.zeros((3, 3), dtype=np.float32),),
    (np.zeros((1, 9), dtype=np.float32),),
    (np.zeros((9, 1), dtype=np.float32),),
    pytest.param(np.zeros((1, 1), dtype=np.float32),
        marks=pytest.mark.xfail),
    pytest.param(np.zeros((3, 3), dtype=np.float64),
        marks=pytest.mark.xfail),
])
def test_Matrix_init(args):
    assert isinstance(skia.Matrix(*args), skia.Matrix)


def test_Matrix_getType(matrix):
    assert isinstance(matrix.getType(), skia.Matrix.TypeMask)


def test_Matrix_isIdentity(matrix):
    assert isinstance(matrix.isIdentity(), bool)


def test_Matrix_isScaleTranslate(matrix):
    assert isinstance(matrix.isScaleTranslate(), bool)


def test_Matrix_isTranslate(matrix):
    assert isinstance(matrix.isTranslate(), bool)


def test_Matrix_rectStaysRect(matrix):
    assert isinstance(matrix.rectStaysRect(), bool)


def test_Matrix_preservesAxisAlignment(matrix):
    assert isinstance(matrix.preservesAxisAlignment(), bool)


def test_Matrix_hasPerspective(matrix):
    assert isinstance(matrix.hasPerspective(), bool)


def test_Matrix_isSimilarity(matrix):
    assert isinstance(matrix.isSimilarity(), bool)


def test_Matrix_preservesRightAngles(matrix):
    assert isinstance(matrix.preservesRightAngles(), bool)


def test_Matrix_getitem(matrix):
    assert isinstance(matrix[0], float)
    with pytest.raises(IndexError):
        matrix[-1]


def test_Matrix_get(matrix):
    assert isinstance(matrix.get(0), float)


def test_Matrix_rc(matrix):
    assert isinstance(matrix.rc(0, 0), float)


def test_Matrix_getScaleX(matrix):
    assert isinstance(matrix.getScaleX(), float)


def test_Matrix_getScaleY(matrix):
    assert isinstance(matrix.getScaleY(), float)


def test_Matrix_getSkewY(matrix):
    assert isinstance(matrix.getSkewY(), float)


def test_Matrix_getSkewX(matrix):
    assert isinstance(matrix.getSkewX(), float)


def test_Matrix_getTranslateX(matrix):
    assert isinstance(matrix.getTranslateX(), float)


def test_Matrix_getTranslateY(matrix):
    assert isinstance(matrix.getTranslateY(), float)


def test_Matrix_getPerspX(matrix):
    assert isinstance(matrix.getPerspX(), float)


def test_Matrix_getPerspY(matrix):
    assert isinstance(matrix.getPerspY(), float)


def test_Matrix_setitem(matrix):
    matrix[0] = 1.0
    with pytest.raises(IndexError):
        matrix[-1] = 1.0


def test_Matrix_set(matrix):
    matrix.set(0, 1.0)


def test_Matrix_setScaleX(matrix):
    assert isinstance(matrix.setScaleX(1.0), skia.Matrix)


def test_Matrix_setScaleY(matrix):
    assert isinstance(matrix.setScaleY(1.0), skia.Matrix)


def test_Matrix_setSkewY(matrix):
    assert isinstance(matrix.setSkewY(1.0), skia.Matrix)


def test_Matrix_setSkewX(matrix):
    assert isinstance(matrix.setSkewX(1.0), skia.Matrix)


def test_Matrix_setTranslateX(matrix):
    assert isinstance(matrix.setTranslateX(1.0), skia.Matrix)


def test_Matrix_setTranslateY(matrix):
    assert isinstance(matrix.setTranslateY(1.0), skia.Matrix)


def test_Matrix_setPerspX(matrix):
    assert isinstance(matrix.setPerspX(1.0), skia.Matrix)


def test_Matrix_setPerspY(matrix):
    assert isinstance(matrix.setPerspY(1.0), skia.Matrix)


def test_Matrix_setAll(matrix):
    matrix.setAll(*range(9))


def test_Matrix_get9(matrix):
    values = matrix.get9()
    assert isinstance(values, list)
    assert len(values) == 9


def test_Matrix_set9(matrix):
    assert isinstance(matrix.set9([0, 0, 0, 0, 0, 0, 0, 0, 0]), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0,),
    (skia.Point(0, 0),),
])
def test_Matrix_setTranslate(matrix, args):
    assert isinstance(matrix.setTranslate(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (1, 1, 0, 0,),
    (1, 1,),
])
def test_Matrix_setScale(matrix, args):
    assert isinstance(matrix.setScale(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0, 0,),
    (0,),
])
def test_Matrix_setRotate(matrix, args):
    assert isinstance(matrix.setRotate(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 1, 0, 0,),
    (0, 1,),
])
def test_Matrix_setSinCos(matrix, args):
    assert isinstance(matrix.setSinCos(*args), skia.Matrix)


def test_Matrix_setRSXform(matrix):
    assert isinstance(
        matrix.setRSXform(skia.RSXform.Make(1, 0, 5, 10)), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0, 0, 0,),
    (0, 0,),
])
def test_Matrix_setSkew(matrix, args):
    assert isinstance(matrix.setSkew(*args), skia.Matrix)


def test_Matrix_setConcat(matrix):
    assert isinstance(
        matrix.setConcat(skia.Matrix(), skia.Matrix()), skia.Matrix)


def test_Matrix_preTranslate(matrix):
    assert isinstance(matrix.preTranslate(0, 0), skia.Matrix)


@pytest.mark.parametrize('args', [
    (1, 1, 0, 0,),
    (1, 1,),
])
def test_Matrix_preScale(matrix, args):
    assert isinstance(matrix.preScale(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0, 0,),
    (0,),
])
def test_Matrix_preRotate(matrix, args):
    assert isinstance(matrix.preRotate(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0, 0, 0,),
    (0, 0,),
])
def test_Matrix_preSkew(matrix, args):
    assert isinstance(matrix.preSkew(*args), skia.Matrix)


def test_Matrix_preConcat(matrix):
    assert isinstance(matrix.preConcat(skia.Matrix()), skia.Matrix)


def test_Matrix_postTranslate(matrix):
    assert isinstance(matrix.postTranslate(0, 0), skia.Matrix)


@pytest.mark.parametrize('args', [
    (1, 1, 0, 0,),
    (1, 1,),
])
def test_Matrix_postScale(matrix, args):
    assert isinstance(matrix.postScale(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0, 0,),
    (0,),
])
def test_Matrix_postRotate(matrix, args):
    assert isinstance(matrix.postRotate(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0, 0, 0,),
    (0, 0,),
])
def test_Matrix_postSkew(matrix, args):
    assert isinstance(matrix.postSkew(*args), skia.Matrix)


def test_Matrix_postConcat(matrix):
    assert isinstance(matrix.postConcat(skia.Matrix()), skia.Matrix)


def test_Matrix_setRectToRect(matrix):
    assert isinstance(
        matrix.setRectToRect(
            skia.Rect(100, 100),
            skia.Rect(100, 100),
            skia.Matrix.kCenter_ScaleToFit),
        bool)


def test_Matrix_setPolyToPoly(matrix):
    assert isinstance(
        matrix.setPolyToPoly([skia.Point(100, 100)], [skia.Point(100, 100)]),
        bool)


def test_Matrix_invert(matrix):
    assert isinstance(matrix.invert(skia.Matrix()), bool)


def test_Matrix_asAffine(matrix):
    assert isinstance(matrix.asAffine(), (list, type(None)))


def test_Matrix_setAffine(matrix):
    assert isinstance(matrix.setAffine([1, 0, 5, 0, 1, 3]), skia.Matrix)


def test_Matrix_normalizePerspective(matrix):
    matrix.normalizePerspective()


def test_Matrix_mapPoints(matrix):
    assert isinstance(matrix.mapPoints([skia.Point(1, 1)]), list)


@pytest.mark.parametrize('pts', [
    [skia.Point3(1, 1, 1)],
    [skia.Point(1, 1)]
])
def test_Matrix_mapHomogeneousPoints(matrix, pts):
    assert isinstance(matrix.mapHomogeneousPoints(pts), list)


def test_Matrix_mapXY(matrix):
    assert isinstance(matrix.mapXY(0, 0), skia.Point)


def test_Matrix_mapVectors(matrix):
    assert isinstance(matrix.mapVectors([skia.Point(0, 0)]), list)


def test_Matrix_mapVector(matrix):
    assert isinstance(matrix.mapVector(0, 0), skia.Point)


def test_Matrix_mapRect(matrix):
    assert isinstance(matrix.mapRect(skia.Rect(10, 10)), skia.Rect)


def test_Matrix_mapRectToQuad(matrix):
    assert isinstance(matrix.mapRectToQuad(skia.Rect(10, 10)), list)


def test_Matrix_mapRectScaleTranslate(matrix):
    assert isinstance(
        matrix.mapRectScaleTranslate(skia.Rect(10, 10)), skia.Rect)


def test_Matrix_mapRadius(matrix):
    assert isinstance(matrix.mapRadius(10), float)


def test_Matrix_dump(matrix):
    matrix.dump()


def test_Matrix_getMinScale(matrix):
    assert isinstance(matrix.getMinScale(), float)


def test_Matrix_getMaxScale(matrix):
    assert isinstance(matrix.getMaxScale(), float)


def test_Matrix_getMinMaxScales(matrix):
    assert isinstance(matrix.getMinMaxScales(), tuple)


def test_Matrix_decomposeScale(matrix):
    scale = skia.Size(0, 0)
    remaining = skia.Matrix()
    assert isinstance(matrix.decomposeScale(scale, remaining), bool)


def test_Matrix_dirtyMatrixTypeCache(matrix):
    matrix.dirtyMatrixTypeCache()


def test_Matrix_setScaleTranslate(matrix):
    matrix.setScaleTranslate(1, 1, 0, 0)


def test_Matrix_isFinite(matrix):
    assert isinstance(matrix.isFinite(), bool)


def test_Matrix_Scale():
    assert isinstance(skia.Matrix.Scale(1, 1), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0, 0),
    (skia.Point(0, 0),),
    (skia.IPoint(0, 0),),
])
def test_Matrix_Translate(args):
    assert isinstance(skia.Matrix.Translate(*args), skia.Matrix)


@pytest.mark.parametrize('args', [
    (0,),
    (0, skia.Point(0, 0)),
])
def test_Matrix_RotateDeg(args):
    assert isinstance(skia.Matrix.RotateDeg(*args), skia.Matrix)


def test_Matrix_RotateRad():
    assert isinstance(skia.Matrix.RotateDeg(0), skia.Matrix)


def test_Matrix_MakeAll():
    assert isinstance(
        skia.Matrix.MakeAll(1, 0, 0, 0, 1, 0, 0, 0, 1), skia.Matrix)


def test_Matrix_MakeRectToRect():
    assert isinstance(
        skia.Matrix.MakeRectToRect(
            skia.Rect(100, 100),
            skia.Rect(100, 100),
            skia.Matrix.kCenter_ScaleToFit),
        skia.Matrix)


def test_Matrix_SetAffineIdentity():
    assert isinstance(skia.Matrix.SetAffineIdentity(), list)


def test_Matrix_I():
    assert isinstance(skia.Matrix.I(), skia.Matrix)


def test_Matrix_InvalidMatrix():
    assert isinstance(skia.Matrix.InvalidMatrix(), skia.Matrix)


def test_Matrix_Concat(matrix):
    assert isinstance(skia.Matrix.Concat(matrix, matrix), skia.Matrix)


@pytest.fixture
def m44():
    return skia.M44()


def test_m44_set_outofbound(m44):
    try:
        outofbound = m44.rc(4,0)
        raise RuntimeError('Not reachable - Out of Bound is expected, so you should not see this!')
    except ValueError:
        # expected.
        pass


def test_m44_set_and_get(m44):
    oldvalue = m44.rc(3,2)
    m44.setRC(3,2, 100)
    assert m44.rc(3,2) == 100
    m44.setRC(3,2, oldvalue)
