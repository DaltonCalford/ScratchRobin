/**
 * @file test_project_git_sync.cpp
 * @brief Lightweight tests for Git sync behavior
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <random>

#include "core/project.h"

using namespace scratchrobin;

class ProjectGitSyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto base = std::filesystem::temp_directory_path();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(10000, 99999);
        temp_dir_ = base / ("scratchrobin_git_sync_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir_);
    }

    std::filesystem::path temp_dir_;
};

TEST_F(ProjectGitSyncTest, SyncToDatabaseInitializesRepository) {
    Project project;
    ProjectConfig cfg;
    cfg.name = "Git Sync Test";
    cfg.description = "Test Git sync";
    cfg.version = "1.0";
    cfg.database_type = "scratchbird";
    cfg.git.enabled = true;

    ASSERT_TRUE(project.CreateNew(temp_dir_.string(), cfg));

    auto obj = project.CreateObject("table", "orders", "public");
    ASSERT_TRUE(obj != nullptr);

    ASSERT_TRUE(project.SyncToDatabase());

    EXPECT_TRUE(std::filesystem::exists(temp_dir_ / ".git"));
    EXPECT_FALSE(project.sync_state.project_repo.head_commit.empty());

    auto events = project.GetStatusEvents();
    ASSERT_FALSE(events.empty());
    EXPECT_FALSE(events.back().message.empty());
}
