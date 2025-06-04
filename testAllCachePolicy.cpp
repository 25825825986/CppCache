#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

#include "CachePolicy.h"
#include "LFUCache.h"
#include "LRUCache.h"
#include "ArcCache/ArcCache.h"

using namespace std;
using namespace Cache;

class Timer {
public:
    Timer() : start_(chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto now = chrono::high_resolution_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(now - start_).count();
    }

private:
    chrono::time_point<chrono::high_resolution_clock> start_;
};

// 辅助函数：打印结果
void printResults(const string& testName, int capacity, 
                 const vector<int>& get_operations, 
                 const vector<int>& hits) {
    cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    cout << "缓存大小: " << capacity << std::endl;
    
    // 对应的算法名称已在测试函数中定义
    vector<string> names;
    if (hits.size() == 3) {
        names = {"LRU", "LFU", "ARC"};
    } else if (hits.size() == 4) {
        names = {"LRU", "LFU", "ARC", "LRU-K"};
    } else if (hits.size() == 5) {
        names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};
    }
    
    for (size_t i = 0; i < hits.size(); ++i) {
        double hitRate = 100.0 * hits[i] / get_operations[i];
        cout << (i < names.size() ? names[i] : "Algorithm " + to_string(i+1)) 
                  << " - 命中率: " << fixed << setprecision(2) 
                  << hitRate << "% ";
        // 添加具体命中次数和总操作次数
        cout << "(" << hits[i] << "/" << get_operations[i] << ")" << endl;
    }
    
    cout << endl;  // 添加空行，使输出更清晰
}

void testHotDataAccess() {
    cout << "\n=== 测试场景1：热点数据访问测试（优化版） ===" << endl;

    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int MID_KEYS = 100;      // 新增中等热度区间
    const int COLD_KEYS = 1000;    // 缩小冷数据范围

    LRUCache<int, string> lru(CAPACITY);
    LFUCache<int, string> lfu(CAPACITY);
    ArcCache<int, string> arc(CAPACITY);
    KLruKCache<int, string> lruk(CAPACITY, HOT_KEYS + MID_KEYS + COLD_KEYS, 2);
    LFUCache<int, string> lfuAging(CAPACITY, 20000);

    random_device rd;
    mt19937 gen(rd());

    array<CachePolicy<int, string>*, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    vector<int> hits(5, 0);
    vector<int> get_operations(5, 0);

    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < HOT_KEYS; ++key) {
            string value = "value" + to_string(key);
            caches[i]->put(key, value);
        }

        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen() % 100 < 20); // 降低写操作比例
            int key;
            int r = gen() % 100;
            if (r < 60) {
                key = gen() % HOT_KEYS; // 60%热点
            } else if (r < 80) {
                key = HOT_KEYS + (gen() % MID_KEYS); // 20%中等热度
            } else {
                key = HOT_KEYS + MID_KEYS + (gen() % COLD_KEYS); // 20%冷数据
            }

            if (isPut) {
                string value = "value" + to_string(key) + "_v" + to_string(op % 100);
                caches[i]->put(key, value);
            } else {
                string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("热点数据访问测试（优化版）", CAPACITY, get_operations, hits);
}

void testLoopPattern() {
    cout << "\n=== 测试场景2：循环扫描测试（优化版） ===" << endl;

    const int CAPACITY = 100;         // 增大缓存容量
    const int LOOP_SIZE = 200;        // 减小循环区间
    const int OPERATIONS = 200000;    // 总操作次数
    const int HOT_REGION = 40;        // 热点区间大小

    LRUCache<int, string> lru(CAPACITY);
    LFUCache<int, string> lfu(CAPACITY);
    ArcCache<int, string> arc(CAPACITY);
    KLruKCache<int, string> lruk(CAPACITY, LOOP_SIZE * 2, 2);
    LFUCache<int, string> lfuAging(CAPACITY, 3000);

    array<CachePolicy<int, string>*, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    vector<int> hits(5, 0);
    vector<int> get_operations(5, 0);
    vector<string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};

    random_device rd;
    mt19937 gen(rd());

    for (int i = 0; i < caches.size(); ++i) {
        // 预热缓存
        for (int key = 0; key < CAPACITY; ++key) {
            string value = "loop" + to_string(key);
            caches[i]->put(key, value);
        }

        int current_pos = 0;

        for (int op = 0; op < OPERATIONS; ++op) {
            // 10%写操作，90%读操作
            bool isPut = (gen() % 100 < 10);
            int key;

            // 70%概率访问热点区间，20%顺序扫描，10%范围外
            int mode = gen() % 100;
            if (mode < 70) {
                key = gen() % HOT_REGION; // 热点区间
            } else if (mode < 90) {
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else {
                key = LOOP_SIZE + (gen() % LOOP_SIZE); // 范围外
            }

            if (isPut) {
                string value = "loop" + to_string(key) + "_v" + to_string(op % 100);
                caches[i]->put(key, value);
            } else {
                string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("循环扫描测试（优化版）", CAPACITY, get_operations, hits);
}

void testWorkloadShift() {
    cout << "\n=== 测试场景3：工作负载剧烈变化测试（优化版） ===" << endl;

    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LENGTH = OPERATIONS / 5;

    LRUCache<int, string> lru(CAPACITY);
    LFUCache<int, string> lfu(CAPACITY);
    ArcCache<int, string> arc(CAPACITY);
    KLruKCache<int, string> lruk(CAPACITY, 500, 2);
    LFUCache<int, string> lfuAging(CAPACITY, 10000);

    random_device rd;
    mt19937 gen(rd());
    array<CachePolicy<int, string>*, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    vector<int> hits(5, 0);
    vector<int> get_operations(5, 0);

    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < 30; ++key) {
            string value = "init" + to_string(key);
            caches[i]->put(key, value);
        }

        for (int op = 0; op < OPERATIONS; ++op) {
            int phase = op / PHASE_LENGTH;
            int putProbability;
            int key;
            switch (phase) {
                case 0: // 热点访问
                    putProbability = 15;
                    key = gen() % 5;
                    break;
                case 1: // 热点迁移
                    putProbability = 20;
                    key = 5 + (gen() % 5); // 新热点区间
                    break;
                case 2: // 大范围随机
                    putProbability = 30;
                    key = gen() % 400;
                    break;
                case 3: // 局部性随机
                    putProbability = 20;
                    key = (gen() % 5) * 10 + (gen() % 10);
                    break;
                case 4: // 混合
                default:
                    putProbability = 20;
                    int r = gen() % 100;
                    if (r < 40) key = gen() % 5;
                    else if (r < 70) key = 10 + (gen() % 40);
                    else key = 50 + (gen() % 350);
            }

            bool isPut = (gen() % 100 < putProbability);

            if (isPut) {
                string value = "value" + to_string(key) + "_p" + to_string(phase);
                caches[i]->put(key, value);
            } else {
                string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("工作负载剧烈变化测试（优化版）", CAPACITY, get_operations, hits);
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}