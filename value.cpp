#include "value.h"
#include "3rdparty/jsmn/jsmn.h"
#include "3rdparty/utf8/utf8.h"
#include "stringutils.h"

#include <string>
#include <cfloat> /* DBL_MAX */
#include <vector>
#include <cstdlib> /* malloc */

/*---------------------------------------------------------------------------*/
/*  Implementation.                */
/*---------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////////////////////////
//
//
//  KeyValue class implementation
//
//
//////////////////////////////////////////////////////////////////////////////
KeyValue::KeyValue(std::string k, JsonValue& v) : key(k), value(v)
{
}

KeyValue::KeyValue(size_t n, JsonValue& v) : key(n), value(v)
{
}

KeyValue::KeyValue(JsonValue& v) : value(v)
{
}

//////////////////////////////////////////////////////////////////////////////
//
//
//  Value class implementation
//
//
//////////////////////////////////////////////////////////////////////////////
const JsonValue JsonValue::_dummyValue;

JsonValue::JsonValue(Type type) : _type(type), _parent(0)
{
    switch (_type)
    {
    case OBJECT:
        _value._o = new ObjectContainer ();
        break;

    case ARRAY:
        _value._a = new ArrayContainer ();
        break;

    case STRING:
        _value._s = new std::string ();
        break;

    default:
        break;
    }
}

JsonValue::JsonValue(bool v) : _type(BOOLEAN), _parent(0)
{
    _value._l = v;
}

JsonValue::JsonValue(int v) : _type(INTEGER), _parent(0)
{
    _value._i = (long long)v;
}

JsonValue::JsonValue(long v) : _type(INTEGER), _parent(0)
{
    _value._i = (long long)v;
}

JsonValue::JsonValue(long long v) : _type(INTEGER), _parent(0)
{
    _value._i = v;
}

JsonValue::JsonValue(size_t v) : _type(INTEGER), _parent(0)
{
    _value._i = (long long)v;
}

JsonValue::JsonValue(double v) : _type(NUMBER), _parent(0)
{
    _value._d = v;
}

JsonValue::JsonValue(const char* v) : _type(STRING), _parent(0)
{
    _value._s = new std::string (v);
}

JsonValue::JsonValue(const std::string& v) : _type(STRING), _parent(0)
{
    _value._s = new std::string (v);
}

JsonValue::JsonValue(std::string&& v) : _type(STRING), _parent(0)
{
    _value._s = new std::string ();
    std::swap (*(std::string*)_value._s, v);
}

/* ХИТРЫЙ КОНСТРУКТОР для объектов, прочитанных из потока */
JsonValue::JsonValue (char* buffer, size_t size, bool itIsString)
: _type(UNDEFINED), _parent(0)
{
    double d = 0;
    long long l = 0;
    char *p = 0;
    char lc = *(buffer + size);
    *(buffer + size) = 0;
    
    if (itIsString) goto parse_string;
    
    if (l = strtoll(buffer, &p, 10), (*p == 0)) /* ЦЕЛОЕ ЧИСЛО ... */
    {
        _type     = INTEGER;
        _value._i = l;
    }
    else if (d = strtod(buffer, &p), (*p == 0)) /* ЧИСЛО С ПЛАВАЮЩЕЙ ... */
    {
        _type     = NUMBER;
        _value._d = d;
    }
    else if (strcmp(buffer, "true") == 0)
    {
        _type     = BOOLEAN;
        _value._l = true;
    }
    else if (strcmp(buffer, "false") == 0)
    {
        _type     = BOOLEAN;
        _value._l = false;
    }
    else if (strcmp(buffer, "null") == 0)
    {
    }
    else
    {
    parse_string:
        _type = STRING;
        _value._s = new std::string(buffer, size);
        std::string& ps = *(_value._s);
        size_t r = (size_t)u8_unescape(&ps[0], (int)size, ps.c_str());
        ps.resize(r);
    }
    *(buffer + size) = lc;
}

JsonValue::Type JsonValue::type() const
{
    return _type;
}

bool JsonValue::isUndefined() const
{
    return _type == UNDEFINED;
}

bool JsonValue::isBoolean() const
{
    return _type == BOOLEAN;
}

