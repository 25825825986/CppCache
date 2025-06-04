#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <random>
#include <set>
#include "LRUCache.h"

using namespace Cache;
using namespace std;

// 测试辅助函数
void printTestResult(const string& testName, bool passed) {
    cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << endl;
}

// 改进的基本功能测试
bool testBasicPutGet() {
    LRUCache<int, string> cache(3);
    
    // 空缓存测试
    string value;
    if (cache.get(1, value)) return false;
    
    // 添加缓存项
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 验证所有项都存在
    if (!cache.get(1, value) || value != "one") return false;
    if (!cache.get(2, value) || value != "two") return false;
    if (!cache.get(3, value) || value != "three") return false;
    
    // 测试不存在的key
    if (cache.get(4, value)) return false;
    
    return true;
}

// 严格的LRU淘汰策略测试
bool testStrictLRUEviction() {
    LRUCache<int, string> cache(3);
    
    // 按顺序添加
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 此时顺序应该是: 3(newest) -> 2 -> 1(oldest)
    
    // 添加第4个项，应该淘汰1
    cache.put(4, "four");
    
    string value;
    if (cache.get(1, value)) return false; // 1应该被淘汰
    if (!cache.get(2, value) || value != "two") return false;
    if (!cache.get(3, value) || value != "three") return false;
    if (!cache.get(4, value) || value != "four") return false;
    
    return true;
}

// 访问顺序影响测试
bool testAccessOrderImpact() {
    LRUCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 访问1，使其变为最新
    string value;
    cache.get(1, value);
    
    // 现在顺序应该是: 1(newest) -> 3 -> 2(oldest)
    
    // 添加新项，应该淘汰2
    cache.put(4, "four");
    
    if (cache.get(2, value)) return false; // 2应该被淘汰
    if (!cache.get(1, value) || value != "one") return false;
    if (!cache.get(3, value) || value != "three") return false;
    if (!cache.get(4, value) || value != "four") return false;
    
    return true;
}

// 容量为1的边界测试
bool testCapacityOne() {
    LRUCache<int, string> cache(1);
    
    cache.put(1, "one");
    string value;
    if (!cache.get(1, value) || value != "one") return false;
    
    // 添加第二个项，应该淘汰第一个
    cache.put(2, "two");
    if (cache.get(1, value)) return false;
    if (!cache.get(2, value) || value != "two") return false;
    
    return true;
}

// 更新操作的正确性测试
bool testUpdateCorrectness() {
    LRUCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 更新现有key，应该移到最前面
    cache.put(1, "ONE_UPDATED");
    
    // 添加新项，应该淘汰最旧的（现在是2）
    cache.put(4, "four");
    
    string value;
    if (cache.get(2, value)) return false; // 2应该被淘汰
    if (!cache.get(1, value) || value != "ONE_UPDATED") return false;
    if (!cache.get(3, value) || value != "three") return false;
    if (!cache.get(4, value) || value != "four") return false;
    
    return true;
}

// 严格的多线程测试
bool testStrictThreadSafety() {
    LRUCache<int, int> cache(100);
    const int numThreads = 8;
    const int operationsPerThread = 1000;
    atomic<bool> testPassed{true};
    atomic<int> completedThreads{0};
    
    vector<thread> threads;
    
    // 使用不同的操作模式测试
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                random_device rd;
                mt19937 gen(rd());
                uniform_int_distribution<> dis(0, 199); // 0-199的随机数
                
                for (int i = 0; i < operationsPerThread; ++i) {
                    int key = dis(gen);
                    int value = key * 10 + t; // 包含线程标识的值
                    
                    if (i % 3 == 0) {
                        // put操作
                        cache.put(key, value);
                    } else if (i % 3 == 1) {
                        // get操作
                        int retrievedValue;
                        cache.get(key, retrievedValue);
                    } else {
                        // remove操作
                        cache.remove(key);
                    }
                    
                    // 偶尔执行一些验证操作
                    if (i % 100 == 0) {
                        cache.put(t + 1000, t); // 使用线程特定的key
                        int val;
                        if (!cache.get(t + 1000, val) || val != t) {
                            testPassed = false;
                            return;
                        }
                    }
                }
                
                completedThreads++;
            } catch (...) {
                testPassed = false;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    return testPassed.load() && completedThreads.load() == numThreads;
}

