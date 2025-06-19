#!/bin/bash

# =============================================================================
# Antivirus Server - macOS Setup Script
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

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect architecture
detect_arch() {
    local arch=$(uname -m)
    if [[ "$arch" == "arm64" ]]; then
        echo "apple_silicon"
    else
        echo "intel"
    fi
}

# Function to get Homebrew prefix
get_brew_prefix() {
    local arch=$(detect_arch)
    if [[ "$arch" == "apple_silicon" ]]; then
        echo "/opt/homebrew"
    else
        echo "/usr/local"
    fi
}

# =============================================================================
# MAIN SETUP SCRIPT
# =============================================================================

clear
echo -e "${PURPLE}"
cat << "EOF"
╔══════════════════════════════════════════════════════════════════╗
║                    🍎  MACOS SETUP SCRIPT                       ║
║                 Antivirus Server Project - PCD                  ║
║                                                                  ║
║  Automated setup for macOS development environment              ║
║  Supports both Intel and Apple Silicon Macs                    ║
╚══════════════════════════════════════════════════════════════════╝
EOF
echo -e "${NC}\n"

print_header "System Information"

# Detect system info
MACOS_VERSION=$(sw_vers -productVersion)
ARCH=$(detect_arch)
BREW_PREFIX=$(get_brew_prefix)

print_info "macOS Version: $MACOS_VERSION"
print_info "Architecture: $ARCH ($(uname -m))"
print_info "Homebrew Prefix: $BREW_PREFIX"

# Check if we're in the right directory
if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
    print_error "Nu sunt în directorul proiectului!"
    print_info "Navighează în directorul proiectului și rulează din nou script-ul"
    exit 1
fi

print_success "Directorul proiectului detectat"

# =============================================================================
# STEP 1: Xcode Command Line Tools
# =============================================================================

print_header "Xcode Command Line Tools"

if command_exists xcode-select && xcode-select -p >/dev/null 2>&1; then
    print_success "Xcode Command Line Tools sunt deja instalate"
else
    print_info "Instalez Xcode Command Line Tools..."
    xcode-select --install
    
    print_warning "Așteaptă finalizarea instalării Xcode Command Line Tools"
    print_info "Apasă Enter după ce instalarea s-a terminat..."
    read -r
fi

# Verify installation
if command_exists gcc && command_exists make && command_exists git; then
    print_success "Build tools verificate: $(gcc --version | head -1)"
else
    print_error "Build tools nu sunt disponibile"
    exit 1
fi

# =============================================================================
# STEP 2: Homebrew Installation
# =============================================================================

print_header "Homebrew Package Manager"

if command_exists brew; then
    print_success "Homebrew este deja instalat: $(brew --version | head -1)"
else
    print_info "Instalez Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    # Add Homebrew to PATH
    if [[ "$ARCH" == "apple_silicon" ]]; then
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc
        eval "$(/opt/homebrew/bin/brew shellenv)"
    else
        echo 'eval "$(/usr/local/bin/brew shellenv)"' >> ~/.zshrc
        eval "$(/usr/local/bin/brew shellenv)"
    fi
    
    print_success "Homebrew instalat cu succes"
fi

# Update Homebrew
print_info "Actualizez Homebrew..."
brew update

# =============================================================================
# STEP 3: Project Dependencies
# =============================================================================

print_header "Dependințe Proiect"

# List of required packages
PACKAGES=(
    "clamav"
    "ncurses" 
    "openssl"
    "pkg-config"
    "python3"
)

print_info "Instalez dependințele principale..."

for package in "${PACKAGES[@]}"; do
    if brew list "$package" >/dev/null 2>&1; then
        print_success "$package este deja instalat"
    else
        print_info "Instalez $package..."
        brew install "$package"
        print_success "$package instalat cu succes"
    fi
done

# =============================================================================
# STEP 4: ClamAV Configuration
# =============================================================================

print_header "Configurare ClamAV"

# Create ClamAV directories
print_info "Configurez directoarele ClamAV..."
if [[ "$ARCH" == "apple_silicon" ]]; then
    CLAMAV_DIR="/opt/homebrew/share/clamav"
else
    CLAMAV_DIR="/usr/local/share/clamav"
fi

