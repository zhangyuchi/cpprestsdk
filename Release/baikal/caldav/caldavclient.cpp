/***
* Copyright (C) Microsoft. All rights reserved.
* Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
*
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* bingrequest.cpp - Simple cmd line application that makes an HTTP GET request to bing searching and outputting
*       the resulting HTML response body into a file.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/tinyxml2.h>
#include <cpprest/webdav.h>
#include <stdlib.h>

using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
using namespace web::tinyxml2;
using namespace web::webdav;

#if 0
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

static string_t loadFile(const char* fn){
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

int update(const Calendra& cal, string_t& etag){
    int ret=0;
    // Create an HTTP request.
    // Encode the URI query since it could contain special characters like spaces.
    //http_client_config config;
    //credentials cred("username", "Password");
    //config.set_credentials(cred);
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

    http_request req(methods::PUT);
    //autharg2:= base64("user:pass")
    req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
    req.headers().add("If-Match", cal.etag);
    auto uri = uri_builder().append_path(cal.uri);
    req.set_request_uri(uri.to_uri());
    req.set_body(cal.data, utf8string("text/calendar; charset=utf-8"));

    auto outbuf = container_buffer<std::vector<uint8_t>>(std::ios_base::out);

    client.request(req)
            .then([&outbuf, &etag, &ret](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      if ( response.status_code() == 201 || response.status_code() == 204 ){
                          etag = response.headers()["ETag"];
                      }else{
                          ret = response.status_code();
                      }

                      return response.body().read_to_end(outbuf);
                  })
                    // Close the file buffer.
            .then([&outbuf](size_t)
                  {
                      return outbuf.close();
                  })

                    // Wait for the entire response body to be written into the file.
            .wait();
    return ret;
}

int create(const Calendra& cal, string_t& etag){
    int ret=0;
    // Create an HTTP request.
    // Encode the URI query since it could contain special characters like spaces.
    //http_client_config config;
    //credentials cred("username", "Password");
    //config.set_credentials(cred);
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

    http_request req(methods::PUT);
    //autharg2:= base64("user:pass")
    req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
    auto uri = uri_builder().append_path(cal.uri);
    req.set_request_uri(uri.to_uri());
    req.set_body(cal.data, utf8string("text/calendar; charset=utf-8"));

    /*
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
    return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
    */

    auto outbuf = container_buffer<std::vector<uint8_t>>(std::ios_base::out);

    client.request(req)
            .then([&outbuf, &etag, &ret](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      if ( response.status_code() == 201 || response.status_code() == 204 ){
                          etag = response.headers()["ETag"];
                      }else{
                          ret = response.status_code();
                      }

                      return response.body().read_to_end(outbuf);
                  })
                  // Close the file buffer.
            .then([&outbuf](size_t)
                  {
                      return outbuf.close();
                  })

                    // Wait for the entire response body to be written into the file.
            .wait();

    return ret;
}

