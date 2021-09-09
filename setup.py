#!/usr/bin/env python
from distutils import ccompiler
import re

from setuptools import Extension, setup
from distutils.ccompiler import get_default_compiler
import os
import io
import sys
from os.path import dirname, join as pjoin

__version__ = '0.0.1'

# Edit posix platname for pypi upload error
if os.name == "posix" and any(x.startswith("bdist") for x in sys.argv) \
        and not ("--plat-name" in sys.argv or "-p" in sys.argv):

    if "64" in os.uname()[-1]:
        from setup_platname import get_platname_64bit

        plat = get_platname_64bit()
    else:
        from setup_platname import get_platname_32bit

        plat = get_platname_32bit()
    sys.argv.extend(["--plat-name", plat])

MOD_NAME = 'ccore'
MOD_SRC = ['src/ccore.cpp']

ext_modules = [
    Extension(
        MOD_NAME,
        extra_compile_args=[
            '-std:c++14',
            # '-std:c++17',
            # '/IC:/usr/lib/boost'
        ],
        sources=MOD_SRC,
        language="c++"
    )]

if any("--debug" in x or "-g" in x for x in sys.argv) and get_default_compiler() == "msvc":
    ext_modules = [
        Extension(
            MOD_NAME,
            sources=MOD_SRC,
            # Reason is Debuging Error "Access violation executing location 0x00000000" when using mwArray in Visual-C++
            undef_macros=["_DEBUG"],
            extra_compile_args=[
                # Reason https://docs.microsoft.com/ja-jp/cpp/build/reference/ltcg-link-time-code-generation?view=msvc-160
                "/GL",
                # Reason unicode string crash #
                "/source-charset:utf-8",
                # Reason IDE warning link crash #
                "/FC",
                "/std:c++14",
                # "/std:c++17",
            ],
            language="c++"
        )]

CF = """
Development Status :: 2 - Pre-Alpha
License :: OSI Approved :: GNU General Public License v2 (GPLv2)
Programming Language :: C
Programming Language :: Python :: 3.6
Programming Language :: Python :: 3.7
Programming Language :: Python :: 3.8
Programming Language :: Python :: 3.9
Programming Language :: Python :: Implementation :: CPython
Operating System :: OS Independent
Operating System :: Microsoft :: Windows
Operating System :: MacOS
Operating System :: POSIX
"""

# Readme read or edit
readme = pjoin(dirname(__file__), "README.md")
badge = re.compile(r'(\[!\[.*?\]\(https://.*?badge\.(?:svg|png)\?branch=([^\)]+)\)\])')
description = ""
is_change = False
with io.open(readme, encoding="utf-8") as f:
    for line in f:
        res = badge.search(line)
        if res and __version__ not in res.group(2):
            for b, k in badge.findall(line):
                line = line.replace(b, b.replace(k, "v" + __version__))
            is_change = True
        description += line

if is_change:
    with io.open(readme, "w", encoding="utf-8") as f:
        f.write(description)

# for python2.7
tests = {}
if sys.version_info[:2] >= (3, 3):
    tests = dict(
        setup_requires=["pytest-runner"],
        tests_require=["pytest", "pytest-cov", "psutil"])

setup(name="ccore",
      version=__version__,
      description="core utility units for python",
      long_description_content_type='text/markdown',
      long_description=description,
      url='https://github.com/kirin123kirin/ccore',
      author='kirin123kirin',
      ext_modules=ext_modules,
      keywords=["flatten", "to_datetime"],
      license="GPL2",
      platforms=["Windows", "Linux", "Mac OS-X"],
      classifiers=CF.strip().splitlines(),
      **tests
      )
