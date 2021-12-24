#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
import re
import os
from os.path import dirname, join as pjoin
import platform

import skbuild.constants
skbuild.constants.SKBUILD_DIR = lambda: BUILD_DIR
from skbuild import setup
from tools import updatebadge

# Please Setting ----------------------------------------------------------
# If you wan't install compiled scripts by C++ etc

PROJECT_NAME = 'ccore'

# If you wan't change build directory name
BUILD_DIR = "build"

# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/Useful-Variables
# https://scikit-build.readthedocs.io/en/stable/usage.html#usage-scikit-build-options
cmake_args = {
    "common": [
    ],
    "windows": [
    ],
    "linux": [
    ],
    "darwin": [
    ]
}
# -------------------------------------------------------------------------

thisdir = dirname(__file__)
__version__ = open(pjoin(thisdir, "VERSION"), "r").read().strip()

# OS Environment Infomation
osname = platform.system().lower()
iswin = os.name == "nt"
isposix = os.name == "posix"
islinux = osname == "linux"
isosx = osname == "darwin"
is_debug = "--debug" in sys.argv[1:] or re.search(r" \-[^ ]*g", " ".join(sys.argv[1:]))
is_test = 'pytest' in sys.argv or 'test' in sys.argv

# convert to scikit-build option
if "--build-type" not in sys.argv:
    sys.argv.extend([
        "--build-type", "PYDEBUG" if is_debug else "Release"
    ])


# Readme badge link update.
updatebadge.readme(pjoin(thisdir, "README.md"), new_version=__version__)

# Edit posix platname for pypi upload error
if islinux and any(x.startswith("bdist") for x in sys.argv) \
        and not ("--plat-name" in sys.argv or "-p" in sys.argv):
    from tools.platforms import get_platname_64bit as x64
    from tools.platforms import get_platname_32bit as x86
    sys.argv.extend(["--plat-name", x64() if "64" in os.uname()[-1] else x86()])


setup(
    # to be package directory name.
    packages=[PROJECT_NAME],
    cmake_args=cmake_args["common"] + cmake_args.get(osname, []),

    # Require pytest-runner only when running tests
    setup_requires=['pytest-runner>=2.0,<3dev'] if is_test else [],
)
# Other Setting to setup.cfg
