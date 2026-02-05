/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include "crypto_utils.h"

#include <cstring>
#include <iomanip>
#include <sstream>

namespace scratchrobin {

// SHA-256 Constants
static const unsigned int K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Rotate right
static inline unsigned int ROTR(unsigned int x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

// SHA-256 functions
static inline unsigned int Ch(unsigned int x, unsigned int y, unsigned int z) {
    return (x & y) ^ (~x & z);
}

static inline unsigned int Maj(unsigned int x, unsigned int y, unsigned int z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline unsigned int Sigma0(unsigned int x) {
    return ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22);
}

static inline unsigned int Sigma1(unsigned int x) {
    return ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25);
}

static inline unsigned int sigma0(unsigned int x) {
    return ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3);
}

static inline unsigned int sigma1(unsigned int x) {
    return ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10);
}

void Sha256::Transform(unsigned int state[8], const unsigned char block[64]) {
    unsigned int W[64];
    unsigned int a, b, c, d, e, f, g, h;
    unsigned int T1, T2;
    
    // Prepare message schedule
    for (int i = 0; i < 16; i++) {
        W[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) |
               (block[i * 4 + 2] << 8) | block[i * 4 + 3];
    }
    
    for (int i = 16; i < 64; i++) {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }
    
    // Initialize working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];
    
    // Main loop
    for (int i = 0; i < 64; i++) {
        T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
        T2 = Sigma0(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }
    
    // Update state
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

std::string Sha256::Hash(const std::string& input) {
    // Initial hash values
    unsigned int state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    // Prepare message
    std::vector<unsigned char> message(input.begin(), input.end());
    
    // Padding
    size_t original_length = message.size();
    message.push_back(0x80);
    
    while ((message.size() % 64) != 56) {
        message.push_back(0x00);
    }
    
    // Append length (in bits) as 64-bit big-endian
    unsigned long long length_bits = original_length * 8;
    for (int i = 7; i >= 0; i--) {
        message.push_back((length_bits >> (i * 8)) & 0xFF);
    }
    
    // Process message in 512-bit chunks
    for (size_t i = 0; i < message.size(); i += 64) {
        Transform(state, &message[i]);
    }
    
    // Produce output
    std::string result;
    result.reserve(32);
    for (int i = 0; i < 8; i++) {
        result.push_back((state[i] >> 24) & 0xFF);
        result.push_back((state[i] >> 16) & 0xFF);
        result.push_back((state[i] >> 8) & 0xFF);
        result.push_back(state[i] & 0xFF);
    }
    
    return result;
}

std::string Sha256::Hash(const std::string& input, const std::string& salt) {
    return Hash(salt + input);
}

std::string Sha256::HashToHex(const std::string& input) {
    std::string hash = Hash(input);
    std::ostringstream oss;
    for (unsigned char c : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string Sha256::HashToHex(const std::string& input, const std::string& salt) {
    return HashToHex(salt + input);
}

std::string Sha256::Hmac(const std::string& key, const std::string& message) {
    // Simplified HMAC implementation
    std::string ipad_key(64, 0x36);
    std::string opad_key(64, 0x5c);
    
    std::string key_padded = key;
    if (key_padded.length() > 64) {
        key_padded = Hash(key_padded);
    }
    key_padded.resize(64, 0);
    
    for (size_t i = 0; i < 64; i++) {
        ipad_key[i] ^= key_padded[i];
        opad_key[i] ^= key_padded[i];
    }
    
    std::string inner = ipad_key + message;
    std::string inner_hash = Hash(inner);
    std::string outer = opad_key + inner_hash;
    
    return HashToHex(outer);
}


// ============================================================================
// Format-Preserving Encryption Implementation (Simplified FF1)
// ============================================================================
FormatPreservingEncryption::FormatPreservingEncryption(const std::string& key) : key_(key) {}

std::string FormatPreservingEncryption::Encrypt(const std::string& plaintext) {
    // Determine radix based on content
    bool all_digits = true;
    for (char c : plaintext) {
        if (!std::isdigit(c)) {
            all_digits = false;
            break;
        }
    }
    
    if (all_digits) {
        return FF1Encrypt(plaintext, "0123456789", "");
    }
    
    // Default: alphanumeric
    return FF1Encrypt(plaintext, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", "");
}

std::string FormatPreservingEncryption::Decrypt(const std::string& ciphertext) {
    bool all_digits = true;
    for (char c : ciphertext) {
        if (!std::isdigit(c)) {
            all_digits = false;
            break;
        }
    }
    
    if (all_digits) {
        return FF1Decrypt(ciphertext, "0123456789", "");
    }
    
    return FF1Decrypt(ciphertext, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", "");
}

std::string FormatPreservingEncryption::Encrypt(const std::string& plaintext, const std::string& tweak) {
    return FF1Encrypt(plaintext, "0123456789", tweak);
}

std::string FormatPreservingEncryption::Decrypt(const std::string& ciphertext, const std::string& tweak) {
    return FF1Decrypt(ciphertext, "0123456789", tweak);
}

std::string FormatPreservingEncryption::FF1Encrypt(const std::string& plaintext,
                                                    const std::string& radix,
                                                    const std::string& tweak) {
    // Simplified FF1-like encryption
    // In production, use proper FF1 from NIST SP 800-38G
    
    if (plaintext.empty()) return plaintext;
    
    std::string result = plaintext;
    unsigned int seed = 0;
    
    // Create seed from key and tweak
    std::string combined = key_ + tweak;
    for (char c : combined) {
        seed = seed * 31 + c;
    }
    
    // Simple Feistel-like structure
    for (int round = 0; round < 10; round++) {
        seed = seed * 1103515245 + 12345;
        
        for (size_t i = 0; i < result.length(); i++) {
            size_t pos = radix.find(result[i]);
            if (pos != std::string::npos) {
                size_t new_pos = (pos + seed + i * 31 + round * 17) % radix.length();
                result[i] = radix[new_pos];
            }
        }
    }
    
    return result;
}

std::string FormatPreservingEncryption::FF1Decrypt(const std::string& ciphertext,
                                                    const std::string& radix,
                                                    const std::string& tweak) {
    if (ciphertext.empty()) return ciphertext;
    
    std::string result = ciphertext;
    unsigned int seed = 0;
    
    std::string combined = key_ + tweak;
    for (char c : combined) {
        seed = seed * 31 + c;
    }
    
    // Reverse the Feistel rounds
    for (int round = 9; round >= 0; round--) {
        seed = seed * 1103515245 + 12345;
        
        for (size_t i = 0; i < result.length(); i++) {
            size_t pos = radix.find(result[i]);
            if (pos != std::string::npos) {
                // Reverse the operation
                size_t prev_pos = (pos + radix.length() - ((seed + i * 31 + round * 17) % radix.length())) % radix.length();
                result[i] = radix[prev_pos];
            }
        }
    }
    
    return result;
}

std::string FormatPreservingEncryption::EncryptCreditCard(const std::string& card_number) {
    // Keep first 6 (BIN) and last 4 digits, encrypt middle
    if (card_number.length() < 13) return card_number;
    
    std::string prefix = card_number.substr(0, 6);
    std::string suffix = card_number.substr(card_number.length() - 4);
    std::string middle = card_number.substr(6, card_number.length() - 10);
    
    std::string encrypted_middle = Encrypt(middle);
    
    // Ensure encrypted middle has same length
    while (encrypted_middle.length() < middle.length()) {
        encrypted_middle += "0";
    }
    encrypted_middle = encrypted_middle.substr(0, middle.length());
    
    return prefix + encrypted_middle + suffix;
}

std::string FormatPreservingEncryption::EncryptSSN(const std::string& ssn) {
    // Format: XXX-XX-XXXX or XXXXXXXXX
    std::string digits;
    for (char c : ssn) {
        if (std::isdigit(c)) digits += c;
    }
    
    if (digits.length() != 9) return ssn;
    
    std::string encrypted = Encrypt(digits);
    while (encrypted.length() < 9) encrypted += "0";
    encrypted = encrypted.substr(0, 9);
    
    // Return in original format
    if (ssn.find('-') != std::string::npos) {
        return encrypted.substr(0, 3) + "-" + encrypted.substr(3, 2) + "-" + encrypted.substr(5);
    }
    return encrypted;
}

std::string FormatPreservingEncryption::EncryptPhone(const std::string& phone) {
    // Keep country code and area code, encrypt rest
    std::string digits;
    for (char c : phone) {
        if (std::isdigit(c)) digits += c;
    }
    
    if (digits.length() < 10) return phone;
    
    std::string prefix = digits.substr(0, digits.length() - 7);
    std::string middle = digits.substr(digits.length() - 7, 3);
    std::string suffix = digits.substr(digits.length() - 4);
    
    std::string encrypted_suffix = Encrypt(suffix);
    while (encrypted_suffix.length() < 4) encrypted_suffix += "0";
    encrypted_suffix = encrypted_suffix.substr(0, 4);
    
    // Reconstruct with original formatting
    std::string result;
    size_t digit_idx = 0;
    for (char c : phone) {
        if (std::isdigit(c)) {
            if (digit_idx < prefix.length() + 3) {
                result += c;  // Keep prefix and middle
            } else {
                result += encrypted_suffix[digit_idx - prefix.length() - 3];
            }
            digit_idx++;
        } else {
            result += c;  // Keep non-digits (dashes, spaces, etc.)
        }
    }
    
    return result;
}

std::string FormatPreservingEncryption::DecryptCreditCard(const std::string& encrypted) {
    return Decrypt(encrypted);  // Same as general decrypt
}

std::string FormatPreservingEncryption::DecryptSSN(const std::string& encrypted) {
    return Decrypt(encrypted);
}

std::string FormatPreservingEncryption::DecryptPhone(const std::string& encrypted) {
    return Decrypt(encrypted);
}

std::string FormatPreservingEncryption::NumToStr(unsigned long long num, int min_length) {
    std::string result;
    while (num > 0) {
        result = char('0' + (num % 10)) + result;
        num /= 10;
    }
    while ((int)result.length() < min_length) {
        result = "0" + result;
    }
    return result;
}

unsigned long long FormatPreservingEncryption::StrToNum(const std::string& str) {
    unsigned long long result = 0;
    for (char c : str) {
        if (std::isdigit(c)) {
            result = result * 10 + (c - '0');
        }
    }
    return result;
}

} // namespace scratchrobin
