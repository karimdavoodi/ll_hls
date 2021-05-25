#include <complex>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <served/methods.hpp>
#include <served/request.hpp>
#include <served/response.hpp>
#include <served/served.hpp>
#include <thread>

#include "config.h"
#include "hls_server.h"
#include "hls_transcode_profile.h"

using namespace std;

/**
 * @brief Start http server for HLS
 * 
 * @param address : address to bind 
 * @param _port : port to bind
 */
void fill_redis();
void Hls_server::start(const std::string address, int _port)
{
#ifdef TEST_ONLY
    fill_redis();
#endif
    port = _port;

    if(redis_address.empty()){
        LOG(error) << "Please set Redis client info!";
        return;
    }
    LOG(info) << "Listen on:" << port;
    served::net::server server(address, to_string(port), mux);
    server.run(HSL_SERVER_THREADS_NUM); 
}

/**
 * @brief Define route for HLS server
 * Routes are:
 *   HLS_SERVER_API_BASE + "/lives_list.m3u8"
 *   HLS_SERVER_API_BASE + "/master/{name}/play.m3u8"
 *   HLS_SERVER_API_BASE + "/single/{name}/{profile}/play.m3u8"
 *   HLS_SERVER_API_BASE + "/segment/{name}/{profile}/{segment}"
 *   HLS_SERVER_API_BASE + "/psegment/{name}/{profile}/{segment}"
 * 
 */
void Hls_server::init_routes()
{
    mux.use_before([&](served::response& , const served::request& req){
            LOG(debug) << served::method_to_string(req.method())
                       << ":" << req.url().URI(); 
                });
    mux.handle("/test").get([](served::response& res, const served::request&){
                    res << "TEST API\n";
                });
    mux.handle(HLS_SERVER_API_BASE + "/lives_list.m3u8").
        get([this](served::response& res, const served::request& req){
                return this->list_lives(res, req);
        });
    mux.handle(HLS_SERVER_API_BASE + "/master/{name}/play.m3u8").
        get([this](served::response& res, const served::request& req){
                return this->master_playlist(res, req);
        });
    mux.handle(HLS_SERVER_API_BASE + "/single/{name}/{profile}/play.m3u8").
        get([this](served::response& res, const served::request& req){
                return this->single_playlist(res, req);
        });
    mux.handle(HLS_SERVER_API_BASE +"/segment/{name}/{profile}/{segment}").
        get([this](served::response& res, const served::request& req){
                return this->segment(res, req);
        });
    mux.handle(HLS_SERVER_API_BASE + "/psegment/{name}/{profile}/{segment}").
        get([this](served::response& res, const served::request& req){
                return this->psegment(res, req);
        });
}

/**
 * @brief Destroy the Hls_server::Hls_server object
 * 
 */
Hls_server::~Hls_server()
{
    // disconnect redis connections
    for(auto [id, redis] : redis_map){
        if(redis) redis->close();
    }
}

/**
 * @brief return Redis connection from map pool for this thread
 * 
 * @return RedisClient* : the pointer to redis client 
 */
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
////////////////  IMPLEMENT APIs /////////////////////////////////////////////

