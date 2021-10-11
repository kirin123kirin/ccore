#include <Python.h>
#include <datetime.h>
#include <setjmp.h>
#include <array>
#include <ctime>
#include <numeric>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#undef getc
#undef ungetc
#define getc(f) pynkf_getc(f)

#undef putchar
#undef TRUE
#undef FALSE
#define putchar(c) pynkf_putchar(c)

typedef int nkf_char;

static int pynkf_ibufsize, pynkf_obufsize;
static unsigned char *pynkf_inbuf, *pynkf_outbuf;
static int pynkf_icount, pynkf_ocount;
static unsigned char *pynkf_iptr, *pynkf_optr;
static jmp_buf env;
static int pynkf_guess_flag;

static int pynkf_getc(FILE* f) {
    unsigned char c;
    if(pynkf_icount >= pynkf_ibufsize)
        return EOF;
    c = *pynkf_iptr++;
    pynkf_icount++;
    return (int)c;
}

static void pynkf_putchar(int c) {
    std::size_t size;
    unsigned char* p;

    if(pynkf_guess_flag) {
        return;
    }

    if(pynkf_ocount--) {
        *pynkf_optr++ = (unsigned char)c;
    } else {
        size = (std::size_t)(pynkf_obufsize + pynkf_obufsize);
        p = (unsigned char*)PyMem_Realloc(pynkf_outbuf, size + 1);
        if(pynkf_outbuf == NULL) {
            longjmp(env, 1);
        }
        pynkf_outbuf = p;
        pynkf_optr = pynkf_outbuf + pynkf_obufsize;
        pynkf_ocount = pynkf_obufsize;
        pynkf_obufsize = (int)size;
        *pynkf_optr++ = (unsigned char)c;
        pynkf_ocount--;
    }
}

#undef max
template <class T>
struct PyMallocator {
    typedef T value_type;

