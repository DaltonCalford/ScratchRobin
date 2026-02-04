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
        config_ = std::make_unique<AppConfig>();
    }
    
    std::unique_ptr<AppConfig> config_;
    std::filesystem::path temp_dir_ = std::filesystem::temp_directory_path();
};

TEST_F(ConfigTest, DefaultValues) {
    EXPECT_FALSE(config_->connections.empty());
    EXPECT_EQ(config_->editor.font_family, "Consolas");
    EXPECT_EQ(config_->editor.font_size, 11);
    EXPECT_EQ(config_->editor.tab_width, 4);
    EXPECT_TRUE(config_->editor.use_spaces_for_tabs);
}

TEST_F(ConfigTest, LoadFromTOML) {
    std::string toml_content = R"(
[editor]
font_family = "Fira Code"
font_size = 14
tab_width = 2

[results]
max_rows = 5000
)
";
    
    std::istringstream stream(toml_content);
    auto loaded = AppConfig::LoadFromStream(stream);
    
    EXPECT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->editor.font_family, "Fira Code");
    EXPECT_EQ(loaded->editor.font_size, 14);
    EXPECT_EQ(loaded->editor.tab_width, 2);
    EXPECT_EQ(loaded->results.max_rows, 5000);
}

TEST_F(ConfigTest, SaveAndLoadRoundTrip) {
    // Modify config
    config_->editor.font_family = "Monaco";
    config_->editor.font_size = 13;
    config_->results.max_rows = 10000;
    
    // Save to string
    std::ostringstream output;
    bool saved = config_->SaveToStream(output);
    EXPECT_TRUE(saved);
    
    // Load back
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    EXPECT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->editor.font_family, "Monaco");
    EXPECT_EQ(loaded->editor.font_size, 13);
    EXPECT_EQ(loaded->results.max_rows, 10000);
}

TEST_F(ConfigTest, InvalidTOML) {
    std::string invalid_toml = "[invalid toml content{{{";
    
    std::istringstream stream(invalid_toml);
    auto loaded = AppConfig::LoadFromStream(stream);
    
    EXPECT_FALSE(loaded.has_value());
}

TEST_F(ConfigTest, MissingFieldsUseDefaults) {
    std::string partial_toml = R"(
[editor]
font_size = 20
)
";
    
    std::istringstream stream(partial_toml);
    auto loaded = AppConfig::LoadFromStream(stream);
    
    EXPECT_TRUE(loaded.has_value());
    // font_size should be loaded
    EXPECT_EQ(loaded->editor.font_size, 20);
    // font_family should use default
    EXPECT_EQ(loaded->editor.font_family, "Consolas");
}

TEST_F(ConfigTest, ConnectionProfileRoundTrip) {
    ConnectionProfile profile;
    profile.name = "Test Connection";
    profile.host = "localhost";
    profile.port = 5432;
    profile.database = "testdb";
    profile.username = "testuser";
    profile.backend_type = BackendType::PostgreSQL;
    profile.ssl_mode = SslMode::Require;
    
    config_->connections.push_back(profile);
    
    // Save and reload
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    ASSERT_EQ(loaded->connections.size(), 1);
    EXPECT_EQ(loaded->connections[0].name, "Test Connection");
    EXPECT_EQ(loaded->connections[0].host, "localhost");
    EXPECT_EQ(loaded->connections[0].port, 5432);
    EXPECT_EQ(loaded->connections[0].backend_type, BackendType::PostgreSQL);
}

TEST_F(ConfigTest, ColorSchemeConfiguration) {
    config_->colors.scheme = ColorScheme::Dark;
    config_->colors.editor_background = {30, 30, 30};
    config_->colors.editor_foreground = {220, 220, 220};
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->colors.scheme, ColorScheme::Dark);
    EXPECT_EQ(loaded->colors.editor_background.r, 30);
    EXPECT_EQ(loaded->colors.editor_foreground.g, 220);
}

TEST_F(ConfigTest, ResultGridConfiguration) {
    config_->results.max_rows = 50000;
    config_->results.page_size = 500;
    config_->results.show_row_numbers = false;
    config_->results.null_display = "<null>";
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->results.max_rows, 50000);
    EXPECT_EQ(loaded->results.page_size, 500);
    EXPECT_FALSE(loaded->results.show_row_numbers);
    EXPECT_EQ(loaded->results.null_display, "<null>");
}

TEST_F(ConfigTest, KeyboardShortcuts) {
    config_->shortcuts.execute_sql = "Ctrl+E";
    config_->shortcuts.cancel_query = "Ctrl+Period";
    config_->shortcuts.new_editor = "Ctrl+N";
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->shortcuts.execute_sql, "Ctrl+E");
    EXPECT_EQ(loaded->shortcuts.cancel_query, "Ctrl+Period");
}

TEST_F(ConfigTest, NetworkSettings) {
    config_->network.connect_timeout_seconds = 30;
    config_->network.query_timeout_seconds = 300;
    config_->network.keep_alive = true;
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->network.connect_timeout_seconds, 30);
    EXPECT_EQ(loaded->network.query_timeout_seconds, 300);
    EXPECT_TRUE(loaded->network.keep_alive);
}

TEST_F(ConfigTest, ExportSettings) {
    config_->export_settings.csv_delimiter = "\t";
    config_->export_settings.csv_include_header = false;
    config_->export_settings.json_pretty_print = true;
    config_->export_settings.default_format = ExportFormat::JSON;
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->export_settings.csv_delimiter, "\t");
    EXPECT_FALSE(loaded->export_settings.csv_include_header);
    EXPECT_TRUE(loaded->export_settings.json_pretty_print);
}

TEST_F(ConfigTest, EmptyConnections) {
    config_->connections.clear();
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    EXPECT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->connections.empty());
}

TEST_F(ConfigTest, MultipleConnections) {
    for (int i = 0; i < 5; ++i) {
        ConnectionProfile profile;
        profile.name = "Connection " + std::to_string(i);
        profile.host = "host" + std::to_string(i);
        profile.port = 5432 + i;
        config_->connections.push_back(profile);
    }
    
    std::ostringstream output;
    config_->SaveToStream(output);
    
    std::istringstream input(output.str());
    auto loaded = AppConfig::LoadFromStream(input);
    
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->connections.size(), 5);
}

TEST_F(ConfigTest, MergeWithDefaults) {
    // Start with defaults
    std::string partial = R"(
[editor]
font_size = 16

[network]
connect_timeout_seconds = 45
)
";
    
    std::istringstream stream(partial);
    auto loaded = AppConfig::LoadFromStream(stream);
    
    ASSERT_TRUE(loaded.has_value());
    // Custom values
    EXPECT_EQ(loaded->editor.font_size, 16);
    EXPECT_EQ(loaded->network.connect_timeout_seconds, 45);
    // Defaults preserved
    EXPECT_EQ(loaded->editor.font_family, "Consolas");
    EXPECT_EQ(loaded->editor.tab_width, 4);
}
