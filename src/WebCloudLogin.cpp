#include <crypt.h>
#include <time.h>
#include <sstream>
#include <workflow/MySQLResult.h>
#include <openssl/md5.h>
#include "WebCloudLogin.h"


using std::string;
using std::vector;

bool WebCloudLogin::isMysqlTaskSuccess(WFMySQLTask *mysqlTask)
{
    int state = mysqlTask->get_state();
    int error = mysqlTask->get_error();
    if (state != WFT_STATE_SUCCESS) {
        printf("%s\n", WFGlobal::get_error_string(state, error));
        return false;
    }
    return true;
}

bool WebCloudLogin::isSqlError(WFMySQLTask *mysqlTask)
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

bool WebCloudLogin::getPassword(WFMySQLTask *mysqlTask, std::string &oldPassword)
{
    auto resp = mysqlTask->get_resp();
    protocol::MySQLResultCursor cursor(resp);
    if (cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT)
    {
        vector<vector<protocol::MySQLCell>> rows;
        cursor.fetch_all(rows);
        oldPassword = rows[0][0].as_string();
        return true;
    }
    return false;
}

bool WebCloudLogin::isPasswordCorrect(const std::string &password, const std::string &saltPassword)
{
    string salt = saltPassword.substr(0, 20);
    string newPassword = crypt(password.c_str(), salt.c_str());
    if (newPassword == saltPassword)
        return true;
    else
        return false;
}

string WebCloudLogin::getToken(const string &username, const string &saltPassword)
{
    string salt = saltPassword.substr(0, 20);
    string tmp = salt + username;
    unsigned char md[16] = {0};
    MD5((const unsigned char *) tmp.c_str(), tmp.size(), md);

    char fragment[3] = {0};
    string result;
    for (int i = 0; i < 16; ++i)
    {
        sprintf(fragment, "%02x", md[i]);
        result += fragment;
    }
      
    time_t secs = time(nullptr);
    struct tm  * ptm = localtime(&secs);
    char buff[15] = {0};
    sprintf(buff, "%04d%02d%02d%02d%02d%02d", 
            ptm->tm_year + 1900,
            ptm->tm_mon + 1,
            ptm->tm_mday,
            ptm->tm_hour,
            ptm->tm_min,
            ptm->tm_sec);
    return result + buff; 
}

