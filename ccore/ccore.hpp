
/* ccore.cpp | MIT License | https://github.com/kirin123kirin/ccore/raw/ccore.exe/LICENSE */

#pragma once
#ifndef CCORE_HPP
#define CCORE_HPP

#include <Python.h>
#include <datetime.h>
#include <setjmp.h>
#include <array>
#include <ctime>
#include <numeric>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if PY_MAJOR_VERSION == 2
#define PyUnicode_DATA PyUnicode_AS_DATA
#define PyUnicode_KIND(x) 2
#define PyUnicode_READY(x) true
#endif

#define IS_WIN _WIN32 || _WIN64

#if IS_WIN
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

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

// static std::unordered_map<wchar_t, const wchar_t*> ZEN2HAN;
#include "../resource/ZEN2HAN.const"

// static const std::unordered_map<wchar_t, wchar_t> han2zen;
#include "../resource/han2zen.const"

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

// static std::unordered_map<char, std::vector<dic>> start =
#include "../resource/binaryheaders.const"

struct reg {
    const char* key;
    const char* val;
    std::regex re;
    reg() : key(0), val(0) {}
    reg(std::nullptr_t) : key(0), val(0) {}
    reg(const char* _k, const char* _v) : key(_k), val(_v), re(std::regex(_k)) {}
    bool match(const char* b) const noexcept { return std::regex_match(b, re); }
};

// static std::unordered_map<char, std::vector<reg>> regs =
#include "../resource/binaryregexheaders.const"

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
        Py_ssize_t len = PyObject_Length(u);
        data_type wdat = PyMem_NEW(value_type, len + 1);
        PyUnicode_AsWideChar(u, wdat, len + 1);
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

// const std::unordered_map<Kansuji::value_type, Kansuji::index_type> Kansuji::WK_UNIT =
#include "../resource/Kansuji_WK_UNIT.const"

// const std::unordered_map<Kansuji::value_type, Kansuji::index_type> Kansuji::D3_UNIT = {
#include "../resource/Kansuji_D3_UNIT.const"

// const std::unordered_map<Kansuji::value_type, Kansuji::index_type> Kansuji::D4_UNIT = {
#include "../resource/Kansuji_D4_UNIT.const"

// const std::unordered_map<Kansuji::value_type, Kansuji::value_type, Kansuji::no_hash> Kansuji::Collections =
#include "../resource/Kansuji_Collections.const"

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

