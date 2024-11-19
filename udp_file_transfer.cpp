/**
 * @file udp_file_transfer.cpp
 * @author 
 * @brief UDP File Transfer System Implementation File
 */
#include <vector>
#include <iostream>
#include <cstdint>
#include <numeric>
#include <fstream>
#include <string>

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
 * @brief Encrypts a vector of bytes using a simple XOR operation.
 * 
 * @param data A vector of bytes representing the data to encrypt.
 * @return std::vector<uint8_t> A vector of bytes representing the encrypted data.
 * 
 * @note This function uses XOR encryption with a fixed key (0xAA). It is a simple 
 *       and insecure encryption method, suitable only for demonstration purposes.
 */
std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> encrypted(data);
    for (auto& byte : encrypted) byte ^= 0xAA; // Simple XOR encryption
    return encrypted;
}

/**
 * @brief Decrypts a vector of bytes using a simple XOR operation.
 * 
 * @param data A vector of bytes representing the data to decrypt.
 * @return std::vector<uint8_t> A vector of bytes representing the decrypted data.
 * 
 * @note This function uses the same XOR operation as `encrypt_data`, since XOR is symmetric.
 */
std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& data) {
    return encrypt_data(data); // Same as encryption for XOR
}

/**
 * @brief Logs an error message to a file named `server_error.log`.
 * 
 * @param message The error message to log.
 */
void log_error(const std::string& message) {
    std::ofstream log("server_error.log", std::ios::app);
    if (log.is_open()) {
        log << message << std::endl;
    } else {
        std::cerr << "Failed to open log file for writing." << std::endl;
    }
}