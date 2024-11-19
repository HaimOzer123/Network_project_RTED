/**
 * @file server.cpp
 * @brief Implements the server-side logic for the UDP File Transfer System.
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <fstream>

/// Directory where server stores uploaded files.
const std::string SERVER_STORAGE_DIR = "./server_files/";

/// Directory for backup copies of uploaded files.
const std::string BACKUP_STORAGE_DIR = "./backup_files/";

#ifdef _WIN32
/**
 * @brief Defines the ssize_t type for Windows.
 */
typedef int ssize_t;

/**
 * @brief Initializes the Windows Socket API.
 * This function must be called before any socket operations on Windows.
 */
void initialize_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << '\n';
        exit(EXIT_FAILURE);
    }
}
#endif

/**
 * @brief Starts the UDP server to listen for client requests.
 * 
 * @param port The port number on which the server listens for incoming requests.
 */
void start_server(int port) {
#ifdef _WIN32
    initialize_winsock();
#endif

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    // Configure the server address structure
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        CLOSE_SOCKET(sockfd);
        return;
    }

    std::cout << "Server listening on port " << port << '\n';

    while (true) {
        Packet packet;
        sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        // Receive a packet from a client
        recvfrom(sockfd, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &len);

        switch (packet.operationID) {
            case RRQ: { // Read Request
                std::string filePath = SERVER_STORAGE_DIR + packet.filename;
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    std::cerr << "File not found: " << packet.filename << '\n';
                    const char* error = "Error: File not found.";
                    sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, len);
                    break;
                }

                char buffer[PACKET_SIZE];
                while (file.read(buffer, PACKET_SIZE) || file.gcount() > 0) {
                    sendto(sockfd, buffer, file.gcount(), 0, (struct sockaddr*)&clientAddr, len);
                }
                file.close();
                std::cout << "File sent: " << packet.filename << '\n';
                break;
            }
            case WRQ: { // Write Request
                std::string filePath = SERVER_STORAGE_DIR + packet.filename;
                std::ofstream file(filePath, std::ios::binary);
                if (!file) {
                    std::cerr << "Could not create file: " << packet.filename << '\n';
                    const char* error = "Error: Could not create file.";
                    sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, len);
                    break;
                }

                char buffer[PACKET_SIZE];
                ssize_t received;
                while ((received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&clientAddr, &len)) > 0) {
                    file.write(buffer, received);
                }
                file.close();

                std::cout << "File uploaded: " << packet.filename << '\n';

                // Backup the file
                std::string backupPath = BACKUP_STORAGE_DIR + packet.filename;
                if (std::rename(filePath.c_str(), backupPath.c_str()) != 0) {
                    std::cerr << "Failed to create backup for file: " << packet.filename << '\n';
                } else {
                    std::cout << "File backed up: " << packet.filename << '\n';
                }
                break;
            }
            case DEL: { // Delete Request
                std::string filePath = SERVER_STORAGE_DIR + packet.filename;
                if (remove(filePath.c_str()) != 0) {
                    std::cerr << "Failed to delete file: " << packet.filename << '\n';
                    const char* error = "Error: Failed to delete file.";
                    sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, len);
                } else {
                    std::cout << "File deleted: " << packet.filename << '\n';
                    const char* success = "Success: File deleted.";
                    sendto(sockfd, success, strlen(success), 0, (struct sockaddr*)&clientAddr, len);
                }
                break;
            }
            default:
                std::cerr << "Unknown operation code received: " << packet.operationID << '\n';
                const char* error = "Error: Unknown operation.";
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr*)&clientAddr, len);
                break;
        }
    }

    // Close the socket
    CLOSE_SOCKET(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * @brief The main entry point of the server application.
 * 
 * Starts the server to listen for client requests on the specified port.
 * 
 * @return int Exit code.
 */
int main() {
    int port = 12345; ///< The port number on which the server listens for incoming requests
    start_server(port);
    return 0;
}