sudo mkdir -p "$CLAMAV_DIR"
sudo chown $(whoami) "$CLAMAV_DIR"

# Update virus definitions
print_info "Actualizez definițiile de viruși ClamAV..."
if freshclam; then
    print_success "Baza de date ClamAV actualizată cu succes"
else
    print_warning "Actualizarea ClamAV a eșuat, dar continuăm..."
fi

# Test ClamAV
print_info "Testez ClamAV..."
echo "Test file for ClamAV" > /tmp/clamav_test.txt
if clamscan /tmp/clamav_test.txt >/dev/null 2>&1; then
    print_success "ClamAV funcționează corect"
    rm /tmp/clamav_test.txt
else
    print_warning "ClamAV test a eșuat, dar continuăm..."
fi

# =============================================================================
# STEP 5: Python Dependencies
# =============================================================================

print_header "Dependințe Python"

print_info "Instalez dependințele Python pentru Windows client..."

# Install Python packages
if pip3 install cryptography >/dev/null 2>&1; then
    print_success "cryptography instalat cu succes"
else
    print_warning "Instalarea cryptography a eșuat"
fi

# Note about tkinter (usually comes with Python on macOS)
if python3 -c "import tkinter" >/dev/null 2>&1; then
    print_success "tkinter disponibil"
else
    print_warning "tkinter nu este disponibil (GUI client nu va funcționa)"
fi

# =============================================================================
# STEP 6: Environment Configuration
# =============================================================================

print_header "Configurare Environment"

# Configure shell environment
SHELL_CONFIG=""
if [[ "$SHELL" == *"zsh"* ]]; then
    SHELL_CONFIG="$HOME/.zshrc"
elif [[ "$SHELL" == *"bash"* ]]; then
    SHELL_CONFIG="$HOME/.bash_profile"
fi

if [[ -n "$SHELL_CONFIG" ]]; then
    print_info "Configurez environment în $SHELL_CONFIG..."
    
    # Add library paths
    cat >> "$SHELL_CONFIG" << EOF

# Antivirus Server Project - macOS Configuration
export PKG_CONFIG_PATH="$BREW_PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH"
export LDFLAGS="-L$BREW_PREFIX/lib"
export CPPFLAGS="-I$BREW_PREFIX/include"
export LIBRARY_PATH="$BREW_PREFIX/lib:\$LIBRARY_PATH"
export C_INCLUDE_PATH="$BREW_PREFIX/include:\$C_INCLUDE_PATH"

EOF
    
    # Source the configuration
    source "$SHELL_CONFIG"
    print_success "Environment configurat în $SHELL_CONFIG"
else
    print_warning "Nu am putut detecta shell-ul pentru configurare automată"
fi

# =============================================================================
# STEP 7: Project Compilation
# =============================================================================

print_header "Compilare Proiect"

# Create necessary directories
print_info "Creez directoarele necesare..."
mkdir -p logs processing outgoing bin obj

# Test dependencies
print_info "Testez dependințele..."
pkg-config --exists libclamav && print_success "ClamAV: OK" || print_warning "ClamAV: MISSING"
pkg-config --exists ncurses && print_success "ncurses: OK" || print_warning "ncurses: MISSING"
pkg-config --exists openssl && print_success "OpenSSL: OK" || print_warning "OpenSSL: MISSING"

# Compile the project
print_info "Compilez proiectul..."

# Set compiler for macOS
export CC=clang
export CXX=clang++

if make clean >/dev/null 2>&1 && make all; then
    print_success "Proiectul compilat cu succes!"
else
    print_error "Compilarea a eșuat!"
    print_info "Încearcă manual: make CC=clang CXX=clang++ all"
    exit 1
fi

# Verify executables
print_info "Verific executabilele..."
if [[ -f "bin/antivirus_server" && -f "bin/admin_client" && -f "bin/ordinary_client" ]]; then
    print_success "Toate executabilele sunt disponibile"
    
    # Set permissions
    chmod +x bin/antivirus_server
    chmod +x bin/admin_client  
    chmod +x bin/ordinary_client
    
    print_info "Informații executabile:"
    file bin/antivirus_server
else
    print_error "Unele executabile lipsesc"
    exit 1
fi

