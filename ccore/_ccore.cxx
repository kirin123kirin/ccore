/* _ccore.cxx | MIT License | https://github.com/kirin123kirin/ccore/raw/main/LICENSE */
#include <Python.h>
#include <datetime.h>
// #include <string>
// #include <utility>
#include "ccore.hpp"

extern "C" PyObject* binopen_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;
    char* mode = (char*)"rb";
    int buffering = -1;
    PyObject* encoding = Py_None;
    PyObject* errors = Py_None;
    PyObject* newline = Py_None;
    PyObject* closefd = Py_True;
    PyObject* _opener = Py_None;

    const char* kwlist[9] = {"o", "mode", "buffering", "errors", "closefd", "opener", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|siOOO", (char**)kwlist, &o, &mode, &buffering, &errors, &closefd,
                                    &_opener))
        return NULL;

    return binopen(o, mode, buffering, encoding, errors, newline, closefd, _opener);
}

extern "C" PyObject* opener_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;
    char* mode = (char*)"r";
    int buffering = -1;
    char* encoding = NULL;
    PyObject* errors = Py_None;
    PyObject* newline = Py_None;
    PyObject* closefd = Py_True;
    PyObject* _opener = Py_None;

    const char* kwlist[9] = {"o", "mode", "buffering", "encoding", "errors", "newline", "closefd", "opener", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sisOOOO", (char**)kwlist, &o, &mode, &buffering, &encoding,
                                    &errors, &newline, &closefd, &_opener))
        return NULL;

    return opener(o, mode, buffering, encoding, errors, newline, closefd, _opener);
}

extern "C" PyObject* headtail_py(PyObject* self, PyObject* args) {
    PyObject *o, *ret;
    Py_ssize_t bufsize;

    if(!PyArg_ParseTuple(args, "On", &o, &bufsize))
        return NULL;

    if(bufsize < 0)
        return PyErr_Format(PyExc_ValueError, "`bufsize` argument : Negative values are invalid.");

    if((ret = headtail(o, bufsize)) != NULL)
        return ret;
    Py_RETURN_NONE;
}

