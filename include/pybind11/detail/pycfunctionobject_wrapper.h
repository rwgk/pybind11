#pragma once

// A small, safe wrapper for PyCFunctionObject that lets pybind11 control
// __qualname__ (and intercept other attrs if desired) while delegating calls
// and descriptor behavior to the real builtin function.
//
// Design goals:
//  - Keep CPython's semantics for call + binding (tp_call, tp_descr_get) by
//    delegating to the inner PyCFunctionObject.
//  - Provide a read-only __qualname__ getter that returns a short/sanitized
//    string (stored on the wrapper).
//  - Forward all other attributes to the inner object via a getattro fallback.
//  - Be GC-safe and easy to integrate into pybind11 (helpers to wrap/unwrap).
//
// Integration pattern in pybind11:
//  - Create the inner with PyCFunction_NewEx(...).
//  - Compute the "short" qualname string from pybind11 metadata.
//  - Wrap: auto *wrapped = cfunc_wrapper_New(inner, qualname_unicode);
//  - Store 'wrapped' in the module/class dict instead of 'inner'.
//  - Where pybind11 needs the raw PyCFunctionObject (e.g. overload chaining),
//    call unwrap_cfunction_if_wrapper(func_obj) before PyCFunction_* access.

#include "pybind11/pytypes.h"

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

// ---------------------------------------------------------------------------
// Wrapper instance layout
// ---------------------------------------------------------------------------

struct cfunc_wrapper_PyObject {
    PyObject_HEAD
    PyObject *inner;     // Strong ref to a callable, normally a PyCFunctionObject (or bound variant)
    PyObject *qualname;  // Optional PyUnicode* override for __qualname__ (may be NULL)
};

// ---------------------------------------------------------------------------
// Forward decl for the heap type object
// ---------------------------------------------------------------------------

static PyTypeObject *cfunc_wrapper_PyTypeObject = nullptr;

// Small RAII helpers for error propagation with pybind11
inline void throw_error_if(bool cond) {
    if (cond) {
        throw error_already_set();
    }
}

// ---------------------------------------------------------------------------
// GC support
// ---------------------------------------------------------------------------

static int cfunc_wrapper_traverse(cfunc_wrapper_PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(self->inner);
    Py_VISIT(self->qualname);
    return 0;
}

static int cfunc_wrapper_clear(cfunc_wrapper_PyObject *self) {
    Py_CLEAR(self->inner);
    Py_CLEAR(self->qualname);
    return 0;
}

static void cfunc_wrapper_dealloc(cfunc_wrapper_PyObject *self) {
    PyObject_GC_UnTrack(self);
    (void) cfunc_wrapper_clear(self);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

// ---------------------------------------------------------------------------
// __call__: delegate to inner
// ---------------------------------------------------------------------------

static PyObject *cfunc_wrapper_call(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    auto *self = (cfunc_wrapper_PyObject *) self_obj;
    if (self->inner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "pybind11 cfunc wrapper: inner is NULL");
        return nullptr;
    }
    return PyObject_Call(self->inner, args, kwargs);
}

// ---------------------------------------------------------------------------
// __repr__ (optional, for debugging)
// ---------------------------------------------------------------------------

static PyObject *cfunc_wrapper_repr(cfunc_wrapper_PyObject *self) {
    if (self->qualname && PyUnicode_Check(self->qualname)) {
        return PyUnicode_FromFormat("<pybind11.cfunc %U>", self->qualname);
    }
    return PyUnicode_FromString("<pybind11.cfunc>");
}

// ---------------------------------------------------------------------------
// getset: expose __qualname__ (read-only); optionally forward __name__/__doc__
// ---------------------------------------------------------------------------

static PyObject *cfunc_wrapper_get_qualname(cfunc_wrapper_PyObject *self, void *) {
    if (self->qualname) {
        Py_INCREF(self->qualname);
        return self->qualname;
    }
    // Fallback: ask the inner object
    if (self->inner) {
        return PyObject_GetAttrString(self->inner, "__qualname__");
    }
    Py_RETURN_NONE;
}

