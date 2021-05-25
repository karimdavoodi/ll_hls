#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <hiredis/hiredis.h>
#include <boost/log/trivial.hpp>
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
    while(true){
        connection = redisConnectWithTimeout(address.c_str(), port, timeout);
        if(connection == NULL || connection->err){
            LOG(error) << "Can't connect to redis " << address << ". try again!";
            if(connection) redisFree(connection);
            wait();
            continue;
        }
        break;
    }
    if(!passwd.empty()){
        redisReply *reply;
        reply = (redisReply *)redisCommand(connection, "AUTH %s", passwd.c_str());
        if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
            LOG(error) << "Redis Authentication feild!";
        }
        if(reply) freeReplyObject(reply);
    }
}
void RedisClient::wait()
{ 
    std::this_thread::sleep_for(1s); 
}
/**
 * @brief add member to set
 * 
 * @param key 
 * @param val 
 */
void RedisClient::sadd(const string& key, const string& val)
{
    if(!connection) connect();
    redisReply *reply;
    while(true){
        reply = (redisReply*) redisCommand(connection,
                "SADD %s %s", key.c_str(), val.c_str()); 
        if(!reply){
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
vector<string> RedisClient::smembers(const string& key)
{
    if(!connection) connect();
    redisReply *reply;
    while(true){
        reply = (redisReply*) redisCommand(connection, "SMEMBERS %s", key.c_str() ); 
        if(!reply){
            connect();
            continue;
        } 
        LOG(info) << "SMEMBERS " << key << " elements:" <<  reply->elements
            << " conn fd:" << connection->fd;
        vector<string> res;
        for(size_t i=0; i< reply->elements; ++i){
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
void RedisClient::set(const string& key, const string& val, int timeout)
{
    if(!connection) connect();
    redisReply *reply;
    while(true){
        if(timeout != 0)
            reply = (redisReply*) redisCommand(connection,
                "SET %b %b EX %d", 
                key.data(), key.size(), 
                val.data(), val.size(), timeout); 
        else
            reply = (redisReply*) redisCommand(connection,
                "SET %b %b", 
                key.data(), key.size(), 
                val.data(), val.size()); 
        if(!reply){
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
string RedisClient::get(const string& key)
{
    if(!connection) connect();
    redisReply *reply;
    while(true){
        reply = (redisReply*) redisCommand(connection, "GET %s", key.c_str() ); 
        if(!reply){
            connect();
            continue;
        } 
        LOG(info) << "GET " << key << " len:" <<  reply->len 
            << " conn fd:" << connection->fd;
        string res;
        if(reply->len) 
            res.append(reply->str, reply->len);
        freeReplyObject(reply);
        return res;
    }
}
void RedisClient::close()
{
    if(connection != NULL)
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