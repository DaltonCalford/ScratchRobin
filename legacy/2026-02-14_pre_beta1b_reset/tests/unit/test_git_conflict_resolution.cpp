/**
 * @file test_git_conflict_resolution.cpp
 * @brief Conflict-focused Git tests (ours/theirs)
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <random>

#include "core/git_client.h"

using namespace scratchrobin;

namespace {

std::filesystem::path MakeTempDir(const std::string& prefix) {
    auto base = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10000, 99999);
    auto dir = base / (prefix + std::to_string(dist(gen)));
    std::filesystem::create_directories(dir);
    return dir;
}

void WriteFile(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << content;
}

} // namespace

TEST(GitConflictResolutionTest, CheckoutOursResolvesConflict) {
    auto repo = MakeTempDir("scratchrobin_git_conflict_ours_");

    GitClient git;
    ASSERT_TRUE(git.InitRepository(repo.string()));
    git.SetConfig("user.name", "ScratchRobin Test");
    git.SetConfig("user.email", "test@example.com");

    auto base_branch = git.GetCurrentBranch();
    ASSERT_TRUE(base_branch.has_value());

    auto file_path = repo / "designs/public.orders.table.json";
    WriteFile(file_path, "base\n");
    ASSERT_TRUE(git.AddAll().success);
    ASSERT_TRUE(git.Commit("base").success);

    ASSERT_TRUE(git.CreateBranch("feature").success);
    ASSERT_TRUE(git.CheckoutBranch("feature").success);
    WriteFile(file_path, "theirs\n");
    ASSERT_TRUE(git.AddAll().success);
    ASSERT_TRUE(git.Commit("theirs").success);

    ASSERT_TRUE(git.CheckoutBranch(base_branch->name).success);
    WriteFile(file_path, "ours\n");
    ASSERT_TRUE(git.AddAll().success);
    ASSERT_TRUE(git.Commit("ours").success);

    git.MergeBranch("feature");

    auto conflicts = git.GetConflictedFiles();
    ASSERT_FALSE(conflicts.empty());

    ASSERT_TRUE(git.CheckoutOurs("designs/public.orders.table.json").success);
    ASSERT_TRUE(git.MarkResolved("designs/public.orders.table.json").success);

    auto conflicts_after = git.GetConflictedFiles();
    EXPECT_TRUE(conflicts_after.empty());

    std::ifstream in(file_path);
    std::string content;
    std::getline(in, content);
    EXPECT_EQ(content, "ours");
}

TEST(GitConflictResolutionTest, CheckoutTheirsResolvesConflict) {
    auto repo = MakeTempDir("scratchrobin_git_conflict_theirs_");

    GitClient git;
    ASSERT_TRUE(git.InitRepository(repo.string()));
    git.SetConfig("user.name", "ScratchRobin Test");
    git.SetConfig("user.email", "test@example.com");

    auto base_branch = git.GetCurrentBranch();
    ASSERT_TRUE(base_branch.has_value());

    auto file_path = repo / "designs/public.orders.table.json";
    WriteFile(file_path, "base\n");
    ASSERT_TRUE(git.AddAll().success);
    ASSERT_TRUE(git.Commit("base").success);

    ASSERT_TRUE(git.CreateBranch("feature").success);
    ASSERT_TRUE(git.CheckoutBranch("feature").success);
    WriteFile(file_path, "theirs\n");
    ASSERT_TRUE(git.AddAll().success);
    ASSERT_TRUE(git.Commit("theirs").success);

    ASSERT_TRUE(git.CheckoutBranch(base_branch->name).success);
    WriteFile(file_path, "ours\n");
    ASSERT_TRUE(git.AddAll().success);
    ASSERT_TRUE(git.Commit("ours").success);

    git.MergeBranch("feature");

    auto conflicts = git.GetConflictedFiles();
    ASSERT_FALSE(conflicts.empty());

    ASSERT_TRUE(git.CheckoutTheirs("designs/public.orders.table.json").success);
    ASSERT_TRUE(git.MarkResolved("designs/public.orders.table.json").success);

    auto conflicts_after = git.GetConflictedFiles();
    EXPECT_TRUE(conflicts_after.empty());

    std::ifstream in(file_path);
    std::string content;
    std::getline(in, content);
    EXPECT_EQ(content, "theirs");
}
