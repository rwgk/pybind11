import re

import pytest

from pybind11_tests import caster_scope_1 as m


@pytest.mark.parametrize(
    "rtrn_f, expected",
    [
        (m.rtrn_bval, "1cast_rref:bval_CpCtor"),
        (m.rtrn_rref, "1cast_rref:rref"),
        (m.rtrn_cref, "1cast_cref:cref"),
        (m.rtrn_mref, "1cast_mref:mref"),
        (m.rtrn_cptr, "1cast_cptr:cptr"),
        (m.rtrn_mptr, "1cast_mptr:mptr"),
    ],
)
def test_cast(rtrn_f, expected):
    assert re.match(expected, rtrn_f())


@pytest.mark.parametrize(
    "pass_f, expected",
    [
        (m.pass_bval, "pass_bval:1rref_MvCtor"),
        (m.pass_rref, "pass_rref:1rref"),
        (m.pass_cref, "pass_cref:1cref"),
        (m.pass_mref, "pass_mref:1mref"),
        (m.pass_cptr, "pass_cptr:1cptr"),
        (m.pass_mptr, "pass_mptr:1mptr"),
    ],
)
def test_operator(pass_f, expected):
    assert re.match(expected, pass_f(None))
