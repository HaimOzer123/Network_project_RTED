/**
 * @file udp_file_transfer.cpp
 * @brief UDP File Transfer System Implementation File
 */

#include <vector>
#include <iostream>
#include <cstdint>
#include <numeric>
#include <fstream>
#include <string>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <chrono>

/**
 * @brief Calculates the checksum for a given data vector.
 * 
 * @param data A vector of bytes representing the data for which the checksum is calculated.
 * @return uint32_t The computed checksum as a 32-bit unsigned integer.
 */
uint32_t calculate_checksum(const std::vector<uint8_t>& data) {
    return std::accumulate(data.begin(), data.end(), 0u);
}

/**
 * @brief Verifies the checksum of a given data vector against an expected checksum.
 * 
 * @param data A vector of bytes representing the data to verify.
 * @param checksum The expected checksum value.
 * @return true If the checksum matches the calculated checksum for the data.
 * @return false If the checksum does not match.
 */
bool verify_checksum(const std::vector<uint8_t>& data, uint32_t checksum) {
    return calculate_checksum(data) == checksum;
}

/**
 * @brief Encrypts data using AES-256-CBC.
 * 
 * @param data The plaintext data to encrypt.
 * @param key  The encryption key (must be 32 bytes for AES-256).
 * @param iv   The initialization vector (16 bytes).
 * @return std::vector<uint8_t> Encrypted data.
 */
std::vector<uint8_t> aes_encrypt(const std::vector<uint8_t>& data, const std::string& key, const std::string& iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> encrypted(data.size() + AES_BLOCK_SIZE);

    int len, ciphertext_len;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, (const uint8_t*)key.data(), (const uint8_t*)iv.data());
    EVP_EncryptUpdate(ctx, encrypted.data(), &len, data.data(), data.size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, encrypted.data() + len, &len);
    ciphertext_len += len;

    encrypted.resize(ciphertext_len);
    EVP_CIPHER_CTX_free(ctx);

    return encrypted;
}

/**
 * @brief Decrypts data using AES-256-CBC.
 * 
 * @param data The encrypted data to decrypt.
 * @param key  The decryption key (must be 32 bytes for AES-256).
 * @param iv   The initialization vector (16 bytes).
 * @return std::vector<uint8_t> Decrypted data.
 */
std::vector<uint8_t> aes_decrypt(const std::vector<uint8_t>& data, const std::string& key, const std::string& iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> decrypted(data.size());

    int len, plaintext_len;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, (const uint8_t*)key.data(), (const uint8_t*)iv.data());
    EVP_DecryptUpdate(ctx, decrypted.data(), &len, data.data(), data.size());
    plaintext_len = len;

    EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &len);
    plaintext_len += len;

    decrypted.resize(plaintext_len);
    EVP_CIPHER_CTX_free(ctx);

    return decrypted;
}

/**
 * @brief Logs an error message to a file named `server_error.log`.
 * 
 * @param message The error message to log.
 */
void log_error(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ofstream log("server_error.log", std::ios::app);
    if (log.is_open()) {
        log << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;
    } else {
        std::cerr << "Failed to open log file for writing." << std::endl;
    }
}

/**
 * @brief Generates a versioned filename with a timestamp.
 * 
 * @param filename The original filename.
 * @return std::string The versioned filename with a timestamp appended.
 */
 std::string generate_versioned_filename(const std::string& filename) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << filename << "_v" << std::put_time(std::localtime(&time), "%Y%m%d%H%M%S");
    return oss.str();
}
