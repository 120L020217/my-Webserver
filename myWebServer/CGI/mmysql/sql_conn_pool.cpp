#include "sql_conn_pool.h"

void connection_pool::init(string url, string User, string PassWord, string DatabaseName, int port, int MaxConn) {
    m_url = url;
    m_User = User;
    m_Port = port;
    m_PassWord = PassWord;
    m_DatabaseName = DatabaseName;

    for (int i = 0; i < MaxConn; i++) {
        MYSQL* con = mysql_init(nullptr); // 初始化一个 MYSQL 对象
        if (con == nullptr) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }

        if (mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DatabaseName.c_str(), port, NULL, 0) == nullptr) {
            LOG_ERROR("MySQL Error");
            mysql_close(con);  // 在连接失败时释放连接对象
            exit(1);
        } else {
            connList.push_back(con);
            ++m_FreeConn;
        }
    } // 执行完毕，mysql对象不会析构，再cAPI中，mysql连接对象的生命周期由程序员管理

    reserve = sem(m_FreeConn);
    m_MaxConn = m_FreeConn;
}

MYSQL* connection_pool::GetConnection() {
    MYSQL* con = nullptr;
    if (0 == connList.size()) {
        return nullptr;
    }

    reserve.wait();
    lock.lock();

    con = connList.front();
    connList.pop_front();
    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return con;
}

bool connection_pool::ReleaseConnection(MYSQL* conn) {
    if (nullptr == conn) {
        return false;
    } 

    lock.lock();

    connList.push_back(conn);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();
    reserve.post();
    return true;
}

int connection_pool::GetFreeConn() {
    return this->m_FreeConn;
}

void connection_pool::DestroyPool() {
    lock.lock();
    if (connList.size() > 0) {
        for (list<MYSQL*>::iterator it = connList.begin(); it != connList.end(); it++) {
            mysql_close(*it); // 关闭连接
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear(); // 清理容器中这些悬空指针
    }
    lock.unlock();
}