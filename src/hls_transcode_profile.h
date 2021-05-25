#pragma once
#include <string>
#include <cstdio>
#include <vector>
using std::string;

/**
 * @brief Profile class to implement adaptive bit rate
 * 
 */
class Profile {
    private:
        bool transcode = false;
        string video_codec = "copy";
        string video_resolution = "1280x720";
        int video_bitrate = 3000000;
        int video_fps = 24;
        string audio_codec = "copy";
        int audio_bitrate = 128000;
        int audio_channel = 2;

    public:
        Profile() {}
        Profile(
                bool transcode,
                string video_codec,
                string video_resolution,
                int video_bitrate,
                int video_fps,
                string audio_codec,
                int audio_bitrate,
                int audio_channel) : transcode{transcode},
            video_codec{video_codec},
            video_resolution{video_resolution},
            video_bitrate{video_bitrate},
            video_fps{video_fps},
            audio_codec{audio_codec},
            audio_bitrate{audio_bitrate},
            audio_channel{audio_channel} {}
        void setProfile(
                bool _transcode,
                string _video_codec,
                string _video_resolution,
                int _video_bitrate,
                int _video_fps,
                string _audio_codec,
                int _audio_bitrate,
                int _audio_channels)
        {
            transcode = _transcode;
            video_codec = _video_codec;
            video_resolution = _video_resolution;
            video_bitrate = _video_bitrate;
            video_fps = _video_fps;
            audio_codec = _audio_codec;
            audio_bitrate = _audio_bitrate;
            audio_channel = _audio_channels;
        }
        void setDefault()
        {
            transcode = true;
            video_codec = "libx264";
            video_fps = 24;
            audio_codec = "aac";
            audio_bitrate = 128000;
            audio_channel = 2;
        }
        void setSD()
        {
            setDefault();
            video_resolution = "720x432";
            video_bitrate = 1500000;
        }
        void setHD()
        {
            setDefault();
            video_resolution = "1280x720";
            video_bitrate = 3500000;
        }
        void setFHD()
        {
            setDefault();
            video_resolution = "1920x1080";
            video_bitrate = 5000000;
        }
        void setOriginal(const string resolution, int bitrate)
        {
            transcode = false;
            video_resolution = resolution;
            video_bitrate = bitrate;
            video_codec = "copy";
            audio_codec = "copy";
        }
        string to_string()
        {
            char str[512];
            std::sprintf(str, "%s %s %d %d %s %d %d",
                    video_codec.c_str(),
                    video_resolution.c_str(),
                    video_bitrate,
                    video_fps,
                    audio_codec.c_str(),
                    audio_bitrate,
                    audio_channel);
            return string(str);
        }
        void from_string(const char *str)
        {
            char v_codec[64], v_resolution[64], a_codec[64];
            std::sscanf(str, "%s %s %d %d %s %d %d",
                    v_codec,
                    v_resolution,
                    &video_bitrate,
                    &video_fps,
                    a_codec,
                    &audio_bitrate,
                    &audio_channel);
            video_resolution = v_resolution;
            video_codec = v_codec;
            audio_codec = a_codec;
        }
        string to_ext_x_stream_inf()
        {
            char str[512];
            if(video_resolution.empty())
                std::sprintf(str, "BANDWIDTH=%d", video_bitrate);
            else
                std::sprintf(str, "BANDWIDTH=%d,RESOLUTION='%s'",
                        video_bitrate, video_resolution.c_str());
            return string(str);

        }
        int get_video_fps() const { return video_fps; }
        int get_video_bitrate() const { return video_bitrate; }
        string get_video_resolution() const { return video_resolution; }
        string get_video_codec() const { return video_codec; }
        string get_audio_codec() const { return audio_codec; }
        int get_audio_bitrate() const { return audio_bitrate; }
        int get_audio_channel() const { return audio_channel; }
        string get_id() const { return std::to_string(video_bitrate); }

        void set_video_fps(int fps) { video_fps = fps; }
        void set_video_bitrate(int bitrate) { video_bitrate = bitrate; }
        void set_video_resolution(string resolution) { video_resolution = resolution; }
        void set_video_codec(string codec) { video_codec = codec; }
        void set_audio_codec(string codec) { audio_codec = codec; }
        void set_audio_bitrate(int bitrate) { audio_bitrate = bitrate; }
        void set_audio_channel(int channel) { audio_channel = channel; }
};