    PyMallocator() = default;
    template <class U>
    constexpr PyMallocator(const PyMallocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if(n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();
        if(auto p = PyMem_New(T, n)) {
            return p;
        }
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept {
        PyMem_Del(p);
        ;
    }

    bool operator==(const PyMallocator<T>&) { return true; }

    bool operator!=(const PyMallocator<T>&) { return false; }
};

using py_ustring = std::basic_string<wchar_t, std::char_traits<wchar_t>, PyMallocator<wchar_t>>;

static std::unordered_map<wchar_t, const wchar_t*> ZEN2HAN = {
    {L'ァ', L"ｧ"},  {L'ア', L"ｱ"},  {L'ィ', L"ｨ"},  {L'イ', L"ｲ"},  {L'ゥ', L"ｩ"},  {L'ウ', L"ｳ"},  {L'ェ', L"ｪ"},
    {L'エ', L"ｴ"},  {L'ォ', L"ｫ"},  {L'オ', L"ｵ"},  {L'カ', L"ｶ"},  {L'ガ', L"ｶﾞ"}, {L'キ', L"ｷ"},  {L'ギ', L"ｷﾞ"},
    {L'ク', L"ｸ"},  {L'グ', L"ｸﾞ"}, {L'ケ', L"ｹ"},  {L'ゲ', L"ｹﾞ"}, {L'コ', L"ｺ"},  {L'ゴ', L"ｺﾞ"}, {L'サ', L"ｻ"},
    {L'ザ', L"ｻﾞ"}, {L'シ', L"ｼ"},  {L'ジ', L"ｼﾞ"}, {L'ス', L"ｽ"},  {L'ズ', L"ｽﾞ"}, {L'セ', L"ｾ"},  {L'ゼ', L"ｾﾞ"},
    {L'ソ', L"ｿ"},  {L'ゾ', L"ｿﾞ"}, {L'タ', L"ﾀ"},  {L'ダ', L"ﾀﾞ"}, {L'チ', L"ﾁ"},  {L'ヂ', L"ﾁﾞ"}, {L'ッ', L"ｯ"},
    {L'ツ', L"ﾂ"},  {L'ヅ', L"ﾂﾞ"}, {L'テ', L"ﾃ"},  {L'デ', L"ﾃﾞ"}, {L'ト', L"ﾄ"},  {L'ド', L"ﾄﾞ"}, {L'ナ', L"ﾅ"},
    {L'ニ', L"ﾆ"},  {L'ヌ', L"ﾇ"},  {L'ネ', L"ﾈ"},  {L'ノ', L"ﾉ"},  {L'ハ', L"ﾊ"},  {L'バ', L"ﾊﾞ"}, {L'パ', L"ﾊﾟ"},
    {L'ヒ', L"ﾋ"},  {L'ビ', L"ﾋﾞ"}, {L'ピ', L"ﾋﾟ"}, {L'フ', L"ﾌ"},  {L'ブ', L"ﾌﾞ"}, {L'プ', L"ﾌﾟ"}, {L'ヘ', L"ﾍ"},
    {L'ベ', L"ﾍﾞ"}, {L'ペ', L"ﾍﾟ"}, {L'ホ', L"ﾎ"},  {L'ボ', L"ﾎﾞ"}, {L'ポ', L"ﾎﾟ"}, {L'マ', L"ﾏ"},  {L'ミ', L"ﾐ"},
    {L'ム', L"ﾑ"},  {L'メ', L"ﾒ"},  {L'モ', L"ﾓ"},  {L'ャ', L"ｬ"},  {L'ヤ', L"ﾔ"},  {L'ュ', L"ｭ"},  {L'ユ', L"ﾕ"},
    {L'ョ', L"ｮ"},  {L'ヨ', L"ﾖ"},  {L'ラ', L"ﾗ"},  {L'リ', L"ﾘ"},  {L'ル', L"ﾙ"},  {L'レ', L"ﾚ"},  {L'ロ', L"ﾛ"},
    {L'ワ', L"ﾜ"},  {L'ヲ', L"ｦ"},  {L'ン', L"ﾝ"},  {L'ヴ', L"ｳﾞ"}, {L'・', L"･"},  {L'ー', L"ー"}, {L'、', L"､"},
    {L'「', L"｢"},  {L'」', L"｣"},  {L'゛', L"゛"}, {L'゜', L"゜"}};

static const std::unordered_map<wchar_t, wchar_t> han2zen = {
    {L'ｧ', L'ァ'}, {L'ｨ', L'ィ'}, {L'ｩ', L'ゥ'}, {L'ｪ', L'ェ'}, {L'ｫ', L'ォ'}, {L'ｬ', L'ャ'}, {L'ｭ', L'ュ'},
    {L'ｮ', L'ョ'}, {L'ｯ', L'ッ'}, {L'ｰ', L'ー'}, {L'ｱ', L'ア'}, {L'ｲ', L'イ'}, {L'ｳ', L'ウ'}, {L'ｴ', L'エ'},
    {L'ｵ', L'オ'}, {L'ｶ', L'カ'}, {L'ｷ', L'キ'}, {L'ｸ', L'ク'}, {L'ｹ', L'ケ'}, {L'ｺ', L'コ'}, {L'ｻ', L'サ'},
    {L'ｼ', L'シ'}, {L'ｽ', L'ス'}, {L'ｾ', L'セ'}, {L'ｿ', L'ソ'}, {L'ﾀ', L'タ'}, {L'ﾁ', L'チ'}, {L'ﾂ', L'ツ'},
    {L'ﾃ', L'テ'}, {L'ﾄ', L'ト'}, {L'ﾅ', L'ナ'}, {L'ﾆ', L'ニ'}, {L'ﾇ', L'ヌ'}, {L'ﾈ', L'ネ'}, {L'ﾉ', L'ノ'},
    {L'ﾊ', L'ハ'}, {L'ﾋ', L'ヒ'}, {L'ﾌ', L'フ'}, {L'ﾍ', L'ヘ'}, {L'ﾎ', L'ホ'}, {L'ﾏ', L'マ'}, {L'ﾐ', L'ミ'},
    {L'ﾑ', L'ム'}, {L'ﾒ', L'メ'}, {L'ﾓ', L'モ'}, {L'ﾔ', L'ヤ'}, {L'ﾕ', L'ユ'}, {L'ﾖ', L'ヨ'}, {L'ﾗ', L'ラ'},
    {L'ﾘ', L'リ'}, {L'ﾙ', L'ル'}, {L'ﾚ', L'レ'}, {L'ﾛ', L'ロ'}, {L'ﾜ', L'ワ'}, {L'ﾝ', L'ン'}, {L'ｦ', L'ヲ'},
    {L'ﾞ', L'゛'}, {L'ﾟ', L'゜'}, {L'｢', L'「'}, {L'｣', L'」'}, {L'､', L'、'}, {L'･', L'・'},
};

/* for filetype.hpp */

#define ITEMSIZE 20

struct dic {
    const char* key;
    const char* val;
    std::size_t size;
    dic() : key(0), val(0), size(0) {}
    dic(std::nullptr_t) : key(0), val(0), size(0) {}
    dic(const char* _k, const char* _v, std::size_t _s) : key(_k), val(_v), size(_s) {}
    constexpr bool match(const char* b) const noexcept {
        for(size_t i = 0; i < size; i++) {
            if(key[i] != b[i])
                return false;
        }
        return true;
    }
};

static std::unordered_map<char, std::vector<dic>> start = {
    {'\x00',
     {
         dic{"\x00\x00\x01\x00", "icon", 4},
         dic{"\x00\x00\x01\xba", "mpg", 4},
         dic{"\x00\x01\x00\x00\x53\x74\x61\x6e\x64\x61\x72\x64\x20\x41\x43\x45\x20\x44\x42\x00", "accdb", 20},
         dic{"\x00\x01\x00\x00\x53\x74\x61\x6e\x64\x61\x72\x64\x20\x4a\x65\x74\x20\x44\x42\x00", "mdb", 20},
         dic{"\x00\x01\x00\x00", "palmdata", 4},
         dic{"\x00\x01\x42\x44", "palmarchivedata", 4},
         dic{"\x00\x01\x44\x54", "palmcalenderdata", 4},
         dic{"\x00\x61\x73\x6d", "asm", 4},
         dic{"\x00\x6d\x6c\x6f\x63\x61\x74\x65", "locate", 8},
     }},
    {'\x04',
     {
         dic{"\x04\x22\x4d\x18", "lz4", 4},
     }},
    {'\x05',
     {
         dic{"\x05\x07\x00\x00\x42\x4f\x42\x4f\x05\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", "cwk", 22},
     }},
    {'\x06',
     {
         dic{"\x06\x07\xe1\x00\x42\x4f\x42\x4f\x06\x07\xe1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", "cwk", 22},
     }},
    {'\x0a',
     {
         dic{"\x0a\x0d\x0d\x0a", "pcapng", 4},
     }},
    {'\x0e',
     {
         dic{"\x0E\x03\x13\x01\x00", "hdf", 4},
     }},
    {'\x1a',
     {
         dic{"\x1a\x45\xdf\xa3", "mkv", 4},
     }},
    {'\x1b',
     {
         dic{"\x1b\x4c\x75\x61", "luac", 4},
     }},
    {'\x1f',
     {
         dic{"\x1f\x8b", "gz", 2},
         dic{"\x1f\x9d", "Z", 2},
         dic{"\x1f\xa0", "Z", 2},
     }},
    {'\x20',
     {
         dic{"\x20\x02\x01\x62\xa0\x1e\xab\x07\x02\x00\x00\x00", "tde", 12},
     }},
    {'\x21',
     {
         dic{"\x21\x3c\x61\x72\x63\x68\x3e", "linux deb file", 7},
     }},
    {'\x24',
     {
         dic{"\x24\x53\x44\x49\x30\x30\x30\x31", "System Deployment Image", 8},
     }},
    {'\x25',
     {
         dic{"\x25\x21\x50\x53", "ps", 4},
         dic{"\x25\x50\x44\x46", "pdf", 4},
     }},
    {'\x27',
     {
         dic{"\x27\x05\x19\x56", "U-Boot / uImage. Das U-Boot Universal Boot Loader.", 4},
     }},
    {'\x28',
     {
         dic{"\x28\xb5\x2f\xfd", "Z", 4},
     }},
    {'\x30',
     {
         dic{"\x30\x26\xb2\x75\x8e\x66\xcf\x11\xa6\xd9\x00\xaa\x00\x62\xce\x6c", "asf", 16},
         dic{"\x30\x82", "der", 2},
     }},
    {'\x37',
     {
         dic{"\x37\x48\x03\x02\x00\x00\x00\x00\x58\x35\x30\x39\x4b\x45\x59", "kdb", 15},
         dic{"\x37\x7a\xbc\xaf\x27\x1c", "7z", 6},
         dic{"\x37\x80\x68\x70\x45", "pdf", 5},
     }},
    {'\x38',
     {
         dic{"\x38\x42\x50\x53", "psd", 4},
     }},
    {'\x3a',
     {
         dic{"\x3a\x29\x0a", "Smile file", 3},
     }},
    {'\x41',
     {
         dic{"\x41\x47\x44\x33", "fh8", 4},
     }},
    {'\x42',
     {
         dic{"\x42\x41\x43\x4b\x4d\x49\x4b\x45\x44\x49\x53\x4b",
             "File or tape containing a backup done with AmiBack on an Amiga. It typically is paired with an index "
             "file ",
             12},
         dic{"\x42\x4d", "bmp", 2},
         dic{"\x42\x50\x47\xfb", "Better Portable Graphics format", 4},
         dic{"\x42\x5a\x68", "bz2", 3},
     }},
    {'\x43',
     {
         dic{"\x43\x57\x53\x46\x57\x53", "swf", 6},
         dic{"\x43\x72\x32\x34", "Google Chrome extension", 4},
     }},
    {'\x44',
     {
         dic{"\x44\x43\x4d\x01\x50\x41\x33\x30", "Windows Update Binary Delta Compression", 8},
     }},
    {'\x45',
     {
         dic{"\x45\x4d\x55\x33", "Emulator III synth samples", 4},
         dic{"\x45\x4d\x58\x32", "Emulator Emaxsynth samples", 4},
         dic{"\x45\x52\x02\x00\x00\x00\x8b\x45\x52\x02\x00\x00\x00", "Roxio Toast disc image file", 13},
     }},
    {'\x46',
     {
         dic{"\x46\x4c\x49\x46", "Free Lossless Image Format", 4},
     }},
    {'\x47',
     {
         dic{"\x47\x49\x46\x38\x37\x61\x47\x49\x46\x38\x39\x61", "gif", 12},
     }},
    {'\x49',
     {
         dic{"\x49\x44\x33", "mp3", 3},
         dic{"\x49\x49\x2a\x00", "tiff", 4},
         dic{"\x49\x49\x2a\x00\x10\x00\x00\x00\x43\x52", "Canon RAW Format Version 2", 10},
         dic{"\x49\x4e\x44\x58", "Index file to a file or tape containing a backup done with AmiBack on an Amiga.", 4},
     }},
    {'\x4c',
     {
         dic{"\x4c\x00\x00\x00", "lnk", 4},
         dic{"\x4c\x5a\x49\x50", "lzip", 4},
     }},
    {'\x4b',
     {
         dic{"\x4b\x44\x4d", "vmdk", 3},
     }},
    {'\x4d',
     {
         dic{"\x4d\x49\x4c\x20", "SEAN", 4},
         dic{"\x4d\x4c\x56\x49", "Magic Lantern Video file", 4},
         dic{"\x4d\x53\x43\x46", "cab", 4},
         dic{"\x4d\x54\x68\x64", "midi", 4},
         dic{"\x4d\x5a", "exe", 2},
     }},
    {'\x4e',
     {
         dic{"\x4e\x45\x53\x1a", "Nintendo Entertainment System ROM file", 4},
     }},
    {'\x4f',
     {
         dic{"\x4f\x52\x43", "Apache ORC ", 3},
         dic{"\x4f\x62\x6a\x01", "Apache Avro binary file format", 4},
         dic{"\x4f\x67\x67\x53", "Ogg", 4},
     }},
    {'\x50',
     {
         dic{"\x50\x41\x52\x31", "Apache Parquet columnar file format", 4},
         dic{"\x50\x4b\x03\x04", "zip", 4},
         dic{"\x50\x4b\x05\x06", "zip empty archive", 4},
         dic{"\x50\x4b\x07\x08", "zip spanned archive", 4},
         dic{"\x50\x4d\x4f\x43\x43\x4d\x4f\x43", "Windows Files And Settings Transfer Repository", 8},
     }},
    {'\x52',
     {
         dic{"\x52\x4e\x43\x01\x52\x4e\x43\x02", "Compressed file using Rob Northen Compression ", 8},
         dic{"\x52\x61\x72\x21\x1a\x07\x00", "rar", 7},
         dic{"\x52\x61\x72\x21\x1a\x07\x01\x00", "rar", 8},
         dic{"\x52\x65\x63\x65\x69\x76\x65\x64", "Email Message var5", 8},
     }},
    {'\x53',
     {
         dic{"\x53\x45\x51\x36", "RCFile columnar file format", 4},
         dic{"\x53\x49\x4d\x50\x4c\x45\x20\x20\x3d\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
             "\x20\x20\x20\x54",
             "Flexible Image Transport System ", 30},
         dic{"\x53\x50\x30\x31", "Amazon Kindle Update Package ", 4},
         dic{"\x53\x51\x4c\x69\x74\x65\x20\x66\x6f\x72\x6d\x61\x74\x20\x33\x00", "sqlite3", 16},
         dic{"\x53\x5a\x44\x44\x88\xf0\x27\x33", "Microsoft compressed file in Quantum format", 8},
     }},
    {'\x54',
     {
         dic{"\x54\x41\x50\x45", "Microsoft Tape Format", 4},
         dic{"\x54\x44\x45\x46", "Telegram Desktop Encrypted File", 4},
         dic{"\x54\x44\x46\x24", "Telegram Desktop File", 4},
     }},
    {'\x55',
     {
         dic{"\x55\x55\xaa\xaa", "PhotoCap Vector", 4},
     }},
    {'\x58',
     {
         dic{"\x58\x50\x44\x53", "SMPTE DPX image", 4},
     }},
    {'\x5b',
     {
         dic{"\x5b\x5a\x6f\x6e\x65\x54\x72\x61\x6e\x73\x66\x65\x72\x5d",
             "Microsoft Zone Identifier for URL Security Zones", 14},
     }},
    {'\x62',
     {
         dic{"\x62\x6f\x6f\x6b\x00\x00\x00\x00\x6d\x61\x72\x6b\x00\x00\x00\x00", "macOS file Alias", 16},
         dic{"\x62\x76\x78\x32",
             "LZFSE - Lempel-Ziv style data compression algorithm using Finite State Entropy coding. OSS by Apple.", 4},
     }},
    {'\x64',
     {
         dic{"\x64\x65\x78\x0a\x30\x33\x35\x00", "Dalvik Executable", 8},
     }},
    {'\x65',
     {
         dic{"\x65\x87\x78\x56", "PhotoCap Object Templates", 4},
     }},
    {'\x66',
     {
         dic{"\x66\x4c\x61\x43", "Free Lossless Audio Codec", 4},
     }},
    {'\x74',
     {
         dic{"\x74\x6f\x78\x33", "Open source portable voxel file", 4},
     }},
    {'\x76',
     {
         dic{"\x76\x2f\x31\x01", "OpenEXR image", 4},
     }},
    {'\x77',
     {
         dic{"\x77\x4f\x46\x32", "WOFF File Format 2.0", 4},
         dic{"\x77\x4f\x46\x46", "WOFF File Format 1.0", 4},
     }},
    {'\x78',
     {
         dic{"\x78\x01\x73\x0d\x62\x62\x60", "dmg", 7},
         dic{"\x78\x01\x37\x38\x9c\x37\x38\xda", "No Compression/low Default Compression Best Compression", 8},
         dic{"\x78\x56\x34", "PhotoCap Template", 3},
         dic{"\x78\x61\x72\x21", "eXtensible ARchive format", 4},
     }},
    {'\x7b',
     {
         dic{"\x7b\x5c\x72\x74\x66\x31", "rtf", 6},
     }},
    {'\x7f',
     {
         dic{"\x7f\x45\x4c\x46", "Executable and Linkable Format", 4},
     }},
    {'\x80',
     {
         dic{"\x80\x2a\x5f\xd7", "Kodak Cineon image", 4},
         dic{"\x80\x02", "pickle", 2},
         dic{"\x80\x03", "pickle", 2},
         dic{"\x80\x04", "pickle", 2},
         dic{"\x80\x05", "pickle", 2},
     }},
    {'\x89',
     {
         dic{"\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", "png", 8},
         dic{"\x89\x48\x44\x46\x0d\x0a\x1a\x0a", "hdf5", 8},
     }},
    {'\x96',
     {
         dic{"\x96\xd5\x75\x21", "sarbin", 4},
     }},
    {'\xa1',
     {
         dic{"\xa1\xb2\xc3\xd4\xd4\xc3\xb2\xa1", "Libpcap File Format", 8},
     }},
    {'\xbe',
     {
         dic{"\xbe\xba\xfe\xca", "palmcalenderdata", 4},
     }},
    {'\xca',
     {
         dic{"\xca\xfe\xba\xbe", "javaclass", 4},
     }},
    {'\xce',
     {
         dic{"\xce\xfa\xed\xfe", "Mach-O binary ", 4},
     }},
    {'\xcf',
     {
         dic{"\xcf\x84\x01", "jpg", 3},
         dic{"\xcf\xfa\xed\xfe", "Mach-O binary ", 4},
     }},
    {'\xd0',
     {
         dic{"\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", "Microsoft Office 2003older", 8},
     }},
    {'\xd4',
     {
         dic{"\xd4\xc3\xb2\xa1", "pcap", 4},
     }},
    {'\xed',
     {
         dic{"\xed\xab\xee\xdb", "rpm ", 4},
     }},
    {'\xef',
     {
         dic{"\xef\xbb\xbf", "UTF-8 encoded Unicode byte order mark", 3},
     }},
    {'\xfd',
     {
         dic{"\xfd\x37\x7a\x58\x5a\x00\x00", "xz", 7},
     }},
    {'\xfe',
     {
         dic{"\xfe\xed\xfe\xed", "JKS JavakeyStore", 4},
     }},
    {'\xff',
     {
         dic{"\xff\xfb", "mp3", 2},
         dic{"\xff\xfe", "Byte-order mark for text file encoded in little-endian 16-bit Unicode Transfer Format", 2},
         dic{"\xff\xfe\x00\x00",
             "Byte-order mark for text file encoded in little-endian 32-bit Unicode Transfer Format", 4},
     }},

};

struct reg {
    const char* key;
    const char* val;
    std::regex re;
    reg() : key(0), val(0) {}
    reg(std::nullptr_t) : key(0), val(0) {}
    reg(const char* _k, const char* _v) : key(_k), val(_v), re(std::regex(_k)) {}
    bool match(const char* b) const noexcept { return std::regex_match(b, re); }
};

static std::unordered_map<char, std::vector<reg>> regs = {
    {'F',
     {
         reg{R"(FORM....AIFF)", "aiff"},
         reg{R"(FORM....ANBM)", "anbm"},
         reg{R"(FORM....ANIM)", "anim"},
         reg{R"(FORM....CMUS)", "cmus"},
         reg{R"(FORM....FANT)", "fant"},
         reg{R"(FORM....FAXX)", "faxx"},
         reg{R"(FORM....FTXT)", "ftxt"},
         reg{R"(FORM....ILBM)", "ilbm"},
         reg{R"(FORM....SMUS)", "smus"},
         reg{R"(FORM....YUVN)", "yuvn"},
     }},                                                                                                    // 70
    {'R', {reg{R"(RIFF....AVI )", "avi"}, reg{R"(RIFF....WAVE)", "wav"}, reg{R"(RIFF....WEBP)", "webp"}}},  // 82
    {'\xff',
     {reg{R"(\xff\xd8\xff\xdb\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\xff\xd8\xff\xee\xff\xd8\xff\xe1..Exif\x00\x00)",
          "jpg"}}},  // 255
};

/* for gengo */

template <typename T>
struct nohash {
    constexpr T operator()(const T& s) const noexcept { return s; }
};

template <typename T>
T replaceall(T& std1, T target_std, T change_std) {
    typename T::size_type Pos(std1.find(target_std));
    while(Pos != T::npos) {
        std1.replace(Pos, target_std.length(), change_std);
        Pos = std1.find(target_std, Pos + change_std.length());
    }
    return std1;
}

template <typename T>
T replaceall(T& std1, typename T::value_type target_std, typename T::value_type change_std) {
    typename T::size_type Pos(std1.find(target_std));
    while(Pos != T::npos) {
        std1[Pos] = change_std;
        Pos = std1.find(target_std, Pos + 1);
    }
    return std1;
}

const char* memstr(const char* str, size_t str_size, const char* target, size_t target_size) {
    for(size_t i = 0; i != str_size - target_size; ++i) {
        if(!memcmp(str + i, target, target_size)) {
            return str + i;
        }
    }

    return NULL;
}

#define PERL_XS 1
#include "nkf/nkf.c"
#include "nkf/utf8tbl.c"
#define SCORE_L2 (1)                      /* Kanji Level 2 */
#define SCORE_KANA (SCORE_L2 << 1)        /* Halfwidth Katakana */
#define SCORE_DEPEND (SCORE_KANA << 1)    /* MD Characters */
#define SCORE_CP932 (SCORE_DEPEND << 1)   /* IBM extended characters */
#define SCORE_X0212 (SCORE_CP932 << 1)    /* JIS X 0212 */
#define SCORE_X0213 (SCORE_X0212 << 1)    /* JIS X 0213 */
#define SCORE_NO_EXIST (SCORE_X0213 << 1) /* Undefined Characters */
#define SCORE_iMIME (SCORE_NO_EXIST << 1) /* MIME selected */
#define SCORE_ERROR (SCORE_iMIME << 1)    /* Error */

#define SCORE_INIT (SCORE_iMIME)

static PyObject* pynkf_convert_guess(unsigned char* str, int strlen) {
    pynkf_ibufsize = strlen + 1;
    pynkf_icount = 0;
    pynkf_inbuf = str;
    pynkf_iptr = pynkf_inbuf;

    pynkf_guess_flag = 1;
    reinit();
    guess_f = 1;

    kanji_convert(NULL);

    struct input_code* p = find_inputcode_byfunc(iconv);

    if(input_codename && !*input_codename) {
        Py_RETURN_NONE;
    } else if(!input_codename) {
        return PyUnicode_FromString("ascii");
    } else if(strcmp(input_codename, "Shift_JIS") == 0) {
        return PyUnicode_FromString("cp932");
    } else if(strcmp(input_codename, "EUC-JP") == 0) {
        if(p->score & SCORE_X0213)
            return PyUnicode_FromString("euc_jis_2004");  // EUC-JIS-2004
        else if(p->score & (SCORE_X0212))
            return PyUnicode_FromString("euc_jp");  // EUCJP-MS
        else if(p->score & (SCORE_DEPEND | SCORE_CP932))
            return PyUnicode_FromString("euc_jp");  // CP51932
        return PyUnicode_FromString("euc_jp");      // CP51932
    } else if(strcmp(input_codename, "ISO-2022-JP") == 0) {
        if(p->score & (SCORE_KANA))
            return PyUnicode_FromString("iso2022_jp_1");  // CP50221
        else if(p->score & (SCORE_DEPEND | SCORE_CP932))
            return PyUnicode_FromString("iso2022_jp");  // CP50220
        return PyUnicode_FromString("iso2022_jp");      // CP50220
    }
    return PyUnicode_FromString(input_codename);
}

int flatten(PyObject*& mapping, PyObject*& iterable) {
    PyObject *it, *item;

    it = PyObject_GetIter(iterable);
    if(it == NULL) {
        return 0;
    }

    while((item = PyIter_Next(it)) != NULL) {
        /* do something with item */
        if(PyTuple_Check(item) || PyList_Check(item) || PyDict_Check(item) || PyAnySet_Check(item) ||
           PyGen_Check(item) || PyIter_Check(item) || PyObject_CheckBuffer(item) ||
           PyObject_TypeCheck(item, &PyDictItems_Type) || PyObject_TypeCheck(item, &PyDictKeys_Type) ||
           PyObject_TypeCheck(item, &PyDictValues_Type)) {
            flatten(mapping, item);
        } else {
            PyList_Append(mapping, item);
        }

        /* release reference when done */
        Py_DECREF(item);
    }
    Py_DECREF(it);

    if(PyErr_Occurred()) {
        /* propagate error */
        return 0;
    } else {
        /* continue doing useful work */
        return 1;
    }
}

static py_ustring to_hankaku(const wchar_t* data, std::size_t len) {
    py_ustring res;
    res.reserve(len * 2);

    wchar_t s;
    for(std::size_t i = 0; i < len; ++i) {
        s = data[i];

        if(s == 0x3000)
            res += (wchar_t)0x20;
        else if(s > 0xff00 && s < 0xff5f)
            res += (wchar_t)(0x20 + (s % 0xff));
        else if(s > 0x3098 && s < 0x30FD)
            res += ZEN2HAN[s];
        else
            res += s;
    }
    return res;
}

static py_ustring to_zenkaku(const wchar_t* data, std::size_t len) {
    py_ustring res;
    res.reserve(len);

    wchar_t s, t;

    for(std::size_t i = 0; i < len; ++i) {
        s = data[i];

        if(s == 0x20)
            res += (wchar_t)0x3000;
        else if(s > 0x20 && s < 0x7f)
            res += (wchar_t)(s + 0xfee0);
        else if(s > 0xff62 && s < 0xff9f) {
            t = han2zen.at(s);
            if(s == 0xff73 || (s > 0xff75 && s < 0xff82) || (s > 0xff89 && s < 0xff8F)) {
                auto next = data[i + 1];
                if(next == 0xFF9E || next == 0x309B)
                    ++t, ++i;
                else if(next == 0xFF9F || next == 0x309C)
                    ++++t, ++i;
            }
            res += t;
        } else {
            res += s;
        }
    }
    return res;
}

class Kansuji {
    static const uint8_t MAX_UNIT_SIZE = 20;  // man, oku, cho, kei = (4 * 4keta) + 4(ichi,ju,hyaku,sen) => 20
    using value_type = wchar_t;
    using index_type = uint8_t;
    using size_type = std::size_t;
    using readPtr = value_type*;
    using wk_type = std::array<index_type, 4>;
    using nums_type = std::array<index_type, MAX_UNIT_SIZE>;
    using data_type = value_type*;

    struct no_hash {
        constexpr value_type operator()(const value_type& s) const noexcept { return s; }
    };

    static const std::unordered_map<value_type, value_type, no_hash> Collections;
    static const std::unordered_map<value_type, index_type> WK_UNIT;
    static const std::unordered_map<value_type, index_type> D3_UNIT;
    static const std::unordered_map<value_type, index_type> D4_UNIT;
    static const std::array<value_type, 10> D1_KURAI;
    static const std::array<value_type, 3> D3_KURAI;
    static const std::array<const value_type*, 18> D4_KURAI;

    static const size_type ARRAY_LIMIT = 1024;  //<- pow(2, n)

    /* data */
    const value_type* ucsdata;
    data_type data_;
    value_type fast_data_[ARRAY_LIMIT + 1];
    wk_type wk;
    nums_type nums;

    /* in out iterator */
    value_type* _reader;
    wk_type::iterator _worker;
    nums_type::iterator _nums;
    data_type _writer;

    size_type len;

    /* Initialize */
    Kansuji() : ucsdata(nullptr), data_(), fast_data_(), wk(), nums(), _reader(NULL), len((size_type)-1) {}
    Kansuji(std::nullptr_t)
        : ucsdata(nullptr), data_(), fast_data_(), wk(), nums(), _reader(NULL), len((size_type)-1) {}
    Kansuji(const value_type* u, size_type _len) : ucsdata(u), _reader(NULL), len(_len) {
        if((len * 5) < ARRAY_LIMIT) {
            data_ = fast_data_;
            std::memset(data_, 0, ARRAY_LIMIT + 1);
        } else {
            size_type memsize = len * 5;
            data_ = (data_type)malloc(memsize * sizeof(value_type));
            std::memset(data_, 0, memsize);
        }
        initialize();
    }
    Kansuji(const value_type* u, size_type _len, data_type buf, size_type buflen)
        : ucsdata(u), data_(buf), _reader(NULL), len(_len) {
        std::memset(data_, 0, buflen * sizeof(value_type));
        initialize();
    }

   private:
    void initialize() {
        _reader = (value_type*)(ucsdata + len);
        _writer = data_;
        clear_wk();
        clear_nums();
    }
    index_type get_d3(value_type s, index_type _default = (index_type)-1) {
        if(D3_UNIT.find(s) == D3_UNIT.end())
            return _default;
        return D3_UNIT.at(s);
    }

    index_type get_wk(value_type s, index_type _default = (index_type)-1) {
        if(WK_UNIT.find(s) == WK_UNIT.end())
            return _default;
        return WK_UNIT.at(s);
    }
    index_type get_d4(value_type s, index_type _default = (index_type)-1) {
        if(D4_UNIT.find(s) == D4_UNIT.end())
            return _default;
        return D4_UNIT.at(s);
    }

    void clear_wk() {
        _worker = wk.begin();
        wk.fill(index_type(0));
    }
    void clear_nums() {
        _nums = nums.begin();
        nums.fill(index_type(0));
    }

    bool is_wkunit(value_type s) {
        return WK_UNIT.find(s) != WK_UNIT.end();
        ;
    }
    bool is_wkdata() {
        return std::any_of(wk.begin(), wk.end(), [](auto x) { return x != 0; });
        ;
    }

    value_type read() {
        if(_reader-- == ucsdata)
            return value_type();
        return *_reader;
    }
    data_type write(index_type i) {
        *_writer = (value_type)(0x0030 + i);
        return _writer;
    }

    value_type collection(value_type s) {
        if(Collections.find(s) != Collections.end()) {
            auto r = Collections.at(s);
            if(r == L""[0])
                return value_type();
            else
                return r;
        }
        return s;
    }

    void doFloat() {
        auto nx = *(_reader - 1);
        for(auto it = wk.begin(), end = _worker + 1; it != end; ++it, _writer++)
            *_writer = (value_type)(0x0030 + *it);
        *_writer = L'.';
        ++_writer;
        if(get_wk(nx) == 0) {
            *_writer = L'0';
            ++_writer;
        }
        clear_wk();
    }

    void doWK(index_type i) {
        if(WK_UNIT.find(*(_reader + 1)) != WK_UNIT.end()) {
            if(_worker == wk.end() - 1) {
                to_s();
                clear_nums();
            } else if(_worker < wk.end() - 1) {
                _worker++;
            } else {
                return;  // bug?
            }
        }
        *_worker = i;
    }
    void doD3(index_type i) {
        _worker = wk.begin() + i;
        *_worker = 1;
    }
    void doD4(index_type i) {
        std::copy(wk.begin(), wk.end(), _nums);
        _nums = nums.begin() + i;
        clear_wk();
    }

    void to_s() {
        if(std::any_of(wk.begin(), wk.end(), [](auto x) { return x != 0; })) {
            std::copy(wk.begin(), wk.end(), _nums);
            clear_wk();
        }

        int i = MAX_UNIT_SIZE - 1;
        for(; i > -1; --i) {
            if(nums[(size_type)i] != 0)
                break;
        }
        ++i;
        for(int j = 0; j < i; ++j, ++_writer) {
            *_writer = (value_type)(0x0030 + nums.at((size_type)j));
        }
    }

    int64_t ktoi() {
        value_type s, c;
        index_type r;

        while((s = read()) != value_type()) {
            if(Collections.find(s) != Collections.end()) {
                if((c = Collections.at(s)) == L""[0])
                    continue;
                else
                    s = c;
            }

            auto pr = _reader + 1;
            if(s == L',' && is_wkunit(*pr) && is_wkunit(*(pr + 1)) && is_wkunit(*(pr + 2))) {
                _worker += 1;
                continue;
            }

            if(s == L'.') {
                doFloat();
                continue;
            }

            if((r = get_wk(s)) != (index_type)-1) {
                doWK(r);
                continue;
            }

            auto nx = *(_reader - 1);
            if((r = get_d3(s)) != (index_type)-1) {
                doD3(r);

            } else if((r = get_d4(s)) != (index_type)-1 && (is_wkunit(nx) || get_d3(nx, 0))) {
                doD4(r);

            } else {
                to_s();
                *_writer = s;
                ++_writer;
                clear_wk();
                clear_nums();
            }
        }

        if(is_wkdata())
            to_s();
        std::reverse(data_, _writer);
        return _writer - data_;
    }

    size_type itok(const uint64_t _integer, const data_type& buffer) {
        if(_integer == 0) {
            *buffer = L'零';
            return size_type(1);
        }
        uint64_t integer = _integer;
        data_type ret = buffer;
        uint64_t mod = integer % 10;

        for(auto&& d4 : D4_KURAI) {
            for(int i = (int)wcslen(d4) - 1; i >= 0; --i, ++ret) {
                *ret = d4[i];
            }

            if(mod) {
                *ret = D1_KURAI[mod];
                ++ret;
            }

            integer /= 10;
            if(integer == 0)
                break;
            mod = integer % 10;

            for(auto&& d3 : D3_KURAI) {
                if(mod) {
                    *ret = d3;
                    ++ret;
                    if(mod != 1) {
                        *ret = D1_KURAI[mod];
                        ++ret;
                    }
                }
                integer /= 10;
                if(integer == 0)
                    break;
                mod = integer % 10;
            }

            if(integer == 0)
                break;
        }
        std::reverse(buffer, ret);
        return (size_type)(ret - buffer);
    }

   public:
    static data_type kanji2int(const value_type* u, size_type len = (size_type)-1) {
        len = len == (size_type)-1 ? wcslen(u) : len;
        Kansuji ks(u, len);
        auto retlen = ks.ktoi();
        if(retlen == int64_t())
            return NULL;
        return ks.data_;
    }
    static PyObject* kanji2int(PyObject* u) {
        Py_ssize_t len;

        data_type wdat = PyUnicode_AsWideCharString(u, &len);
        if(wdat == NULL)
            return NULL;

        Kansuji ks(wdat, (size_type)len);
        auto retlen = ks.ktoi();
        PyMem_Free(wdat);
        return PyUnicode_FromWideChar(ks.data_, retlen);
    }
    static data_type int2kanji(const uint64_t i) {
        Kansuji ks;
        data_type buffer = (data_type)PyMem_MALLOC(129 * sizeof(value_type));
        std::fill(buffer, buffer + 129, value_type());
        auto retlen = ks.itok(i, buffer);
        if(retlen == size_type())
            return NULL;
        return buffer;
    }
    static PyObject* int2kanji(PyObject* n) {
        const uint64_t i = PyLong_AsUnsignedLongLong(n);
        Kansuji ks;
        value_type buffer[129] = {0};
        // data_type buffer = PyMem_NEW(value_type, 129);
        auto len = (Py_ssize_t)ks.itok(i, buffer);
        if(len == Py_ssize_t())
            return NULL;
        return PyUnicode_FromWideChar(buffer, len);
    }

   public:
    Kansuji& operator=(const Kansuji&) { return *this; }
};

const std::unordered_map<Kansuji::value_type, Kansuji::index_type> Kansuji::WK_UNIT = {
    {L'〇', (Kansuji::index_type)0}, {L'一', (Kansuji::index_type)1}, {L'二', (Kansuji::index_type)2},
    {L'三', (Kansuji::index_type)3}, {L'四', (Kansuji::index_type)4}, {L'五', (Kansuji::index_type)5},
    {L'六', (Kansuji::index_type)6}, {L'七', (Kansuji::index_type)7}, {L'八', (Kansuji::index_type)8},
    {L'九', (Kansuji::index_type)9}, {L'０', (Kansuji::index_type)0}, {L'１', (Kansuji::index_type)1},
    {L'２', (Kansuji::index_type)2}, {L'３', (Kansuji::index_type)3}, {L'４', (Kansuji::index_type)4},
    {L'５', (Kansuji::index_type)5}, {L'６', (Kansuji::index_type)6}, {L'７', (Kansuji::index_type)7},
    {L'８', (Kansuji::index_type)8}, {L'９', (Kansuji::index_type)9}, {L'0', (Kansuji::index_type)0},
    {L'1', (Kansuji::index_type)1},  {L'2', (Kansuji::index_type)2},  {L'3', (Kansuji::index_type)3},
    {L'4', (Kansuji::index_type)4},  {L'5', (Kansuji::index_type)5},  {L'6', (Kansuji::index_type)6},
    {L'7', (Kansuji::index_type)7},  {L'8', (Kansuji::index_type)8},  {L'9', (Kansuji::index_type)9},
    {L'零', (Kansuji::index_type)0}, {L'壱', (Kansuji::index_type)1}, {L'弐', (Kansuji::index_type)2},
    {L'参', (Kansuji::index_type)3}};

const std::unordered_map<Kansuji::value_type, Kansuji::index_type> Kansuji::D3_UNIT = {
    {L'十', (Kansuji::index_type)1},
    {L'百', (Kansuji::index_type)2},
    {L'千', (Kansuji::index_type)3},
    {L'拾', (Kansuji::index_type)1},
};

const std::unordered_map<Kansuji::value_type, Kansuji::index_type> Kansuji::D4_UNIT = {
    {L'万', (Kansuji::index_type)4},
    {L'億', (Kansuji::index_type)8},
    {L'兆', (Kansuji::index_type)12},
    {L'京', (Kansuji::index_type)16},
};

const std::unordered_map<Kansuji::value_type, Kansuji::value_type, Kansuji::no_hash> Kansuji::Collections = {
    {L' ', L""[0]},   // delete
    {L'　', L""[0]},  // delete
    {L'，', L'，'},   // through
    {L',', L','},     // through
    {L'．', L'.'},    // replace
    {L'拾', L'十'},   // replace
};

const std::array<Kansuji::value_type, 10> Kansuji::D1_KURAI = {L""[0], L'一', L'二', L'三', L'四',
                                                               L'五',  L'六', L'七', L'八', L'九'};
const std::array<Kansuji::value_type, 3> Kansuji::D3_KURAI = {L'十', L'百', L'千'};
const std::array<const Kansuji::value_type*, 18> Kansuji::D4_KURAI = {
    L"",   L"万", L"億", L"兆", L"京",     L"垓",     L"予",     L"穣",       L"溝",
    L"潤", L"正", L"載", L"極", L"恒河沙", L"阿僧祇", L"那由他", L"不可思議", L"無量大数"};

inline bool isin(const char* b, std::size_t pos, const std::string& kw) {
    for(std::size_t i = 0, len = kw.size(); i < len; ++i) {
        if(b[i + pos] != kw[i])
            return false;
    }
    return true;
}

inline bool is_tar(const char* b) {
    if(memcmp(b + 257, "\x75\x73\x74\x61\x72", 5) == 0)
        return true;
    return false;
}

inline constexpr bool is_lha(const char* b) {
    if(b[0] == '\x21' && b[2] == '\x2d' && b[3] == '\x6c' && b[4] == '\x68' && b[6] == '\x2d')
        return true;
    return false;
}

inline bool is_xls(const char* b, std::size_t len) {
    if(memcmp(b + 0, "\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", 8) == 0) {
        std::size_t s = (1U << (b[30] + b[31])) * (b[48] + b[49]) + 640U;
        if(s > len)
            return false;
        if(b[s] == 'W' && b[s + 2] == 'o' && b[s + 4] == 'r' && b[s + 6] == 'k' && b[s + 8] == 'b' &&
           b[s + 10] == 'o' && b[s + 12] == 'o' && b[s + 14] == 'k')
            return true;
        if(b[s] == 'B' && b[s + 2] == 'o' && b[s + 4] == 'o' && b[s + 6] == 'k')
            return true;
    }
    if(b[0] == '\x50' && b[1] == '\x4B') {
        if(memcmp(b + 30, "[Content_Types].xml", 19) == 0 && memstr(b, len, "\x00xl/", 4))
            return true;
        if(memcmp(b + 30, "mimetypeapplication/vnd.oasis.opendocument.spreadsheet", 54) == 0)
            return true;
    }
    return false;
}

inline bool is_doc(const char* b, std::size_t len) {
    if(memcmp(b + 0, "\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", 8) == 0) {
        if(b[512] == '\xec' && b[513] == '\xa5')
            return true;
    }
    if(b[0] == '\x50' && b[1] == '\x4B') {
        if(memcmp(b + 30, "[Content_Types].xml", 19) == 0 && memstr(b, len, "\x00word/", 6))
            return true;
        if(memcmp(b + 30, "mimetypeapplication/vnd.oasis.opendocument.text", 47) == 0)
            return true;
    }

    return false;
}

inline bool is_ppt(const char* b, std::size_t len) {
    if(memcmp(b + 0, "\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", 8) == 0) {
        if(b[512] == '\xec' && b[513] == '\xa5')
            return false;
        std::size_t s = (1U << (b[30] + b[31])) * (b[48] + b[49]) + 640U;
        if(s > len)
            return false;
        if(b[s] == 'W' && b[s + 2] == 'o' && b[s + 4] == 'r' && b[s + 6] == 'k' && b[s + 8] == 'b' &&
           b[s + 10] == 'o' && b[s + 12] == 'o' && b[s + 14] == 'k')
            return false;
        if(b[s] == 'B' && b[s + 2] == 'o' && b[s + 4] == 'o' && b[s + 6] == 'k')
            return false;
        if(b[s])
            return true;
    }
    if(b[0] == '\x50' && b[1] == '\x4B') {
        if(memcmp(b + 30, "[Content_Types].xml", 19) == 0 ||
           (b[30] == '\x70' && b[31] == '\x70' && b[32] == '\x74' && b[33] == '\x2f'))
            return memstr(b, len, "\x00ppt/", 5) != NULL;
        if(memcmp(b + 30, "mimetypeapplication/vnd.oasis.opendocument.presentation", 55) == 0)
            return true;
    }
    return false;
}

inline bool is_xml(const char* b) {
    return memcmp(b, "<?xml version", 13) == 0;
}

inline bool is_html(const char* b) {
    return memcmp(b, "<html", 5) == 0 || memcmp(b, "<!doctype", 9) == 0;
}

inline constexpr bool is_json(const char* b) {
    return b[0] == '{' && strchr(b + 1, '}');
}

inline bool is_dml(const char* b, std::size_t len) {
    const char *r1, *r2;
    if((r1 = memstr(b, len, "record", 6)) != NULL) {
        if((r2 = memstr(r1, len, "end", 3)) != NULL)
            return strchr(r2, ';') != NULL;
    }
    return false;
}

template <char V>
inline constexpr bool is_xsv(const char* uc, std::size_t len) {
    size_t nf = 0, tf = 0, nl = 0, eat = 0;
    const char* ue = uc + len;
    int quote = 0;

    while(uc < ue) {
        switch(*uc++) {
            case '"':
                // Eat until the matching quote

                while(uc < ue) {
                    char c = *uc++;
                    if(c != '"') {
                        // We already got one, done.
                        if(quote) {
                            --uc;
                            ++eat;
                            break;
                        }
                        continue;
                    }
                    if(quote) {
                        // quote-quote escapes
                        quote = 0;
                        continue;
                    }
                    // first quote
                    quote = 1;
                }
                if(eat == 0)
                    uc = ue;
                break;
            case V:
                nf++;
                break;
            case '\n':
                // DPRINTF("%zu %zu %zu\n", nl, nf, tf);
                nl++;
                if(tf == 0) {
                    // First time and no fields, give up
                    if(nf == 0)
                        return 0;
                    // First time, set the number of fields
                    tf = nf;
                } else if(tf != nf) {
                    // Field number mismatch, we are done.
                    return 0;
                }
                if(nl == 3)
                    return true;
                nf = 0;
                break;
            default:
                break;
        }
    }
    return tf && nl > 2;
}

inline constexpr bool is_csv(const char* b, std::size_t len) {
    return is_xsv<','>(b, len) || is_xsv<'\t'>(b, len) || is_xsv<';'>(b, len) || is_xsv<'|'>(b, len) ||
           is_xsv<':'>(b, len);
}

const char* lookuptype(const char* b, std::size_t len) {
    if(memchr(b, 0, len)) {
        if(len > 513) {
            if(b[0] == 'P' && b[1] == 'K') {
                if(is_doc(b, len))
                    return "docx";
                if(is_xls(b, len))
                    return "xlsx";
                if(is_ppt(b, len))
                    return "pptx";
            } else if(b[0] == '\xd0') {
                if(is_doc(b, len))
                    return "doc";
                if(is_xls(b, len))
                    return "xls";
                if(is_ppt(b, len))
                    return "ppt";
            }
        }
        if(len > 262 && is_tar(b)) {
            return "tar";
        } else if(len > 6 && is_lha(b)) {
            return "lha";
        } else if(len > 1) {
            /* start magic number lookup */
            auto head = b[0];
            for(auto& sd : start[head]) {
                if(sd.size < len && memcmp(sd.key, b, sd.size) == 0)
                    return sd.val;
            }

            /* regs magic number lookup */
            for(auto& rd : regs[head]) {
                if(std::regex_match(b, rd.re))
                    return rd.val;
            }
        }

        return NULL;
    }

    /* BOM & space cut */
    std::size_t i = strspn(b, "\x20\xef\xbb\xbf");
    const char* trimb = i == len ? b : b + i;

    if(len > 13 && is_xml(trimb))
        return "xml";
    if(len > 13 && is_html(trimb))
        return "html";
    if(len > 1 && is_json(trimb))
        return "json";
    if(is_csv(b, len))
        return "csv";
    if(len > 10 && is_dml(b, len))
        return "dml";

    return "txt";
};

static const std::unordered_map<wchar_t, int, nohash<wchar_t>> TRAN = {
    {L'0', int(0)},    {L'1', int(1)},    {L'2', int(2)},    {L'3', int(3)},    {L'4', int(4)},    {L'5', int(5)},
    {L'6', int(6)},    {L'7', int(7)},    {L'8', int(8)},    {L'9', int(9)},    {L'０', int(0)},   {L'１', int(1)},
    {L'２', int(2)},   {L'３', int(3)},   {L'４', int(4)},   {L'５', int(5)},   {L'６', int(6)},   {L'７', int(7)},
    {L'８', int(8)},   {L'９', int(9)},   {L'.', int(10)},   {L'．', int(10)},  {L'〇', int(0)},   {L'一', int(1)},
    {L'二', int(2)},   {L'三', int(3)},   {L'四', int(4)},   {L'五', int(5)},   {L'六', int(6)},   {L'七', int(7)},
    {L'八', int(8)},   {L'九', int(9)},   {L'十', int(11)},  {L'_', int(12)},   {L'＿', int(12)},  {L'-', int(13)},
    {L'－', int(13)},  {L',', int(14)},   {L'，', int(14)},  {L'/', int(15)},   {L'／', int(15)},  {L'年', int(16)},
    {L'元', int(17)},  {L'\'', int(18)},  {L'＇', int(18)},  {L'T', int(19)},   {L'Ｔ', int(19)},  {L'M', int(20)},
    {L'Ｍ', int(20)},  {L'S', int(21)},   {L'Ｓ', int(21)},  {L'H', int(22)},   {L'Ｈ', int(22)},  {L'R', int(23)},
    {L'Ｒ', int(23)},  {L'A', int(24)},   {L'Ａ', int(24)},  {L'N', int(25)},   {L'Ｎ', int(25)},  {L'D', int(26)},
    {L'Ｄ', int(26)},  {L':', int(27)},   {L'：', int(27)},  {L'O', int(28)},   {L'Ｏ', int(28)},  {L'p', int(29)},
    {L'ｐ', int(29)},  {L'正', int(30)},  {L'午', int(31)},  {L'後', int(32)},  {L'時', int(33)},  {L'前', int(34)},
    {L'秒', int(35)},  {L'分', int(36)},  {L'日', int(37)},  {L'月', int(38)},  {L'曜', int(39)},  {L'火', int(40)},
    {L'水', int(41)},  {L'木', int(42)},  {L'金', int(43)},  {L'土', int(44)},  {L'(', int(45)},   {L'（', int(45)},
    {L')', int(46)},   {L'）', int(46)},  {L'e', int(47)},   {L'ｅ', int(47)},  {L'F', int(48)},   {L'Ｆ', int(48)},
    {L'i', int(49)},   {L'ｉ', int(49)},  {L'u', int(50)},   {L'ｕ', int(50)},  {L'y', int(51)},   {L'ｙ', int(51)},
    {L'W', int(52)},   {L'Ｗ', int(52)},  {L'b', int(53)},   {L'ｂ', int(53)},  {L'c', int(54)},   {L'ｃ', int(54)},
    {L'g', int(55)},   {L'ｇ', int(55)},  {L'J', int(56)},   {L'Ｊ', int(56)},  {L'l', int(57)},   {L'ｌ', int(57)},
    {L'v', int(58)},   {L'ｖ', int(58)},  {L'K', int(59)},   {L'Ｋ', int(59)},  {L'X', int(60)},   {L'Ｘ', int(60)},
    {L'Z', int(61)},   {L'Ｚ', int(61)},  {L'+', int(62)},   {L'＋', int(62)},  {L'安', int(63)},  {L'雲', int(64)},
    {L'永', int(65)},  {L'延', int(66)},  {L'応', int(67)},  {L'化', int(68)},  {L'嘉', int(69)},  {L'乾', int(70)},
    {L'寛', int(71)},  {L'感', int(72)},  {L'観', int(73)},  {L'喜', int(74)},  {L'亀', int(75)},  {L'吉', int(76)},
    {L'久', int(77)},  {L'亨', int(78)},  {L'享', int(79)},  {L'興', int(80)},  {L'慶', int(81)},  {L'景', int(82)},
    {L'建', int(83)},  {L'護', int(84)},  {L'康', int(85)},  {L'弘', int(86)},  {L'衡', int(87)},  {L'国', int(88)},
    {L'至', int(89)},  {L'字', int(90)},  {L'治', int(91)},  {L'朱', int(92)},  {L'寿', int(93)},  {L'授', int(94)},
    {L'勝', int(95)},  {L'承', int(96)},  {L'昌', int(97)},  {L'昭', int(98)},  {L'祥', int(99)},  {L'神', int(100)},
    {L'仁', int(101)}, {L'成', int(102)}, {L'政', int(103)}, {L'斉', int(104)}, {L'泰', int(105)}, {L'大', int(106)},
    {L'中', int(107)}, {L'長', int(108)}, {L'鳥', int(109)}, {L'貞', int(110)}, {L'禎', int(111)}, {L'天', int(112)},
    {L'同', int(113)}, {L'銅', int(114)}, {L'徳', int(115)}, {L'白', int(116)}, {L'武', int(117)}, {L'福', int(118)},
    {L'文', int(119)}, {L'平', int(120)}, {L'保', int(121)}, {L'宝', int(122)}, {L'万', int(123)}, {L'明', int(124)},
    {L'養', int(125)}, {L'令', int(126)}, {L'霊', int(127)}, {L'暦', int(128)}, {L'老', int(129)}, {L'禄', int(130)},
    {L'和', int(131)}, {L'祚', int(132)}, {L'雉', int(133)}, {L't', int(19)},   {L'ｔ', int(19)},  {L'm', int(20)},
    {L'ｍ', int(20)},  {L's', int(21)},   {L'ｓ', int(21)},  {L'h', int(22)},   {L'ｈ', int(22)},  {L'r', int(23)},
    {L'ｒ', int(23)},  {L'a', int(24)},   {L'ａ', int(24)},  {L'n', int(25)},   {L'ｎ', int(25)},  {L'd', int(26)},
    {L'ｄ', int(26)},  {L'o', int(28)},   {L'ｏ', int(28)},  {L'P', int(29)},   {L'Ｐ', int(29)},  {L'E', int(47)},
    {L'Ｅ', int(47)},  {L'f', int(48)},   {L'ｆ', int(48)},  {L'I', int(49)},   {L'Ｉ', int(49)},  {L'U', int(50)},
    {L'Ｕ', int(50)},  {L'Y', int(51)},   {L'Ｙ', int(51)},  {L'w', int(52)},   {L'ｗ', int(52)},  {L'B', int(53)},
    {L'Ｂ', int(53)},  {L'C', int(54)},   {L'Ｃ', int(54)},  {L'G', int(55)},   {L'Ｇ', int(55)},  {L'j', int(56)},
    {L'ｊ', int(56)},  {L'L', int(57)},   {L'Ｌ', int(57)},  {L'V', int(58)},   {L'Ｖ', int(58)},  {L'k', int(59)},
    {L'ｋ', int(59)},  {L'x', int(60)},   {L'ｘ', int(60)},  {L'z', int(61)},   {L'ｚ', int(61)}};

static const std::unordered_set<wchar_t, nohash<wchar_t>> NUMBERS = {
    L'0',  L'1',  L'2',  L'3',  L'4',  L'5',  L'6',  L'7',  L'8',  L'9',  L'０', L'１', L'２', L'３', L'４', L'５',
    L'６', L'７', L'８', L'９', L'〇', L'一', L'二', L'三', L'四', L'五', L'六', L'七', L'八', L'九', L'十'};

static std::unordered_set<wchar_t, nohash<wchar_t>> VALIDATOR = {
    L' ',  L'　', L'/',  L'-',  L'+',  L':',  L'.',  L',',  L'0',  L'1',  L'2',  L'3',  L'4',  L'5',
    L'6',  L'7',  L'8',  L'9',  L'０', L'１', L'２', L'３', L'４', L'５', L'６', L'７', L'８', L'９',
    L'〇', L'一', L'二', L'三', L'四', L'五', L'六', L'七', L'八', L'九', L'十', L'年', L'月', L'日',
    L'時', L'分', L'秒', L'A',  L'D',  L'F',  L'J',  L'M',  L'N',  L'O',  L'S',  L'T',  L'W',  L'S'};

static int mkdir_p(const char* filepath) {
    char* p = NULL;
    char* buf = NULL;

    std::size_t buflen = strlen(filepath) + 4;
    buf = (char*)malloc(buflen);
    if(buf == NULL) {
        return -1;
    }

#if defined(_WIN32) || defined(_WIN64)
    strcpy_s(buf, buflen, filepath);
    for(p = strchr(buf + 1, '\\'); p; p = strchr(p + 1, '\\')) {
#else
    strcpy(buf, filepath);
    for(p = strchr(buf + 1, '/'); p; p = strchr(p + 1, '/')) {
#endif
        *p = '\0';

        struct stat sb = {0};
        if(stat(filepath, &sb) == 0) {
            free(buf);
            return 0;
        }

#if defined(_WIN32) || defined(_WIN64)
        if(_mkdir(filepath)) {
#else
        if(mkdir(filepath, 0777)) {
#endif
            free(buf);
            return -1;
        }

#if defined(_WIN32) || defined(_WIN64)
        *p = '\\';
#else
        *p = '/';
#endif
    }

    free(buf);
    return 0;
}

template <std::size_t N>
struct Trie {
    struct TrieNode {
        int first[N + 1];
        int second;

        TrieNode() : second(0) {
            ;
            std::fill(std::begin(first), std::end(first), -1);
        }
    };

    std::vector<TrieNode> nodes;
    uint64_t len;

    Trie() : len(1) {
        TrieNode root{};
        this->nodes.push_back(root);
        this->len = 1;
    }
    Trie(std::nullptr_t) : len(0) {}

    Trie(size_t len) {
        if(len) {
            this->len = len;
            this->nodes.resize(len);
            TrieNode root{};
            this->nodes[0] = root;
        } else {
            TrieNode root{};
            this->nodes.push_back(root);
            this->len = 1;
        }
    }

    constexpr void insert(const std::wstring& str, int value) noexcept {
        uint64_t i = 0;
        int sid = 0;

        for(auto&& s : str) {
            if(TRAN.find(s) == TRAN.end())
                break;
            sid = TRAN.at(s);
            if(nodes[i].first[sid] == -1) {
                TrieNode new_node{};
                nodes.push_back(new_node);
                ++len;
                nodes[i].first[sid] = (int)(nodes.size() - 1);
            }
            i = (uint64_t)nodes[i].first[sid];
        }
        nodes[i].second = value;
    }

    constexpr int common_prefix(const std::wstring& str) noexcept {
        uint64_t i = 0;
        int sid = 0, tmp = 0;
        for(auto&& c : str) {
            if(TRAN.find(c) == TRAN.end())
                break;
            sid = TRAN.at(c);
            if((tmp = nodes[i].first[sid]) == -1)
                break;
            i = (uint64_t)tmp;
        }
        return nodes[i].second;
    }

    constexpr bool query(const std::wstring& str) noexcept {
        uint64_t i = 0;
        int sid = 0, tmp = 0;
        for(auto&& c : str) {
            if(TRAN.find(c) == TRAN.end())
                return false;
            sid = TRAN.at(c);
            if((tmp = nodes[i].first[sid]) == -1)
                return false;
            i = (uint64_t)tmp;
        }
        return true;
    }

    constexpr uint64_t save(const char* filepath) noexcept {
        if(nodes.size() > 0 && len > 0 && nodes.size() == len) {
            FILE* fp = NULL;
            const char* magic = "TRIEDATE";

#if defined(_WIN32) || defined(_WIN64)
            if(fopen_s(&fp, filepath, "wb") != 0)
#else
            if((fp = fopen(filepath, "wb")) == NULL)
#endif
                return (uint64_t)-1;
            if(fp == NULL)
                return (uint64_t)-1;
            fwrite(magic, 1, 8, fp);

            fwrite(&len, sizeof(len), 1, fp);

            fwrite(nodes.data(), sizeof(TrieNode), nodes.size(), fp);

            fclose(fp);
            return len;
        } else {
            return (uint64_t)-1;
        }
    }

    constexpr uint64_t load(const char* filepath) noexcept {
        FILE* fp = NULL;
        char magic[9] = {0};
        char checkmagic[9] = "TRIEDATE";

#if defined(_WIN32) || defined(_WIN64)
        if(fopen_s(&fp, filepath, "rb") != 0)
#else
        if((fp = fopen(filepath, "rb")) == NULL)
#endif
            return (uint64_t)-1;
        if(fp == NULL)
            return (uint64_t)-1;
        std::size_t r = fread(magic, 1, 8, fp);

        if(r < 8 || magic[0] != 0 || strcmp(magic, checkmagic))
            return (uint64_t)-1;

        if(fread(&len, sizeof(len), 1, fp) < 1)
            return (uint64_t)-1;
        nodes.resize(len + 1);

        if(fread(&(nodes.data()[0]), sizeof(TrieNode), len, fp) < len)
            return (uint64_t)-1;

        fclose(fp);
        return nodes.size();
    }
};

template <std::size_t N>
void insert(Trie<N>& NODE, std::wstring str, int value) {
    NODE.insert(str, value);

    wchar_t k = L""[0];
    wchar_t kj = L""[0];
    std::wstring zenkaku;
    std::wstring kansuji;
    std::wstring kansuji_j;

    for(wchar_t s : str) {
        if(VALIDATOR.find(s) == VALIDATOR.end())
            VALIDATOR.emplace(s);

        if(s > 0x0020 && s < 0x007f) {
            k = wchar_t(s + 0xfee0);
            zenkaku += k;
            if(VALIDATOR.find(k) == VALIDATOR.end())
                VALIDATOR.emplace(k);

            if(0x002f < s && s < 0x003a) {
                kj = L"〇一二三四五六七八九"[s - 0x0030];
                kansuji += kj;
                if(value < 100)
                    kansuji_j = Kansuji::int2kanji((uint64_t)value);

            } else {
                kansuji += k;
                kansuji_j += k;
            }
        } else {
            zenkaku += s;
            kansuji += s;
            kansuji_j += s;
        }
    }
    if(!zenkaku.empty())
        NODE.insert(zenkaku, value);

    if(!kansuji.empty())
        NODE.insert(kansuji, value);

    if(!kansuji_j.empty())
        NODE.insert(kansuji_j, value);
}

static Trie<133> GG;
static Trie<16> YYYY;
static Trie<18> yy;
static Trie<58> MM;
static Trie<37> DD;
static Trie<34> HH;
static Trie<36> mi;
static Trie<35> SS;
static Trie<10> sss;
static Trie<52> WW;
static Trie<62> ZZ;

int builder_datetime(const char* dirpath) {
    static const std::wstring ml[12][2] = {
        {L"January", L"Jan"},   {L"February", L"Feb"}, {L"March", L"Mar"},    {L"April", L"Apr"},
        {L"May", L"May"},       {L"June", L"Jun"},     {L"July", L"Jul"},     {L"August", L"Aug"},
        {L"September", L"Sep"}, {L"October", L"Oct"},  {L"November", L"Nov"}, {L"December", L"Dec"},
    };

    static const std::wstring weekday[7][6] = {
        {L"Sunday", L"Sun", L"日曜日", L"日曜", L"(日)", L"日"},
        {L"Monday", L"Mon", L"月曜日", L"月曜", L"(月)", L"月"},
        {L"Tuesday", L"Tue", L"火曜日", L"火曜", L"(火)", L"火"},
        {L"Wednesday", L"Wed", L"水曜日", L"水曜", L"(水)", L"水"},
        {L"Thursday", L"Thu", L"木曜日", L"木曜", L"(木)", L"木"},
        {L"Friday", L"Fri", L"金曜日", L"金曜", L"(金)", L"金"},
        {L"Saturday", L"Sat", L"土曜日", L"土曜", L"(土)", L"土"},
    };

    static const std::vector<std::pair<std::wstring, int>> gengo = {
        {L"令和", int(2019)},    {L"R.", int(2019)},      {L"R", int(2019)},       {L"令", int(2019)},
        {L"平成", int(1989)},    {L"H.", int(1989)},      {L"H", int(1989)},       {L"平", int(1989)},
        {L"昭和", int(1926)},    {L"S.", int(1926)},      {L"S", int(1926)},       {L"昭", int(1926)},
        {L"大正", int(1912)},    {L"T.", int(1912)},      {L"T", int(1912)},       {L"大", int(1912)},
        {L"明治", int(1868)},    {L"M.", int(1868)},      {L"M", int(1868)},       {L"明", int(1868)},
        {L"慶応", int(1865)},    {L"元治", int(1864)},    {L"文久", int(1861)},    {L"万延", int(1860)},
        {L"安政", int(1855)},    {L"嘉永", int(1848)},    {L"弘化", int(1845)},    {L"天保", int(1831)},
        {L"文政", int(1818)},    {L"文化", int(1804)},    {L"享和", int(1801)},    {L"寛政", int(1789)},
        {L"天明", int(1781)},    {L"安永", int(1772)},    {L"明和", int(1764)},    {L"宝暦", int(1751)},
        {L"寛延", int(1748)},    {L"延享", int(1744)},    {L"寛保", int(1741)},    {L"元文", int(1736)},
        {L"享保", int(1716)},    {L"正徳", int(1711)},    {L"宝永", int(1704)},    {L"元禄", int(1688)},
        {L"貞享", int(1684)},    {L"天和", int(1681)},    {L"延宝", int(1673)},    {L"寛文", int(1661)},
        {L"万治", int(1658)},    {L"明暦", int(1655)},    {L"承応", int(1652)},    {L"慶安", int(1648)},
        {L"正保", int(1645)},    {L"寛永", int(1624)},    {L"元和", int(1615)},    {L"慶長", int(1596)},
        {L"文禄", int(1593)},    {L"天正", int(1573)},    {L"元亀", int(1570)},    {L"永禄", int(1558)},
        {L"弘治", int(1555)},    {L"天文", int(1532)},    {L"享禄", int(1528)},    {L"大永", int(1521)},
        {L"永正", int(1504)},    {L"文亀", int(1501)},    {L"明応", int(1492)},    {L"延徳", int(1489)},
        {L"長享", int(1487)},    {L"文明", int(1469)},    {L"応仁", int(1467)},    {L"文正", int(1466)},
        {L"寛正", int(1461)},    {L"長禄", int(1457)},    {L"康正", int(1455)},    {L"享徳", int(1452)},
        {L"宝徳", int(1449)},    {L"文安", int(1444)},    {L"嘉吉", int(1441)},    {L"永享", int(1429)},
        {L"正長", int(1428)},    {L"応永", int(1394)},    {L"明徳", int(1390)},    {L"康応", int(1389)},
        {L"嘉慶", int(1387)},    {L"至徳", int(1384)},    {L"永徳", int(1381)},    {L"康暦", int(1379)},
        {L"永和", int(1375)},    {L"応安", int(1368)},    {L"貞治", int(1362)},    {L"康安", int(1361)},
        {L"延文", int(1356)},    {L"文和", int(1352)},    {L"観応", int(1350)},    {L"貞和", int(1345)},
        {L"康永", int(1342)},    {L"暦応", int(1338)},    {L"元中", int(1384)},    {L"弘和", int(1381)},
        {L"天授", int(1375)},    {L"文中", int(1372)},    {L"建徳", int(1370)},    {L"正平", int(1347)},
        {L"興国", int(1340)},    {L"延元", int(1336)},    {L"建武", int(1334)},    {L"正慶", int(1332)},
        {L"元弘", int(1331)},    {L"元徳", int(1329)},    {L"嘉暦", int(1326)},    {L"正中", int(1324)},
        {L"元亨", int(1321)},    {L"元応", int(1319)},    {L"文保", int(1317)},    {L"正和", int(1312)},
        {L"応長", int(1311)},    {L"延慶", int(1308)},    {L"徳治", int(1307)},    {L"嘉元", int(1303)},
        {L"乾元", int(1302)},    {L"正安", int(1299)},    {L"永仁", int(1293)},    {L"正応", int(1288)},
        {L"弘安", int(1278)},    {L"建治", int(1275)},    {L"文永", int(1264)},    {L"弘長", int(1261)},
        {L"文応", int(1260)},    {L"正元", int(1259)},    {L"正嘉", int(1257)},    {L"康元", int(1256)},
        {L"建長", int(1249)},    {L"宝治", int(1247)},    {L"寛元", int(1243)},    {L"仁治", int(1240)},
        {L"延応", int(1239)},    {L"暦仁", int(1238)},    {L"嘉禎", int(1235)},    {L"文暦", int(1234)},
        {L"天福", int(1233)},    {L"貞永", int(1232)},    {L"寛喜", int(1229)},    {L"安貞", int(1228)},
        {L"嘉禄", int(1225)},    {L"元仁", int(1224)},    {L"貞応", int(1222)},    {L"承久", int(1219)},
        {L"建保", int(1214)},    {L"建暦", int(1211)},    {L"承元", int(1207)},    {L"建永", int(1206)},
        {L"元久", int(1204)},    {L"建仁", int(1201)},    {L"正治", int(1199)},    {L"建久", int(1190)},
        {L"文治", int(1185)},    {L"元暦", int(1184)},    {L"寿永", int(1182)},    {L"養和", int(1181)},
        {L"治承", int(1177)},    {L"安元", int(1175)},    {L"承安", int(1171)},    {L"嘉応", int(1169)},
        {L"仁安", int(1166)},    {L"永万", int(1165)},    {L"長寛", int(1163)},    {L"応保", int(1161)},
        {L"永暦", int(1160)},    {L"平治", int(1159)},    {L"保元", int(1156)},    {L"久寿", int(1154)},
        {L"仁平", int(1151)},    {L"久安", int(1145)},    {L"天養", int(1144)},    {L"康治", int(1142)},
        {L"永治", int(1141)},    {L"保延", int(1135)},    {L"長承", int(1132)},    {L"天承", int(1131)},
        {L"大治", int(1126)},    {L"天治", int(1124)},    {L"保安", int(1120)},    {L"元永", int(1118)},
        {L"永久", int(1113)},    {L"天永", int(1110)},    {L"天仁", int(1108)},    {L"嘉承", int(1106)},
        {L"長治", int(1104)},    {L"康和", int(1099)},    {L"承徳", int(1097)},    {L"永長", int(1097)},
        {L"嘉保", int(1095)},    {L"寛治", int(1087)},    {L"応徳", int(1084)},    {L"永保", int(1081)},
        {L"承暦", int(1077)},    {L"承保", int(1074)},    {L"延久", int(1069)},    {L"治暦", int(1065)},
        {L"康平", int(1058)},    {L"天喜", int(1053)},    {L"永承", int(1046)},    {L"寛徳", int(1044)},
        {L"長久", int(1040)},    {L"長暦", int(1037)},    {L"長元", int(1028)},    {L"万寿", int(1024)},
        {L"治安", int(1021)},    {L"寛仁", int(1017)},    {L"長和", int(1013)},    {L"寛弘", int(1004)},
        {L"長保", int(999)},     {L"長徳", int(995)},     {L"正暦", int(990)},     {L"永祚", int(989)},
        {L"永延", int(987)},     {L"寛和", int(985)},     {L"永観", int(983)},     {L"天元", int(978)},
        {L"貞元", int(976)},     {L"天延", int(974)},     {L"天禄", int(970)},     {L"安和", int(968)},
        {L"康保", int(964)},     {L"応和", int(961)},     {L"天徳", int(957)},     {L"天暦", int(947)},
        {L"天慶", int(938)},     {L"承平", int(931)},     {L"延長", int(923)},     {L"延喜", int(901)},
        {L"昌泰", int(898)},     {L"寛平", int(889)},     {L"仁和", int(885)},     {L"元慶", int(877)},
        {L"貞観", int(859)},     {L"天安", int(857)},     {L"斉衡", int(854)},     {L"仁寿", int(851)},
        {L"嘉祥", int(848)},     {L"承和", int(834)},     {L"天長", int(824)},     {L"弘仁", int(810)},
        {L"大同", int(806)},     {L"延暦", int(782)},     {L"天応", int(781)},     {L"宝亀", int(770)},
        {L"神護景雲", int(767)}, {L"天平神護", int(765)}, {L"天平宝字", int(757)}, {L"天平勝宝", int(749)},
        {L"天平感宝", int(749)}, {L"天平", int(729)},     {L"神亀", int(724)},     {L"養老", int(717)},
        {L"霊亀", int(715)},     {L"和銅", int(708)},     {L"慶雲", int(704)},     {L"大宝", int(701)},
        {L"朱鳥", int(686)},     {L"白雉", int(650)},     {L"大化", int(645)},
    };

    static wchar_t ymdsep[] = {0, L'/', L'-', L'_', L'.', L','};
    static wchar_t hmssep[] = {0, L':', L'_', L'.'};
    static const std::wstring half[] = {L"am", L"pm",  L"a.m", L"p.m",  L"a.m.", L"p.m.", L"AM",
                                        L"PM", L"A.M", L"P.M", L"A.M.", L"P.M.", L"午前", L"午後"};
    static const std::vector<std::pair<std::wstring, int>> tzone = {
        {L"ACDT", int(37800)},     {L"ACST", int(34200)},      {L"ACT", int(-18000)},      {L"ACWST", int(31500)},
        {L"ADT", int(-10800)},     {L"AEDT", int(39600)},      {L"AEST", int(36000)},      {L"AFT", int(16200)},
        {L"AKDT", int(-28800)},    {L"AKST", int(-32400)},     {L"AMST", int(-10800)},     {L"AMT", int(-14400)},
        {L"AMT", int(14400)},      {L"ART", int(-10800)},      {L"AST", int(-14400)},      {L"AST", int(10800)},
        {L"AT", int(-14400)},      {L"AWST", int(28800)},      {L"AZOST", int(0)},         {L"AZOT", int(-3600)},
        {L"AZT", int(14400)},      {L"BDT", int(28800)},       {L"BNT", int(28800)},       {L"BOT", int(-14400)},
        {L"BRST", int(-7200)},     {L"BRT", int(-10800)},      {L"BST", int(21600)},       {L"BST", int(3600)},
        {L"BTT", int(21600)},      {L"CAT", int(7200)},        {L"CCT", int(23400)},       {L"CDT", int(-14400)},
        {L"CDT", int(-18000)},     {L"CEST", int(7200)},       {L"CET", int(3600)},        {L"CHADT", int(49500)},
        {L"CHAST", int(45900)},    {L"CHOST", int(32400)},     {L"CHOT", int(28800)},      {L"CHST", int(36000)},
        {L"CHUT", int(36000)},     {L"CIT", int(28800)},       {L"CKT", int(-36000)},      {L"CLST", int(-10800)},
        {L"CLT", int(-14400)},     {L"COST", int(-14400)},     {L"COT", int(-18000)},      {L"CST", int(-18000)},
        {L"CST", int(-21600)},     {L"CST", int(28800)},       {L"CT", int(-21600)},       {L"CVT", int(-3600)},
        {L"CXT", int(25200)},      {L"DAVT", int(25200)},      {L"DDUT", int(36000)},      {L"EASST", int(-18000)},
        {L"EAST", int(-21600)},    {L"EAT", int(10800)},       {L"ECT", int(-18000)},      {L"EDT", int(-14400)},
        {L"EEST", int(10800)},     {L"EET", int(7200)},        {L"EGST", int(0)},          {L"EGT", int(-3600)},
        {L"EIT", int(32400)},      {L"EST", int(-18000)},      {L"ET", int(-18000)},       {L"FET", int(10800)},
        {L"FJT", int(43200)},      {L"FKST", int(-10800)},     {L"FKT", int(-14400)},      {L"FNT", int(-7200)},
        {L"GALT", int(-21600)},    {L"GAMT", int(-32400)},     {L"GET", int(14400)},       {L"GFT", int(-10800)},
        {L"GILT", int(43200)},     {L"GIT", int(-32400)},      {L"GMT", int(0)},           {L"GMT+0", int(0)},
        {L"GMT+1", int(3600)},     {L"GMT+10", int(36000)},    {L"GMT+10:30", int(37800)}, {L"GMT+11", int(39600)},
        {L"GMT+12", int(43200)},   {L"GMT+12:45", int(45900)}, {L"GMT+13", int(46800)},    {L"GMT+13:45", int(49500)},
        {L"GMT+14", int(50400)},   {L"GMT+2", int(7200)},      {L"GMT+3", int(10800)},     {L"GMT+3:30", int(12600)},
        {L"GMT+4", int(14400)},    {L"GMT+4:30", int(16200)},  {L"GMT+5", int(18000)},     {L"GMT+5:30", int(19800)},
        {L"GMT+5:45", int(20700)}, {L"GMT+6", int(21600)},     {L"GMT+6:30", int(23400)},  {L"GMT+7", int(25200)},
        {L"GMT+8", int(28800)},    {L"GMT+8:45", int(31500)},  {L"GMT+9", int(32400)},     {L"GMT+9:30", int(34200)},
        {L"GMT-1", int(-3600)},    {L"GMT-10", int(-36000)},   {L"GMT-11", int(-39600)},   {L"GMT-2", int(-7200)},
        {L"GMT-2:30", int(-9000)}, {L"GMT-3", int(-10800)},    {L"GMT-3:30", int(-12600)}, {L"GMT-4", int(-14400)},
        {L"GMT-5", int(-18000)},   {L"GMT-6", int(-21600)},    {L"GMT-7", int(-25200)},    {L"GMT-8", int(-28800)},
        {L"GMT-9", int(-32400)},   {L"GMT-9:30", int(-34200)}, {L"GST", int(-7200)},       {L"GST", int(14400)},
        {L"GYT", int(-14400)},     {L"HADT", int(-32400)},     {L"HAST", int(-36000)},     {L"HKT", int(28800)},
        {L"HOVST", int(28800)},    {L"HOVT", int(25200)},      {L"ICT", int(25200)},       {L"IDT", int(10800)},
        {L"IRDT", int(16200)},     {L"IRKT", int(28800)},      {L"IRST", int(12600)},      {L"IST", int(19800)},
        {L"IST", int(3600)},       {L"IST", int(7200)},        {L"JST", int(32400)},       {L"KGT", int(21600)},
        {L"KOST", int(39600)},     {L"KRAT", int(25200)},      {L"KST", int(32400)},       {L"LHDT", int(39600)},
        {L"LHST", int(37800)},     {L"LINT", int(50400)},      {L"MAGT", int(39600)},      {L"MART", int(-34200)},
        {L"MAWT", int(18000)},     {L"MDT", int(-21600)},      {L"MHT", int(43200)},       {L"MIST", int(39600)},
        {L"MIT", int(-34200)},     {L"MMT", int(23400)},       {L"MSK", int(10800)},       {L"MST", int(-25200)},
        {L"MST", int(28800)},      {L"MT", int(-25200)},       {L"MUT", int(14400)},       {L"MVT", int(18000)},
        {L"MYT", int(28800)},      {L"NCT", int(39600)},       {L"NDT", int(-9000)},       {L"NFT", int(39600)},
        {L"NPT", int(20700)},      {L"NRT", int(43200)},       {L"NST", int(-12600)},      {L"NT", int(-12600)},
        {L"NUT", int(-39600)},     {L"NZDT", int(46800)},      {L"NZST", int(43200)},      {L"OMST", int(21600)},
        {L"ORAT", int(18000)},     {L"PDT", int(-25200)},      {L"PET", int(-18000)},      {L"PETT", int(43200)},
        {L"PGT", int(36000)},      {L"PHOT", int(46800)},      {L"PHT", int(28800)},       {L"PKT", int(18000)},
        {L"PMDT", int(-7200)},     {L"PMST", int(-10800)},     {L"PONT", int(39600)},      {L"PST", int(-28800)},
        {L"PST", int(28800)},      {L"PT", int(-28800)},       {L"PWT", int(32400)},       {L"PYST", int(-10800)},
        {L"PYT", int(-14400)},     {L"RET", int(14400)},       {L"ROTT", int(-10800)},     {L"SAKT", int(39600)},
        {L"SAMT", int(14400)},     {L"SAST", int(7200)},       {L"SBT", int(39600)},       {L"SCT", int(14400)},
        {L"SGT", int(28800)},      {L"SLST", int(19800)},      {L"SRT", int(-10800)},      {L"SST", int(-39600)},
        {L"SYOT", int(10800)},     {L"TAHT", int(-36000)},     {L"TFT", int(18000)},       {L"THA", int(25200)},
        {L"TJT", int(18000)},      {L"TKT", int(46800)},       {L"TLT", int(32400)},       {L"TMT", int(18000)},
        {L"TOT", int(46800)},      {L"TRT", int(10800)},       {L"TVT", int(43200)},       {L"ULAST", int(32400)},
        {L"ULAT", int(28800)},     {L"USZ1", int(7200)},       {L"UTC", int(0)},           {L"UTC+0", int(0)},
        {L"UTC+1", int(3600)},     {L"UTC+10", int(36000)},    {L"UTC+10:30", int(37800)}, {L"UTC+11", int(39600)},
        {L"UTC+12", int(43200)},   {L"UTC+12:45", int(45900)}, {L"UTC+13", int(46800)},    {L"UTC+13:45", int(49500)},
        {L"UTC+14", int(50400)},   {L"UTC+2", int(7200)},      {L"UTC+3", int(10800)},     {L"UTC+3:30", int(12600)},
        {L"UTC+4", int(14400)},    {L"UTC+4:30", int(16200)},  {L"UTC+5", int(18000)},     {L"UTC+5:30", int(19800)},
        {L"UTC+5:45", int(20700)}, {L"UTC+6", int(21600)},     {L"UTC+6:30", int(23400)},  {L"UTC+7", int(25200)},
        {L"UTC+8", int(28800)},    {L"UTC+8:45", int(31500)},  {L"UTC+9", int(32400)},     {L"UTC+9:30", int(34200)},
        {L"UTC-1", int(-3600)},    {L"UTC-10", int(-36000)},   {L"UTC-11", int(-39600)},   {L"UTC-2", int(-7200)},
        {L"UTC-2:30", int(-5400)}, {L"UTC-3", int(-10800)},    {L"UTC-3:30", int(-9000)},  {L"UTC-4", int(-14400)},
        {L"UTC-5", int(-18000)},   {L"UTC-6", int(-21600)},    {L"UTC-7", int(-25200)},    {L"UTC-8", int(-28800)},
        {L"UTC-9", int(-32400)},   {L"UTC-9:30", int(-30600)}, {L"UYST", int(-7200)},      {L"UYT", int(-10800)},
        {L"UZT", int(18000)},      {L"VET", int(-14400)},      {L"VLAT", int(36000)},      {L"VOLT", int(14400)},
        {L"VOST", int(21600)},     {L"VUT", int(39600)},       {L"WAKT", int(43200)},      {L"WAST", int(7200)},
        {L"WAT", int(3600)},       {L"WEST", int(3600)},       {L"WET", int(0)},           {L"WFT", int(43200)},
        {L"WGST", int(-10800)},    {L"WGST", int(-7200)},      {L"WIB", int(25200)},       {L"WIT", int(32400)},
        {L"YAKT", int(32400)},     {L"YEKT", int(18000)},
    };

    ymdsep[0] = L'年';

    for(int v = 1; v < 2200; ++v) {
        std::wstring st = std::to_wstring(v);
        insert(YYYY, st, v);
        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it) {
            insert(YYYY, st + *it, v);
            insert(YYYY, *it + st, v);
        }
    }
    for(int v = 1; v < 100; ++v) {
        std::wstring st = std::to_wstring(v);
        insert(yy, st, v);
        insert(yy, L'\'' + st, v < 60 ? v + 2000 : v + 1900);
        insert(YYYY, L'\'' + st, v < 60 ? v + 2000 : v + 1900);
        if(v < 10) {
            std::wstring zfill = L'0' + st;
            insert(yy, zfill, v);
            insert(yy, L'\'' + zfill, v < 60 ? v + 2000 : v + 1900);
            insert(YYYY, L'\'' + zfill, v < 60 ? v + 2000 : v + 1900);
        }
        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it) {
            wchar_t sp = *it;
            insert(yy, st + sp, v);
            insert(yy, sp + st, v);
            if(v < 10) {
                std::wstring zfill = L'0' + st;
                insert(yy, zfill + sp, v);
                insert(YYYY, (L'\'' + zfill) + sp, v);
                insert(yy, sp + (L'\'' + zfill), v < 60 ? v + 2000 : v + 1900);
                insert(YYYY, sp + zfill, v < 60 ? v + 2000 : v + 1900);
            }
        }
    }
    insert(yy, L"元年", 1);

    for(auto it = std::begin(gengo); it != std::end(gengo); ++it)
        insert(GG, it->first, it->second);

    ymdsep[0] = L'月';
    for(int v = 1; v < 13; ++v) {
        std::wstring st = std::to_wstring(v);
        insert(MM, st, v);

        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it)
            insert(MM, st + *it, v);

        for(auto it = std::begin(ml[v - 1]); it != std::end(ml[v - 1]); ++it) {
            insert(MM, *it, v);
            insert(MM, *it + L'.', v);
            insert(MM, *it + L',', v);
            insert(MM, *it + L'/', v);
        }
    }
    for(int v = 1; v < 10; ++v) {
        std::wstring st = L'0' + std::to_wstring(v);
        insert(MM, st, v);

        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it)
            insert(MM, st + *it, v);
    }

