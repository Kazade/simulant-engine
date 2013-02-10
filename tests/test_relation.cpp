#include <UnitTest++.h>
#include <tr1/memory>

#include "kglt/generic/relation.h"

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

TEST(test_relations_work) {
    A a1, a2, a3;
    
    CHECK(!a1.b.get());
    CHECK(!a2.b.get());
    CHECK(!a3.b.get());    

    B b1;
    
    a1.b = b1;
    CHECK_EQUAL(&b1, a1.b.get());
    CHECK_EQUAL(1, b1.a_set.all().size());
    CHECK_EQUAL(&a1, b1.a_set.all().at(0));
    CHECK(!a1.c.get());

    CHECK_EQUAL(&a1.b(), &b1);

    a2.b = b1;
    CHECK_EQUAL(&b1, a2.b.get());
    CHECK_EQUAL(2, b1.a_set.all().size());

    a3.b = b1;
    CHECK_EQUAL(&b1, a3.b.get());
    CHECK_EQUAL(3, b1.a_set.all().size());

    std::tr1::shared_ptr<C> new_c(new C());
    a1.c = *new_c;
    CHECK(a1.c.get());
    new_c.reset();
    CHECK(!a1.c.get());

}

