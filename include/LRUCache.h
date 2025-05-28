#pragma once 

#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cmath>
#include <thread>

#include "CachePolicy.h"

namespace Cache
{

// LRU缓存实现

template<typename Key, typename Value>
class LRUCache : public CachePolicy<Key, Value>
{
public:
    using Node = std::pair<Key, Value>;
    using ListIterator = typename std::list<Node>::iterator;

    LRUCache(int capacity)
        : capacity_(capacity) {}

    ~LRUCache() override = default;

    void put(Key key, Value value) override
    {
        if (capacity_ <= 0) return;

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end()) {
            // 存在则更新并移到前面
            it->second->second = value;
            cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
        } else {
            // 不存在则新建
            if (cacheList_.size() >= capacity_) {
                // 删除最久未使用元素
                auto del = cacheList_.back();
                cacheMap_.erase(del.first);
                cacheList_.pop_back();
            }
            cacheList_.emplace_front(key, value);
            cacheMap_[key] = cacheList_.begin();
        }
    }

    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cacheMap_.find(key);
        if (it == cacheMap_.end()) return false;

        // 将节点移动到链表头部
        cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
        value = it->second->second;
        return true;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

    void remove(Key key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end()) {
            cacheList_.erase(it->second);
            cacheMap_.erase(it);
        }
    }

private:
    int capacity_;
    std::list<Node> cacheList_; // 双向链表：头部是最近访问，尾部是最久未访问
    std::unordered_map<Key, ListIterator> cacheMap_; // key -> list迭代器
    std::mutex mutex_;
};

// LRU-k缓存

template<typename Key, typename Value>
class KLruKCache : public LRUCache<Key, Value>
{
public:
    KLruKCache(int capacity, int historyCapacity, int k)
        : LRUCache<Key, Value>(capacity),
          historyList_(std::make_unique<LRUCache<Key, size_t>>(historyCapacity)),
          k_(k) {}

    Value get(Key key)
    {
        Value value{};
        bool inMain = LRUCache<Key, Value>::get(key, value);

        size_t count = historyList_->get(key);
        ++count;
        historyList_->put(key, count);

        if (inMain) return value;

        if (count >= k_) {
            auto it = historyValueMap_.find(key);
            if (it != historyValueMap_.end()) {
                Value storedValue = it->second;
                historyList_->remove(key);
                historyValueMap_.erase(it);
                LRUCache<Key, Value>::put(key, storedValue);
                return storedValue;
            }
        }

        return value;
    }

    void put(Key key, Value value)
    {
        Value existing;
        if (LRUCache<Key, Value>::get(key, existing)) {
            LRUCache<Key, Value>::put(key, value);
            return;
        }

        size_t count = historyList_->get(key);
        ++count;
        historyList_->put(key, count);

        historyValueMap_[key] = value;

        if (count >= k_) {
            historyList_->remove(key);
            historyValueMap_.erase(key);
            LRUCache<Key, Value>::put(key, value);
        }
    }

private:
    int k_;
    std::unique_ptr<LRUCache<Key, size_t>> historyList_;
    std::unordered_map<Key, Value> historyValueMap_;
};

// 分片 LRU

template<typename Key, typename Value>
class HashLruCaches
{
public:
    HashLruCaches(size_t capacity, int sliceNum)
        : capacity_(capacity),
          sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
    {
        size_t sliceSize = std::ceil(capacity / static_cast<double>(sliceNum_));
        for (int i = 0; i < sliceNum_; ++i) {
            lruSliceCaches_.emplace_back(new LRUCache<Key, Value>(sliceSize));
        }
    }

    void put(Key key, Value value)
    {
        size_t index = hash(key) % sliceNum_;
        lruSliceCaches_[index]->put(key, value);
    }

    bool get(Key key, Value& value)
    {
        size_t index = hash(key) % sliceNum_;
        return lruSliceCaches_[index]->get(key, value);
    }

    Value get(Key key)
    {
        Value value{};
        get(key, value);
        return value;
    }

private:
    size_t hash(Key key)
    {
        return std::hash<Key>{}(key);
    }

private:
    size_t capacity_;
    int sliceNum_;
    std::vector<std::unique_ptr<LRUCache<Key, Value>>> lruSliceCaches_;
};

} // namespace Cache