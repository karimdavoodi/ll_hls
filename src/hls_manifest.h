#pragma once
#include <vector>
#include <string>
#include <hiredis/hiredis.h>

using std::string;
class RedisClient;

class Hls_manifest {
    private:
        RedisClient* redis;
    public:
        Hls_manifest() = delete;
        Hls_manifest(RedisClient* r):redis(r){}

        int get_last_segment(const string &name,
                const string &profile);
        string get_segment_time(const string &name,
                const string &profile,
                int segment_id);
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
};
