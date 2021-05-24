#pragma once
#include <string>
#include <vector>

struct Profile {
    bool transcode;
    std::string video_codec;
    std::string video_resolution;
    float video_bitrate;
    float video_fps;
    std::string audoi_codec;
    float audoi_bitrate;
    float audoi_channels;

    Profile():transcode(false),video_codec{"copy"},audoi_codec{"copy"}{}
    Profile(
        bool transcode,
        std::string video_codec,
        std::string video_resolution,
        float video_bitrate,
        float video_fps,
        std::string audoi_codec,
        float audoi_bitrate,
        float audoi_channels):
            transcode{transcode},
            video_codec{video_codec},
            video_resolution{video_resolution},
            video_bitrate{video_bitrate},
            video_fps{video_fps},
            audoi_codec{audoi_codec},
            audoi_bitrate{audoi_bitrate},
            audoi_channels{audoi_channels}{}
    void setProfile(
        bool _transcode,
        std::string _video_codec,
        std::string _video_resolution,
        float _video_bitrate,
        float _video_fps,
        std::string _audoi_codec,
        float _audoi_bitrate,
        float _audoi_channels){
            transcode = _transcode;
            video_codec = _video_codec;
            video_resolution = _video_resolution;
            video_bitrate = _video_bitrate;
            video_fps = _video_fps;
            audoi_codec = _audoi_codec;
            audoi_bitrate = _audoi_bitrate;
            audoi_channels = _audoi_channels; 
    }
    void setDefault(){
        transcode = true;
        video_codec = "libx264";
        video_fps = 24;
        audoi_codec = "aac";
        audoi_bitrate = 128000;
        audoi_channels = 2; 
    }
    void setSD(){
        setDefault();
        video_resolution = "720x432";
        video_bitrate = 1500000;
    }
    void setHD(){
        setDefault();
        video_resolution = "1280x720";
        video_bitrate = 3500000;
    }
    void setFHD(){
        setDefault();
        video_resolution = "1920x1080";
        video_bitrate = 5000000;
    }

};
