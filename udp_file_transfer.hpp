/**
 * @file udp_file_transfer.hpp
 * @brief Contains shared constants, enums, structs, and helper functions for the UDP File Transfer System.
 */

#ifndef UDP_FILE_TRANSFER_HPP
#define UDP_FILE_TRANSFER_HPP

#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#endif

/// Maximum packet size for UDP communication.
constexpr size_t PACKET_SIZE = 512;

/// Enumeration of operation codes for client-server communication.
enum OperationCode {
    RRQ = 1, ///< Read Request (Download a file)
    WRQ,     ///< Write Request (Upload a file)
    DEL      ///< Delete Request (Remove a file)
};

/**
 * @class Packet
 * @brief Represents a packet used in the UDP File Transfer System.
 *
 * Defines the format of packets exchanged between the client and server. Each
 * packet includes the operation code, the file name, data payload, and a checksum
 * to ensure data integrity.
 */
struct Packet {
    int operationID;           ///< Operation code (e.g., RRQ, WRQ, DEL)
    char filename[256];        ///< File name to operate on
    uint8_t data[PACKET_SIZE]; ///< Data payload (for WRQ or RRQ responses)
    uint32_t checksum;         ///< Checksum for integrity verification
    size_t dataSize;           ///< Size of valid data in the packet
};

/**
 * @brief Computes a checksum for a given data vector.
 *
 * This function generates a checksum for a block of data to ensure its integrity.
 *
 * @param data The data block for which to compute the checksum.
 * @return A 32-bit checksum value.
 */
uint32_t calculate_checksum(const std::vector<uint8_t>& data);

/**
 * @brief Verifies the checksum of a given data block.
 *
 * Checks if the provided checksum matches the calculated checksum for the data.
 *
 * @param data The data block to verify.
 * @param checksum The checksum to compare against.
 * @return True if the checksum matches, false otherwise.
 */
bool verify_checksum(const std::vector<uint8_t>& data, uint32_t checksum);

/**
 * @brief Encrypts a block of data.
 *
 * Encrypts data to ensure secure transmission between the client and server.
 *
 * @param data The data to encrypt.
 * @return A vector containing the encrypted data.
 */
std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& data);

/**
 * @brief Decrypts a block of data.
 *
 * Decrypts data received from the client or server.
 *
 * @param data The data to decrypt.
 * @return A vector containing the decrypted data.
 */
std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& data);

/**
 * @brief Cross-platform function to close a socket.
 * 
 * Defines a macro for closing a socket that works across different operating systems.
 */
#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

#endif // UDP_FILE_TRANSFER_HPP
