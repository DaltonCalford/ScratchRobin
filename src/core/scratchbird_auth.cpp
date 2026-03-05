/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "scratchbird_auth.h"

#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <fstream>
#include <regex>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace scratchrobin {
namespace core {

namespace {

// Base64 encoding
std::string Base64EncodeImpl(const std::vector<uint8_t>& data) {
  BIO* bio = BIO_new(BIO_s_mem());
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bio = BIO_push(b64, bio);
  
  BIO_write(bio, data.data(), static_cast<int>(data.size()));
  BIO_flush(bio);
  
  BUF_MEM* buffer;
  BIO_get_mem_ptr(bio, &buffer);
  
  std::string result(buffer->data, buffer->length);
  BIO_free_all(bio);
  
  return result;
}

std::vector<uint8_t> Base64DecodeImpl(const std::string& str) {
  BIO* bio = BIO_new_mem_buf(str.c_str(), static_cast<int>(str.length()));
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bio = BIO_push(b64, bio);
  
  std::vector<uint8_t> result(str.length());
  int decoded = BIO_read(bio, result.data(), static_cast<int>(str.length()));
  result.resize(decoded > 0 ? decoded : 0);
  
  BIO_free_all(bio);
  return result;
}

// Generate random nonce
std::string GenerateNonce() {
  std::vector<uint8_t> nonce_bytes(24);
  if (RAND_bytes(nonce_bytes.data(), static_cast<int>(nonce_bytes.size())) != 1) {
    // Fallback to random_device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    for (auto& b : nonce_bytes) {
      b = static_cast<uint8_t>(dis(gen));
    }
  }
  return Base64EncodeImpl(nonce_bytes);
}

// Parse SCRAM server-first-message
bool ParseServerFirst(const std::string& message,
                      std::string& nonce,
                      std::string& salt,
                      int& iterations) {
  std::istringstream iss(message);
  std::string part;
  
  while (std::getline(iss, part, ',')) {
    if (part.empty()) continue;
    
    char key = part[0];
    std::string value = part.substr(2);  // Skip key and '='
    
    switch (key) {
      case 'r':  // Combined nonce
        nonce = value;
        break;
      case 's':  // Salt (base64)
        salt = value;
        break;
      case 'i':  // Iteration count
        iterations = std::stoi(value);
        break;
    }
  }
  
  return !nonce.empty() && !salt.empty() && iterations > 0;
}

// Parse SCRAM server-final-message
bool ParseServerFinal(const std::string& message,
                      std::string& server_signature) {
  if (message.substr(0, 2) == "e=") {
    // Error response
    return false;
  }
  
  if (message.substr(0, 2) == "v=") {
    server_signature = message.substr(2);
    return true;
  }
  
  return false;
}

}  // anonymous namespace

// ============================================================================
// HBARule Implementation
// ============================================================================

bool HBARule::Matches(const std::string& conn_db,
                      const std::string& conn_user,
                      const std::string& conn_addr,
                      bool conn_ssl) const {
  // Check connection type
  if (conn_type == ConnectionType::kHostSSL && !conn_ssl) {
    return false;
  }
  if (conn_type == ConnectionType::kHostNoSSL && conn_ssl) {
    return false;
  }
  
  // Check database pattern
  if (database != "all" && database != conn_db) {
    if (database != "sameuser" || conn_db != conn_user) {
      return false;
    }
  }
  
  // Check user pattern
  if (user != "all" && user != conn_user) {
    // TODO: Support group membership (@group)
    return false;
  }
  
  // Check address
  if (conn_type != ConnectionType::kLocal && address != "all") {
    // TODO: Implement IP/CIDR matching
    if (address != conn_addr) {
      return false;
    }
  }
  
  return true;
}

// ============================================================================
// HBAConfig Implementation
// ============================================================================

HBAConfig::HBAConfig() = default;
HBAConfig::~HBAConfig() = default;

bool HBAConfig::LoadFromFile(const std::string& file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    return false;
  }
  
  std::string line;
  std::string config;
  while (std::getline(file, line)) {
    config += line + "\n";
  }
  
  file.close();
  return LoadFromString(config);
}

