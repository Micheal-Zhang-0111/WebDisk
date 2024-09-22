#include<../include/WebCloudHome.h>
#include<sstream>
#include<workflow/MySQLResult.h>

using std::vector;

bool WebCloudHome::isMysqlTaskSuccess(WFMySQLTask *mysqlTask)
{
    int state = mysqlTask->get_state();
    int error = mysqlTask->get_error();
    if (state != WFT_STATE_SUCCESS) {
        printf("%s\n", WFGlobal::get_error_string(state, error));
        return false;
    }
    return true;
}

bool WebCloudHome::isSqlError(WFMySQLTask *mysqlTask)
{
    auto resp = mysqlTask->get_resp();
    if(resp->get_packet_type() == MYSQL_PACKET_ERROR) {
        std::ostringstream oss;
        oss << "ERROR " << resp->get_error_code() 
            << " : " << resp->get_error_msg();
        printf("%s\n", oss.str().c_str());
        return true;
    }
    return false; 
}

bool WebCloudHome::getSignupTime(WFMySQLTask *mysqlTask, std::string &signupTime)
{
    protocol::MySQLResultCursor cursor(mysqlTask->get_resp());
    if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT)
        return false;

    vector<vector<protocol::MySQLCell>> rows;
    cursor.fetch_all(rows);
    if (rows.size() == 0)
        return false;
    
    signupTime = rows[0][0].as_datetime();
    return true;
}

bool WebCloudHome::getToken(WFMySQLTask *mysqlTask, std::string &token)
{
    protocol::MySQLResultCursor cursor(mysqlTask->get_resp());
    if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT)
        return false;

    vector<vector<protocol::MySQLCell>> rows;
    cursor.fetch_all(rows);
    if (rows.size() == 0)
        return false;
    
    token = rows[0][0].as_string();
    return true;
}

bool WebCloudHome::getFileList(WFMySQLTask *mysqlTask, nlohmann::json &msgArray)
{
    protocol::MySQLResultCursor cursor(mysqlTask->get_resp());
    if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT)
        return false;

    vector<vector<protocol::MySQLCell>> rows;
    cursor.fetch_all(rows);

    for (size_t i = 0; i < rows.size(); ++i)
    {
        //保持查询的顺序一致
        nlohmann::json row;
        row["FileName"] = rows[i][0].as_string();
        row["FileSize"] = rows[i][1].as_ulonglong();
        row["FileHash"] = rows[i][2].as_string();
        row["UploadAt"] = rows[i][3].as_datetime();
        row["LastUpdated"] = rows[i][4].as_datetime();
        msgArray.push_back(row);
    }
    return true;
}

