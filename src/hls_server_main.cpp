#include "hls_server.h"

int main()
{
    Hls_server HlsServer;
    HlsServer.init_routes();
    HlsServer.set_redis_connection_info("127.0.0.1", 6379, "31233123");
    HlsServer.start("0.0.0.0", 8080);
}
