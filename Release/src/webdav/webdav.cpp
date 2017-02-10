//
// Created by zhangjun on 17-2-10.
//

#include "stdafx.h"
#include <cpprest/filestream.h>

using namespace web;
using namespace web::tinyxml2;
using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
using namespace web::tinyxml2;

static const char* getXmlElem(const XMLElement* element, std::vector<string_t>&& path){
    const char *text = nullptr;

    size_t num = path.size();
    if (num>0){
        for (auto i=0; i<path.size() && element; ++i){
            element = element->FirstChildElement(path[i].c_str());
        }

        if ( element )
            text = element->GetText();
    }

    return text;
}

static const XMLElement* getXmlResp(const XMLDocument& doc){

    const XMLElement* elem = nullptr;
    elem = doc.FirstChildElement("d:multistatus");
    if (elem){
        elem = elem->FirstChildElement("d:response");
    }

    return elem;
}

static utility::string_t loadFile(const char* fn){
    std::unique_ptr<uint8_t[]> bodyBuf = nullptr;
    string_t fcontent;

    file_buffer<uint8_t>::open(fn, std::ios::in)
            .then([&](streambuf<uint8_t> inFile) -> pplx::task<size_t>
                  {
                      bodyBuf = std::make_unique<uint8_t[]>(inFile.size());
                      return inFile.getn(bodyBuf.get(), inFile.size());
                  })
            .then([&](size_t len)
                  {
                      fcontent.assign((char *)bodyBuf.get(), len);
                      return;
                  })
            .wait();

    return fcontent;
}

/* Can pass proxy information via environment variable http_proxy.
   Example:
   Linux:   export http_proxy=http://192.1.8.1:8080
 */
static web::http::client::http_client_config client_config_for_proxy()
{
    web::http::client::http_client_config client_config;
#ifdef _WIN32
    wchar_t* pValue;
    size_t len;
    auto err = _wdupenv_s(&pValue, &len, L"http_proxy");
    if (!err) {
        std::unique_ptr<wchar_t, void(*)(wchar_t*)> holder(pValue, [](wchar_t* p) { free(p); });
        uri proxy_uri(std::wstring(pValue, len));
#else
    if(const char* env_http_proxy = std::getenv("http_proxy")) {
        uri proxy_uri(utility::conversions::to_string_t(env_http_proxy));
#endif
        web::web_proxy proxy(proxy_uri);
        client_config.set_proxy(proxy);
    }

    return client_config;
}