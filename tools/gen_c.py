#!/usr/bin/env python
import os
import re
import subprocess
import sys

src = None
dest = None


def _find_classes_and_functions(path):
    """
    Given a header file, this extracts:

        - Class names, with their associated line
        - Free function names, with their associated line
    """

    with open(path, "r") as f:
        content = f.read()

        lines = content.splitlines()

        # Regex for class/struct declarations
        class_pattern = re.compile(
            r"^\s*(class|struct)\s+([A-Za-z_]\w*)"
            r"(?:\s*:\s*[^{]+)?\s*\{?",
        )

        # Regex for free function prototypes (heuristic)
        function_pattern = re.compile(
            r"""^\s*
            (?:template\s*<[^>]+>\s*)?                # optional template
            (?:inline\s+|constexpr\s+|static\s+)?     # optional specifiers
            ([\w:\<\>\*&\s]+?)                        # return type
            \s+
            ([A-Za-z_]\w*)                            # function name
            \s*
            \([^;{}]*\)                               # parameters
            \s*
            (?:const\s*)?                             # optional const
            (?:noexcept\s*)?                          # optional noexcept
            (?:=\s*0\s*)?                             # optional pure virtual
            ;                                         # must end with ;
            """,
            re.VERBOSE,
        )

        classes = []
        functions = []

        inside_class = False
        brace_depth = 0

        for idx, line in enumerate(lines, start=1):
            stripped = line.strip()

            # Track brace depth to avoid matching member functions
            brace_depth += line.count("{")
            brace_depth -= line.count("}")

            if brace_depth > 0:
                inside_class = True
            else:
                inside_class = False

            # Match class/struct
            class_match = class_pattern.match(line)
            if class_match:
                classes.append({"line": idx, "name": class_match.group(2)})

            # Match free function (only if not inside class)
            if not inside_class:
                func_match = function_pattern.match(line)
                if func_match:
                    functions.append({"line": idx, "prototype": stripped})

        return {"classes": classes, "functions": functions}


# def handle_header_swig(path):
#     rel_path = path.replace(src, "").lstrip("/")
#     dest_path = os.path.join(dest, os.path.splitext(rel_path)[0] + ".i")

#     module = os.path.splitext(os.path.split(rel_path)[-1])[0]

#     parsed = _find_classes_and_functions(path)

#     i_file_content = [f"%module {module}", "%{", f'#include "{rel_path}"', "%}"]
#     for klass in parsed["classes"]:
#         i_file_content.append(f"%ignore {klass['name']}")

#     i_file_content.append(f'%include "{path}"')

#     parent = os.path.abspath(os.path.dirname(dest_path))
#     os.makedirs(parent, exist_ok=True)

#     with open(dest_path, "w") as f:
#         f.write("\n".join(i_file_content))

#     print(f"Processing: {dest_path}")
#     os.chdir(os.path.dirname(dest_path))
#     subprocess.check_call(f"swig -I={dest} -std=c++20 -c++ -c {dest_path}".split())


def handle_header(path):
    rel_path = path.replace(src, "").lstrip("/")
    dest_path = os.path.join(dest, os.path.splitext(rel_path)[0] + ".h")

    parsed = _find_classes_and_functions(path)

    i_file_content = ["#pragma once", ""]

    i_file_content.extend(
        [
            "#ifdef __cplusplus",
            'extern "C" {',
            "#endif",
            "",
        ]
    )

    for klass in parsed["classes"]:
        class_type_name = "smlt_" + klass["name"].lower() + "_t"
        class_name = "smlt_" + klass["name"].lower()
        var_name = klass["name"].lower()
        i_file_content.append(f"struct {class_type_name};")
        i_file_content.append("")
        i_file_content.append(f"{class_type_name}* {class_name}_create();")
        i_file_content.append(
            f"{class_type_name}* {class_name}_destroy({class_type_name}* {var_name});"
        )
        i_file_content.extend([""])

    i_file_content.extend(
        [
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
        ]
    )

    parent = os.path.abspath(os.path.dirname(dest_path))
    os.makedirs(parent, exist_ok=True)

    with open(dest_path, "w") as f:
        f.write("\n".join(i_file_content))

    # Now the fun part, generate the source files
    i_file_content = [
        "#include <simulant/simulant.h>",
        f'#include "./{rel_path}"',
    ]

    for klass in parsed["classes"]:
        class_type_name = "smlt_" + klass["name"].lower() + "_t"
        class_name = "smlt_" + klass["name"].lower()
        var_name = klass["name"].lower()

        constructor = """
%(class_type_name)s* %(class_name)s_create() {
    return new %(klass_name)s();
}""" % dict(
            class_type_name=class_type_name,
            class_name=class_name,
            klass_name=klass["name"],
        )

        i_file_content.extend(constructor.split("\n"))

    dest_path = os.path.splitext(dest_path)[0] + ".cpp"
    with open(dest_path, "w") as f:
        f.write("\n".join(i_file_content))

    # Now the fun part, generate the source files
    i_file_content.clear()


def recurse(path):
    for file in os.listdir(path):
        abs_path = os.path.join(path, file)
        if os.path.splitext(abs_path)[-1] == ".h":
            handle_header(abs_path)
        if os.path.isdir(abs_path):
            recurse(abs_path)


if __name__ == "__main__":
    src = os.path.abspath(sys.argv[1])
    dest = os.path.abspath(sys.argv[2])

    recurse(src)
