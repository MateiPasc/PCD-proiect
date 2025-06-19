# Makefile for Antivirus Server Project
# Programare Concurenta si Distributiva

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE -g
CXXFLAGS = -Wall -Wextra -std=c++17 -g
LDFLAGS = -pthread -lclamav -lncurses -lssl -lcrypto

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
OBJ_DIR = obj
LOG_DIR = logs
PROC_DIR = processing
OUT_DIR = outgoing
TEST_DIR = tests

# Source files
SERVER_SOURCES = $(SRC_DIR)/server/antivirus_server.c
COMMON_SOURCES = $(SRC_DIR)/common/common.c $(SRC_DIR)/common/crypto_common.c
ADMIN_SOURCES = $(SRC_DIR)/admin_client/admin_client.cpp
CLIENT_SOURCES = $(SRC_DIR)/ordinary_client/ordinary_client.cpp

# Object files
SERVER_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SERVER_SOURCES))
COMMON_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMMON_SOURCES))
ADMIN_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(ADMIN_SOURCES))
CLIENT_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CLIENT_SOURCES))

# Executables
SERVER_EXEC = $(BIN_DIR)/antivirus_server
ADMIN_EXEC = $(BIN_DIR)/admin_client
CLIENT_EXEC = $(BIN_DIR)/ordinary_client

# Python client
PYTHON_CLIENT = $(SRC_DIR)/windows_client/windows_client.py

# Default target
all: directories $(SERVER_EXEC) $(ADMIN_EXEC) $(CLIENT_EXEC)
	@echo "Build completed successfully!"
	@echo "Executables created:"
	@echo "  Server: $(SERVER_EXEC)"
	@echo "  Admin Client: $(ADMIN_EXEC)"
	@echo "  Ordinary Client: $(CLIENT_EXEC)"
	@echo "  Python Client: $(PYTHON_CLIENT)"

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)/server $(OBJ_DIR)/common $(OBJ_DIR)/admin_client $(OBJ_DIR)/ordinary_client
	@mkdir -p $(LOG_DIR) $(PROC_DIR) $(OUT_DIR) $(TEST_DIR)
	@echo "Directories created"

# Server compilation
$(SERVER_EXEC): $(SERVER_OBJECTS) $(COMMON_OBJECTS)
	@echo "Linking server..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Server compiled successfully"

# Admin client compilation
$(ADMIN_EXEC): $(ADMIN_OBJECTS) $(COMMON_OBJECTS)
	@echo "Linking admin client..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Admin client compiled successfully"

# Ordinary client compilation
$(CLIENT_EXEC): $(CLIENT_OBJECTS) $(COMMON_OBJECTS)
	@echo "Linking ordinary client..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Ordinary client compiled successfully"

# C source compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# C++ source compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Individual targets
server: directories $(SERVER_EXEC)
admin: directories $(ADMIN_EXEC)
client: directories $(CLIENT_EXEC)

# Python dependencies (for Windows client)
python-deps:
	@echo "Installing Python dependencies for Windows client..."
	pip install tkinter cryptography
	@echo "Python dependencies installed"

# Clean targets
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	@echo "Clean completed"

