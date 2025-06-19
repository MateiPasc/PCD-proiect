#include "../../include/common.h"
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int perform_key_exchange(int socket_fd, crypto_key_t* shared_key, int is_server);

class OrdinaryClient {
private:
    int socket_fd;
    bool connected;
    crypto_key_t encryption_key;
    std::string server_host;
    int server_port;
    
public:
    OrdinaryClient(const std::string& host = "localhost", int port = SERVER_PORT) 
        : socket_fd(-1), connected(false), server_host(host), server_port(port) {}
    
    ~OrdinaryClient() {
        disconnect();
    }
    
    bool connect_to_server() {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            perror("socket");
            return false;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        
        if (inet_pton(AF_INET, server_host.c_str(), &server_addr.sin_addr) <= 0) {
            perror("inet_pton");
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        connected = true;
        
        // Perform key exchange for E2E encryption
        if (perform_key_exchange(socket_fd, &encryption_key, 0) != 0) {
            std::cerr << "Key exchange failed" << std::endl;
            disconnect();
            return false;
        }
        
        std::cout << "E2E encryption established" << std::endl;
        
        // Register with server
        std::string register_cmd = "REGISTER_CLIENT";
        if (!send_command(register_cmd)) {
            disconnect();
            return false;
        }
        
        std::string response = receive_response();
        if (response.find("OK") != 0) {
            std::cerr << "Registration failed: " << response << std::endl;
            disconnect();
            return false;
        }
        
        std::cout << "Successfully connected and registered with server" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (socket_fd != -1) {
            close(socket_fd);
            socket_fd = -1;
        }
        connected = false;
    }
    
    bool send_command(const std::string& command) {
        if (!connected) return false;
        
        std::string cmd = command + "\n";
        int bytes_sent = send(socket_fd, cmd.c_str(), cmd.length(), 0);
        if (bytes_sent == -1) {
            perror("send");
            return false;
        }
        return true;
    }
    
    std::string receive_response() {
        if (!connected) return "";
        
        char buffer[MAX_MESSAGE];
        int bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            return "";
        }
        
        buffer[bytes_received] = '\0';
        
        // Remove trailing newline
        std::string response(buffer);
        if (!response.empty() && response.back() == '\n') {
            response.pop_back();
        }
        
        return response;
    }
    
