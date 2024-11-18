/**
 * @file udp_file_transfer.hpp
 * @brief Contains shared constants, structs, and helper functions for the UDP File Transfer System.
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

/// Maximum packet size for UDP
constexpr size_t PACKET_SIZE = 512;

/// Operation codes for requests
enum OperationCode {
    RRQ = 1, ///< Read Request (Download)
    WRQ,     ///< Write Request (Upload)
    DEL      ///< Delete Request
};

/**
 * @class Packet
 * @brief Represents a packet used in the UDP File Transfer System.
 *
 * This structure defines the format of packets exchanged between the client
 * and the server in the file transfer system. It includes operation codes,
 * filenames, data payload, and a checksum for integrity.
 */
struct Packet {
    int operationID;           ///< Operation code (RRQ, WRQ, DEL)
    char filename[256];        ///< File name to operate on
    uint8_t data[PACKET_SIZE]; ///< File data
    uint32_t checksum;         ///< Checksum for file integrity
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

// Cross-platform socket close
#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

#endif // UDP_FILE_TRANSFER_HPP