    ymdsep[0] = L'日';
    for(int v = 1; v < 32; ++v) {
        std::wstring st = std::to_wstring(v);
        insert(DD, st, v);
        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it) {
            insert(DD, st + *it, v);
        }
        if(v == 1)
            insert(DD, L"1st", v);
        else if(v == 2)
            insert(DD, L"2nd", v);
        else if(v == 3)
            insert(DD, L"3rd", v);
        else
            insert(DD, st + L"th", v);
    }
    for(int v = 1; v < 10; ++v) {
        std::wstring st = L'0' + std::to_wstring(v);
        insert(DD, st, v);
        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it) {
            insert(DD, st + *it, v);
        }
    }

    hmssep[0] = L'時';
    for(int v = 0; v < 24; ++v) {
        std::wstring st = std::to_wstring(v);
        std::wstring st_2d = L'0' + std::to_wstring(v);
        insert(HH, st, v);
        insert(HH, (v < 10 ? st_2d : st), v);
        insert(HH, L'T' + (v < 10 ? st_2d : st), v);
        insert(HH, L':' + (v < 10 ? st_2d : st), v);
        if(v < 13) {
            for(auto it = std::begin(half); it != std::end(half); ++it) {
                insert(HH, *it + st, v);
                insert(HH, *it + (v < 10 ? st_2d : st), v);
            }
        }
        for(auto it = std::begin(hmssep); it != std::end(hmssep); ++it) {
            insert(HH, st + *it, v);
            insert(HH, (v < 10 ? st_2d : st) + *it, v);
            if(v < 13) {
                for(auto ith = std::begin(half); ith != std::end(half); ++ith) {
                    insert(HH, *ith + st + *it, v);
                    insert(HH, *ith + (v < 10 ? st_2d : st) + *it, v);
                }
            }
        }
    }
    insert(HH, L"正午", 12);

    hmssep[0] = L'分';
    for(int v = 0; v < 60; ++v) {
        std::wstring st = std::to_wstring(v);
        std::wstring st_2d = L'0' + std::to_wstring(v);

        insert(mi, st, v);
        insert(mi, (v < 10 ? st_2d : st), v);
        insert(mi, L':' + (v < 10 ? st_2d : st), v);
        for(auto it = std::begin(hmssep); it != std::end(hmssep); ++it) {
            insert(mi, st + *it, v);
            insert(mi, (v < 10 ? st_2d : st) + *it, v);
        }
    }

    hmssep[0] = L'秒';
    for(int v = 0; v < 60; ++v) {
        std::wstring st = std::to_wstring(v);
        std::wstring st_2d = L'0' + std::to_wstring(v);

        insert(SS, st, v);
        insert(SS, (v < 10 ? st_2d : st), v);
        insert(SS, L':' + (v < 10 ? st_2d : st), v);
        for(auto it = std::begin(hmssep) + 1; it != std::end(hmssep); ++it) {
            insert(SS, st + *it, v);
            insert(SS, (v < 10 ? st_2d : st) + *it, v);
        }
    }

    /* microseconds */
    for(int v = 0; v < 1000; ++v) {
        std::wstring st = std::to_wstring(v);
        if(v < 10) {
            insert(sss, st, v * 100000);
            st = L"00" + st;
        } else if(v < 100) {
            insert(sss, st, v * 10000);
            st = L'0' + st;
        }
        insert(sss, st, v * 1000);
    }

    for(int i = 0; i < 7; ++i) {
        for(int j = 0; j < 6; ++j) {
            auto&& w = weekday[i][j];
            insert(WW, w, i);
            if(j < 2) {
                insert(WW, w + L'.', i);
                insert(WW, w + L',', i);
                insert(WW, w.substr(0, 2) + L'.', i);
                insert(WW, w.substr(0, 2) + L',', i);
            }
        }
    }

    for(auto it = std::begin(tzone); it != std::end(tzone); ++it)
        insert(ZZ, it->first, it->second);

    for(int v = 0; v < 13; ++v) {
        std::wstring st;
        if(v < 10)
            st = L'0' + std::to_wstring(v);
        else
            st = std::to_wstring(v);

        for(int m = 0; m < 60; ++m) {
            std::wstring sm;
            if(m < 10)
                sm = L'0' + std::to_wstring(m);
            else
                sm = std::to_wstring(m);

            int sec = 60 * ((60 * v) + m);
            insert(ZZ, L'+' + st + sm, sec);
            insert(ZZ, L'-' + st + sm, -1 * sec);
            insert(ZZ, L'+' + st + L':' + sm, sec);
            insert(ZZ, L'-' + st + L':' + sm, -1 * sec);
        }
    }

    struct stat statBuf;
    if(stat(dirpath, &statBuf)) {
        if(mkdir_p(dirpath)) {
            return -1;
        }
    }