bool HBAConfig::LoadFromString(const std::string& config) {
  rules_.clear();
  
  std::istringstream iss(config);
  std::string line;
  
  while (std::getline(iss, line)) {
    // Skip comments and empty lines
    line = line.substr(0, line.find('#'));
    
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t");
    line = line.substr(start, end - start + 1);
    
    if (line.empty()) continue;
    
    try {
      HBARule rule = ParseRule(line);
      rules_.push_back(rule);
    } catch (const std::exception& e) {
      // Skip invalid rules
      continue;
    }
  }
  
  return true;
}

HBARule HBAConfig::ParseRule(const std::string& line) {
  HBARule rule;
  std::istringstream iss(line);
  std::string token;
  
  // Parse connection type
  iss >> token;
  if (token == "local") {
    rule.conn_type = HBARule::ConnectionType::kLocal;
  } else if (token == "host") {
    rule.conn_type = HBARule::ConnectionType::kHost;
  } else if (token == "hostssl") {
    rule.conn_type = HBARule::ConnectionType::kHostSSL;
  } else if (token == "hostnossl") {
    rule.conn_type = HBARule::ConnectionType::kHostNoSSL;
  } else {
    throw std::invalid_argument("Invalid connection type");
  }
  
  // Parse database
  iss >> rule.database;
  
  // Parse user
  iss >> rule.user;
  
  // Parse address (if not local)
  if (rule.conn_type != HBARule::ConnectionType::kLocal) {
    iss >> rule.address;
    // Check for IP/mask format
    if (rule.address.find('/') == std::string::npos) {
      // Might be IP mask format
      std::string next;
      std::streampos pos = iss.tellg();
      if (iss >> next && next.find('.') != std::string::npos) {
        rule.netmask = next;
      } else {
        iss.seekg(pos);
      }
    }
  }
  
  // Parse auth method
  std::string method;
  iss >> method;
  
  // Map method string to enum
  static const std::map<std::string, ScratchBirdAuthType> method_map = {
    {"trust", ScratchBirdAuthType::kTrust},
    {"reject", ScratchBirdAuthType::kReject},
    {"password", ScratchBirdAuthType::kPassword},
    {"md5", ScratchBirdAuthType::kMD5},
    {"scram-sha-256", ScratchBirdAuthType::kScramSha256},
    {"scram-sha-512", ScratchBirdAuthType::kScramSha512},
    {"cert", ScratchBirdAuthType::kCertificate},
    {"ldap", ScratchBirdAuthType::kLDAP},
    {"gss", ScratchBirdAuthType::kKerberos},
    {"sspi", ScratchBirdAuthType::kKerberos},
    {"peer", ScratchBirdAuthType::kPeer},
    {"ident", ScratchBirdAuthType::kIdent},
    {"radius", ScratchBirdAuthType::kRADIUS},
    {"pam", ScratchBirdAuthType::kPAM},
    {"oauth", ScratchBirdAuthType::kToken},
    {"jwt", ScratchBirdAuthType::kToken}
  };
  
  auto it = method_map.find(method);
  if (it != method_map.end()) {
    rule.auth_type = it->second;
  } else {
    rule.auth_type = ScratchBirdAuthType::kReject;
  }
  
  // Parse auth options
  std::string option;
  while (iss >> option) {
    size_t eq_pos = option.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = option.substr(0, eq_pos);
      std::string value = option.substr(eq_pos + 1);
      rule.auth_options[key] = value;
    }
  }
  
  return rule;
}

std::optional<HBARule> HBAConfig::FindMatchingRule(const std::string& database,
                                                    const std::string& user,
                                                    const std::string& address,
                                                    bool is_ssl) const {
  for (const auto& rule : rules_) {
    if (rule.Matches(database, user, address, is_ssl)) {
      return rule;
    }
  }
  return std::nullopt;
}

void HBAConfig::AddRule(const HBARule& rule) {
  rules_.push_back(rule);
}

void HBAConfig::Clear() {
  rules_.clear();
}

bool HBAConfig::Validate(std::vector<std::string>& errors) const {
  errors.clear();
  
  for (size_t i = 0; i < rules_.size(); ++i) {
    const auto& rule = rules_[i];
    
    if (rule.database.empty()) {
      errors.push_back("Rule " + std::to_string(i) + ": Missing database");
    }
    if (rule.user.empty()) {
      errors.push_back("Rule " + std::to_string(i) + ": Missing user");
    }
  }
  
  return errors.empty();
}

