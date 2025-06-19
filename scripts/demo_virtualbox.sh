#!/bin/bash

# =============================================================================
# Antivirus Server - Demo VirtualBox Script
# =============================================================================

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_header() {
    echo -e "\n${BLUE}=== $1 ===${NC}\n"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${CYAN}ℹ️ $1${NC}"
}

# Function to wait for user input
wait_for_user() {
    echo -e "\n${YELLOW}Press Enter to continue...${NC}"
    read -r
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if port is available
check_port() {
    local port=$1
    if netstat -tuln 2>/dev/null | grep -q ":$port "; then
        return 1  # Port is in use
    else
        return 0  # Port is available
    fi
}

# Function to cleanup on exit
cleanup() {
    print_header "Cleanup în curs..."
    
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        print_info "Opresc serverul (PID: $SERVER_PID)..."
        kill "$SERVER_PID" 2>/dev/null || true
        sleep 2
        kill -9 "$SERVER_PID" 2>/dev/null || true
    fi
    
    # Remove test files
    rm -f test_clean.txt test_virus.txt 2>/dev/null || true
    
    # Remove socket file
    rm -f /tmp/antivirus_admin.sock 2>/dev/null || true
    
    print_success "Cleanup terminat!"
}

# Set trap for cleanup
trap cleanup EXIT INT TERM

# =============================================================================
# MAIN DEMO SCRIPT
# =============================================================================

clear
echo -e "${PURPLE}"
cat << "EOF"
╔══════════════════════════════════════════════════════════════════╗
║                    🖥️  VIRTUALBOX DEMO                           ║
║                 Antivirus Server Project - PCD                  ║
║                                                                  ║
║  Server de Scanare Antiviruși cu Criptare E2E                  ║
║  Concurrent și Distribuit Programming                           ║
╚══════════════════════════════════════════════════════════════════╝
EOF
echo -e "${NC}\n"

print_header "Verificări Preliminare"

# Check if we're in the right directory
if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
    print_error "Nu sunt în directorul proiectului!"
    print_info "Rulează: cd ~/antivirus-project"
    exit 1
fi

print_success "Directorul proiectului detectat"

# Check if binaries exist
if [ ! -f "bin/antivirus_server" ]; then
    print_warning "Executabilele nu există. Compilez..."
    make all
fi

print_success "Executabilele sunt gata"

# Check dependencies
print_info "Verificând dependințele..."

if ! command_exists clamscan; then
    print_error "ClamAV nu este instalat!"
    print_info "Rulează: sudo apt install -y clamav"
    exit 1
fi

if ! command_exists gcc; then
    print_error "GCC nu este instalat!"
    print_info "Rulează: sudo apt install -y build-essential"
    exit 1
fi

print_success "Toate dependințele sunt satisfăcute"

# Get VM IP
VM_IP=$(hostname -I | awk '{print $1}')
print_info "IP VM detectat: $VM_IP"

# Check if port 8080 is available
if ! check_port 8080; then
    print_error "Portul 8080 este ocupat!"
    print_info "Oprește alte servere sau schimbă portul"
    exit 1
fi

print_success "Portul 8080 este disponibil"

wait_for_user

# =============================================================================
# DEMO STEP 1: Create Test Files
# =============================================================================

print_header "Pas 1: Crearea Fișierelor de Test"

print_info "Creez fișier clean pentru testare..."
echo "This is a clean test file for antivirus scanning." > test_clean.txt
echo "Created at: $(date)" >> test_clean.txt
echo "VM IP: $VM_IP" >> test_clean.txt

print_info "Creez fișier EICAR (virus test standard)..."
echo 'X5O!P%@AP[4\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*' > test_virus.txt

print_success "Fișiere de test create:"
echo "  - test_clean.txt (clean)"
echo "  - test_virus.txt (EICAR test virus)"

# Test ClamAV on files
print_info "Test rapid ClamAV:"
clamscan test_clean.txt || true
clamscan test_virus.txt || true

wait_for_user

# =============================================================================
# DEMO STEP 2: Start Server
# =============================================================================

print_header "Pas 2: Pornirea Serverului"

print_info "Pornesc Antivirus Server în background..."

# Start server in background and capture PID
./bin/antivirus_server > logs/demo_server.log 2>&1 &
SERVER_PID=$!

print_success "Server pornit cu PID: $SERVER_PID"

# Wait for server to initialize
print_info "Aștept inițializarea serverului..."
sleep 3

# Check if server is running
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    print_error "Serverul nu a pornit corect!"
    print_info "Verifică logs/demo_server.log pentru detalii"
    exit 1
fi

print_success "Serverul rulează și acceptă conexiuni"

# Show server info
print_info "Informații server:"
echo "  - PID: $SERVER_PID"
echo "  - Admin socket: /tmp/antivirus_admin.sock"
echo "  - Client port: 8080"
echo "  - VM IP: $VM_IP"

wait_for_user

# =============================================================================
# DEMO STEP 3: Instructions for Testing
# =============================================================================

print_header "Pas 3: Instrucțiuni pentru Testare"

echo -e "${CYAN}Serverul rulează! Acum poți testa în mai multe moduri:${NC}\n"

echo -e "${YELLOW}📋 OPȚIUNEA 1 - În VM Linux (Terminal):${NC}"
echo "  1. Terminal nou: ./bin/admin_client"
echo "  2. Alt terminal: ./bin/ordinary_client"
echo "  3. Încă un terminal: ./bin/ordinary_client"

echo -e "\n${YELLOW}📋 OPȚIUNEA 2 - Client Python pe Windows Host:${NC}"
echo "  1. Pe Windows, în directorul: src/windows_client/"
echo "  2. Modifică în 'windows_client.py':"
echo "     self.server_host = \"$VM_IP\""
echo "  3. Rulează: python windows_client.py"

echo -e "\n${YELLOW}📋 OPȚIUNEA 3 - Test Automat (recomandat):${NC}"
echo "  1. Deschide alt terminal"
echo "  2. Rulează: make test-scenario"

echo -e "\n${YELLOW}🧪 COMENZI DE TEST în Client:${NC}"
echo "  upload test_clean.txt"
echo "  status 1"
echo "  result 1"
echo "  upload test_virus.txt"
echo "  status 2" 
echo "  result 2"
echo "  download 1 clean_result.txt"
echo "  quit"

echo -e "\n${YELLOW}👑 COMENZI ADMIN:${NC}"
echo "  get_stats"
echo "  get_logs"
echo "  set_log_level DEBUG"
echo "  disconnect_client <client_id>"
echo "  quit"

wait_for_user

# =============================================================================
# DEMO STEP 4: Show Live Logs
# =============================================================================

print_header "Pas 4: Monitorizarea Live"

echo -e "${CYAN}Monitorizez activitatea serverului...${NC}"
echo -e "${YELLOW}Deschide alte terminale pentru a testa!${NC}\n"

# Show live logs for a few seconds
print_info "Log-uri server (ultimele 10 linii):"
tail logs/demo_server.log || echo "Log-uri nu sunt încă disponibile"

echo -e "\n${YELLOW}Pentru monitorizare continuă, rulează în alt terminal:${NC}"
echo "  tail -f logs/demo_server.log"

echo -e "\n${YELLOW}Pentru statistici în timp real:${NC}"
echo "  watch -n 2 'ps aux | grep antivirus; netstat -an | grep 8080'"

wait_for_user

# =============================================================================
# DEMO STEP 5: Network Information
# =============================================================================

print_header "Pas 5: Informații Network pentru Cross-Platform Testing"

echo -e "${CYAN}Pentru testarea cross-platform:${NC}\n"

echo -e "${YELLOW}🌐 INFORMAȚII NETWORK:${NC}"
echo "  VM IP Address: $VM_IP"
echo "  Server Port: 8080"
echo "  Admin Socket: /tmp/antivirus_admin.sock"

echo -e "\n${YELLOW}🔥 FIREWALL (dacă ai probleme de conectare):${NC}"
echo "  sudo ufw allow 8080"
echo "  sudo ufw status"

echo -e "\n${YELLOW}🔍 VERIFICARE CONECTIVITATE din Windows:${NC}"
echo "  telnet $VM_IP 8080"
echo "  # sau"
echo "  Test-NetConnection -ComputerName $VM_IP -Port 8080"

echo -e "\n${YELLOW}📝 EDITEAZĂ windows_client.py:${NC}"
echo "  Schimbă linia:"
echo "  self.server_host = \"localhost\""
echo "  cu:"
echo "  self.server_host = \"$VM_IP\""

wait_for_user

# =============================================================================
# DEMO STEP 6: Performance Monitoring
# =============================================================================

print_header "Pas 6: Monitorizare Performanță"

echo -e "${CYAN}Informații despre performanța serverului:${NC}\n"

print_info "Utilizarea resurse server:"
if command_exists htop; then
    echo "  Pentru monitorizare interactivă: htop"
else
    echo "  Instalează htop: sudo apt install -y htop"
fi

# Show current resource usage
echo -e "\n${YELLOW}📊 RESURSE CURENTE:${NC}"
echo "Process info:"
ps aux | grep antivirus_server | grep -v grep || echo "  Server process not visible"

echo -e "\nMemory usage:"
free -h

echo -e "\nDisk usage:"
df -h . | tail -1

echo -e "\nNetwork connections:"
netstat -tuln | grep 8080 || echo "  No connections on port 8080"

wait_for_user

# =============================================================================
# DEMO STEP 7: Final Instructions
# =============================================================================

print_header "Pas 7: Demo Activ - Instrucțiuni Finale"

echo -e "${GREEN}🎉 DEMO SETUP COMPLET!${NC}\n"

echo -e "${CYAN}Serverul Antivirus rulează și este gata pentru testare!${NC}\n"

cat << EOF
${YELLOW}📋 CHECKLIST PENTRU DEMONSTRAȚIE:${NC}

✅ Server pornit și funcțional (PID: $SERVER_PID)
✅ Fișiere de test create (clean + EICAR virus)
✅ VM IP detectat: $VM_IP
✅ Port 8080 disponibil pentru clienți
✅ Socket admin disponibil: /tmp/antivirus_admin.sock

${YELLOW}🚀 URMĂTORII PAȘI:${NC}

1. TESTARE LOCALĂ:
   - Deschide alt terminal → ./bin/admin_client
   - Deschide alt terminal → ./bin/ordinary_client

2. TESTARE CROSS-PLATFORM:
   - Pe Windows: modifică IP în windows_client.py
   - Rulează clientul GUI Python

3. TESTARE AUTOMATĂ:
   - Rulează: make test-scenario

4. MONITORIZARE:
   - tail -f logs/demo_server.log
   - htop (pentru resurse)

${YELLOW}⚠️ IMPORTANT:${NC}
- Serverul va rula până apăsezi Enter în acest terminal
- Pentru oprire: apasă Enter sau Ctrl+C
- Log-urile sunt în: logs/demo_server.log

${GREEN}Succes cu demonstrația! 🎯${NC}
EOF

echo -e "\n${RED}Apasă Enter pentru a opri serverul și a încheia demo-ul...${NC}"
read -r

print_header "Demo Terminat"
print_success "Demonstrația s-a încheiat cu succes!"
print_info "Toate resursele au fost curățate."
print_info "Pentru a rula din nou: ./scripts/demo_virtualbox.sh"

echo -e "\n${PURPLE}Mulțumim pentru testarea proiectului Antivirus Server! 🚀${NC}\n" 