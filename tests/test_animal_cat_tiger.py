from __future__ import annotations

from pybind11_tests import class_animal as m

import pytest


@pytest.mark.parametrize("tiger_type", [m.TigerSP, m.TigerSH])
def test_with_smart_holder(tiger_type):
    print(f"\nLOOOK {tiger_type=!r}", flush=True)
    tiger = tiger_type()
    print(f"\nLOOOK {tiger=!r}", flush=True)
    cloned = tiger.clone()
    print(f"\nLOOOK {cloned=!r}", flush=True)