std::string HBAConfig::GetDefaultConfig() {
  return R"(# ScratchBird HBA Configuration
# TYPE  DATABASE        USER            ADDRESS                 METHOD

# Local connections
local   all             all                                     scram-sha-256

# IPv4 local connections:
host    all             all             127.0.0.1/32            scram-sha-256

# IPv6 local connections:
host    all             all             ::1/128                 scram-sha-256

# Allow replication connections from localhost
local   replication     all                                     scram-sha-256
host    replication     all             127.0.0.1/32            scram-sha-256
host    replication     all             ::1/128                 scram-sha-256
)";
}

// ============================================================================
// ScratchBirdAuthClient Implementation
// ============================================================================

ScratchBirdAuthClient::ScratchBirdAuthClient() = default;

ScratchBirdAuthClient::~ScratchBirdAuthClient() = default;

void ScratchBirdAuthClient::Initialize(const AuthConnectionContext& context) {
  context_ = context;
  Reset();
}

void ScratchBirdAuthClient::Reset() {
  state_ = AuthState::kInitial;
  auth_type_ = ScratchBirdAuthType::kScramSha256;
  scram_mechanism_ = ScramMechanism::kScramSha256;
  credentials_ = ScramCredentials();
  last_error_.clear();
  client_first_message_.clear();
  server_first_message_.clear();
  auth_message_.clear();
}

AuthResult ScratchBirdAuthClient::AuthenticateScram(const std::string& username,
                                                     const std::string& password,
                                                     ScramMechanism mechanism) {
  Reset();
  
  scram_mechanism_ = mechanism;
  auth_type_ = (mechanism == ScramMechanism::kScramSha512) ?
               ScratchBirdAuthType::kScramSha512 :
               ScratchBirdAuthType::kScramSha256;
  
  credentials_.username = username;
  credentials_.password = password;
  
  // Step 1: Build and send client-first-message
  std::string client_nonce = GenerateClientNonce();
  credentials_.client_nonce = client_nonce;
  
  client_first_message_ = BuildClientFirstMessage(username, client_nonce);
  state_ = AuthState::kClientFirstSent;
  
  // In a real implementation, we would send this to the server
  // and wait for server-first-message
  AuthResult result;
  result.success = false;
  result.state = state_;
  result.server_payload = client_first_message_;  // Echo for now
  
  return result;
}

AuthResult ScratchBirdAuthClient::BeginScramAuth(const std::string& username,
                                                  ScramMechanism mechanism) {
  return AuthenticateScram(username, "", mechanism);
}

AuthResult ScratchBirdAuthClient::ContinueScramAuth(const std::string& client_payload,
                                                     const std::string& server_payload) {
  AuthResult result;
  result.state = state_;
  
  switch (state_) {
    case AuthState::kClientFirstSent: {
      // Parse server-first-message
      server_first_message_ = server_payload;
      
      std::string combined_nonce;
      std::string salt_b64;
      int iterations = 0;
      
      if (!ParseServerFirst(server_payload, combined_nonce, salt_b64, iterations)) {
        state_ = AuthState::kFailed;
        result.success = false;
        result.state = state_;
        result.error_message = "Invalid server-first-message";
        return result;
      }
      
      credentials_.salt = salt_b64;
      credentials_.iterations = iterations;
      
      // Build client-final-message
      std::string client_final = BuildClientFinalMessage(
          credentials_.password,
          server_first_message_,
          client_first_message_,
          combined_nonce);
      
      state_ = AuthState::kClientFinalSent;
      result.success = false;  // Not complete yet
      result.state = state_;
      result.server_payload = client_final;
      break;
    }
      
    case AuthState::kClientFinalSent: {
      // Parse server-final-message
      std::string server_signature;
      if (!ParseServerFinal(server_payload, server_signature)) {
        state_ = AuthState::kFailed;
        result.success = false;
        result.state = state_;
        result.error_message = "Authentication failed: " + server_payload;
        return result;
      }
      
      // Verify server signature
      // In a real implementation, we would verify the server signature
      // against our calculated server_key
      
      state_ = AuthState::kAuthenticated;
      result.success = true;
      result.state = state_;
      result.authenticated_user = credentials_.username;
      break;
    }
      
    default:
      result.success = false;
      result.error_message = "Invalid authentication state";
      break;
  }
  
  return result;
}

