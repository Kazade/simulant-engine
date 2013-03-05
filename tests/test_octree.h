#ifndef TEST_OCTREE_H
#define TEST_OCTREE_H

#include "kglt/kazbase/testing.h"

#include "kglt/kglt.h"
#include "global.h"

#include "kglt/partitioners/octree.h"
#include "kglt/types.h"

class OctreeTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_moving_objects() {
        kglt::Octree tree;

        Object obj(10, 10, 10);
        obj.set_centre(kglt::Vec3(0, 0, 0));

        tree.grow(&obj); //Insert the obj

        kglt::Vec3 expected_centre(0, 0, 0);
        assert_true(kmVec3AreEqual(&tree.root().centre(), &expected_centre));
        assert_equal(10, tree.root().strict_diameter());
        assert_equal(20, tree.root().loose_diameter());

        //CONTINUE: write tests for relocate
    }

    void test_insertion() {

        kglt::Octree tree;

        //Create an object 5 units high, centred a 10, 10, 10
        Object obj(2, 5, 2);
        obj.set_centre(kglt::Vec3(10, 10, 10));

        tree.grow(&obj);

        // The root node should have a strict diameter matching
        // the largest dimension of the object
        assert_equal(5, tree.root().strict_diameter());
        assert_equal(10, tree.root().loose_diameter());

        kmVec3 root_centre = tree.root().centre();
        kmVec3 obj_centre = obj.centre();
        assert_true(kmVec3AreEqual(&root_centre, &obj_centre));

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

        assert_equal(10, tree.root().strict_diameter());
        assert_equal(20, tree.root().loose_diameter());
        assert_equal((uint32_t)2, (uint32_t) tree.root().child_count());
        assert_close(12.5, tree.root().centre().z, 0.001);

        //Neither object should be in the root node
        assert_true(!tree.find(&obj).is_root());
        assert_true(!tree.find(&obj2).is_root());

        //And, they shouldn't be in the same node either
        assert_true(&tree.find(&obj) != &tree.find(&obj2));

        assert_equal(5, tree.find(&obj2).strict_diameter());
        assert_equal(10, tree.find(&obj2).loose_diameter());

        kmVec3 expected_centre;
        kmVec3Fill(&expected_centre, 10, 10, 15);
        assert_true(kmVec3AreEqual(&expected_centre, &tree.find(&obj2).centre()));

        //Root shouldn't have objects
        assert_true(!tree.root().has_objects());

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
private:
    class Object :
        public kglt::Boundable {

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
};


#endif // TEST_OCTREE_H