static PyObject *cfunc_wrapper_get_name(cfunc_wrapper_PyObject *self, void *) {
    if (self->inner) {
        return PyObject_GetAttrString(self->inner, "__name__");
    }
    Py_RETURN_NONE;
}

static PyObject *cfunc_wrapper_get_doc(cfunc_wrapper_PyObject *self, void *) {
    if (self->inner) {
        return PyObject_GetAttrString(self->inner, "__doc__");
    }
    Py_RETURN_NONE;
}

// NOTE: setters intentionally left NULL → read-only presentation
static PyGetSetDef cfunc_wrapper_getset[] = {
    {(char *)"__qualname__", (getter)cfunc_wrapper_get_qualname, nullptr,
     (char *)"qualified name", nullptr},
    {(char *)"__name__",     (getter)cfunc_wrapper_get_name,     nullptr,
     (char *)"name", nullptr},
    {(char *)"__doc__",      (getter)cfunc_wrapper_get_doc,      nullptr,
     (char *)"docstring", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// ---------------------------------------------------------------------------
// tp_getattro: first try wrapper's own attributes, then fall back to inner
// ---------------------------------------------------------------------------

static PyObject *cfunc_wrapper_getattro(PyObject *self_obj, PyObject *name) {
    // First try attributes defined on the wrapper (getset above, type attributes)
    PyObject *res = PyObject_GenericGetAttr(self_obj, name);
    if (res != nullptr) {
        return res;
    }
    // Fallback to inner's attributes
    PyErr_Clear();
    auto *self = (cfunc_wrapper_PyObject *) self_obj;
    if (!self->inner) {
        PyErr_SetObject(PyExc_AttributeError, name);
        return nullptr;
    }
    return PyObject_GetAttr(self->inner, name);
}

// ---------------------------------------------------------------------------
// tp_descr_get: delegate binding to inner descr_get, then re-wrap the result
// ---------------------------------------------------------------------------

static PyObject *cfunc_wrapper_descr_get(PyObject *self_obj, PyObject *obj, PyObject *type) {
    auto *self = (cfunc_wrapper_PyObject *) self_obj;
    if (!self->inner) {
        Py_INCREF(self_obj);
        return self_obj;  // nothing else to do
    }

    // If the inner has a descriptor, use it (PyCFunction_Type does).
    PyTypeObject *inner_type = Py_TYPE(self->inner);
    if (inner_type->tp_descr_get) {
        PyObject *bound = inner_type->tp_descr_get(self->inner, obj, type);
        if (bound == nullptr) {
            return nullptr; // propagate
        }
        // If binding returned the same object (common), keep identity when possible.
        if (bound == self->inner) {
            Py_DECREF(bound);
            Py_INCREF(self_obj);
            return self_obj;
        }
        // Otherwise, wrap the newly bound callable with the same qualname.
        // Reuse the cached qualname (or NULL) exactly as this wrapper has.
        cfunc_wrapper_PyObject *wrap
            = (cfunc_wrapper_PyObject *) PyObject_GC_New(cfunc_wrapper_PyObject, cfunc_wrapper_PyTypeObject);
        if (!wrap) {
            Py_DECREF(bound);
            return nullptr;
        }
        wrap->inner = bound;  // take ownership of 'bound' ref
        wrap->qualname = self->qualname;
        Py_XINCREF(wrap->qualname);
        PyObject_GC_Track(wrap);
        return (PyObject *)wrap;
    }

    // No descriptor: default is to return self unchanged
    Py_INCREF(self_obj);
    return self_obj;
}

// ---------------------------------------------------------------------------
// Optional: hash forwarding (keeps sets/dicts behavior similar to inner)
// ---------------------------------------------------------------------------

static Py_hash_t cfunc_wrapper_hash(PyObject *self_obj) {
    auto *self = (cfunc_wrapper_PyObject *) self_obj;
    if (!self->inner) {
        // Fallback to object identity if inner is missing
        return _Py_HashPointer(self_obj);
    }
    return PyObject_Hash(self->inner);
}

// ---------------------------------------------------------------------------
// Type creation via PyType_FromSpec (heap type)
// ---------------------------------------------------------------------------

static PyType_Slot cfunc_wrapper_PyType_Slots[] = {
    {Py_tp_dealloc,  (void *) cfunc_wrapper_dealloc},
    {Py_tp_traverse, (void *) cfunc_wrapper_traverse},
    {Py_tp_clear,    (void *) cfunc_wrapper_clear},
    {Py_tp_call,     (void *) cfunc_wrapper_call},
    {Py_tp_repr,     (void *) cfunc_wrapper_repr},
    {Py_tp_getattro, (void *) cfunc_wrapper_getattro},
    {Py_tp_getset,   (void *) cfunc_wrapper_getset},
    {Py_tp_descr_get,(void *) cfunc_wrapper_descr_get},
    {Py_tp_hash,     (void *) cfunc_wrapper_hash},
    {0, 0}
};

static PyType_Spec cfunc_wrapper_PyType_Spec = {
    // tp_name: keep it internal/namespaced to avoid user confusion
    (char *)"pybind11_detail.cfunc_wrapper",
    sizeof(cfunc_wrapper_PyObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    cfunc_wrapper_PyType_Slots
};

inline void cfunc_wrapper_ensure_type() {
    if (cfunc_wrapper_PyTypeObject) {
        return;
    }
    PyObject *type_obj = PyType_FromSpec(&cfunc_wrapper_PyType_Spec);
    throw_error_if(type_obj == nullptr);
    cfunc_wrapper_PyTypeObject = (PyTypeObject *) type_obj;
}

// ---------------------------------------------------------------------------
// Public helpers (inline) for pybind11 integration
// ---------------------------------------------------------------------------

// Create a new wrapper instance. Steals no references from the caller; INCREFs both.
inline PyObject *cfunc_wrapper_New(PyObject *inner_callable, PyObject *qualname_unicode_or_null) {
    cfunc_wrapper_ensure_type();

    if (inner_callable == nullptr) {
        PyErr_SetString(PyExc_ValueError, "cfunc_wrapper_New: inner is NULL");
        return nullptr;
    }
    // N.B. We don't enforce PyCFunction_Check here, to allow wrapping bound variants too.
    cfunc_wrapper_PyObject *self
        = (cfunc_wrapper_PyObject *) PyObject_GC_New(cfunc_wrapper_PyObject, cfunc_wrapper_PyTypeObject);
    if (!self) {
        return nullptr;
    }
    Py_INCREF(inner_callable);
    self->inner = inner_callable;

    if (qualname_unicode_or_null && PyUnicode_Check(qualname_unicode_or_null)) {
        Py_INCREF(qualname_unicode_or_null);
        self->qualname = qualname_unicode_or_null;
    } else {
        self->qualname = nullptr; // the getter will fallback to inner.__qualname__
    }

    PyObject_GC_Track(self);
    return (PyObject *) self;
}

// Returns true if obj is a wrapper instance.
inline bool is_cfunc_wrapper(PyObject *obj) {
    cfunc_wrapper_ensure_type();
    return PyObject_TypeCheck(obj, cfunc_wrapper_PyTypeObject);
}

// If obj is a wrapper, return its inner; otherwise return obj (borrowed).
inline PyObject *unwrap_cfunction_if_wrapper(PyObject *obj) {
    if (obj && is_cfunc_wrapper(obj)) {
        return ((cfunc_wrapper_PyObject *) obj)->inner;
    }
    return obj;
}

// Convenience: get the wrapper type object (borrowed).
inline PyTypeObject *cfunc_wrapper_type() {
    cfunc_wrapper_ensure_type();
    return cfunc_wrapper_PyTypeObject;
}

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
