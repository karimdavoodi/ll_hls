#include <thread>
#include <chrono>
#include <string>
#include <hiredis/hiredis.h>
#include <boost/log/trivial.hpp>
#include "config.h"
#include "redis_client.h"
 
using namespace std;

void RedisClient::connect()
{
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
void RedisClient::set(const string& key, const string& val)
{
    if(!connection) connect();
    redisReply *reply;
    while(true){
        reply = (redisReply*) redisCommand(connection,
                "SET %b %b", 
                key.data(), key.size(), 
                val.data(), val.size()); 
        if(!reply){
            connect();
            continue;
        } 
        LOG(info) << "SET (binary API): " << reply->str
            << " conn fd:" << connection->fd;
        freeReplyObject(reply);
        return;
    }
}
string RedisClient::get(const string& key)
{
    if(!connection) connect();
    redisReply *reply;
    string cmd = "GET " + key;
    while(true){
        reply = (redisReply*) redisCommand(connection, cmd.c_str()); 
        if(!reply){
            connect();
            continue;
        } 
        LOG(info) << "GET " << key << " len:" <<  reply->len 
            << " conn fd:" << connection->fd;
        string result;
        if(reply->len) 
            result.append(reply->str, reply->len);
        freeReplyObject(reply);
        return result;
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
