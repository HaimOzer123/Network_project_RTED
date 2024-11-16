/**
 * @file server.cpp
 * @brief Implements the server-side logic for the UDP File Transfer System.
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <unistd.h>

/// Server storage directory
const std::string SERVER_STORAGE_DIR = "./server_files/";

/**
 * @brief Starts the UDP server.
 * @param port The port number to listen on.
 */
void start_server(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        Packet packet;
        sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        recvfrom(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &len);

        switch (packet.operationID) {
            case RRQ: {
                std::string filePath = SERVER_STORAGE_DIR + packet.filename;
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    std::cerr << "File not found: " << packet.filename << std::endl;
                    break;
                }
                char buffer[PACKET_SIZE];
                while (file.read(buffer, PACKET_SIZE) || file.gcount() > 0) {
                    sendto(sockfd, buffer, file.gcount(), 0, (struct sockaddr*)&clientAddr, len);
                }
                file.close();
                break;
            }
            case WRQ: {
                std::string filePath = SERVER_STORAGE_DIR + packet.filename;
                std::ofstream file(filePath, std::ios::binary);
                if (!file) {
                    std::cerr << "Could not create file: " << packet.filename << std::endl;
                    break;
                }
                char buffer[PACKET_SIZE];
                ssize_t received;
                while ((received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&clientAddr, &len)) > 0) {
                    file.write(buffer, received);
                }
                file.close();
                break;
            }
            case DEL: {
                std::string filePath = SERVER_STORAGE_DIR + packet.filename;
                if (remove(filePath.c_str()) != 0) {
                    std::cerr << "Failed to delete file: " << packet.filename << std::endl;
                } else {
                    std::cout << "File deleted: " << packet.filename << std::endl;
                }
                break;
            }
            default:
                std::cerr << "Unknown operation code received: " << packet.operationID << std::endl;
                break;
        }
    }

    close(sockfd);
}

int main() {
    int port = 12345;
    start_server(port);
    return 0;
}
