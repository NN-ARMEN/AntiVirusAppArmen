#pragma once
#include <string>
#include <map>
#include <vector>

class JsonParser {
public:
    static std::string GetStringValue(const std::string& json, const std::string& key);
    static int GetIntValue(const std::string& json, const std::string& key);
    static bool GetBoolValue(const std::string& json, const std::string& key);
    static std::map<std::string, std::string> ParseObject(const std::string& json);
    static std::string SerializeObject(const std::map<std::string, std::string>& obj);
    
private:
    static size_t FindKey(const std::string& json, const std::string& key);
    static std::string ExtractString(const std::string& json, size_t pos);
};