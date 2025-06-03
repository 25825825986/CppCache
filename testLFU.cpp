#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include "LFUCache.h"

using namespace Cache;
using namespace std;

// 测试辅助函数
void printTestResult(const string& testName, bool passed) {
    cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << endl;
}

// 测试1: 基本的put和get功能
bool testBasicPutGet() {
    LFUCache<int, string> cache(3);
    
    // 添加缓存项
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 测试get功能
    string value;
    bool found = cache.get(1, value);
    if (!found || value != "one") return false;
    
    found = cache.get(2, value);
    if (!found || value != "two") return false;
    
    found = cache.get(3, value);
    if (!found || value != "three") return false;
    
    // 测试不存在的key
    found = cache.get(4, value);
    if (found) return false;
    
    return true;
}

// 测试2: 缓存容量限制和LFU淘汰策略
bool testLFUEviction() {
    LFUCache<int, string> cache(3);
    
    // 添加3个缓存项
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 访问key=1多次，增加其频率
    string value;
    cache.get(1, value);
    cache.get(1, value);
    cache.get(1, value);
    
    // 访问key=2一次
    cache.get(2, value);
    
    // key=3没有被访问，频率最低
    
    // 添加第4个缓存项，应该淘汰key=3
    cache.put(4, "four");
    
    // 验证key=3被淘汰
    bool found = cache.get(3, value);
    if (found) return false;
    
    // 验证其他缓存项仍然存在
    found = cache.get(1, value);
    if (!found || value != "one") return false;
    
    found = cache.get(2, value);
    if (!found || value != "two") return false;
    
    found = cache.get(4, value);
    if (!found || value != "four") return false;
    
    return true;
}

// 测试3: 更新已存在的key
bool testUpdateExistingKey() {
    LFUCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    
    // 更新已存在的key
    cache.put(1, "ONE");
    
    string value;
    bool found = cache.get(1, value);
    if (!found || value != "ONE") return false;
    
    return true;
}

// 测试4: 零容量缓存
bool testZeroCapacity() {
    LFUCache<int, string> cache(0);
    
    cache.put(1, "one");
    
    string value;
    bool found = cache.get(1, value);
    if (found) return false;  // 零容量缓存不应该存储任何数据
    
    return true;
}

// 测试5: 大量数据测试
bool testLargeDataSet() {
    LFUCache<int, int> cache(1000);
    
    // 添加1000个缓存项
    for (int i = 0; i < 1000; ++i) {
        cache.put(i, i * 2);
    }
    
    // 验证所有缓存项都存在
    for (int i = 0; i < 1000; ++i) {
        int value;
        bool found = cache.get(i, value);
        if (!found || value != i * 2) return false;
    }
    
    // 添加超出容量的缓存项
    cache.put(1000, 2000);
    
    // 验证新项存在
    int value;
    bool found = cache.get(1000, value);
    if (!found || value != 2000) return false;
    
    return true;
}

// 测试6: 复杂的LFU场景
bool testComplexLFUScenario() {
    LFUCache<int, string> cache(3);
    
    // 添加3个缓存项
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    string value;
    
    // 访问模式：key=1访问5次，key=2访问3次，key=3访问1次
    for (int i = 0; i < 5; ++i) cache.get(1, value);
    for (int i = 0; i < 3; ++i) cache.get(2, value);
    cache.get(3, value);
    
    // 添加新项，应该淘汰访问频率最低的key=3
    cache.put(4, "four");
    
    bool found = cache.get(3, value);
    if (found) return false;  // key=3应该被淘汰
    
    // 再添加一项，应该淘汰key=2
    cache.put(5, "five");
    
    found = cache.get(2, value);
    if (found) return false;  // key=2应该被淘汰
    
    // key=1应该仍然存在
    found = cache.get(1, value);
    if (!found || value != "one") return false;
    
    return true;
}

