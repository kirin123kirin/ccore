#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

from glob import glob
from timeit import timeit
from psutil import Process
from datetime import datetime
if sys.version_info[:2] >= (3, 7):
    from datetime import timezone, timedelta
PY2 = sys.version_info[0] == 2

# github action problem in windows default codepage 1252 environment
# https://stackoverflow.com/questions/27092833/unicodeencodeerror-charmap-codec-cant-encode-characters
defaultencoding = 'utf-8'
if sys.stdout.encoding != defaultencoding:
    if PY2:
        reload(sys)
        sys.setdefaultencoding(defaultencoding)
    elif os.name == "nt":
        sys.stdout.reconfigure(encoding=defaultencoding)

from os.path import dirname, abspath, join as pjoin
shome = abspath(pjoin(dirname(__file__), ".."))
sys.path.insert(0, pjoin(shome, "build"))
sys.path.insert(0, pjoin(shome, "build", "cmake-build"))
sys.path.insert(0, pjoin(shome, "_skbuild", "cmake-build"))
sys.path.insert(0, pjoin(shome, "build", "cmake-install"))
sys.path.insert(0, pjoin(shome, "_skbuild", "cmake-install"))
try:
    from ccore import *
    kw = {"setup": "from ccore import *"} if PY2 else {}
except ImportError:
    try:
        from _ccore import *
        kw = {"setup": "from _ccore import *"} if PY2 else {}
    except ImportError:
        from ccore._ccore import *
        kw = {"setup": "from ccore._ccore import *"} if PY2 else {}

from io import BytesIO, StringIO
from zipfile import ZipFile, ZipExtFile
from pathlib import Path
import bz2
import gzip
import lzma
import tarfile
from io import TextIOWrapper, BufferedReader
from data import tdir, TMPDIR

process = Process(os.getpid())
def memusage():
    return process.memory_info()[0] / 1024

def runtimeit(funcstr, number=1000):
    i = 0
    kw["number"] = number
    if sys.version_info[0] >= 3:
        kw["globals"] = globals()

    for fc in funcstr.strip().splitlines():
        fc = fc.strip()

        if i == 0:
            timeit(fc, **kw)
        bm = memusage()
        p = timeit(fc, **kw)

        am = (memusage() - bm)
        assert am < 1000, "{} function {}KB Memory Leak Error".format(fc, am)
        print("{}: {} ns (mem after {}KB)".format(fc, int(1000000000 * p / number), am))
        i += 1

def test_binopen_normal():
    global f, z
    import http.client
    import urllib.request
    with binopen(tdir + "/test.html", "rb") as f:
        isinstance(f, BufferedReader)
    runtimeit('binopen(tdir + "/test.html", "rb")')

    with open(tdir + "/test.html", "rb") as f:
        assert binopen(f) == f
        runtimeit("binopen(f)")

    with open(tdir + "/test.html", "rb") as f:
        fr = binopen(f.fileno())
        assert isinstance(fr, BufferedReader)

    f = open(tdir + "/test.html", "r")
    fr = binopen(f)
    assert isinstance(fr, BufferedReader)
    runtimeit('binopen(f)')
    fr.close()

    with open(tdir + "/test.html") as f:
        fr = binopen(f)
        isinstance(fr, BufferedReader)
        runtimeit("binopen(f)")

    with binopen(Path(tdir + "/test.html")) as f:
        assert isinstance(f, BufferedReader)
        runtimeit("binopen(f)")

    with BytesIO(b"test") as f:
        try: f.name
        except AttributeError: pass
        assert binopen(f) == f
        assert f.name == None
        runtimeit("binopen(f)")

    with StringIO("test") as f:
        bio = binopen(f)
        isinstance(bio, BytesIO)
        try: f.name
        except AttributeError: pass
        bio.name == None
        runtimeit("binopen(f)")

    with bz2.open(tdir + "/test.csv.bz2") as f:
        assert isinstance(binopen(f), bz2.BZ2File)
        runtimeit("binopen(f)")

    with gzip.open(tdir + "/test.csv.gz") as f:
        assert isinstance(binopen(f), gzip.GzipFile)
        runtimeit("binopen(f)")

    with lzma.open(tdir + "/test.csv.xz") as f:
        assert isinstance(binopen(f), lzma.LZMAFile)
        runtimeit("binopen(f)")

    with ZipFile(tdir + "/test.zip") as f:
        z = f.open(f.infolist()[0])
        assert isinstance(binopen(z), ZipExtFile)
        runtimeit("binopen(z)")

    with tarfile.open(tdir + "/test.tar") as f:
        n = f.getmembers()[0].name
        z = f.extractfile(n)
        assert isinstance(binopen(z), tarfile.ExFileObject)
        runtimeit("binopen(z)")

    with urllib.request.urlopen("https://www.google.com") as res:
        assert isinstance(binopen(res), http.client.HTTPResponse)

    with binopen("abc" * 1024) as f:
        assert isinstance(f, BytesIO)
        assert f.getvalue() == b"abc" * 1024

    with binopen(b"abc" * 1024) as f:
        assert isinstance(f, BytesIO)
        assert f.getvalue() == b"abc" * 1024

    with binopen("https://www.google.com") as res:
        assert isinstance(binopen(res), http.client.HTTPResponse)


