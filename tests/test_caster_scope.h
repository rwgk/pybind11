#include "pybind11/pybind11.h"

#include <string>

namespace test_caster_scope {

struct atyp { // Short for "any type".
    std::string trace;
    atyp() : trace("default") {}
    atyp(atyp const &other) { trace = other.trace + "_CpCtor"; }
    atyp(atyp &&other) { trace = other.trace + "_MvCtor"; }
};

// clang-format off

inline atyp        rtrn_bval() { static atyp obj; obj.trace = "bval"; return obj; }
inline atyp&&      rtrn_rref() { static atyp obj; obj.trace = "rref"; return std::move(obj); }
inline atyp const& rtrn_cref() { static atyp obj; obj.trace = "cref"; return obj; }
inline atyp&       rtrn_mref() { static atyp obj; obj.trace = "mref"; return obj; }
inline atyp const* rtrn_cptr() { static atyp obj; obj.trace = "cptr"; return &obj; }
inline atyp*       rtrn_mptr() { static atyp obj; obj.trace = "mptr"; return &obj; }

inline std::string pass_bval(atyp obj)        { return "pass_bval:" + obj.trace; }
inline std::string pass_rref(atyp&& obj)      { return "pass_rref:" + obj.trace; }
inline std::string pass_cref(atyp const& obj) { return "pass_cref:" + obj.trace; }
inline std::string pass_mref(atyp& obj)       { return "pass_mref:" + obj.trace; }
inline std::string pass_cptr(atyp const* obj) { return "pass_cptr:" + obj->trace; }
inline std::string pass_mptr(atyp* obj)       { return "pass_mptr:" + obj->trace; }

// clang-format on

namespace py = pybind11;

template <int Serial>
struct caster_name;
template <>
struct caster_name<1> {
    static constexpr auto value = py::detail::const_name("atyp_caster1");
};
template <>
struct caster_name<2> {
    static constexpr auto value = py::detail::const_name("atyp_caster2");
};

template <int Serial>
struct type_caster_atyp {
    static constexpr auto name = caster_name<1>::value;

    static std::string ss() { return std::to_string(Serial); }

    static py::handle cast(atyp &&src, py::return_value_policy /*policy*/, py::handle /*parent*/) {
        return py::str(ss() + "cast_rref:" + src.trace).release();
    }

    static py::handle
    cast(atyp const &src, py::return_value_policy /*policy*/, py::handle /*parent*/) {
        return py::str(ss() + "cast_cref:" + src.trace).release();
    }

    static py::handle cast(atyp &src, py::return_value_policy /*policy*/, py::handle /*parent*/) {
        return py::str(ss() + "cast_mref:" + src.trace).release();
    }

    static py::handle
    cast(atyp const *src, py::return_value_policy /*policy*/, py::handle /*parent*/) {
        return py::str(ss() + "cast_cptr:" + src->trace).release();
    }

    static py::handle cast(atyp *src, py::return_value_policy /*policy*/, py::handle /*parent*/) {
        return py::str(ss() + "cast_mptr:" + src->trace).release();
    }

    template <typename T_>
    using cast_op_type = py::detail::conditional_t<
        std::is_same<py::detail::remove_reference_t<T_>, atyp const *>::value,
        atyp const *,
        py::detail::conditional_t<
            std::is_same<py::detail::remove_reference_t<T_>, atyp *>::value,
            atyp *,
            py::detail::conditional_t<
                std::is_same<T_, atyp const &>::value,
                atyp const &,
                py::detail::conditional_t<
                    std::is_same<T_, atyp &>::value,
                    atyp &,
                    py::detail::conditional_t<std::is_same<T_, atyp &&>::value, atyp &&, atyp>>>>>;

    // clang-format off

    operator atyp()        { static atyp obj; obj.trace = ss() + "bval"; return obj; }
    operator atyp&&()      { static atyp obj; obj.trace = ss() + "rref"; return std::move(obj); }
    operator atyp const&() { static atyp obj; obj.trace = ss() + "cref"; return obj; }
    operator atyp&()       { static atyp obj; obj.trace = ss() + "mref"; return obj; }
    operator atyp const*() { static atyp obj; obj.trace = ss() + "cptr"; return &obj; }
    operator atyp*()       { static atyp obj; obj.trace = ss() + "mptr"; return &obj; }

    // clang-format on

    // The entire load logic is intentionally omitted.
    bool load(py::handle /*src*/, bool /*convert*/) { return true; }
};

} // namespace test_caster_scope
