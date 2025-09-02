#include "pybind11_tests.h"

#include <memory>

namespace pybind11_tests {
namespace class_animal {

class Animal {
public:
    Animal() = default;
    Animal(const Animal &) = default;
    Animal &operator=(const Animal &) = default;
    virtual std::shared_ptr<Animal> clone() const = 0;
    virtual ~Animal() = default;
};

class Cat : virtual public Animal {
public:
    Cat() = default;
    Cat(const Cat &) = default;
    Cat &operator=(const Cat &) = default;
    ~Cat() override = default;
};

class Tiger : virtual public Cat {
public:
    Tiger() = default;
    Tiger(const Tiger &) = default;
    Tiger &operator=(const Tiger &) = default;
    ~Tiger() override = default;
    std::shared_ptr<Animal> clone() const override {
        std::shared_ptr<Tiger> sp_tiger = std::make_shared<Tiger>(*this);
        fflush(stderr); fprintf(stdout, "\nLOOOK _sp_tiger=PTR0x%016llu %s:%d\n", (unsigned long long) sp_tiger.get(), __FILE__, __LINE__); fflush(stdout);
        std::shared_ptr<Animal> sp_animal = sp_tiger;
        fflush(stderr); fprintf(stdout, "\nLOOOK sp_animal=PTR0x%016llu %s:%d\n", (unsigned long long) sp_animal.get(), __FILE__, __LINE__); fflush(stdout);
        return sp_animal;
    }
};

TEST_SUBMODULE(class_animal, m) {
    namespace py = pybind11;

    py::class_<Animal, py::smart_holder>(m, "Animal");

    py::class_<Cat, Animal, py::smart_holder>(m, "Cat");

    py::class_<Tiger, Cat, py::smart_holder>(m, "Tiger", py::multiple_inheritance())
        .def(py::init<>())
        .def("clone", &Tiger::clone);
}

} // namespace class_animal
} // namespace pybind11_tests
