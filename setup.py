#!/usr/bin/env python
import re

from setuptools import Extension, setup
from distutils.ccompiler import get_default_compiler
import os
import io
import sys
from os.path import dirname, join as pjoin

__version__ = '0.0.5'

DESCRIPTION = "core utility units for python"
KEYWORDS = ["flatten", "to_datetime"]
LISCENSE = "MIT"
LANGUAGE = 'c++'
MOD_NAME = 'ccore'
MOD_SRC = ['src/ccore.cpp']
AUTHOR = 'kirin123kirin'
URL = 'https://github.com/' + AUTHOR + '/' + MOD_NAME
PLATFORMS = ["Windows", "Linux", "Mac OS-X"]

# # https://pypi.org/classifiers/
CLASSIFIERS = """
Development Status :: 2 - Pre-Alpha
License :: OSI Approved :: MIT License
Programming Language :: C
Programming Language :: C++
Programming Language :: Python :: 3.6
Programming Language :: Python :: 3.7
Programming Language :: Python :: 3.8
Programming Language :: Python :: 3.9
Operating System :: OS Independent
Operating System :: Microsoft :: Windows
Operating System :: MacOS
Operating System :: POSIX
"""
UNDEF_MACROS = []

iswin = os.name == "nt"
isposix = os.name == "posix"
ismsvc = get_default_compiler() == "msvc"

globalinc = '/IC:/usr/lib/' if iswin else '-I/usr/include/'

def sep(*x):
    return (":" if ismsvc else "=").join(x)


COMPILE_ARGS = [
    sep('-std', 'c++14'),
    globalinc + 'boost',
]

if any("--debug" in x or "-g" in x for x in sys.argv):
    if ismsvc:
        UNDEF_MACROS.extend(
            [
                "_DEBUG",
            ]
        )
        COMPILE_ARGS.extend(
            [
                # Reason https://docs.microsoft.com/ja-jp/cpp/build/reference/ltcg-link-time-code-generation?view=msvc-160
                "/GL",
                # Reason unicode string crash #
                "/source-charset:utf-8",
                # Reason IDE warning link crash #
                "/FC",
            ]
        )

    elif isposix:
        COMPILE_ARGS.extend(
            [
                "-O0",
            ]
        )

# Edit posix platname for pypi upload error
if isposix and any(x.startswith("bdist") for x in sys.argv) \
        and not ("--plat-name" in sys.argv or "-p" in sys.argv):

    if "64" in os.uname()[-1]:
        from setup_platname import get_platname_64bit

        plat = get_platname_64bit()
    else:
        from setup_platname import get_platname_32bit

        plat = get_platname_32bit()
    sys.argv.extend(["--plat-name", plat])


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

setup(name=MOD_NAME,
      version=__version__,
      description=DESCRIPTION,
      long_description_content_type='text/markdown',
      long_description=description,
      url=URL,
      author=AUTHOR,
      ext_modules=[
          Extension(
              MOD_NAME,
              sources=MOD_SRC,
              undef_macros=UNDEF_MACROS,
              extra_compile_args=COMPILE_ARGS,
              language=LANGUAGE
          )
      ],
      keywords=KEYWORDS,
      license=LISCENSE,
      platforms=PLATFORMS,
      classifiers=CLASSIFIERS.strip().splitlines(),
      **tests
      )
