//
// Created by zhangjun on 17-2-9.
//

#ifndef CPPREST_WEBDAV_H
#define CPPREST_WEBDAV_H

#include <string>

namespace web
{
namespace webdav
{

utility::string_t loadFile(const char *fn);

struct Calendra{
    std::string data;
    std::string etag;
    std::string uri;
};

enum AuthMethod{
    BasicAuth=0,
    DigestAuth=1,
    NegotiateAuth=2
};

class CalDav {
private:
    std::string scheme_;
    std::string address_;//host:port
    std::string user_;
    std::string password_;
    std::string sync_token_;//for sync
    std::string auth_token_;//for third party auth
    AuthMethod method_;

private:
    int update(const Calendra& cal, std::string& etag);
    int create(const Calendra& cal, std::string& etag);
    int gets(const std::string& url, const std::vector<std::string>& uris, std::vector<Calendra>& cals);
    int getall(const std::string& url, std::vector<Calendra>& cals);
    int propfind(const std::string& url, std::string& displayName, std::string& syncToken);
    int sync(const std::string& url, const std::string& syncToken, std::vector<Calendra>& cals, std::string& newSyncToken);

    std::string getAuthToken();
public:
    CalDav() = delete;
    CalDav(const char* scheme, const char* address, const char *user, const char* password)
    :scheme_(scheme), address_(address), user_(user), password_(password),method_(BasicAuth)
    {}

    void SetAddress(const char* address){ address_.assign(address); }
    void SetUser(const char* user){ user_.assign(user); }
    void SetPassword(const char* password){ password_.assign(password); }
    void SetScheme(const char* scheme){ scheme_.assign(scheme); }
    void SetAuthMethod(int method){method_ = (AuthMethod)method;}

    int Propfind(const std::string& url, std::string& displayName, std::string& syncToken);
    int Upload(const Calendra& cal,  std::string& etag);
    int DownloadItems(const std::string& url, const std::vector<std::string>& uris, std::vector<Calendra>& cals);
    int DownloadAll(const std::string& url, std::vector<Calendra>& cals);
    int Sync(const std::string& url, const std::string& syncToken, std::vector<Calendra>& cals, std::string& newSyncToken);

};

}

}

#endif //CPPREST_WEBDAV_H
