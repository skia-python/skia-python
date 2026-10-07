import pytest
import skia


@pytest.fixture()
def ipoint():
    return skia.IPoint.Make(4, 3)


@pytest.mark.parametrize('args', [
    (4, 3,),
    ((4, 3),),
])
def test_IPoint_init(args):
    assert isinstance(skia.IPoint(*args), skia.IPoint)


def test_IPoint_x(ipoint):
    assert ipoint.x() == 4


def test_IPoint_y(ipoint):
    assert ipoint.y() == 3


def test_IPoint_isZero(ipoint):
    assert not ipoint.isZero()


def test_IPoint_set(ipoint):
    ipoint.set(4, 3)


def test_IPoint_neg(ipoint):
    assert (-ipoint).x() == -4


def test_IPoint_iadd(ipoint):
    ipoint += skia.IPoint(1, 1)
    assert ipoint.equals(5, 4)


def test_IPoint_isub(ipoint):
    ipoint -= skia.IPoint(1, 1)
    assert ipoint.equals(3, 2)


def test_IPoint_equals(ipoint):
    assert ipoint.equals(4, 3)


def test_IPoint_fX_fY(ipoint):
    ipoint.fX = 4
    ipoint.fY = 3
    assert ipoint.fX == 4
    assert ipoint.fY == 3


def test_IPoint_eq(ipoint):
    assert ipoint == skia.IPoint(4, 3)
    assert ipoint == (4, 3)


def test_IPoint_ne(ipoint):
    assert ipoint != skia.IPoint(4, 0)


@pytest.mark.parametrize('other', [skia.IPoint(0, 0), (1, 0)])
def test_IPoint_add(ipoint, other):
    assert isinstance(ipoint + other, skia.IPoint)


@pytest.mark.parametrize('other', [skia.IPoint(0, 0), (1, 0)])
def test_IPoint_sub(ipoint, other):
    assert isinstance(ipoint - other, skia.IPoint)


def test_IPoint_Make():
    assert isinstance(skia.IPoint.Make(4, 3), skia.IPoint)


def test_IPoint_iter(ipoint):
    assert tuple(ipoint) == (4, 3)


def test_IPoint_len(ipoint):
    assert len(ipoint) == 2


def test_IPoint_repr(ipoint):
    assert isinstance(repr(ipoint), str)


@pytest.fixture()
def point():
    return skia.Point(4, 3)


@pytest.mark.parametrize('args', [
    (4., 3.),
    ((4., 3.),),
    (skia.IPoint(4, 3),),
])
def test_Point_init(args):
    assert isinstance(skia.Point(*args), skia.Point)


def test_Point_Offset():
    points = [skia.Point(1, 2), skia.Point(3, 4)]
    points = skia.Point.Offset(points, 1, 1)
    assert points[0].equals(2, 3)
    assert points[1].equals(4, 5)


def test_Point_x(point):
    assert point.x() == 4


def test_Point_x(point):
    assert point.y() == 3


def test_Point_isZero(point):
    assert not point.isZero()


def test_Point_set(point):
    point.set(4, 3)


def test_Point_iset(point):
    point.iset(4, 3)


def test_Point_iset(point):
    point.iset((4, 3))


def test_Point_neg_setAbs(point):
    point.setAbs(-point)
    assert point.x() == 4


def test_Point_offset(point):
    point.offset(0, 0)


def test_Point_length(point):
    assert point.length() == 5


def test_Point_distanceToOrigin(point):
    assert point.distanceToOrigin() == 5


def test_Point_normalize(point):
    assert point.normalize()


def test_Point_setNormalize(point):
    assert point.setNormalize(4, 3)


@pytest.mark.parametrize('args', [(5,), (4, 3, 5),])
def test_Point_setLength(point, args):
    assert point.setLength(*args)


def test_Point_scale(point):
    assert isinstance(point.scale(1), skia.Point)


def test_Point_negate(point):
    point.negate()
    assert point.x() == -4


def test_Point_neg(point):
    assert (-point).x() == -4


def test_Point_iadd(point):
    point += (1, 1)
    assert point.equals(5, 4)


def test_Point_isub(point):
    point -= (1, 1)
    assert point.equals(3, 2)


