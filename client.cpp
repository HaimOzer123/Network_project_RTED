/**
 * @file client.cpp
 * @brief Implements client-side logic for a UDP File Transfer System.
 */

#include "udp_file_transfer.hpp"
#include <iostream>
#include <cstring>
#include <fstream>

/**
 * @brief Sends a request packet to the server.
 * 
 * @param serverIP The server's IP address.
 * @param port The server's port number.
 * @param packet The packet containing the request to be sent.
 */
void send_request(const std::string& serverIP, int port, const Packet& packet);

/**
 * @brief Sends a Read Request (RRQ) to the server to download a file.
 * 
 * @param serverIP The server's IP address.
 * @param port The server's port number.
 * @param filename The name of the file to be downloaded.
 */
void send_rrq(const std::string& serverIP, int port, const std::string& filename) {
    Packet packet = {RRQ, {}, {}, 0};
    strncpy(packet.filename, filename.c_str(), sizeof(packet.filename) - 1);
    send_request(serverIP, port, packet);
}

/**
 * @brief Sends a Write Request (WRQ) to the server to upload a file.
 * 
 * @param serverIP The server's IP address.
 * @param port The server's port number.
 * @param filename The name of the file to be uploaded.
 */
void send_wrq(const std::string& serverIP, int port, const std::string& filename) {
    Packet packet = {WRQ, {}, {}, 0};
    strncpy(packet.filename, filename.c_str(), sizeof(packet.filename) - 1);

    // Open the file for reading in binary mode
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "File not found: " << filename << '\n';
        return;
    }

    // Read the file's content into the packet's data buffer
    file.read(reinterpret_cast<char*>(packet.data), sizeof(packet.data));
    packet.dataSize = file.gcount();

    send_request(serverIP, port, packet);
}

/**
 * @brief Sends a Delete Request (DEL) to the server to delete a file.
 * 
 * @param serverIP The server's IP address.
 * @param port The server's port number.
 * @param filename The name of the file to be deleted.
 */
void send_del(const std::string& serverIP, int port, const std::string& filename) {
    Packet packet = {DEL, {}, {}, 0};
    strncpy(packet.filename, filename.c_str(), sizeof(packet.filename) - 1);
    send_request(serverIP, port, packet);
}

/**
 * @brief The main entry point for the client application.
 * 
 * The user is prompted to choose an operation: Read Request (RRQ), Write Request (WRQ), or Delete Request (DEL).
 * Based on the input, the appropriate function is invoked.
 * 
 * @return int Exit code.
 */
int main() {
    std::string serverIP = "127.0.0.1"; ///< The server's IP address
    int port = 12345; ///< The server's port number

    std::cout << "Choose operation: [1] RRQ [2] WRQ [3] DEL: ";
    int choice;
    std::cin >> choice;

    std::string filename;
    std::cout << "Enter filename: ";
    std::cin >> filename;

    switch (choice) {
        case 1:
            send_rrq(serverIP, port, filename);
            break;
        case 2:
            send_wrq(serverIP, port, filename);
            break;
        case 3:
            send_del(serverIP, port, filename);
            break;
        default:
            std::cerr << "Invalid choice\n";
            break;
    }

    return 0;
}