#if defined(_WIN32) || defined(_WIN64)
    std::size_t len = strnlen_s(dirpath, 255);
#else
    std::size_t len = strnlen(dirpath, 255);
#endif

    if(len == 0)
        return -1;
    std::string dp(dirpath);
#if defined(_WIN32) || defined(_WIN64)
    if(dirpath[len - 1] != '\\')
        dp += '\\';
#else
    if(dirpath[len - 1] != '/')
        dp += '/';
#endif

    const char* ext = ".dat";
    GG.save((dp + std::string("GG") + ext).data());
    YYYY.save((dp + std::string("YYYY") + ext).data());
    yy.save((dp + std::string("yy") + ext).data());
    MM.save((dp + std::string("MM") + ext).data());
    DD.save((dp + std::string("DD") + ext).data());
    HH.save((dp + std::string("HH") + ext).data());
    mi.save((dp + std::string("mi") + ext).data());
    SS.save((dp + std::string("SS") + ext).data());
    sss.save((dp + std::string("sss") + ext).data());
    WW.save((dp + std::string("WW") + ext).data());
    ZZ.save((dp + std::string("ZZ") + ext).data());
    { /* save VALIDATOR */
        FILE* fp = NULL;
        const char* magic = "TRIEDATE";
        auto _len = VALIDATOR.size();
#if defined(_WIN32) || defined(_WIN64)
        if(fopen_s(&fp, (dp + std::string("VALIDATOR") + ext).data(), "wb") != 0)
#else
        if((fp = fopen((dp + std::string("VALIDATOR") + ext).data(), "wb")) == NULL)
#endif
            return -1;
        fwrite(magic, 1, 8, fp);
        fwrite(&_len, sizeof(_len), 1, fp);
        for(auto it : VALIDATOR)
            fwrite(&it, sizeof(it), 1, fp);
        fclose(fp);
    }
    return 0;
}

