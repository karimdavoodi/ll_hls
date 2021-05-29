#pragma once
#include <string>
using std::string;

namespace Util {
    int to_int(const string&);
    void wait_millisecond(int msec);
    string unique_name(const string&);
}
