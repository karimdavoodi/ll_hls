#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include "config.h"
#include "hls_manifest.h"
#include "redis_client.h"
/**
 * @brief return last segment number of live
 * 
 * @param name 
 * @param profile 
 * @return int 
 */
int Hls_manifest::get_last_segment(
    const string &name,
    const string &profile)
{
    string key_last;
    key_last = (boost::format("Live_segment:%s:%s:last_seg") % name % profile).str();
    int last_seg = redis->get_int(key_last);
    if (last_seg < 0)
    {
        LOG(error) << "Can't get last segment number for " << name;
        return -1;
    }
    return last_seg;
}
/**
 * @brief retutn last partial segment number of live
 * 
 * @param name 
 * @param profile 
 * @return std::pair<int, int> 
 */
std::pair<int, int> Hls_manifest::get_last_partial_segment(
    const string &name,
    const string &profile)
{
    string key_last = (boost::format("Live_segment:%s:%s:last_pseg") % name % profile).str();
    string last_p = redis->get(key_last);
    auto it = last_p.find(':');
    auto last_pseg = std::make_pair(-1, -1);
    if (it != string::npos)
    {
        last_pseg.first = stoi(last_p.substr(0, it));
        last_pseg.second = stoi(last_p.substr(it + 1));
    }
    return last_pseg;
}
/**
 * @brief generate laylist item for segment
 * 
 * @param name 
 * @param profile 
 * @param segment_id 
 * @return string 
 */
string Hls_manifest::playlist_item_segment(
    const string &name,
    const string &profile,
    int segment_id)
{
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d") % name % profile % segment_id).str();
    string duration = redis->get(key_dur);
    return (boost::format("#EXTINF:%s,\n") % duration).str() +
           (boost::format("%s/segment/%s/%s/%d\n") % HLS_SERVER_API_BASE % name % profile % segment_id).str();
}
/**
 * @brief generate playlist line for partial segment
 * 
 * @param name 
 * @param profile 
 * @param psegment_id 
 * @return string 
 */
string Hls_manifest::playlist_item_psegment(
    const string &name,
    const string &profile,
    std::pair<int, int> &psegment_id)
{
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d:%d") % name % profile % psegment_id.first % psegment_id.second).str();
    string duration = redis->get(key_dur);
    return (boost::format("#EXT-X-PART:DURATION=%d,URI=\"%s/%s/%s/%d/%d\"\n") % duration % HLS_SERVER_API_BASE % name % profile % psegment_id.first % psegment_id.second).str();
}
/**
 * @brief set last segment number
 * 
 * @param name 
 * @param profile 
 * @param segment_index 
 */
void Hls_manifest::set_last_segment(const string &name,
                                   const string &profile,
                                   int segment_index)
{
    string key_last = (boost::format("Live_segment:%s:%s:last_seg") % name % profile).str();
    redis->set(key_last, std::to_string(segment_index));
}
/**
 * @brief return segment date time
 * 
 * @param name 
 * @param profile 
 * @param segment_index 
 * @return string 
 */
string Hls_manifest::get_segment_time(const string &name,
                                     const string &profile,
                                     int segment_index)
{
    string key = (boost::format("Live_segment_datetime:%s:%s:%d") % name % profile % segment_index).str();
    string time_epotch = redis->get(key);
    const time_t time_e = std::stoull(time_epotch);
    char buf[512];
    strftime(buf, 512, "%Y-%m-%dT%H:%M:%SZ", gmtime(&time_e));
    return string(buf);
}

/**
 * @brief set last partial segment number
 * 
 * @param name 
 * @param profile 
 * @param segment_index 
 * @param psegment_index 
 */
void Hls_manifest::set_last_partial_segment(const string &name,
                                           const string &profile,
                                           int segment_index,
                                           int psegment_index)
{
    string key_last = (boost::format("Live_segment:%s:%s:last_pseg") % name % profile).str();
    redis->set(key_last, std::to_string(segment_index) + ":" + std::to_string(psegment_index));
}
/**
 * @brief set segment data and duration in Redis
 * 
 * @param name 
 * @param profile 
 * @param segment_data 
 * @param segment_index 
 * @param segment_duration 
 * @param timeout 
 */
void Hls_manifest::set_segment(const string &name,
                              const string &profile,
                              const string &segment_data,
                              int segment_index,
                              float segment_duration,
                              int timeout)
{
    string key_seg = (boost::format("Live_segment_data:%s:%s:%d") % name % profile % segment_index).str();
    redis->set(key_seg, segment_data, timeout);
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d") % name % profile % segment_index).str();
    redis->set(key_dur, std::to_string(segment_duration), timeout);
}
/**
 * @brief set partial segment data and duration in Redis
 * 
 * @param name 
 * @param profile 
 * @param psegment_data 
 * @param segment_index 
 * @param psegment_index 
 * @param psegment_duration 
 * @param timeout 
 */
void Hls_manifest::set_partial_segment(const string &name,
                                      const string &profile,
                                      const string &psegment_data,
                                      int segment_index,
                                      int psegment_index,
                                      float psegment_duration,
                                      int timeout)
{
    string key_seg = (boost::format("Live_segment_data:%s:%s:%d:%d") %
                      name % profile % segment_index % psegment_index)
                         .str();
    redis->set(key_seg, psegment_data, timeout);
    string key_dur = (boost::format("Live_segment_duration:%s:%s:%d:%d") %
                      name % profile % segment_index % psegment_index)
                         .str();
    redis->set(key_dur, std::to_string(psegment_duration), timeout);
}