int loader_datetime(const char* dirpath) {
#if defined(_WIN32) || defined(_WIN64)
    std::size_t len = strnlen_s(dirpath, 255);
#else
    std::size_t len = strnlen(dirpath, 255);
#endif

    if(len == 0)
        return -1;
    std::string dp(dirpath);
    if(dirpath[len - 1] != '/')
        dp += '/';

    const char* ext = ".dat";
    GG.load((dp + std::string("GG") + ext).data());
    YYYY.load((dp + std::string("YYYY") + ext).data());
    yy.load((dp + std::string("yy") + ext).data());
    MM.load((dp + std::string("MM") + ext).data());
    DD.load((dp + std::string("DD") + ext).data());
    HH.load((dp + std::string("HH") + ext).data());
    mi.load((dp + std::string("mi") + ext).data());
    SS.load((dp + std::string("SS") + ext).data());
    sss.load((dp + std::string("sss") + ext).data());
    WW.load((dp + std::string("WW") + ext).data());
    ZZ.load((dp + std::string("ZZ") + ext).data());
    { /* load VALIDATOR */
        FILE* fp = NULL;
        char magic[9] = {0};
        char checkmagic[9] = "TRIEDATE";
        std::size_t _len = (std::size_t)-1;

#if defined(_WIN32) || defined(_WIN64)
        if(fopen_s(&fp, (dp + std::string("VALIDATOR") + ext).data(), "rb") != 0) {
#else
        if((fp = fopen((dp + std::string("VALIDATOR") + ext).data(), "rb")) == NULL) {
#endif
            return -1;
        }
        if(fp == NULL)
            return -1;

        std::size_t r = fread(magic, 1, 8, fp);
        if(r < 8 || magic[0] != 0 || strcmp(magic, checkmagic)) {
            fclose(fp);
            return -1;
        }
        r = fread(&_len, sizeof(_len), 1, fp);
        if(r < 1 || len < 1) {
            fclose(fp);
            return -1;
        }

        std::size_t sz = sizeof(wchar_t);

        for(std::size_t i = 0; i < _len; i++) {
            wchar_t s = 0;
            if(fread(&s, sz, 1, fp) < 1)
                return -1;
            if(VALIDATOR.find(s) == VALIDATOR.end())
                VALIDATOR.insert(s);
        }
        fclose(fp);
    }
    return 0;
}

struct datetime {
    static const int monthes[12];
    union {
        std::tm timeinfo;
        struct {
            int sec;
            int min;
            int hour;
            int day;
            int month;
            int year;
            int weekday;
            int yearday;
            int isdst;
        };
    };
    int microsec;
    int offset;
    int noon;
    std::wstring tzname;

    struct _tzstr {
        union {
            wchar_t hmsu[13];
            struct {
                wchar_t sign;
                wchar_t h[2];
                wchar_t m[2];
                wchar_t s[2];
                wchar_t microsec[6];
            };
        };
    } tzstr{};

    datetime() : timeinfo(), microsec(0), offset(-1), noon(0), tzname() {}
    datetime(std::nullptr_t) : timeinfo(), microsec(0), offset(-1), noon(0), tzname() {}
    datetime(int _year, int _month, int _day, int _hour, int _minn, int _sec, int _mincrosec, int _offset = -1) {
        this->operator()(_year, _month, _day, _hour, _minn, _sec, microsec, _offset);
    }
    ~datetime() {}

    bool operator()(int _year,
                    int _month,
                    int _day,
                    int _hour,
                    int _min,
                    int _sec,
                    int _microsec,
                    int _offset = -1) noexcept {
        year = month = day = hour = min = sec = microsec = 0;
        offset = -1;

        if(_year == 0)
            return false;
        year = _year - 1900;
        if(_month < 1 || 12 < _month)
            return false;
        month = _month - 1;
        if((_month == 2 && _day == 29) && !((_year % 400 == 0) || ((_year % 4 == 0) && (_year % 100 != 0))))
            return false;
        if(_day < 1 || _day > monthes[_month - 1])
            return false;
        day = _day;

        if(_hour < 0 || 23 < _hour)
            return false;
        hour = _hour + noon;
        if(_min < 0 || 59 < _min)
            return false;
        min = _min;
        if(_sec < 0 || 59 < _sec)
            return false;
        sec = _sec;
        if(_microsec < 0 || 999999 < _microsec)
            return false;

        microsec = _microsec;
        if(microsec) {
            wchar_t* p = tzstr.microsec;
            int r = microsec;
            for(auto x : {100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL}) {
                *p++ = (wchar_t)((r / x) + 0x0030);
                r %= x;
            }
        }
        if(_offset != -1) {
            offset = _offset;
            if(_offset < 0) {
                _offset *= -1;
                tzstr.sign = L'-';
            } else {
                tzstr.sign = L'+';
            }

            int h, m, s, rest;

            h = _offset / 3600;
            tzstr.h[0] = h < 10 ? L'0' : (wchar_t)((h / 10) + 0x0030);
            tzstr.h[1] = (wchar_t)((h < 10 ? h : h % 10) + 0x0030);

            rest = _offset % 3600;
            m = rest / 60;
            tzstr.m[0] = m < 10 ? L'0' : (wchar_t)((m / 10) + 0x0030);
            tzstr.m[1] = (wchar_t)((m < 10 ? m : m % 10) + 0x0030);

            s = rest % 60;
            if(s || microsec) {
                tzstr.s[0] = s < 10 ? L'0' : (wchar_t)((s / 10) + 0x0030);
                tzstr.s[1] = (wchar_t)((s < 10 ? s : s % 10) + 0x0030);
            }
        }

        return true;
    }

    bool operator==(datetime& other) {
        return microsec == other.microsec && sec == other.sec && min == other.min && hour == other.hour &&
               day == other.day && month == other.month && year == other.year && offset == other.offset &&
               noon == other.noon && tzname == other.tzname;
    }

    bool operator==(std::nullptr_t) {
        return microsec == 0 && sec == 0 && min == 0 && hour == 0 && day == 0 && month == 0 && year == 0 &&
               offset == -1 && noon == 0 && tzname.empty();
    }
    bool operator!=(datetime& other) { return !operator==(other); }
    bool operator!=(std::nullptr_t) { return !operator==(nullptr); }

    template <typename... Tp>
    constexpr std::array<int, 9> triefind(const std::wstring& str, std::tuple<Tp...> NODES) noexcept {
        std::array<int, 9> ret = {0};
        ret[8] = -1;
        uint64_t i = 0;

        ret[0] = _find(str, &i, std::get<0>(NODES));

        if((ret[1] = _find(str, &i, std::get<1>(NODES))) == 0)
            return ret;

        ret[2] = _find(str, &i, std::get<2>(NODES));
        ret[3] = _find(str, &i, std::get<3>(NODES));
        ret[4] = _find(str, &i, std::get<4>(NODES));
        ret[5] = _find(str, &i, std::get<5>(NODES));
        ret[6] = _find(str, &i, std::get<6>(NODES));
        if(std::get<7>(NODES) != nullptr && i < str.size())
            ret[7] = _find(str, &i, std::get<7>(NODES));
        if(std::get<8>(NODES) != nullptr && i < str.size()) {
            uint64_t j = i;
            ret[8] = _find(str, &i, std::get<8>(NODES));
            tzname.clear();
            if(i - j < 3) {
                ret[8] = -1;
                return ret;
            }

            for(uint64_t count = 0; j < i; ++j) {
                auto _s = str[j];
                if(0x0040 < _s && _s < 0x005b) {
                    tzname += _s;
                    if(++count == 4)
                        break;
                }
            }

            if(!tzname.empty() && !ZZ.query(tzname))
                tzname.clear();
        }
        return ret;
    }

    std::wstring strftime(const wchar_t* format) {
        /* formatter for microsecond and timezone*/
        const int alen = 80;
        wchar_t newformat[alen] = {0};
        wchar_t* p = &newformat[0];
#if defined(_WIN32) || defined(_WIN64)
        uint64_t n = wcsnlen_s(format, alen);
#else
        uint64_t n = wcsnlen(format, alen);
#endif
        if(!n)
            return format;

        for(auto ch = format, end = format + n; ch != end; ++ch) {
            if(*ch != L'%') {
                *p++ = *ch;
                continue;
            }

            ++ch;
            if(*ch == L'f') {
                for(uint64_t i = 0; i < 6; i++)
                    *p++ = tzstr.microsec[0] ? tzstr.microsec[i] : L'0';
            } else if(*ch == L'z') {
                if(tzstr.hmsu[0]) {
                    for(uint64_t i = 0; i < 15 && tzstr.hmsu + i; i++)
                        *p++ = tzstr.hmsu[i];
                }
            } else if(*ch == L'Z') {
                if(tzname[0]) {
                    for(uint64_t i = 0, len = tzname.size(); i < len; i++)
                        *p++ = tzname[i];
                }
            } else {
                *p++ = L'%';
                *p++ = *ch;
            }
        }

        wchar_t buffer[alen * 2] = {0};
        if(std::wcsftime(buffer, alen * 2, newformat, &timeinfo))
            return buffer;
        return NULL;
    }
    // std::wstring strftime(const wchar_t* format) {

    //     /* formatter for microsecond and timezone*/
    //     wchar_t ch;
    //     wchar_t zreplace[13] = {0};  // the string to use for %z
    //     uint64_t i = 0, n = 0;
    //     const int alen = 80;
    //     wchar_t newformat[alen] = {0};
    //     wchar_t* pos = newformat;
    //     n = wcsnlen_s(format, alen);

    //     while(i < n) {
    //         ch = format[i];
    //         i += 1;
    //         if(ch == L'%') {
    //             if(i < n) {
    //                 ch = format[i];
    //                 i += 1;
    //                 if(ch == L'f') {
    //                     memcpy_s(pos, alen * sizeof(wchar_t), tzstr.microsec, 6 * sizeof(wchar_t));
    //                     pos += 6;
    //                 } else if(ch == L'z') {
    //                     if(zreplace[0] == NULL && offset != -1 && offset != 0)
    //                         memcpy_s(zreplace, 13 * sizeof(wchar_t), tzstr.hmsu, 13 * sizeof(wchar_t));
    //                     memcpy_s(pos, 13 * sizeof(wchar_t), zreplace, 13 * sizeof(wchar_t));
    //                     pos += wcsnlen_s(zreplace, 13);

    //                 } else if(ch == L'Z') {
    //                     uint64_t tlen = tzname.size();
    //                     memcpy_s(pos, 4 * sizeof(wchar_t), tzname.data(), tlen * sizeof(wchar_t));
    //                     pos += tlen;
    //                 } else {
    //                     *pos++ = L'%';
    //                     *pos++ = ch;
    //                 }
    //             } else {
    //                 *pos++ = L'%';
    //             }
    //         } else {
    //             *pos++ = ch;
    //         }
    //     }

    //     /* relay time format */
    //     wchar_t buffer[alen * 2] = {0};
    //     if(std::wcsftime(buffer, alen * 2, newformat, &timeinfo))
    //         return std::wstring(buffer);
    //     return NULL;
    // }

    static constexpr int _find(const std::wstring& str, uint64_t* i, std::nullptr_t) noexcept { return 0; }

    template <std::size_t N>
    static constexpr int _find(const std::wstring& str, uint64_t* i, const Trie<N>* node) noexcept {
        wchar_t s = L' ';

        uint64_t nid = 0;
        const int nlim = (sizeof(node->nodes[nid].first) / sizeof(int)) - 1;
        const uint64_t strlen = str.size();
        const uint64_t strlim = strlen - 1;

        while(*i < strlen && s) {
            s = str[*i];
            if(!s)
                break;

            *i += 1;
            if(s == L' ' || s == L'\u3000')
                continue;

            if(*i < strlim && s == L'T' && str[*i + 1] != L'h')
                break;

            uint64_t sid = (uint64_t)TRAN.at(s);
            if(nlim < sid) {
                if(*i == 1)
                    return 0;
                *i -= 1;
                break;
            }

            if(node->nodes[nid].first[sid] == -1) {
                *i -= 1;
                break;
            }
            nid = (uint64_t)node->nodes[nid].first[sid];
        }
        return node->nodes[nid].second;
    }
};
const int datetime::monthes[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void const_datetime() {
    if(YYYY.len == 1) {
#if defined(_WIN32) || defined(_WIN64)
        char* pth;
        size_t len;
        if(_dupenv_s(&pth, &len, "TMP"))
            return;
        std::string dirpath(pth);
        dirpath += "\\_ccore_datetimedata";
        free(pth);
#else
        const char* pth = getenv("TMP");
        if(!pth)
            pth = "/tmp";
        std::string dirpath(pth);
        dirpath += "/_ccore_datetimedata";
#endif
        if(loader_datetime(dirpath.data()) == -1) {
            builder_datetime(dirpath.data());
            loader_datetime(dirpath.data());
        }
    }
}

datetime parse_datetime(const std::wstring& str, const bool dayfirst = false) noexcept {
    int numcount = 0;
    std::array<int, 9> r;
    datetime dt = nullptr;

    std::size_t len = 0;
    std::size_t nt = 0;

    for(auto &&it = str.crbegin(), end = str.crend(); it != end; ++it, ++len) {
        if(NUMBERS.find(*it) != NUMBERS.end() && ++numcount > 9)
            break;

        if(*it == L'm' || *it == L'M') {
            auto&& n = std::tolower(*(it + 1));
            if(n == L'p' || (n == L'.' && (*(it + 2) == L'p' || *(it + 2) == L'P')))
                dt.noon = 12;
        } else if(*it == L'後' && *(it + 1) == L'午') {
            dt.noon = 12;
        } else if(*it == L'/' || *it == L'-' || *it == L',' || *it == L'年' || *it == L'月') {
            ++nt;
        }
    }

    if(nt == 0 && len - numcount < 4 && (str[2] == L':' || numcount == 4 || numcount == 6 || numcount == 9)) {
        r = dt.triefind(str, std::make_tuple(&HH, &mi, &SS, &sss, nullptr, nullptr, nullptr, nullptr, &ZZ));
        if(dt(1970, 1, 1, r[0], r[1], r[2], r[3], r[8]))
            return dt;
    }

    if(NUMBERS.find(str[2]) == NUMBERS.end()) {
        r = dt.triefind(str, std::make_tuple(&WW, &MM, &DD, &YYYY, &HH, &mi, &SS, &sss, &ZZ));
        if(r[3] && r[1] && r[2]) {
            if(dt(r[3], r[1], r[2], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

        r = dt.triefind(str, std::make_tuple(&WW, &DD, &MM, &YYYY, &HH, &mi, &SS, &sss, &ZZ));
        if(r[3] && r[2] && r[1]) {
            if(dt(r[3], r[2], r[1], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }
    }

    if(dayfirst == false) {
        r = dt.triefind(str, std::make_tuple(&YYYY, &MM, &DD, &WW, &HH, &mi, &SS, &sss, &ZZ));
        if(r[0] && r[1] && r[2]) {
            if(dt(r[0], r[1], r[2], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

        r = dt.triefind(str, std::make_tuple(&YYYY, &DD, &MM, &WW, &HH, &mi, &SS, &sss, &ZZ));
        if(r[0] && r[2] && r[1]) {
            if(dt(r[0], r[2], r[1], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

    } else {
        r = dt.triefind(str, std::make_tuple(&YYYY, &DD, &MM, &WW, &HH, &mi, &SS, &sss, &ZZ));
        if(r[0] && r[2] && r[1]) {
            if(dt(r[0], r[2], r[1], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

        r = dt.triefind(str, std::make_tuple(&YYYY, &MM, &DD, &WW, &HH, &mi, &SS, &sss, &ZZ));
        if(r[0] && r[1] && r[2]) {
            if(dt(r[0], r[1], r[2], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }
    }

    r = dt.triefind(str, std::make_tuple(&DD, &MM, &yy, &WW, &HH, &mi, &SS, &sss, &ZZ));
    if(r[0] && r[1] && r[2]) {
        r[2] += r[2] < 60 ? 2000 : 1900;
        if(dt(r[2], r[1], r[0], r[4], r[5], r[6], r[7], r[8]))
            return dt;
    }

    r = dt.triefind(str, std::make_tuple(&MM, &DD, &WW, &HH, &mi, &SS, &sss, nullptr, &ZZ));
    if(r[0] && r[1]) {
        if(dt(1970, r[0], r[1], r[3], r[4], r[5], r[6], r[8]))
            return dt;
    }

    r = dt.triefind(str, std::make_tuple(&GG, &yy, &MM, &DD, &WW, &HH, &mi, &SS, nullptr));
    if(r[0] && r[1] && r[2] && r[3]) {
        if(dt(r[0] + r[1] - 1, r[2], r[3], r[5], r[6], r[7], 0, 32400))
            return dt;
    }

    return nullptr;
}

datetime to_datetime(const std::wstring& str, const bool dayfirst = false, const uint64_t minlimit = 3) {
    const_datetime();
    uint64_t i = 0, j = 0, k = 0, c = 0, beg = 0, last = 0;
    int ps = 0, ww = 0;
    wchar_t ts = 0;
    datetime dt = nullptr;
    bool isbothkako = false;
    const uint64_t len_2 = str.size() - 2;

    for(auto &&s = str.cbegin(), end = str.cend() + 1; s != end; ++s, ++j) {
        if(*s == L'(' || *s == L')' || *s == L'（' || *s == L'）') {
            ps = TRAN.at(*s);
            ww = 0;
            isbothkako = false;

            if(j < len_2 && ps == 45) {
                ts = str[j + 1];
                ww = TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts);
                ts = str[j + 2];
                isbothkako = (TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts)) == 46;
            } else if(j > 1 && ps == 46) {
                ts = str[j - 1];
                ww = TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts);
                ts = str[j - 2];
                isbothkako = (TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts)) == 45;
            }
            if(isbothkako && 36 < ww && ww < 45) {
                i += 1;
                continue;
            }

        } else if(i == 0 && (*s == L' ' || *s == L'–' || *s == L'-' || *s == L'_')) {
            continue;
        } else if(VALIDATOR.find(*s) != VALIDATOR.end()) {
            i += 1;
            continue;
        }

        if(i > minlimit) {
            c = 0;
            beg = j - i;
            last = j;

            for(k = j - 1; k > beg || k == 0; --k) {
                if(k == (std::size_t)-1)
                    break;
                ts = str[k];
                if(c == 0 && (ts == L' ' || ts == L'–' || ts == L'-' || ts == L'_')) {
                    --last;
                    continue;
                }
                c += (VALIDATOR.find(ts) != VALIDATOR.end());
                if(c > minlimit && (dt = parse_datetime(str.substr(beg, last - beg), dayfirst)) != nullptr)
                    return dt;
            }
        }
        i = 0;
    }
    return dt;
}

PyObject* extractdate(const std::wstring& str, const bool dayfirst = false, const uint64_t minlimit = 3) {
    const_datetime();
    uint64_t i = 0, j = 0, k = 0, c = 0, beg = 0, last = 0;
    int ps = 0, ww = 0;
    wchar_t ts = 0;
    PyObject* ret = PyList_New(0);
    datetime dt = nullptr;
    bool isbothkako = false;
    const uint64_t len_2 = str.size() - 2;

    for(auto &&s = str.begin(), end = str.end() + 1; s != end; ++s, ++j) {
        if(*s == L'(' || *s == L')' || *s == L'（' || *s == L'）') {
            ps = TRAN.at(*s);
            ww = 0;
            isbothkako = false;

            if(j < len_2 && ps == 45) {
                ts = str[j + 1];
                ww = TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts);
                ts = str[j + 2];
                isbothkako = (TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts)) == 46;
            } else if(j > 1 && ps == 46) {
                ts = str[j - 1];
                ww = TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts);
                ts = str[j - 2];
                isbothkako = (TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts)) == 45;
            }
            if(isbothkako && 36 < ww && ww < 45) {
                i += 1;
                continue;
            }

        } else if(i == 0 && (*s == L' ' || *s == L'–' || *s == L'-' || *s == L'_')) {
            continue;
        } else if(VALIDATOR.find(*s) != VALIDATOR.end()) {
            i += 1;
            continue;
        }

        if(i > minlimit) {
            c = 0;
            beg = j - i;
            last = j;

            for(k = j - 1; k > beg || k == 0; --k) {
                ts = str[k];
                if(c == 0 && (ts == L' ' || ts == L'–' || ts == L'-' || ts == L'_')) {
                    --last;
                    continue;
                }
                c += (VALIDATOR.find(ts) != VALIDATOR.end());
                if(c > minlimit) {
                    if((dt = parse_datetime(str.substr(beg, last - beg), dayfirst)) != nullptr) {
                        auto en = last - beg;
                        auto substr = str.substr(beg, en);
                        PyObject* u = PyUnicode_FromWideChar(substr.data(), (Py_ssize_t)substr.size());
                        if(u) {
                            PyList_Append(ret, u);
                            Py_DECREF(u);
                        }
                    }
                    break;
                }
            }
        }
        i = 0;
    }
    return ret;
}

std::wstring normalized_datetime(const std::wstring& str,
                                 const wchar_t* format = L"%Y/%m/%d %H:%M:%S",
                                 const bool dayfirst = false,
                                 const uint64_t minlimit = 3) {
    const_datetime();
    uint64_t i = 0, j = 0, k = 0, t = 0, c = 0, beg = 0, last = 0;
    int ps = 0, ww = 0;
    wchar_t ts = 0;
    std::wstring ret;
    datetime dt = nullptr;
    bool isbothkako = false;
    const uint64_t len_2 = str.size() - 2;

    for(auto &&s = str.cbegin(), end = str.cend() + 1; s != end; ++s, ++j) {
        if(i == 0)
            t = j;

        if(*s == L'(' || *s == L')' || *s == L'（' || *s == L'）') {
            ps = TRAN.at(*s);
            ww = 0;
            isbothkako = false;

            if(j < len_2 && ps == 45) {
                ts = str[j + 1];
                ww = TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts);
                ts = str[j + 2];
                isbothkako = (TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts)) == 46;
            } else if(j > 1 && ps == 46) {
                ts = str[j - 1];
                ww = TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts);
                ts = str[j - 2];
                isbothkako = (TRAN.find(ts) == TRAN.end() ? 0 : TRAN.at(ts)) == 45;
            }
            if(isbothkako && 36 < ww && ww < 45) {
                i += 1;
                continue;
            }

        } else if(i == 0 && (*s == L' ' || *s == L'–' || *s == L'-' || *s == L'_')) {
            ret += *s;
            continue;
        } else if(VALIDATOR.find(*s) != VALIDATOR.end()) {
            i += 1;
            continue;
        }

        if(i > minlimit) {
            c = 0;
            beg = j - i;
            last = j;

            for(k = j - 1; k > beg || k == 0; --k) {
                ts = str[k];
                if(c == 0 && (ts == L' ' || ts == L'–' || ts == L'-' || ts == L'_')) {
                    --last;
                    continue;
                }
                c += (VALIDATOR.find(ts) != VALIDATOR.end());
                if(c > minlimit)
                    break;
            }
            if(c > minlimit) {
                if((dt = parse_datetime(str.substr(beg, last - beg), dayfirst)) == nullptr)
                    ret += str.substr(beg, last - beg);
                else
                    ret += dt.strftime(format);
                if(last < j + 1)
                    ret += str.substr(last, j + 1 - last);
            } else {
                ret += str.substr(beg, j + 1 - beg);
            }
        } else if(t == j) {
            ret += *s;
        } else {
            ret += str.substr(t, j + 1 - t);
        }
        i = 0;
    }
    return ret;
}

extern "C" PyObject* flatten_py(PyObject* self, PyObject* args) {
    PyObject *iterable, *mapping;
    if(!PyArg_UnpackTuple(args, "_count_elements", 1, 1, &iterable))
        return NULL;

    if(iterable == NULL)
        return NULL;

    mapping = PyList_New(0);

    if(!flatten(mapping, iterable)) {
        PyErr_Clear();
        PyList_Append(mapping, iterable);
    }
    return mapping;
}

extern "C" PyObject* listify_py(PyObject* self, PyObject* args) {
    PyObject *iterable, *mapping;
    if(!PyArg_UnpackTuple(args, "_count_elements", 1, 1, &iterable))
        return NULL;

    if(iterable == NULL)
        return NULL;

    if(iterable == Py_None)
        return PyList_New(0);

    if(PyList_Check(iterable))
        return iterable;

    if(PyTuple_Check(iterable) || PyDict_Check(iterable) || PyAnySet_Check(iterable) || PyGen_Check(iterable) ||
       PyIter_Check(iterable) || PyObject_CheckBuffer(iterable) || PyObject_TypeCheck(iterable, &PyDictItems_Type) ||
       PyObject_TypeCheck(iterable, &PyDictKeys_Type) || PyObject_TypeCheck(iterable, &PyDictValues_Type)) {
        return PySequence_List(iterable);
    }
    mapping = PyList_New(0);
    PyList_Append(mapping, iterable);
    return mapping;
}

extern "C" PyObject* nkf_guess_py(PyObject* self, PyObject* args) {
    unsigned char* str;
    PyObject *res, *o;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = (unsigned char*)PyBytes_AsString(o)) == NULL)
        return NULL;

    res = pynkf_convert_guess(str, (int)PyObject_Length(o));
    return res;
}

extern "C" PyObject* to_hankaku_py(PyObject* self, PyObject* args) {
    PyObject* str;

    if(!PyArg_ParseTuple(args, "O", &str))
        return NULL;

    if(!PyUnicode_Check(str) || PyUnicode_READY(str) == -1)
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");

    unsigned int kind = PyUnicode_KIND(str);
    Py_ssize_t len;
    wchar_t* wdat = PyUnicode_AsWideCharString(str, &len);
    if(wdat == NULL)
        return PyErr_Format(PyExc_MemoryError, "Unknow Error.");
    if(len == 0 || kind == 1)
        return str;
    auto&& res = to_hankaku(wdat, (std::size_t)len);
    PyMem_Free(wdat);

    if(!res.empty())
        return PyUnicode_FromWideChar(res.data(), (Py_ssize_t)res.size());
    return PyErr_Format(PyExc_RuntimeError, "Unknown converting error");
}

extern "C" PyObject* to_zenkaku_py(PyObject* self, PyObject* args) {
    PyObject* str;

    if(!PyArg_ParseTuple(args, "O", &str))
        return NULL;

    if(!PyUnicode_Check(str) || PyUnicode_READY(str) == -1)
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");

    Py_ssize_t len;
    wchar_t* wdat = PyUnicode_AsWideCharString(str, &len);
    if(wdat == NULL)
        return PyErr_Format(PyExc_MemoryError, "Unknow Error.");
    if(len == 0)
        return str;

    auto&& res = to_zenkaku(wdat, (std::size_t)len);
    PyMem_Free(wdat);

    if(!res.empty())
        return PyUnicode_FromWideChar(res.data(), (Py_ssize_t)res.size());
    return PyErr_Format(PyExc_RuntimeError, "Unknown converting error");
}

extern "C" PyObject* kanji2int_py(PyObject* self, PyObject* args) {
    PyObject *str, *res = NULL;
    std::size_t len = (std::size_t)-1;
    unsigned int kind;

    if(!PyArg_ParseTuple(args, "O", &str))
        return NULL;

    if(PyUnicode_READY(str) == -1 || (len = (std::size_t)PyObject_Length(str)) == (std::size_t)-1)
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");

    if((kind = PyUnicode_KIND(str)) == 1)
        return str;

    res = Kansuji::kanji2int(str);
    if(res == NULL)
        return NULL;

    return res;
}

extern "C" PyObject* int2kanji_py(PyObject* self, PyObject* args) {
    PyObject *num, *res = NULL;
    if(!PyArg_ParseTuple(args, "O", &num))
        return NULL;
    res = Kansuji::int2kanji(num);
    if(res == NULL)
        return NULL;

    return res;
}

extern "C" PyObject* lookuptype_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char *str, *res;
    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = lookuptype(str, (std::size_t)PyObject_Length(o));
    if(res != NULL)
        return PyUnicode_FromString(res);
    else
        Py_RETURN_NONE;
}

extern "C" PyObject* is_tar_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_tar(str);
    return PyBool_FromLong(res);
}

extern "C" PyObject* is_lha_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_lha(str);
    return PyBool_FromLong(res);
}

extern "C" PyObject* is_xls_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_xls(str, (std::size_t)PyObject_Length(o));
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_doc_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_doc(str, (std::size_t)PyObject_Length(o));
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_ppt_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_ppt(str, (std::size_t)PyObject_Length(o));
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_xml_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_xml(str);
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_html_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_html(str);
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_json_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_json(str);
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_dml_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_dml(str, (std::size_t)PyObject_Length(o));
    return PyBool_FromLong(res);
}
extern "C" PyObject* is_csv_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_csv(str, (std::size_t)PyObject_Length(o));
    return PyBool_FromLong(res);
}

