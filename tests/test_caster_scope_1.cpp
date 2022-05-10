#include "pybind11_tests.h"
#include "test_caster_scope.h"

namespace pybind11 {
namespace detail {
template <>
struct type_caster<test_caster_scope::atyp> : test_caster_scope::type_caster_atyp<1> {};
} // namespace detail
} // namespace pybind11

TEST_SUBMODULE(caster_scope_1, m) {
    using namespace test_caster_scope;

    m.def("rtrn_bval", rtrn_bval);
    m.def("rtrn_rref", rtrn_rref);
    m.def("rtrn_cref", rtrn_cref);
    m.def("rtrn_mref", rtrn_mref);
    m.def("rtrn_cptr", rtrn_cptr);
    m.def("rtrn_mptr", rtrn_mptr);

    m.def("pass_bval", pass_bval);
    m.def("pass_rref", pass_rref);
    m.def("pass_cref", pass_cref);
    m.def("pass_mref", pass_mref);
    m.def("pass_cptr", pass_cptr);
    m.def("pass_mptr", pass_mptr);
}
