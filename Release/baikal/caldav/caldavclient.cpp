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

using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
using namespace web::tinyxml2;
using namespace web::caldav;

static const char* getXmlElem(const XMLDocument& doc, std::vector<string_t>&& path);

/* Can pass proxy information via environment variable http_proxy.
   Example:
   Linux:   export http_proxy=http://192.1.8.1:8080
 */
web::http::client::http_client_config client_config_for_proxy()
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

void create(const string_t& inputFileName, const string_t& outputFileName){
    std::unique_ptr<uint8_t[]> bodyBuf = nullptr;
    utility::size64_t bodyLen=0;

    auto inBuffer = std::make_shared<streambuf<uint8_t>>();
    file_buffer<uint8_t>::open(inputFileName, std::ios::in)
            .then([&](streambuf<uint8_t> inFile) -> pplx::task<size_t>
                  {
                      bodyBuf = std::make_unique<uint8_t[]>(inFile.size());
                      return inFile.getn(bodyBuf.get(), inFile.size());
                  })
            .then([&](size_t len)
                  {
                      bodyLen = len;
                      return;
                  })
            .wait();

    printf("bodyBuf is %s\n", bodyBuf.get());

#if 1
    // Open a stream to the file to write the HTTP response body into.
    auto fileBuffer = std::make_shared<streambuf<uint8_t>>();
    file_buffer<uint8_t>::open(outputFileName, std::ios::out)
            .then([&](streambuf<uint8_t> outFile) -> pplx::task<http_response>
                  {
                      *fileBuffer = outFile;

                      // Create an HTTP request.
                      // Encode the URI query since it could contain special characters like spaces.
                      //http_client_config config;
                      //credentials cred("username", "Password");
                      //config.set_credentials(cred);
                      http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());

                      http_request req(methods::PUT);
                      //autharg2:= base64("user:pass")
                      req.headers().add("Authorization", "Basic emo6ZHV6aW1laQ==");
                      auto uri = uri_builder().append_path("/cal.php/calendars/zj/default/").append_path(inputFileName);
                      req.set_request_uri(uri.to_uri());
                      req.set_body(utf8string{(char*)bodyBuf.get(), bodyLen}, utf8string("text/calendar; charset=utf-8"));

                      /*
                      http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
                      return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
                      */
                      return client.request(req);
                  })
                    // Write the response body into the file buffer.
            .then([=](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());

                      return response.body().read_to_end(*fileBuffer);
                  })

                    // Close the file buffer.
            .then([=](size_t)
                  {
                      return fileBuffer->close();
                  })

                    // Wait for the entire response body to be written into the file.
            .wait();

#endif
}

void get(const string_t& inputFileName, const string_t& outputFileName){
    std::unique_ptr<uint8_t[]> bodyBuf = nullptr;
    utility::size64_t bodyLen=0;

    auto inBuffer = std::make_shared<streambuf<uint8_t>>();
    file_buffer<uint8_t>::open(inputFileName, std::ios::in)
            .then([&](streambuf<uint8_t> inFile) -> pplx::task<size_t>
                  {
                      bodyBuf = std::make_unique<uint8_t[]>(inFile.size());
                      return inFile.getn(bodyBuf.get(), inFile.size());
                  })
            .then([&](size_t len)
                  {
                      bodyLen = len;
                      return;
                  })
            .wait();

    printf("bodyBuf is %s\n", bodyBuf.get());

    // Open a stream to the file to write the HTTP response body into.
    auto fileBuffer = std::make_shared<streambuf<uint8_t>>();
    file_buffer<uint8_t>::open(outputFileName, std::ios::out)
            .then([&](streambuf<uint8_t> outFile) -> pplx::task<http_response>
                  {
                      *fileBuffer = outFile;

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
                      req.set_body(utf8string{(char *)bodyBuf.get(), bodyLen}, utf8string("text/xml; charset=utf-8"));

                      /*
                      http_client client(U("http://127.0.0.1:801/"), client_config_for_proxy());
                      return client.request(methods::GET, uri_builder(U("/cal.php/calendars/zj/default/")).append_query(U("q"), searchTerm).to_string());
                      */
                      return client.request(req);
                  })
                    // Write the response body into the file buffer.
            .then([=](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());

                      return response.body().read_to_end(*fileBuffer);
                  })

                    // Close the file buffer.
            .then([=](size_t)
                  {
                      return fileBuffer->close();
                  })

                    // Wait for the entire response body to be written into the file.
            .wait();
}

