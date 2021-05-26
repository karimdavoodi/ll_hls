#include <vector>
#include <string>
#include <hiredis/hiredis.h>

using std::string;
/**
 * @brief Implement some functions of redis 
 * 
 */
class RedisClient{
    private:
        string address;
        int port; 
        string passwd;
        redisContext *connection;
    public:
        RedisClient() = delete;
        RedisClient(const string address, int port, const string passwd):
            address{address}, 
            port{port}, 
            passwd{passwd},
            connection{NULL} {};
        void wait();
        void connect();
        void set(const string& key, const string& val, int timeout = 0);
        void sadd(const string& key, const string& val);
        std::vector<string> smembers(const string& key);
        string get(const string& key);
        int get_int(const string& key) { 
            string val = get(key);
            if(val.empty()) return -1;
            else return stoi(val);
             };
        void close();
        static string unique_name(const string);
        ~RedisClient();
};
