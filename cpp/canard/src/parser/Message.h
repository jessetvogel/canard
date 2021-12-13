//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <string>
#include <vector>

enum MessageStatus {
    SUCCESS = 0, FAIL = 1, ERROR = 2
};

class Message {

    static std::string escape(const std::string &);

public:

    static std::string create(MessageStatus, const std::string &);
    static std::string create(MessageStatus, const std::vector<std::string> &);
    static std::string create(MessageStatus, const std::vector<std::string> &, const std::vector<std::string> &);

};
