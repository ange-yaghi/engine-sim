#include "../include/json_util.h"

#include <fstream>
#include <sstream>

namespace {
    std::string escapeString(const std::string &s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '\\' || c == '"') out += '\\';
            out += c;
        }
        return out;
    }

    // Reads a double-quoted JSON string starting at s[i] == '"'. On success,
    // advances i past the closing quote and returns true.
    bool readQuotedString(const std::string &s, size_t &i, std::string *out) {
        if (i >= s.size() || s[i] != '"') return false;
        ++i;

        out->clear();
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                ++i;
                out->push_back(s[i]);
            }
            else {
                out->push_back(s[i]);
            }
            ++i;
        }

        if (i >= s.size()) return false; // unterminated string
        ++i; // consume closing quote

        return true;
    }

    void skipWhitespace(const std::string &s, size_t &i) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
    }
}

bool json_util::writeFlatStringMap(
        const std::string &path,
        const std::map<std::string, std::string> &values) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;

    file << "{\n";
    size_t index = 0;
    for (const auto &pair : values) {
        file << "    \"" << escapeString(pair.first) << "\": \""
             << escapeString(pair.second) << "\"";
        if (++index != values.size()) file << ",";
        file << "\n";
    }
    file << "}\n";

    return true;
}

bool json_util::readFlatStringMap(
        const std::string &path,
        std::map<std::string, std::string> *out) {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string contents = buffer.str();

    out->clear();

    size_t i = 0;
    skipWhitespace(contents, i);
    if (i >= contents.size() || contents[i] != '{') return false;
    ++i;

    skipWhitespace(contents, i);
    if (i < contents.size() && contents[i] == '}') return true; // empty object

    while (i < contents.size()) {
        skipWhitespace(contents, i);

        std::string key;
        if (!readQuotedString(contents, i, &key)) return false;

        skipWhitespace(contents, i);
        if (i >= contents.size() || contents[i] != ':') return false;
        ++i;

        skipWhitespace(contents, i);
        std::string value;
        if (!readQuotedString(contents, i, &value)) return false;

        (*out)[key] = value;

        skipWhitespace(contents, i);
        if (i < contents.size() && contents[i] == ',') {
            ++i;
            continue;
        }
        else if (i < contents.size() && contents[i] == '}') {
            break;
        }
        else {
            return false;
        }
    }

    return true;
}
