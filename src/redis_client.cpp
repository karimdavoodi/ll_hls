#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <hiredis/hiredis.h>
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include "config.h"
#include "redis_client.h"

using namespace std;
/**
 * @brief connect to redis server and authenticat connection
 * 
 */
void RedisClient::connect()
{
    if (connection)
        redisFree(connection);
    LOG(info) << "Try to connect to Redis server " << address;
    struct timeval timeout = {3, 0};
    while (true)
    {
        connection = redisConnectWithTimeout(address.c_str(), port, timeout);
        if (connection == NULL || connection->err)
        {
            LOG(error) << "Can't connect to redis " << address << ". try again!";
            if (connection)
                redisFree(connection);
            wait();
            continue;
        }
        break;
    }
    if (!passwd.empty())
    {
        redisReply *reply;
        reply = (redisReply *)redisCommand(connection, "AUTH %s", passwd.c_str());
        if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
        {
            LOG(error) << "Redis Authentication feild!";
        }
        if (reply)
            freeReplyObject(reply);
    }
}
void RedisClient::wait(float wait)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(int(wait * 1000)));
}
/**
 * @brief add member to set
 * 
 * @param key 
 * @param val 
 */
void RedisClient::sadd(const string &key, const string &val)
{
    if (!connection)
        connect();
    redisReply *reply;
    while (true)
    {
        reply = (redisReply *)redisCommand(connection,
                                           "SADD %s %s", key.c_str(), val.c_str());
        if (!reply)
        {
            connect();
            continue;
        }
        LOG(debug) << "SADD ret: " << reply->len
                   << " conn fd:" << connection->fd;
        freeReplyObject(reply);
        return;
    }
}
/**
 * @brief return members of set as vector of string
 * 
 * @param key 
 * @return vector<string> 
 */
vector<string> RedisClient::smembers(const string &key)
{
    if (!connection)
        connect();
    redisReply *reply;
    while (true)
    {
        reply = (redisReply *)redisCommand(connection, "SMEMBERS %s", key.c_str());
        if (!reply)
        {
            connect();
            continue;
        }
        LOG(info) << "SMEMBERS " << key << " elements:" << reply->elements
                  << " conn fd:" << connection->fd;
        vector<string> res;
        for (size_t i = 0; i < reply->elements; ++i)
        {
            redisReply *rep = reply->element[i];
            res.push_back(string(rep->str, rep->len));
        }
        freeReplyObject(reply);
        return res;
    }
}
/**
 * @brief set key:val 
 * 
 * @param key 
 * @param val 
 * @param timeout : in second, not set if timeout == 0 
 */
void RedisClient::set(const string &key, const string &val, int timeout)
{
    if (!connection)
        connect();
    redisReply *reply;
    while (true)
    {
        if (timeout != 0)
            reply = (redisReply *)redisCommand(connection,
                                               "SET %b %b EX %d",
                                               key.data(), key.size(),
                                               val.data(), val.size(), timeout);
        else
            reply = (redisReply *)redisCommand(connection,
                                               "SET %b %b",
                                               key.data(), key.size(),
                                               val.data(), val.size());
        if (!reply)
        {
            connect();
            continue;
        }
        LOG(info) << "SET key: " << key
                  << " val len:" << val.size()
                  << " conn fd:" << connection->fd;
        freeReplyObject(reply);
        return;
    }
}
/**
 * @brief return value of key
 * 
 * @param key 
 * @return string 
 */
string RedisClient::get(const string &key)
{
    if (!connection)
        connect();
    redisReply *reply;
    while (true)
    {
        reply = (redisReply *)redisCommand(connection, "GET %s", key.c_str());
        if (!reply)
        {
            connect();
            continue;
        }
        LOG(info) << "GET " << key << " len:" << reply->len
                  << " conn fd:" << connection->fd;
        string res;
        if (reply->len)
            res.append(reply->str, reply->len);
        freeReplyObject(reply);
        return res;
    }
}
void RedisClient::close()
{
    if (connection != NULL)
        redisFree(connection);
}
RedisClient::~RedisClient()
{
    close();
}
/**
 * @brief convert live stream names to ascii 
 * 
 * @param original_name 
 * @return string 
 */
string RedisClient::unique_name(const string original_name)
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
/**
 * @brief return int value of key
 * 
 * @param key 
 * @return int 
 */
int RedisClient::get_int(const string &key)
{
    string val = get(key);
    if (val.empty())
        return -1;
    else
        return stoi(val);
};
/**
 * @brief return last segment number of live
 * 
 * @param name 
 * @param profile 
 * @return int 
 */
