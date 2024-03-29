//
// Created by Jesse Vogel on 01/07/2021.
//

#include <sstream>
#include "Message.h"

std::string Message::json_escape(const std::string &raw) {
    std::ostringstream ss;
    char u_buf[5];
    for (auto it = raw.begin(); it != raw.end(); ++it) {
        unsigned char c = *it;
        if (c == '\"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\b') ss << "\\b";
        else if (c == '\f') ss << "\\f";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else if (c >= 0x20 && c <= 0x7E) ss << c;
        else if (c >= 0b11000000 && c <= 0b11011111) { // 2-char unicode character
            int u = ((c & 0b00011111) << 6) + (*(++it) & 0b00111111);
            snprintf(u_buf, sizeof(u_buf), "%04X", u);
            ss << "\\u" << u_buf;
        } else {
            ss << "?";
        }
    }
    return ss.str();
}

const std::string status_strings[] = {"success", "fail", "error"};

std::string Message::create(MessageStatus status, const std::string &data) {
    std::ostringstream ss;
    ss << R"({"status":")" << status_strings[status] << R"(","data":")" << json_escape(data) << "\"}";
    return ss.str();
}

std::string Message::create(MessageStatus status, const std::vector<std::string> &data) {
    std::ostringstream ss;
    ss << R"({"status":")" << status_strings[status] << R"(","data":[)";
    bool first = true;
    for (const std::string &s: data) {
        if (!first) ss << ',';
        first = false;
        ss << '"' << json_escape(s) << '"';
    }
    ss << "]}";
    return ss.str();
}

std::string Message::create(MessageStatus status, const std::vector<std::string> &keys, const std::vector<std::vector<std::string>> &values) {
    std::ostringstream ss;
    size_t n = keys.size();
    ss << R"({"status":")" << status_strings[status] << R"(","data":[)";
    bool first = true;
    for (const auto &value: values) {
        if (!first)
            ss << ',';
        first = false;
        ss << '{';
        for (int i = 0; i < n; ++i) {
            if (i != 0)
                ss << ',';
            ss << '"' << json_escape(keys[i]) << "\":\"" << json_escape(value[i]) << '"';
        }
        ss << '}';
    }
    ss << "]}";
    return ss.str();
}
