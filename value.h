#ifndef VALUE_H
#define VALUE_H

#include <map>
#include <unordered_map>
#include <vector>
#include <string>

// Если в качестве ArrayContainer используется std::vector
// то ссылки и указатели на элементы контейнера могут стать не валидными
// при изменении размера контейнера
// даже если сам элемент "не пострадал"
//
// Определяя макрос
#define USE_STABLE_ARRAY_CONTAINER
// мы в качестве ArrayContainer начинаем использовать
// собственный класс на базе std::list

// Определяя макрос
#define USE_STABLE_OBJECT_CONTAINER
// мы в качестве ObjectContainer начинаем использовать
// собственный класс на базе std::list

class JsonValue;
struct KeyValue;

#ifdef USE_STABLE_OBJECT_CONTAINER
#include "linkedmap.h"
typedef LinkedMap<std::string, JsonValue> ObjectContainer;
#else
typedef std::map<std::string, JsonValue> ObjectContainer;
#endif

#ifdef USE_STABLE_ARRAY_CONTAINER
class ArrayContainer : public std::list<JsonValue>
{
public:
    ArrayContainer() : std::list<JsonValue>() {}

    template<class InputIt>
    ArrayContainer(InputIt first, InputIt last)
        : std::list<JsonValue>(first, last)
    {}

    JsonValue& operator[](size_t n)
    {
        auto it = begin();
        std::advance(it, n);
        return (*it);
    };
};
#else
typedef std::vector<JsonValue> ArrayContainer;
#endif

class JsonValue
{
public:
    enum Type {UNDEFINED, BOOLEAN, NUMBER, INTEGER, STRING, ARRAY, OBJECT};

    JsonValue (Type type = UNDEFINED);
    ~JsonValue ();

    JsonValue (const JsonValue& v);
    JsonValue (JsonValue&& v);
    JsonValue& operator=(const JsonValue& v);
    JsonValue& operator=(JsonValue&& v);

    JsonValue (bool v);
    JsonValue (int v);
    JsonValue (long v);
    JsonValue (long long v);
    JsonValue (size_t v);
    JsonValue (double v);
    JsonValue (const char* v);
    JsonValue (const char* v, int);
    JsonValue (const std::string& v);
    JsonValue (std::string&& v);

    template<class InputIt>
    JsonValue(InputIt first, InputIt last)
        : _type(ARRAY)
    {
        _value._a = new ArrayContainer(first, last);
        for (auto& rv : * (_value._a)) rv._owner = this;
    }

    template<class T>
    JsonValue(const std::unordered_map<std::string, T>& v)
        : _type(OBJECT)
    {
        _value._o = new ObjectContainer(v.begin(), v.end());
        for (auto& p : *_value._o) p.second._owner = this;
    }

    Type type() const;
    bool isUndefined () const;
    bool isBoolean () const;
    bool isNumber () const;
    bool isInteger () const;
    bool isString () const;
    bool isArray () const;
    bool isObject () const;

    bool asBoolean (bool defaultValue = false) const;
    double asNumber (double defaultValue = 0) const;
    long long asInt (long long defaultValue = 0) const;
    std::string asString (const std::string& defaultValue = "") const;

    std::string asEscapedString (const std::string& defaultValue = "") const;
    bool hasKey (const std::string &str) const;

    JsonValue& operator[](const std::string& key);
    JsonValue& operator[](size_t key);
    JsonValue& add(const JsonValue& v);

    bool insert(size_t pos, const JsonValue& v);
    bool insert(size_t pos, const std::string& key, const JsonValue& v);

    const JsonValue& operator[] (const std::string& key) const;
    const JsonValue& operator[] (size_t key) const;

    size_t size () const;

    /* удаляет элемент с ключом key */
    void erase (const JsonValue& key);
    /* удаляет все элементы контейнера */
    void clear ();

    /* возвращает список ключей объекта в виде Value */
    std::vector<std::string> indexes () const;

    /* два могучих оператора для комбинирования значений Value */
    JsonValue operator+(const JsonValue& id) const;
    JsonValue operator|(const JsonValue& id) const;

