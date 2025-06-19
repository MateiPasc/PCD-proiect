# =============================================================================
# GitHub Setup Script pentru Proiectul Antivirus Server
# =============================================================================

param(
    [Parameter(Mandatory=$true)]
    [string]$GitHubUsername,
    
    [Parameter(Mandatory=$false)]
    [string]$UserName = "",
    
    [Parameter(Mandatory=$false)]
    [string]$UserEmail = ""
)

# Colors for output
$Red = "Red"
$Green = "Green"
$Yellow = "Yellow"
$Blue = "Blue"
$Cyan = "Cyan"

function Write-Header {
    param([string]$Message)
    Write-Host "`n=== $Message ===" -ForegroundColor $Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "✅ $Message" -ForegroundColor $Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠️ $Message" -ForegroundColor $Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "❌ $Message" -ForegroundColor $Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ️ $Message" -ForegroundColor $Cyan
}

# =============================================================================
# Main Setup Script
# =============================================================================

Clear-Host
Write-Host @"
╔══════════════════════════════════════════════════════════════════╗
║                    🚀 GITHUB SETUP SCRIPT                       ║
║                 Antivirus Server Project - PCD                  ║
╚══════════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Magenta

Write-Header "Verificări Preliminare"

# Check if git is installed
try {
    $gitVersion = git --version
    Write-Success "Git detectat: $gitVersion"
} catch {
    Write-Error "Git nu este instalat!"
    Write-Info "Descarcă Git de la: https://git-scm.com/downloads"
    exit 1
}

# Check if we're in the right directory
if (-not (Test-Path "Makefile") -or -not (Test-Path "src")) {
    Write-Error "Nu ești în directorul proiectului!"
    Write-Info "Navighează în directorul: C:\Users\pascb\OneDrive\Desktop\PCD"
    exit 1
}

Write-Success "Directorul proiectului detectat"

# =============================================================================
# Git Configuration
# =============================================================================

Write-Header "Configurare Git"

if ($UserName -eq "") {
    $UserName = Read-Host "Introdu numele tău complet"
}

if ($UserEmail -eq "") {
    $UserEmail = Read-Host "Introdu email-ul tău"
}

try {
    git config --global user.name "$UserName"
    git config --global user.email "$UserEmail"
    Write-Success "Git configurat pentru: $UserName <$UserEmail>"
} catch {
    Write-Warning "Configurarea Git a eșuat, dar continuăm..."
}

# =============================================================================
# Repository Initialization
# =============================================================================

Write-Header "Inițializare Repository Local"

try {
    # Initialize git repository
    git init
    Write-Success "Repository Git inițializat"
    
    # Add all files
    git add .
    Write-Success "Fișiere adăugate pentru commit"
    
    # Check status
    Write-Info "Status Git:"
    git status --short
    
} catch {
    Write-Error "Inițializarea repository-ului a eșuat: $_"
    exit 1
}

# =============================================================================
# First Commit
# =============================================================================

Write-Header "Primul Commit"

$commitMessage = @"
🎉 Initial commit: Complete Antivirus Server Project

✨ Features implemented:
- Multi-threaded C server with ClamAV integration
- Admin client with ncurses interface
- Ordinary client with file upload/download
- Windows GUI client in Python
- E2E encryption with custom implementation
- Comprehensive build system and testing

🏗️ Architecture:
- Server: 4 threads (admin, client, processor, monitor)
- Communication: UNIX + INET sockets
- Encryption: Diffie-Hellman + XOR cipher
- Scanning: ClamAV integration
- Cross-platform: Linux server + Windows client

📚 Documentation:
- Technical architecture (ARHITECTURA_TEHNICA.md)
- Project presentation (PREZENTARE_PROIECT.md)
- VirtualBox setup guide
- Complete README with usage examples

🧪 Testing:
- Automated test scenarios
- Memory leak checking
- Cross-platform compatibility
- E2E encryption verification

Course: Programare Concurentă și Distributivă
"@

try {
    git commit -m "$commitMessage"
    Write-Success "Primul commit creat cu succes"
} catch {
    Write-Error "Commit-ul a eșuat: $_"
    exit 1
}

# =============================================================================
# GitHub Remote Setup
# =============================================================================

Write-Header "Configurare GitHub Remote"

$repoName = "antivirus-server-pcd"
$remoteUrl = "https://github.com/$GitHubUsername/$repoName.git"

Write-Info "Repository URL: $remoteUrl"
Write-Warning "Asigură-te că ai creat repository-ul pe GitHub cu numele: $repoName"

$continue = Read-Host "Ai creat repository-ul pe GitHub? (y/n)"
if ($continue -ne "y" -and $continue -ne "Y") {
    Write-Info "Creează repository-ul pe GitHub și rulează din nou script-ul"
    Write-Info "Repository name: $repoName"
    Write-Info "Description: 🦠 Multi-threaded Antivirus Server with E2E Encryption | Concurrent & Distributed Programming Course Project"
    exit 0
}

try {
    # Add remote origin
    git remote add origin $remoteUrl
    Write-Success "Remote origin adăugat: $remoteUrl"
    
    # Set main branch
    git branch -M main
    Write-Success "Branch principal setat la: main"
    
    # Push to GitHub
    Write-Info "Push la GitHub în curs..."
    git push -u origin main
    Write-Success "Codul a fost încărcat pe GitHub!"
    
} catch {
    Write-Error "Push la GitHub a eșuat: $_"
    Write-Info "Verifică autentificarea GitHub (Personal Access Token)"
    Write-Info "Ghid: https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token"
    exit 1
}

# =============================================================================
# Create Release Tag
# =============================================================================

Write-Header "Creare Tag pentru Release"

$tagMessage = @"
🎓 Release v1.0.0: Complete PCD Course Project

✅ All requirements implemented:
- Multi-threaded server architecture
- Cross-platform client support  
- E2E encryption implementation
- ClamAV integration
- Comprehensive documentation
- Full test coverage

Ready for course submission and demonstration.
"@

try {
    git tag -a v1.0.0 -m "$tagMessage"
    git push origin v1.0.0
    Write-Success "Tag v1.0.0 creat și push-at pe GitHub"
} catch {
    Write-Warning "Crearea tag-ului a eșuat, dar repository-ul principal este funcțional"
}

# =============================================================================
# Final Instructions
# =============================================================================

Write-Header "Setup Complet!"

Write-Host @"

🎉 Proiectul a fost încărcat cu succes pe GitHub!

📊 Informații Repository:
   Repository: https://github.com/$GitHubUsername/$repoName
   Clone URL:  $remoteUrl
   Releases:   https://github.com/$GitHubUsername/$repoName/releases

🚀 Următorii pași recomandate:
   1. Vizitează repository-ul pe GitHub
   2. Adaugă topics: c-programming, cpp, python, concurrent-programming, etc.
   3. Creează un Release din tag-ul v1.0.0
   4. Adaugă descrierea proiectului în README

📝 Pentru actualizări viitoare:
   git add .
   git commit -m "📝 Update: Descrierea modificării"
   git push origin main

🎓 Proiectul este gata pentru demonstrație și evaluare!

"@ -ForegroundColor Green

# Open GitHub repository in browser
$openBrowser = Read-Host "Deschid repository-ul în browser? (y/n)"
if ($openBrowser -eq "y" -or $openBrowser -eq "Y") {
    Start-Process "https://github.com/$GitHubUsername/$repoName"
}

Write-Success "Setup GitHub finalizat cu succes! 🚀" 