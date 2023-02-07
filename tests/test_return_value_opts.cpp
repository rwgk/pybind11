#include <pybind11/stl.h>

#include "pybind11_tests.h"

#include <string>
#include <utility>

namespace {

using PairString = std::pair<std::string, std::string>;

PairString return_pair_string() { return PairString({"", ""}); }

using NestedPairString = std::pair<PairString, PairString>;

NestedPairString return_nested_pair_string() {
    return NestedPairString(return_pair_string(), return_pair_string());
}

} // namespace

TEST_SUBMODULE(return_value_opts, m) {
    auto rvpc = py::return_value_policy::_clif_automatic;
    auto rvpb = py::return_value_policy::_return_as_bytes;

    m.def("return_pair_str_str", []() { return return_pair_string(); });
    m.def(
        "return_pair_bytes_bytes", []() { return return_pair_string(); }, rvpb);
    m.def(
        "return_pair_str_bytes",
        []() { return return_pair_string(); },
        py::return_value_opts({rvpc, rvpb}));
    m.def(
        "return_pair_bytes_str",
        []() { return return_pair_string(); },
        py::return_value_opts({rvpb, rvpc}));

    m.def("return_nested_pair_str", []() { return return_nested_pair_string(); });
    m.def(
        "return_nested_pair_bytes", []() { return return_nested_pair_string(); }, rvpb);
    m.def(
        "return_nested_pair_str_bytes_bytes_str",
        []() { return return_nested_pair_string(); },
        py::return_value_opts({{rvpc, rvpb}, {rvpb, rvpc}}));
    m.def(
        "return_nested_pair_bytes_str_str_bytes",
        []() { return return_nested_pair_string(); },
        py::return_value_opts({{rvpb, rvpc}, {rvpc, rvpb}}));
}