/**
 * @brief return playlist of all live streams in system
 * PATH: HLS_SERVER_API_BASE/lives_list/play.m3u8
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::list_lives(served::response& res, const served::request& req){
    auto lives_list = get_redis()->smembers("Lives_list");
    string playlist = "#EXTM3U\n#EXT-X-VERSION:3\n\n";
    for(auto live : lives_list){
        playlist += "#EXTINF:, " + live + "\n";
        playlist += HLS_SERVER_API_BASE + "/master/" + live + "/play.m3u8\n\n";
    }
    res << playlist;
}

/**
 * @brief return master playlist of live stream with name 'name'
 * PATH: HLS_SERVER_API_BASE/master/{name}/play.m3u8
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::master_playlist(served::response& res, const served::request& req){
    auto live_name = req.params.get("name");
    auto profile_list = get_redis()->smembers("Live_master:" + live_name);

    string playlist = "#EXTM3U\n#EXT-X-VERSION:3\n\n";
    for(auto profile_str : profile_list){
        Profile profile;
        profile.from_string(profile_str.c_str());
        LOG(debug) << "profile in set:" << profile_str
            << " to Profile:" << profile.to_string();
        playlist += "#EXT-X-STREAM-INF:" + profile.to_ext_x_stream_inf() + "\n";
        playlist += HLS_SERVER_API_BASE + "/single/" + live_name + "/" + 
                   to_string(profile.get_video_bitrate()) + 
                   "/play.m3u8\n\n";
    }
    res << playlist;
}

/**
 * @brief return single playlist of live stream with name 'name'
 * PATH: HLS_SERVER_API_BASE/single/{name}/{profile}/play.m3u8
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::single_playlist(served::response& res, const served::request& req){
    auto redis = get_redis();
    string name = req.params.get("name");
    string profile = req.params.get("profile");
    string key_last = "Live_segment:" + name + ":" + profile + ":last";
    int last = redis->get_int(key_last);
    string playlist;
    int i = last;
    for(; i>0 && i>(last-4); --i){
        string key_dur = "Live_segment_duration:" + name + ":" + profile + 
                    ":" + to_string(i);
        auto dur = redis->get(key_dur);
        playlist = "#EXTINF:" + dur + ",\n" + 
                HLS_SERVER_API_BASE + "/segment/" + name + "/" + 
                profile + "/" + to_string(i) + "\n" + 
                playlist;
    }
    playlist = string("#EXTM3U\n#EXT-X-VERSION:3\n\n") +
               "#EXT-X-MEDIA-SEQUENCE:" + to_string(i) +
               "\n#EXT-X-TARGETDURATION:10" +
               "\n#EXT-X-PLAYLIST-TYPE:EVENT\n\n" +
               playlist;
    res << playlist;
}

/**
 * @brief return one segment of live stream with name 'name' and profile 'profile'
 * PATH: HLS_SERVER_API_BASE/segment/{name}/{profile}/{segment}
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::segment(served::response& res, const served::request& req){
    string name = req.params.get("name");
    string profile = req.params.get("profile");
    string segment = req.params.get("segment");
    string key_data = "Live_segment_data:" + name + ":" + profile +
                     ":" + segment;
    string data = get_redis()->get(key_data);
    res.set_body(data);
    res.set_header("Content-type", "video/MP2T");
    res.set_status(200);
}

/**
 * @brief return one partial segment of live stream with name 'name' and profile 'profile'
 * PATH: HLS_SERVER_API_BASE/psegment/{name}/{profile}/{segment}
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::psegment(served::response& res, const served::request& req){
    LOG(info)   << " name:"<<  req.params.get("name")
        << " profile:"<<  req.params.get("profile")
        << " segment:"<<  req.params.get("segment");
    res << "psegment\n";
    // TODO: implement....
}
void fill_redis()
{
    string key_last;
    ifstream segfile("/home/karim/Videos/20.mp4", std::ios::binary);
    string segment(
        std::istreambuf_iterator<char>(segfile), {});
    LOG(debug) << "seg data len " << segment.size();
    RedisClient redis = RedisClient{"127.0.0.1", 6379, "31233123"};
    auto P = [&](string name, Profile& p){
        string key_last;
        redis.sadd("Live_master:" + name, p.to_string());
        key_last = "Live_segment:" + name + ":" + p.get_id() + ":last";
        redis.set(key_last, "9");
        for(int j : {1,2,3,4,5,6,7,8,9}) {
            string key_seg = "Live_segment_data:" + name + ":" + 
                                p.get_id() + ":" + to_string(j);
            redis.set(key_seg, segment, 60);
            string key_dur = "Live_segment_duration:" + name + ":" + 
                            p.get_id() + ":" + to_string(j);
            redis.set(key_dur, "5.3", 60);
        }
    };
    for (auto i : {1, 2, 3, 4, 5, 6})
    {
        string name = RedisClient::unique_name("Chan " + to_string(i));
        LOG(info) << "Add " << name;
        redis.sadd("Lives_list", name);
        Profile p;
        p.setOriginal("1280x720", 3000000);
        P(name, p);
        p.setSD();
        P(name, p);
        p.setHD();
        P(name, p);
    }
}