std::string ScratchBirdAuthClient::BuildClientFirstMessage(const std::string& username,
                                                            const std::string& nonce) {
  std::ostringstream oss;
  
  // GS2 header (no channel binding, not reserved)
  oss << "n,";  // gs2-cb-flag
  oss << ",";   // gs2-authzid (empty)
  
  // Username (SASLprep, escaped)
  std::string prep_username = SaslPrep(username);
  oss << "n=" << prep_username << ",";
  
  // Client nonce
  oss << "r=" << nonce;
  
  return oss.str();
}

std::string ScratchBirdAuthClient::BuildClientFinalMessage(const std::string& password,
                                                            const std::string& server_first,
                                                            const std::string& client_first,
                                                            const std::string& combined_nonce) {
  std::ostringstream oss;
  
  // Channel binding data (empty for now)
  std::string cb_data = Base64Encode(std::vector<uint8_t>());
  oss << "c=" << cb_data << ",";
  
  // Combined nonce
  oss << "r=" << combined_nonce << ",";
  
  // Build auth message
  auth_message_ = client_first + "," + server_first + "," + oss.str();
  
  // Calculate proof
  // SaltedPassword = Hi(Normalize(password), salt, i)
  std::vector<uint8_t> salt_bytes = Base64Decode(credentials_.salt);
  std::string salt(salt_bytes.begin(), salt_bytes.end());
  std::vector<uint8_t> salted_password = Hi(password, salt, credentials_.iterations, 32);
  
  // ClientKey = HMAC(SaltedPassword, "Client Key")
  std::vector<uint8_t> client_key = Hmac(salted_password, "Client Key");
  
  // StoredKey = H(ClientKey)
  std::string client_key_str(reinterpret_cast<char*>(client_key.data()), client_key.size());
  std::vector<uint8_t> stored_key = Hash(client_key_str);
  
  // ClientSignature = HMAC(StoredKey, AuthMessage)
  std::vector<uint8_t> client_signature = Hmac(stored_key, auth_message_);
  
  // ClientProof = ClientKey XOR ClientSignature
  std::vector<uint8_t> client_proof = Xor(client_key, client_signature);
  
  oss << "p=" << Base64Encode(client_proof);
  
  return oss.str();
}

std::vector<uint8_t> ScratchBirdAuthClient::Hi(const std::string& password,
                                                const std::string& salt,
                                                int iterations,
                                                int key_len) {
  std::vector<uint8_t> result(key_len);
  
  // Use PKCS5_PBKDF2_HMAC for SCRAM
  const EVP_MD* digest = (scram_mechanism_ == ScramMechanism::kScramSha512) ?
                         EVP_sha512() : EVP_sha256();
  
  PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                    reinterpret_cast<const unsigned char*>(salt.c_str()), 
                    static_cast<int>(salt.length()),
                    iterations, digest, key_len, result.data());
  
  return result;
}

std::vector<uint8_t> ScratchBirdAuthClient::Hmac(const std::vector<uint8_t>& key,
                                                  const std::string& data) {
  const EVP_MD* digest = (scram_mechanism_ == ScramMechanism::kScramSha512) ?
                         EVP_sha512() : EVP_sha256();
  
  unsigned int result_len = EVP_MAX_MD_SIZE;
  std::vector<uint8_t> result(result_len);
  
  HMAC(digest, key.data(), static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char*>(data.c_str()), 
       static_cast<int>(data.length()),
       result.data(), &result_len);
  
  result.resize(result_len);
  return result;
}

std::vector<uint8_t> ScratchBirdAuthClient::Hash(const std::string& data) {
  const EVP_MD* digest = (scram_mechanism_ == ScramMechanism::kScramSha512) ?
                         EVP_sha512() : EVP_sha256();
  
  unsigned int result_len = EVP_MAX_MD_SIZE;
  std::vector<uint8_t> result(result_len);
  
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, digest, nullptr);
  EVP_DigestUpdate(ctx, data.c_str(), data.length());
  EVP_DigestFinal_ex(ctx, result.data(), &result_len);
  EVP_MD_CTX_free(ctx);
  
  result.resize(result_len);
  return result;
}

std::vector<uint8_t> ScratchBirdAuthClient::Xor(const std::vector<uint8_t>& a,
                                                 const std::vector<uint8_t>& b) {
  size_t len = std::min(a.size(), b.size());
  std::vector<uint8_t> result(len);
  
  for (size_t i = 0; i < len; ++i) {
    result[i] = a[i] ^ b[i];
  }
  
  return result;
}