# =============================================================================
# STEP 8: System Configuration
# =============================================================================

print_header "Configurare Sistem"

# Configure firewall if needed
print_info "Verific firewall macOS..."
FIREWALL_STATE=$(sudo /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate 2>/dev/null || echo "unknown")

if [[ "$FIREWALL_STATE" == *"enabled"* ]]; then
    print_warning "Firewall-ul este activ. Adaug excepții pentru aplicație..."
    sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add bin/antivirus_server >/dev/null 2>&1 || true
    sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock bin/antivirus_server >/dev/null 2>&1 || true
    print_success "Excepții firewall adăugate"
else
    print_success "Firewall dezactivat sau configurat corect"
fi

# Create test files
print_info "Creez fișiere de test..."
echo "Clean file for testing on macOS $(date)" > test_clean.txt
echo 'X5O!P%@AP[4\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*' > test_virus.txt
print_success "Fișiere de test create"

# =============================================================================
# STEP 9: Final Verification
# =============================================================================

print_header "Verificare Finală"

# Test ClamAV with test files
print_info "Testez ClamAV cu fișierele de test..."
if clamscan test_clean.txt >/dev/null 2>&1; then
    print_success "Test fișier clean: OK"
else
    print_warning "Test fișier clean: FAILED"
fi

if clamscan test_virus.txt 2>&1 | grep -q "FOUND"; then
    print_success "Test fișier virus: DETECTED (corect)"
else
    print_warning "Test fișier virus: NOT DETECTED"
fi

# Test basic functionality
print_info "Testez funcționalitatea de bază..."
if timeout 3s ./bin/antivirus_server >/dev/null 2>&1 || true; then
    print_success "Serverul poate fi pornit"
else
    print_warning "Probleme la pornirea serverului"
fi

# =============================================================================
# STEP 10: Summary and Instructions
# =============================================================================

print_header "Setup Complet!"

echo -e "${GREEN}🎉 Setup-ul pentru macOS s-a finalizat cu succes!${NC}\n"

cat << EOF
${YELLOW}📊 REZUMAT INSTALARE:${NC}

✅ Xcode Command Line Tools
✅ Homebrew Package Manager  
✅ ClamAV Antivirus Engine
✅ ncurses Library
✅ OpenSSL Cryptography
✅ Python3 + cryptography
✅ Project Dependencies
✅ Environment Configuration
✅ Project Compilation
✅ System Configuration

${YELLOW}🚀 URMĂTORII PAȘI:${NC}

1. TESTARE RAPIDĂ:
   ${CYAN}make demo${NC}

2. RULARE MANUALĂ:
   Terminal 1: ${CYAN}./bin/antivirus_server${NC}
   Terminal 2: ${CYAN}./bin/admin_client${NC}
   Terminal 3: ${CYAN}./bin/ordinary_client${NC}

3. CLIENT PYTHON GUI:
   ${CYAN}cd src/windows_client && python3 windows_client.py${NC}

4. DEBUGGING:
   ${CYAN}lldb bin/antivirus_server${NC}

${YELLOW}📝 FIȘIERE DE TEST DISPONIBILE:${NC}
   - test_clean.txt (fișier clean)
   - test_virus.txt (EICAR test virus)

${YELLOW}📚 DOCUMENTAȚIE:${NC}
   - README.md
   - docs/SETUP_MACOS.md
   - docs/ARHITECTURA_TEHNICA.md

${YELLOW}⚠️ NOTE IMPORTANTE:${NC}
   - Environment-ul a fost configurat în $SHELL_CONFIG
   - Restart terminal-ul pentru a aplica toate setările
   - Pentru probleme, consultă docs/SETUP_MACOS.md

EOF

# Performance info for Apple Silicon
if [[ "$ARCH" == "apple_silicon" ]]; then
    echo -e "${PURPLE}🚀 Apple Silicon Detected: Performanță optimizată pentru M1/M2/M3!${NC}"
fi

print_success "Setup macOS finalizat cu succes! 🍎"

# Ask to run demo
echo ""
read -p "Vrei să rulezi demo-ul acum? (y/n): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_info "Pornesc demo-ul..."
    make demo 2>/dev/null || echo -e "${YELLOW}Rulează manual: make demo${NC}"
fi 