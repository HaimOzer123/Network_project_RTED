/**
 * @file udp_file_transfer.hpp
 * @author 
 * @brief UDP File Transfer System Header File
 */

#ifndef UDP_FILE_TRANSFER_HPP
#define UDP_FILE_TRANSFER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

/// Maximum packet size for UDP communication.
constexpr size_t PACKET_SIZE = 512;

/// Acknowledgment timeout in milliseconds.
constexpr int ACK_TIMEOUT = 1000;

/// Enumeration of operation codes for client-server communication.
enum OperationCode {
    RRQ = 1, ///< Read Request (Download a file)
    WRQ,     ///< Write Request (Upload a file)
    DEL,     ///< Delete Request (Remove a file)
    ACK,     ///< Acknowledgment Packet
    ERROR_PACKET ///< Error Packet
};

/**
 * @class Packet
 * @brief Represents a packet used in the UDP File Transfer System.
 */
struct Packet {
    int operationID;           ///< Operation code (e.g., RRQ, WRQ, DEL, ACK, ERROR_PACKET)
    char filename[256];        ///< File name to operate on
    uint8_t data[PACKET_SIZE]; ///< Data payload (for WRQ or RRQ responses)
    uint32_t checksum;         ///< Checksum for integrity verification
    size_t dataSize;           ///< Size of valid data in the packet
};

/**
 * @brief Computes a checksum for a given data vector.
 * @param data The data vector for which the checksum is to be computed.
 * @return The computed checksum as a 32-bit unsigned integer.
 */
uint32_t calculate_checksum(const std::vector<uint8_t>& data);

/**
 * @brief Verifies the checksum of a given data block.
 * @param data The data vector to verify.
 * @param checksum The expected checksum.
 * @return True if the checksum is valid, false otherwise.
 */
bool verify_checksum(const std::vector<uint8_t>& data, uint32_t checksum);

/**
 * @brief Encrypts a block of data.
 * @param data The data vector to encrypt.
 * @return The encrypted data as a vector of uint8_t.
 */
std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& data);

/**
 * @brief Decrypts a block of data.
 * @param data The data vector to decrypt.
 * @return The decrypted data as a vector of uint8_t.
 */
std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& data);

/**
 * @brief Logs an error message to a file.
 * @param message The error message to log.
 */
void log_error(const std::string& message);

/**
 * @brief Validates the existence of necessary directories for file storage.
 * @details Creates directories if they do not exist.
 */
void validate_directories();

/**
 * @brief Cross-platform function to close a socket.
 */
#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

#endif // UDP_FILE_TRANSFER_HPP
