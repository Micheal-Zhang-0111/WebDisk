#pragma once

#include<string>
#include<workflow/WFTaskFactory.h>

class WebCloudSignup
{
public:
    static std::string createSalt();

    static std::string cryptPassword(std::string password);

    static bool isMysqlTaskSuccess(WFMySQLTask *mysqlTask);

    static bool isSqlError(WFMySQLTask *mysqlTask);

    static bool isInsertSuccess(WFMySQLTask *mysqlTask);
};
