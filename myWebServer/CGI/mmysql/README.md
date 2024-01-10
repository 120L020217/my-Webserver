1. RAII机制  
将资源的生命周期和对象生命周期绑定在一起，从而保证对象生命周期结束，资源被正确释放。

2. 超出对象作用域，不会自动析构的情况  
```cpp
// sql_conn_pool.cpp connection_pool::init()
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

```