/*
    pybind11/functional.h: std::function<> support

    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "pybind11.h"

#include <functional>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

template <typename Return, typename... Args>
struct type_caster<std::function<Return(Args...)>> {
    using type = std::function<Return(Args...)>;
    using retval_type = conditional_t<std::is_same<Return, void>::value, void_type, Return>;
    using function_type = Return (*)(Args...);

public:
    bool load(handle src, from_python_policies fpp) {
  printf("\nLOOOK type_caster-std::function-load %s:%d\n", __FILE__, __LINE__); fflush(stdout);
  printf("\nLOOOK                      policy %d  %s:%d\n", int(fpp.rvpp.policy), __FILE__, __LINE__); fflush(stdout);
  printf("\nLOOOK             vec_rvpp.size() %d  %s:%d\n", int(fpp.rvpp.vec_rvpp.size()), __FILE__, __LINE__); fflush(stdout);
#ifdef JUNK
  if (int(fpp.rvpp.policy) == 0) {
    long *BAD = nullptr;
    *BAD = 101;
  }
#endif
        if (src.is_none()) {
            // Defer accepting None to other overloads (if we aren't in convert mode):
            if (!fpp.convert) {
                return false;
            }
            return true;
        }

        if (!isinstance<function>(src)) {
            return false;
        }

        auto func = reinterpret_borrow<function>(src);

        /*
           When passing a C++ function as an argument to another C++
           function via Python, every function call would normally involve
           a full C++ -> Python -> C++ roundtrip, which can be prohibitive.
           Here, we try to at least detect the case where the function is
           stateless (i.e. function pointer or lambda function without
           captured variables), in which case the roundtrip can be avoided.
         */
        if (auto cfunc = func.cpp_function()) {
            auto *cfunc_self = PyCFunction_GET_SELF(cfunc.ptr());
            if (cfunc_self == nullptr) {
                PyErr_Clear();
            } else if (isinstance<capsule>(cfunc_self)) {
                auto c = reinterpret_borrow<capsule>(cfunc_self);

                function_record *rec = nullptr;
                // Check that we can safely reinterpret the capsule into a function_record
                if (detail::is_function_record_capsule(c)) {
                    rec = c.get_pointer<function_record>();
                }

                while (rec != nullptr) {
                    if (rec->is_stateless
                        && same_type(typeid(function_type),
                                     *reinterpret_cast<const std::type_info *>(rec->data[1]))) {
                        struct capture {
                            function_type f;
                        };
                        value = ((capture *) &rec->data)->f;
                        return true;
                    }
                    rec = rec->next;
                }
            }
            // PYPY segfaults here when passing builtin function like sum.
            // Raising an fail exception here works to prevent the segfault, but only on gcc.
            // See PR #1413 for full details
        }

        // ensure GIL is held during functor destruction
        struct func_handle {
            function f;
#if !(defined(_MSC_VER) && _MSC_VER == 1916 && defined(PYBIND11_CPP17))
            // This triggers a syntax error under very special conditions (very weird indeed).
            explicit
#endif
                func_handle(function &&f_) noexcept
                : f(std::move(f_)) {
            }
            func_handle(const func_handle &f_) { operator=(f_); }
            func_handle &operator=(const func_handle &f_) {
                gil_scoped_acquire acq;
                f = f_.f;
                return *this;
            }
            ~func_handle() {
                gil_scoped_acquire acq;
                function kill_f(std::move(f));
            }
        };

        // to emulate 'move initialization capture' in C++11
        struct func_wrapper {
            func_handle hfunc;
            return_value_policy_pack rvpp;
            func_wrapper(func_handle &&hf, const return_value_policy_pack &rvpp) noexcept
                : hfunc(std::move(hf)), rvpp(rvpp) {}
            Return operator()(Args... args) const {
                gil_scoped_acquire acq;
                // casts the returned object as a rvalue to the return type
                return hfunc.f.call_with_policies(rvpp, std::forward<Args>(args)...)
                    .template cast<Return>();
            }
        };

        value = func_wrapper(func_handle(std::move(func)), fpp.rvpp);
        return true;
    }

    template <typename Func>
    static handle cast(Func &&f_, return_value_policy_pack rvpp, handle /* parent */) {
  printf("\nLOOOK callable-to-python %s:%d\n", __FILE__, __LINE__); fflush(stdout);
  printf("\nLOOOK             policy %d  %s:%d\n", int(rvpp.policy), __FILE__, __LINE__); fflush(stdout);
  printf("\nLOOOK    vec_rvpp.size() %d  %s:%d\n", int(rvpp.vec_rvpp.size()), __FILE__, __LINE__); fflush(stdout);
        if (!f_) {
            return none().release();
        }

        auto result = f_.template target<function_type>();
        if (result) {
  printf("\nLOOOK              CASE1 ENTR %s:%d\n", __FILE__, __LINE__); fflush(stdout);
            auto retval = cpp_function(*result, rvpp).release();
  printf("\nLOOOK              CASE1 EXIT %s:%d\n", __FILE__, __LINE__); fflush(stdout);
            return retval;
        }
  printf("\nLOOOK              CASE2 %s:%d\n", __FILE__, __LINE__); fflush(stdout);
        return cpp_function(std::forward<Func>(f_), rvpp).release();
    }

    PYBIND11_TYPE_CASTER(type,
                         const_name("Callable[[") + concat(make_caster<Args>::name...)
                             + const_name("], ") + make_caster<retval_type>::name
                             + const_name("]"));
};

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
