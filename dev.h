#ifndef DEV_H
//by xuyimin for debugging ease


//#define DBGXU
#include <iostream>
#include <fstream>

std::ofstream mLog;

#define LOG(x) if(!mLog.is_open()){mLog.open("log.txt");}mLog<<x<<std::endl;mLog.flush();

#define DEV_H
#endif