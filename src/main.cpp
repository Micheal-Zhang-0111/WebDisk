#include <signal.h>
#include <iostream>
#include "WebCloudServer.h"

void test(){
    WebCloudServer server(1);
    server.loadFavicon();
    server.loadUserSignupModule();
    server.loadUserLoginModule();
    server.loadHomePage();
    server.loadUploadModule();
    server.loadDownloadModule();
    server.start(1234);
}

int main()
{
    test();
    return 0;
}