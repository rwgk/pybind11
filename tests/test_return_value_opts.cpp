#include <pybind11/stl.h>

#include "pybind11_tests.h"

#include <string>
#include <utility>

namespace {
std::pair<std::string, std::string> return_pair_string() {
    return std::pair<std::string, std::string>({"", ""});
}
} // namespace

TEST_SUBMODULE(return_value_opts, m) {
    m.def("return_pair_str_str", []() { return return_pair_string(); });
    m.def(
        "return_pair_bytes_bytes",
        []() { return return_pair_string(); },
        py::return_value_policy::_return_as_bytes);
    m.def(
        "return_pair_str_bytes",
        []() { return return_pair_string(); },
        py::return_value_opts({py::return_value_policy::_clif_automatic,
                               py::return_value_policy::_return_as_bytes}));
    m.def(
        "return_pair_bytes_str",
        []() { return return_pair_string(); },
        py::return_value_opts({py::return_value_policy::_return_as_bytes,
                               py::return_value_policy::_clif_automatic}));
}
