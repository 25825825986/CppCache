#pragma once

#include <memory>

namespace Cache 
{

template<typename Key, typename Value>

//通用的缓存节点类
class ArcNode 
{
private:
    Key key_;
    Value value_;
    //记录访问次数
    size_t accessCount_;
    std::weak_ptr<ArcNode> prev_;
    std::shared_ptr<ArcNode> next_;

public:

    //默认构造函数，初始化访问次数和指向后节点指针
    ArcNode() : accessCount_(1), next_(nullptr) {}
    //带参构造函数，赋值
    ArcNode(Key key, Value value) 
        : key_(key)
        , value_(value)
        , accessCount_(1)
        , next_(nullptr) 
    {}

    // Getters，外部访问
    Key getKey() const { return key_; }
    Value getValue() const { return value_; }
    size_t getAccessCount() const { return accessCount_; }
    
    // Setters，更改value和递增访问次数
    void setValue(const Value& value) { value_ = value; }
    void incrementAccessCount() { ++accessCount_; }

    //声明友元类模版LRU（最近最少访问）和LFU（最近访问频率最少），可以直接访问私有成员
    template<typename K, typename V> friend class ArcLruPart;
    template<typename K, typename V> friend class ArcLfuPart;
};

} // namespace Cache