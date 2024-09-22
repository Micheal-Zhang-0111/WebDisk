#include <iostream>
#include <workflow/WFTaskFactory.h>
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <nlohmann/json.hpp>

#include "WebCloudServer.h"
#include "WebCloudSignup.h"
#include "WebCloudLogin.h"
#include "WebCloudHome.h"
#include "WebCloudUpload.h"
//消息队列rabbitmq
#include"mqProducer.h"

using namespace wfrest;
using std::string;
using Json = nlohmann::json;

const string MYSQL_USER("root");
const string MYSQL_PASSWORD("root");
const string MYSQL_HOST("localhost");

bool isTokenCorrect(const string &username, const string &token, SeriesWork *series)
{
    bool ret = true;
    string mysqlURL = "mysql://" + MYSQL_USER + ":" + MYSQL_PASSWORD + "@" + MYSQL_HOST;
    auto mysqlTask = WFTaskFactory::create_mysql_task(
        mysqlURL, 1,
        [&](WFMySQLTask *mysqlTask)
        {
            if (!WebCloudHome::isMysqlTaskSuccess(mysqlTask) || WebCloudHome::isSqlError(mysqlTask))
            {
                ret = false;
                return;
            }

            string tokenData;
            if (!WebCloudHome::getToken(mysqlTask, tokenData))
            {
                ret = false; // token 表中没有信息
                return; 
            }
            if (token != tokenData)
            {
                ret = false; // token 不正确
                return;
            }
        });

    string sql = "select user_token from WebDisk.tbl_user_token where user_name='" + username + "'";
    mysqlTask->get_req()->set_query(sql);
    series->push_back(mysqlTask); 
    return ret;
}

WebCloudServer::WebCloudServer(int nleft)
:_server(),
_waitGroup(nleft)
{
}

void WebCloudServer::loadFavicon(){
    _server.GET(
        "/favicon.ico", 
        [](const HttpReq *, HttpResp *resp)
        {
            resp->File("../static/img/logo.png");
        });
    _server.GET(
        "/static/img/xinhai.mp4", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/img/xinhai.mp4"); 
        });
    _server.GET(
        "/static/img/nahida.jpeg", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/img/nahida.jpeg"); 
        });
    _server.GET(
        "/static/img/shenhe.jpg", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/img/shenhe.jpg"); 
        });
    _server.GET(
        "/static/img/bg.jpg", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/img/bg.jpg"); 
        });

    _server.GET(
        "/static/js/auth.js", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/js/auth.js"); 
        });
}

void WebCloudServer::loadUserSignupModule(){
    string mysqlURL = "mysql://" + MYSQL_USER + ":" + MYSQL_PASSWORD + "@" + MYSQL_HOST;

    _server.GET(
        "/user/signup", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/view/signup.html"); 
        });

    _server.POST(
        "/user/signup",
        [mysqlURL](const HttpReq *req, HttpResp *resp, SeriesWork *series) 
        {
            if (req->content_type() != APPLICATION_URLENCODED)
            {
                resp->String("REQUEST ERROR");
                return;
            }

            auto fromKV = req->form_kv();
            string username = fromKV["username"];
            string password = fromKV["password"];
            if (username.size() == 0 || password.size() == 0)
            {
                resp->String("IS EMPTY");
                return;
            }
            string saltPassword = WebCloudSignup::cryptPassword(password);

            auto mysqlTask = WFTaskFactory::create_mysql_task(
                mysqlURL, 1,
                [resp](WFMySQLTask *mysqlTask)
                {
                    if (!WebCloudSignup::isMysqlTaskSuccess(mysqlTask))
                    {
                        resp->String("SERVER ERROR");
                        return;
                    }
                    if (WebCloudSignup::isSqlError(mysqlTask))
                    {
                        resp->String("USER EXIST");
                        return;
                    }

                    if (WebCloudSignup::isInsertSuccess(mysqlTask))
                        resp->String("SUCCESS");
                    else
                        resp->String("SERVER ERROR");
                });

            string sql = "insert into WebDisk.tbl_user(user_name, user_pwd)values('" + username + "', '" + saltPassword + "')";
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask);
        });
}

