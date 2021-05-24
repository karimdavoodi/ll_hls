#pragma once
#include <map>
#include <thread>
#include <served/served.hpp>
#include "redis_client.h"
#include "config.h"

class Hls_server
{
    private:
        int port;
        served::multiplexer mux;

        std::string redis_address;
        int         redis_port; 
        string      redis_passwd;

        std::map < std::thread::id , RedisClient* > redis_map; 
    public:
        Hls_server(): port{HSL_SERVER_PORT}{}
        Hls_server(int port):port{port}{}
        void set_redis_connection_info( const std::string& addr, int port, 
                const std::string& passwd){
            redis_address = addr;
            redis_port = port;
            redis_passwd = passwd;
        }
        RedisClient *get_redis();    
        void init_routes();

        void list_lives(served::response&, const served::request&);
        void master_playlist(served::response&, const served::request&);
        void single_playlist(served::response&, const served::request&);
        void segment(served::response&, const served::request&);
        void psegment(served::response&, const served::request&);
        void start(const std::string address = "0.0.0.0", int port = 8088);
        ~Hls_server();
};
/*
 
     input -----> HlsGenerate --> to Redis
                                        ---> HlsServer
                
        key                         type     description
        Lives_list                  list     names of lives channels     
        Live_master:name            list     profiles of live 'name' 
        Live_segment:name:P:N       binary   segment 'N' of profile 'P' of live 'name'     
        Live_psegment:name:P:N:M    binary   partial segment 'M' of segment 'N' 
                                                            of profile 'P' of live 'name'     

        Client

        Get all lives playlist
            path: /v1/cs/play/llhls/lives_list.m3u8
        Get one live master playlist 
            path: /v1/cs/play/llhls/master/<name>/play.m3u8
        Get one live profile playlist 
            path: /v1/cs/play/llhls/single/<name>/<profile>/play.m3u8
                  parameters:       
                   - _HLS_msn=<M>  server must response when the segment 'M' is ready
                   - _HLS_skip=YES|v2  request playlist delta update
        Get segment  
            path: /v1/cs/play/llhls/segment/<name>/<profile>/<segment>
                Support byterange:
                    Server has header : Accept-Ranges: bytes
                    Client has header : Range: bytes=0-1023
                    Server response   : Content-Range: bytes 0-1023/146515
        Get partial segment  
            path: /v1/cs/play/llhls/psegment/<name>/<profile>/<segment>

 
  LL-HLS new features:
   - generate partial media segments: 
           EXT-X-PART
   - provide playlist delta update: 
           EXT-X-SKIP 
           client wants skip old segmen from playlist
   - block playlist reload: 
           client needs playlist with new segment, 
           server block until requested segment be ready
   - Preload Hints and Blocking of Media Downloads: 
           EXT-X-PRELOAD-HINT
           server hint cient for new contents, client can ask them, 
           server block until it will be ready
   - Provide Rendition Reports
           EXT-X-RENDITION-REPORT
           server report rendition for client to do ABR in fast manner
   --------------
   - GET parameter of LL-HLS:
       - _HLS_msn=<M>  server must response when the segment 'M' is ready
       - _HLS_skip=YES|v2  request playlist delta update
   -------------
   - New TAGS
        EXT-X-SERVER-CONTROL
              CAN-SKIP-UNTIL
              CAN-SKIP-DATERANGES
              HOLD-BACK
              PART-HOLD-BACK
              CAN-BLOCK-RELOAD

        EXT-X-PART-INF
        EXT-X-PART
        EXT-X-PRELOAD-HINT
        EXT-X-RENDITION-REPORT
        EXT-X-SKIP
---------- EXAMPLE ----------------
#EXTM3U
# This Playlist is a response to: GET https://example.com/2M/waitForMSN.php?_HLS_msn=273&_HLS_part=2
#EXT-X-TARGETDURATION:4
#EXT-X-VERSION:6
#EXT-X-SERVER-CONTROL:CAN-BLOCK-RELOAD=YES,PART-HOLD-BACK=1.0,CAN-SKIP-UNTIL=12.0
#EXT-X-PART-INF:PART-TARGET=0.33334
#EXT-X-MEDIA-SEQUENCE:266
#EXT-X-PROGRAM-DATE-TIME:2019-02-14T02:13:36.106Z
#EXT-X-MAP:URI="init.mp4"
#EXTINF:4.00008,
fileSequence266.mp4
#EXTINF:4.00008,
fileSequence267.mp4
#EXTINF:4.00008,
fileSequence268.mp4
#EXTINF:4.00008,
fileSequence269.mp4
#EXTINF:4.00008,
fileSequence270.mp4
#EXT-X-PART:DURATION=0.33334,URI="filePart271.0.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.1.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.2.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.3.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.4.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart271.5.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.6.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.7.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.8.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart271.9.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.10.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.11.mp4"
#EXTINF:4.00008,
fileSequence271.mp4
#EXT-X-PROGRAM-DATE-TIME:2019-02-14T02:14:00.106Z
#EXT-X-PART:DURATION=0.33334,URI="filePart272.a.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.b.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.c.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.d.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.e.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.f.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart272.g.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.h.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.i.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.j.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.k.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.l.mp4"
#EXTINF:4.00008,
fileSequence272.mp4
#EXT-X-PART:DURATION=0.33334,URI="filePart273.0.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart273.1.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart273.2.mp4"
#EXT-X-PRELOAD-HINT:TYPE=PART,URI="filePart273.3.mp4"

#EXT-X-RENDITION-REPORT:URI="../1M/waitForMSN.php",LAST-MSN=273,LAST-PART=2
#EXT-X-RENDITION-REPORT:URI="../4M/waitForMSN.php",LAST-MSN=273,LAST-PART=1
---------- EXAMPLE PARTIAL PLAYLIST ----------------
#EXTM3U
#  GET https://example.com/2M/waitForMSN.php?_HLS_msn=273&_HLS_part=3 &_HLS_skip=YES
#EXT-X-TARGETDURATION:4
#EXT-X-VERSION:9
#EXT-X-SERVER-CONTROL:CAN-BLOCK-RELOAD=YES,PART-HOLD-BACK=1.0,CAN-SKIP-UNTIL=12.0
#EXT-X-PART-INF:PART-TARGET=0.33334
#EXT-X-MEDIA-SEQUENCE:266
#EXT-X-SKIP:SKIPPED-SEGMENTS=3
#EXTINF:4.00008,
fileSequence269.mp4
#EXTINF:4.00008,
fileSequence270.mp4
#EXT-X-PART:DURATION=0.33334,URI="filePart271.0.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.1.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.2.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.3.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.4.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart271.5.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.6.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.7.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.8.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart271.9.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.10.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart271.11.mp4"
#EXTINF:4.00008,
fileSequence271.mp4
#EXT-X-PROGRAM-DATE-TIME:2019-02-14T02:14:00.106Z
#EXT-X-PART:DURATION=0.33334,URI="filePart272.a.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.b.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.c.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.d.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.e.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.f.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart272.g.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.h.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.i.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.j.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.k.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart272.l.mp4"
#EXTINF:4.00008,
fileSequence272.mp4
#EXT-X-PART:DURATION=0.33334,URI="filePart273.0.mp4",INDEPENDENT=YES
#EXT-X-PART:DURATION=0.33334,URI="filePart273.1.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart273.2.mp4"
#EXT-X-PART:DURATION=0.33334,URI="filePart273.3.mp4"
#EXT-X-PRELOAD-HINT:TYPE=PART,URI="filePart273.4.mp4"

#EXT-X-RENDITION-REPORT:URI="../1M/waitForMSN.php",LAST-MSN=273,LAST-PART=3
#EXT-X-RENDITION-REPORT:URI="../4M/waitForMSN.php",LAST-MSN=273,LAST-PART=3
-------------------------
# In these examples only the end of the Playlist is shown.
# This is Playlist update 1
#EXTINF:4.08,
fs270.mp4
#EXT-X-PART:DURATION=1.02,URI="fs271.mp4",BYTERANGE="20000@0"
#EXT-X-PART:DURATION=1.02,URI="fs271.mp4",BYTERANGE="23000@20000"
#EXT-X-PART:DURATION=1.02,URI="fs271.mp4",BYTERANGE="18000@43000"
#EXT-X-PRELOAD-HINT:TYPE=PART,URI="fs271.mp4",BYTERANGE-START=61000
 * */
