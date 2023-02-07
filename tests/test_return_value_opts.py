import pytest

from pybind11_tests import return_value_opts as m


@pytest.mark.parametrize(
    "func, expected",
    [
        (m.return_pair_str_str, (str, str)),
        (m.return_pair_bytes_bytes, (bytes, bytes)),
        (m.return_pair_str_bytes, (str, bytes)),
        (m.return_pair_bytes_str, (bytes, str)),
    ],
)
def test_return_pair_string(func, expected):
    ps = func()
    assert all(isinstance(e, t) for e, t in zip(ps, expected))


@pytest.mark.parametrize(
    "func, expected",
    [
        (m.return_nested_pair_str, (str, str, str, str)),
        (m.return_nested_pair_bytes, (bytes, bytes, bytes, bytes)),
        (m.return_nested_pair_str_bytes_bytes_str, (str, bytes, bytes, str)),
        (m.return_nested_pair_bytes_str_str_bytes, (bytes, str, str, bytes)),
    ],
)
def test_return_nested_pair_string(func, expected):
    nps = func()
    assert len(nps) == 2
    assert all(isinstance(e, t) for e, t in zip(nps[0] + nps[1], expected))
