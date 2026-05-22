#include "json_parser.h"
#include <sstream>

std::string JsonParser::GetStringValue(const std::string& json, const std::string& key) {
    size_t pos = FindKey(json, key);
    if (pos == std::string::npos) return "";
    
    // Ищем открывающую кавычку после двоеточия
    size_t colonPos = json.find(':', pos);
    if (colonPos == std::string::npos) return "";
    
    size_t quoteStart = json.find('"', colonPos);
    if (quoteStart == std::string::npos) return "";
    
    size_t quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) return "";
    
    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

int JsonParser::GetIntValue(const std::string& json, const std::string& key) {
    std::string str = GetStringValue(json, key);
    if (str.empty()) return 0;
    return std::stoi(str);
}

bool JsonParser::GetBoolValue(const std::string& json, const std::string& key) {
    std::string str = GetStringValue(json, key);
    if (str.empty()) return false;
    return (str == "true" || str == "1");
}

size_t JsonParser::FindKey(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    return json.find(searchKey);
}

std::map<std::string, std::string> JsonParser::ParseObject(const std::string& json) {
    std::map<std::string, std::string> result;
    // Упрощённый парсинг JSON
    return result;
}

std::string JsonParser::SerializeObject(const std::map<std::string, std::string>& obj) {
    std::stringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& pair : obj) {
        if (!first) ss << ",";
        ss << "\"" << pair.first << "\":\"" << pair.second << "\"";
        first = false;
    }
    ss << "}";
    return ss.str();
}