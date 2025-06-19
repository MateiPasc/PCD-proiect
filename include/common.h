#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <semaphore.h>
#include <fcntl.h>
#include <dirent.h>

// Constants
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define MAX_FILENAME 256
#define MAX_PATH 512
#define MAX_MESSAGE 1024
#define ADMIN_SOCKET_PATH "/tmp/antivirus_admin.sock"
#define SERVER_PORT 8080
#define ADMIN_TIMEOUT 300  // 5 minutes
#define MAX_JOBS 1000

// Protocol Commands
#define CMD_ADMIN_AUTH "ADMIN_AUTH"
#define CMD_SET_LOG_LEVEL "SET_LOG_LEVEL"
#define CMD_GET_LOGS "GET_LOGS"
#define CMD_GET_STATS "GET_STATS"
#define CMD_DISCONNECT_CLIENT "DISCONNECT_CLIENT"
#define CMD_SHUTDOWN_SERVER "SHUTDOWN_SERVER"

#define CMD_REGISTER_CLIENT "REGISTER_CLIENT"
#define CMD_UPLOAD_FILE "UPLOAD_FILE"
#define CMD_GET_SCAN_STATUS "GET_SCAN_STATUS"
#define CMD_GET_SCAN_RESULT "GET_SCAN_RESULT"
#define CMD_DOWNLOAD_FILE "DOWNLOAD_FILE"

// Response codes
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_INFECTED "INFECTED"
#define RESP_CLEAN "CLEAN"
#define RESP_PENDING "PENDING"
#define RESP_NOT_FOUND "NOT_FOUND"

// Log levels
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
} log_level_t;

// Scan status
typedef enum {
    SCAN_PENDING = 0,
    SCAN_PROCESSING = 1,
    SCAN_COMPLETED = 2,
    SCAN_ERROR = 3
} scan_status_t;

// Client info structure
typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    char ip_string[INET_ADDRSTRLEN];
    time_t connect_time;
    time_t last_activity;
    int is_active;
    pthread_t thread_id;
} client_info_t;

// Job structure for scan queue
typedef struct {
    int job_id;
    int client_fd;
    char filename[MAX_FILENAME];
    char filepath[MAX_PATH];
    char encrypted_key[256];
    size_t file_size;
    scan_status_t status;
    char result[MAX_MESSAGE];
    time_t created_time;
    time_t completed_time;
} scan_job_t;

// Server statistics
typedef struct {
    int total_connections;
    int active_connections;
    int total_scans;
    int clean_files;
    int infected_files;
    int errors;
    time_t server_start_time;
} server_stats_t;

// Global server state
typedef struct {
    int admin_socket_fd;
    int client_socket_fd;
    int admin_client_fd;
    client_info_t clients[MAX_CLIENTS];
    scan_job_t job_queue[MAX_JOBS];
    int job_count;
    int next_job_id;
    server_stats_t stats;
    log_level_t current_log_level;
    int server_running;
    
    // Synchronization
    pthread_mutex_t clients_mutex;
    pthread_mutex_t jobs_mutex;
    pthread_mutex_t stats_mutex;
    pthread_mutex_t log_mutex;
    pthread_cond_t job_available;
    sem_t job_semaphore;
    
    // Threads
    pthread_t admin_thread;
    pthread_t client_thread;
    pthread_t processor_thread;
    pthread_t monitor_thread;
} server_state_t;

// Encryption structures
typedef struct {
    unsigned char key[32];  // 256-bit key
    unsigned char iv[16];   // 128-bit IV
} crypto_key_t;

// Function prototypes
void log_message(log_level_t level, const char* format, ...);
void init_server_state(server_state_t* state);
void cleanup_server_state(server_state_t* state);
int create_admin_socket(void);
int create_client_socket(void);
void* admin_thread_handler(void* arg);
void* client_thread_handler(void* arg);
void* processor_thread_handler(void* arg);
void* monitor_thread_handler(void* arg);

// Encryption functions
int encrypt_file(const char* input_file, const char* output_file, const crypto_key_t* key);
int decrypt_file(const char* input_file, const char* output_file, const crypto_key_t* key);
void generate_key(crypto_key_t* key);
int send_encrypted_data(int socket_fd, const void* data, size_t size, const crypto_key_t* key);
int receive_encrypted_data(int socket_fd, void* data, size_t size, const crypto_key_t* key);

// Protocol functions
int parse_admin_command(const char* command, char* cmd, char* args);
int parse_client_command(const char* command, char* cmd, char* args);
int send_response(int socket_fd, const char* status, const char* message);
int receive_file(int socket_fd, const char* filepath, size_t expected_size);
int send_file(int socket_fd, const char* filepath);

// Scanner functions
int scan_file_with_clamav(const char* filepath, char* result, size_t result_size);
int is_file_infected(const char* filepath);

// Utility functions
const char* log_level_to_string(log_level_t level);
log_level_t string_to_log_level(const char* level_str);
const char* scan_status_to_string(scan_status_t status);
void get_current_timestamp(char* buffer, size_t buffer_size);
int create_directory_if_not_exists(const char* path);

#endif // COMMON_H 