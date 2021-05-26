#include <iostream>
#include <boost/log/trivial.hpp>
#include "hls_server.h"

int main()
{
    Hls_server HlsServer;

    const char *redis_host = getenv("REDIS_HOST");
    const char *redis_port = getenv("REDIS_PORT");
    const char *redis_pass = getenv("REDIS_PASS");
    const char *hls_server_port = getenv("HLS_SERVER_PORT");

    if (!redis_host || !redis_port || !redis_pass || !hls_server_port)
    {
        LOG(error) << "Please set environement vars: "
                   << "REDIS_HOST, REDIS_PORT, REDIS_PASS, HLS_SERVER_PORT";
        return EXIT_FAILURE;
    }
    HlsServer.init_routes();
    HlsServer.set_redis_connection_info(redis_host, std::stoi(redis_port), redis_pass);
    HlsServer.start("0.0.0.0", std::stoi(hls_server_port));
    
    return EXIT_FAILURE; // it should not finish!
}
