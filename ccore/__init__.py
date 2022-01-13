# -*- coding: utf-8 -*-
from ._ccore import (
    getencoding,
    binopen,
    opener,
    headtail,
    guesstype,
    sniffer,
    flatten,
    which,
    Counter,
    Grouper,
    uniq,
    listify,
    to_hankaku,
    to_zenkaku,
    kanji2int,
    int2kanji,
    lookuptype,
    is_tar,
    is_lha,
    is_office,
    is_xls,
    is_doc,
    is_ppt,
    is_xml,
    is_html,
    is_json,
    is_dml,
    is_csv,
    g2d,
    to_datetime,
    extractdate,
    normalized_datetime,
    iterhead,
    itertail,
    iterheadtail,
    mklink,
)


from typing import Union, Iterable, Any

def example(o: Union[bytes, bytearray, str],
            hoeg: int,
            foo: bool = False
            ) -> dict:
    """
    # This is Markdown OK
    """

# def sniffer(o: Union[bytes, bytearray, str],
#             maxlines: int,
#             with_encoding: bool = False
#             ) -> dict:
#     """
#     """
