/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <memory>

#include "application.h"
#include "window.h"

#define assert_equal(expected, actual) _assert_equal((expected), (actual), __FILE__, __LINE__)
#define assert_not_equal(expected, actual) _assert_not_equal((expected), (actual), __FILE__, __LINE__)
#define assert_false(actual) _assert_false((actual), __FILE__, __LINE__)
#define assert_true(actual) _assert_true((actual), __FILE__, __LINE__)
#define assert_close(expected, actual, difference) _assert_close((expected), (actual), (difference), __FILE__, __LINE__)
#define assert_is_null(actual) _assert_is_null((actual), __FILE__, __LINE__)
#define assert_is_not_null(actual) _assert_is_not_null((actual), __FILE__, __LINE__)
#define assert_raises(exception, func) _assert_raises<exception>((func), __FILE__, __LINE__)
#define assert_items_equal(expected, actual) _assert_items_equal((actual), (expected), __FILE__, __LINE__)
#define not_implemented() _not_implemented(__FILE__, __LINE__)


namespace smlt {
namespace test {


class StringFormatter {
public:
    StringFormatter(const std::string& templ):
        templ_(templ) { }

    struct Counter {
        Counter(uint32_t c): c(c) {}
        uint32_t c;
    };

    template<typename T>
    std::string format(T value) {
        std::stringstream ss;
        ss << value;
        return _do_format(0, ss.str());
    }

    template<typename T>
    std::string format(Counter count, T value) {
        std::stringstream ss;
        ss << value;
        return _do_format(count.c, ss.str());
    }

    template<typename T, typename... Args>
    std::string format(T value, const Args&... args) {
        std::stringstream ss;
        ss << value;
        return StringFormatter(_do_format(0, ss.str())).format(Counter(1), args...);
    }

    template<typename T, typename... Args>
    std::string format(Counter count, T value, const Args&... args) {
        std::stringstream ss;
        ss << value;
        return StringFormatter(_do_format(count.c, ss.str())).format(Counter(count.c + 1), args...);
    }

    std::string _do_format(uint32_t counter, const std::string& value) {
        std::stringstream ss; // Can't use to_string on all platforms
        ss << counter;

        const std::string to_replace = "{" + ss.str() + "}";
        std::string output = templ_;

        auto replace = [](std::string& str, const std::string& from, const std::string& to) -> bool {
            size_t start_pos = str.find(from);
            if(start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        };

        replace(output, to_replace, value);
        return output;
    }

private:
    std::string templ_;
};

class StringSplitter {
public:
    StringSplitter(const std::string& str):
        str_(str) {

    }

    std::vector<std::string> split() {
        std::vector<std::string> result;
        std::string buffer;

        for(auto c: str_) {
            if(c == '\n') {
                if(!buffer.empty()) {
                    result.push_back(buffer);
                    buffer.clear();
                }
            } else {
                buffer.push_back(c);
            }
        }

        if(!buffer.empty()) {
            result.push_back(buffer);
        }

        return result;
    }

private:
    std::string str_;
};

typedef StringFormatter _Format;

class AssertionError : public std::logic_error {
public:
    AssertionError(const std::string& what):
        std::logic_error(what),
        file(""),
        line(-1) {
    }

    AssertionError(const std::pair<std::string, int> file_and_line, const std::string& what):
        std::logic_error(what),
        file(file_and_line.first),
        line(file_and_line.second) {

    }

    ~AssertionError() noexcept (true) {

    }

    std::string file;
    int line;
};


class NotImplementedError: public std::logic_error {
public:
    NotImplementedError(const std::string& file, int line):
        std::logic_error(_Format("Not implemented at {0}:{1}").format(file, line)) {}
};


class SkippedTestError: public std::logic_error {
public:
    SkippedTestError(const std::string& reason):
    std::logic_error(reason) {

    }
};

class TestCase {
public:
    virtual ~TestCase() {}

    virtual void set_up() {}
    virtual void tear_down() {}

    void skip_if(const bool& flag, const std::string& reason) {
        if(flag) { throw test::SkippedTestError(reason); }
    }

    template<typename T, typename U>
    void _assert_equal(T expected, U actual, std::string file, int line) {
        if(expected != actual) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, test::_Format("{0} does not match {1}").format(actual, expected));
        }
    }

    template<typename T, typename U>
    void _assert_not_equal(T lhs, U rhs, std::string file, int line) {
        if(lhs == (T) rhs) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, test::_Format("{0} should not match {1}").format(lhs, rhs));
        }
    }

    template<typename T>
    void _assert_true(T actual, std::string file, int line) {
        if(!bool(actual)) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, test::_Format("{0} is not true").format(bool(actual) ? "true" : "false"));
        }
    }

    template<typename T>
    void _assert_false(T actual, std::string file, int line) {
        if(bool(actual)) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, test::_Format("{0} is not false").format(bool(actual) ? "true" : "false"));
        }
    }

    template<typename T, typename U, typename V>
    void _assert_close(T expected, U actual, V difference, std::string file, int line) {
        if(actual < expected - difference ||
           actual > expected + difference) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, test::_Format("{0} is not close enough to {1}").format(actual, expected));
        }
    }

    template<typename T>
    void _assert_is_null(T* thing, std::string file, int line) {
        if(thing != nullptr) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, "Pointer was not NULL");
        }
    }

    template<typename T>
    void _assert_is_not_null(T* thing, std::string file, int line) {
        if(thing == nullptr) {
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, "Pointer was unexpectedly NULL");
        }
    }

    template<typename T, typename Func>
    void _assert_raises(Func func, std::string file, int line) {
        try {
            func();
            auto file_and_line = std::make_pair(file, line);
            throw test::AssertionError(file_and_line, test::_Format("Expected exception ({0}) was not thrown").format(typeid(T).name()));
        } catch(T& e) {}
    }

    template<typename T, typename U>
    void _assert_items_equal(const T& lhs, const U& rhs, std::string file, int line) {
        auto file_and_line = std::make_pair(file, line);

        if(lhs.size() != rhs.size()) {
            throw test::AssertionError(file_and_line, "Containers are not the same length");
        }

        for(auto item: lhs) {
            if(std::find(rhs.begin(), rhs.end(), item) == rhs.end()) {
                throw test::AssertionError(file_and_line, test::_Format("Container does not contain {0}").format(item));
            }
        }
    }

    void _not_implemented(std::string file, int line) {
        throw test::NotImplementedError(file, line);
    }
};

