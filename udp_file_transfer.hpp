/**
 * @file udp_file_transfer.hpp
 * @brief Contains shared constants, structs, and helper functions for the UDP File Transfer System.
 */

#ifndef UDP_FILE_TRANSFER_HPP
#define UDP_FILE_TRANSFER_HPP

#include <string>
#include <vector>

/// Maximum packet size for UDP
constexpr size_t PACKET_SIZE = 512;

/// Operation codes for requests
enum OperationCode {
    RRQ = 1, ///< Read Request (Download)
    WRQ,     ///< Write Request (Upload)
    DEL      ///< Delete Request
};

/**
 * @struct Packet
 * @brief Structure for UDP packets used in file transfer.
 */
struct Packet {
    int operationID;           ///< Operation code (RRQ, WRQ, DEL)
    std::string filename;      ///< File name to operate on
    std::vector<uint8_t> data; ///< File data (for upload/download)
};

/**
 * @brief Calculates checksum for a given data vector.
 * @param data Data for which to compute the checksum.
 * @return A string representing the checksum.
 */
std::string calculate_checksum(const std::vector<uint8_t>& data);

/**
 * @brief Encrypts the given data.
 * @param data Data to encrypt.
 * @return Encrypted data.
 */
std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& data);

/**
 * @brief Decrypts the given data.
 * @param data Data to decrypt.
 * @return Decrypted data.
 */
std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& data);

#endif // UDP_FILE_TRANSFER_HPP