extern "C" PyObject* guesstype_py(PyObject* self, PyObject* args) {
    PyObject* o;
    Py_ssize_t bufsize = 2048;

    if(!PyArg_ParseTuple(args, "O|n", &o, &bufsize))
        return NULL;

    if(bufsize < 0)
        return PyErr_Format(PyExc_ValueError, "`bufsize` argument : Negative values are invalid.");

    const char* ret = guesstype(o, bufsize);
    if(PyErr_Occurred())
        return NULL;
    if(ret == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromString(ret);
}

const char* _getsniffargs(PyObject* o, const char* default_value, std::size_t default_value_len, Py_ssize_t* len) {
    const char* carg = NULL;
    if(o == NULL) {
        carg = default_value;
        *len = default_value_len;
    } else if(PyUnicode_Check(o)) {
        carg = PyUnicode_AsUTF8AndSize(o, len);
    } else if(PyBytes_Check(o)) {
        carg = PyBytes_AsString(o);
        *len = PyBytes_GET_SIZE(o);
    } else {
        PyObject *res = NULL, *sep1 = NULL, *tmp = NULL;
        if(!PySequence_Check(o)) {
            if((tmp = PySequence_List(o)) == NULL)
                return NULL;
        }
        if(!PySequence_Check(tmp ? tmp : o))
            return NULL;
        if((sep1 = PyUnicode_FromString("")) == NULL)
            return NULL;
        res = PyUnicode_Join(sep1, tmp ? tmp : o);
        Py_DECREF(sep1);
        Py_XDECREF(tmp);
        if(res) {
            carg = PyUnicode_AsUTF8AndSize(res, len);
            Py_DECREF(res);
        } else {
            PyObject* sep2 = PyBytes_FromString("");
            res = _PyBytes_Join(sep2, o);
            Py_DECREF(sep2);
            if(res) {
                carg = PyBytes_AsString(res);
                *len = PyBytes_GET_SIZE(res);
                Py_DECREF(res);
            } else {
                return NULL;
            }
        }
    }
    if(carg == NULL || *len == -1) {
        PyErr_Format(PyExc_TypeError, "Unknown type of Option arguments. Please sequencial object.");
        carg = NULL;
        *len = -1;
    }
    return carg;
}

extern "C" PyObject* sniffer_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject *o, *DL = NULL, *QU = NULL, *ES = NULL, *LN = NULL;
    int maxlines = 100, with_encoding = false;
    const char *delimiters, *quotechars, *escapechars, *lineterminators;
    Py_ssize_t len, len_delims, len_quotes, len_escapes, len_lineterms;
    const char* dat = NULL;

    const char* kwlist[] = {"o",          "maxlines",    "with_encoding",   "delimiters",
                            "quotechars", "escapechars", "lineterminators", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iiOOOO", (char**)kwlist, &o, &maxlines, &with_encoding, &DL, &QU,
                                    &ES, &LN))
        return NULL;
    
    if(o == Py_None)
        return PyErr_Format(PyExc_TypeError, "Nonetype is not iteratable.");

    if((delimiters = _getsniffargs(DL, ",\t;:| ", 6, &len_delims)) == NULL)
        return NULL;

    if((quotechars = _getsniffargs(QU, "\"'`", 3, &len_quotes)) == NULL)
        return NULL;

    if((escapechars = _getsniffargs(ES, R"(\\^)", 2, &len_escapes)) == NULL)
        return NULL;

    if((lineterminators = _getsniffargs(LN, "\r\n", 2, &len_lineterms)) == NULL)
        return NULL;

    if(PyUnicode_Check(o) && (dat = PyUnicode_AsUTF8AndSize(o, &len)) == NULL)
        return NULL;

    if(PyBytes_Check(o) && ((dat = PyBytes_AsString(o)) == NULL || (len = PyBytes_GET_SIZE(o)) == -1))
        return NULL;

    if(PyByteArray_Check(o) && ((dat = PyByteArray_AsString(o)) == NULL || (len = PyByteArray_GET_SIZE(o)) == -1))
        return NULL;
    
    return sniffer(dat, len, maxlines, with_encoding, delimiters, len_delims, quotechars, len_quotes, escapechars,
                   len_escapes, lineterminators, len_lineterms);
}