int RedisClient::get_last_segment(
    const string &name,
    const string &profile)
{
    string key_last;
    key_last = (boost::format("Live_segment:%s:%s:last_seg") % name % profile).str();
    int last_seg = get_int(key_last);
    if (last_seg < 0)
    {
        LOG(error) << "Can't get last segment number for " << name;
        return -1;
    }
    return last_seg;
}
/**
 * @brief retutn last partial segment number of live
 * 
 * @param name 
 * @param profile 
 * @return std::pair<int, int> 
 */
std::pair<int, int> RedisClient::get_last_partial_segment(
    const string &name,
    const string &profile)
{
    string key_last = (boost::format("Live_segment:%s:%s:last_pseg") % name % profile).str();
    string last_p = get(key_last);
    auto it = last_p.find(':');
    auto last_pseg = make_pair(-1, -1);
    if (it != string::npos)
    {
        last_pseg.first = stoi(last_p.substr(0, it));
        last_pseg.second = stoi(last_p.substr(it + 1));
    }
    return last_pseg;
}
/**
 * @brief generate laylist item for segment
 * 
 * @param name 
 * @param profile 
 * @param segment_id 
 * @return string 
 */
string RedisClient::playlist_item_segment(
    const string &name,
    const string &profile,
    int segment_id)
{
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d") % name % profile % segment_id).str();
    string duration = get(key_dur);
    return (boost::format("#EXTINF:%s,\n") % duration).str() +
           (boost::format("%s/segment/%s/%s/%d\n") % HLS_SERVER_API_BASE % name % profile % segment_id).str();
}
/**
 * @brief generate playlist line for partial segment
 * 
 * @param name 
 * @param profile 
 * @param psegment_id 
 * @return string 
 */
string RedisClient::playlist_item_psegment(
    const string &name,
    const string &profile,
    pair<int, int> &psegment_id)
{
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d:%d") % name % profile % psegment_id.first % psegment_id.second).str();
    string duration = get(key_dur);
    return (boost::format("#EXT-X-PART:DURATION=%d,URI=\"%s/%s/%s/%d/%d\"\n") % duration % HLS_SERVER_API_BASE % name % profile % psegment_id.first % psegment_id.second).str();
}
/**
 * @brief set last segment number
 * 
 * @param name 
 * @param profile 
 * @param segment_index 
 */
void RedisClient::set_last_segment(const string &name,
                                   const string &profile,
                                   int segment_index)
{
    string key_last = (boost::format("Live_segment:%s:%s:last_seg") % name % profile).str();
    set(key_last, to_string(segment_index));
}
/**
 * @brief set last partial segment number
 * 
 * @param name 
 * @param profile 
 * @param segment_index 
 * @param psegment_index 
 */
void RedisClient::set_last_partial_segment(const string &name,
                                           const string &profile,
                                           int segment_index,
                                           int psegment_index)
{
    string key_last = (boost::format("Live_segment:%s:%s:last_pseg") % name % profile).str();
    set(key_last, to_string(segment_index) + ":" + to_string(psegment_index));
}
/**
 * @brief set segment data and duration in Redis
 * 
 * @param name 
 * @param profile 
 * @param segment_data 
 * @param segment_index 
 * @param segment_duration 
 * @param timeout 
 */
void RedisClient::set_segment(const string &name,
                              const string &profile,
                              const string &segment_data,
                              int segment_index,
                              float segment_duration,
                              int timeout)
{
    string key_seg = (boost::format("Live_segment_data:%s:%s:%d") % name % profile % segment_index).str();
    set(key_seg, segment_data, timeout);
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d") % name % profile % segment_index).str();
    set(key_dur, to_string(segment_duration), timeout);
}
/**
 * @brief set partial segment data and duration in Redis
 * 
 * @param name 
 * @param profile 
 * @param psegment_data 
 * @param segment_index 
 * @param psegment_index 
 * @param psegment_duration 
 * @param timeout 
 */
void RedisClient::set_partial_segment(const string &name,
                                      const string &profile,
                                      const string &psegment_data,
                                      int segment_index,
                                      int psegment_index,
                                      float psegment_duration,
                                      int timeout)
{
    string key_seg = (boost::format("Live_segment_data:%s:%s:%d:%d") %
                      name % profile % segment_index % psegment_index)
                         .str();
    set(key_seg, psegment_data, timeout);
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d:%d") %
                      name % profile % segment_index % psegment_index)
                         .str();
    set(key_dur, to_string(psegment_duration), timeout);
}
