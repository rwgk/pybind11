import re

import pytest

from pybind11_tests import caster_scope_2 as m


@pytest.mark.parametrize(
    "rtrn_f, expected",
    [
        (m.rtrn_bval, "2cast_rref:bval_CpCtor"),
        (m.rtrn_rref, "2cast_rref:rref"),
        (m.rtrn_cref, "2cast_cref:cref"),
        (m.rtrn_mref, "2cast_mref:mref"),
        (m.rtrn_cptr, "2cast_cptr:cptr"),
        (m.rtrn_mptr, "2cast_mptr:mptr"),
    ],
)
def test_cast(rtrn_f, expected):
    assert re.match(expected, rtrn_f())


@pytest.mark.parametrize(
    "pass_f, expected",
    [
        (m.pass_bval, "pass_bval:2rref_MvCtor"),
        (m.pass_rref, "pass_rref:2rref"),
        (m.pass_cref, "pass_cref:2cref"),
        (m.pass_mref, "pass_mref:2mref"),
        (m.pass_cptr, "pass_cptr:2cptr"),
        (m.pass_mptr, "pass_mptr:2mptr"),
    ],
)
def test_operator(pass_f, expected):
    assert re.match(expected, pass_f(None))