class TestRunner {
public:
    template<typename T, typename U>
    void register_case(std::vector<U> methods, std::vector<std::string> names) {
        std::shared_ptr<TestCase> instance = std::make_shared<T>();

        instances_.push_back(instance); //Hold on to it

        for(std::string name: names) {
            names_.push_back(name);
        }

        for(U& method: methods) {
            std::function<void()> func = std::bind(method, dynamic_cast<T*>(instance.get()));
            tests_.push_back([=]() {
                instance->set_up();
                func();
                instance->tear_down();
            });
        }
    }

    int32_t run(const std::string& test_case) {
        int failed = 0;
        int skipped = 0;
        int ran = 0;
        int crashed = 0;

        auto new_tests = tests_;
        auto new_names = names_;

        if(!test_case.empty()) {
            new_tests.clear();
            new_names.clear();

            for(uint32_t i = 0; i < names_.size(); ++i) {
                if(names_[i].find(test_case) == 0) {
                    new_tests.push_back(tests_[i]);
                    new_names.push_back(names_[i]);
                }
            }
        }

        std::cout << std::endl << "Running " << new_tests.size() << " tests" << std::endl << std::endl;

        for(std::function<void ()> test: new_tests) {
            try {
                std::string output = "    " + new_names[ran];

                for(int i = output.length(); i < 76; ++i) {
                    output += " ";
                }

                std::cout << output;
                test();
                std::cout << "\033[32m" << "   OK   " << "\033[0m" << std::endl;
            } catch(test::NotImplementedError& e) {
                std::cout << "\033[34m" << " SKIPPED" << "\033[0m" << std::endl;
                ++skipped;
        } catch(test::SkippedTestError& e) {
                std::cout << "\033[34m" << " SKIPPED" << "\033[0m" << std::endl;
                ++skipped;
            } catch(test::AssertionError& e) {
                std::cout << "\033[33m" << " FAILED " << "\033[0m" << std::endl;
                std::cout << "        " << e.what() << std::endl;
                if(!e.file.empty()) {
                    std::cout << "        " << e.file << ":" << e.line << std::endl;

                    std::ifstream ifs(e.file);
                    if(ifs.good()) {
            std::string buffer;
            std::vector<std::string> lines;
            while(std::getline(ifs, buffer)) {
                            lines.push_back(buffer);
            }

                        int line_count = lines.size();
                        if(line_count && e.line <= line_count) {
                            std::cout << lines.at(e.line - 1) << std::endl << std::endl;
                        }
                    }
                }
                ++failed;
            } catch(std::exception& e) {
                std::cout << "\033[31m" << " EXCEPT " << std::endl;
                std::cout << "        " << e.what() << "\033[0m" << std::endl;
                ++crashed;
            }
            std::cout << "\033[0m";
            ++ran;
        }

        std::cout << "-----------------------" << std::endl;
        if(!failed && !crashed && !skipped) {
            std::cout << "All tests passed" << std::endl << std::endl;
        } else {
            if(skipped) {
                std::cout << skipped << " tests skipped";
            }

            if(failed) {
                if(skipped) {
                    std::cout << ", ";
                }
                std::cout << failed << " tests failed";
            }

            if(crashed) {
                if(failed) {
                    std::cout << ", ";
                }
                std::cout << crashed << " tests crashed";
            }
            std::cout << std::endl << std::endl;
        }

        return failed + crashed;
    }

private:
    std::vector<std::shared_ptr<TestCase>> instances_;
    std::vector<std::function<void()> > tests_;
    std::vector<std::string> names_;
};


class TestApp: public smlt::Application {
public:
    TestApp(const AppConfig& config):
        Application(config) {
    }

private:
    bool init() {
        return true;
    }
};

class SimulantTestCase : public TestCase {
private:
    void set_app_and_window(std::shared_ptr<Application>* app, Window** window) {
        static std::shared_ptr<Application> application;

        if(!application) {
            AppConfig config;
            config.width = 640;
            config.height = 480;
            config.fullscreen = false;

            application.reset(new TestApp(config));

            // FIXME: This is a bit simulant-specific, you wouldn't necessarily want this
            // path on user apps.
            application->window->resource_locator->add_search_path("sample_data");
            application->window->resource_locator->add_search_path("assets");
        } else {
            application->window->reset();
        }

        *app = application;
        *window = (*app)->window;
    }

protected:
    Window* window;
    std::shared_ptr<Application> application;

public:
    void set_up() {
        TestCase::set_up();

        set_app_and_window(&application, &window);
    }
};

} // test
} // smlt

