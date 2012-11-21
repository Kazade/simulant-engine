#include <unittest++/UnitTest++.h>
#include "kglt/partitioners/octree.h"
#include "kglt/types.h"

class Object :
    public Boundable {

public:
    Object(float width, float height, float depth) {
        kmVec3Zero(&centre_);
        kmAABBInitialize(&absolute_bounds_, nullptr, width, height, depth);
    }

    void set_bounds(float width, float height, float depth) {
        kmAABBInitialize(&absolute_bounds_,
            &centre_,
            width,
            height,
            depth
        );
    }

    void set_centre(const kmVec3& centre) {
        //Reinitialize the AABB with the same dimensions but a different
        //central point.
        kmVec3Assign(&centre_, &centre);
        kmAABBInitialize(&absolute_bounds_,
            &centre_,
            kmAABBDiameterX(&absolute_bounds_),
            kmAABBDiameterY(&absolute_bounds_),
            kmAABBDiameterZ(&absolute_bounds_)
        );
    }

    const kmAABB absolute_bounds() const {
        return absolute_bounds_;
    }

    const kmAABB local_bounds() const {
        kmAABB local;
        kmAABBInitialize(
            &local,
            nullptr,
            kmAABBDiameterX(&absolute_bounds_),
            kmAABBDiameterY(&absolute_bounds_),
            kmAABBDiameterZ(&absolute_bounds_)
        );
        return local;
    }

    const kmVec3 centre() const {
        return centre_;
    }
private:
    kmAABB absolute_bounds_;
    kmVec3 centre_;

};

SUITE(octree_tests) {
    TEST(test_insertion) {

        Octree tree;

        //Create an object 5 units high, centred a 10, 10, 10
        Object obj(2, 5, 2);
        obj.set_centre(kglt::Vec3(10, 10, 10));

        tree.grow(&obj);

        // The root node should have a strict diameter matching
        // the largest dimension of the object
        CHECK_EQUAL(5, tree.root().strict_diameter());
        CHECK_EQUAL(10, tree.root().loose_diameter());

        kmVec3 root_centre = tree.root().centre();
        kmVec3 obj_centre = obj.centre();
        CHECK(kmVec3AreEqual(&root_centre, &obj_centre));

        Object obj2(3, 3, 3); //Add a smaller object
        obj2.set_centre(kglt::Vec3(10, 10, 17));

        tree.grow(&obj2);
        /* This object was positioned outside the loose bounds of the
         * first one. This should have grown the tree upwards, the root
         * should now have 2 child nodes. The original root, and a new
         * child containing obj2
         *
         * The original root went from 7.5 to 12.5 on each axis (centred at 10, 10, 10)
         *
         * The new root should go from 7.5 to 17.5 on the z-axis
        */        

        CHECK_EQUAL(10, tree.root().strict_diameter());
        CHECK_EQUAL(20, tree.root().loose_diameter());
        CHECK_EQUAL(2, (uint32_t) tree.root().child_count());
        CHECK_CLOSE(12.5, tree.root().centre().z, 0.001);

        //Neither object should be in the root node
        CHECK(!tree.find(&obj).is_root());
        CHECK(!tree.find(&obj2).is_root());

        //And, they shouldn't be in the same node either
        CHECK(&tree.find(&obj) != &tree.find(&obj2));

        CHECK_EQUAL(5, tree.find(&obj2).strict_diameter());
        CHECK_EQUAL(10, tree.find(&obj2).loose_diameter());

        kmVec3 expected_centre;
        kmVec3Fill(&expected_centre, 10, 10, 15);
        CHECK(kmVec3AreEqual(&expected_centre, &tree.find(&obj2).centre()));

        //Root shouldn't have objects
        CHECK(!tree.root().has_objects());

        /** Situation Now
         *
         *  ---------------
         *  |      |      |
         *  |  o1  |  NE  |
         *  --------------
         *  |  o2  |      |
         *  |      |  NE  |
         *  ---------------
         *
         *  NE == Doesn't exist
         */

    }
}