std::string ScratchBirdAuthClient::Base64Encode(const std::vector<uint8_t>& data) {
  return Base64EncodeImpl(data);
}

std::vector<uint8_t> ScratchBirdAuthClient::Base64Decode(const std::string& str) {
  return Base64DecodeImpl(str);
}

std::string ScratchBirdAuthClient::GenerateClientNonce() {
  return GenerateNonce();
}

std::string ScratchBirdAuthClient::SaslPrep(const std::string& input) {
  // Simplified SASLprep - just return the input
  // Full implementation would normalize Unicode and prohibit certain characters
  return input;
}

AuthResult ScratchBirdAuthClient::AuthenticatePassword(const std::string& username,
                                                        const std::string& password) {
  Reset();
  auth_type_ = ScratchBirdAuthType::kPassword;
  
  AuthResult result;
  // In a real implementation, this would send the password
  // and verify the server's response
  result.success = true;  // Placeholder
  result.authenticated_user = username;
  
  return result;
}

AuthResult ScratchBirdAuthClient::AuthenticateCertificate(const std::string& username,
                                                           const std::vector<uint8_t>& certificate) {
  Reset();
  auth_type_ = ScratchBirdAuthType::kCertificate;
  
  AuthResult result;
  result.success = true;  // Placeholder
  result.authenticated_user = username;
  
  return result;
}

AuthResult ScratchBirdAuthClient::AuthenticateToken(const std::string& token) {
  Reset();
  auth_type_ = ScratchBirdAuthType::kToken;
  
  AuthResult result;
  result.success = true;  // Placeholder
  
  return result;
}

void ScratchBirdAuthClient::SetCredentialCallback(CredentialCallback callback) {
  credential_callback_ = callback;
}

// ============================================================================
// AuthPluginManager Implementation
// ============================================================================

AuthPluginManager::AuthPluginManager() = default;
AuthPluginManager::~AuthPluginManager() = default;

bool AuthPluginManager::Initialize(const std::string& plugin_dir) {
  plugin_dir_ = plugin_dir;
  
  // Register built-in methods
  MethodInfo scram256;
  scram256.id = "scram-sha-256";
  scram256.name = "SCRAM-SHA-256";
  scram256.description = "Salted Challenge Response Authentication Mechanism with SHA-256";
  scram256.is_builtin = true;
  methods_["scram-sha-256"] = scram256;
  
  MethodInfo scram512;
  scram512.id = "scram-sha-512";
  scram512.name = "SCRAM-SHA-512";
  scram512.description = "Salted Challenge Response Authentication Mechanism with SHA-512";
  scram512.is_builtin = true;
  methods_["scram-sha-512"] = scram512;
  
  MethodInfo password;
  password.id = "password";
  password.name = "Password";
  password.description = "Plain password authentication (not recommended)";
  password.is_builtin = true;
  methods_["password"] = password;
  
  MethodInfo cert;
  cert.id = "cert";
  cert.name = "Certificate";
  cert.description = "TLS client certificate authentication";
  cert.is_builtin = true;
  methods_["cert"] = cert;
  
  return true;
}

std::vector<std::string> AuthPluginManager::ListAvailableMethods() {
  std::vector<std::string> result;
  for (const auto& [id, info] : methods_) {
    result.push_back(id);
  }
  return result;
}

bool AuthPluginManager::IsMethodAvailable(const std::string& method_id) {
  return methods_.count(method_id) > 0;
}

std::optional<AuthPluginManager::MethodInfo> 
AuthPluginManager::GetMethodInfo(const std::string& method_id) {
  auto it = methods_.find(method_id);
  if (it != methods_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::unique_ptr<ScratchBirdAuthClient> 
AuthPluginManager::CreateAuthenticator(const std::string& method_id) {
  if (!IsMethodAvailable(method_id)) {
    return nullptr;
  }
  
  auto client = std::make_unique<ScratchBirdAuthClient>();
  
  // Configure based on method
  if (method_id == "scram-sha-256") {
    // Default configuration
  } else if (method_id == "scram-sha-512") {
    // Will use SHA-512
  }
  
  return client;
}

}  // namespace core
}  // namespace scratchrobin