// 测试7: 多线程安全性测试
bool testThreadSafety() {
    LFUCache<int, int> cache(1000);
    const int numThreads = 10;
    const int operationsPerThread = 100;
    atomic<bool> testPassed{true};
    
    vector<thread> threads;
    
    // 创建多个线程同时进行put和get操作
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&cache, &testPassed, t, operationsPerThread]() {
            try {
                for (int i = 0; i < operationsPerThread; ++i) {
                    int key = t * operationsPerThread + i;
                    int value = key * 2;
                    
                    // put操作
                    cache.put(key, value);
                    
                    // get操作
                    int retrievedValue;
                    bool found = cache.get(key, retrievedValue);
                    if (!found || retrievedValue != value) {
                        testPassed = false;
                        return;
                    }
                }
            } catch (...) {
                testPassed = false;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    return testPassed.load();
}

// 测试8: KHashLfuCache分片缓存测试
bool testKHashLfuCache() {
    KHashLfuCache<int, string> cache(100, 4);  // 总容量100，4个分片
    
    // 添加缓存项
    for (int i = 0; i < 50; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    // 验证缓存项
    for (int i = 0; i < 50; ++i) {
        string value;
        bool found = cache.get(i, value);
        if (!found || value != "value_" + to_string(i)) return false;
    }
    
    // 测试超出容量的情况
    for (int i = 50; i < 150; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    // 验证新添加的项存在
    string value;
    bool found = cache.get(149, value);
    if (!found || value != "value_149") return false;
    
    return true;
}

// 测试9: purge功能测试
bool testPurgeFunction() {
    LFUCache<int, string> cache(5);
    
    // 添加缓存项
    for (int i = 0; i < 5; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    // 验证缓存项存在
    string value;
    bool found = cache.get(0, value);
    if (!found) return false;
    
    // 清空缓存
    cache.purge();
    
    // 验证缓存项被清空
    found = cache.get(0, value);
    if (found) return false;
    
    // 重新添加项验证缓存仍然可用
    cache.put(10, "ten");
    found = cache.get(10, value);
    if (!found || value != "ten") return false;
    
    return true;
}

// 测试10: get方法重载测试
bool testGetOverload() {
    LFUCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    
    // 测试返回值版本的get方法
    string value1 = cache.get(1);
    if (value1 != "one") return false;
    
    string value2 = cache.get(2);
    if (value2 != "two") return false;
    
    // 测试不存在的key（应该返回默认构造的值）
    string value3 = cache.get(999);
    if (!value3.empty()) return false;  // 对于string，默认构造应该是空字符串
    
    return true;
}

// 性能测试
void performanceTest() {
    cout << "\n=== 性能测试 ===" << endl;
    
    const int cacheSize = 10000;
    const int operations = 100000;
    
    LFUCache<int, int> cache(cacheSize);
    
    auto start = chrono::high_resolution_clock::now();
    
    // 执行大量put操作
    for (int i = 0; i < operations; ++i) {
        cache.put(i % (cacheSize * 2), i);  // 故意超出容量以测试淘汰策略
    }
    
    // 执行大量get操作
    int value;
    for (int i = 0; i < operations; ++i) {
        cache.get(i % (cacheSize * 2), value);
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "执行 " << operations * 2 << " 次操作耗时: " << duration.count() << " ms" << endl;
    cout << "平均每次操作耗时: " << (double)duration.count() / (operations * 2) << " ms" << endl;
}

int main() {
    cout << "开始LFU缓存测试..." << endl;
    cout << "===================" << endl;
    
    vector<pair<string, function<bool()>>> tests = {
        {"基本put和get功能", testBasicPutGet},
        {"LFU淘汰策略", testLFUEviction},
        {"更新已存在的key", testUpdateExistingKey},
        {"零容量缓存", testZeroCapacity},
        {"大量数据测试", testLargeDataSet},
        {"复杂LFU场景", testComplexLFUScenario},
        {"多线程安全性", testThreadSafety},
        {"KHashLfuCache分片缓存", testKHashLfuCache},
        {"purge功能", testPurgeFunction},
        {"get方法重载", testGetOverload}
    };
    
    int passedTests = 0;
    int totalTests = tests.size();
    
    for (const auto& test : tests) {
        try {
            bool result = test.second();
            printTestResult(test.first, result);
            if (result) passedTests++;
        } catch (const exception& e) {
            printTestResult(test.first + " (异常: " + e.what() + ")", false);
        } catch (...) {
            printTestResult(test.first + " (未知异常)", false);
        }
    }
    
    cout << "\n===================" << endl;
    cout << "测试结果: " << passedTests << "/" << totalTests << " 通过" << endl;
    
    if (passedTests == totalTests) {
        cout << "所有测试通过! ✓" << endl;
        
        // 运行性能测试
        performanceTest();
    } else {
        cout << "有 " << (totalTests - passedTests) << " 个测试失败! ✗" << endl;
    }
    
    return passedTests == totalTests ? 0 : 1;
}