#pragma once
#include <string>
#include <vector>
#include <boost/log/trivial.hpp>

#include "config.h"
#include "hls_transcode_profile.h"

class Hls_generator
{
    private:
        std::string input_name;
        std::string input_url;
        std::vector<Profile> outputs;
        
        float main_segment_len;
        float partial_segment_len;

    public:
        Hls_generator():
                input_name{""},
                input_url{""},
                main_segment_len{HLS_GENERATOR_SEGMENT_LEN},
                partial_segment_len{HLS_GENERATOR_PARTIAL_SEGMENT_LEN}{}
        Hls_generator(
                const std::string input_name,
                const std::string input_url,
                float main_segment_len = HLS_GENERATOR_SEGMENT_LEN,
                float partial_segment_len = HLS_GENERATOR_PARTIAL_SEGMENT_LEN):
                input_name{input_name},
                input_url{input_url},
                main_segment_len(main_segment_len),
                partial_segment_len{partial_segment_len}{}
        void init(
                const std::string _input_name,
                const std::string _input_url,
                float _main_segment_len = HLS_GENERATOR_SEGMENT_LEN,
                float _partial_segment_len = HLS_GENERATOR_PARTIAL_SEGMENT_LEN){
                input_url = _input_name;
                input_url = _input_url;
                main_segment_len = _main_segment_len;
                partial_segment_len = _partial_segment_len;}

        void add_output(const Profile&);
        void start();
        ~Hls_generator();
};
