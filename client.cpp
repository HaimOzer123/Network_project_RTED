/**
 * @file client.cpp
 * @brief Implements the client-side logic for the UDP File Transfer System.
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>

/**
 * @brief Sends a file transfer request to the server.
 * @param serverIP The server's IP address.
 * @param port The server's port number.
 * @param packet The request packet.
 */
void send_request(const std::string& serverIP, int port, const Packet& packet) {
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
    recvfrom(sockfd, ack, PACKET_SIZE, 0, (struct sockaddr*)&serverAddr, &len);

    std::cout << "Response received: " << ack << std::endl;

    close(sockfd);
}

int main() {
    std::string serverIP = "127.0.0.1";
    int port = 12345;

    Packet packet = {RRQ, "example.txt", {}};
    send_request(serverIP, port, packet);

    return 0;
}
