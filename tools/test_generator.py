#!/usr/bin/env python3

import argparse
import re
import sys

parser = argparse.ArgumentParser(description="Generate C++ unit tests")
parser.add_argument("--output", type=str, nargs=1, help="The output source file for the generated test main()", required=True)
parser.add_argument("test_files", type=str, nargs="+", help="The list of C++ files containing your tests")
parser.add_argument("--verbose", help="Verbose logging", action="store_true", default=False)


CLASS_REGEX = r"\s*class\s+(\w+)\s*([\:|,]\s*(?:public|private|protected)\s+[\w|::]+\s*)*"
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

#include "simulant/test.h"

%(includes)s

int main(int argc, char* argv[]) {
    auto runner = std::make_shared<smlt::test::TestRunner>();

    std::string test_case;
    if(argc > 1) {
        test_case = argv[1];
    }

    %(registrations)s

    return runner->run(test_case);
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

                class_name = match.group().split(":")[0].replace("class", "").strip()

                try:
                    parents = match.group().split(":", 1)[1]
                except IndexError:
                    pass
                else:
                    parents = [ x.strip() for x in parents.split(",") ]
                    parents = [
                        x.replace("public", "").replace("private", "").replace("protected", "").strip()
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
                        if class_data[-1] == '{': brace_counter += 1
                        if class_data[-1] == '}': brace_counter -= 1
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


    # Now, simplify the list by finding all potential superclasses, and then keeping any classes
    # that subclass them.
    test_case_subclasses = []
    i = 0
    while i < len(subclasses):
        subclass_names = [x.rsplit("::")[-1] for x in subclasses[i][2]]

        # If this subclasses TestCase, or it subclasses any of the already found testcase subclasses
        # then add it to the list
        if "TestCase" in subclass_names or "SimulantTestCase" in subclass_names or any(x[1] in subclasses[i][2] for x in test_case_subclasses):
            if subclasses[i] not in test_case_subclasses:
                test_case_subclasses.append(subclasses[i])

                i = 0 # Go back to the start, as we may have just found another parent class
                continue
        i += 1

    log_verbose("\n".join([str(x) for x in test_case_subclasses]))
    return test_case_subclasses


def main():
    global VERBOSE

    args = parser.parse_args()

    VERBOSE = args.verbose

    testcases = find_tests(args.test_files)

    includes = "\n".join([ INCLUDE_TEMPLATE % { 'file_path' : x } for x in set([y[0] for y in testcases]) ])
    registrations = []

    for path, class_name, superclasses, funcs in testcases:
        BIND_TEMPLATE = "&%(class_name)s::%(func)s"

        members = ", ".join([ BIND_TEMPLATE % { 'class_name' : class_name, 'func' : x } for x in funcs ])
        names = ", ".join([ '"%s::%s"' % (class_name, x) for x in funcs ])

        registrations.append(REGISTER_TEMPLATE % { 'class_name' : class_name, 'members' : members, 'names' : names })

    registrations = "\n".join(registrations)

    final = MAIN_TEMPLATE % {
        'registrations' : registrations,
        'includes' : includes
    }

    open(args.output[0], "w").write(final)

    return 0

if __name__ == '__main__':
    sys.exit(main())
