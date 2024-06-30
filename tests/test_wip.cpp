#include "pybind11_tests.h"

namespace pybind11_tests {
namespace wip {

struct SomeType {};

} // namespace wip
} // namespace pybind11_tests

TEST_SUBMODULE(wip, m) {
    m.attr("__doc__") = "WIP";

    using namespace pybind11_tests::wip;

    py::class_<SomeType>(m, "SomeType").def(py::init([]() {
        return std::unique_ptr<SomeType>(new SomeType);
    }));

    m.def("make_some_type", []() { return std::unique_ptr<SomeType>(new SomeType); });
}