inline bool is_office(const char* b, std::size_t len) {
    if(b[0] == '\x50' && b[1] == '\x4B') {
        if(memcmp(b + 30, "[Content_Types].xml", 19) == 0 && memstr(b, len, "\x00ppt/", 4))
            return true;
        if(memcmp(b + 30, "mimetypeapplication/vnd.oasis.opendocument.", 73) == 0)
            return true;
    } else if(memcmp(b + 0, "\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1", 8) == 0)
        return true;
    else if(memcmp(b + 0, "\x00\x01\x00\x00Standard Jet DB\x00", 19) == 0)
        return true;
    else if(memcmp(b + 0, "\x00\x01\x00\x00Standard ACE DB\x00", 19) == 0)
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

// static const std::unordered_map<wchar_t, int, nohash<wchar_t>> TRAN = {
#include "../resource/TRAN.const"

// static const std::unordered_set<wchar_t, nohash<wchar_t>> NUMBERS = {
#include "../resource/NUMBERS.const"

// static std::unordered_set<wchar_t, nohash<wchar_t>> VALIDATOR = {
#include "../resource/VALIDATOR.const"

static int mkdir_p(const char* filepath) {
    char* p = NULL;
    char* buf = NULL;

    std::size_t buflen = strlen(filepath) + 4;
    buf = (char*)malloc(buflen);
    if(buf == NULL) {
        return -1;
    }

#if IS_WIN
    strcpy_s(buf, buflen, filepath);
    for(p = strchr(buf + 1, '\\'); p; p = strchr(p + 1, '\\')) {
#else
    strcpy(buf, filepath);
    for(p = strchr(buf + 1, '/'); p; p = strchr(p + 1, '/')) {
#endif
        *p = '\0';

#if IS_WIN
        struct _stat sb = {0};
        if(_stat(filepath, &sb) == 0) {
#else
        struct stat sb = {0};
        if(stat(filepath, &sb) == 0) {
#endif
            free(buf);
            return 0;
        }

#if IS_WIN
        if(_mkdir(filepath)) {
#else
        if(mkdir(filepath, 0777)) {
#endif
            free(buf);
            return -1;
        }

#if IS_WIN
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

    void insert(const std::wstring& str, int value) noexcept {
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

    int common_prefix(const std::wstring& str) noexcept {
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

    bool query(const std::wstring& str) noexcept {
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

#if IS_WIN
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

#if IS_WIN
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
// static const std::wstring ml[12][2] = {
#include "../resource/datetime_month.const"

// static const std::wstring weekday[7][6] = {
#include "../resource/datetime_weekday.const"

// static const std::vector<std::pair<std::wstring, int>> gengo = {
#include "../resource/datetime_gengo.const"

// static const std::wstring half[] = {
#include "../resource/datetime_half.const"

// static const std::vector<std::pair<std::wstring, int>> tzone = {
#include "../resource/datetime_tzone.const"

// static wchar_t ymdsep[];
#include "../resource/datetime_ymdsep.const"

// static wchar_t hmssep[];
#include "../resource/datetime_hmssep.const"

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

#if IS_WIN
    struct _stat statBuf;
    if(_stat(dirpath, &statBuf)) {
#else
    struct stat statBuf;
    if(stat(dirpath, &statBuf)) {
#endif
        if(mkdir_p(dirpath)) {
            return -1;
        }
    }

#if IS_WIN
    std::size_t len = strnlen_s(dirpath, 255);
#else
    std::size_t len = strnlen(dirpath, 255);
#endif

    if(len == 0)
        return -1;
    std::string dp(dirpath);
#if IS_WIN
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
#if IS_WIN
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
#if IS_WIN
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

#if IS_WIN
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

    template <typename _T0,
              typename _T1,
              typename _T2,
              typename _T3,
              typename _T4,
              typename _T5,
              typename _T6,
              typename _T7,
              typename _T8>
    constexpr std::array<int, 9>
    triefind(const std::wstring& str, _T0 n0, _T1 n1, _T2 n2, _T3 n3, _T4 n4, _T5 n5, _T6 n6, _T7 n7, _T8 n8) noexcept {
        std::array<int, 9> ret = {0};
        ret[8] = -1;
        uint64_t i = 0;

        ret[0] = _find(str, &i, n0);

        if((ret[1] = _find(str, &i, n1)) == 0)
            return ret;

        ret[2] = _find(str, &i, n2);
        ret[3] = _find(str, &i, n3);
        ret[4] = _find(str, &i, n4);
        ret[5] = _find(str, &i, n5);
        ret[6] = _find(str, &i, n6);
        if(n7 != nullptr && i < str.size())
            ret[7] = _find(str, &i, n7);
        if(n8 != nullptr && i < str.size()) {
            uint64_t j = i;
            ret[8] = _find(str, &i, n8);
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
#if IS_WIN
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
#if IS_WIN
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
        r = dt.triefind(str, &HH, &mi, &SS, &sss, nullptr, nullptr, nullptr, nullptr, &ZZ);
        if(dt(1970, 1, 1, r[0], r[1], r[2], r[3], r[8]))
            return dt;
    }

    if(NUMBERS.find(str[2]) == NUMBERS.end()) {
        r = dt.triefind(str, &WW, &MM, &DD, &YYYY, &HH, &mi, &SS, &sss, &ZZ);
        if(r[3] && r[1] && r[2]) {
            if(dt(r[3], r[1], r[2], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

        r = dt.triefind(str, &WW, &DD, &MM, &YYYY, &HH, &mi, &SS, &sss, &ZZ);
        if(r[3] && r[2] && r[1]) {
            if(dt(r[3], r[2], r[1], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }
    }

    if(dayfirst == false) {
        r = dt.triefind(str, &YYYY, &MM, &DD, &WW, &HH, &mi, &SS, &sss, &ZZ);
        if(r[0] && r[1] && r[2]) {
            if(dt(r[0], r[1], r[2], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

        r = dt.triefind(str, &YYYY, &DD, &MM, &WW, &HH, &mi, &SS, &sss, &ZZ);
        if(r[0] && r[2] && r[1]) {
            if(dt(r[0], r[2], r[1], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

    } else {
        r = dt.triefind(str, &YYYY, &DD, &MM, &WW, &HH, &mi, &SS, &sss, &ZZ);
        if(r[0] && r[2] && r[1]) {
            if(dt(r[0], r[2], r[1], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }

        r = dt.triefind(str, &YYYY, &MM, &DD, &WW, &HH, &mi, &SS, &sss, &ZZ);
        if(r[0] && r[1] && r[2]) {
            if(dt(r[0], r[1], r[2], r[4], r[5], r[6], r[7], r[8]))
                return dt;
        }
    }

    r = dt.triefind(str, &DD, &MM, &yy, &WW, &HH, &mi, &SS, &sss, &ZZ);
    if(r[0] && r[1] && r[2]) {
        r[2] += r[2] < 60 ? 2000 : 1900;
        if(dt(r[2], r[1], r[0], r[4], r[5], r[6], r[7], r[8]))
            return dt;
    }

    r = dt.triefind(str, &MM, &DD, &WW, &HH, &mi, &SS, &sss, nullptr, &ZZ);
    if(r[0] && r[1]) {
        if(dt(1970, r[0], r[1], r[3], r[4], r[5], r[6], r[8]))
            return dt;
    }

    r = dt.triefind(str, &GG, &yy, &MM, &DD, &WW, &HH, &mi, &SS, nullptr);
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

#endif
