#include "../include/linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/MySQLResult.h>
#include <workflow/MySQLMessage.h>
#include <wfrest/HttpServer.h>
#include <wfrest/json.hpp>
#include "../include/UserInfo.h"
#include "../include/Token.h"
#include "../include/FileUtil.h"
static WFFacilities::WaitGroup waitGroup(1);
using Json = nlohmann::json;
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr,"wait group is done\n");
}
int main(){
    signal(SIGINT,sigHandler);
    wfrest::HttpServer server;
    server.GET("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("../static/view/index.html");
    });
    server.POST("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // 从URL中提取用户名的信息
        auto userInfo = req->query_list();
        std::string username = userInfo["username"];
        //读取文件内容，解析form-data类型的请求报文
        using Form = std::map<std::string,std::pair<std::string,std::string>>;
        Form &form = req->form();
        auto fileInfo = form["file"];
        //fileInfo.first是文件名，fileInfo.second是文件内容
        std::string filepath = "../tmp/" + fileInfo.first;
        int fd = open(filepath.c_str(),O_CREAT|O_RDWR,0666);
        if(fd < 0){
            resp->set_status_code("500");
            fprintf(stderr,"open file failed\n");
            return;
        }
        int ret = write(fd,fileInfo.second.c_str(),fileInfo.second.size());
        close(fd);
        //把上传文件之后的逻辑写到callback里面
        // auto pwriteTask = WFTaskFactory::create_pwrite_task(fd,fileInfo.second.c_str(),fileInfo.second.size(),0,callback);
        // series->push_back(pwriteTask);
        std::string sql = "INSERT INTO WebDisk.tbl_file (file_sha1,file_name,file_size,file_addr,status) VALUES('" 
        + FileUtil::sha1File(filepath.c_str()) + "','" 
        + fileInfo.first + "'," 
        + std::to_string(fileInfo.second.size()) + ",'" 
        + filepath + "',0);";
        sql += "INSERT INTO WebDisk.tbl_user_file (user_name, file_sha1, file_name, file_size) VALUES ('"
                   + username + "','"
                   + FileUtil::sha1File(filepath.c_str()) + "','"
                   + fileInfo.first + "',"
                   + std::to_string(fileInfo.second.size()) + ");";
        fprintf(stderr,"sql = %s\n",sql.c_str());
        resp->MySQL("mysql://root:root@localhost",sql,[](Json *pjson){
            fprintf(stderr,"out = %s\n", pjson->dump().c_str());
        });
        resp->set_status_code("302");
        resp->headers["Location"] = "/file/upload/success";
    });
    server.GET("/file/upload/success",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->String("upload success");
    });
    server.GET("/file/download",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // /file/download?filehash=2aae6c35c94fcfb415dbe95f408b9ce91ee846ed&filename=1.txt&filesize=11
        auto fileInfo = req->query_list();
        std::string filehash = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        std::string filepath = "../tmp/" + filename;

        resp->set_status_code("302");
        resp->headers["Location"] = "http://192.168.191.128:1235/" + filename;
    });

    server.GET("/user/signup",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("../static/view/signup.html");
    });
    server.POST("/user/signup",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        //1、按urlencoded的形式去解析post报文体
        std::map<std::string,std::string> &form_kv = req->form_kv();
        std::string username = form_kv["username"];
        std::string password = form_kv["password"];
        //2、把密码进行加密
        std::string salt = "12345678";
        char *encryptPassword = crypt(password.c_str(),salt.c_str());
        // fprintf(stderr,"username:%s,password:%s\n",username.c_str(),password.c_str());
        // fprintf(stderr,"encryptPassword:%s\n",encryptPassword);
        //3、把用户信息插入到数据库
        std::string sql = "INSERT INTO WebDisk.tbl_user (user_name,user_pwd) VALUES( '"+username+"','"+encryptPassword+"');";
        // fprintf(stderr,"sql:%s\n",sql.c_str());
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:root@localhost",0,[](WFMySQLTask *mysqlTask){
            //4、回复SUCCESS给前端
            wfrest::HttpResp *resp2client = static_cast<wfrest::HttpResp*>(mysqlTask->user_data);
            // 检查连接错误
            if(mysqlTask->get_state() != WFT_STATE_SUCCESS){
                fprintf(stderr,"error msg:%s\n",WFGlobal::get_error_string(mysqlTask->get_state(), mysqlTask->get_error()));
                resp2client->append_output_body("FAIL",4);
                return;
            }
            protocol::MySQLResponse *resp = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(resp);
            // 检查语法错误
            if(resp->get_packet_type() == MYSQL_PACKET_ERROR){
                fprintf(stderr,"error_code = %d msg = %s\n",resp->get_error_code(), resp->get_error_msg().c_str());
                resp2client->append_output_body("FAIL",4);
                return;
            }
            if(cursor.get_cursor_status() == MYSQL_STATUS_OK){
                //写指令，执行成功
                fprintf(stderr,"OK. %llu rows affected. %d warnings. insert_id = %llu.\n",
                cursor.get_affected_rows(), cursor.get_warnings(), cursor.get_insert_id());
                if(cursor.get_affected_rows() == 1){
                    resp2client->append_output_body("SUCCESS",7);
                    return;
                }
            }
        });
        mysqlTask->get_req()->set_query(sql);
        mysqlTask->user_data = resp;
        series->push_back(mysqlTask);
    });
    server.GET("/static/view/signin.html",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("../static/view/signin.html");
    });
    server.GET("/static/view/home.html",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("../static/view/home.html");
    });
    server.GET("/static/js/auth.js",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("../static/js/auth.js");
    });
    server.GET("/static/img/avatar.jpeg",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("../static/img/avatar.jpeg");
    });

    server.POST("/user/signin",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        //1、解析用户请求
        std::map<std::string,std::string> &form_kv = req->form_kv();
        std::string username = form_kv["username"];
        std::string password = form_kv["password"];
        //2、查询数据库
        std::string url = "mysql://root:root@localhost";
        std::string sql = "SELECT user_pwd FROM WebDisk.tbl_user WHERE user_name = '"+username+"' LIMIT 1;";
        auto readTask = WFTaskFactory::create_mysql_task(url,0,[](WFMySQLTask *readTask){
            //提取readTask的结果
            auto resp = readTask->get_resp();
            protocol::MySQLResultCursor cursor(resp);

            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);

            std::string nowPassword = rows[0][0].as_string();
            fprintf(stderr,"nowPassword:%s\n",nowPassword.c_str());

            UserInfo *userinfo = static_cast<UserInfo*>(series_of(readTask)->get_context());
            char *inPassword = crypt(userinfo->password.c_str(),"12345678");
            fprintf(stderr,"inPassword:%s\n",inPassword);
            if(strcmp(nowPassword.c_str(),inPassword) != 0){
                userinfo->resp->append_output_body("FAIL",4);
                return;
            }
            //3、生成一个token，存入数据库中
            //用户的信息->加密得到密文 拼接上登陆时间
            Token usertoken(userinfo->username,"12345678");
            // fprintf(stderr,"token:%s\n",usertoken.token.c_str());
            userinfo->token = usertoken.token;
            //存入数据库中
            std::string url = "mysql://root:root@localhost";
            std::string sql = "INSERT INTO WebDisk.tbl_user_token (user_name,user_token) VALUES('"      +userinfo->username+"','"+usertoken.token+"');";
            auto writeTask = WFTaskFactory::create_mysql_task(url,0,[](WFMySQLTask *writeTask){
                UserInfo *userinfo = static_cast<UserInfo*>(series_of(writeTask)->get_context());
                Json uinfo;
                uinfo["Username"] = userinfo->username;
                uinfo["Token"] = userinfo->token;
                uinfo["Location"] = "/static/view/home.html";
                Json respInfo;
                respInfo["code"] = 0;
                respInfo["msg"] = "OK";
                respInfo["data"] = uinfo;
                userinfo->resp->String(respInfo.dump());
            });
            writeTask->get_req()->set_query(sql);
            series_of(readTask)->push_back(writeTask);
            //4、将信息包装成json返回给客户端
        });
        readTask->get_req()->set_query(sql);
        series->push_back(readTask);

        UserInfo *userinfo = new UserInfo;
        userinfo->username = username;
        userinfo->password = password;
        userinfo->resp = resp;
        series->set_context(userinfo);
        //在序列回调函数中释放
        series->set_callback([](const SeriesWork *series){
            UserInfo *userinfo = static_cast<UserInfo*>(series->get_context());
            delete userinfo;
            fprintf(stderr,"delete userinfo\n");
        });
    });
    server.POST("/user/info",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        //1、解析用户请求
        auto userInfo = req->query_list();
        //2、校验token是否合法 --> 拦截器 
        //3、根据用户信息，查询sql
        std::string sql = "SELECT user_name,signup_at FROM WebDisk.tbl_user WHERE user_name = '"+userInfo["username"]+"'LIMIT 1;";
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:root@localhost",0,[resp](WFMySQLTask *mysqlTask){
            //提取mysqlTask的结果
            auto respMysql = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(respMysql);
            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);
            // fprintf(stderr,"username = %s,signup_at = %s\n",rows[0][0].as_string().c_str(),rows[0][1].as_string().c_str());
            Json uInfo;
            uInfo["Username"] = rows[0][0].as_string();
            uInfo["SignupAt"] = rows[0][1].as_datetime();
            Json respInfo;
            respInfo["data"] = uInfo;
            respInfo["code"] = 0;
            respInfo["msg"] = "OK";
            resp->String(respInfo.dump());
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    });

    server.POST("/file/query",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
         // 解析用户请求
        auto userInfo = req->query_list();
        std::string username = userInfo["username"];
        auto form_kv = req->form_kv();
        std::string limit = form_kv["limit"];
        // 根据用户名查tbl_user_file
        std::string sql = "SELECT file_sha1,file_name,file_size,upload_at,last_update FROM WebDisk.tbl_user_file WHERE user_name = '"
                        + username + "' LIMIT " + limit + ";";
        //fprintf(stderr,"sql = %s\n", sql.c_str());
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:root@localhost",0,[resp](WFMySQLTask *mysqlTask){
            auto respMysql = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(respMysql);
            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);
            Json respArr;
            for(auto &row:rows){
                Json fileJson;
                // row[0] file_sha1
                fileJson["FileHash"] = row[0].as_string();
                // row[1] file_name
                fileJson["FileName"] = row[1].as_string();
                // row[2] file_size
                fileJson["FileSize"] = row[2].as_ulonglong();
                // row[3] upload_at
                fileJson["UploadAt"] = row[3].as_datetime();
                // row[4] lastupdate
                fileJson["LastUpdated"] = row[4].as_datetime();
                respArr.push_back(fileJson);
            }
            fprintf(stderr,"out = %s\n", respArr.dump().c_str());
            resp->String(respArr.dump());
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    });
    server.POST("/file/downloadurl",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        auto fileInfo = req->query_list();
        std::string sha1 = fileInfo["filehash"];
        std::string sql = "SELECT file_name FROM WebDisk.tbl_file WHERE file_sha1='" + sha1 + "';";
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:root@localhost",0,[resp](WFMySQLTask *mysqlTask){
            auto respMysql = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(respMysql);
            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);
            resp->String("http://192.168.191.128:1235/"+rows[0][0].as_string());                                                  
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    });


    if(server.track().start(1234) == 0){
        server.list_routes();
        waitGroup.wait();
        server.stop();
    }
    else{
        fprintf(stderr,"can not start server!\n");
        return -1;
    }
    
}