extern "C" PyObject* to_datetime_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;
    wchar_t* str;
    datetime res;

    int dayfirst = false;
    uint64_t minlimit = uint64_t(3);
    const char* kwlist[5] = {"o", "dayfirst", "minlimit", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ii", (char**)kwlist, &o, &dayfirst, &minlimit))
        return NULL;

    if(PyDate_Check(o))
        return o;
    else if(!PyUnicode_Check(o))
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");
    Py_ssize_t len;
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)
        return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");

    res = to_datetime(str, (bool)dayfirst, minlimit);
    PyMem_Free(str);

    if(res == nullptr)
        Py_RETURN_NONE;
    else if(res.offset == -1)
        return PyDateTime_FromDateAndTime(res.year + 1900, res.month + 1, res.day, res.hour, res.min, res.sec,
                                          res.microsec);

    PyDateTime_DateTime* dt = (PyDateTime_DateTime*)PyDateTime_FromDateAndTime(
        res.year + 1900, res.month + 1, res.day, res.hour, res.min, res.sec, res.microsec);

#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 7
    PyObject* timedelta = PyDelta_FromDSU(0, res.offset, 0);
    if(res.tzname.empty()) {
        dt->tzinfo = PyTimeZone_FromOffset(timedelta);
    } else {
        PyObject* name = PyUnicode_FromWideChar(res.tzname.data(), (Py_ssize_t)res.tzname.size());
        dt->tzinfo = PyTimeZone_FromOffsetAndName(timedelta, name);
        Py_DECREF(name);
    }
    dt->hastzinfo = 1;
    Py_DECREF(timedelta);
