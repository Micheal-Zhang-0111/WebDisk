#pragma once

#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>

class WebCloudServer
{
public:
    WebCloudServer(int nleft);
    ~WebCloudServer() = default;

    void loadFavicon();
    void loadUserSignupModule();
    void loadUserLoginModule();
    void loadHomePage();
    void loadUploadModule();
    void loadDownloadModule();

    void start(unsigned short port);

private:
    wfrest::HttpServer _server;
    WFFacilities::WaitGroup _waitGroup;
};