bool JsonValue::isNumber() const
{
    return _type == NUMBER || _type == INTEGER;
}

bool JsonValue::isInteger() const
{
    return _type == INTEGER;
}

bool JsonValue::isString() const
{
    return _type == STRING;
}

bool JsonValue::isArray() const
{
    return _type == ARRAY;
}

bool JsonValue::isObject() const
{
    return _type == OBJECT;
}

void JsonValue::reset ()
{
    switch (_type)
    {
    case OBJECT:
        delete _value._o;
        break;

    case ARRAY:
        delete _value._a;
        break;

    case STRING:
        delete _value._s;
        break;

    default:
        break;
    }

    _type = UNDEFINED;
    _value = _Value();

}

const JsonValue &JsonValue::emptyValue()
{
    return _dummyValue;
}

bool JsonValue::isDummyValue() const
{
    return this == &_dummyValue;
}

JsonValue::Iterator JsonValue::begin() const
{
    return Iterator(this);
}

JsonValue::Iterator JsonValue::end() const
{
    return Iterator();
}

JsonValue::~JsonValue ()
{
    reset ();
}

JsonValue::JsonValue(const JsonValue& v) : _type(v._type), _parent(0)
{
    switch (_type)
    {
    case OBJECT:
        _value._o = new ObjectContainer (*v._value._o);
        for (auto& p : *_value._o) p.second._parent = this;
        break;

    case ARRAY:
        _value._a = new ArrayContainer (*v._value._a);
        for (auto& rv : * (_value._a)) rv._parent = this;
        break;

    case STRING:
        _value._s = new std::string (*v._value._s);
        break;

    default:
        _value = v._value;
        break;
    }
}

JsonValue::JsonValue (JsonValue&& v) : _type(v._type), _value(v._value),
    _parent(0)
{
    switch (_type)
    {
    case OBJECT:
        for (auto& p : *_value._o) p.second._parent = this;
        break;

    case ARRAY:
        for (auto& rv : * (_value._a)) rv._parent = this;
        break;

    default:
        break;
    }
    v._type = UNDEFINED;
}

JsonValue& JsonValue::operator= (const JsonValue& v)
{
    if (&v == this) return *this;

    Type savedType = v._type;
    _Value savedValue;

    switch (savedType)
    {
    case OBJECT:
        savedValue._o = new ObjectContainer (*v._value._o);
        for (auto& p : *savedValue._o) p.second._parent = this;
        break;

    case ARRAY:
        savedValue._a = new ArrayContainer (*v._value._a);
        for (auto& rv : * (savedValue._a)) rv._parent = this;
        break;

    case STRING:
        savedValue._s = new std::string (*v._value._s);
        break;

    default:
        savedValue = v._value;
        break;
    }

    reset ();

    _type = savedType;
    _value = savedValue;

    return *this;
}


JsonValue& JsonValue::operator= (JsonValue&& v)
{
    if (&v == this) return *this;

    reset ();

    _type = v._type;
    _value = v._value;

    v._type = UNDEFINED;

    switch (_type)
    {
    case OBJECT:
        for (auto& p : *_value._o) p.second._parent = this;
        break;

    case ARRAY:
        for (auto& rv : * (_value._a)) rv._parent = this;
        break;

    default:
        break;
    }

    return *this;
}

bool JsonValue::asBoolean (bool defaultValue) const
{
    switch (_type)
    {
    case BOOLEAN:
        return _value._l;
    case STRING:
        return !_value._s->empty();
    case INTEGER:
        return _value._i != 0;
    case NUMBER:
        return _value._d != 0;
    case UNDEFINED:
    case OBJECT:
    case ARRAY:
    default:
        return defaultValue;
    }
}

double JsonValue::asNumber (double defaultValue) const
{
    char *p = 0;
    switch (_type)
    {
    case BOOLEAN:
        return _value._l ? 1 : 0;
    case INTEGER:
        return (double)_value._i;
    case NUMBER:
        return _value._d;
    case STRING:
        return strtod (_value._s->c_str(), &p);
    default:
        return defaultValue;
    }
}

