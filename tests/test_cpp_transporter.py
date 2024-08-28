from __future__ import annotations

import exo_planet

from pybind11_tests import cpp_transporter as home_planet


def test_home_only():
    t_h = home_planet.Traveler("home")
    assert t_h.luggage == "home"
    assert home_planet.get_luggage(t_h) == "home"


def test_exo_only():
    t_e = exo_planet.Traveler("exo")
    assert t_e.luggage == "exo"
    assert exo_planet.get_luggage(t_e) == "exo"


def test_home_passed_to_exo():
    t_h = home_planet.Traveler("home")
    assert exo_planet.get_luggage(t_h) == "home"


def test_exo_passed_to_home():
    t_e = exo_planet.Traveler("exo")
    assert home_planet.get_luggage(t_e) == "exo"


def test_call_cpp_transporter_success():
    t_h = home_planet.Traveler("home")
    cap = t_h.__cpp_transporter__(
        home_planet.PYBIND11_PLATFORM_ABI_ID,
        home_planet.typeid_Traveler_name,
        "raw_pointer_ephemeral",
    )
    assert cap.__class__.__name__ == "PyCapsule"


def test_call_cpp_transporter_platform_abi_id_mismatch():
    t_h = home_planet.Traveler("home")
    cap = t_h.__cpp_transporter__(
        home_planet.PYBIND11_PLATFORM_ABI_ID + "MISMATCH",
        home_planet.typeid_Traveler_name,
        "raw_pointer_ephemeral",
    )
    assert cap is None
    diag = t_h.__cpp_transporter__(
        home_planet.PYBIND11_PLATFORM_ABI_ID + "MISMATCH",
        home_planet.typeid_Traveler_name,
        "query_mismatch",
    )
    assert diag == "pybind11_platform_abi_id_mismatch"


def test_call_cpp_transporter_type_id_name_mismatch():
    t_h = home_planet.Traveler("home")
    cap = t_h.__cpp_transporter__(
        home_planet.PYBIND11_PLATFORM_ABI_ID,
        home_planet.typeid_Traveler_name + "MISMATCH",
        "raw_pointer_ephemeral",
    )
    assert cap is None
    diag = t_h.__cpp_transporter__(
        home_planet.PYBIND11_PLATFORM_ABI_ID,
        home_planet.typeid_Traveler_name + "MISMATCH",
        "query_mismatch",
    )
    assert diag == "cpp_typeid_name_mismatch"
