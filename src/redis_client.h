#include <vector>
#include <string>
#include <hiredis/hiredis.h>

using std::string;
/**
 * @brief Implement some functions of redis 
 * 
 */
class RedisClient
{
private:
    string address;
    int port;
    string passwd;
    redisContext *connection;

public:
    RedisClient() = delete;
    RedisClient(const string address, int port,
                const string passwd) : address{address},
                                       port{port},
                                       passwd{passwd},
                                       connection{NULL} {};
    void wait(float = 1.0);
    void connect();

    // Redis related methods
    void set(const string &key, const string &val, int timeout = 0);
    void sadd(const string &key, const string &val);
    std::vector<string> smembers(const string &key);
    string get(const string &key);
    int get_int(const string &key);

    // Playlist related methods
    int get_last_segment(const string &name,
                         const string &profile);
    std::pair<int, int> get_last_partial_segment(const string &name,
                                                 const string &profile);
    void set_last_segment(const string &name,
                          const string &profile,
                          int segment_index);
    void set_last_partial_segment(const string &name,
                                  const string &profile,
                                  int segment_index,
                                  int psegment_index);
    void set_segment(const string &name,
                     const string &profile,
                     const string &segment_data,
                     int segment_index,
                     float segment_duration,
                     int timeout = 60);
    void set_partial_segment(const string &name,
                             const string &profile,
                             const string &psegment_data,
                             int segment_index,
                             int psegment_index,
                             float psegment_duration,
                             int timeout = 60);

    string playlist_item_segment(const string &name,
                                 const string &profile, int segment_id);
    string playlist_item_psegment(const string &name,
                                  const string &profile, std::pair<int, int> &);

    void close();
    static string unique_name(const string);
    ~RedisClient();
};
