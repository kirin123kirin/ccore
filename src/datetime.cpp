#include <Python.h>
#include <datetime.h>
#include <setjmp.h>
#include <array>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <ctime>

#if defined(_WIN32) || defined(_WIN64)
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

    bool operator==(const PyMallocator<T>&) {
        return true;
    }

    bool operator!=(const PyMallocator<T>&) {
        return false;
    }
};

using py_ustring = std::basic_string<wchar_t, std::char_traits<wchar_t>, PyMallocator<wchar_t>>;

template <typename T>
struct nohash {
    constexpr T operator()(const T& s) const noexcept {
        return s;
    }
};

static const std::unordered_map<wchar_t, int, nohash<wchar_t> > TRAN = {
    {L'0', int(0)},    {L'1', int(1)},    {L'2', int(2)},    {L'3', int(3)},
    {L'4', int(4)},    {L'5', int(5)},    {L'6', int(6)},    {L'7', int(7)},
    {L'8', int(8)},    {L'9', int(9)},    {L'０', int(0)},   {L'１', int(1)},
    {L'２', int(2)},   {L'３', int(3)},   {L'４', int(4)},   {L'５', int(5)},
    {L'６', int(6)},   {L'７', int(7)},   {L'８', int(8)},   {L'９', int(9)},
    {L'.', int(10)},   {L'．', int(10)},  {L'〇', int(0)},   {L'一', int(1)},
    {L'二', int(3)},   {L'三', int(3)},   {L'四', int(4)},   {L'五', int(5)},
    {L'六', int(6)},   {L'七', int(7)},   {L'八', int(8)},   {L'九', int(9)},
    {L'十', int(11)},  {L'_', int(12)},   {L'＿', int(12)},  {L'-', int(13)},
    {L'－', int(13)},  {L',', int(14)},   {L'，', int(14)},  {L'/', int(15)},
    {L'／', int(15)},  {L'年', int(16)},  {L'元', int(17)},  {L'\'', int(18)},
    {L'＇', int(18)},  {L'T', int(19)},   {L'Ｔ', int(19)},  {L'M', int(20)},
    {L'Ｍ', int(20)},  {L'S', int(21)},   {L'Ｓ', int(21)},  {L'H', int(22)},
    {L'Ｈ', int(22)},  {L'R', int(23)},   {L'Ｒ', int(23)},  {L'A', int(24)},
    {L'Ａ', int(24)},  {L'N', int(25)},   {L'Ｎ', int(25)},  {L'D', int(26)},
    {L'Ｄ', int(26)},  {L':', int(27)},   {L'：', int(27)},  {L'O', int(28)},
    {L'Ｏ', int(28)},  {L'p', int(29)},   {L'ｐ', int(29)},  {L'正', int(30)},
    {L'午', int(31)},  {L'後', int(32)},  {L'時', int(33)},  {L'前', int(34)},
    {L'秒', int(35)},  {L'分', int(36)},  {L'日', int(37)},  {L'月', int(38)},
    {L'曜', int(39)},  {L'火', int(40)},  {L'水', int(41)},  {L'木', int(42)},
    {L'金', int(43)},  {L'土', int(44)},  {L'(', int(45)},   {L'（', int(45)},
    {L')', int(46)},   {L'）', int(46)},  {L'e', int(47)},   {L'ｅ', int(47)},
    {L'F', int(48)},   {L'Ｆ', int(48)},  {L'i', int(49)},   {L'ｉ', int(49)},
    {L'u', int(50)},   {L'ｕ', int(50)},  {L'y', int(51)},   {L'ｙ', int(51)},
    {L'W', int(52)},   {L'Ｗ', int(52)},  {L'b', int(53)},   {L'ｂ', int(53)},
    {L'c', int(54)},   {L'ｃ', int(54)},  {L'g', int(55)},   {L'ｇ', int(55)},
    {L'J', int(56)},   {L'Ｊ', int(56)},  {L'l', int(57)},   {L'ｌ', int(57)},
    {L'v', int(58)},   {L'ｖ', int(58)},  {L'K', int(59)},   {L'Ｋ', int(59)},
    {L'X', int(60)},   {L'Ｘ', int(60)},  {L'Z', int(61)},   {L'Ｚ', int(61)},
    {L'+', int(62)},   {L'＋', int(62)},  {L'安', int(63)},  {L'雲', int(64)},
    {L'永', int(65)},  {L'延', int(66)},  {L'応', int(67)},  {L'化', int(68)},
    {L'嘉', int(69)},  {L'乾', int(70)},  {L'寛', int(71)},  {L'感', int(72)},
    {L'観', int(73)},  {L'喜', int(74)},  {L'亀', int(75)},  {L'吉', int(76)},
    {L'久', int(77)},  {L'亨', int(78)},  {L'享', int(79)},  {L'興', int(80)},
    {L'慶', int(81)},  {L'景', int(82)},  {L'建', int(83)},  {L'護', int(84)},
    {L'康', int(85)},  {L'弘', int(86)},  {L'衡', int(87)},  {L'国', int(88)},
    {L'至', int(89)},  {L'字', int(90)},  {L'治', int(91)},  {L'朱', int(92)},
    {L'寿', int(93)},  {L'授', int(94)},  {L'勝', int(95)},  {L'承', int(96)},
    {L'昌', int(97)},  {L'昭', int(98)},  {L'祥', int(99)},  {L'神', int(100)},
    {L'仁', int(101)}, {L'成', int(102)}, {L'政', int(103)}, {L'斉', int(104)},
    {L'泰', int(105)}, {L'大', int(106)}, {L'中', int(107)}, {L'長', int(108)},
    {L'鳥', int(109)}, {L'貞', int(110)}, {L'禎', int(111)}, {L'天', int(112)},
    {L'同', int(113)}, {L'銅', int(114)}, {L'徳', int(115)}, {L'白', int(116)},
    {L'武', int(117)}, {L'福', int(118)}, {L'文', int(119)}, {L'平', int(120)},
    {L'保', int(121)}, {L'宝', int(122)}, {L'万', int(123)}, {L'明', int(124)},
    {L'養', int(125)}, {L'令', int(126)}, {L'霊', int(127)}, {L'暦', int(128)},
    {L'老', int(129)}, {L'禄', int(130)}, {L'和', int(131)}, {L'祚', int(132)},
    {L'雉', int(133)}, {L't', int(19)},   {L'ｔ', int(19)},  {L'm', int(20)},
    {L'ｍ', int(20)},  {L's', int(21)},   {L'ｓ', int(21)},  {L'h', int(22)},
    {L'ｈ', int(22)},  {L'r', int(23)},   {L'ｒ', int(23)},  {L'a', int(24)},
    {L'ａ', int(24)},  {L'n', int(25)},   {L'ｎ', int(25)},  {L'd', int(26)},
    {L'ｄ', int(26)},  {L'o', int(28)},   {L'ｏ', int(28)},  {L'P', int(29)},
    {L'Ｐ', int(29)},  {L'E', int(47)},   {L'Ｅ', int(47)},  {L'f', int(48)},
    {L'ｆ', int(48)},  {L'I', int(49)},   {L'Ｉ', int(49)},  {L'U', int(50)},
    {L'Ｕ', int(50)},  {L'Y', int(51)},   {L'Ｙ', int(51)},  {L'w', int(52)},
    {L'ｗ', int(52)},  {L'B', int(53)},   {L'Ｂ', int(53)},  {L'C', int(54)},
    {L'Ｃ', int(54)},  {L'G', int(55)},   {L'Ｇ', int(55)},  {L'j', int(56)},
    {L'ｊ', int(56)},  {L'L', int(57)},   {L'Ｌ', int(57)},  {L'V', int(58)},
    {L'Ｖ', int(58)},  {L'k', int(59)},   {L'ｋ', int(59)},  {L'x', int(60)},
    {L'ｘ', int(60)},  {L'z', int(61)},   {L'ｚ', int(61)}};

