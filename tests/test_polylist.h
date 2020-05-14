#pragma once

#include "../simulant/test.h"
#include "../simulant/generic/containers/polylist.h"

namespace {

class A {
public:
    virtual ~A() {}
};

class B : public A {
public:
    B() {}

    B(std::function<void()> func) {
        func();
    }
};

class C : public A {
public:
    C() {}
    C(std::function<void()> func) {
        func();
    }
};

class D : public A {};

class E : public A {
public:
    E(const std::string& name):
        name_(name) {}

    std::string name_;
};

using namespace smlt;

typedef Polylist<A, B, C> List;

class PolylistTests : public smlt::test::SimulantTestCase {
public:
    void test_reserve() {
        List list(64);

        assert_equal(0u, list.capacity());

        assert_equal(0u, list.size());
        list.reserve(8);

        assert_equal(64u, list.capacity());

        list.reserve(65);

        assert_equal(0u, list.size());
        assert_equal(128u, list.capacity());

        list.create<B>();

        assert_equal(1u, list.size());
        assert_equal(128u, list.capacity());
    }

    void test_create() {
        List list(10);

        bool b_called = false, c_called = false;

        auto b = list.create<B>([&]() {b_called = true;});
        assert_true(b_called);

        auto c = list.create<C>([&]() {c_called = true;});
        assert_true(c_called);

        assert_equal(b.second, 1u);
        assert_equal(c.second, 2u);

        assert_equal(list.size(), 2u);
        assert_equal(list.capacity(), 10u);
    }

    void test_erase() {
        List list(2);

        auto p1 = list.create<C>();
        auto p2 = list.create<C>();
        auto p3 = list.create<C>();
        auto p4 = list.create<C>();
        auto p5 = list.create<C>();
        auto p6 = list.create<C>();

        assert_equal(list.size(), 6u);

        list.erase(p1.first);
        assert_equal(list.size(), 5u);
        assert_is_null(list[p1.second]);

        list.erase(p2.first);
        assert_equal(list.size(), 4u);
        assert_is_null(list[p2.second]);

        list.erase(p3.first);
        assert_equal(list.size(), 3u);
        assert_is_null(list[p3.second]);

        list.erase(p4.first);
        assert_equal(list.size(), 2u);
        assert_is_null(list[p4.second]);

        list.erase(p5.first);
        assert_equal(list.size(), 1u);
        assert_is_null(list[p5.second]);

        list.erase(p6.first);
        assert_equal(list.size(), 0u);
        assert_is_null(list[p6.second]);
    }

    void test_iteration() {
        List list(2);

        list.create<C>();
        list.create<C>();
        list.create<C>();
        list.create<C>();
        list.create<C>();
        list.create<C>();

        std::size_t i = 0;
        for(auto& entry: list) {
            _S_UNUSED(entry);
            ++i;
        }

        assert_equal(i, 6u);
    }

    void test_index_operator() {
        List list(2);

        list.create<E>("one");
        list.create<E>("two");
        auto p = list.create<E>("three");

        E* obj = dynamic_cast<E*>(list[p.second]);

        assert_equal(obj->name_, "three");
    }
};


}
