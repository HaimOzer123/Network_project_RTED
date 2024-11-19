/**
 * @file client.cpp
 * @author 
 * @brief UDP File Transfer Client
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#ifdef _WIN32
typedef int ssize_t;
#endif

/**
 * @brief Displays the main menu to the user.
 */
void show_menu() {
    std::cout << "\n=== UDP File Transfer Client ===\n";
    std::cout << "1. Download a file (RRQ)\n";
    std::cout << "2. Upload a file (WRQ)\n";
    std::cout << "3. Delete a file (DEL)\n";
    std::cout << "4. Exit\n";
    std::cout << "Choose an option (1-4): ";
}

/**
 * @brief Sends a request packet to the server and waits for acknowledgment.
 * @param sockfd The socket file descriptor.
 * @param serverAddr The server address structure.
 * @param packet The packet to send.
 * @return True if the acknowledgment was received; false otherwise.
 */
bool send_request_with_ack(int sockfd, const sockaddr_in& serverAddr, const Packet& packet) {
    socklen_t serverLen = sizeof(serverAddr);
    char ackBuffer[PACKET_SIZE];

    for (int attempt = 0; attempt < 3; ++attempt) { // Retry up to 3 times
        sendto(sockfd, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, serverLen);
        fd_set readfds;
        struct timeval timeout = {0, ACK_TIMEOUT * 1000}; // ACK_TIMEOUT in microseconds
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int activity = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            recvfrom(sockfd, ackBuffer, PACKET_SIZE, 0, nullptr, nullptr);
            return true; // ACK received
        }
    }

    std::cerr << "Acknowledgment not received after 3 attempts\n";
    return false;
}

/**
 * @brief Sends a Read Request (RRQ) to download a file from the server.
 */
void send_rrq(int sockfd, const sockaddr_in& serverAddr, const std::string& filename) {
    Packet packet = {RRQ, {}, {}, 0};
    strncpy(packet.filename, filename.c_str(), sizeof(packet.filename) - 1);

    if (!send_request_with_ack(sockfd, serverAddr, packet)) {
        std::cerr << "Error: Failed to send RRQ for " << filename << '\n';
        return;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not create file " << filename << '\n';
        return;
    }

    char buffer[PACKET_SIZE];
    while (true) {
        ssize_t received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, nullptr, nullptr);
        if (received <= 0) break;

        std::vector<uint8_t> decrypted = decrypt_data(std::vector<uint8_t>(buffer, buffer + received));
        if (!verify_checksum(decrypted, packet.checksum)) {
            std::cerr << "Error: Checksum mismatch for " << filename << '\n';
            continue;
        }
        file.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());

        Packet ack = {ACK, {}, {}, 0};
        sendto(sockfd, (char*)&ack, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    file.close();
    std::cout << "File downloaded successfully: " << filename << '\n';
}

/**
 * @brief Sends a Write Request (WRQ) to upload a file to the server.
 */
void send_wrq(int sockfd, const sockaddr_in& serverAddr, const std::string& filename) {
    Packet packet = {WRQ, {}, {}, 0};
    strncpy(packet.filename, filename.c_str(), sizeof(packet.filename) - 1);

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: File not found: " << filename << '\n';
        return;
    }

    while (file.read(reinterpret_cast<char*>(packet.data), sizeof(packet.data)) || file.gcount() > 0) {
        packet.dataSize = file.gcount();
        std::vector<uint8_t> encrypted = encrypt_data(
            std::vector<uint8_t>(packet.data, packet.data + packet.dataSize));
        memcpy(packet.data, encrypted.data(), encrypted.size());
        packet.checksum = calculate_checksum(encrypted);

        if (!send_request_with_ack(sockfd, serverAddr, packet)) {
            std::cerr << "Error: Failed to send WRQ for " << filename << '\n';
            break;
        }
    }

    file.close();
    std::cout << "File uploaded successfully: " << filename << '\n';
}

/**
 * @brief Sends a Delete Request (DEL) to delete a file on the server.
 */
void send_del(int sockfd, const sockaddr_in& serverAddr, const std::string& filename) {
    Packet packet = {DEL, {}, {}, 0};
    strncpy(packet.filename, filename.c_str(), sizeof(packet.filename) - 1);

    if (!send_request_with_ack(sockfd, serverAddr, packet)) {
        std::cerr << "Error: Failed to send DEL request for " << filename << '\n';
        return;
    }

    std::cout << "Delete request sent successfully for file: " << filename << '\n';
}

/**
 * @brief Main entry point for the client application.
 */
int main() {
    std::string serverIP = "127.0.0.1";
    int port = 12345;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    while (true) {
        show_menu();

        int choice;
        std::cin >> choice;

        if (choice == 4) {
            std::cout << "Exiting...\n";
            break;
        }

        std::string filename;
        std::cout << "Enter filename: ";
        std::cin >> filename;

        switch (choice) {
            case 1:
                send_rrq(sockfd, serverAddr, filename);
                break;
            case 2:
                send_wrq(sockfd, serverAddr, filename);
                break;
            case 3:
                send_del(sockfd, serverAddr, filename);
                break;
            default:
                std::cerr << "Invalid choice! Please try again.\n";
                break;
        }
    }

    CLOSE_SOCKET(sockfd);
    return 0;
}
