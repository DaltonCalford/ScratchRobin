#include "core/sha256.h"

#include <array>
#include <sstream>

namespace scratchrobin {

namespace {

constexpr std::array<std::uint32_t, 64> kSha256K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

inline std::uint32_t RotR(std::uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

inline std::uint32_t Ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline std::uint32_t Maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline std::uint32_t BSig0(std::uint32_t x) {
    return RotR(x, 2) ^ RotR(x, 13) ^ RotR(x, 22);
}

inline std::uint32_t BSig1(std::uint32_t x) {
    return RotR(x, 6) ^ RotR(x, 11) ^ RotR(x, 25);
}

inline std::uint32_t SSig0(std::uint32_t x) {
    return RotR(x, 7) ^ RotR(x, 18) ^ (x >> 3);
}

inline std::uint32_t SSig1(std::uint32_t x) {
    return RotR(x, 17) ^ RotR(x, 19) ^ (x >> 10);
}

std::array<std::uint8_t, 32> Sha256Raw(const std::uint8_t* data, std::size_t size) {
    std::array<std::uint32_t, 8> h = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };

    std::vector<std::uint8_t> msg(data, data + size);
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }

    std::uint64_t bit_len = static_cast<std::uint64_t>(size) * 8;
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<std::uint8_t>((bit_len >> (8 * i)) & 0xFF));
    }

    std::array<std::uint32_t, 64> w{};
    for (std::size_t off = 0; off < msg.size(); off += 64) {
        for (int i = 0; i < 16; ++i) {
            std::size_t j = off + static_cast<std::size_t>(i) * 4;
            w[i] = (static_cast<std::uint32_t>(msg[j]) << 24) |
                   (static_cast<std::uint32_t>(msg[j + 1]) << 16) |
                   (static_cast<std::uint32_t>(msg[j + 2]) << 8) |
                   (static_cast<std::uint32_t>(msg[j + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = SSig1(w[i - 2]) + w[i - 7] + SSig0(w[i - 15]) + w[i - 16];
        }

        std::uint32_t a = h[0];
        std::uint32_t b = h[1];
        std::uint32_t c = h[2];
        std::uint32_t d = h[3];
        std::uint32_t e = h[4];
        std::uint32_t f = h[5];
        std::uint32_t g = h[6];
        std::uint32_t hh = h[7];

        for (int i = 0; i < 64; ++i) {
            std::uint32_t t1 = hh + BSig1(e) + Ch(e, f, g) + kSha256K[i] + w[i];
            std::uint32_t t2 = BSig0(a) + Maj(a, b, c);
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    std::array<std::uint8_t, 32> out{};
    for (int i = 0; i < 8; ++i) {
        out[4 * i] = static_cast<std::uint8_t>((h[i] >> 24) & 0xFF);
        out[4 * i + 1] = static_cast<std::uint8_t>((h[i] >> 16) & 0xFF);
        out[4 * i + 2] = static_cast<std::uint8_t>((h[i] >> 8) & 0xFF);
        out[4 * i + 3] = static_cast<std::uint8_t>(h[i] & 0xFF);
    }
    return out;
}

std::string ToHex(const std::array<std::uint8_t, 32>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (std::uint8_t b : bytes) {
        out.push_back(kHex[(b >> 4) & 0x0F]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

}  // namespace

std::string Sha256Hex(std::string_view data) {
    const auto digest = Sha256Raw(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
    return ToHex(digest);
}

std::string Sha256Hex(const std::vector<std::uint8_t>& data) {
    const auto digest = Sha256Raw(data.data(), data.size());
    return ToHex(digest);
}

}  // namespace scratchrobin
