# 🦠 Server de Scanare Antiviruși la Distanță

[![GitHub](https://img.shields.io/badge/GitHub-Repository-blue?logo=github)](https://github.com/yourusername/antivirus-server-pcd)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B%2FPython-green)](https://github.com/yourusername/antivirus-server-pcd)
[![License](https://img.shields.io/badge/License-Educational-yellow)](https://github.com/yourusername/antivirus-server-pcd)

## 📋 Descriere Proiect

Acest proiect implementează un sistem client-server pentru scanarea antiviruși la distanță, dezvoltat pentru materia "Programare Concurentă și Distributivă".

### Funcționalități Principale

- **Server principal (C/UNIX)**: Primește fișiere de la clienți și le scanează pentru viruși
- **Client de administrare (C++)**: Gestionează serverul, vizualizează loguri, controlează conexiuni
- **Client ordinar UNIX (C++)**: Trimite fișiere pentru scanare
- **Client ordinar Windows (Python)**: Alternativă multi-platformă
- **Criptare E2E**: Fișierele sunt criptate înainte de transfer

## Arhitectura Sistemului

```
┌─────────────────┐    ┌──────────────────────┐    ┌─────────────────┐
│   Admin Client  │────│   Server Principal   │────│  Ordinary Client│
│   (UNIX Socket) │    │     (C/UNIX)         │    │   (INET Socket) │
│     (C++)       │    │                      │    │     (C++/Py)    │
└─────────────────┘    └──────────────────────┘    └─────────────────┘
                              │
                         ┌────▼────┐
                         │ ClamAV  │
                         │Scanner  │
                         └─────────┘
```

## Componente

### 1. Server Principal (`src/server/`)
- **Fișier principal**: `antivirus_server.c`
- **Fire de execuție**:
  - Thread pentru admin client (UNIX socket)
  - Thread pentru clienți ordinari (INET socket)  
  - Thread pentru procesarea cererilor (coadă FIFO)
  - Thread pentru monitorizare fișiere (inotify)
- **Funcționalități**:
  - Scanare cu ClamAV
  - Criptare/decriptare E2E
  - Comunicare sincronă și asincronă
  - Logging configurabil

### 2. Client de Administrare (`src/admin_client/`)
- **Fișier principal**: `admin_client.cpp`
- **Funcționalități**:
  - Vizualizare loguri în timp real
  - Setare nivel de logging
  - Deconectare clienți
  - Statistici server
  - Interfață ncurses

### 3. Client Ordinar UNIX (`src/ordinary_client/`)
- **Fișier principal**: `ordinary_client.cpp`
- **Funcționalități**:
  - Trimitere fișiere pentru scanare
  - Criptare automată E2E
  - Primire rezultate async
  - Interface CLI

### 4. Client Ordinar Windows (`src/windows_client/`)
- **Fișier principal**: `windows_client.py`
- **Funcționalități similare** cu clientul UNIX
- **Platform**: Windows cu Python

## Protocoale de Comunicare

### Protocol Admin (UNIX Socket)
```
ADMIN_AUTH <password>
SET_LOG_LEVEL <level>
GET_LOGS
GET_STATS
DISCONNECT_CLIENT <ip>
SHUTDOWN_SERVER
```

### Protocol Client Ordinar (INET Socket)
```
REGISTER_CLIENT
UPLOAD_FILE <filename> <size> <encrypted_data>
GET_SCAN_STATUS <job_id>
GET_SCAN_RESULT <job_id>
DOWNLOAD_FILE <filename>
```

## Criptare E2E

- **Algoritm**: AES-256 simplificat (implementare proprie)
- **Schimb de chei**: Diffie-Hellman simplificat
- **Flow**:
  1. Client generează cheie temporară
  2. Criptează fișierul cu cheia
  3. Trimite cheia criptată cu cheia publică a serverului
  4. Serverul decriptează și procesează fișierul

## 🚀 Instalare și Utilizare

### 📥 Clonare din GitHub

```bash
# Clonează repository-ul
git clone https://github.com/yourusername/antivirus-server-pcd.git
cd antivirus-server-pcd

# Verifică structura
ls -la
make info
```

### 📋 Cerințe de Sistem
```bash
# Ubuntu/Debian
sudo apt-get install build-essential libclamav-dev libncurses5-dev

# Pentru Windows client
pip install socket cryptography
```

### Compilare
```bash
make all
```

### Rulare
```bash
# 1. Pornire server
./bin/antivirus_server

# 2. Client admin (în alt terminal)
./bin/admin_client

# 3. Client ordinar (în alt terminal)
./bin/ordinary_client

# 4. Client Windows
cd src/windows_client && python windows_client.py
```

## Structura Directoare

```
PCD/
├── src/
│   ├── server/
│   │   ├── antivirus_server.c
│   │   ├── server_threads.c
│   │   ├── encryption.c
│   │   ├── scanner.c
│   │   └── protocol.h
│   ├── admin_client/
│   │   ├── admin_client.cpp
│   │   └── admin_ui.cpp
│   ├── ordinary_client/
│   │   ├── ordinary_client.cpp
│   │   └── client_crypto.cpp
│   ├── windows_client/
│   │   └── windows_client.py
│   └── common/
│       ├── common.h
│       ├── crypto_common.c
│       └── protocol_common.c
├── include/
├── bin/
├── logs/
├── processing/
├── outgoing/
├── tests/
├── docs/
├── Makefile
└── README.md
```

## Demonstrare Funcționare

### Scenarii de Test
1. **Test de bază**: Upload fișier clean → rezultat OK
2. **Test virus**: Upload fișier infectat → rezultat INFECTED  
3. **Test criptare**: Verificare integritate E2E
4. **Test admin**: Schimbare nivel logging, deconectare client
5. **Test concurență**: Multiple clienți simultan
6. **Test async**: Scanare lungă cu notificare

### Loguri și Monitoring
- Loguri centralizate în `logs/`
- Nivele: DEBUG, INFO, WARNING, ERROR
- Monitoring în timp real prin admin client

## Nivel de Complexitate: B/C

- ✅ **Nivel A**: poll/select, fire de execuție, procese copil
- ✅ **Nivel B**: pipe anonim, sincronizare pthread, procesare semnale
- ✅ **Nivel C**: mutex, semafoare, variabile de condiție, bariere
- ✅ **Bonus**: Criptare E2E customizată

## Autori

Proiect realizat pentru materia "Programare Concurentă și Distributivă"

## Licență

Proiect educațional - utilizare academică 