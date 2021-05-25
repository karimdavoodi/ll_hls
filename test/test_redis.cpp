#include <iostream>
#include <catch.hpp>
#include "../src/hls_transcode_profile.h"
#include "../src/redis_client.h"

using namespace std;

TEST_CASE("Test of  ... "){
        RedisClient redis("127.0.0.1", 6379, "31233123");
        for(auto i : {1,2,3,4,5,6} ){
                string name = "Chan " + to_string(i);
                redis.sadd("Lives_list", name);
                Profile p;
                p.setOriginal("1280x720", 3000000);
                redis.sadd("Live_master:"+name, p.to_string());
                p.setSD();
                redis.sadd("Live_master:"+name, p.to_string());
                p.setHD();
                redis.sadd("Live_master:"+name, p.to_string());
        }
}
