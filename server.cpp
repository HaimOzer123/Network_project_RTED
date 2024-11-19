/**
 * @file server.cpp
 * @brief UDP File Transfer Server with AES Encryption and Version Control
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <openssl/rand.h>

#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t; // Use MinGW-provided type
#endif

const std::string SERVER_STORAGE_DIR = "./server_files/";
const std::string BACKUP_STORAGE_DIR = "./backup_files/";

std::mutex client_mutex; // Mutex to manage client threads

/**
 * @brief Validates the existence of directories for storing files.
 * @details Creates `SERVER_STORAGE_DIR` and `BACKUP_STORAGE_DIR` if they do not exist.
 */
void validate_directories() {
    if (!std::filesystem::exists(SERVER_STORAGE_DIR)) {
        std::filesystem::create_directories(SERVER_STORAGE_DIR);
    }
    if (!std::filesystem::exists(BACKUP_STORAGE_DIR)) {
        std::filesystem::create_directories(BACKUP_STORAGE_DIR);
    }
}

/**
 * @brief Logs errors to a file with a timestamp.
 * @param message The error message to log.
 * @param clientAddr The client address structure.
 */
void log_error(const std::string& message, const sockaddr_in& clientAddr) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ofstream log("server_error.log", std::ios::app);
    if (log.is_open()) {
        log << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";
        log << "Client IP: " << inet_ntoa(clientAddr.sin_addr)
            << ", Port: " << ntohs(clientAddr.sin_port) << " - " << message << std::endl;
    }
}

/**
 * @brief Generates a versioned filename with a timestamp.
 * @param filename The original filename.
 * @return std::string The versioned filename.
 */
std::string generate_versioned_filename(const std::string& filename) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << filename << "_v" << std::put_time(std::localtime(&time), "%Y%m%d%H%M%S");
    return oss.str();
}

/**
 * @brief Handles a single client request.
 * @param sockfd The server socket file descriptor.
 * @param clientAddr The client address structure.
 * @param packet The packet received from the client.
 * @param key The AES encryption key.
 * @param iv The AES initialization vector.
 */
void handle_client(int sockfd, sockaddr_in clientAddr, Packet packet, const std::string& key, const std::string& iv) {
    socklen_t clientLen = sizeof(clientAddr);

    switch (packet.operationID) {
        case RRQ: { // Read Request
            std::string filePath = SERVER_STORAGE_DIR + packet.filename;
            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                const char* error = "Error: File not found.";
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
                log_error("File not found: " + filePath, clientAddr);
                return;
            }

            char buffer[PACKET_SIZE];
            while (file.read(buffer, PACKET_SIZE) || file.gcount() > 0) {
                std::vector<uint8_t> encrypted = aes_encrypt(
                    std::vector<uint8_t>(buffer, buffer + file.gcount()), key, iv);
                Packet response = {ACK, {}, {}, 0, static_cast<size_t>(file.gcount())};
                std::memcpy(response.data, encrypted.data(), encrypted.size());
                response.checksum = calculate_checksum(encrypted);
                sendto(sockfd, (char*)&response, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, clientLen);
            }
            file.close();
            break;
        }
        case WRQ: { // Write Request
            std::string filePath = generate_versioned_filename(SERVER_STORAGE_DIR + packet.filename);
            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                const char* error = "Error: Could not create file.";
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
                log_error("Could not create file: " + filePath, clientAddr);
                return;
            }

            ssize_t received;
            char buffer[PACKET_SIZE];
            while ((received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&clientAddr, &clientLen)) > 0) {
                std::vector<uint8_t> decrypted = aes_decrypt(std::vector<uint8_t>(buffer, buffer + received), key, iv);
                if (!verify_checksum(decrypted, packet.checksum)) {
                    log_error("Checksum mismatch for file: " + std::string(packet.filename), clientAddr);
                    continue;
                }
                file.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
            }
            file.close();
            break;
        }
        case DEL: { // Delete Request
            std::string filePath = SERVER_STORAGE_DIR + packet.filename;
            if (remove(filePath.c_str()) != 0) {
                const char* error = "Error: Failed to delete file.";
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
                log_error("Failed to delete file: " + filePath, clientAddr);
            } else {
                const char* success = "Success: File deleted.";
                sendto(sockfd, success, strlen(success), 0, (struct sockaddr*)&clientAddr, clientLen);
            }
            break;
        }
        default: {
            const char* error = "Error: Unknown operation.";
            sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
            log_error("Unknown operation ID: " + std::to_string(packet.operationID), clientAddr);
            break;
        }
    }
}

/**
 * @brief Starts the UDP server to listen for client requests.
 * @param port The port number to listen on.
 */
void start_server(int port) {
    validate_directories();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }
#endif

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed." << std::endl;
        CLOSE_SOCKET(sockfd);
        return;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        Packet packet;
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        // Generate AES key and IV
        std::string key(32, '\0'), iv(16, '\0');
        RAND_bytes(reinterpret_cast<uint8_t*>(&key[0]), key.size());
        RAND_bytes(reinterpret_cast<uint8_t*>(&iv[0]), iv.size());

        ssize_t received = recvfrom(sockfd, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &clientLen);
        if (received > 0) {
            std::lock_guard<std::mutex> lock(client_mutex);
            std::thread(handle_client, sockfd, clientAddr, packet, key, iv).detach();
        }
    }

    CLOSE_SOCKET(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * @brief Main entry point of the server application.
 */
int main() {
    int port = 12345;
    start_server(port);
    return 0;
}