    bool upload_file(const std::string& filepath) {
        if (!connected) {
            std::cerr << "Not connected to server" << std::endl;
            return false;
        }
        
        // Check if file exists
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << filepath << std::endl;
            return false;
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::cout << "Uploading file: " << filepath << " (" << file_size << " bytes)" << std::endl;
        
        // Extract filename from path
        size_t pos = filepath.find_last_of("/\\");
        std::string filename = (pos != std::string::npos) ? filepath.substr(pos + 1) : filepath;
        
        // Create temporary encrypted file
        std::string temp_encrypted = "/tmp/client_encrypted_" + filename;
        
        // Encrypt file
        if (encrypt_file(filepath.c_str(), temp_encrypted.c_str(), &encryption_key) != 0) {
            std::cerr << "File encryption failed" << std::endl;
            file.close();
            return false;
        }
        
        file.close();
        
        // Get encrypted file size
        std::ifstream encrypted_file(temp_encrypted, std::ios::binary);
        encrypted_file.seekg(0, std::ios::end);
        size_t encrypted_size = encrypted_file.tellg();
        encrypted_file.seekg(0, std::ios::beg);
        
        // Send upload command
        std::string upload_cmd = "UPLOAD_FILE " + filename + " " + std::to_string(encrypted_size);
        if (!send_command(upload_cmd)) {
            encrypted_file.close();
            unlink(temp_encrypted.c_str());
            return false;
        }
        
        // Wait for acknowledgment
        std::string response = receive_response();
        if (response != "OK Ready to receive file") {
            std::cerr << "Server not ready to receive file: " << response << std::endl;
            encrypted_file.close();
            unlink(temp_encrypted.c_str());
            return false;
        }
        
        // Send encrypted file data
        char buffer[BUFFER_SIZE];
        size_t total_sent = 0;
        
        while (total_sent < encrypted_size) {
            size_t to_read = std::min((size_t)BUFFER_SIZE, encrypted_size - total_sent);
            encrypted_file.read(buffer, to_read);
            size_t bytes_read = encrypted_file.gcount();
            
            if (bytes_read == 0) break;
            
            int bytes_sent = send(socket_fd, buffer, bytes_read, 0);
            if (bytes_sent <= 0) {
                perror("send file data");
                encrypted_file.close();
                unlink(temp_encrypted.c_str());
                return false;
            }
            
            total_sent += bytes_sent;
            
            // Show progress
            int progress = (total_sent * 100) / encrypted_size;
            std::cout << "\rProgress: " << progress << "% (" << total_sent << "/" << encrypted_size << " bytes)" << std::flush;
        }
        
        std::cout << std::endl;
        encrypted_file.close();
        unlink(temp_encrypted.c_str());
        
        // Wait for upload confirmation
        response = receive_response();
        if (response.find("OK") == 0) {
            std::cout << "File uploaded successfully" << std::endl;
            
            // Extract job ID from response
            size_t pos = response.find("Job ID: ");
            if (pos != std::string::npos) {
                std::string job_id = response.substr(pos + 8);
                std::cout << "Scan job created with ID: " << job_id << std::endl;
                return true;
            }
        } else {
            std::cerr << "Upload failed: " << response << std::endl;
        }
        
        return false;
    }
    
    std::string check_scan_status(const std::string& job_id) {
        if (!connected) return "Not connected";
        
        std::string status_cmd = "GET_SCAN_STATUS " + job_id;
        if (!send_command(status_cmd)) {
            return "Command failed";
        }
        
        return receive_response();
    }
    
    std::string get_scan_result(const std::string& job_id) {
        if (!connected) return "Not connected";
        
        std::string result_cmd = "GET_SCAN_RESULT " + job_id;
        if (!send_command(result_cmd)) {
            return "Command failed";
        }
        
        return receive_response();
    }
    