void getall(std::vector<Calendra>& cals){

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
    std::unique_ptr<uint8_t[]> respBuf = nullptr;

    client.request(req)
            .then([&outbuf](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      return response.body().read_to_end(outbuf);
                  })
            .then([&outbuf,&respBuf,&cals](size_t respLen)
                  {
                      /*
                         size_t acquired_size = 0;
                         uint8_t* ptr;
                         bool acquired = cb.acquire(ptr, acquired_size);//may dont copy
                       */
                      respBuf = std::make_unique<uint8_t[]>(respLen);
                      long newpos = outbuf.seekpos(1, std::ios_base::in);//if canread is false ,seek will fail
                      printf("resplen:%ld, cblen:%ld, newpos:%ld, canread:%d\n",
                             respLen, outbuf.size(), newpos, outbuf.can_read());

                      auto& cbvec = outbuf.collection();
                      printf("resplen:%ld, cap:%ld\n", cbvec.size(), cbvec.capacity());
                      string_t bodyStr{(char *)&cbvec[0], cbvec.size()};
                      printf("resp:\n%s\n", bodyStr.c_str());

                      //parse xml
                      XMLDocument doc;
                      doc.Parse((char *)&cbvec[0], cbvec.size());
                      const char* etag = getXmlElem(doc, std::vector<string_t>{"d:multistatus", "d:response", "d:propstat", "d:prop", "d:getetag"});
                      printf( "displayname is: %s\n", etag );
                      Calendra cal;
                      cal.etag.assign(etag);
                      const char *caldata = getXmlElem(doc, std::vector<string_t>{"d:multistatus", "d:response", "d:propstat", "d:prop", "cal:calendar-data"});
                      printf( "getctag is: %s\n", caldata );
                      cal.data.assign(caldata);

                      return outbuf.close();
                  })
                    // Wait for the entire response body to be written into the file.
            .wait();
}

void propfind(string_t& displayName, string_t& syncToken){

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
    std::unique_ptr<uint8_t[]> respBuf = nullptr;

    client.request(req)
            .then([&outbuf](http_response response) -> pplx::task<size_t>
                  {
                      printf("Response status code %u returned.\n", response.status_code());
                      return response.body().read_to_end(outbuf);
                  })
            .then([&outbuf,&respBuf,&displayName,&syncToken](size_t respLen)
                  {
                   /*
                      size_t acquired_size = 0;
                      uint8_t* ptr;
                      bool acquired = cb.acquire(ptr, acquired_size);//may dont copy
                    */
                      respBuf = std::make_unique<uint8_t[]>(respLen);
                      long newpos = outbuf.seekpos(1, std::ios_base::in);//if canread is false ,seek will fail
                      printf("resplen:%ld, cblen:%ld, newpos:%ld, canread:%d\n",
                             respLen, outbuf.size(), newpos, outbuf.can_read());

                      auto& cbvec = outbuf.collection();
                      printf("resplen:%ld, cap:%ld\n", cbvec.size(), cbvec.capacity());
                      string_t bodyStr{(char *)&cbvec[0], cbvec.size()};
                      printf("resp:\n%s\n", bodyStr.c_str());

                      //parse xml
                      XMLDocument doc;
                      doc.Parse((char *)&cbvec[0], cbvec.size());
                      const char* text = getXmlElem(doc, std::vector<string_t>{"d:multistatus", "d:response", "d:propstat", "d:prop", "d:displayname"});
                      printf( "displayname is: %s\n", text );
                      displayName.assign(text);
                      text = getXmlElem(doc, std::vector<string_t>{"d:multistatus", "d:response", "d:propstat", "d:prop", "cs:getctag"});
                      printf( "getctag is: %s\n", text );
                      syncToken.assign(text);
                      text = getXmlElem(doc, std::vector<string_t>{"d:multistatus", "d:response", "d:propstat", "d:prop", "d:sync-token"});
                      printf( "sync-token is: %s\n", text );

                      return outbuf.close();
                  })
                    // Wait for the entire response body to be written into the file.
            .wait();
}

const char* getXmlElem(const XMLDocument& doc, std::vector<string_t>&& path){
    const char *text = nullptr;

    size_t num = path.size();
    if (num>0){
        const XMLElement* element = doc.FirstChildElement(path[0].c_str());
        for (auto i=1; i<path.size() && element; ++i){
            element = element->FirstChildElement(path[i].c_str());
        }

        if ( element )
            text = element->GetText();
    }

    return text;
}

#ifdef _WIN32
int wmain(int argc, wchar_t *args[])
#else
int main(int argc, char *args[])
#endif
{
    if(argc != 4)
    {
        printf("Usage: BingRequest.exe type search_term output_file\n");
        return -1;
    }

    const int type = atoi(args[1]);
    const string_t inputFileName = args[2];
    const string_t outputFileName = args[3];

    switch (type) {
        case 1:
            create(inputFileName,outputFileName);
            break;
        case 2:
            get(inputFileName, outputFileName);
            break;
        case 3: {
                std::vector<Calendra> cals;
                getall(cals);
            }
            break;
        case 4: {
                string_t displayName,syncToken;
                propfind(displayName, syncToken);
            }
            break;
        default:
            printf("invalid type: %d\n", type);
    }

    return 0;
}
