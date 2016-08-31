#ifndef TEST_WATCHER_H
#define TEST_WATCHER_H

#include <kfs/kfs.h>
#include "kaztest/kaztest.h"
#include "kglt/watcher.h"

namespace {

using namespace kglt;

class WatcherTest : public TestCase {
public:
    void set_up() {
        watcher = Watcher::create();

        modify_counter = 0;
        delete_counter = 0;
        move_counter = 0;
    }

    void callback(const unicode& filename, WatchEvent event) {
        if(event == WATCH_EVENT_MODIFY) {
            modify_counter++;
        } else if(event == WATCH_EVENT_DELETE) {
            delete_counter++;
        } else if(event == WATCH_EVENT_MOVE) {
            move_counter++;
        }
    }

    void test_watching() {
        kfs::Path test_file = kfs::path::join(kfs::temp_dir(), "watcher.test");
        kfs::touch(test_file);

        watcher->watch(test_file, std::bind(&WatcherTest::callback, this, std::placeholders::_1, std::placeholders::_2));

        assert_equal(0, modify_counter);

        kfs::touch(test_file);
        watcher->update();

        assert_equal(1, modify_counter);

        kfs::touch(test_file);
        watcher->update();

        assert_equal(2, modify_counter);

        assert_equal(0, delete_counter);

        kfs::remove(test_file);
        watcher->update();

        assert_equal(1, delete_counter);
    }

private:
    Watcher::ptr watcher;

    int modify_counter;
    int delete_counter;
    int move_counter;
};


}

#endif // TEST_WATCHER_H