void WebCloudServer::loadUserLoginModule(){
    string mysqlURL = "mysql://" + MYSQL_USER + ":" + MYSQL_PASSWORD + "@" + MYSQL_HOST;
    _server.GET(
        "/static/view/signin.html",
        [](const HttpReq *, HttpResp *resp)
        {
            resp->File("../static/view/signin2.html");
        });

    _server.GET(
        "/user/signin", 
        [](const HttpReq *, HttpResp *resp) 
        {
            printf("/user/signin\n");
            resp->File("../static/view/signin2.html"); 
        });
    _server.GET(
        "/user/style.css", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/view/style.css"); 
        });

    _server.POST(
        "/user/signin", 
        [mysqlURL](const HttpReq *req, HttpResp *resp, SeriesWork *series) 
        {
            fprintf(stderr, "signin\n");
            if (req->content_type() != APPLICATION_URLENCODED)
            {
                resp->String("REQUEST ERROR");
                return;
            }

            auto fromKV = req->form_kv();
            fprintf(stderr, "username: %s\n", fromKV["username"].c_str());
            fprintf(stderr, "password: %s\n", fromKV["password"].c_str());
            string username = fromKV["username"];
            string password = fromKV["password"];
            if (username.size() == 0 || password.size() == 0)
            {
                resp->String("PASSWORD ERROR");
                return;
            }

            auto mysqlTask = WFTaskFactory::create_mysql_task(
                mysqlURL, 1,
                [resp, username, password, mysqlURL, series](WFMySQLTask *mysqlTask)
                {
                    if (!WebCloudLogin::isMysqlTaskSuccess(mysqlTask) || WebCloudLogin::isSqlError(mysqlTask))
                    {
                        resp->String("SERVER ERROR");
                        return;
                    }

                    // 验证密码
                    string saltPassword;
                    if (!WebCloudLogin::getPassword(mysqlTask, saltPassword))
                    {
                        resp->String("PASSWORD ERROR");
                        return;
                    }
                    if (!WebCloudLogin::isPasswordCorrect(password, saltPassword))
                    {
                        resp->String("PASSWORD ERROR");
                        return;
                    }
                    // 密码正确就生成 token 值
                    string token = WebCloudLogin::getToken(username, saltPassword);
                    auto nextMysqlTask = WFTaskFactory::create_mysql_task(
                        mysqlURL, 1,
                        [resp](WFMySQLTask *mysqlTask)
                        {
                            if (!WebCloudLogin::isMysqlTaskSuccess(mysqlTask) || 
                                WebCloudLogin::isSqlError(mysqlTask))
                            {
                                resp->String("SERVER ERROR");
                            }
                        });
                    string tokenSql = "replace into WebDisk.tbl_user_token(user_name, user_token)VALUES('" + username + "', '" + token + "')";
                    nextMysqlTask->get_req()->set_query(tokenSql);
                    series->push_back(nextMysqlTask);

                    fprintf(stderr, "token: %s\n", token.c_str());

                    nlohmann::json msg, data;
                    data["Token"] = token;
                    data["Username"] = username;
                    data["Location"] = "/home";
                    msg["data"] = data;
                    resp->String(msg.dump());
                });

            string sql = "select user_pwd from WebDisk.tbl_user where user_name = '" + username + "'";
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask);
        });
}

void WebCloudServer::loadHomePage(){
    string mysqlURL = "mysql://" + MYSQL_USER + ":" + MYSQL_PASSWORD + "@" + MYSQL_HOST;
    _server.GET(
        "/home", 
        [](const HttpReq *, HttpResp *resp) 
        {
            printf("home\n");
            resp->File("../static/view/home.html"); 
        });

    _server.GET(
        "/user/info", 
        [mysqlURL](const HttpReq *req, HttpResp *resp, SeriesWork *series) 
        {

            // string username = "admin";
            // string token = "1234";
            string username = req->query("username");
            string token = req->query("token");  
            if (!isTokenCorrect(username, token, series))
            {
                resp->String("NO DATA");
                return;
            }
            fprintf(stderr, "username: %s\n", username.c_str());
            fprintf(stderr, "token: %s\n", token.c_str());
            auto mysqlTask = WFTaskFactory::create_mysql_task(
                mysqlURL, 1,
                [resp, username](WFMySQLTask *mysqlTask)
                {
                    if (!WebCloudHome::isMysqlTaskSuccess(mysqlTask) || 
                        WebCloudHome::isSqlError(mysqlTask))
                    {
                        resp->String("SERVER ERROR");
                        return;
                    }

                    string signupTime;
                    if (!WebCloudHome::getSignupTime(mysqlTask, signupTime))
                    {
                        resp->String("NO DATA");
                        return;
                    }
                    using Json = nlohmann::json;
                    Json msg, data;
                    data["Username"] = username;
                    data["SignupAt"] = signupTime;
                    msg["data"] = data;
                    resp->String(msg.dump());
                });

            string sql = "select signup_at from WebDisk.tbl_user where user_name='" + username + "'";
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask); 
        });

    _server.POST(
        "/file/query", 
        [mysqlURL](const HttpReq *req, HttpResp *resp, SeriesWork *series) 
        {
            string username = req->query("username");
            string token = req->query("token");

            if (!isTokenCorrect(username, token, series))
            {
                resp->String("NO DATA");
                return;
            }

            // 获取文件列表
            auto mysqlTask = WFTaskFactory::create_mysql_task(
                mysqlURL, 1,
                [resp](WFMySQLTask *mysqlTask)
                {
                    if (!WebCloudHome::isMysqlTaskSuccess(mysqlTask) || 
                        WebCloudHome::isSqlError(mysqlTask))
                    {
                        resp->String("SERVER ERROR");
                        return;
                    }

                    nlohmann::json msgArray;
                    if (!WebCloudHome::getFileList(mysqlTask, msgArray))
                    {
                        resp->String("SERVER ERROR");
                        return;
                    }
                    resp->String(msgArray.dump());
                });

            string sql = "select file_name, file_size, file_sha1, upload_at, last_update from WebDisk.tbl_user_file where user_name='" + username + "'";
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask); 
        });    
}

