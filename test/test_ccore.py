#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from timeit import timeit
from datetime import datetime, timezone, timedelta
import os
import sys
from psutil import virtual_memory, Process
process = Process(os.getpid())

IMPORTS = ' import flatten, listify, lookuptype, to_hankaku, to_zenkaku, kanji2int, int2kanji, to_datetime, extractdate, normalized_datetime'
try:
    from ccore.ccore import *
    smip = 'from ccore.ccore'
except:
    try:
        from ccore.build.ccore import *
        smip = 'from ccore.build.ccore'
    except:
        try:
            from build.ccore import *
            smip = 'from build.ccore'
        except:
            from ccore import *
            smip = 'from ccore'

smip += IMPORTS

def memusage():
    return process.memory_info()[0] / 1024

def runtimeit(funcstr, setup=smip, number=100000, normalize=10000):
    st = setup.strip()
    i = 0
    for fc in funcstr.strip().splitlines():
        fc = fc.strip()
        if i == 0:
            timeit(fc, st, number=number)
        bm = memusage()
        p = timeit(fc, st, number=number)
        am = (memusage() - bm)
        try:
            print("{}: {} ns (mem after {}KB)".format(fc, int(p * normalize), am))
        except UnicodeEncodeError:
            print("???: {} ns (mem after {}KB)".format(int(p * normalize), am))
        i += 1


def test_flatten():
    assert(flatten([[1, 2], [3, 4], [[5, 6]]]) == [1, 2, 3, 4, 5, 6])
    assert(flatten("abc") == ['a', 'b', 'c'])
    assert(flatten("あいう") == ['あ', 'い', 'う'])
    assert(flatten(1) == [1])
    runtimeit('flatten([[1,2], [3,4], [[5, 6]]])')


def test_listify():
    assert(listify("1") == ['1'])
    runtimeit("listify('1')==['1']")


def test_to_hankaku():
    assert(to_hankaku("１２３") == '123')
    assert(to_hankaku("1あ!#ア ２") == "1あ!#ｱ 2")
    runtimeit('to_hankaku("１")')


def test_to_zenkaku():
    assert(to_zenkaku("1") == "１")
    assert(to_zenkaku("1あア!# ２") == "１あア！＃　２")
    assert(to_zenkaku("\"") == "＂")
    runtimeit('to_zenkaku("1")')


def test_lookuptype():
    assert(lookuptype(b"   \x20\xef\xbb\xbf<?xml version>hogejkflkdsfkja;l?>") == "xml")
    assert(lookuptype(b"hoge") == "txt")
    assert(lookuptype(b'PK\x03\x04dsfal\x00') == "zip")
    assert(lookuptype(b"hogegggggggggggggggg;gggggrecord  end;") == "dml")
    runtimeit('lookuptype(b"hoge")')
    runtimeit("lookuptype(b'PK\\x03\\x04dsfal\\x00')")


def test_kanji2int():
    assert(kanji2int("一億２千") == "100002000")
    runtimeit('kanji2int("一億２千")')


def test_int2kanji():
    assert(int2kanji(123456789) == "一億二千三百四十五万六千七百八十九")
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
    assert(to_datetime('2001年8月24日金曜日 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('2001年8月24日(金) 20:10') == datetime(2001, 8, 24, 20, 10))
    assert(to_datetime('3月 25 00:40') == datetime(1970, 3, 25, 0, 40))
    assert(to_datetime('11月 29  2018') == datetime(2018, 11, 29))
    assert(to_datetime('1月 16  2019') == datetime(2019, 1, 16))
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
        assert(to_datetime('平成１３年８月２４日　午後八時十分') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime('平成13年08月24日PM 08:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime('H13年08月24日　PM08:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime('平13年08月24日　午後8:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime('平成13年08/24午後08:10') == datetime(2001, 8, 24, 20, 10))
        assert(to_datetime('平成元年０８月２４日　２０時１０分００秒') == datetime(1989, 8, 24, 20, 10))
        assert(to_datetime("平成一年一月十一日") == datetime(1989, 1, 11))
        assert(to_datetime('天正10年6月2日') == datetime(1582, 6, 2))

    test = """
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

    ans = """
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

    runtimeit('to_datetime("平成13年08月24日PM 08:10")')
    runtimeit('normalized_datetime("hogegefooほげ平成13年08月24日PM 08:10むう")')
    runtimeit('extractdate("hogegefooほげ平成13年08月24日PM 08:10むう")')

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
    _test_expect_ValueError(b"2020/01/01")
    assert(to_datetime("") == None)
    assert(to_datetime("hoge") == None)
    assert(to_datetime("ho123年ge") == None)


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
