//
// Created by zhangjun on 17-2-9.
//

#ifndef CPPREST_WEBDAV_H
#define CPPREST_WEBDAV_H

#include <string>

namespace web
{
namespace caldav
{
struct Calendra{
    std::string data;
    std::string etag;
    std::string uri;
};

using namespace tinyxml2;

class CalDav {
private:
    std::string address_;//host:port
    std::string uri_;
    std::string user_;
    std::string password_;
    std::string dir_;

private:
    int update(const Calendra& cal, utility::string_t& etag);
    int create(const Calendra& cal, utility::string_t& etag);
    int gets(const std::vector<utility::string_t>& uris, std::vector<Calendra>& cals);
    int getall(std::vector<Calendra>& cals);
    int propfind(utility::string_t& displayName, utility::string_t& syncToken);
    int sync(const utility::string_t& syncToken, std::vector<Calendra>& cals, utility::string_t& newSyncToken);

public:
    CalDav() = delete;
    CalDav(const char* address, const char* uri, const char *user, const char* password, const char* dir)
    :address_(address),uri_(uri),user_(user),password_(password),dir_(dir)
    {}

    void SetAddress(const char* address){ address_.assign(address);}
    void SetUri(const char* uri){ uri_.assign(uri);}
    void SetUser(const char* user){ user_.assign(user);}
    void SetPassword(const char* password){ password_.assign(password);}
    void SetDir(const char* dir){ dir_.assign(dir);}

};

}

namespace carddav
{

}

}

#endif //CPPREST_WEBDAV_H