long long JsonValue::asInt (long long defaultValue) const
{
    char *p = 0;
    switch (_type)
    {
    case BOOLEAN:
        return _value._l ? 1 : 0;
    case INTEGER:
        return _value._i;
    case NUMBER:
        return (long long)_value._d;
    case STRING:
        return strtoll (_value._s->c_str(), &p, 10);
    default:
        return defaultValue;
    }
}

std::string JsonValue::asString (const std::string& defaultValue) const
{
    switch (_type)
    {
    case ARRAY:
        return "Array[]";
    case OBJECT:
        return "Object{}";
    case BOOLEAN:
        return _value._l ? "true" : "false";
    case INTEGER:
        return numberToString (_value._i);
    case NUMBER:
        return numberToString (_value._d);
    case STRING:
        return *_value._s;
    case UNDEFINED:
    default:
        return defaultValue;
    }
}

std::string JsonValue::asEscapedString (const std::string& defaultValue) const
{
    return escapedString (this->asString (defaultValue));
}

bool JsonValue::hasKey (const std::string &str) const
{
    if (_type != OBJECT) return false;
    return _value._o->find(str) != _value._o->end();
}

JsonValue& JsonValue::operator[] (size_t key)
{
    if (_type != ARRAY)
    {
        reset ();
        _type = ARRAY;
        _value._a = new ArrayContainer;
    }

    if (key < _value._a->size())
    {
        return (*_value._a)[key];
    }
    else
    {
        size_t oldSize = _value._a->size();
        _value._a->resize (key + 1);
        for (auto& rv : * (_value._a)) rv._parent = this;
        return _value._a->back ();
    }
}

JsonValue& JsonValue::add (const JsonValue& v)
{
    size_t oldSize = size();
    (*this)[oldSize] = v;
    return (*this)[oldSize];
}

bool JsonValue::insert(size_t pos, const JsonValue &v)
{
    if (_type != ARRAY || pos > _value._a->size())
    {
        return false;
    }

    auto it = _value._a->begin();
    std::advance(it, pos);
    _value._a->insert(it, v);
    for (auto& rv : * (_value._a)) rv._parent = this;
    return true;
}

bool JsonValue::insert(size_t pos, const std::string &key, const JsonValue &v)
{
    if (_type != OBJECT || pos > _value._o->size())
    {
        return false;
    }

    auto it = _value._o->begin();
    std::advance(it, pos);
    _value._o->insert(it, key, v);
    for (auto& p : *_value._o) p.second._parent = this;
    return true;
}

JsonValue& JsonValue::operator[] (const std::string& key)
{
    if (_type != OBJECT)
    {
        reset ();
        _type = OBJECT;
        _value._o = new ObjectContainer;
    }
    JsonValue& rv = _value._o->operator[](key);
    rv._parent = this;
    return rv;
}

const JsonValue& JsonValue::operator[] (const std::string& key) const
{
    switch (_type)
    {
    case OBJECT:
    {
        ObjectContainer::iterator i = _value._o->find (key);
        if (i != _value._o->end ()) return i->second;
    }
    break;

    default:
        break;
    }
    return _dummyValue;
}

const JsonValue& JsonValue::operator[] (size_t key) const
{
    switch (_type)
    {
    case ARRAY:
    {
        if (key < _value._a->size()) return (*_value._a)[key];
    }
    break;

    default:
        break;
    }
    return _dummyValue;
}

size_t JsonValue::size () const
{
    switch (_type)
    {
    case ARRAY:
        return _value._a->size();
    case OBJECT:
        return _value._o->size();
    default:
        return 0;
    }
}

void JsonValue::clear ()
{
    switch (_type)
    {
    case ARRAY:
        _value._a->clear ();
        break;

    case OBJECT:
        _value._o->clear ();
        break;

    default:
        break;
    }
}

void JsonValue::erase (const JsonValue& key)
{
    switch (_type)
    {
    case ARRAY:
    {
        int N = key.asInt();
        if (N >= 0 && N < (int)_value._a->size())
        {
            auto it = _value._a->begin();
            std::advance(it, N);
            _value._a->erase (it);
        }
    }
    break;

    case OBJECT:
        _value._o->erase (key.asString ());
        break;

    default:
        break;
    }
}