    bool operator==(const JsonValue& id) const;

    std::string stringifyThis() const;
    std::string prettyStringifyThis() const;

    ObjectContainer* asObject () const;
    ArrayContainer* asArray () const;

private:
    void reset ();

    Type _type;

    union _Value
    {
        ObjectContainer* _o;
        ArrayContainer* _a;
        std::string* _s;

        bool _l;
        long long _i;
        double _d;
    } _value;

    static const JsonValue _emptyValue;

public:
    static const JsonValue& emptyValue();
    bool isEmptyValue() const;

    /////////////////////////////////////////////////////////////////////////
    // Итератор Value сделан для обхода через Range-based for loop
private:
    enum ValueIteratorTypes
    {
        UNDEFINED_ITERATOR,
        ARRAY_ITERATOR,
        OBJECT_ITERATOR,
        BASIC_ITERATOR
    };

    class Iterator
    {
        const JsonValue* _owner;
        ValueIteratorTypes _type;
        ptrdiff_t _idx;
        union
        {
            ArrayContainer::iterator* _arrayIterator;
            ObjectContainer::iterator* _objectIterator;
        } _base;

    public:
        Iterator(const JsonValue* owner = 0);
        ~Iterator();
        bool operator!=(const Iterator& v);
        Iterator& operator++();
        KeyValue operator*();
    };
public:
    Iterator begin() const;;
    Iterator end() const;;

    /////////////////////////////////////////////////////////////////////////
    // Набор для поддержания двунаправленной иерархии
private:
    const JsonValue* _owner;

public:
    // возвращает указатель на контейнер-владелец
    const JsonValue* owner() const;
    // возвращает указатель на самый верхний контейнер-владелец
    const JsonValue* root() const;
    // возвращает ключ в контейнере-владельце
    JsonValue key() const;
    // возвращает позицию в контейнере-владельце
    int pos() const;
    // возвращает элемент контейнера в позиции pos
    const JsonValue& at(int pos) const;

    ////////////////////////////////////////////////////////////////////////
    // https://tools.ietf.org/html/rfc6901
    // JSON Pointer defines a string syntax for identifying a specific value
    // within a JavaScript Object Notation (JSON) document.
public:
    ///
    /// \brief getPointer вычисляет адрес объекта внутри root
    /// \return строку-указатель согласно rfc6901
    ///
    std::string getPointer() const;
    ///
    /// \brief evalPointer находит объект по адресу внутри root
    /// \param ptr -- строка-указатель
    /// \param zeroIndexOnly -- используется классом JsonSchema
    /// для получения кусков дефолтного документа
    /// \return константную ссылку на искомый объект или
    /// на _emptyValue
    ///
    const JsonValue& evalPointer(const std::string& ptr,
                                 bool zeroIndexOnly = false) const;
    ///
    /// \brief getReference раскручивает цепочку ссылок
    /// \return возвращает сам объект или объект на который
    /// он ссылается
    ///
    /// Похожа на функцию evalPointer, но ведёт себя несколько иначе.
    /// Возникла позже.
    /// Использует evalPointer.
    ///
    JsonValue& getReference() const;
private:
    ///
    /// \brief isReference определяет содержит ли JsonValue значение
    /// типа Pointer
    /// используется функцией getRef
    /// \return
    ///
    bool isReference() const;
};

struct KeyValue
{
    JsonValue key;
    JsonValue& value;

    explicit KeyValue(std::string k, JsonValue& v);
    explicit KeyValue(size_t n, JsonValue& v);
    explicit KeyValue(JsonValue& v);
};

JsonValue parse_string (const char* js);
JsonValue parse_file (const char* fileName);

std::string stringify (const JsonValue& v, bool sorted = false);

std::string
prettyStringify (const JsonValue& v,
                 const std::string& indent = "  ",
                 const std::string& indent0 = "",
                 const std::string& indent1 = "  ",
                 const std::string& eol = "\n",
                 bool sorted = false
                );

#endif  // VALUE_H
