# TODO: under develope ...

# ll_hls
Cloud-Base Low-latency HLS

    input --> HLS Generator cluster ----> Redis cluster ---> HLS server cluster
## Dependency
    - C++ wrapper for ffmpeg:  https://github.com/h4tr3d/avcpp.git 
    - C++ REST API library: https://github.com/meltwater/served.git 
    - Redis client library: https://github.com/redis/hiredis.git
## Usage
### API
    Get playlist of all live streams in system:
    curl http://ip:port/v1/cs/play/llhls/lives_list.m3u8
    It contain master playlist for each live 

### With Docker

    - HLS generator for any input:
    docker run --network host            \
        -e INPUT_URL='rtsp://ip:port'    \
        -e REDIS_HOST='127.0.0.1'        \
        -e REDIS_PORT='6379'             \
        -e REDIS_PASS='123123'           \
        -e OUTPUT_PROFILE_1='original'   \      # without transcoding
        -e OUTPUT_PROFILE_2='SD'         \      # resolution 720x438
        -e OUTPUT_PROFILE_3='CD'         \      # resolution 320x240
        hls_generator

    - Redis server:
    docker run  -p 6379:6379             \
        -e REDIS_PASSWORD=123123         \
        bitnami/redis:6.2


    - HLS Server:
    docker run --network host            \
        -e REDIS_HOST='127.0.0.1'        \
        -e REDIS_PORT='6379'             \
        -e REDIS_PASS='123123'           \
        -e HLS_SERVER_PORT='8088'        \ 
        hls_server


## TODO:
    - Support HLS base functionality
    - Support Adaptive bit rate
    - Support Low-latency features: 
        EXT-X-PART-INF
        EXT-X-PART
        EXT-X-PRELOAD-HINT
        EXT-X-RENDITION-REPORT
        EXT-X-SKIP
    - Improve performance:
        Decrease memeory copy operations
