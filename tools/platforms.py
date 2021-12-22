#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from os.path import dirname, normpath, join as pjoin

try:
    from subprocess import getstatusoutput
except ImportError:
    from commands import getstatusoutput
from distutils.util import get_platform
from platform import machine, system

def have_glibc(major, minimum_minor):
    # from PEP571 https://www.python.org/dev/peps/pep-0571/
    import ctypes

    process_namespace = ctypes.CDLL(None)
    try:
        gnu_get_libc_version = process_namespace.gnu_get_libc_version
    except AttributeError:
        # Symbol doesn't exist -> therefore, we are not linked to
        # glibc.
        return False

    # Call gnu_get_libc_version, which returns a string like "2.5".
    gnu_get_libc_version.restype = ctypes.c_char_p
    version_str = gnu_get_libc_version()
    # py2 / py3 compatibility:
    if not isinstance(version_str, str):
        version_str = version_str.decode("ascii")

    # Parse string and check against requested version.
    version = [int(piece) for piece in version_str.split(".")]
    assert len(version) == 2

    if major != version[0]:
        return False

    if minimum_minor > version[1]:
        return False

    return True


def is_manylinux1(bit64=True, bit32=False):
    # from PEP 513 https://www.python.org/dev/peps/pep-0513/#id50
    #
    # Only Linux, and only x86-64 / i686
    if get_platform() not in ["linux-x86_64", "linux-i686"]:
        return []

    # Check glibc version. CentOS 5 uses glibc 2.5.
    if have_glibc(2, 5):
        return []

    ret = []
    if bit64:
        ret.append("manylinux1_x86_64")
    if bit32:
        ret.append("manylinux1_i686")
    return ret


def is_manylinux2010(bit64=True, bit32=False):
    # Only Linux, and only x86-64 / i686
    if get_platform() not in ["linux-x86_64", "linux-i686"]:
        return []

    # Check glibc version. CentOS 6 uses glibc 2.12.
    # PEP 513 contains an implementation of this function.
    if not have_glibc(2, 12):
        return []

    ret = []
    if bit64:
        ret.append("manylinux2010_x86_64")
    if bit32:
        ret.append("manylinux2010_i686")
    return ret


def is_manylinux2014(bit64=True, bit32=False):
    # from PE599 https://www.python.org/dev/peps/pep-0599/
    # Only Linux, and only supported architectures
    platform = get_platform()
    if platform not in [
        "linux-x86_64",
        "linux-i686",
        "linux-aarch64",
        "linux-armv7l",
        "linux-ppc64",
        "linux-ppc64le",
        "linux-s390x",
    ]:
        return []

    # Check glibc version. CentOS 7 uses glibc 2.17.
    # PEP 513 contains an implementation of this function.
    if have_glibc(2, 17):
        ret = []
        arch = platform.split("-")[1]
        if bit64 and ("64" in arch or "s390x" == arch):
            ret.append("manylinux2014_" + arch)
        if bit64 and arch in ["armv7", ]:
            ret.append("manylinux2014_" + arch)
        return []


def get_platname_64bit():
    arch = machine()
    OS = system()

    if OS == "Windows":
        return "win_" + arch.lower()
    elif OS == "Linux":
        if "aarch64" in arch:
            return "manylinux2014_" + arch
        return (is_manylinux2014() or
                is_manylinux2010() or
                is_manylinux1() or
                ["manylinux2010_" + arch])[0]
    elif OS == "Darwin":
        from platform import mac_ver
        import re
        rel, _, arch = mac_ver()
        mj, mn = re.split(r'[\.\-_]', rel)[:2]
        return "macosx_{}_{}_{}".format(mj, mn, arch)
    else:
        return get_platform()


def get_platname_32bit():
    OS = system()

    if OS == "Windows":
        return "win32"
    elif OS == "Linux":
        return (is_manylinux2014(False, True) or
                is_manylinux2010(False, True) or
                is_manylinux1(False, True) or
                ["manylinux1_i686"])[0]
    elif OS == "Darwin":
        return "macosx-x86"
    else:
        return get_platform()


def command(cmd):
    print("Build command Running... ", cmd)
    code, dat = getstatusoutput(cmd)
    if code == 0:
        return dat
    else:
        raise RuntimeError(code, dat)


def cross_wheel_build():
    pth = normpath(pjoin(dirname(__file__), "setup.py"))
    cmd = "python {} bdist_wheel --plat-name ".format(pth)
    command(cmd + get_platname_64bit())
    command(cmd + get_platname_32bit())  # TODO 32bit cross compile dekinai


if __name__ == "__main__":
    cross_wheel_build()
