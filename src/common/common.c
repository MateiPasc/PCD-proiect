#include "../../include/common.h"
#include <stdarg.h>

// Utility functions implementation

const char* log_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

log_level_t string_to_log_level(const char* level_str) {
    if (strcasecmp(level_str, "DEBUG") == 0) return LOG_DEBUG;
    if (strcasecmp(level_str, "INFO") == 0) return LOG_INFO;
    if (strcasecmp(level_str, "WARNING") == 0) return LOG_WARNING;
    if (strcasecmp(level_str, "ERROR") == 0) return LOG_ERROR;
    return -1; // Invalid level
}

const char* scan_status_to_string(scan_status_t status) {
    switch (status) {
        case SCAN_PENDING: return "PENDING";
        case SCAN_PROCESSING: return "PROCESSING";
        case SCAN_COMPLETED: return "COMPLETED";
        case SCAN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void get_current_timestamp(char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

int create_directory_if_not_exists(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            perror("mkdir");
            return -1;
        }
    }
    return 0;
}

// Protocol parsing functions
int parse_admin_command(const char* command, char* cmd, char* args) {
    char* space = strchr(command, ' ');
    if (space == NULL) {
        strcpy(cmd, command);
        args[0] = '\0';
    } else {
        int cmd_len = space - command;
        strncpy(cmd, command, cmd_len);
        cmd[cmd_len] = '\0';
        strcpy(args, space + 1);
    }
    return 0;
}

int parse_client_command(const char* command, char* cmd, char* args) {
    return parse_admin_command(command, cmd, args); // Same logic
}

int send_response(int socket_fd, const char* status, const char* message) {
    char response[MAX_MESSAGE];
    snprintf(response, sizeof(response), "%s %s\n", status, message);
    
    int bytes_sent = send(socket_fd, response, strlen(response), 0);
    if (bytes_sent == -1) {
        perror("send");
        return -1;
    }
    return bytes_sent;
}

int receive_file(int socket_fd, const char* filepath, size_t expected_size) {
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
    char buffer[BUFFER_SIZE];
    size_t total_received = 0;
    
    while (total_received < expected_size) {
        size_t remaining = expected_size - total_received;
        size_t to_receive = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
        
        int bytes_received = recv(socket_fd, buffer, to_receive, 0);
        if (bytes_received <= 0) {
            fclose(file);
            unlink(filepath); // Remove incomplete file
            return -1;
        }
        
        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
    }
    
    fclose(file);
    return 0;
}

int send_file(int socket_fd, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Send file size first
    char size_header[64];
    snprintf(size_header, sizeof(size_header), "SIZE %ld\n", file_size);
    if (send(socket_fd, size_header, strlen(size_header), 0) == -1) {
        fclose(file);
        return -1;
    }
    
    // Send file content
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(socket_fd, buffer, bytes_read, 0) == -1) {
            fclose(file);
            return -1;
        }
    }
    
    fclose(file);
    return 0;
}

// Scanner functions (ClamAV integration)
int scan_file_with_clamav(const char* filepath, char* result, size_t result_size) {
    char command[MAX_PATH + 50];
    snprintf(command, sizeof(command), "clamscan --no-summary %s", filepath);
    
    FILE* fp = popen(command, "r");
    if (!fp) {
        strncpy(result, "Error running scanner", result_size - 1);
        result[result_size - 1] = '\0';
        return -1;
    }
    
    char line[256];
    int infected = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "FOUND")) {
            infected = 1;
            strncpy(result, line, result_size - 1);
            result[result_size - 1] = '\0';
            break;
        }
    }
    
    pclose(fp);
    
    if (!infected) {
        strncpy(result, "OK", result_size - 1);
        result[result_size - 1] = '\0';
    }
    
    return infected;
}

int is_file_infected(const char* filepath) {
    char result[MAX_MESSAGE];
    return scan_file_with_clamav(filepath, result, sizeof(result));
} 