std::vector<std::string> JsonValue::indexes () const
{
    if (_type != OBJECT) return std::vector<std::string>();
    std::vector<std::string> rv(_value._o->size());
    std::vector<std::string>::iterator p = rv.begin ();
    for (auto i = _value._o->begin(); i != _value._o->end(); ++i)
    {
        *p = (*i).first;
        ++p;
    }
    return rv;
}

JsonValue JsonValue::operator+ (const JsonValue& v) const
{
    switch (_type)
    {
    case UNDEFINED:
        return v;
    case BOOLEAN:
        return _value._l && v.asBoolean();
    case INTEGER:
        return _value._i + v.asInt();
    case NUMBER:
        return _value._d + v.asNumber();
    case STRING:
        return *_value._s + v.asString();
    case ARRAY:
    {
        JsonValue rv(*this);
        switch (v._type)
        {
        case ARRAY:
            rv._value._a->insert(
                rv._value._a->end(),
                v._value._a->begin(),
                v._value._a->end());
            for (auto& rv : * (_value._a)) rv._parent = this;
            break;

        default:
            rv._value._a->insert(rv._value._a->end(), v);
            for (auto& rv : * (_value._a)) rv._parent = this;
            break;
        }
        return rv;
    }

    case OBJECT:
    {
        JsonValue rv(*this);
        switch (v._type)
        {
        case OBJECT:
            for (const auto& j : *v.asObject())
            {
                rv[j.first] = rv[j.first] + j.second;
            }
            break;

        default:
            rv[v.asString()] = rv[v.asString()] + v;
            break;
        }
        return rv;
    }
    }
    return JsonValue();
}

JsonValue JsonValue::operator| (const JsonValue& v) const
{
    switch (_type)
    {
    case UNDEFINED:
        return v;

    case BOOLEAN:
    case INTEGER:
    case NUMBER:
    case STRING:
        return *this;

    case ARRAY:
    {
        JsonValue rv(*this);
        switch (v._type)
        {
        case ARRAY:
            rv._value._a->insert(
                rv._value._a->end(),
                v._value._a->begin(),
                v._value._a->end());
            for (auto& rv : * (_value._a)) rv._parent = this;
            break;

        default:
            rv._value._a->insert(rv._value._a->end(), v);
            for (auto& rv : * (_value._a)) rv._parent = this;
            break;
        }
        return rv;
    }

    case OBJECT:
    {
        JsonValue rv(*this);
        switch (v._type)
        {
        case OBJECT:
            for (const auto& j : *v.asObject())
            {
                rv[j.first] = rv[j.first] | j.second;
            }
            break;

        default:
            rv[v.asString()] = rv[v.asString()] | v;
            break;
        }
        return rv;
    }
    }
    return JsonValue();
}



/*
11.9.3 The Abstract Equality Comparison Algorithm
http://www.ecma-international.org/ecma-262/5.1/#sec-11.9.3
*/
bool JsonValue::operator==(const JsonValue& v) const
{
    if (_type == v._type)
    {
        switch (_type)
        {
        case UNDEFINED:
            return true;
        case BOOLEAN:
            return _value._l == v._value._l;
        case INTEGER:
            return _value._i == v._value._i;
        case NUMBER:
            return _value._d == v._value._d;
        case STRING:
            return *_value._s == *v._value._s;
        case ARRAY:
        case OBJECT:
            return stringifyThis() == v.stringifyThis();
        }
    }
    else
    {
        switch (v._type)
        {
        case BOOLEAN:
            return asBoolean() == v._value._l;
        case INTEGER:
            return asInt() == v._value._i;
        case NUMBER:
            return asNumber() == v._value._d;
        case STRING:
            return asString() == *v._value._s;
        case UNDEFINED:
        case ARRAY:
        case OBJECT:
            return false;
        }
    }
    return false;
}

std::string JsonValue::stringifyThis() const
{
    return stringify(*this, true);
}

std::string JsonValue::prettyStringifyThis() const
{
    return prettyStringify(*this);
}

ObjectContainer* JsonValue::asObject () const
{
    switch (_type)
    {
    case OBJECT:
        return _value._o;
    default:
        return 0;
    }
}