int gets(const std::vector<string_t>& uris, std::vector<Calendra>& cals){
    int ret=0;
    string_t origxml = "<c:calendar-multiget xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\">\n"
            "    <d:prop>\n"
            "        <d:getetag />\n"
            "        <c:calendar-data />\n"
            "    </d:prop>\n"
            "    <c:filter>\n"
            "        <c:comp-filter name=\"VCALENDAR\" />\n"
            "    </c:filter>\n"
            "</c:calendar-multiget>";

    //parse xml
    XMLDocument reqxml;
    reqxml.Parse(origxml.data(), origxml.size());
    XMLElement* root = reqxml.FirstChildElement();
    if (root){
        for (auto& uri: uris) {

            XMLElement* pHref = reqxml.NewElement("d:href");
            pHref->SetText(uri.c_str());
            root->InsertFirstChild(pHref);
        }
    }

    XMLPrinter printer;
    reqxml.Print( &printer );
    //printf("reqxml:%s\n", printer.CStr());

    // Create an HTTP request.
    // Encode the URI query since it could contain special characters like spaces.
    //http_client_config config;
    //credentials cred("username", "Password");
    //config.set_credentials(cred);
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

    http_request req(methods::REPORT);
    //autharg2:= base64("user:pass")
    req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
    req.headers().add("DEPTH", "1");

    auto uri = uri_builder().append_path("/cal.php/calendars/zj/default/");
    req.set_request_uri(uri.to_uri());
    req.set_body(printer.CStr(), utf8string("text/xml; charset=utf-8"));

    /*
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
    return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
    */

    auto outbuf = container_buffer<std::vector<uint8_t>>(std::ios_base::out);

    client.request(req)
            .then([&outbuf](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      return response.body().read_to_end(outbuf);
                  })
            .then([&outbuf,&cals](size_t respLen)
                  {
                      /*
                         size_t acquired_size = 0;
                         uint8_t* ptr;
                         bool acquired = cb.acquire(ptr, acquired_size);//may dont copy
                       */
                      printf("resplen:%ld, cblen:%ld, canread:%d\n",
                             respLen, outbuf.size(), outbuf.can_read());

                      auto& cbvec = outbuf.collection();
                      //parse xml
                      XMLDocument doc;
                      doc.Parse((char *)&cbvec[0], cbvec.size());
                      const XMLElement* response = getXmlResp(doc);
                      while(response) {
                          Calendra cal{};
                          const char *etag = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "d:getetag"});
                          const char *caldata = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "cal:calendar-data"});
                          const char *caluri = getXmlElem(response, std::vector<string_t>{"d:href"});

                          if ( !etag || !caldata || !caluri ) {
                              continue;
                          }

                          cal.uri.assign(caluri);
                          cal.etag.assign(etag);
                          cal.data.assign(caldata);
                          cals.push_back(cal);
                          response = response->NextSiblingElement("d:response");
                      }
                      return outbuf.close();
                  })
                    // Wait for the entire response body to be written into the file.
            .wait();
    return ret;
}

int getall(std::vector<Calendra>& cals){
    int ret=0;
    string_t bodyBuf = "<c:calendar-query xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\">\n"
            "    <d:prop>\n"
            "        <d:getetag />\n"
            "        <c:calendar-data />\n"
            "    </d:prop>\n"
            "    <c:filter>\n"
            "        <c:comp-filter name=\"VCALENDAR\" />\n"
            "    </c:filter>\n"
            "</c:calendar-query>";

    // Create an HTTP request.
    // Encode the URI query since it could contain special characters like spaces.
    //http_client_config config;
    //credentials cred("username", "Password");
    //config.set_credentials(cred);
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

    http_request req(methods::REPORT);
    //autharg2:= base64("user:pass")
    req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
    req.headers().add("DEPTH", "1");

    auto uri = uri_builder().append_path("/cal.php/calendars/zj/default/");
    req.set_request_uri(uri.to_uri());
    req.set_body(bodyBuf, utf8string("text/xml; charset=utf-8"));

    /*
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
    return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
    */

    auto outbuf = container_buffer<std::vector<uint8_t>>(std::ios_base::out);

    client.request(req)
            .then([&outbuf](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      return response.body().read_to_end(outbuf);
                  })
            .then([&outbuf,&cals](size_t respLen)
                  {
                      /*
                         size_t acquired_size = 0;
                         uint8_t* ptr;
                         bool acquired = cb.acquire(ptr, acquired_size);//may dont copy
                       */
                      //long newpos = outbuf.seekpos(1, std::ios_base::in);//if canread is false ,seek will fail
                      printf("resplen:%ld, cblen:%ld, canread:%d\n",
                             respLen, outbuf.size(), outbuf.can_read());

                      auto& cbvec = outbuf.collection();
                      //parse xml
                      XMLDocument doc;
                      doc.Parse((char *)&cbvec[0], cbvec.size());
                      const XMLElement* response = getXmlResp(doc);
                      while(response) {
                          Calendra cal{};
                          const char *etag = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "d:getetag"});
                          const char *caldata = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "cal:calendar-data"});
                          const char *uri = getXmlElem(response, std::vector<string_t>{"d:href"});

                          if ( !etag || !caldata || !uri ) {
                              continue;
                          }

                          cal.uri.assign(uri);
                          cal.etag.assign(etag);
                          cal.data.assign(caldata);
                          cals.push_back(cal);
                          response = response->NextSiblingElement("d:response");
                      }
                      return outbuf.close();
                  })
                    // Wait for the entire response body to be written into the file.
            .wait();

    return ret;
}

