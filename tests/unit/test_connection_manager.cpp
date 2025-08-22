#include "core/connection_manager.h"
#include <gtest/gtest.h>

using namespace scratchrobin;

TEST(ConnectionManagerTest, Construction) {
    EXPECT_NO_THROW(ConnectionManager manager);
}

TEST(ConnectionManagerTest, PoolSize) {
    ConnectionManager manager;
    EXPECT_EQ(manager.getPoolSize(), 0);
    EXPECT_EQ(manager.getActiveConnections(), 0);
}
