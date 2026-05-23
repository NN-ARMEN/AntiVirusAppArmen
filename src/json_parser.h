#pragma once
#include <string>
#include <map>

class JsonParser {
public:
    static std::string GetStringValue(const std::string& json, const std::string& key);
    static int GetIntValue(const std::string& json, const std::string& key);
    static std::string SerializeObject(const std::map<std::string, std::string>& obj);
private:
    static size_t FindKey(const std::string& json, const std::string& key);
};