#endif
    return (PyObject*)dt;
}

extern "C" PyObject* extractdate_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject *o, *res;
    wchar_t* str;

    int dayfirst = false;
    uint64_t minlimit = uint64_t(3);
    const char* kwlist[5] = {"o", "dayfirst", "minlimit", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ii", (char**)kwlist, &o, &dayfirst, &minlimit))
        return NULL;

    if(!PyUnicode_Check(o))
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");
    Py_ssize_t len;
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)
        return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");

    res = extractdate(str, (bool)dayfirst, minlimit);
    PyMem_Free(str);
    if(res)
        return res;
    else
        Py_RETURN_NONE;
}

extern "C" PyObject* normalized_datetime_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;
    wchar_t* str = NULL;
    wchar_t* fmt = NULL;
    std::wstring res;

    PyObject* format = NULL;
    int dayfirst = false;
    uint64_t minlimit = uint64_t(3);
    const char* kwlist[5] = {"o", "format", "dayfirst", "minlimit", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oii", (char**)kwlist, &o, &format, &dayfirst, &minlimit))
        return NULL;

    if(!PyUnicode_Check(o))
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");
    Py_ssize_t len;
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)
        return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");

    if(format) {
        if(!PyUnicode_Check(format))
            return PyErr_Format(PyExc_ValueError, "Need strftime formating unicode string.");
        if((fmt = PyUnicode_AsWideCharString(format, &len)) == NULL)
            return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");
    }

    res = normalized_datetime(str, fmt ? fmt : L"%Y/%m/%d %H:%M:%S", (bool)dayfirst, minlimit);
    PyMem_Free(str);
    if(fmt)
        PyMem_Free(fmt);

    if(!res.empty())
        return PyUnicode_FromWideChar(res.data(), (Py_ssize_t)(res.size() - 1));
    else
        Py_RETURN_NONE;
}

