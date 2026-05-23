#include "json_parser.h"
#include <sstream>

size_t JsonParser::FindKey(const std::string& json, const std::string& key) {
    return json.find("\"" + key + "\"");
}

std::string JsonParser::GetStringValue(const std::string& json, const std::string& key) {
    size_t pos = FindKey(json, key);
    if (pos == std::string::npos) return "";
    size_t colonPos = json.find(':', pos);
    if (colonPos == std::string::npos) return "";
    size_t quoteStart = json.find('"', colonPos);
    if (quoteStart == std::string::npos) return "";
    size_t quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) return "";
    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

int JsonParser::GetIntValue(const std::string& json, const std::string& key) {
    std::string val = GetStringValue(json, key);
    return val.empty() ? 0 : std::stoi(val);
}

std::string JsonParser::SerializeObject(const std::map<std::string, std::string>& obj) {
    std::stringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& p : obj) {
        if (!first) ss << ",";
        ss << "\"" << p.first << "\":\"" << p.second << "\"";
        first = false;
    }
    ss << "}";
    return ss.str();
}