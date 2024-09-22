#include "wfrest/HttpServer.h"
#include <workflow/WFFacilities.h>
using namespace wfrest;

static WFFacilities::WaitGroup waitGroup(1);

wfrest::HttpServer svr;


int main()
{
    svr.GET("/hello", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("world\n");
    });

    svr.GET(
        "/user/signup", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/view/signup.html"); 
        });
    // svr.GET(
    //     "/favicon.ico", 
    //     [](const HttpReq *, HttpResp *resp)
    //     {
    //         resp->File("../static/img/avatar.jpeg");
    //     });
    svr.GET(
        "/static/img/nahida.jpeg", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/img/nahida.jpeg"); 
        });
    svr.GET(
        "/static/img/shenhe.jpg", 
        [](const HttpReq *, HttpResp *resp) 
        {
            fprintf(stderr, "/this is /static/img/shenhe.jpg\n");
            resp->File("../static/img/shenhe.jpg"); 
        });
    
    if(svr.track().start(8888) == 0){
        svr.list_routes();
        waitGroup.wait();
        svr.stop();
    }
    else{
        fprintf(stderr,"can not start server!\n");
        return -1;
    }
    
    return 0;
}