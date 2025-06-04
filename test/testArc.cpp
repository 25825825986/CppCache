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
#include <list>
#include "ArcCache/ArcCache.h"

using namespace Cache;
using namespace std;

// 测试辅助函数
void printTestResult(const string& testName, bool passed) {
    cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << endl;
}

// 测试1: 基本的put和get功能
bool testBasicPutGet() {
    ArcCache<int, string> cache(3);
    
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
    
    // 测试get方法重载
    string directValue = cache.get(1);
    if (directValue != "one") return false;
    
    return true;
}

// 测试2: ARC自适应缓存容量调整
bool testArcCapacityAdaptation() {
    ArcCache<int, string> cache(4, 2); // 容量4，转换阈值2
    
    // 模拟LRU友好的访问模式
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    cache.put(4, "four");
    
    string value;
    // 访问1和2，使其在LRU部分频繁访问
    cache.get(1, value);
    cache.get(2, value);
    
    // 添加新元素，测试淘汰策略
    cache.put(5, "five");
    
    // 验证关键元素仍然存在
    if (!cache.get(1, value) || value != "one") return false;
    if (!cache.get(2, value) || value != "two") return false;
    if (!cache.get(5, value) || value != "five") return false;
    
    return true;
}

// 测试3: LRU到LFU的转换机制
bool testLruToLfuTransition() {
    ArcCache<int, string> cache(3, 2); // 转换阈值为2
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    string value;
    // 多次访问key=1，触发LRU到LFU的转换
    cache.get(1, value); // 访问次数: 1
    cache.get(1, value); // 访问次数: 2，应该转换到LFU部分
    cache.get(1, value); // 访问次数: 3
    
    // 添加新元素测试转换后的行为
    cache.put(4, "four");
    
    // key=1应该因为高频访问而保留
    if (!cache.get(1, value) || value != "one") return false;
    if (!cache.get(4, value) || value != "four") return false;
    
    return true;
}

// 测试4: 幽灵缓存功能
bool testGhostCacheFunctionality() {
    ArcCache<int, string> cache(2, 2);
    
    // 填满缓存
    cache.put(1, "one");
    cache.put(2, "two");
    
    string value;
    cache.get(1, value); // 增加访问频率
    
    // 添加新元素，触发淘汰到幽灵缓存
    cache.put(3, "three");
    cache.put(4, "four");
    
    // 再次访问被淘汰的元素（现在在幽灵缓存中）
    cache.put(2, "two_updated"); // 从幽灵缓存恢复
    
    // 验证幽灵缓存的影响
    if (!cache.get(2, value) || value != "two_updated") return false;
    
    return true;
}

// 测试5: 更新已存在的key
bool testUpdateExistingKey() {
    ArcCache<int, string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    // 更新已存在的key
    cache.put(1, "ONE_UPDATED");
    cache.put(2, "TWO_UPDATED");
    
    string value;
    if (!cache.get(1, value) || value != "ONE_UPDATED") return false;
    if (!cache.get(2, value) || value != "TWO_UPDATED") return false;
    if (!cache.get(3, value) || value != "three") return false;
    
    return true;
}

// 测试6: 零容量缓存
bool testZeroCapacity() {
    ArcCache<int, string> cache(0);
    
    cache.put(1, "one");
    
    string value;
    // 零容量缓存不应该存储任何数据
    if (cache.get(1, value)) return false;
    
    return true;
}

// 测试7: 容量为1的边界测试
bool testCapacityOne() {
    ArcCache<int, string> cache(1, 1);
    
    cache.put(1, "one");
    string value;
    if (!cache.get(1, value) || value != "one") return false;
    
    // 添加第二个项，应该淘汰第一个
    cache.put(2, "two");
    if (cache.get(1, value)) return false;
    if (!cache.get(2, value) || value != "two") return false;
    
    return true;
}

// 测试8: 复杂的混合访问模式
bool testComplexAccessPattern() {
    ArcCache<int, string> cache(5, 3);
    
    // 阶段1: 初始填充
    for (int i = 1; i <= 5; ++i) {
        cache.put(i, "value_" + to_string(i));
    }
    
    string value;
    // 阶段2: 混合访问模式
    // 创建LRU友好的访问模式
    cache.get(1, value);
    cache.get(2, value);
    cache.get(3, value);
    
    // 创建LFU友好的访问模式
    for (int i = 0; i < 5; ++i) {
        cache.get(4, value);
        cache.get(5, value);
    }
    
    // 阶段3: 添加新元素测试自适应行为
    cache.put(6, "value_6");
    cache.put(7, "value_7");
    
    // 验证高频访问的元素被保留
    if (!cache.get(4, value) || value != "value_4") return false;
    if (!cache.get(5, value) || value != "value_5") return false;
    
    return true;
}

