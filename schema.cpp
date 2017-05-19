#include "schema.h"
#include "stringutils.h"

JsonSchema::JsonSchema(const std::string& src) : m_source(src)
{
    m_schema = parse_string(m_source.c_str());
    m_expanded_schema = m_schema;
    __expandReference(m_expanded_schema);
}

const JsonValue& JsonSchema::getDefaultDocument() const
{
    if (m_defaultDocument.isObject()) return m_defaultDocument;

    if(!m_expanded_schema.hasKey("type") || !m_expanded_schema.hasKey("properties")) return JsonValue::emptyValue();
    if(m_expanded_schema["type"].asString() != "object") return JsonValue::emptyValue();

    m_defaultDocument = __getObject(m_schema);
    return m_defaultDocument;
}

JsonValue JsonSchema::getInfo(const std::string& ptr) const
{
    std::vector<std::string> keys;
    ssplit(keys, ptr, "/");
    if (keys.size() && keys[0] == "#") keys.erase(keys.begin());

    const JsonValue* prv = &m_expanded_schema;

    for (auto key : keys)
    {
        std::string stype = prv->operator[]("type").asString();
        if (stype == "object")
        {
            prv = &(prv->operator[]("properties")[key]);
        }
        else if (stype == "array")
        {
            prv = &(prv->operator[]("items"));
            if (prv->type() == JsonValue::Type::ARRAY)
            {
                prv = &(prv->operator[](0));
            }
        }
        else
        {
            return *prv;
        }
    }
    return *prv;
}

std::string JsonSchema::getTypeName(const std::string &ptr) const
{
    std::vector<std::string> keys;
    ssplit(keys, ptr, "/");
    if (keys.size() && keys[0] == "#") keys.erase(keys.begin());

    const JsonValue* pvalue = &m_schema;
    std::string key;
    for(auto it = keys.begin(); it != keys.end(); ++it)
    {
        key = *it;

        if (pvalue->hasKey("$ref"))
        {
            pvalue = &(m_schema.evalPointer(pvalue->operator[]("$ref").asString()));
        }

        std::string stype = pvalue->operator[]("type").asString();
        if (stype == "object")
        {
            pvalue = &(pvalue->operator[]("properties")[key]);
        }
        else if (stype == "array")
        {
            pvalue = &(pvalue->operator[]("items"));
            if (pvalue->type() == JsonValue::Type::ARRAY)
            {
                pvalue = &(pvalue->operator[](0));
            }
        }
        else
            break;
    }

    if (pvalue->hasKey("$ref"))
    {
        std::string ptr = pvalue->operator[]("$ref").asString();
        std::vector<std::string> keys;
        ssplit(keys, ptr, "/");
        if (keys.size() && keys[0] == "#") keys.erase(keys.begin());
        if (keys.size()) return keys.back();
    }

    return key;
}

