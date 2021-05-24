#include <exception>
#include <iostream>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <served/methods.hpp>
#include <served/request.hpp>
#include <served/response.hpp>
#include <served/served.hpp>
#include <thread>
#include "config.h"
#include "hls_server.h"

using namespace std;

void Hls_server::init_routes()
{
    mux.use_before([&](served::response& , const served::request& req){
            LOG(debug) << served::method_to_string(req.method())
                       << ":" << req.url().URI(); 
                });
    mux.handle("/test").get([](served::response& res, const served::request&){
                    res << "TEST API\n";
                });
    mux.handle("/v1/cs/play/llhls/lives_list.m3u8").
        get([this](served::response& res, const served::request& req){
                return this->list_lives(res, req);
        });
    mux.handle("/v1/cs/play/llhls/master/{name}/play.m3u8").
        get([this](served::response& res, const served::request& req){
                return this->master_playlist(res, req);
        });
    mux.handle("/v1/cs/play/llhls/single/{name}/{profile}/play.m3u8").
        get([this](served::response& res, const served::request& req){
                return this->single_playlist(res, req);
        });
    mux.handle("/v1/cs/play/llhls/segment/{name}/{profile}/{segment}").
        get([this](served::response& res, const served::request& req){
                return this->segment(res, req);
        });
    mux.handle("/v1/cs/play/llhls/psegment/{name}/{profile}/{segment}").
        get([this](served::response& res, const served::request& req){
                return this->psegment(res, req);
        });
            
}

void Hls_server::start(const std::string address, int _port)
{
    port = _port;
    LOG(info) << "Listen on:" << port;
    served::net::server server(address, to_string(port), mux);
    server.run(HSL_SERVER_THREADS_NUM); 
}

Hls_server::~Hls_server()
{
    for(auto [id, redis] : redis_map){
        if(redis) redis->close();
    }
}
RedisClient* Hls_server::get_redis()
{
    RedisClient* redis;
    auto id = std::this_thread::get_id();

    if(redis_map.count(id) == 0){
        LOG(info) << "Create Redis client for thread " << id;
        redis = new RedisClient(redis_address, redis_port, redis_passwd);
        redis_map[id] = redis;
    }else{
        redis = redis_map[id];
    }
    return redis;
}

void Hls_server::list_lives(served::response& res, const served::request& req){
    //  /v1/cs/play/llhls/lives_list/play.m3u8
    LOG(info)   << " method:"<<  req.method() 
        << " thread:" << std::this_thread::get_id();

    get_redis()->set("key", "val");
    res << "playlist\n";
}
void Hls_server::master_playlist(served::response& res, const served::request& req){
    //  /v1/cs/play/llhls/master/{name}/play.m3u8
    LOG(info)   << " name:"<<  req.params.get("name");

    res << get_redis()->get("key");

}
void Hls_server::single_playlist(served::response& res, const served::request& req){
    //  /v1/cs/play/llhls/single/{name}/{profile}/play.m3u8
    LOG(info)   << " name:"<<  req.params.get("name")
        << " profile:"<<  req.params.get("profile");
    res << get_redis()->get("key");

}
void Hls_server::segment(served::response& res, const served::request& req){
    //  /v1/cs/play/llhls/segment/{name}/{profile}/{segment}
    LOG(info)   << " name:"<<  req.params.get("name")
        << " profile:"<<  req.params.get("profile")
        << " segment:"<<  req.params.get("segment");
    res << "playlist\n";

}
void Hls_server::psegment(served::response& res, const served::request& req){
    //  /v1/cs/play/llhls/psegment/{name}/{profile}/{segment}
    LOG(info)   << " name:"<<  req.params.get("name")
        << " profile:"<<  req.params.get("profile")
        << " segment:"<<  req.params.get("segment");
    res << "psegment\n";
}
