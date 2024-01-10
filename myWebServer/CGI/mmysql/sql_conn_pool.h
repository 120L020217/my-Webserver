#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <pthread.h>
#include "../../lock/locker.h"
#include "../../log/log.h"

using namespace std;

class connection_pool {
public:
    static connection_pool *GetInstance() {
        static connection_pool instance;
        return &instance;
    }

    void init(string url, string User, string PassWord, string DatabaseName, int port, int MaxConn, int close_log);

    MYSQL* GetConnection(); // 获取一个可用连接
    bool ReleaseConnection(MYSQL* conn); // 释放连接
    int GetFreeConn(); // 返回可用连接数
    void DestroyPool(); // 销毁线程池

private:
    connection_pool() {
        m_CurConn = 0;
        m_FreeConn = 0;
    }
    ~connection_pool() {
        DestroyPool();
    }

    int m_MaxConn; // 最大连接数
    int m_CurConn; // 当前使用连接数
    int m_FreeConn; // 当前空闲连接数
    locker lock; // 互斥锁作用 : 保护连接池（共享资源）
    list<MYSQL *> connList; // 数据库连接池
    sem reserve; // 信号量作用 ：标志连接池中剩余资源数量，多线程共享该资源

public:
    string m_url;
    string m_Port;
    string m_User;
    string m_PassWord;
    string m_DatabaseName;
    int m_close_log; // 日志开关
};

class connectionRAII {
public:
    // TODO: 为什么用二级指针？ 能想到的原因是他要修改mysql指针的指向，才会用二级指针
    connectionRAII(MYSQL** con, connection_pool* connPool) {
        *con = connPool->GetConnection();
        conRAII = *con;
        poolRAII = connPool;
    }
    ~connectionRAII() {
        poolRAII->ReleaseConnection(conRAII);
    }
private:
    MYSQL* conRAII;
    connection_pool* poolRAII;
};

#endif