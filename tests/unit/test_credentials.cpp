/**
 * @file test_credentials.cpp
 * @brief Unit tests for credential management
 */

#include <gtest/gtest.h>
#include "core/credentials.h"

using namespace scratchrobin;

class CredentialsTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<CredentialStore>();
    }
    
    std::unique_ptr<CredentialStore> store_;
};

TEST_F(CredentialsTest, StoreAndRetrievePassword) {
    std::string key = "test_connection_1";
    std::string password = "secret_password_123";
    
    bool stored = store_->StorePassword(key, password);
    EXPECT_TRUE(stored);
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), password);
}

TEST_F(CredentialsTest, RetrieveNonExistentPassword) {
    auto retrieved = store_->GetPassword("non_existent_key");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(CredentialsTest, DeletePassword) {
    std::string key = "test_delete";
    store_->StorePassword(key, "password");
    
    EXPECT_TRUE(store_->HasPassword(key));
    
    bool deleted = store_->DeletePassword(key);
    EXPECT_TRUE(deleted);
    
    EXPECT_FALSE(store_->HasPassword(key));
}

TEST_F(CredentialsTest, UpdatePassword) {
    std::string key = "test_update";
    store_->StorePassword(key, "old_password");
    
    store_->StorePassword(key, "new_password");
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), "new_password");
}

TEST_F(CredentialsTest, StoreEmptyPassword) {
    std::string key = "test_empty";
    
    bool stored = store_->StorePassword(key, "");
    EXPECT_TRUE(stored);
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), "");
}

TEST_F(CredentialsTest, StoreLongPassword) {
    std::string key = "test_long";
    std::string long_password(1000, 'a');
    
    bool stored = store_->StorePassword(key, long_password);
    EXPECT_TRUE(stored);
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value().length(), 1000);
}

TEST_F(CredentialsTest, SpecialCharactersInPassword) {
    std::string key = "test_special";
    std::string password = "p@$$w0rd!#$%^&*()_+-=[]{}|;':\",./<>?";
    
    store_->StorePassword(key, password);
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), password);
}

TEST_F(CredentialsTest, UnicodePassword) {
    std::string key = "test_unicode";
    std::string password = "пароль密码パスワード";
    
    store_->StorePassword(key, password);
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), password);
}

TEST_F(CredentialsTest, MultipleCredentials) {
    for (int i = 0; i < 10; ++i) {
        std::string key = "connection_" + std::to_string(i);
        std::string password = "password_" + std::to_string(i);
        store_->StorePassword(key, password);
    }
    
    for (int i = 0; i < 10; ++i) {
        std::string key = "connection_" + std::to_string(i);
        auto retrieved = store_->GetPassword(key);
        ASSERT_TRUE(retrieved.has_value());
        EXPECT_EQ(retrieved.value(), "password_" + std::to_string(i));
    }
}

TEST_F(CredentialsTest, ListKeys) {
    store_->StorePassword("alpha", "pass1");
    store_->StorePassword("beta", "pass2");
    store_->StorePassword("gamma", "pass3");
    
    auto keys = store_->ListKeys();
    
    EXPECT_EQ(keys.size(), 3);
    EXPECT_NE(std::find(keys.begin(), keys.end(), "alpha"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "beta"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "gamma"), keys.end());
}

TEST_F(CredentialsTest, ClearAllCredentials) {
    store_->StorePassword("key1", "pass1");
    store_->StorePassword("key2", "pass2");
    
    store_->Clear();
    
    EXPECT_FALSE(store_->HasPassword("key1"));
    EXPECT_FALSE(store_->HasPassword("key2"));
    EXPECT_TRUE(store_->ListKeys().empty());
}

TEST_F(CredentialsTest, KeyWithSpecialCharacters) {
    std::string key = "conn:prod@host:5432/db";
    std::string password = "secret";
    
    store_->StorePassword(key, password);
    
    auto retrieved = store_->GetPassword(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), password);
}

TEST_F(CredentialsTest, SecureMemory) {
    // This test verifies that passwords are handled securely
    // In a real implementation, this would check that:
    // 1. Passwords are encrypted at rest
    // 2. Memory is locked to prevent swapping
    // 3. Passwords are cleared from memory after use
    
    std::string key = "secure_test";
    std::string password = "very_secret_password";
    
    store_->StorePassword(key, password);
    
    // Password should be retrievable
    auto retrieved = store_->GetPassword(key);
    EXPECT_TRUE(retrieved.has_value());
    
    // After deletion, should not be retrievable
    store_->DeletePassword(key);
    EXPECT_FALSE(store_->GetPassword(key).has_value());
}

TEST_F(CredentialsTest, ConnectionProfileCredentialIntegration) {
    ConnectionProfile profile;
    profile.name = "Production DB";
    profile.host = "prod.example.com";
    profile.port = 5432;
    profile.username = "admin";
    
    // Store credential for this profile
    std::string cred_key = "conn_" + profile.name;
    store_->StorePassword(cred_key, "prod_secret");
    
    // Later, retrieve when connecting
    auto password = store_->GetPassword(cred_key);
    ASSERT_TRUE(password.has_value());
    EXPECT_EQ(password.value(), "prod_secret");
}
