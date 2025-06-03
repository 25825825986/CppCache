#include <iostream>
#include <cassert>
#include "ArcCache/ArcCache.h"

using namespace Cache;
using namespace std;

void basicTest() {
    ArcCache<int, string> cache(3);

    // 基本插入
    cache.put(1, "a");
    cache.put(2, "b");
    cache.put(3, "c");

    string v;
    assert(cache.get(1, v) && v == "a");
    assert(cache.get(2, v) && v == "b");
    assert(cache.get(3, v) && v == "c");

    // 超容量插入，触发淘汰
    cache.put(4, "d");
    int hitCount = 0;
    hitCount += cache.get(1, v); // 1 可能被淘汰
    hitCount += cache.get(2, v); // 2 可能被淘汰
    hitCount += cache.get(3, v); // 3 可能被淘汰
    hitCount += cache.get(4, v); // 4 一定在
    assert(hitCount == 3); // 只会有3个命中

    cout << "basicTest passed." << endl;
}

void lruLfuSwitchTest() {
    ArcCache<int, string> cache(2);

    // 模拟LRU友好模式
    cache.put(1, "a");
    cache.put(2, "b");
    string v;
    cache.get(1, v);
    cache.put(3, "c"); // 淘汰2
    assert(!cache.get(2, v));
    assert(cache.get(1, v));
    assert(cache.get(3, v));

    // 模拟LFU友好模式
    for (int i = 0; i < 10; ++i) cache.get(1, v);
    cache.put(4, "d"); // 淘汰3
    assert(!cache.get(3, v));
    assert(cache.get(1, v));
    assert(cache.get(4, v));

    cout << "lruLfuSwitchTest passed." << endl;
}

int main() {
    basicTest();
    lruLfuSwitchTest();
    cout << "All ArcCache tests passed!" << endl;
    return 0;
}