int propfind(string_t& displayName, string_t& syncToken){
    int ret=0;

    string_t bodyBuf = "<d:propfind xmlns:d=\"DAV:\" xmlns:cs=\"http://calendarserver.org/ns/\">\n"
            "  <d:prop>\n"
            "     <d:displayname />\n"
            "     <cs:getctag />\n"
            "     <d:sync-token />\n"
            "  </d:prop>\n"
            "</d:propfind>";
    // Create an HTTP request.
    // Encode the URI query since it could contain special characters like spaces.
    //http_client_config config;
    //credentials cred("username", "Password");
    //config.set_credentials(cred);
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

    http_request req(methods::PROPFIND);
    //autharg2:= base64("user:pass")
    req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
    req.headers().add("DEPTH", "0");

    auto uri = uri_builder().append_path("/cal.php/calendars/zj/default/");
    req.set_request_uri(uri.to_uri());
    req.set_body(bodyBuf, utf8string("text/xml; charset=utf-8"));

    /*
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
    return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
    */

    auto outbuf = container_buffer<std::vector<uint8_t>>(std::ios_base::out);

    client.request(req)
            .then([&outbuf](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      return response.body().read_to_end(outbuf);
                  })
            .then([&outbuf,&displayName,&syncToken](size_t respLen)
                  {
                   /*
                      size_t acquired_size = 0;
                      uint8_t* ptr;
                      bool acquired = cb.acquire(ptr, acquired_size);//may dont copy
                    */
                      printf("resplen:%ld, cblen:%ld, canread:%d\n",
                             respLen, outbuf.size(), outbuf.can_read());

                      auto& cbvec = outbuf.collection();
                      printf("resplen:%ld, cap:%ld\n", cbvec.size(), cbvec.capacity());
                      string_t bodyStr{(char *)&cbvec[0], cbvec.size()};
                      printf("resp:\n%s\n", bodyStr.c_str());

                      //parse xml
                      XMLDocument doc;
                      doc.Parse((char *)&cbvec[0], cbvec.size());
                      const XMLElement* response = getXmlResp(doc);
                      if (response) {
                          const char *text = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "d:displayname"});
                          if (text) {
                              displayName.assign(text);
                          }
                          text = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "d:sync-token"});
                          if (text) {
                              syncToken.assign(text);
                          }
                      }
                      return outbuf.close();
                  })
                    // Wait for the entire response body to be written into the file.
            .wait();

    return ret;
}

int sync(const string_t& syncToken, std::vector<Calendra>& cals, string_t& newSyncToken){
    int ret = 0;

    if (syncToken.empty()){ return -1; }

    string_t origxml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
            "<d:sync-collection xmlns:d=\"DAV:\">\n"
            "  <d:sync-level>1</d:sync-level>\n"
            "  <d:prop>\n"
            "    <d:getetag/>\n"
            "  </d:prop>\n"
            "</d:sync-collection>";

    //parse xml
    XMLDocument reqxml;
    reqxml.Parse(origxml.data(), origxml.size());
    XMLElement* root = reqxml.FirstChildElement();
    if (root){
        XMLElement* pHref = reqxml.NewElement("d:sync-token");
        pHref->SetText(syncToken.c_str());
        root->InsertFirstChild(pHref);
    }

    XMLPrinter printer;
    reqxml.Print( &printer );
    //printf("reqxml:%s\n", printer.CStr());

    // Create an HTTP request.
    // Encode the URI query since it could contain special characters like spaces.
    //http_client_config config;
    //credentials cred("username", "Password");
    //config.set_credentials(cred);
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

    http_request req(methods::REPORT);
    //autharg2:= base64("user:pass")
    req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
    req.headers().add("DEPTH", "1");

    auto uri = uri_builder().append_path("/cal.php/calendars/zj/default/");
    req.set_request_uri(uri.to_uri());
    req.set_body(printer.CStr(), utf8string("text/xml; charset=utf-8"));

    /*
    http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
    return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
    */

    auto outbuf = container_buffer<std::vector<uint8_t>>(std::ios_base::out);

    client.request(req)
            .then([&outbuf](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      return response.body().read_to_end(outbuf);
                  })
            .then([&outbuf,&cals,&newSyncToken](size_t respLen)
                  {
                      /*
                         size_t acquired_size = 0;
                         uint8_t* ptr;
                         bool acquired = cb.acquire(ptr, acquired_size);//may dont copy
                       */
                      printf("resplen:%ld, cblen:%ld, canread:%d\n",
                             respLen, outbuf.size(), outbuf.can_read());

                      auto& cbvec = outbuf.collection();
                      //parse xml
                      XMLDocument doc;
                      doc.Parse((char *)&cbvec[0], cbvec.size());
                      const XMLElement* response = getXmlResp(doc);

                      const XMLElement* sync_token_elem = response->NextSiblingElement("d:sync-token");
                      if (sync_token_elem){
                          newSyncToken = sync_token_elem->GetText();
                      }

                      while(response) {
                          Calendra cal{};
                          const char *etag = getXmlElem(response, std::vector<string_t>{"d:propstat", "d:prop", "d:getetag"});
                          const char *caluri = getXmlElem(response, std::vector<string_t>{"d:href"});

                          if ( !etag || !caluri ) {
                              continue;
                          }

                          cal.uri.assign(caluri);
                          cal.etag.assign(etag);
                          cals.push_back(cal);
                          response = response->NextSiblingElement("d:response");
                      }
                      return outbuf.close();
                  })
                    // Wait for the entire response body to be written into the file.
            .wait();

    return ret;
}

