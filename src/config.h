#pragma once
#define HSL_SERVER_PORT 8088
#define HSL_SERVER_THREADS_NUM 6

#define HLS_GENERATOR_SEGMENT_LEN    6
#define HLS_GENERATOR_PARTIAL_SEGMENT_LEN    0.3

#define LOG(level) BOOST_LOG_TRIVIAL(level) << \
                "\033[0;32m[" << __func__ << ":" <<__LINE__ << "]\033[0m "
