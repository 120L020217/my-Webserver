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
} // 执行完毕，mysql对象不会析构，因为con是一个指针，指向一个mysql对象，类似new一个对象的情况，mysql对象的销毁在mysql_close(con)执行。

```

3. 正是因为指针无法在超出作用域时析构指向的对象，智能指针应运而生。

```cpp
// 手写一个智能指针类
#include <iostream>

template <typenaem T>
class SmartPointer {
public:
    // 构造函数
    explicit SmartPointer(T* ptr = nullptr) : _ptr(ptr), _count(ptr ? new int (1) : nullptr) {}
    // 拷贝构造函数
    SmartPointer(const SmartPointer<T>& other) : _ptr(other.ptr), _count(other._count) {
        if (_count) {
            ++(*_count);
        }
    }
    // 赋值操作符
    /**
     * const： 不能修改引用的对象
     * 
    */
    SmartPointer<T>& operator=(const SmartPointer<T>& other) {
        if (this != &other) {
            release();
            _ptr = other._ptr;
            _count = other._count;
            if (_count) {
                ++(*_count);
            }
        }
        return *this;
    }
    // 析构函数
    ~SmartPointer<T>() {
        release();
    }
    // 重载解引用操作符
    /**
     * 返回引用，而不是左值：返回函数内修改的对象本身。
     * 
    */
    T& operator*() const {
        return *_ptr;
    }
    // 重载箭头操作符
    T* operator->() const {
        return _ptr;
    }
    // 获取引用计数
    int use_count() const {
        return _count ? *_count : 0;
    }

private:
    // 释放资源
    void release() {
        if (_count && --(*_count) == 0) {
            delete _ptr;
            delete _count;
        }
    }
    T* _ptr;
    int* _count;
};
```