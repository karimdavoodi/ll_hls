#include <complex>
#include <exception>
#include <iostream>
#include <fstream>
#include <deque>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <served/methods.hpp>
#include <served/request.hpp>
#include <served/response.hpp>
#include <served/served.hpp>
#include <boost/format.hpp>
#include <thread>

#include "config.h"
#include "util.h"
#include "hls_server.h"
#include "hls_manifest.h"
#include "hls_transcode_profile.h"

using namespace std;

/**
 * @brief Start http server for HLS
 * 
 * @param address : address to bind 
 * @param _port : port to bind
 */
void Hls_server::start(const std::string address, int _port)
{
#ifdef TEST_ONLY
    void fill_redis();
    fill_redis();
#endif
    port = _port;

    if (redis_address.empty())
    {
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
 *   HLS_SERVER_API_BASE + "/psegment/{name}/{profile}/{segment}/{psegment}"
 * 
 */
void Hls_server::init_routes()
{
    mux.use_before([&](served::response &, const served::request &req)
                   { LOG(debug) << served::method_to_string(req.method())
                                << ":" << req.url().URI(); });
    mux.handle("/test")
        .get([](served::response &res, const served::request &)
             { res << "TEST API\n"; });
    mux.handle(HLS_SERVER_API_BASE + "/lives_list.m3u8")
        .get([this](served::response &res, const served::request &req)
             { return this->list_lives(res, req); });
    mux.handle(HLS_SERVER_API_BASE + "/master/{name}/play.m3u8")
        .get([this](served::response &res, const served::request &req)
             { return this->master_playlist(res, req); });
    mux.handle(HLS_SERVER_API_BASE + "/single/{name}/{profile}/play.m3u8")
        .get([this](served::response &res, const served::request &req)
             { return this->single_playlist(res, req); });
    mux.handle(HLS_SERVER_API_BASE + "/segment/{name}/{profile}/{segment}")
        .get([this](served::response &res, const served::request &req)
             { return this->segment(res, req); });
    mux.handle(HLS_SERVER_API_BASE + "/psegment/{name}/{profile}/{segment}/{psegment}")
        .get([this](served::response &res, const served::request &req)
             { return this->psegment(res, req); });
}

/**
 * @brief Destroy the Hls_server::Hls_server object
 * 
 */
Hls_server::~Hls_server()
{
    // disconnect redis connections
    for (auto &item : redis_map)
    {
        if (item.second)
            item.second->close();
    }
}

/**
 * @brief return Redis connection from map pool for this thread
 * 
 * @return RedisClient* : the pointer to redis client 
 */
RedisClient *Hls_server::get_redis()
{
    RedisClient *redis;
    auto id = std::this_thread::get_id();

    if (redis_map.count(id) == 0)
    {
        LOG(info) << "Create Redis client for thread " << id;
        redis = new RedisClient(redis_address, redis_port, redis_passwd);
        redis_map[id] = redis;
    }
    else
    {
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
void Hls_server::list_lives(served::response &res, const served::request &req)
{
    auto lives_list = get_redis()->smembers("Lives_list");
    string playlist = "#EXTM3U\n#EXT-X-VERSION:3\n\n";
    for (auto live : lives_list)
    {
        playlist += (boost::format("#EXTINF:,%s\n") % live).str();
        playlist += (boost::format("%s/master/%s/play.m3u8\n\n") % HLS_SERVER_API_BASE % live).str();
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
void Hls_server::master_playlist(served::response &res, const served::request &req)
{
    auto live_name = req.params.get("name");
    auto profile_list = get_redis()->smembers("Live_master:" + live_name);

    string playlist = "#EXTM3U\n#EXT-X-VERSION:3\n\n";
    for (auto profile_str : profile_list)
    {
        Profile profile;
        profile.from_string(profile_str.c_str());
        playlist += (boost::format("#EXT-X-STREAM-INF:%s\n") % profile.to_ext_x_stream_inf()).str();
        playlist += (boost::format("%s/single/%s/%d/play.m3u8\n\n") % HLS_SERVER_API_BASE % live_name % profile.get_video_bitrate()).str();
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
void Hls_server::single_playlist(served::response &res, const served::request &req)
{
    RedisClient* redis = get_redis();
    Hls_manifest manifest{redis};
    string name = req.params.get("name");
    string profile = req.params.get("profile");

    // find last segment and partial segment
    auto last_seg = manifest.get_last_segment(name, profile);
    auto last_pseg = manifest.get_last_partial_segment(name, profile);
    if (last_seg < 0 || last_pseg.first < 0 || last_pseg.second < 0)
    {
        LOG(error) << "Can't get last segment numbers from Redis for " << name;
        res << "Can't get last segment numbers from Redis for " << name;
        return;
    }

    // Wait if requested segment is not ready!!!
    int hls_msn = Util::to_int(req.query.get("_HLS_msn"));
    int hls_part = Util::to_int(req.query.get("_HLS_part"));
    if (hls_msn >= 0)
    {
        // TODO: implement in efficatin way!
        // TODO: wait only to next segment
        if (hls_msn == last_seg + 1)
        {
            while (true)
                if (hls_msn > manifest.get_last_segment(name, profile))
                    Util::wait_millisecond(100);
                else
                    break;
        }
    }

    // make playlist
    deque<string> playlist;
    int index = last_seg;
    // insert last segment if it is newer than partial segments
    if (last_seg >= last_pseg.first)
    {
        playlist.push_front(manifest.playlist_item_segment(name, profile, last_seg));
        --index;
    }
    // insert partial segments of last segment
    for (; last_pseg.second >= 0; --last_pseg.second)
    {
        playlist.push_front(manifest.playlist_item_psegment(name, profile, last_pseg));
    }

    // insert remain 4 main segments if hls_skip is not set
    string hls_skip = req.query.get("_HLS_skip");
    if (hls_skip.empty())
    {
        for (; index > 0 && index > (last_seg - 4); --index)
        {
            playlist.push_front(manifest.playlist_item_segment(name, profile, index));
        }
    }
    string first_segment_time = manifest.get_segment_time(name, profile, index);
    // Gather playlist ...
    string playlist_;
    for (auto s : playlist)
        playlist_ += s;
    res << "#EXTM3U\n"
        << "#EXT-X-VERSION:6\n\n"
        << "#EXT-X-MEDIA-SEQUENCE:" << to_string(index) << "\n"
        // TODO: need to implement
        //  << "#EXT-X-SERVER-CONTROL:CAN-BLOCK-RELOAD=YES,CAN-SKIP-UNTIL=12.0,HOLD-BACK=3.0\n"
        //  << "#EXT-X-PART-INF:PART-TARGET=0.33334\n"
        << "#EXT-X-PROGRAM-DATE-TIME:" << first_segment_time << "\n"
        << "#EXT-X-TARGETDURATION:10\n"
        << "#EXT-X-PLAYLIST-TYPE:EVENT\n\n"
        << playlist_;
}

/**
 * @brief return one segment of live stream with name 'name' and profile 'profile'
 * PATH: HLS_SERVER_API_BASE/segment/{name}/{profile}/{segment}
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::segment(served::response &res, const served::request &req)
{
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
 * PATH: HLS_SERVER_API_BASE/psegment/{name}/{profile}/{segment}/{psegment}
 * 
 * @param res : response object of http connection 
 * @param req : request object of http connection
 */
void Hls_server::psegment(served::response &res, const served::request &req)
{
    string name = req.params.get("name");
    string profile = req.params.get("profile");
    string segment = req.params.get("segment");
    string psegment = req.params.get("psegment");
    string key_data = "Live_segment_data:" + name + ":" + profile +
                      ":" + segment + ":" + psegment;
    string data = get_redis()->get(key_data);
    res.set_body(data);
    res.set_header("Content-type", "video/MP2T");
    res.set_status(200);
}
#ifdef TEST_ONLY
void fill_redis()
{
    string key_last;
    ifstream segfile("/home/karim/Videos/20.mp4", std::ios::binary);
    string segment(
        std::istreambuf_iterator<char>(segfile), {});
    LOG(debug) << "seg data len " << segment.size();
    RedisClient redis = RedisClient{"127.0.0.1", 6379, "31233123"};
    Hls_manifest manifest {&redis} ;
    auto P = [&](string name, Profile &p)
    {
        string key_last;
        redis.sadd("Live_master:" + name, p.to_string());
        for (int j : {1, 2, 3, 4, 5, 6, 7, 8, 9})
        {
            manifest.set_segment(name, p.get_id(), segment, j, 5.3);
            for (size_t k : {0, 1, 2, 3, 4})
            {
                string seg(segment.begin() + k * segment.size() / 5,
                           segment.begin() + (k + 1) * segment.size() / 5);
                manifest.set_partial_segment(name, p.get_id(), seg, j, k, 0.8);
            }
            manifest.set_last_partial_segment(name, p.get_id(), j, 4);
        }
        manifest.set_last_segment(name, p.get_id(), 9);
    };
    for (auto i : {1, 2, 3, 4, 5, 6})
    {
        string name = Util::unique_name("Chan " + to_string(i));
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
#endif
