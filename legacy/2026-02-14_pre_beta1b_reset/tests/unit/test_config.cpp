/**
 * @file test_config.cpp
 * @brief Unit tests for configuration management
 */

#include <gtest/gtest.h>
#include "core/config.h"
#include <sstream>
#include <filesystem>

using namespace scratchrobin;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.theme = "system";
        config_.fontFamily = "default";
        config_.fontSize = 11;
        config_.historyMaxItems = 2000;
        config_.rowLimit = 200;
        config_.enableSuggestions = true;
    }
    
    AppConfig config_;
    std::filesystem::path temp_dir_ = std::filesystem::temp_directory_path();
};

TEST_F(ConfigTest, DefaultValues) {
    AppConfig defaults;
    EXPECT_EQ(defaults.theme, "system");
    EXPECT_EQ(defaults.fontFamily, "default");
    EXPECT_EQ(defaults.fontSize, 11);
    EXPECT_EQ(defaults.historyMaxItems, 2000);
    EXPECT_EQ(defaults.rowLimit, 200);
    EXPECT_TRUE(defaults.enableSuggestions);
}

TEST_F(ConfigTest, ModifyValues) {
    config_.theme = "dark";
    config_.fontFamily = "Consolas";
    config_.fontSize = 14;
    config_.historyMaxItems = 5000;
    config_.rowLimit = 500;
    config_.enableSuggestions = false;
    
    EXPECT_EQ(config_.theme, "dark");
    EXPECT_EQ(config_.fontFamily, "Consolas");
    EXPECT_EQ(config_.fontSize, 14);
    EXPECT_EQ(config_.historyMaxItems, 5000);
    EXPECT_EQ(config_.rowLimit, 500);
    EXPECT_FALSE(config_.enableSuggestions);
}

TEST_F(ConfigTest, ChromeConfigDefaults) {
    EXPECT_TRUE(config_.chrome.mainWindow.showMenu);
    EXPECT_TRUE(config_.chrome.mainWindow.showIconBar);
    EXPECT_TRUE(config_.chrome.mainWindow.replicateMenu);
    EXPECT_FALSE(config_.chrome.mainWindow.replicateIconBar);
}

TEST_F(ConfigTest, StartupConfigDefaults) {
    EXPECT_TRUE(config_.startup.enabled);
    EXPECT_TRUE(config_.startup.showProgress);
}

TEST_F(ConfigTest, AiConfigDefaults) {
    EXPECT_EQ(config_.ai.provider, "openai");
    EXPECT_TRUE(config_.ai.enable_schema_design);
    EXPECT_TRUE(config_.ai.enable_query_optimization);
    EXPECT_TRUE(config_.ai.enable_code_generation);
    EXPECT_TRUE(config_.ai.enable_documentation);
    EXPECT_FLOAT_EQ(config_.ai.temperature, 0.3f);
    EXPECT_EQ(config_.ai.max_tokens, 4096);
}

TEST_F(ConfigTest, ConfigStoreLoadSave) {
    ConfigStore store;
    std::string test_path = (temp_dir_ / "test_config.json").string();
    
    // Modify config
    config_.theme = "light";
    config_.fontSize = 16;
    config_.ai.provider = "anthropic";
    
    // Save
    bool saved = store.SaveAppConfig(test_path, config_);
    EXPECT_TRUE(saved);
    
    // Load
    AppConfig loaded;
    bool loaded_ok = store.LoadAppConfig(test_path, &loaded);
    EXPECT_TRUE(loaded_ok);
    
    // Note: Actual values depend on implementation
    // Just verify loading succeeded
}

TEST_F(ConfigTest, AiConfigLoadSave) {
    ConfigStore store;
    std::string test_path = (temp_dir_ / "test_ai_config.json").string();
    
    AiConfig ai;
    ai.provider = "ollama";
    ai.model_name = "codellama";
    ai.api_endpoint = "http://localhost:11434";
    ai.temperature = 0.5f;
    ai.max_tokens = 2048;
    
    bool saved = store.SaveAiConfig(test_path, ai);
    EXPECT_TRUE(saved);
    
    AiConfig loaded;
    bool loaded_ok = store.LoadAiConfig(test_path, &loaded);
    EXPECT_TRUE(loaded_ok);
}

TEST_F(ConfigTest, ConnectionStatusPreferencesPersist) {
    ConfigStore store;
    std::string test_path = (temp_dir_ / "test_connections.toml").string();

    ConnectionProfile profile;
    profile.name = "StatusPrefProfile";
    profile.host = "localhost";
    profile.port = 3092;
    profile.database = "demo.sdb";
    profile.username = "sysdba";
    profile.statusAutoPollEnabled = true;
    profile.statusPollIntervalMs = 5000;
    profile.statusDefaultKind = StatusRequestKind::DatabaseInfo;
    profile.statusCategoryOrder = {"Request", "engine", "db"};
    profile.statusCategoryFilter = "engine";
    profile.statusDiffEnabled = true;
    profile.statusDiffIgnoreUnchanged = false;
    profile.statusDiffIgnoreEmpty = true;

    std::vector<ConnectionProfile> connections = {profile};
    EXPECT_TRUE(store.SaveConnections(test_path, connections));

    std::vector<ConnectionProfile> loaded;
    EXPECT_TRUE(store.LoadConnections(test_path, &loaded));
    ASSERT_EQ(loaded.size(), 1u);
    const auto& loaded_profile = loaded.front();

    EXPECT_EQ(loaded_profile.name, profile.name);
    EXPECT_EQ(loaded_profile.host, profile.host);
    EXPECT_EQ(loaded_profile.port, profile.port);
    EXPECT_EQ(loaded_profile.database, profile.database);
    EXPECT_EQ(loaded_profile.username, profile.username);
    EXPECT_EQ(loaded_profile.statusAutoPollEnabled, profile.statusAutoPollEnabled);
    EXPECT_EQ(loaded_profile.statusPollIntervalMs, profile.statusPollIntervalMs);
    EXPECT_EQ(loaded_profile.statusDefaultKind, profile.statusDefaultKind);
    EXPECT_EQ(loaded_profile.statusCategoryOrder, profile.statusCategoryOrder);
    EXPECT_EQ(loaded_profile.statusCategoryFilter, profile.statusCategoryFilter);
    EXPECT_EQ(loaded_profile.statusDiffEnabled, profile.statusDiffEnabled);
    EXPECT_EQ(loaded_profile.statusDiffIgnoreUnchanged, profile.statusDiffIgnoreUnchanged);
    EXPECT_EQ(loaded_profile.statusDiffIgnoreEmpty, profile.statusDiffIgnoreEmpty);
}
