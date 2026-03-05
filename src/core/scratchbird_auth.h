/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {
namespace core {

// ============================================================================
// Authentication Types
// ============================================================================

enum class ScratchBirdAuthType {
  kTrust,
  kReject,
  kPassword,
  kMD5,
  kScramSha256,
  kScramSha512,
  kCertificate,
  kLDAP,
  kKerberos,
  kPeer,
  kIdent,
  kRADIUS,
  kPAM,
  kToken
};

enum class AuthState {
  kInitial,
  kClientFirstSent,
  kClientFinalSent,
  kAuthenticated,
  kFailed
};

enum class ScramMechanism {
  kScramSha256,
  kScramSha512
};

// ============================================================================
// AuthResult
// ============================================================================

struct AuthResult {
  bool success = false;
  AuthState state = AuthState::kInitial;
  std::string authenticated_user;
  std::string error_message;
  std::string server_payload;
};

// ============================================================================
// HBARule
// ============================================================================

struct HBARule {
  enum class ConnectionType {
    kLocal,
    kHost,
    kHostSSL,
    kHostNoSSL
  };
  
  ConnectionType conn_type = ConnectionType::kHost;
  std::string database;
  std::string user;
  std::string address = "all";
  std::string netmask;
  ScratchBirdAuthType auth_type = ScratchBirdAuthType::kScramSha256;
  std::map<std::string, std::string> auth_options;
  
  bool Matches(const std::string& conn_db,
               const std::string& conn_user,
               const std::string& conn_addr,
               bool conn_ssl) const;
};

// ============================================================================
// HBAConfig
// ============================================================================

class HBAConfig {
 public:
  HBAConfig();
  ~HBAConfig();
  
  bool LoadFromFile(const std::string& file_path);
  bool LoadFromString(const std::string& config);
  
  std::optional<HBARule> FindMatchingRule(const std::string& database,
                                          const std::string& user,
                                          const std::string& address,
                                          bool is_ssl) const;
  
  void AddRule(const HBARule& rule);
  void Clear();
  bool Validate(std::vector<std::string>& errors) const;
  
  static std::string GetDefaultConfig();
  
 private:
  HBARule ParseRule(const std::string& line);
  
  std::vector<HBARule> rules_;
};

// ============================================================================
// AuthConnectionContext
// ============================================================================

struct AuthConnectionContext {
  std::string database;
  std::string host;
  int port = 5432;
  bool ssl = false;
  std::string client_address;
};

// ============================================================================
// ScramCredentials
// ============================================================================

struct ScramCredentials {
  std::string username;
  std::string password;
  std::string client_nonce;
  std::string salt;
  int iterations = 4096;
};

// ============================================================================
// ScratchBirdAuthClient
// ============================================================================

using CredentialCallback = std::function<bool(const std::string& prompt, std::string* value)>;

class ScratchBirdAuthClient {
 public:
  ScratchBirdAuthClient();
  ~ScratchBirdAuthClient();
  
  void Initialize(const AuthConnectionContext& context);
  void Reset();
  
  // SCRAM authentication
  AuthResult AuthenticateScram(const std::string& username,
                               const std::string& password,
                               ScramMechanism mechanism = ScramMechanism::kScramSha256);
  AuthResult BeginScramAuth(const std::string& username,
                            ScramMechanism mechanism = ScramMechanism::kScramSha256);
  AuthResult ContinueScramAuth(const std::string& client_payload,
                               const std::string& server_payload);
  
  // Other authentication methods
  AuthResult AuthenticatePassword(const std::string& username,
                                  const std::string& password);
  AuthResult AuthenticateCertificate(const std::string& username,
                                     const std::vector<uint8_t>& certificate);
  AuthResult AuthenticateToken(const std::string& token);
  
  // Credential callback
  void SetCredentialCallback(CredentialCallback callback);
  
  // State accessors
  AuthState GetState() const { return state_; }
  ScratchBirdAuthType GetAuthType() const { return auth_type_; }
  std::string GetLastError() const { return last_error_; }
  
 private:
  std::string BuildClientFirstMessage(const std::string& username,
                                      const std::string& nonce);
  std::string BuildClientFinalMessage(const std::string& password,
                                      const std::string& server_first,
                                      const std::string& client_first,
                                      const std::string& combined_nonce);
  
  std::vector<uint8_t> Hi(const std::string& password,
                          const std::string& salt,
                          int iterations,
                          int key_len);
  std::vector<uint8_t> Hmac(const std::vector<uint8_t>& key,
                            const std::string& data);
  std::vector<uint8_t> Hash(const std::string& data);
  std::vector<uint8_t> Xor(const std::vector<uint8_t>& a,
                           const std::vector<uint8_t>& b);
  
  std::string Base64Encode(const std::vector<uint8_t>& data);
  std::vector<uint8_t> Base64Decode(const std::string& str);
  std::string GenerateClientNonce();
  std::string SaslPrep(const std::string& input);
  
  AuthConnectionContext context_;
  AuthState state_ = AuthState::kInitial;
  ScratchBirdAuthType auth_type_ = ScratchBirdAuthType::kScramSha256;
  ScramMechanism scram_mechanism_ = ScramMechanism::kScramSha256;
  ScramCredentials credentials_;
  std::string last_error_;
  std::string client_first_message_;
  std::string server_first_message_;
  std::string auth_message_;
  CredentialCallback credential_callback_;
};

// ============================================================================
// AuthPluginManager
// ============================================================================

class AuthPluginManager {
 public:
  struct MethodInfo {
    std::string id;
    std::string name;
    std::string description;
    bool is_builtin = true;
    std::string library_path;
  };
  
  AuthPluginManager();
  ~AuthPluginManager();
  
  bool Initialize(const std::string& plugin_dir);
  
  std::vector<std::string> ListAvailableMethods();
  bool IsMethodAvailable(const std::string& method_id);
  std::optional<MethodInfo> GetMethodInfo(const std::string& method_id);
  
  std::unique_ptr<ScratchBirdAuthClient> CreateAuthenticator(const std::string& method_id);
  
 private:
  std::string plugin_dir_;
  std::map<std::string, MethodInfo> methods_;
};

}  // namespace core
}  // namespace scratchrobin