def test_binopen_thirdparty_opener():
    global z, dat
    try:
        from rarfile import RarFile, DirectReader
        with RarFile(tdir + "/test.rar") as f:
            n = f.infolist()[0]
            z = f.open(n)
            assert isinstance(binopen(z), DirectReader)
            runtimeit("binopen(z)")
            z.close()
    except ImportError:
        pass

    try:
        from lhafile import LhaFile
        with open(tdir + "/test.lzh", "rb") as fp:
            f = LhaFile(fp)
            info = f.namelist()[0]
            dat = f.read(info)
            assert isinstance(binopen(dat), BytesIO)
            runtimeit("binopen(dat)")
    except ImportError:
        pass

def test_binopen_iragular():
    try:binopen(tdir + "nothing.txt")
    except FileNotFoundError: pass
    try : binopen(None)
    except ValueError: pass
    try : binopen(1.01)
    except ValueError: pass
    try : binopen()
    except TypeError: pass
    try : binopen([tdir + "/test.html"])
    except ValueError: pass
    try : binopen([])
    except ValueError: pass
    try : binopen(())
    except ValueError: pass

def test_opener_regular():
    import urllib.request
    import http.client
    global f, z
    with opener(tdir + "/test.html", "r") as f:
        isinstance(f, TextIOWrapper)
    runtimeit('opener(tdir + "/test.html", "r")')

    with open(tdir + "/test.html", "r") as f:
        assert opener(f) == f
        runtimeit("opener(f)")

    with open(tdir + "/test.html", "r") as f:
        fd = f.fileno()
        fr = opener(fd)
        assert isinstance(fr, TextIOWrapper)
    del fr

    with open(tdir + "/test.html", "rb") as f:
        fr = opener(f)
        assert isinstance(fr, TextIOWrapper)
        runtimeit('opener(f)')
    del fr

    with open(tdir + "/test.html") as f:
        fr = opener(f)
        isinstance(fr, TextIOWrapper)
        runtimeit("opener(f)")

    with opener(Path(tdir + "/test.html")) as f:
        assert isinstance(f, TextIOWrapper)
        runtimeit("opener(f)")

    with StringIO("test") as f:
        try: f.name
        except AttributeError: pass
        assert opener(f) == f
        assert f.name == None
        runtimeit("opener(f)")

    with BytesIO(b"test") as f:
        sio = opener(f)
        isinstance(sio, StringIO)
        try: f.name
        except AttributeError: pass
        assert sio.name == None
        runtimeit("opener(f)")

    with bz2.open(tdir + "/test.csv.bz2") as f:
        assert isinstance(opener(f), bz2.BZ2File)
        runtimeit("opener(f)")

    with gzip.open(tdir + "/test.csv.gz") as f:
        assert isinstance(opener(f), gzip.GzipFile)
        runtimeit("opener(f)")

    with lzma.open(tdir + "/test.csv.xz") as f:
        assert isinstance(opener(f), lzma.LZMAFile)
        runtimeit("opener(f)")

    with ZipFile(tdir + "/test.zip") as f:
        z = f.open(f.infolist()[0])
        assert isinstance(opener(z), ZipExtFile)
        runtimeit("opener(z)")

    with tarfile.open(tdir + "/test.tar") as f:
        n = f.getmembers()[0].name
        z = f.extractfile(n)
        assert isinstance(opener(z), tarfile.ExFileObject)
        runtimeit("opener(z)")

    with urllib.request.urlopen("https://www.google.com") as res:
        assert isinstance(opener(res), http.client.HTTPResponse)

    with opener("abc" * 1024) as f:
        assert isinstance(f, StringIO)
        assert f.getvalue() == "abc" * 1024

    with opener(b"abc" * 1024) as f:
        assert isinstance(f, StringIO)
        assert f.getvalue() == "abc" * 1024

    with opener("https://www.google.com") as res:
        assert isinstance(opener(res), http.client.HTTPResponse)

def test_opener_thirdparty_opener():
    global z, dat
    try:
        from rarfile import RarFile, DirectReader
        with RarFile(tdir + "/test.rar") as f:
            n = f.infolist()[0]
            z = f.open(n)
            assert isinstance(opener(z), DirectReader)
            runtimeit("opener(z)")
            z.close()
    except ImportError:
        pass

    try:
        from lhafile import LhaFile
        with open(tdir + "/test.lzh", "rb") as fp:
            f = LhaFile(fp)
            info = f.namelist()[0]
            dat = f.read(info)
            assert isinstance(opener(dat), StringIO)
            runtimeit("opener(dat)")
    except ImportError:
        pass

