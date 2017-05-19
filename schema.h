#ifndef SCHEMA_H
#define SCHEMA_H

#include "value.h"
#include <string>

class JsonSchema
{
public:
    JsonSchema(const std::string& src);
    const JsonValue& getDefaultDocument() const;
    JsonValue getInfo(const std::string& ptr) const;
    std::string getTypeName(const std::string& ptr) const;

private:
    static JsonValue::Type __getType(const std::string& typeName);
    JsonValue __getProperty(const JsonValue &property) const;
    JsonValue __getString(const JsonValue& property) const;
    JsonValue __getInteger(const JsonValue& property) const;
    JsonValue __getNumber(const JsonValue& property) const;
    JsonValue __getBoolean(const JsonValue& property) const;
    JsonValue __getObject(const JsonValue& property) const;
    JsonValue __getArray(const JsonValue& property) const;
    void __expandReference(JsonValue &src) const;

private:
    std::string m_source;
    JsonValue m_schema;
    JsonValue m_expanded_schema;
    mutable JsonValue m_defaultDocument;
};

#endif // SCHEMA_H
