from tempfile import TemporaryDirectory
from os.path import dirname

__all__ = [
    "tdir",
    "TMPDIR"
]

tdir = dirname(__file__)

class _tmpdir(TemporaryDirectory):
    def __del__(self):
        self.cleanup()
_tmp = _tmpdir()
TMPDIR = _tmp.name