ArrayContainer* JsonValue::asArray () const
{
    switch (_type)
    {
    case ARRAY:
        return _value._a;
    default:
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//
//  END OF Value class implementation
//
//
//////////////////////////////////////////////////////////////////////////////
inline std::string jsmn_dump_string_token (jsmntok_t *obj, char* js)
{
    return std::string(js + obj->start, (size_t)(obj->end - obj->start));
}

JsonValue jsmn_dump_token (jsmntok_t **pobj, char* js)
{
    jsmntok_t* obj = *pobj;
    
    switch (obj->type)
    {
        case JSMN_PRIMITIVE:
        case JSMN_STRING:
        {
            return JsonValue(js + obj->start, (size_t)(obj->end - obj->start), (obj->type==JSMN_STRING));
        }

        case JSMN_ARRAY:
        {
            JsonValue arr(JsonValue::Type::ARRAY);
            ArrayContainer& ac = *(arr.asArray());
            for (int i = 0, osz = obj->size; i < osz; ++i)
            {
                ac.emplace_back(jsmn_dump_token (&(++(*pobj)), js));
            }
            return arr;
        }
            
        case JSMN_OBJECT:
        {
            JsonValue obj2(JsonValue::Type::OBJECT);
            for (int i = 0, osz = obj->size; i < osz; ++i)
            {
                obj2[jsmn_dump_string_token (++(*pobj), js)] = jsmn_dump_token (&(++(*pobj)), js);
            }
            return obj2;
        }
            
        default:
            return JsonValue ();
    }
}

JsonValue parse_string(const char* string)
{
    std::string s(string);
    return parse_buffer(&s[0], s.size());
}

JsonValue parse_buffer (char* bufferHead, size_t bufferSize)
{
    JsonValue res;
    
    int r;
    unsigned int num_tokens = 1024;
    jsmntok_t* tokens = 0;
    jsmn_parser parser;
    jsmn_init (&parser);

LOOP_JSMN_ERROR_NOMEM:

    tokens = (jsmntok_t*)realloc(tokens, num_tokens * sizeof(jsmntok_t));
    if (tokens == 0)
    {
        exit(EXIT_FAILURE);
    }

    r = jsmn_parse (&parser, bufferHead, bufferSize, tokens, num_tokens);

    if ( r >= 0 )
    {
        jsmntok_t *T = tokens;
        res = jsmn_dump_token (&T, (char*)bufferHead);

    }
    else if (r == JSMN_ERROR_NOMEM )
    {
        num_tokens += num_tokens;
        goto LOOP_JSMN_ERROR_NOMEM;

    }
    else if (r == JSMN_ERROR_INVAL )
    {
    }
    else if (r == JSMN_ERROR_PART )
    {
    }

    free(tokens);

    return res;
}

JsonValue parse_file (const char* fileName)
{
    JsonValue res;
    char* js = 0;
    size_t filesize = 0;

    FILE* f = fopen (fileName, "rb");
    if (f == 0)
    {
        return res;
    }

    fseek(f,0,SEEK_END);
    filesize=(size_t)ftell(f);
    fseek(f,0,SEEK_SET);
    js=(char*)malloc(filesize);
    if (js) {
        filesize = fread(js, 1, filesize, f);
        fclose(f);
        res = parse_buffer (js, filesize);
        free(js);
    }
    return res;
}

std::string prettyStringify (const JsonValue& v, const std::string &indent,
                             const std::string &indent00,
                             const std::string &indent11,
                             const std::string &eol, bool sorted)
{
    std::string res;
    std::string nextIdent = indent11 + indent;

    switch (v.type())
    {
    case JsonValue::Type::UNDEFINED :
        return std::string ("null");

    case JsonValue::Type::INTEGER :
    case JsonValue::Type::NUMBER :
    case JsonValue::Type::BOOLEAN :
        return v.asString ();

    case JsonValue::Type::STRING :
    {
        res += "\"";
        res += v.asEscapedString ();
        res += "\"";
        return res;
    }
    case JsonValue::Type::ARRAY :
    {
        res.reserve(1024);
        res += "[";
        res += eol;

        int i = 0;
        const ArrayContainer& ac = *v.asArray();
        for (auto a = ac.begin(), e = ac.end(); a != e; ++a)
        {
            if (i)
            {
                res += ",";
                res += eol;
            }

            res += indent11;
            res += prettyStringify (*a, indent, indent11, nextIdent, eol);

            ++i;
        }
        res += eol;
        res += indent00;
        res += "]";
        return res;
    }

    case JsonValue::Type::OBJECT :
    {
        res.reserve(1024);
        res += "{";
        res += eol;

        if (sorted)
        {
            int i = 0;
            std::vector<std::string> keys = v.indexes();
            std::sort(keys.begin(), keys.end());
            for (auto key : keys)
            {
                if (i)
                {
                    res += ",";
                    res += eol;
                }

                res += indent11;
                res += "\"";
                res += escapedString (key);
                res += "\":";
                res += prettyStringify (v[key], indent, indent11, nextIdent, eol);

                ++i;
            }
            res += eol;
        }
        else
        {
            int i = 0;
            ObjectContainer& oc = *v.asObject();
            for (auto p = oc.begin(), e = oc.end(); p != e; ++p)
            {
                if (i)
                {
                    res += ",";
                    res += eol;
                }

                res += indent11;
                res += "\"";
                res += escapedString (p->first);
                res += "\":";
                res += prettyStringify (p->second, indent, indent11, nextIdent, eol);

                ++i;
            }
            res += eol;
        }
        res += indent00;
        res += "}";
        return res;
    }

    default:
        break;
    }
    return res;
}

std::string stringify (const JsonValue& v, bool sorted)
{
    return prettyStringify (v, "", "", "", "", sorted);
}

const JsonValue* JsonValue::parent() const
{
    return _parent;
}

const JsonValue* JsonValue::root() const
{
    const JsonValue* root = this->_parent ? this->_parent : this;
    while (root && root->_parent) root = root->_parent;
    return root;
}

JsonValue JsonValue::key() const
{
    if (_parent != 0)
    {
        for (const auto& p : * (this->_parent))
        {
            if (&(p.value) == this) return p.key;
        }
    }
    return JsonValue();
}

int JsonValue::pos() const
{
    if (_parent != 0)
    {
        int rv = 0;
        for (const auto& p : * (this->_parent))
        {
            if (&(p.value) == this) return rv;
            ++rv;
        }
    }
    return -1;
}

const JsonValue& JsonValue::at(int pos) const
{
    if (pos < 0 || pos >= (int)this->size()) return _dummyValue;

    switch (_type)
    {
    case ARRAY:
        return _value._a->operator[](pos);
        break;

    case OBJECT:
    {
        ObjectContainer::iterator i = _value._o->begin();
        std::advance(i, pos);
        return i->second;
    }
    break;

    default:
        break;
    }
    return _dummyValue;
}

std::string JsonValue::getPointer() const
{
    std::string ptr;
    const JsonValue* v = this;
    while (v && v->_parent)
    {
        ptr = "/" + v->key().asString() + ptr;
        v = v->_parent;
    }
    return ptr;
}

const JsonValue &JsonValue::evalPointer(const std::string& ptr,
                                        bool zeroIndexOnly) const
{
    std::vector<std::string> keys = ssplit(ptr, "/");
    const JsonValue* prv = root();
    if (keys.size() && keys[0] == "#") keys.erase(keys.begin());
    for (auto key : keys)
    {
        if (prv->type() == ARRAY)
        {
            long long l = 0;
            char *p = 0;
            if (l = strtoll(key.c_str(), &p, 10), *p == 0) /* ЦЕЛОЕ ЧИСЛО ... */
            {
                // zeroIndexOnly используется схемой
                // для получения кусков дефолтного документа
                size_t id = zeroIndexOnly ? 0 : l;
                if (id < prv->size())
                {
                    prv = &(prv->operator[](id));
                    continue;
                }
            }
        }
        else if (prv->type() == OBJECT)
        {
            // RFC 6901
            replace(key, "~1", "/");
            replace(key, "~0", "~");

            if (prv->hasKey(key))
            {
                prv = &(prv->operator[](key));
                continue;
            }
        }

        prv = &_dummyValue;
        break;
    }
    return *prv;
}

bool JsonValue::isReference() const
{
    return (
               (type() == JsonValue::STRING) &&
               (_value._s->at(0) == '/') &&
               (*(_value._s) != this->getPointer())
                );
}

JsonValue& JsonValue::getReference() const
{
    const JsonValue* ptr = this;
    while (ptr != &_dummyValue && ptr->isReference())
    {
        ptr = &(ptr->evalPointer(ptr->asString()));
    }
    return *(JsonValue*)(ptr == &_dummyValue ? this : ptr);
}


//////////////////////////////////////////////////////////////////////////////
//
//
//  Value::Iterator class implementation
//
//
//////////////////////////////////////////////////////////////////////////////
JsonValue::Iterator::Iterator(const JsonValue* owner) : _owner(owner)
{
    if (_owner)
    {
        switch (_owner->type())
        {
        case ARRAY:
        {
            ArrayContainer::iterator* pit = new ArrayContainer::iterator(
                owner->asArray()->begin()
            );
            if (*pit == owner->asArray()->end())
            {
                delete pit;
                _type = ValueIteratorTypes::UNDEFINED_ITERATOR;
                _base._arrayIterator = 0;
            }
            else
            {
                _type = ValueIteratorTypes::ARRAY_ITERATOR;
                _base._arrayIterator = pit;
                _idx = 0;
            }
        }
        break;

        case OBJECT:
        {
            ObjectContainer::iterator* pit = new ObjectContainer::iterator(
                owner->asObject()->begin()
            );
            if (*pit == owner->asObject()->end())
            {
                delete pit;
                _type = ValueIteratorTypes::UNDEFINED_ITERATOR;
                _base._objectIterator = 0;
            }
            else
            {
                _type = ValueIteratorTypes::OBJECT_ITERATOR;
                _base._objectIterator = pit;
            }
        }
        break;

        default:
        {
            _type = ValueIteratorTypes::BASIC_ITERATOR;
        }
        break;
        }
    }
    else
    {
        _type = ValueIteratorTypes::UNDEFINED_ITERATOR;
    }
}

JsonValue::Iterator::~Iterator()
{
    switch (_type)
    {
    case ARRAY_ITERATOR:
        delete _base._arrayIterator;
        break;

    case OBJECT_ITERATOR:
        delete _base._objectIterator;
        break;

    default:
        break;
    }
}

bool JsonValue::Iterator::operator!=(const Iterator& v)
{
    if (_type != v._type) return true;
    switch (_type)
    {
    case ARRAY_ITERATOR:
        return *(_base._arrayIterator) != *(v._base._arrayIterator);
    case OBJECT_ITERATOR:
        return *(_base._objectIterator) != *(v._base._objectIterator);
    default:
        return _type != v._type;
    }
    return true;
}

JsonValue::Iterator& JsonValue::Iterator::operator++()
{
    switch (_type)
    {
    case ARRAY_ITERATOR:
    {
        if (++(*(_base._arrayIterator)) == _owner->asArray()->end())
        {
            delete _base._arrayIterator;
            _base._arrayIterator = 0;
            _type = ValueIteratorTypes::UNDEFINED_ITERATOR;
        }
        _idx++;
    }
    break;

    case OBJECT_ITERATOR:
    {
        if (++(*(_base._objectIterator)) == _owner->asObject()->end())
        {
            delete _base._objectIterator;
            _base._objectIterator = 0;
            _type = ValueIteratorTypes::UNDEFINED_ITERATOR;
        }
    }
    break;

    default:
        _type = ValueIteratorTypes::UNDEFINED_ITERATOR;
    }
    return *this;
}

KeyValue JsonValue::Iterator::operator*()
{
    switch (_type)
    {
    case ARRAY_ITERATOR:
        return KeyValue(_idx, *(*(_base._arrayIterator)));
    case OBJECT_ITERATOR:
        return KeyValue(
                   (*(*(_base._objectIterator))).first,
                   (*(*(_base._objectIterator))).second
               );
    default:
        return KeyValue(*(const_cast<JsonValue *>(_owner)));
    }
}