def test_opener_iragular():
    try:opener(tdir + "nothing.txt")
    except FileNotFoundError: pass
    try : opener(None)
    except ValueError: pass
    try : opener(1.01)
    except ValueError: pass
    try : opener()
    except TypeError: pass
    try : opener([tdir + "/test.html"])
    except ValueError: pass
    try : opener([])
    except ValueError: pass
    try : opener(())
    except ValueError: pass

def test_headtail():
    global f, anser_0, anser_1, anser_5, anser_10, anser_full
    anser_0    = b''
    anser_1    = b'n\n'
    anser_5    = b'n,aa\r,\x82\xa0\r\n'
    anser_10   = b'n,aa\r\n1,\r\n2,\x82\xa0\r\n'
    anser_full = b'n,aa\r\n1,1\r\n2,\x82\xa0\r\n'

    assert(headtail(anser_full, 10) == anser_full)
    assert(headtail(anser_full, 17) == anser_full)
    assert(headtail(anser_full, 18) == anser_full)
    assert(headtail(anser_full, 5) == anser_5)
    assert(headtail(anser_full, 1) == anser_1)
    assert(headtail(anser_full, 0) == anser_0)
    try: headtail(anser_full, -1)
    except ValueError: pass
    runtimeit("headtail(anser_full, 10)")
    runtimeit("headtail(anser_full, 17)")
    runtimeit("headtail(anser_full, 5)")

    with open(tdir + "/test.csv", "rb") as f:
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with open(tdir + "/test.csv", "r") as f:
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with open(tdir + "/test.csv", "rb") as f:
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with bz2.open(tdir + "/test.csv.bz2") as f:
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with gzip.open(tdir + "/test.csv.gz") as f:
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with lzma.open(tdir + "/test.csv.xz") as f:
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with ZipFile(tdir + "/test.zip") as z:
        f = z.open(z.infolist()[0])
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

    with tarfile.open(tdir + "/test.tar") as z:
        n = z.getmembers()[0].name
        f = z.extractfile(n)
        assert(headtail(f, 10) == anser_full)
        assert(headtail(f, 17) == anser_full)
        assert(headtail(f, 18) == anser_full)
        assert(headtail(f, 5) == anser_5)
        assert(headtail(f, 1) == anser_1)
        assert(headtail(f, 0) == anser_0)
        try: headtail(f, -1)
        except ValueError: pass
        runtimeit("headtail(f, 10)")
        runtimeit("headtail(f, 17)")
        runtimeit("headtail(f, 5)")

def test_sniffer_byteslike():
    global dat
    with open(tdir + "/diff1.csv", "rb") as f:
        dat = f.read()
        defaultvalue_anser = {'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False}
        assert(sniffer(dat) == defaultvalue_anser)
        assert(sniffer(dat, delimiters="\t;:| ") == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=b"\t;:| ") == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=list("\t;:| ")) == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=tuple("\t;:| ")) == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=["\t;:", "| "]) == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, with_encoding=False) == {'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, with_encoding=True) == {'encoding': 'CP932', 'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, -1, with_encoding=False) == {'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, -1, with_encoding=True) == {'encoding': 'CP932', 'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters="") == defaultvalue_anser)
        assert(sniffer(dat, delimiters=[]) == defaultvalue_anser)
        runtimeit("sniffer(dat, with_encoding=False)")
        runtimeit("sniffer(dat, with_encoding=True)")
        runtimeit("sniffer(dat, -1, with_encoding=False)")
        runtimeit("sniffer(dat, -1, with_encoding=True)")