extern "C" PyObject* guess_encoding_py(PyObject* self, PyObject* args) {
    PyObject* o;
    int strict = false;
    unsigned char* str;
    int strlen;

    if(!PyArg_ParseTuple(args, "Oi", &o, &strict))
        return NULL;

    if(PyBytes_Check(o)) {
        str = (unsigned char*)PyBytes_AsString(o);
        strlen = PyObject_Length(o);
    } else if(PyUnicode_Check(o)) {
        str = (unsigned char*)PyUnicode_DATA(o);
        if((strlen = PyObject_Length(o)) != -1)
            strlen *= (int)PyUnicode_KIND(o);
    } else {
        return PyErr_Format(PyExc_ValueError, "only bytes or unicode.");
    }

    const char* ret = guess_encoding(str, strlen, (bool)strict);
    if(ret)
        return PyUnicode_FromString(ret);
    Py_RETURN_NONE;
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

extern "C" PyObject* which_py(PyObject* self, PyObject* args) {
    PyObject* o;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;

    Py_ssize_t len = -1;
    wchar_t c[PATHMAX_LENGTH] = {0};
    if(o == NULL || (len = PyUnicode_AsWideChar(o, c, PATHMAX_LENGTH)) == NULL)
        return NULL;

    wchar_t res[PATHMAX_LENGTH] = {0};

    if(which(c, len, res))
        return PyUnicode_FromWideChar(res, -1);

    Py_RETURN_NONE;
}

extern "C" PyObject* Counter_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject *iterable, *res;
    PyObject* keyfunc = NULL;

    const char* kwlist[3] = {"iterable", "keyfunc", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", (char**)kwlist, &iterable, &keyfunc))
        return NULL;

    if(iterable == Py_None)
        PyErr_Format(PyExc_TypeError, "missing required argument 'iteraable' (pos 1)");

    if(keyfunc && !PyCallable_Check(keyfunc))
        PyErr_Format(PyExc_TypeError, "keyfunc required callable");

    if((res = PyDict_New()) == NULL)
        return NULL;

    if(Counter(res, iterable, keyfunc))
        return res;
    return NULL;
}

extern "C" PyObject* Grouper_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject *iterable, *res;
    PyObject *keyfunc = NULL, *valfunc = NULL;

    const char* kwlist[4] = {"iterable", "keyfunc", "valfunc", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO", (char**)kwlist, &iterable, &keyfunc, &valfunc))
        return NULL;

    if(iterable == Py_None)
        PyErr_Format(PyExc_TypeError, "missing required argument 'iteraable' (pos 1)");

    if(keyfunc && !PyCallable_Check(keyfunc))
        PyErr_Format(PyExc_TypeError, "keyfunc required callable");

    if(valfunc && !PyCallable_Check(valfunc))
        PyErr_Format(PyExc_TypeError, "valfunc required callable");

    if((res = PyDict_New()) == NULL)
        return NULL;

    if(Grouper(res, iterable, keyfunc, valfunc))
        return res;
    return NULL;
}

extern "C" PyObject* uniq_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* iterable;
    PyObject* keyfunc = NULL;
    int select_first = 1;

    const char* kwlist[4] = {"iterable", "keyfunc", "select_first", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi", (char**)kwlist, &iterable, &keyfunc, &select_first))
        return NULL;

    if(iterable == Py_None)
        PyErr_Format(PyExc_TypeError, "missing required argument 'iteraable' (pos 1)");

    return uniq(iterable, keyfunc == Py_None ? NULL : keyfunc, (bool)select_first);
}

extern "C" PyObject* listify_py(PyObject* self, PyObject* args) {
    PyObject *iterable, *mapping;
    if(!PyArg_ParseTuple(args, "O", &iterable))
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

extern "C" PyObject* to_hankaku_py(PyObject* self, PyObject* args) {
    PyObject* str;

    if(!PyArg_ParseTuple(args, "O", &str))
        return NULL;

    if(!PyUnicode_Check(str) || PyUnicode_READY(str) == -1)
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");

    unsigned int kind = PyUnicode_KIND(str);
    Py_ssize_t len;
    wchar_t* wdat = PyUnicode_AsWideCharString(str, &len);  //@todo bytes against
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
    wchar_t* wdat = PyUnicode_AsWideCharString(str, &len);  //@todo bytes against
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

extern "C" PyObject* is_office_py(PyObject* self, PyObject* args) {
    PyObject* o;
    const char* str;
    long res;

    if(!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    if((str = PyBytes_AsString(o)) == NULL)
        return PyErr_Format(PyExc_ValueError, "Need bytes string.");
    res = is_office(str, (std::size_t)PyObject_Length(o));
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

    Py_ssize_t len;
    if(PyDateTime_Check(o) || PyDate_Check(o))
        return o;
#if PY_MAJOR_VERSION == 2
    else if(PyString_Check(o)) {
        PyObject* u = PyObject_Unicode(o);
#else
    else if(PyBytes_Check(o)) {
        PyObject* u = PyObject_CallMethod(o, "decode", NULL);
#endif
        if(u == NULL)
            return PyErr_Format(PyExc_ValueError, "Cannot converting Unicode Data.");
        str = PyUnicode_AsWideCharString(u, &len);
        Py_DECREF(u);
    }

    else if(PyUnicode_Check(o))
        str = PyUnicode_AsWideCharString(o, &len);
    else
        return PyErr_Format(PyExc_ValueError, "Need unicode string data.");

    if(str == NULL)
        return PyErr_Format(PyExc_ValueError, "Cannot converting Data.");

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
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)  //@todo bytes against
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
    if((str = PyUnicode_AsWideCharString(o, &len)) == NULL)  //@todo bytes against
        return PyErr_Format(PyExc_UnicodeError, "Cannot converting Unicode Data.");

    if(format) {
        if(!PyUnicode_Check(format))
            return PyErr_Format(PyExc_ValueError, "Need strftime formating unicode string.");
        if((fmt = PyUnicode_AsWideCharString(format, &len)) == NULL)  //@todo bytes against
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

PyObject* deepcopy(PyObject* o) {
    PyObject *cp, *dcp, *otmp;
    if((cp = PyImport_ImportModule("copy")) == NULL) {
        return PyErr_Format(PyExc_ImportError, "Failed copy Module import");
    }
    if((dcp = PyObject_GetAttrString(cp, "deepcopy")) == NULL) {
        Py_DECREF(cp);
        return PyErr_Format(PyExc_ImportError, "Failed deepcopy Module import.");
    }
    if((otmp = PyObject_CallFunctionObjArgs(dcp, o)) == NULL) {
        Py_DECREF(cp);
        Py_DECREF(dcp);
        return PyErr_Format(PyExc_AttributeError, "Cannot deepcopy function Called.");
    }
    Py_DECREF(otmp);

    return otmp;
}

extern "C" PyObject* iterhead_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;

    const char* kwlist[2] = {"o", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O", (char**)kwlist, &o))
        return NULL;

    PyObject *head = NULL, *otmp = NULL;

    if((PySequence_Check(o) || PyRange_Check(o)) && (head = PySequence_GetItem(o, 0)) != NULL) {
        return head;
    }

    if(PyGen_Check(o) || PyIter_Check(o) || PyObject_CheckBuffer(o)) {
        if((otmp = deepcopy(o)) == NULL)
            return NULL;
    } else if(PyMapping_Check(o)) {
        if((otmp = PyObject_GetIter(o)) == NULL)
            return PyErr_Format(PyExc_ValueError, "Not iteratoratable.");
    } else {
        return PyErr_Format(PyExc_ValueError, "Unknown Object.");
    }

    if((head = PyIter_Next(otmp)) == NULL) {
        Py_DECREF(otmp);
        return PyErr_Format(PyExc_StopIteration, "Cannot iterator next call.");
    }
    Py_DECREF(otmp);
    return head;
}

extern "C" PyObject* itertail_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;

    const char* kwlist[2] = {"o", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O", (char**)kwlist, &o))
        return NULL;

    PyObject *tail = NULL, *otmp = NULL;
    Py_ssize_t len;

    if(PySequence_Check(o) || PyRange_Check(o)) {
        len = PyObject_Length(o) != -1;
        if((tail = PySequence_GetItem(o, len - 1)) != NULL)
            return tail;
    } else {
        return PyErr_Format(PyExc_IndexError, "Failed get tail.");
    }

    if(PyGen_Check(o) || PyIter_Check(o) || PyObject_CheckBuffer(o)) {
        if((otmp = deepcopy(o)) == NULL)
            return NULL;
    } else if(PyMapping_Check(o)) {
        if((otmp = PyObject_GetIter(o)) == NULL)
            return PyErr_Format(PyExc_ValueError, "Not iteratoratable.");
    } else {
        return PyErr_Format(PyExc_ValueError, "Unknown Object.");
    }

    while((tail = PyIter_Next(otmp)) != NULL) {
        // if(tail == NULL) {
        //     Py_DECREF(otmp);
        //     return tail;
        // }
        Py_DECREF(tail);
    }
    Py_DECREF(otmp);
    return tail;
}

extern "C" PyObject* iterheadtail_py(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* o;

    const char* kwlist[2] = {"o", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O", (char**)kwlist, &o))
        return NULL;

    PyObject *head = NULL, *tail = NULL, *otmp = NULL;
    Py_ssize_t len;

    if(PySequence_Check(o) || PyRange_Check(o)) {
        if((head = PySequence_GetItem(o, 0)) == NULL || (len = PyObject_Length(o)) != -1)
            return NULL;
        if((tail = PySequence_GetItem(o, len - 1)) == NULL) {
            Py_DECREF(head);
            return NULL;
        }
        return Py_BuildValue("[OO]", head, tail);
    }

    if(PyGen_Check(o) || PyIter_Check(o) || PyObject_CheckBuffer(o)) {
        if((otmp = deepcopy(o)) == NULL)
            return NULL;
    } else if(PyMapping_Check(o)) {
        if((otmp = PyObject_GetIter(o)) == NULL)
            return PyErr_Format(PyExc_ValueError, "Not iteratoratable.");
    } else {
        return PyErr_Format(PyExc_ValueError, "Unknown Object.");
    }

    if((head = PyIter_Next(otmp)) == NULL) {
        Py_DECREF(otmp);
        return PyErr_Format(PyExc_ValueError, "Cannot get head data.");
    }

    while((tail = PyIter_Next(otmp)) != NULL) {
        // if(tail == NULL) {
        //     Py_DECREF(otmp);
        //     return tail;
        // }
        Py_DECREF(tail);
    }
    Py_DECREF(otmp);
    return Py_BuildValue("[OO]", head, tail);
}

extern "C" PyObject* mklink_py(PyObject* self, PyObject* args) {
    PyObject* o;
    PyObject* srcpath;
    PyObject* targetpath;
    unsigned char* str;
    Py_ssize_t srclen;
    Py_ssize_t tarlen;

#if IS_WIN
    if(!PyArg_ParseTuple(args, "Ouu", &o, &srcpath, &targetpath))
        return NULL;

    wchar_t* src = PyUnicode_AsWideCharString(srcpath, &srclen);
    wchar_t* tar = PyUnicode_AsWideCharString(targetpath, &tarlen);
#else
    if(!PyArg_ParseTuple(args, "OUU", &o, &srcpath, &targetpath))
        return NULL;

    const char* src = PyUnicode_AS_DATA(srcpath);
    const char* tar = PyUnicode_AS_DATA(targetpath);
#endif
    if(mklink(src, tar))
        return NULL;
    Py_INCREF(targetpath);
    return targetpath;
}

#define MODULE_NAME _ccore
#define MODULE_NAME_S "_ccore"

/* {{{ */
// this module description
#define MODULE_DOCS "hevy use function faster function by CAPI Python implementation.\n"

#define binopen_DESC "always binary mode open.\n"
#define opener_DESC "always text mode open.\n"
#define headtail_DESC "header and tail binary data.\n"
#define guesstype_DESC "guess filetype from binary data.\n"
#define sniffer_DESC "csv delimiter guess define from data.\n"
#define getencoding_DESC "guess encoding from binary data.\n"
#define flatten_DESC "Always return 1D array(flatt list) object\n"
#define which_DESC "like GNU which function\n"
#define Counter_DESC "C implement Counter function\nDict Object return."
#define Grouper_DESC "C implement like groupby function\nDict Object return."
#define uniq_DESC "iterable data to uniq list."
#define listify_DESC "Always return list object.\n"
#define to_hankaku_DESC "from zenkaku data to hankaku data.\n"
#define to_zenkaku_DESC "from hankaku data to zenkaku data.\n"
#define kanji2int_DESC "from kanji num char to arabic num char.\n"
#define int2kanji_DESC "from arabic num char to kanji num char.\n"
#define lookuptype_DESC "lookup first file type.\n"
#define is_tar_DESC "tar file header check.\n"
#define is_lha_DESC "lha file header check.\n"
#define is_office_DESC "Microsoft Office file header check.\n"
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
#define iterhead_DESC "get head data\n"
#define itertail_DESC "get tail data\n"
#define iterheadtail_DESC "get head & tail data\n"
#define mklink_DESC "link maker for windows.\n"

/* }}} */
#define PY_ADD_METHOD(py_func, c_func, desc) \
    { py_func, (PyCFunction)c_func, METH_VARARGS, desc }
#define PY_ADD_METHOD_KWARGS(py_func, c_func, desc) \
    { py_func, (PyCFunction)c_func, METH_VARARGS | METH_KEYWORDS, desc }
#define PY_ADD_CLASS(klasstype)                                                   \
    do {                                                                          \
        if(PyType_Ready(klasstype) < 0)                                           \
            return NULL;                                                          \
        Py_INCREF(klasstype);                                                     \
        if(PyModule_AddObject(m, *klasstype.tp_name, (PyObject*)klasstype) < 0) { \
            Py_XDECREF(klasstype);                                                \
            goto error;                                                           \
        }                                                                         \
    } while(0)

/* Please extern method define for python */
/* PyMethodDef Parameter Help
 * https://docs.python.org/ja/3/c-api/structures.html#c.PyMethodDef
 */
static PyMethodDef py_methods[] = {
    PY_ADD_METHOD_KWARGS("binopen", binopen_py, binopen_DESC),
    PY_ADD_METHOD_KWARGS("opener", opener_py, opener_DESC),
    PY_ADD_METHOD("headtail", headtail_py, headtail_DESC),
    PY_ADD_METHOD("guesstype", guesstype_py, guesstype_DESC),
    PY_ADD_METHOD_KWARGS("sniffer", sniffer_py, sniffer_DESC),
    PY_ADD_METHOD("getencoding", guess_encoding_py, getencoding_DESC),
    PY_ADD_METHOD("flatten", flatten_py, flatten_DESC),
    PY_ADD_METHOD("which", which_py, which_DESC),
    PY_ADD_METHOD_KWARGS("Counter", Counter_py, Counter_DESC),
    PY_ADD_METHOD_KWARGS("Grouper", Grouper_py, Grouper_DESC),
    PY_ADD_METHOD_KWARGS("uniq", uniq_py, uniq_DESC),
    PY_ADD_METHOD("listify", listify_py, listify_DESC),
    PY_ADD_METHOD("to_hankaku", to_hankaku_py, to_hankaku_DESC),
    PY_ADD_METHOD("to_zenkaku", to_zenkaku_py, to_zenkaku_DESC),
    PY_ADD_METHOD("kanji2int", kanji2int_py, kanji2int_DESC),
    PY_ADD_METHOD("int2kanji", int2kanji_py, int2kanji_DESC),
    PY_ADD_METHOD("lookuptype", lookuptype_py, lookuptype_DESC),
    PY_ADD_METHOD("is_tar", is_tar_py, is_tar_DESC),
    PY_ADD_METHOD("is_lha", is_lha_py, is_lha_DESC),
    PY_ADD_METHOD("is_office", is_office_py, is_office_DESC),
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
    PY_ADD_METHOD_KWARGS("iterhead", iterhead_py, iterhead_DESC),
    PY_ADD_METHOD_KWARGS("itertail", itertail_py, itertail_DESC),
    PY_ADD_METHOD_KWARGS("iterheadtail", iterheadtail_py, iterheadtail_DESC),
    PY_ADD_METHOD("mklink", mklink_py, mklink_DESC),
    {NULL, NULL, 0, NULL}};

#include "../resource/g2d.const"

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef py_defmod = {PyModuleDef_HEAD_INIT, MODULE_NAME_S, MODULE_DOCS, 0, py_methods};
#define PARSE_NAME(mn) PyInit_##mn
#define PARSE_FUNC(mn)                             \
    PyMODINIT_FUNC PARSE_NAME(mn)() {              \
        PyDateTime_IMPORT;                         \
        PyObject* m = PyModule_Create(&py_defmod); \
        if(m == NULL)                              \
            return NULL;                           \
        APPEND_MODULE;                             \
        return m;                                  \
    error:                                         \
        Py_DECREF(m);                              \
        return NULL;                               \
    }

#else
#define PARSE_NAME(mn)                                                \
    init##mn(void) {                                                  \
        PyDateTime_IMPORT;                                            \
        (void)Py_InitModule3(MODULE_NAME_S, py_methods, MODULE_DOCS); \
    }
#define PARSE_FUNC(mn) PyMODINIT_FUNC PARSE_NAME(mn)
#endif

PARSE_FUNC(MODULE_NAME);
