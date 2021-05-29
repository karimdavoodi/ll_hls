#include <chrono>
#include <string>
#include <thread>
#include <boost/log//trivial.hpp>

#include "config.h"

using std::string;

namespace Util {
    int to_int(const string& str)
    {
        try{
            if(str.empty()) return -1;
            return stoi(str);
        }catch(...){
            LOG(error) <<  str << " is not integer!";
            return -1;
        }
    }
    void wait_millisecond(int msec)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    }
    string unique_name(const string& original_name)
    {
        string name;
        for (char c : original_name)
        {
            if (c >= 'a' && c <= 'z')
                name.push_back(c - ('a' - 'A'));
            else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                name.push_back(c);
            else
                name.push_back('_');
        }
        return name;
    }

}
