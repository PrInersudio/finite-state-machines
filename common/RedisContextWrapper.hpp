#ifndef REDISCONTEXTWRAPPER_HPP
#define REDISCONTEXTWRAPPER_HPP

#include <hiredis/hiredis.h>
#include <vector>
#include <string>

#define IP "127.0.0.1"
#define PORT 6379
#define CONNECTION_ATTEMPTS 11

class RedisContextWrapper {
    private:
        redisContext* c;
    
        void reconnect();
    public:
        RedisContextWrapper();
        std::vector<std::vector<std::string>> command(const char* format, ...);
        ~RedisContextWrapper();
};

#endif