void JsonSchema::__expandReference(JsonValue& src) const
{
    switch(src.type())
    {
    case JsonValue::Type::OBJECT:
    {
        if (src.hasKey("oneOf"))
        {
            JsonValue oneOf = src["oneOf"];
            src.erase("oneOf");
            src = src | oneOf[0];
        }

        while (src.hasKey("$ref"))
        {
            std::string jsonPtr = src["$ref"].asString();
            src.erase("$ref");
            JsonValue ref = m_schema.evalPointer(jsonPtr);
            src = src | ref;

            if (src.hasKey("oneOf"))
            {
                JsonValue oneOf = src["oneOf"];
                src.erase("oneOf");
                src = src | oneOf[0];
            }
        }
    }
    for(auto p : src) __expandReference(p.value);
    break;

    case JsonValue::Type::ARRAY:
        for(auto p : src) __expandReference(p.value);
        break;

    default:
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// enum ValueTypes {
//        UNDEFINED, BOOLEAN, NUMBER, INTEGER, STRING, ARRAY, OBJECT, RAW
//    };
//
JsonValue::Type JsonSchema::__getType(const std::string &typeName)
{
    if(typeName == "string")
        return JsonValue::Type::STRING;
    if(typeName == "integer")
        return JsonValue::Type::INTEGER;
    if(typeName == "number")
        return JsonValue::Type::NUMBER;
    if(typeName == "boolean")
        return JsonValue::Type::BOOLEAN;
    if(typeName == "object")
        return JsonValue::Type::OBJECT;
    if(typeName == "array")
        return JsonValue::Type::ARRAY;

    return JsonValue::Type::UNDEFINED;
}

JsonValue JsonSchema::__getProperty(const JsonValue &property) const
{
    JsonValue prop = property;

    if (prop.hasKey("oneOf"))
    {
        JsonValue oneOf = prop["oneOf"];
        prop.erase("oneOf");
        prop = prop | oneOf[0];
    }

    while (prop.hasKey("$ref"))
    {
        std::string jsonPtr = prop["$ref"].asString();
        prop.erase("$ref");

        JsonValue ref = m_schema.evalPointer(jsonPtr);
        prop = prop | ref;

        if (prop.hasKey("oneOf"))
        {
            JsonValue oneOf = prop["oneOf"];
            prop.erase("oneOf");
            prop = prop | oneOf[0];
        }
    }

    const JsonValue& property_type = prop["type"];

    JsonValue::Type type = JsonValue::Type::UNDEFINED;
    if(property_type.isString()) type = __getType(property_type.asString());
    if(property_type.isArray() && property_type.size()) type = __getType(property_type[0].asString());

    switch(type)
    {
    case JsonValue::Type::STRING:
        return __getString(prop);
    case JsonValue::Type::INTEGER:
        return __getInteger(prop);
    case JsonValue::Type::NUMBER:
        return __getNumber(prop);
    case JsonValue::Type::BOOLEAN:
        return __getBoolean(prop);
    case JsonValue::Type::ARRAY:
        return __getArray(prop);
    case JsonValue::Type::OBJECT:
        return __getObject(prop);
    default:
        return JsonValue();
    }
}

JsonValue JsonSchema::__getString(const JsonValue &property) const
{
    return property.hasKey("default") ? property["default"] : JsonValue().asString();
}

JsonValue JsonSchema::__getInteger(const JsonValue &property) const
{
    return property.hasKey("default") ? property["default"] : JsonValue().asInt();
}

JsonValue JsonSchema::__getNumber(const JsonValue &property) const
{
    return property.hasKey("default") ? property["default"] : JsonValue().asNumber();
}

JsonValue JsonSchema::__getBoolean(const JsonValue &property) const
{
    return property.hasKey("default") ? property["default"] : JsonValue().asBoolean();
}

JsonValue JsonSchema::__getObject(const JsonValue &property) const
{
    if(property.hasKey("default")) return property["default"];

    JsonValue rv(JsonValue::Type::OBJECT);
    if(property.hasKey("required") && property["required"].isArray())
    {
        const JsonValue& properties = property["properties"];
        for(const auto& p : property["required"])
        {
            std::string key = p.value.asString();
            if (properties.hasKey(key))
                rv[key] = __getProperty(properties[key]);
        }
    }
    return rv;
}

JsonValue JsonSchema::__getArray(const JsonValue &property) const
{
    if(property.hasKey("default")) return property["default"];

    JsonValue rv(JsonValue::Type::ARRAY);
    size_t minItemsCount = (size_t)std::max(0LL, property["minItems"].asInt());
    const JsonValue& items = property["items"];
    if (items.isObject())
    {
        JsonValue v = __getProperty(items);
        for(size_t i = 0; i < minItemsCount; ++i) rv.add(v);
    }
    else if (items.isArray())
    {
        size_t i = 0;
        for(const auto& item : items)
        {
            JsonValue v = __getProperty(item.value);
            if(i++ < minItemsCount) rv.add(v);
        }
    }
    return rv;
}
