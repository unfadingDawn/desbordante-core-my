#include "get_py_type.h"

#include <functional>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <Python.h>
#include <pybind11/stl/filesystem.h>

#include "algorithms/cfd/enums.h"
#include "algorithms/dd/dd.h"
#include "algorithms/md/hymd/enums.h"
#include "algorithms/md/hymd/hymd.h"
#include "algorithms/metric/enums.h"
#include "association_rules/ar_algorithm_enums.h"
#include "config/custom_random_seed/type.h"
#include "config/error_measure/type.h"
#include "config/tabular_data/input_table_type.h"
#include "config/tabular_data/input_tables_type.h"
#include "model/table/column_combination.h"

namespace py = pybind11;

namespace {

constexpr PyTypeObject* const kPyInt = &PyLong_Type;
constexpr PyTypeObject* const kPyBool = &PyBool_Type;
constexpr PyTypeObject* const kPyFloat = &PyFloat_Type;
constexpr PyTypeObject* const kPyStr = &PyUnicode_Type;
constexpr PyTypeObject* const kPyList = &PyList_Type;
constexpr PyTypeObject* const kPyTuple = &PyTuple_Type;
constexpr PyTypeObject* const kPySet = &PySet_Type;

py::handle MakeType(py::type type) {
    return type;
}

py::handle MakeType(PyTypeObject* py_type_ptr) {
    // Does the same as `&py_type_ptr->ob_base.ob_base`: Python API simulates
    // inheritance here. There is also the `_PyObject_CAST` macro that does a
    // C-style cast, but, as indicated by the underscore, it's an internal
    // Python macro, so it should be avoided.
    return reinterpret_cast<PyObject*>(py_type_ptr);
}

[[maybe_unused]] py::handle MakeType(PyObject* py_type_ptr) {
    return py_type_ptr;
}

template <typename... TypePtrs>
py::tuple MakeTypeTuple(TypePtrs... type_ptrs) {
    return py::make_tuple(MakeType(type_ptrs)...);
}

template <typename CppType, PyTypeObject*... PyTypes>
std::pair<std::type_index, std::function<py::tuple()>> const PyTypePair{
        std::type_index{typeid(CppType)}, []() { return MakeTypeTuple(PyTypes...); }};

}  // namespace

namespace python_bindings {

py::tuple GetPyType(std::type_index type_index) {
    // Type indexes are mapped to Python type tuples. The first element of all
    // the tuples is the type of the parameter itself. If that element is
    // `list`, the following element denotes the type of its elements. If that
    // parameter is `tuple` then the following parameters denote the types of
    // their respective elements.
    // The tuples are created at runtime from Python API's raw pointers (when
    // possible) as storing pybind11's objects themselves statically is
    // unpredictable and can lead to errors related to garbage collection.
    static std::unordered_map<std::type_index, std::function<py::tuple()>> const type_map{
            PyTypePair<bool, kPyBool>,
            PyTypePair<unsigned short, kPyInt>,
            PyTypePair<int, kPyInt>,
            PyTypePair<unsigned int, kPyInt>,
            PyTypePair<double, kPyFloat>,
            PyTypePair<size_t, kPyInt>,
            PyTypePair<long double, kPyFloat>,
            PyTypePair<std::size_t, kPyInt>,
            PyTypePair<config::CustomRandomSeedType, kPyInt>,
            PyTypePair<algos::metric::Metric, kPyStr>,
            PyTypePair<algos::metric::MetricAlgo, kPyStr>,
            PyTypePair<config::PfdErrorMeasureType, kPyStr>,
            PyTypePair<config::AfdErrorMeasureType, kPyStr>,
            PyTypePair<algos::InputFormat, kPyStr>,
            PyTypePair<algos::cfd::Substrategy, kPyStr>,
            PyTypePair<algos::hymd::LevelDefinition, kPyStr>,
            PyTypePair<std::vector<unsigned int>, kPyList, kPyInt>,
            {typeid(algos::hymd::HyMD::ColumnMatches),
             []() {
                 return MakeTypeTuple(
                         kPyList,
                         py::type::of<algos::hymd::preprocessing::column_matches::ColumnMatch>());
             }},
            {typeid(model::DDString),
             []() { return MakeTypeTuple(kPyList, py::type::of<model::DFStringConstraint>()); }},
            {typeid(config::InputTable),
             []() { return MakeTypeTuple(py::type::of<config::InputTable>()); }},
            {typeid(config::InputTables),
             []() { return MakeTypeTuple(kPyList, py::type::of<config::InputTable>()); }},
            PyTypePair<std::filesystem::path, kPyStr>,
            PyTypePair<std::vector<std::filesystem::path>, kPyList, kPyStr>,
            PyTypePair<std::unordered_set<size_t>, kPySet, kPyInt>,
            PyTypePair<std::string, kPyStr>,
    };
    return type_map.at(type_index)();
}

}  // namespace python_bindings