#define MODULE_NAME ccore
#define MODULE_NAME_S "ccore"

/* {{{ */
// this module description
#define MODULE_DOCS "\n"

#define flatten_DESC "Always return 1D array(flatt list) object\n"
#define listify_DESC "Always return list object.\n"
#define getencoding_DESC "guess encoding from binary data.\n"
#define to_hankaku_DESC "from zenkaku data to hankaku data.\n"
#define to_zenkaku_DESC "from hankaku data to zenkaku data.\n"
#define kanji2int_DESC "from kanji num char to arabic num char.\n"
#define int2kanji_DESC "from arabic num char to kanji num char.\n"
#define lookuptype_DESC "lookup first file type.\n"
#define is_tar_DESC "tar file header check.\n"
#define is_lha_DESC "lha file header check.\n"
#define is_xls_DESC "Microsoft Excel file header check.\n"
#define is_doc_DESC "Microsoft word file header check.\n"
#define is_ppt_DESC "Microsoft PowerPoint file header check.\n"
#define is_xml_DESC "XML file header check. very dirty check\n"
#define is_html_DESC "HTML file header check. very dirty check\n"
#define is_json_DESC "JSON file header check. very dirty check\n"
#define is_dml_DESC "DML file check.\n"
#define is_csv_DESC "CSV file check.\n"
#define to_datetime_DESC "guess datetimeobject from maybe datetime strings\n"
#define extractdate_DESC "extract datetimestrings from maybe datetime strings\n"
#define normalized_datetime_DESC "replace from maybe datetime strings to normalized datetime strings\n"

/* }}} */
#define PY_ADD_METHOD(py_func, c_func, desc) \
    { py_func, (PyCFunction)c_func, METH_VARARGS, desc }
#define PY_ADD_METHOD_KWARGS(py_func, c_func, desc) \
    { py_func, (PyCFunction)c_func, METH_VARARGS | METH_KEYWORDS, desc }

/* Please extern method define for python */
/* PyMethodDef Parameter Help
 * https://docs.python.org/ja/3/c-api/structures.html#c.PyMethodDef
 */
static PyMethodDef py_methods[] = {
    PY_ADD_METHOD("flatten", flatten_py, flatten_DESC),
    PY_ADD_METHOD("listify", listify_py, listify_DESC),
    PY_ADD_METHOD("getencoding", nkf_guess_py, getencoding_DESC),
    PY_ADD_METHOD("to_hankaku", to_hankaku_py, to_hankaku_DESC),
    PY_ADD_METHOD("to_zenkaku", to_zenkaku_py, to_zenkaku_DESC),
    PY_ADD_METHOD("kanji2int", kanji2int_py, kanji2int_DESC),
    PY_ADD_METHOD("int2kanji", int2kanji_py, int2kanji_DESC),
    PY_ADD_METHOD("lookuptype", lookuptype_py, lookuptype_DESC),
    PY_ADD_METHOD("is_tar", is_tar_py, is_tar_DESC),
    PY_ADD_METHOD("is_lha", is_lha_py, is_lha_DESC),
    PY_ADD_METHOD("is_xls", is_xls_py, is_xls_DESC),
    PY_ADD_METHOD("is_doc", is_doc_py, is_doc_DESC),
    PY_ADD_METHOD("is_ppt", is_ppt_py, is_ppt_DESC),
    PY_ADD_METHOD("is_xml", is_xml_py, is_xml_DESC),
    PY_ADD_METHOD("is_html", is_html_py, is_html_DESC),
    PY_ADD_METHOD("is_json", is_json_py, is_json_DESC),
    PY_ADD_METHOD("is_dml", is_dml_py, is_dml_DESC),
    PY_ADD_METHOD("is_csv", is_csv_py, is_csv_DESC),
    PY_ADD_METHOD_KWARGS("to_datetime", to_datetime_py, to_datetime_DESC),
    PY_ADD_METHOD_KWARGS("extractdate", extractdate_py, extractdate_DESC),
    PY_ADD_METHOD_KWARGS("normalized_datetime", normalized_datetime_py, normalized_datetime_DESC),
    {NULL, NULL, 0, NULL}};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef py_defmod = {PyModuleDef_HEAD_INIT, MODULE_NAME_S, MODULE_DOCS, 0, py_methods};
#define PARSE_NAME(mn) PyInit_##mn
#define PARSE_FUNC(mn)                      \
    PyMODINIT_FUNC PARSE_NAME(mn)() {       \
        PyDateTime_IMPORT;                  \
        return PyModule_Create(&py_defmod); \
    }

#else
#define PARSE_NAME(mn) \
    init##mn(void) { (void)Py_InitModule3(MODULE_NAME_S, py_methods, MODULE_DOCS); }
#define PARSE_FUNC(mn) PyMODINIT_FUNC PARSE_NAME(mn)
#endif

PARSE_FUNC(MODULE_NAME);
