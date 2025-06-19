#include "../../include/common.h"
#include <stdarg.h>
#include <sys/wait.h>

// Global server state
server_state_t g_server_state;

// Signal handling
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_message(LOG_INFO, "Received shutdown signal, stopping server gracefully...");
        g_server_state.server_running = 0;
    }
}

// Initialize server state
void init_server_state(server_state_t* state) {
    memset(state, 0, sizeof(server_state_t));
    
    state->admin_socket_fd = -1;
    state->client_socket_fd = -1;
    state->admin_client_fd = -1;
    state->current_log_level = LOG_INFO;
    state->server_running = 1;
    state->next_job_id = 1;
    
    // Initialize mutexes and condition variables
    pthread_mutex_init(&state->clients_mutex, NULL);
    pthread_mutex_init(&state->jobs_mutex, NULL);
    pthread_mutex_init(&state->stats_mutex, NULL);
    pthread_mutex_init(&state->log_mutex, NULL);
    pthread_cond_init(&state->job_available, NULL);
    sem_init(&state->job_semaphore, 0, 0);
    
    // Initialize stats
    state->stats.server_start_time = time(NULL);
    
    // Initialize client array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        state->clients[i].socket_fd = -1;
        state->clients[i].is_active = 0;
    }
    
    log_message(LOG_INFO, "Server state initialized");
}

// Cleanup server state
void cleanup_server_state(server_state_t* state) {
    state->server_running = 0;
    
    // Close sockets
    if (state->admin_socket_fd != -1) {
        close(state->admin_socket_fd);
        unlink(ADMIN_SOCKET_PATH);
    }
    if (state->client_socket_fd != -1) {
        close(state->client_socket_fd);
    }
    if (state->admin_client_fd != -1) {
        close(state->admin_client_fd);
    }
    
    // Close client connections
    pthread_mutex_lock(&state->clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].is_active && state->clients[i].socket_fd != -1) {
            close(state->clients[i].socket_fd);
            state->clients[i].is_active = 0;
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
    
    // Wait for threads to finish
    if (state->admin_thread) pthread_join(state->admin_thread, NULL);
    if (state->client_thread) pthread_join(state->client_thread, NULL);
    if (state->processor_thread) pthread_join(state->processor_thread, NULL);
    if (state->monitor_thread) pthread_join(state->monitor_thread, NULL);
    
    // Cleanup synchronization objects
    pthread_mutex_destroy(&state->clients_mutex);
    pthread_mutex_destroy(&state->jobs_mutex);
    pthread_mutex_destroy(&state->stats_mutex);
    pthread_mutex_destroy(&state->log_mutex);
    pthread_cond_destroy(&state->job_available);
    sem_destroy(&state->job_semaphore);
    
    log_message(LOG_INFO, "Server state cleaned up");
}

// Logging function
void log_message(log_level_t level, const char* format, ...) {
    if (level < g_server_state.current_log_level) {
        return;
    }
    
    pthread_mutex_lock(&g_server_state.log_mutex);
    
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    const char* level_str = log_level_to_string(level);
    
    // Print to console
    printf("[%s] [%s] ", timestamp, level_str);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    // Also write to log file
    FILE* log_file = fopen("logs/server.log", "a");
    if (log_file) {
        fprintf(log_file, "[%s] [%s] ", timestamp, level_str);
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fprintf(log_file, "\n");
        fclose(log_file);
    }
    
    pthread_mutex_unlock(&g_server_state.log_mutex);
}

