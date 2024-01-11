1. 析构函数是否显示释放成员资源问题
```cpp
#include <list>

class MyClass {
public:
    MyClass() {
        // 在构造函数中分配资源
        pmyList = new std::list<int>();
    }

    ~MyClass() {
        // 在析构函数中释放资源
        delete pmyList;
    }

private:
    std::list<int>* pmyList; // 动态分配，需要析构函数中显示释放这个指针
    std::list<int> myList; // 不需要析构函数中释放，随着对象被销毁，自动调用std::list的析构函数
};

int main() {
    MyClass obj;
    // 在这里，obj 在 main 函数结束时被销毁，析构函数将会被调用，从而释放 myList 的内存。
    return 0;
}

```