#endif

#ifdef _WIN32
int wmain(int argc, wchar_t *args[])
#else
int main(int argc, char *args[])
#endif
{
    if(argc < 2)
    {
        printf("Usage: BingRequest.exe type [filename]\n");
        return -1;
    }

    const int type = atoi(args[1]);
    CalDav caldav("http", "127.0.0.1:801", "zj", "duzimei");

    switch (type) {
        case 1:{ //create
            Calendra cal;
            cal.uri = "/cal.php/calendars/zj/default/my2.ics";
            cal.data = loadFile(args[2]);
            string_t etag;
            caldav.Upload(cal, etag);
            printf("etag:%s\n", etag.c_str());
            }
            break;
        case 2: { //get指定的ical
                std::vector<Calendra> cals;
                caldav.DownloadItems("/cal.php/calendars/zj/default/", std::vector<string_t>{"/cal.php/calendars/zj/default/my.ics","/cal.php/calendars/zj/default/my1.ics"}, cals);
                for(Calendra& cal:cals){
                    printf("uri:%s\netag:%s\ndata:%s\n\n", cal.uri.c_str(),cal.etag.c_str(),cal.data.c_str());
                }
            }
            break;
        case 3: {//取所有的ical
                std::vector<Calendra> cals;
                caldav.DownloadAll("/cal.php/calendars/zj/default/", cals);
                for(Calendra& cal:cals){
                    printf("uri:%s\netag:%s\ndata:%s\n\n", cal.uri.c_str(),cal.etag.c_str(),cal.data.c_str());
                }
            }
            break;
        case 4: {//取属性信息
                string_t displayName,syncToken;
                caldav.Propfind("/cal.php/calendars/zj/default/", displayName, syncToken);
                printf("displayname:%s, synctoken:%s\n", displayName.c_str(), syncToken.c_str());
            }
            break;
        case 5: { //update
            Calendra cal;
            cal.uri = "/cal.php/calendars/zj/default/my2.ics";
            cal.data = loadFile(args[2]);
            cal.etag = "\"0e294ea9e5485f7dbd539bf506f6707e\"";
            string_t etag;
            caldav.Upload(cal, etag);
            printf("etag:%s\n", etag.c_str());
        }
            break;
        case 6: { //sync
            std::vector<Calendra> cals;
            string_t newSyncToken;
            caldav.Sync("/cal.php/calendars/zj/default/","http://sabre.io/ns/sync/1", cals, newSyncToken);
            printf("new sync-token:%s\n", newSyncToken.c_str());
            for(Calendra& cal:cals){
                printf("uri:%s\netag:%s\n\n", cal.uri.c_str(),cal.etag.c_str());
            }
        }
            break;
        default:
            printf("invalid type: %d\n", type);
    }

    return 0;
}
