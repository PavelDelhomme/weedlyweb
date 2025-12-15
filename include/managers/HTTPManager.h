#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <string>
#include <curl/curl.h>

class HTTPManager {
public:
    std::string recuperer(const std::string& url);
};

#endif
