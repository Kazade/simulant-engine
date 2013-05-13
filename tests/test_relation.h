#ifndef TEST_RELATION_H
#define TEST_RELATION_H

#include "kglt/kazbase/testing.h"

#include "kglt/kglt.h"
#include "global.h"

namespace relation_test {

using namespace kglt;

class B;
class C;

class A : public Relatable {
public:
    Relation<A, B> b;
    Relation<A, C> c;

    A():
        b(this),
        c(this) {
    }

    ~A() {}

};

class B : public Relatable {
public:
    ReverseRelation<B, A> a_set;

    B():
        a_set(this) {

    }
};

class C : public Relatable {
public:
    ReverseRelation<C, A> a_set;

    C():
        a_set(this) {

    }

};

}

class RelationTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        }

        //window->reset();
    }


    void test_relations_work() {
        using namespace relation_test;

        A a1, a2, a3;

        assert_false(a1.b.get());
        assert_false(a2.b.get());
        assert_false(a3.b.get());

        B b1;

        a1.b = b1;
        assert_equal(&b1, a1.b.get());
        assert_equal((uint32_t)1, b1.a_set.all().size());
        assert_equal(&a1, b1.a_set.all().at(0));
        assert_false(a1.c.get());

        assert_equal(&a1.b(), &b1);

        a2.b = b1;
        assert_equal(&b1, a2.b.get());
        assert_equal((uint32_t)2, b1.a_set.all().size());

        a3.b = b1;
        assert_equal(&b1, a3.b.get());
        assert_equal((uint32_t)3, b1.a_set.all().size());

        std::tr1::shared_ptr<C> new_c(new C());
        a1.c = *new_c;
        assert_true(a1.c.get());
        new_c.reset();
        assert_false(a1.c.get());

    }
};

#endif // TEST_RELATION_H
