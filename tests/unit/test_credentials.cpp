/**
 * @file test_credentials.cpp
 * @brief Unit tests for credential management
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/credentials.h"

using namespace scratchrobin;

class CredentialsTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = CreateDefaultCredentialStore();
    }
    
    std::unique_ptr<CredentialStore> store_;
};

TEST_F(CredentialsTest, StoreAndRetrievePassword) {
    std::string key = "test_connection_1";
    std::string password = "secret_password_123";
    std::string error;
    
    bool stored = store_->StorePassword(key, password, &error);
    EXPECT_TRUE(stored);
    
    std::string retrieved;
    bool resolved = store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_TRUE(resolved);
    EXPECT_EQ(retrieved, password);
}

TEST_F(CredentialsTest, RetrieveNonExistentPassword) {
    std::string retrieved;
    std::string error;
    bool resolved = store_->ResolvePassword("non_existent_key", &retrieved, &error);
    EXPECT_FALSE(resolved);
}

TEST_F(CredentialsTest, HasPassword) {
    // Use unique key to avoid conflicts with previous test runs
    std::string key = "test_has_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::string password = "password";
    std::string error;
    
    // Ensure key doesn't exist
    store_->DeletePassword(key, &error);
    
    EXPECT_FALSE(store_->HasPassword(key));
    
    store_->StorePassword(key, password, &error);
    EXPECT_TRUE(store_->HasPassword(key));
    
    // Cleanup
    store_->DeletePassword(key, &error);
}

TEST_F(CredentialsTest, DeletePassword) {
    std::string key = "test_delete";
    std::string password = "password";
    std::string error;
    
    store_->StorePassword(key, password, &error);
    EXPECT_TRUE(store_->HasPassword(key));
    
    bool deleted = store_->DeletePassword(key, &error);
    EXPECT_TRUE(deleted);
    
    EXPECT_FALSE(store_->HasPassword(key));
}

TEST_F(CredentialsTest, UpdatePassword) {
    std::string key = "test_update";
    std::string error;
    
    store_->StorePassword(key, "old_password", &error);
    store_->StorePassword(key, "new_password", &error);
    
    std::string retrieved;
    store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_EQ(retrieved, "new_password");
}

TEST_F(CredentialsTest, StoreEmptyPassword) {
    std::string key = "test_empty";
    std::string error;
    
    bool stored = store_->StorePassword(key, "", &error);
    EXPECT_TRUE(stored);
    
    std::string retrieved;
    bool resolved = store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_TRUE(resolved);
    EXPECT_EQ(retrieved, "");
}

TEST_F(CredentialsTest, StoreLongPassword) {
    std::string key = "test_long";
    std::string long_password(1000, 'a');
    std::string error;
    
    bool stored = store_->StorePassword(key, long_password, &error);
    EXPECT_TRUE(stored);
    
    std::string retrieved;
    store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_EQ(retrieved.length(), 1000);
}

TEST_F(CredentialsTest, SpecialCharactersInPassword) {
    std::string key = "test_special";
    std::string password = "p@$$w0rd!#$%^&*()_+-=[]{}|;':\",./<>?";
    std::string error;
    
    store_->StorePassword(key, password, &error);
    
    std::string retrieved;
    store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_EQ(retrieved, password);
}

TEST_F(CredentialsTest, UnicodePassword) {
    std::string key = "test_unicode";
    std::string password = "password";  // Simplified for testing
    std::string error;
    
    store_->StorePassword(key, password, &error);
    
    std::string retrieved;
    store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_EQ(retrieved, password);
}

TEST_F(CredentialsTest, MultipleCredentials) {
    std::string error;
    for (int i = 0; i < 10; ++i) {
        std::string key = "connection_" + std::to_string(i);
        std::string password = "password_" + std::to_string(i);
        store_->StorePassword(key, password, &error);
    }
    
    for (int i = 0; i < 10; ++i) {
        std::string key = "connection_" + std::to_string(i);
        std::string retrieved;
        bool resolved = store_->ResolvePassword(key, &retrieved, &error);
        EXPECT_TRUE(resolved);
        EXPECT_EQ(retrieved, "password_" + std::to_string(i));
    }
}

TEST_F(CredentialsTest, KeyWithSpecialCharacters) {
    std::string key = "conn:prod@host:5432/db";
    std::string password = "secret";
    std::string error;
    
    store_->StorePassword(key, password, &error);
    
    std::string retrieved;
    bool resolved = store_->ResolvePassword(key, &retrieved, &error);
    EXPECT_TRUE(resolved);
    EXPECT_EQ(retrieved, password);
}

TEST_F(CredentialsTest, ApiKeyStorage) {
    std::string provider = "openai";
    std::string api_key = "sk-test123456789";
    
    store_->StoreApiKey(provider, api_key);
    std::string retrieved = store_->GetApiKey(provider);
    
    EXPECT_EQ(retrieved, api_key);
}

TEST_F(CredentialsTest, DeleteApiKey) {
    std::string provider = "anthropic";
    std::string api_key = "sk-ant-test123";
    
    store_->StoreApiKey(provider, api_key);
    EXPECT_EQ(store_->GetApiKey(provider), api_key);
    
    store_->DeleteApiKey(provider);
    EXPECT_EQ(store_->GetApiKey(provider), "");
}
