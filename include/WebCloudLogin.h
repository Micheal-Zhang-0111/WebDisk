#pragma once

#include <workflow/WFTaskFactory.h>

class WebCloudLogin {
public:
    static bool isMysqlTaskSuccess(WFMySQLTask *mysqlTask);
    static bool isSqlError(WFMySQLTask *mysqlTask);

    static bool getPassword(WFMySQLTask *mysqlTask, std::string &oldPassword);
    static bool isPasswordCorrect(const std::string &password, const std::string &saltPassword);

    static std::string getToken(const std::string &username, const std::string &saltPassword);
};