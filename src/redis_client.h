#include <string>
#include <hiredis/hiredis.h>

using std::string;

class RedisClient{
    private:
        string address;
        int port; 
        string passwd;

        redisContext *connection;
        redisReply *reply;
    public:
        RedisClient(){}
        RedisClient(const string address, int port, const string passwd):
            address{address}, port{port}, passwd{passwd} {};
        void wait();
        void connect();
        void set(const string& key, const string& val);
        string get(const string& key);
        void close();
        ~RedisClient();
};