static const std::unordered_set<wchar_t, nohash<wchar_t> > NUMBERS = {
    L'0',  L'1',  L'2',  L'3',  L'4',  L'5',  L'6',  L'7',  L'8',  L'9',  L'０', L'１', L'２', L'３', L'４', L'５',
    L'６', L'７', L'８', L'９', L'〇', L'一', L'二', L'三', L'四', L'五', L'六', L'七', L'八', L'九', L'十'};

static std::unordered_set<wchar_t, nohash<wchar_t> > VALIDATOR = {
    L' ',  L'　', L'/', L'-', L'+', L':', L'.', L',', L'0',  L'1',  L'2',  L'3',  L'4',  L'5',  L'6',  L'7',  L'8',  L'9',  L'０',
    L'１', L'２', L'３', L'４', L'５', L'６', L'７', L'８', L'９', L'〇', L'一', L'二', L'三',
    L'四', L'五', L'六', L'七', L'八', L'九', L'十', L'年', L'月', L'日', L'時', L'分', L'秒',
    L'A',  L'D',  L'F',  L'J',  L'M',  L'N',  L'O',  L'S',  L'T',  L'W',  L'S'};

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

        if (fread(&len, sizeof(len), 1, fp) < 1)
            return (uint64_t)-1;
        nodes.resize(len + 1);

        if (fread(&(nodes.data()[0]), sizeof(TrieNode), len, fp) < len)
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
                if(s != 0x0030 && s == ((value / 10) + 0x0030)) {
                    if (value >= 20)
                        kansuji_j += kj;
                    if(kansuji_j.back() != L'十')
                        kansuji_j += L'十';
                    else
                        kansuji_j += kj;
                } else if(s != 0x0030) {
                    kansuji_j += kj;
                }


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

    static const std::vector<std::pair<std::wstring, int> > gengo = {
        {L"令和", int(2019)},    {L"R.", int(2019)},      {L"R", int(2019)},
        {L"令", int(2019)},      {L"平成", int(1989)},    {L"H.", int(1989)},
        {L"H", int(1989)},       {L"平", int(1989)},      {L"昭和", int(1926)},
        {L"S.", int(1926)},      {L"S", int(1926)},       {L"昭", int(1926)},
        {L"大正", int(1912)},    {L"T.", int(1912)},      {L"T", int(1912)},
        {L"大", int(1912)},      {L"明治", int(1868)},    {L"M.", int(1868)},
        {L"M", int(1868)},       {L"明", int(1868)},      {L"慶応", int(1865)},
        {L"元治", int(1864)},    {L"文久", int(1861)},    {L"万延", int(1860)},
        {L"安政", int(1855)},    {L"嘉永", int(1848)},    {L"弘化", int(1845)},
        {L"天保", int(1831)},    {L"文政", int(1818)},    {L"文化", int(1804)},
        {L"享和", int(1801)},    {L"寛政", int(1789)},    {L"天明", int(1781)},
        {L"安永", int(1772)},    {L"明和", int(1764)},    {L"宝暦", int(1751)},
        {L"寛延", int(1748)},    {L"延享", int(1744)},    {L"寛保", int(1741)},
        {L"元文", int(1736)},    {L"享保", int(1716)},    {L"正徳", int(1711)},
        {L"宝永", int(1704)},    {L"元禄", int(1688)},    {L"貞享", int(1684)},
        {L"天和", int(1681)},    {L"延宝", int(1673)},    {L"寛文", int(1661)},
        {L"万治", int(1658)},    {L"明暦", int(1655)},    {L"承応", int(1652)},
        {L"慶安", int(1648)},    {L"正保", int(1645)},    {L"寛永", int(1624)},
        {L"元和", int(1615)},    {L"慶長", int(1596)},    {L"文禄", int(1593)},
        {L"天正", int(1573)},    {L"元亀", int(1570)},    {L"永禄", int(1558)},
        {L"弘治", int(1555)},    {L"天文", int(1532)},    {L"享禄", int(1528)},
        {L"大永", int(1521)},    {L"永正", int(1504)},    {L"文亀", int(1501)},
        {L"明応", int(1492)},    {L"延徳", int(1489)},    {L"長享", int(1487)},
        {L"文明", int(1469)},    {L"応仁", int(1467)},    {L"文正", int(1466)},
        {L"寛正", int(1461)},    {L"長禄", int(1457)},    {L"康正", int(1455)},
        {L"享徳", int(1452)},    {L"宝徳", int(1449)},    {L"文安", int(1444)},
        {L"嘉吉", int(1441)},    {L"永享", int(1429)},    {L"正長", int(1428)},
        {L"応永", int(1394)},    {L"明徳", int(1390)},    {L"康応", int(1389)},
        {L"嘉慶", int(1387)},    {L"至徳", int(1384)},    {L"永徳", int(1381)},
        {L"康暦", int(1379)},    {L"永和", int(1375)},    {L"応安", int(1368)},
        {L"貞治", int(1362)},    {L"康安", int(1361)},    {L"延文", int(1356)},
        {L"文和", int(1352)},    {L"観応", int(1350)},    {L"貞和", int(1345)},
        {L"康永", int(1342)},    {L"暦応", int(1338)},    {L"元中", int(1384)},
        {L"弘和", int(1381)},    {L"天授", int(1375)},    {L"文中", int(1372)},
        {L"建徳", int(1370)},    {L"正平", int(1347)},    {L"興国", int(1340)},
        {L"延元", int(1336)},    {L"建武", int(1334)},    {L"正慶", int(1332)},
        {L"元弘", int(1331)},    {L"元徳", int(1329)},    {L"嘉暦", int(1326)},
        {L"正中", int(1324)},    {L"元亨", int(1321)},    {L"元応", int(1319)},
        {L"文保", int(1317)},    {L"正和", int(1312)},    {L"応長", int(1311)},
        {L"延慶", int(1308)},    {L"徳治", int(1307)},    {L"嘉元", int(1303)},
        {L"乾元", int(1302)},    {L"正安", int(1299)},    {L"永仁", int(1293)},
        {L"正応", int(1288)},    {L"弘安", int(1278)},    {L"建治", int(1275)},
        {L"文永", int(1264)},    {L"弘長", int(1261)},    {L"文応", int(1260)},
        {L"正元", int(1259)},    {L"正嘉", int(1257)},    {L"康元", int(1256)},
        {L"建長", int(1249)},    {L"宝治", int(1247)},    {L"寛元", int(1243)},
        {L"仁治", int(1240)},    {L"延応", int(1239)},    {L"暦仁", int(1238)},
        {L"嘉禎", int(1235)},    {L"文暦", int(1234)},    {L"天福", int(1233)},
        {L"貞永", int(1232)},    {L"寛喜", int(1229)},    {L"安貞", int(1228)},
        {L"嘉禄", int(1225)},    {L"元仁", int(1224)},    {L"貞応", int(1222)},
        {L"承久", int(1219)},    {L"建保", int(1214)},    {L"建暦", int(1211)},
        {L"承元", int(1207)},    {L"建永", int(1206)},    {L"元久", int(1204)},
        {L"建仁", int(1201)},    {L"正治", int(1199)},    {L"建久", int(1190)},
        {L"文治", int(1185)},    {L"元暦", int(1184)},    {L"寿永", int(1182)},
        {L"養和", int(1181)},    {L"治承", int(1177)},    {L"安元", int(1175)},
        {L"承安", int(1171)},    {L"嘉応", int(1169)},    {L"仁安", int(1166)},
        {L"永万", int(1165)},    {L"長寛", int(1163)},    {L"応保", int(1161)},
        {L"永暦", int(1160)},    {L"平治", int(1159)},    {L"保元", int(1156)},
        {L"久寿", int(1154)},    {L"仁平", int(1151)},    {L"久安", int(1145)},
        {L"天養", int(1144)},    {L"康治", int(1142)},    {L"永治", int(1141)},
        {L"保延", int(1135)},    {L"長承", int(1132)},    {L"天承", int(1131)},
        {L"大治", int(1126)},    {L"天治", int(1124)},    {L"保安", int(1120)},
        {L"元永", int(1118)},    {L"永久", int(1113)},    {L"天永", int(1110)},
        {L"天仁", int(1108)},    {L"嘉承", int(1106)},    {L"長治", int(1104)},
        {L"康和", int(1099)},    {L"承徳", int(1097)},    {L"永長", int(1097)},
        {L"嘉保", int(1095)},    {L"寛治", int(1087)},    {L"応徳", int(1084)},
        {L"永保", int(1081)},    {L"承暦", int(1077)},    {L"承保", int(1074)},
        {L"延久", int(1069)},    {L"治暦", int(1065)},    {L"康平", int(1058)},
        {L"天喜", int(1053)},    {L"永承", int(1046)},    {L"寛徳", int(1044)},
        {L"長久", int(1040)},    {L"長暦", int(1037)},    {L"長元", int(1028)},
        {L"万寿", int(1024)},    {L"治安", int(1021)},    {L"寛仁", int(1017)},
        {L"長和", int(1013)},    {L"寛弘", int(1004)},    {L"長保", int(999)},
        {L"長徳", int(995)},     {L"正暦", int(990)},     {L"永祚", int(989)},
        {L"永延", int(987)},     {L"寛和", int(985)},     {L"永観", int(983)},
        {L"天元", int(978)},     {L"貞元", int(976)},     {L"天延", int(974)},
        {L"天禄", int(970)},     {L"安和", int(968)},     {L"康保", int(964)},
        {L"応和", int(961)},     {L"天徳", int(957)},     {L"天暦", int(947)},
        {L"天慶", int(938)},     {L"承平", int(931)},     {L"延長", int(923)},
        {L"延喜", int(901)},     {L"昌泰", int(898)},     {L"寛平", int(889)},
        {L"仁和", int(885)},     {L"元慶", int(877)},     {L"貞観", int(859)},
        {L"天安", int(857)},     {L"斉衡", int(854)},     {L"仁寿", int(851)},
        {L"嘉祥", int(848)},     {L"承和", int(834)},     {L"天長", int(824)},
        {L"弘仁", int(810)},     {L"大同", int(806)},     {L"延暦", int(782)},
        {L"天応", int(781)},     {L"宝亀", int(770)},     {L"神護景雲", int(767)},
        {L"天平神護", int(765)}, {L"天平宝字", int(757)}, {L"天平勝宝", int(749)},
        {L"天平感宝", int(749)}, {L"天平", int(729)},     {L"神亀", int(724)},
        {L"養老", int(717)},     {L"霊亀", int(715)},     {L"和銅", int(708)},
        {L"慶雲", int(704)},     {L"大宝", int(701)},     {L"朱鳥", int(686)},
        {L"白雉", int(650)},     {L"大化", int(645)},
    };

    static wchar_t ymdsep[] = {0, L'/', L'-', L'_', L'.', L','};
    static wchar_t hmssep[] = {0, L':', L'_', L'.'};
    static const std::wstring half[] = {L"am", L"pm",  L"a.m", L"p.m",  L"a.m.", L"p.m.", L"AM",
                          L"PM", L"A.M", L"P.M", L"A.M.", L"P.M.", L"午前", L"午後"};
    static const std::vector<std::pair<std::wstring, int> > tzone = {
        {L"ACDT", int(37800)},      {L"ACST", int(34200)},      {L"ACT", int(-18000)},
        {L"ACWST", int(31500)},     {L"ADT", int(-10800)},      {L"AEDT", int(39600)},
        {L"AEST", int(36000)},      {L"AFT", int(16200)},       {L"AKDT", int(-28800)},
        {L"AKST", int(-32400)},     {L"AMST", int(-10800)},     {L"AMT", int(-14400)},
        {L"AMT", int(14400)},       {L"ART", int(-10800)},      {L"AST", int(-14400)},
        {L"AST", int(10800)},       {L"AT", int(-14400)},       {L"AWST", int(28800)},
        {L"AZOST", int(0)},         {L"AZOT", int(-3600)},      {L"AZT", int(14400)},
        {L"BDT", int(28800)},       {L"BNT", int(28800)},       {L"BOT", int(-14400)},
        {L"BRST", int(-7200)},      {L"BRT", int(-10800)},      {L"BST", int(21600)},
        {L"BST", int(3600)},        {L"BTT", int(21600)},       {L"CAT", int(7200)},
        {L"CCT", int(23400)},       {L"CDT", int(-14400)},      {L"CDT", int(-18000)},
        {L"CEST", int(7200)},       {L"CET", int(3600)},        {L"CHADT", int(49500)},
        {L"CHAST", int(45900)},     {L"CHOST", int(32400)},     {L"CHOT", int(28800)},
        {L"CHST", int(36000)},      {L"CHUT", int(36000)},      {L"CIT", int(28800)},
        {L"CKT", int(-36000)},      {L"CLST", int(-10800)},     {L"CLT", int(-14400)},
        {L"COST", int(-14400)},     {L"COT", int(-18000)},      {L"CST", int(-18000)},
        {L"CST", int(-21600)},      {L"CST", int(28800)},       {L"CT", int(-21600)},
        {L"CVT", int(-3600)},       {L"CXT", int(25200)},       {L"DAVT", int(25200)},
        {L"DDUT", int(36000)},      {L"EASST", int(-18000)},    {L"EAST", int(-21600)},
        {L"EAT", int(10800)},       {L"ECT", int(-18000)},      {L"EDT", int(-14400)},
        {L"EEST", int(10800)},      {L"EET", int(7200)},        {L"EGST", int(0)},
        {L"EGT", int(-3600)},       {L"EIT", int(32400)},       {L"EST", int(-18000)},
        {L"ET", int(-18000)},       {L"FET", int(10800)},       {L"FJT", int(43200)},
        {L"FKST", int(-10800)},     {L"FKT", int(-14400)},      {L"FNT", int(-7200)},
        {L"GALT", int(-21600)},     {L"GAMT", int(-32400)},     {L"GET", int(14400)},
        {L"GFT", int(-10800)},      {L"GILT", int(43200)},      {L"GIT", int(-32400)},
        {L"GMT", int(0)},           {L"GMT+0", int(0)},         {L"GMT+1", int(3600)},
        {L"GMT+10", int(36000)},    {L"GMT+10:30", int(37800)}, {L"GMT+11", int(39600)},
        {L"GMT+12", int(43200)},    {L"GMT+12:45", int(45900)}, {L"GMT+13", int(46800)},
        {L"GMT+13:45", int(49500)}, {L"GMT+14", int(50400)},    {L"GMT+2", int(7200)},
        {L"GMT+3", int(10800)},     {L"GMT+3:30", int(12600)},  {L"GMT+4", int(14400)},
        {L"GMT+4:30", int(16200)},  {L"GMT+5", int(18000)},     {L"GMT+5:30", int(19800)},
        {L"GMT+5:45", int(20700)},  {L"GMT+6", int(21600)},     {L"GMT+6:30", int(23400)},
        {L"GMT+7", int(25200)},     {L"GMT+8", int(28800)},     {L"GMT+8:45", int(31500)},
        {L"GMT+9", int(32400)},     {L"GMT+9:30", int(34200)},  {L"GMT-1", int(-3600)},
        {L"GMT-10", int(-36000)},   {L"GMT-11", int(-39600)},   {L"GMT-2", int(-7200)},
        {L"GMT-2:30", int(-9000)},  {L"GMT-3", int(-10800)},    {L"GMT-3:30", int(-12600)},
        {L"GMT-4", int(-14400)},    {L"GMT-5", int(-18000)},    {L"GMT-6", int(-21600)},
        {L"GMT-7", int(-25200)},    {L"GMT-8", int(-28800)},    {L"GMT-9", int(-32400)},
        {L"GMT-9:30", int(-34200)}, {L"GST", int(-7200)},       {L"GST", int(14400)},
        {L"GYT", int(-14400)},      {L"HADT", int(-32400)},     {L"HAST", int(-36000)},
        {L"HKT", int(28800)},       {L"HOVST", int(28800)},     {L"HOVT", int(25200)},
        {L"ICT", int(25200)},       {L"IDT", int(10800)},       {L"IRDT", int(16200)},
        {L"IRKT", int(28800)},      {L"IRST", int(12600)},      {L"IST", int(19800)},
        {L"IST", int(3600)},        {L"IST", int(7200)},        {L"JST", int(32400)},
        {L"KGT", int(21600)},       {L"KOST", int(39600)},      {L"KRAT", int(25200)},
        {L"KST", int(32400)},       {L"LHDT", int(39600)},      {L"LHST", int(37800)},
        {L"LINT", int(50400)},      {L"MAGT", int(39600)},      {L"MART", int(-34200)},
        {L"MAWT", int(18000)},      {L"MDT", int(-21600)},      {L"MHT", int(43200)},
        {L"MIST", int(39600)},      {L"MIT", int(-34200)},      {L"MMT", int(23400)},
        {L"MSK", int(10800)},       {L"MST", int(-25200)},      {L"MST", int(28800)},
        {L"MT", int(-25200)},       {L"MUT", int(14400)},       {L"MVT", int(18000)},
        {L"MYT", int(28800)},       {L"NCT", int(39600)},       {L"NDT", int(-9000)},
        {L"NFT", int(39600)},       {L"NPT", int(20700)},       {L"NRT", int(43200)},
        {L"NST", int(-12600)},      {L"NT", int(-12600)},       {L"NUT", int(-39600)},
        {L"NZDT", int(46800)},      {L"NZST", int(43200)},      {L"OMST", int(21600)},
        {L"ORAT", int(18000)},      {L"PDT", int(-25200)},      {L"PET", int(-18000)},
        {L"PETT", int(43200)},      {L"PGT", int(36000)},       {L"PHOT", int(46800)},
        {L"PHT", int(28800)},       {L"PKT", int(18000)},       {L"PMDT", int(-7200)},
        {L"PMST", int(-10800)},     {L"PONT", int(39600)},      {L"PST", int(-28800)},
        {L"PST", int(28800)},       {L"PT", int(-28800)},       {L"PWT", int(32400)},
        {L"PYST", int(-10800)},     {L"PYT", int(-14400)},      {L"RET", int(14400)},
        {L"ROTT", int(-10800)},     {L"SAKT", int(39600)},      {L"SAMT", int(14400)},
        {L"SAST", int(7200)},       {L"SBT", int(39600)},       {L"SCT", int(14400)},
        {L"SGT", int(28800)},       {L"SLST", int(19800)},      {L"SRT", int(-10800)},
        {L"SST", int(-39600)},      {L"SYOT", int(10800)},      {L"TAHT", int(-36000)},
        {L"TFT", int(18000)},       {L"THA", int(25200)},       {L"TJT", int(18000)},
        {L"TKT", int(46800)},       {L"TLT", int(32400)},       {L"TMT", int(18000)},
        {L"TOT", int(46800)},       {L"TRT", int(10800)},       {L"TVT", int(43200)},
        {L"ULAST", int(32400)},     {L"ULAT", int(28800)},      {L"USZ1", int(7200)},
        {L"UTC", int(0)},           {L"UTC+0", int(0)},         {L"UTC+1", int(3600)},
        {L"UTC+10", int(36000)},    {L"UTC+10:30", int(37800)}, {L"UTC+11", int(39600)},
        {L"UTC+12", int(43200)},    {L"UTC+12:45", int(45900)}, {L"UTC+13", int(46800)},
        {L"UTC+13:45", int(49500)}, {L"UTC+14", int(50400)},    {L"UTC+2", int(7200)},
        {L"UTC+3", int(10800)},     {L"UTC+3:30", int(12600)},  {L"UTC+4", int(14400)},
        {L"UTC+4:30", int(16200)},  {L"UTC+5", int(18000)},     {L"UTC+5:30", int(19800)},
        {L"UTC+5:45", int(20700)},  {L"UTC+6", int(21600)},     {L"UTC+6:30", int(23400)},
        {L"UTC+7", int(25200)},     {L"UTC+8", int(28800)},     {L"UTC+8:45", int(31500)},
        {L"UTC+9", int(32400)},     {L"UTC+9:30", int(34200)},  {L"UTC-1", int(-3600)},
        {L"UTC-10", int(-36000)},   {L"UTC-11", int(-39600)},   {L"UTC-2", int(-7200)},
        {L"UTC-2:30", int(-5400)},  {L"UTC-3", int(-10800)},    {L"UTC-3:30", int(-9000)},
        {L"UTC-4", int(-14400)},    {L"UTC-5", int(-18000)},    {L"UTC-6", int(-21600)},
        {L"UTC-7", int(-25200)},    {L"UTC-8", int(-28800)},    {L"UTC-9", int(-32400)},
        {L"UTC-9:30", int(-30600)}, {L"UYST", int(-7200)},      {L"UYT", int(-10800)},
        {L"UZT", int(18000)},       {L"VET", int(-14400)},      {L"VLAT", int(36000)},
        {L"VOLT", int(14400)},      {L"VOST", int(21600)},      {L"VUT", int(39600)},
        {L"WAKT", int(43200)},      {L"WAST", int(7200)},       {L"WAT", int(3600)},
        {L"WEST", int(3600)},       {L"WET", int(0)},           {L"WFT", int(43200)},
        {L"WGST", int(-10800)},     {L"WGST", int(-7200)},      {L"WIB", int(25200)},
        {L"WIT", int(32400)},       {L"YAKT", int(32400)},      {L"YEKT", int(18000)},
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
        insert(yy, L'\'' + st, v);
        if(v < 10) {
            std::wstring zfill = L'0' + st;
            insert(yy, zfill, v);
            insert(yy, L'\'' + zfill, v);
        }
        for(auto it = std::begin(ymdsep); it != std::end(ymdsep); ++it) {
            wchar_t sp = *it;
            insert(yy, st + sp, v);
            insert(yy, sp + st, v);
            if(v < 10) {
                std::wstring zfill = L'0' + st;
                insert(yy, zfill + sp, v);
                insert(yy, sp + zfill, v);
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
            if (fread(&s, sz, 1, fp) < 1)
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

        if(0 <= year && year <= 68) {
            std::time_t rawtime = std::mktime(&timeinfo);
#if defined(_WIN32) || defined(_WIN64)
            if(localtime_s(&timeinfo, &rawtime))
#else
            if(localtime_r(&rawtime, &timeinfo))
#endif
                return false;
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
    bool operator!=(datetime& other) {
        return !operator==(other);
    }
    bool operator!=(std::nullptr_t) {
        return !operator==(nullptr);
    }

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
            if(i - j < 3){
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

    for(auto&& s = str.cbegin(), end = str.cend() + 1; s != end; ++s, ++j) {
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
                if (k == (std::size_t)-1)
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

    for(auto&& s = str.begin(), end = str.end() + 1; s != end; ++s, ++j) {
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

    for(auto&& s = str.cbegin(), end = str.cend() + 1; s != end; ++s, ++j) {
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
    else if (!PyUnicode_Check(o))
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");
    Py_ssize_t len;
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)
        return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");
    
    res = to_datetime(str, (bool)dayfirst, minlimit);
    PyMem_Free(str);
    
    if(res == nullptr)
        Py_RETURN_NONE;
    else if (res.offset == -1)
        return PyDateTime_FromDateAndTime(res.year + 1900, res.month + 1, res.day, res.hour, res.min, res.sec, res.microsec);
    
    PyDateTime_DateTime* dt = (PyDateTime_DateTime*)PyDateTime_FromDateAndTime(res.year + 1900, res.month + 1, res.day, res.hour, res.min, res.sec, res.microsec);

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
    PyObject* o, *res;
    wchar_t* str;

    int dayfirst = false;
    uint64_t minlimit = uint64_t(3);
    const char* kwlist[5] = {"o", "dayfirst", "minlimit", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ii", (char**)kwlist, &o, &dayfirst, &minlimit))
        return NULL;

    if (!PyUnicode_Check(o))
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");
    Py_ssize_t len;
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)
        return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");
    
    res = extractdate(str, (bool)dayfirst, minlimit);
    PyMem_Free(str);
    if (res)
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

    if (!PyUnicode_Check(o))
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
    if (fmt)
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