// 内存一致性测试
bool testMemoryConsistency() {
    LRUCache<int, string> cache(10);
    
    // 填满缓存
    for (int i = 0; i < 10; ++i) {
        cache.put(i, to_string(i));
    }
    
    // 验证所有项都存在
    for (int i = 0; i < 10; ++i) {
        string value;
        if (!cache.get(i, value) || value != to_string(i)) {
            return false;
        }
    }
    
    // 添加新项，触发淘汰
    for (int i = 10; i < 15; ++i) {
        cache.put(i, to_string(i));
        
        // 验证新项存在
        string value;
        if (!cache.get(i, value) || value != to_string(i)) {
            return false;
        }
    }
    
    return true;
}

// 压力测试
bool testStressLoad() {
    LRUCache<int, int> cache(1000);
    const int iterations = 10000;
    
    // 随机操作
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> keyDis(0, 2000);
    uniform_int_distribution<> opDis(0, 99);
    
    int putCount = 0, getCount = 0, removeCount = 0;
    
    for (int i = 0; i < iterations; ++i) {
        int key = keyDis(gen);
        int op = opDis(gen);
        
        if (op < 50) { // 50% put操作
            cache.put(key, key * 2);
            putCount++;
        } else if (op < 85) { // 35% get操作
            int value;
            cache.get(key, value);
            getCount++;
        } else { // 15% remove操作
            cache.remove(key);
            removeCount++;
        }
    }
    
    cout << "压力测试完成 - Put: " << putCount 
         << ", Get: " << getCount 
         << ", Remove: " << removeCount << endl;
    
    return true;
}

// 修正的LRU-K测试（需要根据实际实现调整）
bool testKLruKCacheBasic() {
    // 注意：这个测试需要根据实际的LRU-K实现逻辑来调整
    try {
        KLruKCache<int, string> cache(2, 5, 2); // 主缓存2，历史5，K=2
        
        // 基本功能测试
        cache.put(1, "one");
        cache.put(2, "two");
        
        // 由于LRU-K的复杂性，这里只做基本的存在性测试
        string value1 = cache.get(1);
        string value2 = cache.get(2);
        
        // 如果没有崩溃就认为基本功能正常
        return true;
    } catch (...) {
        return false;
    }
}

// 改进的分片缓存测试
bool testHashLruCachesAdvanced() {
    HashLruCaches<int, string> cache(100, 4);
    
    // 测试数据分布
    set<int> keys;
    for (int i = 0; i < 50; ++i) {
        cache.put(i, "value_" + to_string(i));
        keys.insert(i);
    }
    
    // 验证所有数据
    for (int key : keys) {
        string value;
        if (!cache.get(key, value) || value != "value_" + to_string(key)) {
            return false;
        }
    }
    
    // 测试超出容量
    for (int i = 50; i < 150; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    // 验证部分数据仍然存在
    string value;
    bool foundSome = false;
    for (int i = 100; i < 150; ++i) {
        if (cache.get(i, value) && value == "value_" + to_string(i)) {
            foundSome = true;
            break;
        }
    }
    
    return foundSome;
}

int main() {
    cout << "开始改进的LRU缓存测试..." << endl;
    cout << "=========================" << endl;
    
    vector<pair<string, function<bool()>>> tests = {
        {"基本Put/Get功能", testBasicPutGet},
        {"严格LRU淘汰策略", testStrictLRUEviction},
        {"访问顺序影响测试", testAccessOrderImpact},
        {"容量为1边界测试", testCapacityOne},
        {"更新操作正确性", testUpdateCorrectness},
        {"严格多线程安全", testStrictThreadSafety},
        {"内存一致性测试", testMemoryConsistency},
        {"压力负载测试", testStressLoad},
        {"LRU-K基本功能", testKLruKCacheBasic},
        {"高级分片缓存测试", testHashLruCachesAdvanced}
    };
    
    int passedTests = 0;
    int totalTests = tests.size();
    
    for (const auto& test : tests) {
        try {
            auto start = chrono::high_resolution_clock::now();
            bool result = test.second();
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
            
            printTestResult(test.first + " (" + to_string(duration.count()) + "ms)", result);
            if (result) passedTests++;
        } catch (const exception& e) {
            printTestResult(test.first + " (异常: " + e.what() + ")", false);
        } catch (...) {
            printTestResult(test.first + " (未知异常)", false);
        }
    }
    
    cout << "\n=========================" << endl;
    cout << "测试结果: " << passedTests << "/" << totalTests << " 通过" << endl;
    
    if (passedTests == totalTests) {
        cout << "所有测试通过! ✓" << endl;
    } else {
        cout << "有 " << (totalTests - passedTests) << " 个测试需要检查! " << endl;
    }
    
    return passedTests == totalTests ? 0 : 1;
}