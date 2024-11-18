/**
 * @file client.cpp
 * @brief Implements the client-side logic for the UDP File Transfer System.
 */

#include "udp_file_transfer.hpp"
#include <iostream>

#ifdef _WIN32
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
 * @brief Sends a request to the server and handles the response.
 * 
 * @param serverIP The server's IP address.
 * @param port The server's port number.
 * @param packet The packet containing the request to be sent.
 */
void send_request(const std::string& serverIP, int port, const Packet& packet) {
#ifdef _WIN32
    initialize_winsock();
#endif
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    char ack[PACKET_SIZE];
    socklen_t len = sizeof(serverAddr);
    if (recvfrom(sockfd, ack, PACKET_SIZE, 0, (struct sockaddr*)&serverAddr, &len) > 0) {
        std::cout << "Response received: " << ack << '\n';
    } else {
        std::cerr << "No acknowledgment received. Resending..." << '\n';
    }

    CLOSE_SOCKET(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * @brief The entry point of the client application.
 * Sends a read request (RRQ) to the server to download a file.
 * 
 * @return int Exit code.
 */
int main() {
    std::string serverIP = "127.0.0.1";
    int port = 12345;

    Packet packet = {RRQ, "example.txt", {}, 0};
    send_request(serverIP, port, packet);

    return 0;
}
