//
// Created by Byakuya on 2026/8/8.
//

#ifndef USERACTIVATOR_FILEEXECUTER_H
#define USERACTIVATOR_FILEEXECUTER_H

void add(const char* fileName, const char* msg, ...);
void remove(const char* fileName, const char* targetLine);
bool contains(const char* fileName, const char* targetLine);

class FileExecuter {
};


#endif //USERACTIVATOR_FILEEXECUTER_H
