/* _ccore.cxx | MIT License | https://github.com/kirin123kirin/ccore/raw/main/LICENSE */
#include <Python.h>
#include <datetime.h>
#include <libnkf.h>
// #include <string>
// #include <utility>
#include "ccore.hpp"

// extern "C" PyObject* binopen_py(PyObject* self, PyObject* args, PyObject* kwargs) {
//     PyObject *o, *ioMod, *openedFile = NULL, *cn, *klass;
//     const char* mode = "rb";
//     const char* kwlist[3] = {"o", "mode", NULL};

//     if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O|s", (char**)kwlist, &o, &mode))
//         return NULL;

//     if(PyUnicode_Check(o)) {
//         ioMod = PyImport_ImportModule("io");
//         openedFile = PyObject_CallMethod(ioMod, "open", "Os", o, mode);
//         Py_DECREF(ioMod);
//         return openedFile;
//     } else if(PyObject_HasAttrString(o, "joinpath")) {
//         return PyObject_CallMethod(o, "open", "s", mode);
//     } else if(PyObject_HasAttrString(o, "_mode")) {
//         Py_INCREF(o);
//         return o;
//     } else if(PyObject_HasAttrString(o, "mode")) {
//         PyObject* m = PyObject_GetAttrString(o, "mode");
//         if(m && std::string(PyUnicode_AsUTF8(m)).find('b') != std::string::npos) {
//             Py_DECREF(m);
//             Py_INCREF(o);
//             return o;
//         }
//         Py_DECREF(m);
//     }

//     std::string klassname;

//     cn = PyObject_GetAttrString(o, "__class__");
//     klass = PyObject_GetAttrString(cn, "__name__");
//     PyErr_Clear();
//     if(klass) {
//         klassname += PyUnicode_AsUTF8(klass);
//         if(klassname == "ExFileObject" || klassname == "ZipExtFile") {
//             Py_DECREF(cn);
//             Py_DECREF(klass);
//             Py_INCREF(o);
//             return o;
//         }
//     }
//     Py_DECREF(cn);
//     Py_DECREF(klass);

//     if(klassname[0] == 'B' &&
//        (klassname == "BytesIO" || klassname == "BufferedReader" || klassname == "BufferedRWPair")) {
//         Py_INCREF(o);
//         return o;
//     }

//     if(PyObject_HasAttrString(o, "name")) {
//         PyObject* name = PyObject_GetAttrString(o, "name");
//         if(name && name != Py_None) {
//             // PyObject_CallMethod(o, "close", NULL);
//             ioMod = PyImport_ImportModule("io");
//             openedFile = PyObject_CallMethod(ioMod, "open", "Os", name, mode);
//             Py_DECREF(ioMod);
//             Py_DECREF(name);
//             Py_XINCREF(openedFile);
//             return openedFile;
//         }
//         Py_XDECREF(name);
//     }

//     if(klassname == "StringIO" || klassname == "TextIOWrapper") {
//         PyObject* encoding = PyObject_GetAttrString(o, "encoding");
//         PyObject* cb = PyObject_CallMethod(o, "getvalue", NULL);
//         PyObject* bio;
//         if(encoding && encoding != Py_None) {
//             bio = PyObject_CallMethod(cb ? cb : o, "encode", "O", encoding);
//         } else {
//             bio = PyObject_CallMethod(cb ? cb : o, "encode", NULL);
//         }
//         Py_XDECREF(encoding);
//         Py_XDECREF(cb);
//         if(bio == NULL)
//             return NULL;
//         ioMod = PyImport_ImportModule("io");
//         openedFile = PyObject_CallMethod(ioMod, "BytesIO", "O", bio);
//         if(PyObject_HasAttrString(o, "name")) {
//             PyObject_SetAttrString(openedFile, "name", PyObject_GetAttrString(o, "name"));
//         } else {
//             Py_INCREF(Py_None);
//             PyObject_SetAttrString(openedFile, "name", Py_None);
//         }
//         Py_DECREF(bio);
//         Py_DECREF(ioMod);
//         Py_XINCREF(openedFile);
//         return openedFile;
//     }

//     if(PyBytes_Check(o) || PyByteArray_Check(o)) {
//         ioMod = PyImport_ImportModule("io");
//         openedFile = PyObject_CallMethod(ioMod, "BytesIO", "O", o);
//         Py_DECREF(ioMod);
//         Py_INCREF(Py_None);
//         PyObject_SetAttrString(openedFile, "name", Py_None);
//         Py_XINCREF(openedFile);
//         return openedFile;
//     }

//     return PyErr_Format(PyExc_ValueError, "Unknown Object %s. filename or filepointer buffer", klassname.data());
// }

extern "C" PyObject* guess_encoding_py(PyObject* self, PyObject* args) {
    PyObject* o;
    unsigned char* str;
    int strlen;

    if(!PyArg_ParseTuple(args, "O", &o))
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

    const char* ret = guess_encoding(str, strlen);
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

#define MODULE_NAME _ccore
#define MODULE_NAME_S "_ccore"

/* {{{ */
// this module description
#define MODULE_DOCS "\n"

// #define binopen_DESC "Always binary mode open\n"
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
    // PY_ADD_METHOD_KWARGS("binopen", binopen_py, binopen_DESC),
    PY_ADD_METHOD("getencoding", guess_encoding_py, getencoding_DESC),
    PY_ADD_METHOD("flatten", flatten_py, flatten_DESC),
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
