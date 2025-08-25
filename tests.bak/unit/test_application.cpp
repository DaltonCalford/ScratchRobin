#include "core/application.h"
#include "core/connection_manager.h"
#include "core/metadata_manager.h"
#include <gtest/gtest.h>

using namespace scratchrobin;

TEST(ApplicationTest, Construction) {
    ConnectionManager connManager;
    MetadataManager metadataManager(&connManager);
    EXPECT_NO_THROW(Application app(&connManager, &metadataManager));
}
