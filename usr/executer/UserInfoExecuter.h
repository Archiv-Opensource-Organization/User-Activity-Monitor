//
// Created by Work on 2026/8/5.
//

#ifndef LIBUSERACTIVITYMONITOR_USER_INFO_EXECUTER_H
#define LIBUSERACTIVITYMONITOR_USER_INFO_EXECUTER_H

#define _HAS_STD_BYTE 0

#include <string>

using namespace std;

class UserInfoExecuter {

public:
    void lockUser(string userName);
    void unlockUser(string userName);
    void changeUserPassword(string username, string password);

private:
    void modifyUserAccount(const string& username, const string& newPassword, int operationType);
};


#endif //LIBUSERACTIVITYMONITOR_USER_INFO_EXECUTER_H
