#include "core/metadata_manager.h"
#include "core/connection_manager.h"
#include <gtest/gtest.h>

using namespace scratchrobin;

TEST(MetadataManagerTest, Construction) {
    ConnectionManager connManager;
    EXPECT_NO_THROW(MetadataManager metadataManager(&connManager));
}
