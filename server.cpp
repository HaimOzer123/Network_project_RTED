/**
 * @file server.cpp
 * @author Haim Ozer
 * @brief  UDP File Transfer server
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <filesystem>

#ifdef _WIN32
typedef int ssize_t;
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
 * @brief Logs errors to a file.
 * @param error_message The error message to log.
 */
void log_error(const std::string& error_message) {
    std::ofstream log("server_error.log", std::ios::app);
    log << error_message << std::endl;
}

/**
 * @brief Handles a single client request.
 * @param sockfd The server socket file descriptor.
 * @param clientAddr The client address structure.
 * @param packet The packet received from the client.
 */
void handle_client(int sockfd, sockaddr_in clientAddr, Packet packet) {
    socklen_t clientLen = sizeof(clientAddr);

    switch (packet.operationID) {
        case RRQ: { // Read Request
            std::string filePath = SERVER_STORAGE_DIR + packet.filename;
            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                const char* error = "Error: File not found.";
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
                log_error("File not found: " + filePath);
                return;
            }

            char buffer[PACKET_SIZE];
            while (file.read(buffer, PACKET_SIZE) || file.gcount() > 0) {
                std::vector<uint8_t> encrypted = encrypt_data(
                    std::vector<uint8_t>(buffer, buffer + file.gcount()));
                Packet response = {ACK, {}, {}, 0, file.gcount()};
                std::memcpy(response.data, encrypted.data(), encrypted.size());
                response.checksum = calculate_checksum(encrypted);
                sendto(sockfd, (char*)&response, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, clientLen);
            }
            file.close();
            break;
        }
        case WRQ: { // Write Request
            std::string filePath = SERVER_STORAGE_DIR + packet.filename;
            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                const char* error = "Error: Could not create file.";
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
                log_error("Could not create file: " + filePath);
                return;
            }

            ssize_t received;
            char buffer[PACKET_SIZE];
            while ((received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&clientAddr, &clientLen)) > 0) {
                std::vector<uint8_t> decrypted = decrypt_data(std::vector<uint8_t>(buffer, buffer + received));
                if (!verify_checksum(decrypted, packet.checksum)) {
                    log_error("Checksum mismatch for file: " + std::string(packet.filename));
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
                log_error("Failed to delete file: " + filePath);
            } else {
                const char* success = "Success: File deleted.";
                sendto(sockfd, success, strlen(success), 0, (struct sockaddr*)&clientAddr, clientLen);
            }
            break;
        }
        default: {
            const char* error = "Error: Unknown operation.";
            sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, clientLen);
            log_error("Unknown operation ID: " + std::to_string(packet.operationID));
            break;
        }
    }
}

/**
 * @brief Starts the UDP server to listen for client requests.
 * @param port The port number to listen on.
 * @details The server listens for incoming client requests and spawns a new thread to handle each request.
 */
void start_server(int port) {
    validate_directories();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log_error("WSAStartup failed.");
        return;
    }
#endif

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        log_error("Socket creation failed.");
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        log_error("Bind failed.");
        CLOSE_SOCKET(sockfd);
        return;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        Packet packet;
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        ssize_t received = recvfrom(sockfd, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &clientLen);
        if (received > 0) {
            std::lock_guard<std::mutex> lock(client_mutex);
            std::thread(handle_client, sockfd, clientAddr, packet).detach();
        }
    }

    CLOSE_SOCKET(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * @brief The main entry point of the server application.
 * @details The function starts the server on a specified port and listens for incoming client requests.
 */
int main() {
    int port = 12345;
    start_server(port);
    return 0;
}