void WebCloudServer::loadUploadModule(){
    string mysqlURL = "mysql://" + MYSQL_USER + ":" + MYSQL_PASSWORD + "@" + MYSQL_HOST;

    _server.GET(
        "/file/upload", 
        [](const HttpReq *, HttpResp *resp) 
        {
            resp->File("../static/view/index.html"); 
        });

    _server.POST(
        "/file/upload",
        [mysqlURL](const HttpReq *req, HttpResp *resp, SeriesWork *series) 
        {
            string username = req->query("username");
            string token = req->query("token");

            if (!isTokenCorrect(username, token, series))
            {
                resp->String("NO DATA");
                return;
            }

            // 解析 form-data 格式的 body
            auto fileInfo =  req->form();
            string fileName = fileInfo["file"].first;
            string fileData = fileInfo["file"].second;
            string fileSizeStr = std::to_string(fileData.size());

            if (!WebCloudUpload::writeFile(fileName, fileData))
            {
                resp->String("SERVER ERROR");
                return;
            }

            string fileHash = WebCloudUpload::makeSha1(fileName);
            if (fileHash.empty())
            {
                resp->String("SERVER ERROR");
                return;
            }

            // 文件信息写入数据库中
            auto mysqlTask = WFTaskFactory::create_mysql_task(
                mysqlURL, 1,
                [resp](WFMySQLTask *mysqlTask)
                {
                    if (!WebCloudHome::isMysqlTaskSuccess(mysqlTask) || 
                        WebCloudHome::isSqlError(mysqlTask))
                    {
                        resp->String("SERVER ERROR");
                        return;
                    }
                });
            string sql = "insert into WebDisk.tbl_user_file(user_name, file_sha1, file_size, file_name)values('" + username + "','" + fileHash + "'," + fileSizeStr + ",'"+ fileName + "')";
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask); 

            // // 阿里云 OSS 简单上传
            // WebCloudUpload::uploadOSS(fileName);
            //使用rabbitmq发送消息，通过消息队列来异步上传文件到阿里云oss
            nlohmann::json backup2OssInfo;
            string filePath = "../tmp/" + fileName;
            backup2OssInfo["filePath"] = filePath;
            backup2OssInfo["fileName"] = fileName;
            backup2OssInfo["fileHash"] = fileHash;
            // 自定义了一个mqProducer类，用来作为消息的生产者
            mqProducer producer;
            // 交换机名 路由键 消息
            producer.doPublish("WebDisk_oss_exchange","oss",backup2OssInfo.dump());

            // 文件上传成功就重定向到主页
            resp->set_status_code("301");
            resp->set_reason_phrase("Moved Permanently");
            resp->headers["Location"] = "/home";
        });
}

void WebCloudServer::loadDownloadModule(){
    _server.GET(
        "/file/downloadurl",
        [](const HttpReq *req, HttpResp *resp) 
        {
            string fileName = req->query("filename");
            printf("downloadurl\n");
            // 生成下载链接, 把下载任务交给 nginx 执行
            string downloadURL = "http://192.168.198.128:8080/" + fileName;
            resp->String(downloadURL);
            fprintf(stderr, "downloadURL: %s\n", downloadURL.c_str());
        });
}

void WebCloudServer::start(unsigned short port){
    printf("0\n");
    if (_server.track().start(port) == 0)
    {
        _server.list_routes();
        fprintf(stderr, "server start success\n");
        _waitGroup.wait();
        _server.stop();
    }
    printf("server start failed\n");
}