// Create admin socket (UNIX domain socket)
int create_admin_socket(void) {
    int sock_fd;
    struct sockaddr_un addr;
    
    // Remove existing socket file
    unlink(ADMIN_SOCKET_PATH);
    
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        log_message(LOG_ERROR, "Failed to create admin socket: %s", strerror(errno));
        return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, ADMIN_SOCKET_PATH);
    
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        log_message(LOG_ERROR, "Failed to bind admin socket: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }
    
    if (listen(sock_fd, 1) == -1) {
        log_message(LOG_ERROR, "Failed to listen on admin socket: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }
    
    log_message(LOG_INFO, "Admin socket created at %s", ADMIN_SOCKET_PATH);
    return sock_fd;
}

// Create client socket (INET socket)
int create_client_socket(void) {
    int sock_fd;
    struct sockaddr_in addr;
    int opt = 1;
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        log_message(LOG_ERROR, "Failed to create client socket: %s", strerror(errno));
        return -1;
    }
    
    // Set socket options
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        log_message(LOG_WARNING, "Failed to set socket options: %s", strerror(errno));
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);
    
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        log_message(LOG_ERROR, "Failed to bind client socket: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }
    
    if (listen(sock_fd, MAX_CLIENTS) == -1) {
        log_message(LOG_ERROR, "Failed to listen on client socket: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }
    
    log_message(LOG_INFO, "Client socket created on port %d", SERVER_PORT);
    return sock_fd;
}

// Admin thread handler
void* admin_thread_handler(void* arg) {
    server_state_t* state = (server_state_t*)arg;
    struct sockaddr_un client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int client_fd;
    
    log_message(LOG_INFO, "Admin thread started");
    
    while (state->server_running) {
        // Accept admin connection (blocking with timeout)
        struct pollfd pfd;
        pfd.fd = state->admin_socket_fd;
        pfd.events = POLLIN;
        
        int poll_result = poll(&pfd, 1, 1000); // 1 second timeout
        if (poll_result == -1) {
            if (errno == EINTR) continue;
            log_message(LOG_ERROR, "Admin poll error: %s", strerror(errno));
            break;
        }
        
        if (poll_result == 0) continue; // Timeout, check server_running
        
        client_fd = accept(state->admin_socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd == -1) {
            log_message(LOG_ERROR, "Failed to accept admin connection: %s", strerror(errno));
            continue;
        }
        
        // Check if admin is already connected
        if (state->admin_client_fd != -1) {
            send_response(client_fd, RESP_ERROR, "Admin already connected");
            close(client_fd);
            continue;
        }
        
        state->admin_client_fd = client_fd;
        log_message(LOG_INFO, "Admin client connected");
        
        // Handle admin commands
        time_t last_activity = time(NULL);
        while (state->server_running && state->admin_client_fd != -1) {
            pfd.fd = client_fd;
            pfd.events = POLLIN;
            
            poll_result = poll(&pfd, 1, 1000);
            if (poll_result == -1) {
                if (errno == EINTR) continue;
                log_message(LOG_ERROR, "Admin client poll error: %s", strerror(errno));
                break;
            }
            
            if (poll_result == 0) {
                // Check timeout
                if (time(NULL) - last_activity > ADMIN_TIMEOUT) {
                    log_message(LOG_INFO, "Admin client timeout, disconnecting");
                    break;
                }
                continue;
            }
            
            int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                log_message(LOG_INFO, "Admin client disconnected");
                break;
            }
            
            buffer[bytes_received] = '\0';
            last_activity = time(NULL);
            
            // Process admin command (simplified)
            char cmd[256], args[256];
            if (parse_admin_command(buffer, cmd, args) == 0) {
                if (strcmp(cmd, CMD_SET_LOG_LEVEL) == 0) {
                    log_level_t new_level = string_to_log_level(args);
                    if (new_level != -1) {
                        state->current_log_level = new_level;
                        send_response(client_fd, RESP_OK, "Log level updated");
                        log_message(LOG_INFO, "Log level changed to %s", args);
                    } else {
                        send_response(client_fd, RESP_ERROR, "Invalid log level");
                    }
                } else if (strcmp(cmd, CMD_GET_STATS) == 0) {
                    char stats_msg[MAX_MESSAGE];
                    pthread_mutex_lock(&state->stats_mutex);
                    snprintf(stats_msg, sizeof(stats_msg), 
                            "Connections: %d, Active: %d, Scans: %d, Clean: %d, Infected: %d",
                            state->stats.total_connections, state->stats.active_connections,
                            state->stats.total_scans, state->stats.clean_files,
                            state->stats.infected_files);
                    pthread_mutex_unlock(&state->stats_mutex);
                    send_response(client_fd, RESP_OK, stats_msg);
                } else if (strcmp(cmd, CMD_SHUTDOWN_SERVER) == 0) {
                    send_response(client_fd, RESP_OK, "Server shutting down");
                    log_message(LOG_INFO, "Shutdown requested by admin");
                    state->server_running = 0;
                    break;
                } else {
                    send_response(client_fd, RESP_ERROR, "Unknown command");
                }
            } else {
                send_response(client_fd, RESP_ERROR, "Invalid command format");
            }
        }
        
        close(client_fd);
        state->admin_client_fd = -1;
        log_message(LOG_INFO, "Admin client disconnected");
    }
    
    log_message(LOG_INFO, "Admin thread terminated");
    return NULL;
}

// Client thread handler
void* client_thread_handler(void* arg) {
    server_state_t* state = (server_state_t*)arg;
    struct pollfd pfds[MAX_CLIENTS + 1];
    int nfds = 1;
    
    log_message(LOG_INFO, "Client thread started");
    
    // Initialize poll structure
    pfds[0].fd = state->client_socket_fd;
    pfds[0].events = POLLIN;
    
    while (state->server_running) {
        // Set up poll for all active clients
        nfds = 1;
        pthread_mutex_lock(&state->clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (state->clients[i].is_active) {
                pfds[nfds].fd = state->clients[i].socket_fd;
                pfds[nfds].events = POLLIN;
                nfds++;
            }
        }
        pthread_mutex_unlock(&state->clients_mutex);
        
        int poll_result = poll(pfds, nfds, 1000);
        if (poll_result == -1) {
            if (errno == EINTR) continue;
            log_message(LOG_ERROR, "Client poll error: %s", strerror(errno));
            break;
        }
        
        if (poll_result == 0) continue;
        
        // Check for new connections
        if (pfds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(state->client_socket_fd, (struct sockaddr*)&client_addr, &addr_len);
            
            if (client_fd != -1) {
                // Find free slot for client
                pthread_mutex_lock(&state->clients_mutex);
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!state->clients[i].is_active) {
                        slot = i;
                        break;
                    }
                }
                
                if (slot != -1) {
                    state->clients[slot].socket_fd = client_fd;
                    state->clients[slot].address = client_addr;
                    inet_ntop(AF_INET, &client_addr.sin_addr, state->clients[slot].ip_string, INET_ADDRSTRLEN);
                    state->clients[slot].connect_time = time(NULL);
                    state->clients[slot].last_activity = time(NULL);
                    state->clients[slot].is_active = 1;
                    
                    pthread_mutex_lock(&state->stats_mutex);
                    state->stats.total_connections++;
                    state->stats.active_connections++;
                    pthread_mutex_unlock(&state->stats_mutex);
                    
                    log_message(LOG_INFO, "Client connected from %s (slot %d)", 
                               state->clients[slot].ip_string, slot);
                } else {
                    log_message(LOG_WARNING, "Maximum clients reached, rejecting connection");
                    close(client_fd);
                }
                pthread_mutex_unlock(&state->clients_mutex);
            }
        }
        
        // Handle client data
        for (int i = 1; i < nfds; i++) {
            if (pfds[i].revents & POLLIN) {
                // Find client by socket fd
                pthread_mutex_lock(&state->clients_mutex);
                int client_slot = -1;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (state->clients[j].is_active && state->clients[j].socket_fd == pfds[i].fd) {
                        client_slot = j;
                        break;
                    }
                }
                pthread_mutex_unlock(&state->clients_mutex);
                
                if (client_slot != -1) {
                    // Handle client request (simplified)
                    char buffer[BUFFER_SIZE];
                    int bytes_received = recv(pfds[i].fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_received <= 0) {
                        // Client disconnected
                        pthread_mutex_lock(&state->clients_mutex);
                        close(state->clients[client_slot].socket_fd);
                        state->clients[client_slot].is_active = 0;
                        pthread_mutex_lock(&state->stats_mutex);
                        state->stats.active_connections--;
                        pthread_mutex_unlock(&state->stats_mutex);
                        log_message(LOG_INFO, "Client disconnected from slot %d", client_slot);
                        pthread_mutex_unlock(&state->clients_mutex);
                    } else {
                        buffer[bytes_received] = '\0';
                        state->clients[client_slot].last_activity = time(NULL);
                        
                        // Process client command (basic implementation)
                        if (strncmp(buffer, CMD_REGISTER_CLIENT, strlen(CMD_REGISTER_CLIENT)) == 0) {
                            send_response(pfds[i].fd, RESP_OK, "Client registered");
                        } else {
                            send_response(pfds[i].fd, RESP_ERROR, "Command not implemented");
                        }
                    }
                }
            }
        }
    }
    
    log_message(LOG_INFO, "Client thread terminated");
    return NULL;
}