    void monitor_scan_async(const std::string& job_id) {
        std::thread monitor_thread([this, job_id]() {
            std::cout << "Monitoring scan job " << job_id << " (async)..." << std::endl;
            
            while (connected) {
                std::string status = check_scan_status(job_id);
                
                if (status.find("COMPLETED") != std::string::npos) {
                    std::cout << "\n*** Scan completed for job " << job_id << " ***" << std::endl;
                    
                    std::string result = get_scan_result(job_id);
                    std::cout << "Result: " << result << std::endl;
                    break;
                } else if (status.find("ERROR") != std::string::npos) {
                    std::cout << "\n*** Scan error for job " << job_id << " ***" << std::endl;
                    std::cout << "Status: " << status << std::endl;
                    break;
                } else if (status.find("PENDING") != std::string::npos || 
                          status.find("PROCESSING") != std::string::npos) {
                    std::cout << "Scan status: " << status << std::endl;
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
        
        monitor_thread.detach();
    }
    
    bool download_file(const std::string& filename, const std::string& local_path) {
        if (!connected) return false;
        
        std::string download_cmd = "DOWNLOAD_FILE " + filename;
        if (!send_command(download_cmd)) {
            return false;
        }
        
        std::string response = receive_response();
        if (response.find("SIZE ") != 0) {
            std::cerr << "Download failed: " << response << std::endl;
            return false;
        }
        
        // Parse file size
        size_t file_size = std::stoul(response.substr(5));
        std::cout << "Downloading " << filename << " (" << file_size << " bytes)" << std::endl;
        
        // Receive encrypted file
        std::string temp_encrypted = "/tmp/client_download_" + filename;
        std::ofstream encrypted_file(temp_encrypted, std::ios::binary);
        
        char buffer[BUFFER_SIZE];
        size_t total_received = 0;
        
        while (total_received < file_size) {
            size_t to_receive = std::min((size_t)BUFFER_SIZE, file_size - total_received);
            int bytes_received = recv(socket_fd, buffer, to_receive, 0);
            
            if (bytes_received <= 0) {
                perror("recv file data");
                encrypted_file.close();
                unlink(temp_encrypted.c_str());
                return false;
            }
            
            encrypted_file.write(buffer, bytes_received);
            total_received += bytes_received;
            
            int progress = (total_received * 100) / file_size;
            std::cout << "\rProgress: " << progress << "% (" << total_received << "/" << file_size << " bytes)" << std::flush;
        }
        
        std::cout << std::endl;
        encrypted_file.close();
        
        // Decrypt file
        if (decrypt_file(temp_encrypted.c_str(), local_path.c_str(), &encryption_key) != 0) {
            std::cerr << "File decryption failed" << std::endl;
            unlink(temp_encrypted.c_str());
            return false;
        }
        
        unlink(temp_encrypted.c_str());
        std::cout << "File downloaded and decrypted: " << local_path << std::endl;
        return true;
    }
    
    void interactive_mode() {
        std::string input;
        std::cout << "\n=== Antivirus Client Interactive Mode ===" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  upload <filepath>     - Upload file for scanning" << std::endl;
        std::cout << "  status <job_id>       - Check scan status" << std::endl;
        std::cout << "  result <job_id>       - Get scan result" << std::endl;
        std::cout << "  download <filename>   - Download file from server" << std::endl;
        std::cout << "  quit                  - Exit client" << std::endl;
        std::cout << std::endl;
        
        while (connected) {
            std::cout << "client> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            std::istringstream iss(input);
            std::string cmd;
            iss >> cmd;
            
            if (cmd == "quit" || cmd == "exit") {
                break;
            } else if (cmd == "upload") {
                std::string filepath;
                iss >> filepath;
                if (!filepath.empty()) {
                    if (upload_file(filepath)) {
                        // Start async monitoring (simplified - assume job ID 1)
                        monitor_scan_async("1");
                    }
                } else {
                    std::cout << "Usage: upload <filepath>" << std::endl;
                }
            } else if (cmd == "status") {
                std::string job_id;
                iss >> job_id;
                if (!job_id.empty()) {
                    std::string status = check_scan_status(job_id);
                    std::cout << "Status: " << status << std::endl;
                } else {
                    std::cout << "Usage: status <job_id>" << std::endl;
                }
            } else if (cmd == "result") {
                std::string job_id;
                iss >> job_id;
                if (!job_id.empty()) {
                    std::string result = get_scan_result(job_id);
                    std::cout << "Result: " << result << std::endl;
                } else {
                    std::cout << "Usage: result <job_id>" << std::endl;
                }
            } else if (cmd == "download") {
                std::string filename;
                iss >> filename;
                if (!filename.empty()) {
                    std::string local_path = "downloaded_" + filename;
                    download_file(filename, local_path);
                } else {
                    std::cout << "Usage: download <filename>" << std::endl;
                }
            } else {
                std::cout << "Unknown command: " << cmd << std::endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Antivirus Client (UNIX)" << std::endl;
    
    std::string server_host = "localhost";
    int server_port = SERVER_PORT;
    
    // Parse command line arguments
    if (argc >= 2) {
        server_host = argv[1];
    }
    if (argc >= 3) {
        server_port = std::atoi(argv[2]);
    }
    
    OrdinaryClient client(server_host, server_port);
    
    if (!client.connect_to_server()) {
        std::cerr << "Failed to connect to server at " << server_host << ":" << server_port << std::endl;
        return 1;
    }
    
    // Run interactive mode
    client.interactive_mode();
    
    std::cout << "Client disconnected." << std::endl;
    return 0;
} 