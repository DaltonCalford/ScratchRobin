/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CRYPTO_UTILS_H
#define SCRATCHROBIN_CRYPTO_UTILS_H

#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// SHA-256 Implementation
// ============================================================================
class Sha256 {
public:
    // Compute SHA-256 hash of input string
    static std::string Hash(const std::string& input);
    static std::string Hash(const std::string& input, const std::string& salt);
    
    // Compute hash and return as hex string
    static std::string HashToHex(const std::string& input);
    static std::string HashToHex(const std::string& input, const std::string& salt);
    
    // HMAC-SHA256
    static std::string Hmac(const std::string& key, const std::string& message);
    
private:
    static void Transform(unsigned int state[8], const unsigned char block[64]);
    static void Pad(std::vector<unsigned char>& message);
};

// ============================================================================
// Format-Preserving Encryption (FPE)
// ============================================================================
class FormatPreservingEncryption {
public:
    // Initialize with a key
    explicit FormatPreservingEncryption(const std::string& key);
    
    // Encrypt preserving format (e.g., digits stay digits)
    std::string Encrypt(const std::string& plaintext);
    std::string Encrypt(const std::string& plaintext, const std::string& tweak);
    
    // Decrypt
    std::string Decrypt(const std::string& ciphertext);
    std::string Decrypt(const std::string& ciphertext, const std::string& tweak);
    
    // Format-preserving encryption for specific types
    std::string EncryptCreditCard(const std::string& card_number);
    std::string EncryptSSN(const std::string& ssn);
    std::string EncryptPhone(const std::string& phone);
    
    std::string DecryptCreditCard(const std::string& encrypted);
    std::string DecryptSSN(const std::string& encrypted);
    std::string DecryptPhone(const std::string& encrypted);
    
private:
    std::string key_;
    
    // FF1 mode Feistel network
    std::string FF1Encrypt(const std::string& plaintext, 
                           const std::string& radix,
                           const std::string& tweak);
    std::string FF1Decrypt(const std::string& ciphertext,
                           const std::string& radix,
                           const std::string& tweak);
    
    // Helper functions
    std::string NumToStr(unsigned long long num, int min_length);
    unsigned long long StrToNum(const std::string& str);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CRYPTO_UTILS_H
