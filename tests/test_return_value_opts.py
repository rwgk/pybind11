import pytest

from pybind11_tests import return_value_opts as m


@pytest.mark.parametrize(
    "func, expected",
    [
        (m.return_pair_str_str, (str, str)),
        (m.return_pair_bytes_bytes, (bytes, bytes)),
        (m.return_pair_str_bytes, (str, bytes)),
    ],
)
def test_return_pair_bytes_bytes(func, expected):
    tup = func()
    assert all(isinstance(e, t) for e, t in zip(tup, expected))