// Processor thread handler
void* processor_thread_handler(void* arg) {
    server_state_t* state = (server_state_t*)arg;
    
    log_message(LOG_INFO, "Processor thread started");
    
    while (state->server_running) {
        // Wait for jobs to be available
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;
        
        if (sem_timedwait(&state->job_semaphore, &timeout) == -1) {
            if (errno == ETIMEDOUT) continue;
            if (errno == EINTR) continue;
            log_message(LOG_ERROR, "Semaphore wait error: %s", strerror(errno));
            break;
        }
        
        // Process job from queue
        pthread_mutex_lock(&state->jobs_mutex);
        scan_job_t* job = NULL;
        for (int i = 0; i < state->job_count; i++) {
            if (state->job_queue[i].status == SCAN_PENDING) {
                job = &state->job_queue[i];
                job->status = SCAN_PROCESSING;
                break;
            }
        }
        pthread_mutex_unlock(&state->jobs_mutex);
        
        if (job) {
            log_message(LOG_INFO, "Processing scan job %d: %s", job->job_id, job->filename);
            
            // Simulate scanning (replace with actual ClamAV integration)
            char scan_result[MAX_MESSAGE];
            int is_infected = scan_file_with_clamav(job->filepath, scan_result, sizeof(scan_result));
            
            pthread_mutex_lock(&state->jobs_mutex);
            job->status = SCAN_COMPLETED;
            job->completed_time = time(NULL);
            
            if (is_infected) {
                strcpy(job->result, "INFECTED");
                pthread_mutex_lock(&state->stats_mutex);
                state->stats.infected_files++;
                pthread_mutex_unlock(&state->stats_mutex);
            } else {
                strcpy(job->result, "CLEAN");
                pthread_mutex_lock(&state->stats_mutex);
                state->stats.clean_files++;
                pthread_mutex_unlock(&state->stats_mutex);
            }
            
            pthread_mutex_lock(&state->stats_mutex);
            state->stats.total_scans++;
            pthread_mutex_unlock(&state->stats_mutex);
            
            pthread_mutex_unlock(&state->jobs_mutex);
            
            log_message(LOG_INFO, "Scan job %d completed: %s", job->job_id, job->result);
        }
    }
    
    log_message(LOG_INFO, "Processor thread terminated");
    return NULL;
}

