#include <time.h>
#include <cstdlib>
#include <crypt.h>
#include <sstream>
#include <workflow/MySQLResult.h>
#include "WebCloudSignup.h"

using std::string;

string WebCloudSignup::createSalt(){
    string saltValue(20, 0);
    saltValue[0] = '$';
    saltValue[1] = '6';
    saltValue[2] = '$';
    saltValue[19] = '$';

    srand(time(nullptr));
    const char char16[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int i = 0; i < 16; ++i)
        saltValue[i + 3] = char16[rand()%(sizeof(char16) - 1)];

    return saltValue;
}

string WebCloudSignup::cryptPassword(std::string password){
    return crypt(password.c_str(), createSalt().c_str());
}

bool WebCloudSignup::isMysqlTaskSuccess(WFMySQLTask *mysqlTask)
{
    int state = mysqlTask->get_state();
    int error = mysqlTask->get_error();
    if (state != WFT_STATE_SUCCESS) {
        printf("%s\n", WFGlobal::get_error_string(state, error));
        return false;
    }
    return true;
}

bool WebCloudSignup::isSqlError(WFMySQLTask *mysqlTask)
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

bool WebCloudSignup::isInsertSuccess(WFMySQLTask *mysqlTask)
{
    auto resp = mysqlTask->get_resp();
    protocol::MySQLResultCursor cursor(resp);
    if (cursor.get_cursor_status() == MYSQL_STATUS_OK) {
        std:: string s = cursor.get_affected_rows() < 2 ? "" : "s";
        std::ostringstream oss;
        oss << "Query OK, " << cursor.get_affected_rows() << " row" << s << " affected";
        // printf("%s\n", oss.str().c_str());
        return true;
    }
    return false;
}