def test_Point_mul(point):
    assert isinstance(point * 1., skia.Point)


def test_Point_imul(point):
    point *= 1.


def test_Point_isFinite(point):
    assert point.isFinite()


def test_Point_equals(point):
    assert point.equals(4, 3)


def test_Point_cross(point):
    assert point.cross(point) == 0.0


def test_Point_dot(point):
    assert point.dot(point) == 25

    # skia.Point.Offset()
    # skia.Point.Offset()


def test_Point_Length():
    assert skia.Point.Length(4, 3) == 5


def test_Point_Normalize(point):
    assert skia.Point.Normalize(point) == 5


def test_Point_Distance(point):
    assert skia.Point.Distance(point, (0, 0)) == 5


def test_Point_DotProduct(point):
    assert skia.Point.DotProduct(point, point) == 25


def test_Point_CrossProduct(point):
    assert skia.Point.CrossProduct(point, point) == 0


def test_Point_fX_fY(point):
    point.fX = 4
    point.fY = 3
    assert point.fX == 4
    assert point.fY == 3


def test_Point_eq(point):
    assert point == skia.Point(4, 3)


def test_Point_ne(point):
    assert point != skia.Point(4, 0)


def test_Point_add(point):
    assert isinstance(point + point, skia.Point)


def test_Point_sub(point):
    assert isinstance(point - point, skia.Point)


def test_Point_iter(point):
    assert tuple(point) == (4, 3)


def test_Point_len(point):
    assert len(point) == 2


def test_Point_repr(point):
    assert isinstance(repr(point), str)



@pytest.fixture()
def point3():
    return skia.Point3(4, 3, 0)



@pytest.mark.parametrize('args', [
    (4, 3, 0),
    ((4, 3, 0),),
])
def test_Point3_init(args):
    assert isinstance(skia.Point3(*args), skia.Point3)


def test_Point3_x(point3):
    assert point3.x() == 4


def test_Point3_y(point3):
    assert point3.y() == 3


def test_Point3_z(point3):
    assert point3.z() == 0


def test_Point3_length(point3):
    assert point3.length() == 5


def test_Point3_normalize(point3):
    assert point3.normalize()


def test_Point3_set(point3):
    point3.set(4, 3, 0)


def test_Point3_makeScale(point3):
    assert point3.makeScale(1.0).x() == 4


def test_Point3_scale(point3):
    point3.scale(1.0)


def test_Point3_neg(point3):
    assert (-point3).x() == -4


def test_Point3_iadd(point3):
    point3 += skia.Point3(1, 1, 1)
    assert point3.x() == 5


def test_Point3_isub(point3):
    point3 -= skia.Point3(1, 1, 1)
    assert point3.x() == 3


def test_Point3_isFinite(point3):
    assert point3.isFinite()


def test_Point3_dot(point3):
    assert point3.dot(point3) == 25


def test_Point3_cross(point3):
    assert isinstance(point3.cross(point3), skia.Point3)


def test_Point3_Length(point3):
    assert skia.Point3.Length(3, 4, 0) == 5


def test_Point3_DotProduct(point3):
    assert skia.Point3.DotProduct(point3, point3) == 25


def test_Point3_CrossProduct(point3):
    assert isinstance(skia.Point3.CrossProduct(point3, point3), skia.Point3)


def test_Point3_fX_fY_fZ(point3):
    point3.fX = 4
    point3.fY = 3
    point3.fZ = 0
    assert point3.fX == 4
    assert point3.fY == 3
    assert point3.fZ == 0


def test_Point3_eq(point3):
    assert point3 == skia.Point3.Make(4, 3, 0)


def test_Point3_ne(point3):
    assert point3 != skia.Point3(4, 1, 1)


def test_Point3_add(point3):
    assert isinstance(point3 + point3, skia.Point3)


def test_Point3_sub(point3):
    assert isinstance(point3 - point3, skia.Point3)


def test_Point3_mul(point3):
    assert isinstance(1 * point3, skia.Point3)


def test_Point3_iter(point3):
    assert tuple(point3) == (4, 3, 0)


def test_Point3_len(point3):
    assert len(point3) == 3


def test_Point3_repr(point3):
    assert isinstance(repr(point3), str)