// Monitor thread handler (inotify)
void* monitor_thread_handler(void* arg) {
    server_state_t* state = (server_state_t*)arg;
    
    log_message(LOG_INFO, "Monitor thread started");
    
    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        log_message(LOG_ERROR, "Failed to initialize inotify: %s", strerror(errno));
        return NULL;
    }
    
    int wd = inotify_add_watch(inotify_fd, "processing", IN_CREATE | IN_MOVED_TO);
    if (wd == -1) {
        log_message(LOG_ERROR, "Failed to add inotify watch: %s", strerror(errno));
        close(inotify_fd);
        return NULL;
    }
    
    char buffer[4096];
    while (state->server_running) {
        struct pollfd pfd;
        pfd.fd = inotify_fd;
        pfd.events = POLLIN;
        
        int poll_result = poll(&pfd, 1, 1000);
        if (poll_result == -1) {
            if (errno == EINTR) continue;
            log_message(LOG_ERROR, "Monitor poll error: %s", strerror(errno));
            break;
        }
        
        if (poll_result == 0) continue;
        
        int length = read(inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            struct inotify_event* event = (struct inotify_event*)buffer;
            log_message(LOG_DEBUG, "File created in processing: %s", event->name);
        }
    }
    
    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    
    log_message(LOG_INFO, "Monitor thread terminated");
    return NULL;
}

// Main function
int main(int argc, char* argv[]) {
    printf("Antivirus Server Starting...\n");
    
    // Install signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    // Create directories
    create_directory_if_not_exists("logs");
    create_directory_if_not_exists("processing");
    create_directory_if_not_exists("outgoing");
    
    // Initialize server state
    init_server_state(&g_server_state);
    
    // Create sockets
    g_server_state.admin_socket_fd = create_admin_socket();
    if (g_server_state.admin_socket_fd == -1) {
        cleanup_server_state(&g_server_state);
        return 1;
    }
    
    g_server_state.client_socket_fd = create_client_socket();
    if (g_server_state.client_socket_fd == -1) {
        cleanup_server_state(&g_server_state);
        return 1;
    }
    
    // Start threads
    if (pthread_create(&g_server_state.admin_thread, NULL, admin_thread_handler, &g_server_state) != 0) {
        log_message(LOG_ERROR, "Failed to create admin thread");
        cleanup_server_state(&g_server_state);
        return 1;
    }
    
    if (pthread_create(&g_server_state.client_thread, NULL, client_thread_handler, &g_server_state) != 0) {
        log_message(LOG_ERROR, "Failed to create client thread");
        cleanup_server_state(&g_server_state);
        return 1;
    }
    
    if (pthread_create(&g_server_state.processor_thread, NULL, processor_thread_handler, &g_server_state) != 0) {
        log_message(LOG_ERROR, "Failed to create processor thread");
        cleanup_server_state(&g_server_state);
        return 1;
    }
    
    if (pthread_create(&g_server_state.monitor_thread, NULL, monitor_thread_handler, &g_server_state) != 0) {
        log_message(LOG_ERROR, "Failed to create monitor thread");
        cleanup_server_state(&g_server_state);
        return 1;
    }
    
    log_message(LOG_INFO, "Antivirus server started successfully");
    
    // Main loop
    while (g_server_state.server_running) {
        sleep(1);
    }
    
    log_message(LOG_INFO, "Shutting down server...");
    cleanup_server_state(&g_server_state);
    
    printf("Antivirus Server Stopped.\n");
    return 0;
} 