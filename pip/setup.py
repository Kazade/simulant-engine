import os
from setuptools import setup, find_packages


NAME = "simulant-tools"
PACKAGES = find_packages()
DESCRIPTION = "Tools to harness the Simulant Game Engine"
URL = "https://simulant-engine.appspot.com/"
LONG_DESCRIPTION = open(os.path.join(os.path.dirname(__file__), "README.md")).read()
AUTHOR = "Luke Benstead"
VERSION = "0.3a"

data_files = [
    ("share/simulant-tools/toolchains", ["toolchains/Dreamcast.cmake"]),
    ("share/simulant-tools/toolchains", ["toolchains/PSP.cmake"]),
    ("share/simulant-tools/toolchains", ["toolchains/Evercade.cmake"]),
    ("share/simulant-tools/toolchains", ["toolchains/RaspberryPi.cmake"]),
    ("share/simulant-tools/toolchains", ["toolchains/CreatePBP.cmake"]),
    ("share/simulant-tools/toolchains", ["toolchains/IP.TMPL"]),
]


for branch in ("main", "next"):
    data_files.extend(
        [
            (
                f"share/simulant-tools/templates/{branch}/assets",
                [f"templates/{branch}/assets/README"],
            ),
            (
                f"share/simulant-tools/templates/{branch}/libraries",
                [f"templates/{branch}/libraries/README"],
            ),
            (
                f"share/simulant-tools/templates/{branch}/sources",
                [f"templates/{branch}/sources/main.cpp"],
            ),
            (
                f"share/simulant-tools/templates/{branch}/tests",
                [f"templates/{branch}/tests/test_sample.h"],
            ),
            (
                f"share/simulant-tools/templates/{branch}/tests",
                [f"templates/{branch}/tests/README"],
            ),
            (
                f"share/simulant-tools/templates/{branch}/sources/scenes",
                [f"templates/{branch}/sources/scenes/game.cpp"],
            ),
            (
                f"share/simulant-tools/templates/{branch}/sources/scenes",
                [f"templates/{branch}/sources/scenes/game.h"],
            ),
            (
                f"share/simulant-tools/templates/{branch}",
                [f"templates/{branch}/CMakeLists.txt"],
            ),
            (
                f"share/simulant-tools/templates/{branch}",
                [f"templates/{branch}/simulant.json"],
            ),
        ],
    )

setup(
    name=NAME,
    version=VERSION,
    packages=PACKAGES,
    scripts=["simulant"],
    author=AUTHOR,
    description=DESCRIPTION,
    long_description=LONG_DESCRIPTION,
    keywords=["gamedev", "game", "engine", "simulant"],
    url=URL,
    install_requires=["docker", "clint", "requests"],
    classifiers=[
        "Environment :: Console" "Development Status :: 3 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
        "Topic :: Software Development :: Build Tools",
    ],
    include_package_data=True,
    data_files=data_files,
)