// 测试9: 多线程安全性测试
bool testThreadSafety() {
    ArcCache<int, int> cache(1000, 2);
    const int numThreads = 8;
    const int operationsPerThread = 500;
    atomic<bool> testPassed{true};
    atomic<int> completedThreads{0};
    
    vector<thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                random_device rd;
                mt19937 gen(rd() + t); // 为每个线程使用不同的种子
                uniform_int_distribution<> dis(0, 199);
                
                for (int i = 0; i < operationsPerThread; ++i) {
                    int key = dis(gen);
                    int value = key * 10 + t;
                    
                    if (i % 2 == 0) {
                        // put操作
                        cache.put(key, value);
                    } else {
                        // get操作
                        int retrievedValue;
                        cache.get(key, retrievedValue);
                    }
                    
                    // 定期验证操作
                    if (i % 50 == 0) {
                        int testKey = t + 1000;
                        cache.put(testKey, t);
                        int val;
                        if (!cache.get(testKey, val) || val != t) {
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
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    return testPassed.load() && completedThreads.load() == numThreads;
}

// 测试10: 压力测试
bool testStressLoad() {
    ArcCache<int, int> cache(500, 3);
    const int iterations = 5000;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> keyDis(0, 1000);
    uniform_int_distribution<> opDis(0, 99);
    
    int putCount = 0, getCount = 0;
    
    for (int i = 0; i < iterations; ++i) {
        int key = keyDis(gen);
        int op = opDis(gen);
        
        if (op < 60) { // 60% put操作
            cache.put(key, key * 2);
            putCount++;
        } else { // 40% get操作
            int value;
            cache.get(key, value);
            getCount++;
        }
    }
    
    cout << "    压力测试完成 - Put: " << putCount << ", Get: " << getCount << endl;
    return true;
}

// 测试11: 自适应行为验证
bool testAdaptiveBehavior() {
    ArcCache<int, string> cache(6, 2);
    
    // 场景1: LRU友好的工作负载
    for (int round = 0; round < 3; ++round) {
        for (int i = 1; i <= 4; ++i) {
            cache.put(i + round * 10, "value_" + to_string(i + round * 10));
        }
    }
    
    string value;
    // 验证最近的数据更容易命中
    bool recentHit = cache.get(21, value); // 最后一轮的数据
    
    // 场景2: LFU友好的工作负载
    cache.put(100, "frequent");
    for (int i = 0; i < 10; ++i) {
        cache.get(100, value);
    }
    
    // 添加更多数据测试频繁访问的数据是否被保留
    for (int i = 200; i < 210; ++i) {
        cache.put(i, "temp_" + to_string(i));
    }
    
    // 高频访问的数据应该被保留
    bool frequentHit = cache.get(100, value) && value == "frequent";
    
    return recentHit || frequentHit; // 至少一种模式应该工作正常
}

// 测试12: 大量数据测试
bool testLargeDataSet() {
    ArcCache<int, int> cache(1000);
    
    // 添加大量数据
    for (int i = 0; i < 800; ++i) {
        cache.put(i, i * 2);
    }
    
    // 验证部分数据
    for (int i = 700; i < 800; ++i) {
        int value;
        if (!cache.get(i, value) || value != i * 2) {
            return false;
        }
    }
    
    // 添加超出容量的数据
    for (int i = 800; i < 1200; ++i) {
        cache.put(i, i * 2);
    }
    
    // 验证新数据存在
    int value;
    return cache.get(1100, value) && value == 2200;
}

// 测试13: 内存一致性测试
bool testMemoryConsistency() {
    ArcCache<int, string> cache(50);
    
    // 多阶段填充和访问
    vector<int> keys;
    for (int phase = 0; phase < 5; ++phase) {
        for (int i = 0; i < 20; ++i) {
            int key = phase * 20 + i;
            cache.put(key, "phase_" + to_string(phase) + "_" + to_string(i));
            keys.push_back(key);
        }
        
        // 随机访问一些键
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, keys.size() - 1);
        
        for (int j = 0; j < 10; ++j) {
            int randomKey = keys[dis(gen)];
            string value;
            cache.get(randomKey, value);
        }
    }
    
    // 验证缓存仍然一致
    string value;
    int successCount = 0;
    for (int i = 80; i < 100; ++i) { // 检查最后阶段的数据
        if (cache.get(i, value)) {
            string expected = "phase_4_" + to_string(i - 80);
            if (value == expected) {
                successCount++;
            }
        }
    }
    
    return successCount > 10; // 至少一半的数据应该仍然正确
}

// 性能测试
void performanceTest() {
    cout << "\n=== ARC缓存性能测试 ===" << endl;
    
    const int cacheSize = 5000;
    const int operations = 50000;
    
    ArcCache<int, int> cache(cacheSize, 3);
    
    auto start = chrono::high_resolution_clock::now();
    
    // 混合操作模式
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> keyDis(0, cacheSize * 2);
    uniform_int_distribution<> opDis(0, 99);
    
    for (int i = 0; i < operations; ++i) {
        int key = keyDis(gen);
        int op = opDis(gen);
        
        if (op < 70) { // 70% put操作
            cache.put(key, key * 2);
        } else { // 30% get操作
            int value;
            cache.get(key, value);
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "执行 " << operations << " 次混合操作耗时: " << duration.count() << " ms" << endl;
    cout << "平均每次操作耗时: " << (double)duration.count() / operations << " ms" << endl;
    cout << "操作吞吐量: " << (operations * 1000) / duration.count() << " ops/sec" << endl;
}

int main() {
    cout << "开始ARC自适应缓存测试..." << endl;
    cout << "=========================" << endl;
    
    vector<pair<string, function<bool()>>> tests = {
        {"基本Put/Get功能", testBasicPutGet},
        {"ARC容量自适应调整", testArcCapacityAdaptation},
        {"LRU到LFU转换机制", testLruToLfuTransition},
        {"幽灵缓存功能", testGhostCacheFunctionality},
        {"更新已存在的key", testUpdateExistingKey},
        {"零容量缓存", testZeroCapacity},
        {"容量为1边界测试", testCapacityOne},
        {"复杂混合访问模式", testComplexAccessPattern},
        {"多线程安全性", testThreadSafety},
        {"压力负载测试", testStressLoad},
        {"自适应行为验证", testAdaptiveBehavior},
        {"大量数据测试", testLargeDataSet},
        {"内存一致性测试", testMemoryConsistency}
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
        
        // 运行性能测试
        performanceTest();
    } else {
        cout << "有 " << (totalTests - passedTests) << " 个测试需要检查! " << endl;
    }
    
    return passedTests == totalTests ? 0 : 1;
}