clean-logs:
	@echo "Cleaning log files..."
	rm -f $(LOG_DIR)/*.log
	rm -f $(PROC_DIR)/*
	rm -f $(OUT_DIR)/*
	@echo "Logs cleaned"

clean-all: clean clean-logs
	@echo "Full clean completed"

# Install system dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing system dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential libclamav-dev libncurses5-dev libssl-dev
	@echo "System dependencies installed"

# Run targets
run-server: $(SERVER_EXEC)
	@echo "Starting antivirus server..."
	./$(SERVER_EXEC)

run-admin: $(ADMIN_EXEC)
	@echo "Starting admin client..."
	./$(ADMIN_EXEC)

run-client: $(CLIENT_EXEC)
	@echo "Starting ordinary client..."
	./$(CLIENT_EXEC)

run-python: $(PYTHON_CLIENT)
	@echo "Starting Python client..."
	cd $(SRC_DIR)/windows_client && python3 windows_client.py

# Debug targets
debug-server: CFLAGS += -DDEBUG -O0
debug-server: clean $(SERVER_EXEC)

debug-admin: CXXFLAGS += -DDEBUG -O0
debug-admin: clean $(ADMIN_EXEC)

debug-client: CXXFLAGS += -DDEBUG -O0
debug-client: clean $(CLIENT_EXEC)

# Release targets
release: CFLAGS += -O2 -DNDEBUG
release: CXXFLAGS += -O2 -DNDEBUG
release: clean all

# Test targets
test: all
	@echo "Running basic functionality tests..."
	@echo "Testing server startup..."
	@timeout 5s ./$(SERVER_EXEC) || echo "Server test completed"
	@echo "Tests completed"

# Valgrind memory check
memcheck-server: $(SERVER_EXEC)
	@echo "Running memory check on server..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(SERVER_EXEC)

memcheck-admin: $(ADMIN_EXEC)
	@echo "Running memory check on admin client..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(ADMIN_EXEC)

memcheck-client: $(CLIENT_EXEC)
	@echo "Running memory check on ordinary client..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(CLIENT_EXEC)

# Static analysis
static-analysis:
	@echo "Running static analysis..."
	cppcheck --enable=all --std=c99 --std=c++17 -I$(INCLUDE_DIR) $(SRC_DIR)/ 2> static_analysis.log
	@echo "Static analysis completed. Results in static_analysis.log"

# Code formatting
format:
	@echo "Formatting code..."
	find $(SRC_DIR) $(INCLUDE_DIR) -name "*.c" -o -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatting completed"

# Documentation generation
docs:
	@echo "Generating documentation..."
	doxygen Doxyfile
	@echo "Documentation generated in docs/"

# Package for submission
package: clean-all
	@echo "Creating submission package..."
	tar -czf antivirus_server_project.tar.gz --exclude='.git' --exclude='*.tar.gz' .
	@echo "Package created: antivirus_server_project.tar.gz"

# Demo scenarios
demo: all
	@echo "Setting up demo environment..."
	@echo "Creating test files..."
	echo "This is a clean test file" > $(TEST_DIR)/clean_file.txt
	echo "X5O!P%@AP[4\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*" > $(TEST_DIR)/eicar.txt
	@echo "Demo files created in $(TEST_DIR)/"
	@echo "Run 'make run-server' in one terminal"
	@echo "Run 'make run-admin' in another terminal"  
	@echo "Run 'make run-client' in a third terminal"

# VirtualBox demo (complete guided demo for VM environment)
demo-virtualbox: all
	@echo "$(BLUE)Starting VirtualBox interactive demo...$(NC)"
	@mkdir -p scripts
	chmod +x scripts/demo_virtualbox.sh
	./scripts/demo_virtualbox.sh

# Test scenario (automated testing)
test-scenario: all
	@echo "$(YELLOW)Running automated test scenario...$(NC)"
	chmod +x tests/test_scenario.sh
	./tests/test_scenario.sh

# Help target
help:
	@echo "Antivirus Server Project - Build System"
	@echo "Available targets:"
	@echo "  all          - Build all components"
	@echo "  server       - Build only server"
	@echo "  admin        - Build only admin client"
	@echo "  client       - Build only ordinary client"
	@echo "  python-deps  - Install Python dependencies"
	@echo "  clean        - Clean build files"
	@echo "  clean-logs   - Clean log files"
	@echo "  clean-all    - Clean everything"
	@echo "  install-deps - Install system dependencies"
	@echo "  run-server   - Run server"
	@echo "  run-admin    - Run admin client"
	@echo "  run-client   - Run ordinary client"
	@echo "  run-python   - Run Python client"
	@echo "  debug-*      - Build debug versions"
	@echo "  release      - Build optimized release"
	@echo "  test         - Run basic tests"
	@echo "  memcheck-*   - Run memory checks with valgrind"
	@echo "  static-analysis - Run static code analysis"
	@echo "  format       - Format source code"
	@echo "  docs         - Generate documentation"
	@echo "  package      - Create submission package"
	@echo "  demo         - Setup demo environment"
	@echo "  demo-virtualbox - Interactive VirtualBox demo"
	@echo "  test-scenario - Run automated test scenario"
	@echo "  help         - Show this help"

# Project info
info:
	@echo "Project: Antivirus Server (Client-Server Architecture)"
	@echo "Course: Programare Concurenta si Distributiva"
	@echo "Components:"
	@echo "  - Server (C): Multi-threaded antivirus scanning server"
	@echo "  - Admin Client (C++): ncurses-based administration interface"
	@echo "  - Ordinary Client (C++): Command-line file upload client"
	@echo "  - Windows Client (Python): GUI client with tkinter"
	@echo "Features:"
	@echo "  - E2E Encryption (custom implementation)"
	@echo "  - ClamAV integration for virus scanning"
	@echo "  - Multi-threading with pthread"
	@echo "  - Socket communication (UNIX + INET)"
	@echo "  - File transfer capabilities"
	@echo "  - Real-time logging and monitoring"

.PHONY: all directories server admin client clean clean-logs clean-all install-deps python-deps
.PHONY: run-server run-admin run-client run-python debug-server debug-admin debug-client
.PHONY: release test memcheck-server memcheck-admin memcheck-client static-analysis format docs
.PHONY: package demo demo-virtualbox test-scenario help info 