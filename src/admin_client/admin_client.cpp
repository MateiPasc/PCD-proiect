#include "../../include/common.h"
#include <iostream>
#include <string>
#include <vector>
#include <ncurses.h>
#include <sys/socket.h>
#include <sys/un.h>

class AdminClient {
private:
    int socket_fd;
    bool connected;
    WINDOW* main_win;
    WINDOW* log_win;
    WINDOW* command_win;
    WINDOW* stats_win;
    
    std::vector<std::string> log_messages;
    std::string current_command;
    
public:
    AdminClient() : socket_fd(-1), connected(false), main_win(NULL), 
                   log_win(NULL), command_win(NULL), stats_win(NULL) {}
    
    ~AdminClient() {
        cleanup();
    }
    
    bool connect_to_server() {
        socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            perror("socket");
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, ADMIN_SOCKET_PATH);
        
        if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            perror("connect");
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        connected = true;
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
        return std::string(buffer);
    }
    
    void init_ui() {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(1);
        
        // Enable colors
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_WHITE, COLOR_BLUE);   // Header
            init_pair(2, COLOR_GREEN, COLOR_BLACK);  // Success
            init_pair(3, COLOR_RED, COLOR_BLACK);    // Error
            init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Warning
        }
        
        int height, width;
        getmaxyx(stdscr, height, width);
        
        // Create windows
        main_win = newwin(height, width, 0, 0);
        log_win = newwin(height - 10, width - 30, 2, 1);
        stats_win = newwin(height - 10, 28, 2, width - 29);
        command_win = newwin(6, width - 2, height - 8, 1);
        
        // Enable scrolling for log window
        scrollok(log_win, TRUE);
        
        refresh_ui();
    }
    
    void cleanup_ui() {
        if (main_win) delwin(main_win);
        if (log_win) delwin(log_win);
        if (command_win) delwin(command_win);
        if (stats_win) delwin(stats_win);
        endwin();
    }
    
    void refresh_ui() {
        int height, width;
        getmaxyx(stdscr, height, width);
        
        // Clear and draw main window border
        wclear(main_win);
        box(main_win, 0, 0);
        
        // Title
        if (has_colors()) wattron(main_win, COLOR_PAIR(1));
        mvwprintw(main_win, 0, (width - 30) / 2, " Antivirus Server Admin Client ");
        if (has_colors()) wattroff(main_win, COLOR_PAIR(1));
        
        // Connection status
        std::string status = connected ? "CONNECTED" : "DISCONNECTED";
        int color = connected ? COLOR_PAIR(2) : COLOR_PAIR(3);
        if (has_colors()) wattron(main_win, color);
        mvwprintw(main_win, 1, width - 20, "Status: %s", status.c_str());
        if (has_colors()) wattroff(main_win, color);
        
        wrefresh(main_win);
        
        // Draw log window
        wclear(log_win);
        box(log_win, 0, 0);
        mvwprintw(log_win, 0, 2, " Server Logs ");
        
        int log_height, log_width;
        getmaxyx(log_win, log_height, log_width);
        
        // Display recent log messages
        int start_idx = std::max(0, (int)log_messages.size() - (log_height - 2));
        for (int i = start_idx; i < log_messages.size(); i++) {
            mvwprintw(log_win, i - start_idx + 1, 1, "%s", 
                     log_messages[i].substr(0, log_width - 2).c_str());
        }
        
        wrefresh(log_win);
        
        // Draw stats window
        wclear(stats_win);
        box(stats_win, 0, 0);
        mvwprintw(stats_win, 0, 2, " Server Stats ");
        wrefresh(stats_win);
        
        // Draw command window
        wclear(command_win);
        box(command_win, 0, 0);
        mvwprintw(command_win, 0, 2, " Commands ");
        
        // Command help
        mvwprintw(command_win, 1, 2, "1: Set Log Level  2: Get Stats");
        mvwprintw(command_win, 2, 2, "3: Get Logs       4: Disconnect Client");
        mvwprintw(command_win, 3, 2, "5: Shutdown       q: Quit");
        mvwprintw(command_win, 4, 2, "Command: %s", current_command.c_str());
        
        wrefresh(command_win);
    }
    
    void add_log_message(const std::string& message) {
        log_messages.push_back(message);
        if (log_messages.size() > 1000) {
            log_messages.erase(log_messages.begin());
        }
    }
    
    void update_stats() {
        if (!connected) return;
        
        send_command("GET_STATS");
        std::string response = receive_response();
        
        if (!response.empty()) {
            wclear(stats_win);
            box(stats_win, 0, 0);
            mvwprintw(stats_win, 0, 2, " Server Stats ");
            
            // Parse and display stats
            if (response.find("OK ") == 0) {
                std::string stats_data = response.substr(3);
                mvwprintw(stats_win, 2, 2, "Stats:");
                mvwprintw(stats_win, 3, 2, "%s", stats_data.substr(0, 24).c_str());
                
                char timestamp[64];
                get_current_timestamp(timestamp, sizeof(timestamp));
                mvwprintw(stats_win, 5, 2, "Updated: %s", timestamp);
            } else {
                mvwprintw(stats_win, 2, 2, "Error getting stats");
            }
            
            wrefresh(stats_win);
        }
    }
    
    void handle_set_log_level() {
        wclear(command_win);
        box(command_win, 0, 0);
        mvwprintw(command_win, 0, 2, " Set Log Level ");
        mvwprintw(command_win, 1, 2, "1: DEBUG  2: INFO");
        mvwprintw(command_win, 2, 2, "3: WARNING  4: ERROR");
        mvwprintw(command_win, 3, 2, "Enter choice: ");
        wrefresh(command_win);
        
        int ch = wgetch(command_win);
        std::string level;
        
        switch (ch) {
            case '1': level = "DEBUG"; break;
            case '2': level = "INFO"; break;
            case '3': level = "WARNING"; break;
            case '4': level = "ERROR"; break;
            default:
                add_log_message("Invalid log level choice");
                return;
        }
        
        std::string command = "SET_LOG_LEVEL " + level;
        send_command(command);
        std::string response = receive_response();
        
        add_log_message("Set log level to " + level + ": " + response);
    }
    
    void handle_get_logs() {
        send_command("GET_LOGS");
        std::string response = receive_response();
        
        if (!response.empty()) {
            if (response.find("OK ") == 0) {
                std::string logs = response.substr(3);
                add_log_message("Server logs: " + logs);
            } else {
                add_log_message("Error getting logs: " + response);
            }
        }
    }
    
    void handle_disconnect_client() {
        wclear(command_win);
        box(command_win, 0, 0);
        mvwprintw(command_win, 0, 2, " Disconnect Client ");
        mvwprintw(command_win, 1, 2, "Enter client IP: ");
        wrefresh(command_win);
        
        echo();
        char ip[INET_ADDRSTRLEN];
        wgetnstr(command_win, ip, sizeof(ip) - 1);
        noecho();
        
        std::string command = "DISCONNECT_CLIENT " + std::string(ip);
        send_command(command);
        std::string response = receive_response();
        
        add_log_message("Disconnect client " + std::string(ip) + ": " + response);
    }
    
    void handle_shutdown() {
        wclear(command_win);
        box(command_win, 0, 0);
        mvwprintw(command_win, 0, 2, " Shutdown Server ");
        mvwprintw(command_win, 1, 2, "Are you sure? (y/N): ");
        wrefresh(command_win);
        
        int ch = wgetch(command_win);
        if (ch == 'y' || ch == 'Y') {
            send_command("SHUTDOWN_SERVER");
            std::string response = receive_response();
            add_log_message("Server shutdown: " + response);
            
            // Wait a bit then disconnect
            napms(1000);
            disconnect();
        }
    }
    
    void run() {
        init_ui();
        
        if (!connect_to_server()) {
            cleanup_ui();
            std::cerr << "Failed to connect to server" << std::endl;
            return;
        }
        
        add_log_message("Connected to antivirus server");
        
        // Main loop
        int ch;
        while ((ch = getch()) != 'q' && ch != 'Q') {
            switch (ch) {
                case '1':
                    handle_set_log_level();
                    break;
                case '2':
                    update_stats();
                    break;
                case '3':
                    handle_get_logs();
                    break;
                case '4':
                    handle_disconnect_client();
                    break;
                case '5':
                    handle_shutdown();
                    break;
                case KEY_RESIZE:
                    // Handle terminal resize
                    endwin();
                    refresh();
                    init_ui();
                    break;
                default:
                    break;
            }
            
            refresh_ui();
            
            // Check if still connected
            if (!connected) {
                add_log_message("Connection lost to server");
                break;
            }
        }
        
        cleanup();
    }
    
    void cleanup() {
        disconnect();
        cleanup_ui();
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Antivirus Server Admin Client" << std::endl;
    std::cout << "Connecting to server..." << std::endl;
    
    AdminClient client;
    client.run();
    
    return 0;
} 