def test_sniffer_str():
    global dat
    with open(tdir + "/diff1.csv", encoding="CP932", newline="\r\n") as f:
        dat = f.read()
        defaultvalue_anser = {'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False}
        assert(sniffer(dat) == defaultvalue_anser)
        assert(sniffer(dat, delimiters="\t;:| ") == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=b"\t;:| ") == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=list("\t;:| ")) == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=tuple("\t;:| ")) == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters=["\t;:", "| "]) == {'delimiter': ' ', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, with_encoding=False) == {'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, with_encoding=True) == {'encoding': "UTF-8", 'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, -1, with_encoding=False) == {'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, -1, with_encoding=True) == {'encoding': "UTF-8", 'delimiter': ',', 'doublequote': True, 'escapechar': None, 'lineterminator': '\r\n', 'quotechar': '"', 'quoting': 0, 'skipinitialspace': False, 'strict': False})
        assert(sniffer(dat, delimiters="") == defaultvalue_anser)
        assert(sniffer(dat, delimiters=[]) == defaultvalue_anser)
        runtimeit("sniffer(dat, with_encoding=False)")
        runtimeit("sniffer(dat, with_encoding=True)")
        runtimeit("sniffer(dat, -1, with_encoding=False)")
        runtimeit("sniffer(dat, -1, with_encoding=True)")

def test_sniffer_iregular():
    try:sniffer()
    except TypeError:pass
    else: raise AssertionError
    try:sniffer("")
    except ValueError:pass
    else: raise AssertionError
    try:sniffer(b"")
    except ValueError:pass
    else: raise AssertionError
    try:sniffer(123)
    except TypeError:pass
    else: raise AssertionError
    try:sniffer([0,1,2,3])
    except TypeError:pass
    else: raise AssertionError
    try:sniffer(None)
    except TypeError:pass
    else: raise AssertionError
    try:sniffer(dat, delimiters=None)
    except TypeError:pass
    else: raise AssertionError
    try:sniffer(dat, delimiters=123)
    except TypeError:pass
    else: raise AssertionError
    try:sniffer(dat, delimiters=[0,1,2])
    except TypeError:pass
    else: raise AssertionError

def test_flatten():
    assert(flatten([[1, 2], [3, 4], [[5, 6]]]) == [1, 2, 3, 4, 5, 6])
    assert(flatten("abc") == ['a', 'b', 'c'])
    assert(flatten(u"あいう") == [u'あ', u'い', u'う'])
    assert(flatten(1) == [1])
    runtimeit('flatten([[1,2], [3,4], [[5, 6]]])')

def test_which():
    assert(which("find").lower() in ["c:\\windows\\system32\\find.exe", "/usr/bin/find", "/bin/find"])
    runtimeit('which("find")')

def test_Counter():
    assert(Counter([1,2,2,3,4]) == {1: 1, 2: 2, 3: 1, 4: 1})
    assert(Counter([(1,2), (1,3), (2, 1, 4), (3, None)]) == {(1, 2): 1, (1, 3): 1, (2, 1, 4): 1, (3, None): 1})
    assert(Counter([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: x) == {(1, 2): 1, (1, 3): 1, (2, 1, 4): 1, (3, None): 1})
    assert(Counter([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: x[0]) == {1: 2, 2: 1, 3: 1})
    runtimeit("Counter([1,2,2,3,4])")
    runtimeit("Counter([(1,2), (1,3), (2, 1, 4), (3, None)]) == {(1, 2): 1, (1, 3): 1, (2, 1, 4): 1, (3, None): 1}")
    runtimeit("Counter([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: x) == {(1, 2): 1, (1, 3): 1, (2, 1, 4): 1, (3, None): 1}")
    runtimeit("Counter([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: x[0]) == {1: 2, 2: 1, 3: 1}")

def test_Counter_Errorhandle():
    try: Counter(None)
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")

    try: Counter()
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")

def test_Grouper():
    assert(Grouper([(1,2), (1,3), (2, 1, 4), (3, None)]) == {1: [2, 3], 2: [(1, 4)], 3: [None]})
    assert(Grouper([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: f"key{x[0]}") == {'key1': [2, 3], 'key2': [(1, 4)], 'key3': [None]})
    assert(Grouper([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: f"key{x[0]}", valfunc=lambda x:x[1]) == {'key1': [2, 3], 'key2': [1], 'key3': [None]})
    runtimeit("Grouper([(1,2), (1,3), (2, 1, 4), (3, None)])")
    runtimeit('Grouper([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: f"key{x[0]}")')
    runtimeit('Grouper([(1,2), (1,3), (2, 1, 4), (3, None)], keyfunc=lambda x: f"key{x[0]}", valfunc=lambda x:x[1])')

def test_Grouper_Errorhandle():
    try: Grouper([1,2,3])
    except ValueError: pass
    else: raise AssertionError("Bad Errorhandling")

    try: Grouper(None)
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")

    try: Grouper()
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")

def test_uniq():
    global data, data2
    data = [1,2,2,3,2] * 100
    assert(uniq(data) == [1,2,3])
    data2 = [[1,"a"], [1,"b"], [1,"c"], [2,"d"], [3,"e"]]
    assert(uniq(data2) == data2)
    assert(uniq(data2, lambda x: x[0]) == [[1, 'a'], [2, 'd'], [3, 'e']])
    assert(uniq(data2, lambda x: x[0], select_first=False) == [[1, 'c'], [2, 'd'], [3, 'e']])
    assert(uniq("123") == ["1", "2", "3"])
    runtimeit('uniq(data)')
    runtimeit('uniq(data2)')

def test_uniq_Errorhandle():
    try: uniq()
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")

    try: uniq(None)
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")

    try: uniq(1)
    except TypeError: pass
    else: raise AssertionError("Bad Errorhandling")



def test_listify():
    assert(listify("1") == ['1'])
    runtimeit("listify('1')==['1']")


def test_to_hankaku():
    assert(to_hankaku(u"１２３") == u'123')
    assert(to_hankaku(u"1あ!#ア ２") == u"1あ!#ｱ 2")
    runtimeit('to_hankaku(u"１")')


def test_to_zenkaku():
    assert(to_zenkaku(u"1") == u"１")
    assert(to_zenkaku(u"1あア!# ２") == u"１あア！＃　２")
    assert(to_zenkaku(u"\"") == u"＂")
    runtimeit('to_zenkaku(u"1")')

def test_lookuptype():
    assert(lookuptype(b"   \x20\xef\xbb\xbf<?xml version>hogejkflkdsfkja;l?>") == "xml")
    assert(lookuptype(b"hoge") == "txt")
    assert(lookuptype(b'PK\x03\x04dsfal\x00') == "zip")
    assert(lookuptype(b"hogegggggggggggggggg;gggggrecord  end;") == "dml")
    runtimeit('lookuptype(b"hoge")')
    runtimeit("lookuptype(b'PK\\x03\\x04dsfal\\x00')")

def _test_guesstype_auto_assertion(fn, expected = None):
    global f, anser
    anser = expected or fn.split(".")[-1].lower()

    print(f"guesstype Runtest start #### {fn} ####")

    with open(fn, "rb") as f:
        assert(guesstype(f) == anser)
        assert(guesstype(f.read()) == anser)
        f.seek(0)
        assert(guesstype(f.name) == anser)
        runtimeit("guesstype(f) == anser")
        runtimeit("guesstype(f.read()) == anser")
        runtimeit("guesstype(f.name) == anser")
    
    f = Path(fn)
    assert(guesstype(f) == anser)
    runtimeit("guesstype(f) == anser")

def test_guesstype():
    _test_guesstype_auto_assertion(tdir + "/diff1.csv")
    _test_guesstype_auto_assertion(tdir + "/diff1.xlsx")
    _test_guesstype_auto_assertion(tdir + "/iostat.log", "txt")
    _test_guesstype_auto_assertion(tdir + "/mpstat.log", "txt")
    _test_guesstype_auto_assertion(tdir + "/sa06", "sarbin")
    _test_guesstype_auto_assertion(tdir + "/sample.accdb")
    _test_guesstype_auto_assertion(tdir + "/sample.mdb")
    _test_guesstype_auto_assertion(tdir + "/sample.sqlite3")
    _test_guesstype_auto_assertion(tdir + "/sar.dat", "sarbin")
    _test_guesstype_auto_assertion(tdir + "/test.csv")
    _test_guesstype_auto_assertion(tdir + "/test.csv.bz2")
    _test_guesstype_auto_assertion(tdir + "/test.csv.gz")
    _test_guesstype_auto_assertion(tdir + "/test.csv.tar")
    _test_guesstype_auto_assertion(tdir + "/test.csv.tar.gz")
    _test_guesstype_auto_assertion(tdir + "/test.csv.xz")
    _test_guesstype_auto_assertion(tdir + "/test.dml")
    _test_guesstype_auto_assertion(tdir + "/test.doc")
    _test_guesstype_auto_assertion(tdir + "/test.docx")
    _test_guesstype_auto_assertion(tdir + "/test.hdf")
    _test_guesstype_auto_assertion(tdir + "/test.html")
    _test_guesstype_auto_assertion(tdir + "/test.ini", "txt")
    _test_guesstype_auto_assertion(tdir + "/test.json")
    _test_guesstype_auto_assertion(tdir + "/test.lzh", "lha")
    _test_guesstype_auto_assertion(tdir + "/test.md", "txt")
    _test_guesstype_auto_assertion(tdir + "/test.pcap")
    _test_guesstype_auto_assertion(tdir + "/test.pdf")
    _test_guesstype_auto_assertion(tdir + "/test.ppt")
    _test_guesstype_auto_assertion(tdir + "/test.pptx")
    _test_guesstype_auto_assertion(tdir + "/test.rar")
    _test_guesstype_auto_assertion(tdir + "/test.tsv", "csv")
    _test_guesstype_auto_assertion(tdir + "/test.xls")
    _test_guesstype_auto_assertion(tdir + "/test.xml")
    _test_guesstype_auto_assertion(tdir + "/test.zip")
    _test_guesstype_auto_assertion(tdir + "/test_utf8.csv")
    _test_guesstype_auto_assertion(tdir + "/vmstat.log", "txt")
    _test_guesstype_auto_assertion(tdir + "/zabbix_template.xml")
    _test_guesstype_auto_assertion(tdir + "/zero.txt", "ZERO")
    assert(guesstype("http://www.example.com") == "html")
    assert(guesstype("https://www.example.com") == "html")
    assert(guesstype(b"") == "ZERO")

def test_guesstype_iregular_common():
    print("guesstype iregurar test Nothing##")
    try: guesstype("NothingFilepath")
    except FileNotFoundError: pass
    else: raise AssertionError
    print("guesstype iregurar test None##")
    try: guesstype(None)
    except ValueError: pass
    else: raise AssertionError
    print("guesstype iregurar test int##")
    try: guesstype(1)
    except OSError: pass
    else: raise AssertionError
    print("guesstype iregurar test float##")
    try: guesstype(1.1)
    except ValueError: pass
    else: raise AssertionError
    print("guesstype iregurar test list##")
    try: guesstype([1])
    except ValueError: pass
    else: raise AssertionError
    print("guesstype iregurar test tuple##")
    try: guesstype((1,2))
    except ValueError: pass
    else: raise AssertionError
    print("guesstype iregurar test empty##")
    try: guesstype()
    except TypeError: pass
    else: raise AssertionError

def test_kanji2int():
    assert(kanji2int(u"一億２千") == "100002000")
    runtimeit('kanji2int(u"一億２千")')


def test_int2kanji():
    assert(int2kanji(123456789) == u"一億二千三百四十五万六千七百八十九")
    runtimeit('int2kanji(123456789)')


def test_to_datetime():
    import shutil
    shutil.rmtree(os.path.join(os.environ.get("TMP", "/tmp"), "dat"), True)
    assert(to_datetime('2000/01/01') == datetime(2000, 1, 1))
    assert(to_datetime('1999/01/01') == datetime(1999, 1, 1))
    assert(to_datetime('20060314') == datetime(2006, 3, 14))
    assert(to_datetime('2006/03/14') == datetime(2006, 3, 14))
    assert(to_datetime('14/03/2006') == datetime(2006, 3, 14))
    assert(to_datetime('03/14/2006') == datetime(2006, 3, 14))
    assert(to_datetime('2006/3/3') == datetime(2006, 3, 3))
    assert(to_datetime('2006-03-14') == datetime(2006, 3, 14))
    assert(to_datetime('03-14-2006') == datetime(2006, 3, 14))
    assert(to_datetime('14.03.2006') == datetime(2006, 3, 14))
    assert(to_datetime('03.14.2006') == datetime(2006, 3, 14))
    assert(to_datetime('03-Mar-06') == datetime(2006, 3, 3))
    assert(to_datetime('14-Mar-06') == datetime(2006, 3, 14))
    assert(to_datetime('03-Mar-2006') == datetime(2006, 3, 3))
    assert(to_datetime('14-Mar-2006') == datetime(2006, 3, 14))
    assert(to_datetime('14-03-2006') == datetime(2006, 3, 14))
    assert(to_datetime('20061403', dayfirst=True) == datetime(2006, 3, 14))
    assert(to_datetime('2006/14/03', dayfirst=True) == datetime(2006, 3, 14))
    assert(to_datetime('2006-14-03', dayfirst=True) == datetime(2006, 3, 14))
    assert(to_datetime('13 27 54 123') == datetime(1970, 1, 1, 13, 27, 54, 123000))
    assert(to_datetime('13 27 54') == datetime(1970, 1, 1, 13, 27, 54))
    assert(to_datetime('13.27.54.123') == datetime(1970, 1, 1, 13, 27, 54, 123000))
    assert(to_datetime('13.27.54') == datetime(1970, 1, 1, 13, 27, 54))
    assert(to_datetime('13:27:54.123') == datetime(1970, 1, 1, 13, 27, 54, 123000))
    assert(to_datetime('13:27:54') == datetime(1970, 1, 1, 13, 27, 54))
    assert(to_datetime('1327') == datetime(1970, 1, 1, 13, 27))
    assert(to_datetime('132754') == datetime(1970, 1, 1, 13, 27, 54))
    assert(to_datetime('132754123') == datetime(1970, 1, 1, 13, 27, 54, 123000))
    assert(to_datetime('20060314 13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('2006/03/14 13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('14/03/2006 13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('20060314 13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('2006/03/14 13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('14/03/2006 13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('2006-03-14 13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('14-03-2006 13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('2006-03-14 13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('14-03-2006 13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('2006-03-14T13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('2006-03-14T13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('20060314T13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('20060314T13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('14-03-2006T13:27:54.123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('14-03-2006T13:27:54') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('20060314T1327') == datetime(2006, 3, 14, 13, 27))
    assert(to_datetime('20060314T132754') == datetime(2006, 3, 14, 13, 27, 54))
    assert(to_datetime('20060314T132754123') == datetime(2006, 3, 14, 13, 27, 54, 123000))
    assert(to_datetime('08-24-2001 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('Friday, August 24th, 2001 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('Fri Aug. 24, 2001 8:10 p.m.') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('Aug. 24, 2001 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('2001/08/24 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('2001/08/24 2010') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime(u'2001年8月24日金曜日 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime(u'2001年8月24日(金) 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime(u'3月 25 00:40') == datetime(1970, 3, 25, 0, 40))
    assert(to_datetime(u'11月 29  2018') == datetime(2018, 11, 29))
    assert(to_datetime(u'1月 16  2019') == datetime(2019, 1, 16))
    assert(to_datetime("1999/12/31") == datetime(1999, 12, 31))
    if sys.version_info[:2] >= (3, 7):
        assert(to_datetime('2006-03-14T13:27:54+03:45') == datetime(2006, 3, 14, 13, 27, 54, tzinfo=timezone(timedelta(hours=3, minutes=45))))
        assert(to_datetime('2006-03-14T13:27+03:45') == datetime(2006, 3, 14, 13, 27, tzinfo=timezone(timedelta(hours=3, minutes=45))))
        assert(to_datetime('14/Mar/2006:13:27:54 -0537') == datetime(2006, 3, 14, 13, 27, 54, tzinfo=timezone(timedelta(hours=-5, minutes=-37))))
        assert(to_datetime('Sat, 14 Mar 2006 13:27:54 GMT') == datetime(2006, 3, 14, 13, 27, 54, tzinfo=timezone(timedelta(seconds=0))))
        assert(to_datetime('平成１３年８月２４日　午後八時十分') == datetime(2001, 8, 24, 20, 10, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime('平成13年08月24日PM 08:10') == datetime(2001, 8, 24, 20, 10, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime('H13年08月24日　PM08:10') == datetime(2001, 8, 24, 20, 10, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime('平13年08月24日　午後8:10') == datetime(2001, 8, 24, 20, 10, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime('平成13年08/24午後08:10') == datetime(2001, 8, 24, 20, 10, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime('平成元年０８月２４日　２０時１０分００秒') == datetime(1989, 8, 24, 20, 10, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime("平成一年一月十一日") == datetime(1989, 1, 11, tzinfo=timezone(timedelta(hours=9))))
        assert(to_datetime('天正10年6月2日') == datetime(1582, 6, 2, tzinfo=timezone(timedelta(hours=9))))
    else:
        assert(to_datetime('2006-03-14T13:27:54+03:45') == datetime(2006, 3, 14, 13, 27, 54))
        assert(to_datetime('2006-03-14T13:27+03:45') == datetime(2006, 3, 14, 13, 27))
        assert(to_datetime('14/Mar/2006:13:27:54 -0537') == datetime(2006, 3, 14, 13, 27, 54))
        assert(to_datetime('Sat, 14 Mar 2006 13:27:54 GMT') == datetime(2006, 3, 14, 13, 27, 54))
        assert(to_datetime(u'平成１３年８月２４日　午後八時十分') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime(u'平成13年08月24日PM 08:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime(u'H13年08月24日　PM08:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime(u'平13年08月24日　午後8:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime(u'平成13年08/24午後08:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime(u'平成元年０８月２４日　２０時１０分００秒') == datetime(1989, 8, 24, 20, 10))
        assert(to_datetime(u"平成一年一月十一日") == datetime(1989, 1, 11))
        assert(to_datetime(u'天正10年6月2日') == datetime(1582, 6, 2))

    test = u"""
    ====== ここからWikipediaの織田信長から引用文 ======
    織田 信長（おだ のぶなが、天文3年5月12日〈1534年6月23日〉 - 天正10年6月2日〈1582年6月21日〉）は、
    日本の戦国時代から安土桃山時代にかけての武将、戦国大名。三英傑の一人。

    尾張国（現在の愛知県）の織田信秀の嫡男。家督争いの混乱を収めた後に、
    桶狭間の戦いで今川義元を討ち取り、勢力を拡大した。足利義昭を奉じて上洛し、後には義昭を追放することで、
    畿内を中心に独自の中央政権（「織田政権」[注釈 4]）を確立して天下人となった。
    しかし天正10年6月2日（1582年6月21日）、重臣・明智光秀に謀反を起こされ、本能寺で自害した。
    これまで信長の政権は、豊臣秀吉による豊臣政権、徳川家康が開いた江戸幕府へと引き継がれていく、
    画期的なものであったとみなされてきた[2]。
    しかし、政策の実態などから「中世社会の最終段階」ともしばしば評され[2]、
    特に近年の歴史学界では信長の革新性を否定する研究が主流となっている[3][4]。


    Oda Nobunaga (織田 信長, About this soundlisten; 3 Jul 1534 - 21 Jun, 1582) was
    a Japanese daimyo and one of the leading figures of the Sengoku period. 
    He is regarded as the first "Great Unifier" of Japan. 
    His reputation in war gave him the nickname of "Demon Daimyo" or "Demon King".

    Nobunaga was head of the very powerful Oda clan, 
    and launched a war against other daimyos to unify Japan in the 1560s. 
    Nobunaga emerged as the most powerful daimyo, 
    overthrowing the nominally ruling shogun Ashikaga Yoshiaki and 
    dissolving the Ashikaga Shogunate in 1573. He conquered most of Honshu island by 1580,
    and defeated the Ikko-ikki rebels by the 1580s.
    Nobunaga's rule was noted for innovative military tactics, fostering free trade,
    reform of Japan's civil government, and encouraging the start of the Momoyama historical art period, 
    but also for the brutal suppression of opponents, eliminating those who refused to cooperate or
    yield to his demands. Nobunaga was killed in the Honno-ji Incident in 1582 when his retainer 
    Akechi Mitsuhide ambushed him in Kyoto and forced him to commit seppuku.
    Nobunaga was succeeded by Toyotomi Hideyoshi who along with Tokugawa Ieyasu completed his war of
    unification shortly afterwards.

    """

    ans = u"""
    ====== ここからWikipediaの織田信長から引用文 ======
    織田 信長（おだ のぶなが、1534/05/12 00:00:00〈1534/06/23 00:00:00〉 - 1582/06/02 00:00:00〈1582/06/21 00:00:00〉）は、
    日本の戦国時代から安土桃山時代にかけての武将、戦国大名。三英傑の一人。

    尾張国（現在の愛知県）の織田信秀の嫡男。家督争いの混乱を収めた後に、
    桶狭間の戦いで今川義元を討ち取り、勢力を拡大した。足利義昭を奉じて上洛し、後には義昭を追放することで、
    畿内を中心に独自の中央政権（「織田政権」[注釈 4]）を確立して天下人となった。
    しかし1582/06/02 00:00:00（1582/06/21 00:00:00）、重臣・明智光秀に謀反を起こされ、本能寺で自害した。
    これまで信長の政権は、豊臣秀吉による豊臣政権、徳川家康が開いた江戸幕府へと引き継がれていく、
    画期的なものであったとみなされてきた[2]。
    しかし、政策の実態などから「中世社会の最終段階」ともしばしば評され[2]、
    特に近年の歴史学界では信長の革新性を否定する研究が主流となっている[3][4]。


    Oda Nobunaga (織田 信長, About this soundlisten; 1534/07/03 21:00:00) was
    a Japanese daimyo and one of the leading figures of the Sengoku period. 
    He is regarded as the first "Great Unifier" of Japan. 
    His reputation in war gave him the nickname of "Demon Daimyo" or "Demon King".

    Nobunaga was head of the very powerful Oda clan, 
    and launched a war against other daimyos to unify Japan in the 1560s. 
    Nobunaga emerged as the most powerful daimyo, 
    overthrowing the nominally ruling shogun Ashikaga Yoshiaki and 
    dissolving the Ashikaga Shogunate in 1573. He conquered most of Honshu island by 1580,
    and defeated the Ikko-ikki rebels by the 1580s.
    Nobunaga's rule was noted for innovative military tactics, fostering free trade,
    reform of Japan's civil government, and encouraging the start of the Momoyama historical art period, 
    but also for the brutal suppression of opponents, eliminating those who refused to cooperate or
    yield to his demands. Nobunaga was killed in the Honno-ji Incident in 1582 when his retainer 
    Akechi Mitsuhide ambushed him in Kyoto and forced him to commit seppuku.
    Nobunaga was succeeded by Toyotomi Hideyoshi who along with Tokugawa Ieyasu completed his war of
    unification shortly afterwards.

    """
    assert(normalized_datetime(test) == ans)

    runtimeit('to_datetime(u"平成13年08月24日PM 08:10")')
    runtimeit('normalized_datetime(u"hogegefooほげ平成13年08月24日PM 08:10むう")')
    runtimeit('extractdate(u"hogegefooほげ平成13年08月24日PM 08:10むう")')

def _test_expect_ValueError(val):
    try:
        to_datetime(val)
        assert(False)
    except ValueError:
        pass
    except:
        assert(False)

def test_error_datetime():
    _test_expect_ValueError(None)
    _test_expect_ValueError(1)
    _test_expect_ValueError(1.0)
    _test_expect_ValueError(int)
    _test_expect_ValueError([])
    _test_expect_ValueError([1])
    _test_expect_ValueError((1,))
    assert(to_datetime(b"2020/01/01") == datetime(2020, 1, 1))
    assert(to_datetime("") == None)
    assert(to_datetime("hoge") == None)
    assert(to_datetime(u"ho123年ge") == None)


if __name__ == '__main__':
    import os
    import traceback

    curdir = os.getcwd()
    try:
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        for fn, func in dict(locals()).items():
            if fn.startswith("test_"):
                print("Runner: %s" % fn)
                func()
    except Exception as e:
        traceback.print_exc()
        raise (e)
    finally:
        os.chdir(curdir)
