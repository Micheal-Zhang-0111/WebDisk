#pragma once

#include<workflow/WFTaskFactory.h>
#include<nlohmann/json.hpp>

class WebCloudHome
{
public:
    static bool isMysqlTaskSuccess(WFMySQLTask *mysqlTask);
    static bool isSqlError(WFMySQLTask *mysqlTask);
    
    static bool getSignupTime(WFMySQLTask *mysqlTask, std::string &signupTime);
    static bool getToken(WFMySQLTask *mysqlTask, std::string &token);

    static bool getFileList(WFMySQLTask *mysqlTask, nlohmann::json &msgArray);
};