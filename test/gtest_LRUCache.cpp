#include <gtest/gtest.h>
#include "LRUCache.h"

using namespace Cache;

TEST(LRUCacheTest, BasicPutGet) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");
    std::string value;
    EXPECT_TRUE(cache.get(1, value));
    EXPECT_EQ(value, "one");
    EXPECT_TRUE(cache.get(2, value));
    EXPECT_EQ(value, "two");
}

TEST(LRUCacheTest, Eviction) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three"); // 应淘汰key=1
    std::string value;
    EXPECT_FALSE(cache.get(1, value));
    EXPECT_TRUE(cache.get(2, value));
    EXPECT_TRUE(cache.get(3, value));
}