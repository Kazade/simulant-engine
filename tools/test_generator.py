#!/usr/bin/env python3

import argparse
import re
import sys

parser = argparse.ArgumentParser(description="Generate C++ unit tests")
parser.add_argument("--output", type=str, nargs=1, help="The output source file for the generated test main()")  # noqa
parser.add_argument("test_files", type=str, nargs="+", help="The list of C++ files containing your tests")  # noqa
parser.add_argument("--verbose", help="Verbose logging", action="store_true", default=False)  # noqa
parser.add_argument("--list-tests", help="Don't generate the tests, simply output the discovered tests", action="store_true", default=False)  # noqa


CLASS_REGEX = r"\s*class\s+(\w+)\s*([\:|,]\s*(?:public|private|protected)\s+[\w|::]+\s*)*"  # noqa
TEST_FUNC_REGEX = r"void\s+(?P<func_name>test_\S[^\(]+)\(\s*(void)?\s*\)"


INCLUDE_TEMPLATE = "#include \"%(file_path)s\""

REGISTER_TEMPLATE = """
    runner->register_case<%(class_name)s>(
        std::vector<void (%(class_name)s::*)()>({%(members)s}),
        {%(names)s}
    );"""

MAIN_TEMPLATE = """

#include <functional>
#include <memory>
#include <map>

#ifdef __DREAMCAST__
#include <kos.h>
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS | INIT_NET);
#endif

#include "simulant/test.h"

%(includes)s


std::map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::map<std::string, std::string> ret;

    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto eq = arg.find('=');
        if(eq != std::string::npos && arg[0] == '-' && arg[1] == '-') {
            auto key = std::string(arg.begin(), arg.begin() + eq);
            auto value = std::string(arg.begin() + eq + 1, arg.end());
            ret[key] = value;
        } else if(arg[0] == '-' && arg[1] == '-') {
            auto key = arg;
            if(i < (argc - 1)) {
                auto value = argv[++i];
                ret[key] = value;
            } else {
                ret[key] = "";
            }
        } else {
            ret[arg] = "";  // Positional, not key=value
        }
    }

    return ret;
}

int main(int argc, char* argv[]) {
    auto runner = std::make_shared<smlt::test::TestRunner>();

    auto args = parse_args(argc, argv);

    std::string junit_xml;
    auto junit_xml_it = args.find("--junit-xml");
    if(junit_xml_it != args.end()) {
        junit_xml = junit_xml_it->second;
        std::cout << "    Outputting junit XML to: " << junit_xml << std::endl;
        args.erase(junit_xml_it);
    }

    std::string test_case;
    if(args.size()) {
        test_case = args.begin()->first;
    }

    %(registrations)s

    return runner->run(test_case, junit_xml);
}


"""

VERBOSE = False


def log_verbose(message):
    if VERBOSE:
        print(message)


def find_tests(files):

    subclasses = []

    # First pass, find all class definitions
    for path in files:
        with open(path, "rt") as f:
            source_file_data = f.read().replace("\r\n", "").replace("\n", "")

            while True:
                match = re.search(CLASS_REGEX, source_file_data)
                if not match:
                    break

                class_name = match.group().split(":")[0].replace(
                    "class", ""
                ).strip()

                try:
                    parents = match.group().split(":", 1)[1]
                except IndexError:
                    pass
                else:
                    parents = [x.strip() for x in parents.split(",")]
                    parents = [
                        x.replace("public", "").replace(
                            "private", ""
                        ).replace("protected", "").strip()
                        for x in parents
                    ]

                    subclasses.append((path, class_name, parents, []))
                    log_verbose("Found: %s" % str(subclasses[-1]))

                start = match.end()

                # Find the next opening brace
                while source_file_data[start] in (' ', '\t'):
                    start += 1

                start -= 1
                end = start
                if source_file_data[start+1] == '{':

                    class_data = []
                    brace_counter = 1
                    for i in range(start+2, len(source_file_data)):
                        class_data.append(source_file_data[i])
                        if class_data[-1] == '{':
                            brace_counter += 1
                        if class_data[-1] == '}':
                            brace_counter -= 1
                        if not brace_counter:
                            end = i
                            break

                    class_data = "".join(class_data)

                    while True:
                        match = re.search(TEST_FUNC_REGEX, class_data)
                        if not match:
                            break

                        subclasses[-1][-1].append(match.group('func_name'))
                        class_data = class_data[match.end():]

                source_file_data = source_file_data[end:]

    # Now, simplify the list by finding all potential superclasses,
    # and then keeping any classes
    # that subclass them.
    test_case_subclasses = []
    i = 0
    while i < len(subclasses):
        subclass_names = [x.rsplit("::")[-1] for x in subclasses[i][2]]

        # If this subclasses TestCase, or it subclasses any of the already
        # found testcase subclasses
        # then add it to the list
        if "TestCase" in subclass_names or \
           "SimulantTestCase" in subclass_names or \
           any(x[1] in subclasses[i][2] for x in test_case_subclasses):
            if subclasses[i] not in test_case_subclasses:
                test_case_subclasses.append(subclasses[i])
                i = 0  # Go back to the start, as we may have just found
                # another parent class
                continue
        i += 1

    log_verbose("\n".join([str(x) for x in test_case_subclasses]))
    return test_case_subclasses


def main():
    global VERBOSE

    args = parser.parse_args()

    VERBOSE = args.verbose

    testcases = find_tests(args.test_files)

    if args.list_tests:
        for f, klass, bases, tests in testcases:
            for test in tests:
                print(f"{klass}::{test}")
        return 0
    elif not args.output:
        print("OUTPUT is required")
        return 1

    includes = "\n".join([
        INCLUDE_TEMPLATE % {'file_path': x}
        for x in set([y[0] for y in testcases])
    ])
    registrations = []

    for path, class_name, superclasses, funcs in testcases:
        BIND_TEMPLATE = "&%(class_name)s::%(func)s"

        members = ", ".join([
            BIND_TEMPLATE % {'class_name': class_name, 'func': x}
            for x in funcs
        ])
        names = ", ".join(['"%s::%s"' % (class_name, x) for x in funcs])

        registrations.append(
            REGISTER_TEMPLATE % {
                'class_name': class_name,
                'members': members,
                'names': names
            }
        )

    registrations = "\n".join(registrations)

    final = MAIN_TEMPLATE % {
        'registrations': registrations,
        'includes': includes
    }

    open(args.output[0], "w").write(final)

    return 0


if __name__ == '__main__':
    sys.exit(main())
