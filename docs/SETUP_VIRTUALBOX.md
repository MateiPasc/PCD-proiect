# 🖥️ Ghid Setup VirtualBox pentru Proiectul Antivirus Server

## 📋 Prezentare Generală

Acest ghid te va ajuta să rulezi proiectul **Server de Scanare Antiviruși** într-un mediu VirtualBox, permițând testarea completă a funcționalității pe un sistem Linux virtual.

---

## 🛠️ Prerequisite

### Software Necesar
- **Oracle VirtualBox** (versiunea 6.1 sau mai nouă)
- **Ubuntu Server/Desktop ISO** (20.04 LTS sau 22.04 LTS recomandat)
- **Minim 4GB RAM** pentru VM
- **20GB spațiu pe disk** pentru VM

---

## 🔧 Setup Mașină Virtuală

### Pasul 1: Crearea VM-ului

1. **Deschide VirtualBox** și click pe "New"
2. **Configurează VM-ul:**
   ```
   Name: Antivirus-Server-PCD
   Type: Linux
   Version: Ubuntu (64-bit)
   Memory: 4096 MB (4GB)
   Hard disk: Create a virtual hard disk now (VDI, 20GB)
   ```

3. **Setări avansate:**
   - **System → Processor:** 2-4 CPU cores
   - **System → Acceleration:** Enable VT-x/AMD-V
   - **Storage:** Attach Ubuntu ISO la CD/DVD drive
   - **Network:** NAT + Host-only Adapter

### Pasul 2: Instalarea Ubuntu

1. **Boot VM-ul** cu Ubuntu ISO
2. **Alege:** "Install Ubuntu Server" sau "Ubuntu Desktop"
3. **Configurează:**
   - Username: `student` (sau preferința ta)
   - Password: parola ta
   - Hostname: `antivirus-server`
4. **Instalare completă** și restart

### Pasul 3: Configurarea Network

Pentru a permite comunicarea între Windows host și Linux VM:

```bash
# În VM Linux
sudo ip addr show  # Notează IP-ul VM-ului (ex: 192.168.56.101)

# Testează conectivitatea
ping google.com
```

---

## 📦 Instalarea Dependințelor

### În VM Linux, rulează:

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install essential build tools
sudo apt install -y build-essential

# Install project dependencies
sudo apt install -y libclamav-dev libncurses5-dev libssl-dev

# Install additional tools
sudo apt install -y git vim wget curl

# Install ClamAV and update database
sudo apt install -y clamav clamav-daemon
sudo systemctl stop clamav-freshclam
sudo freshclam
sudo systemctl start clamav-freshclam

# Install Python for testing (optional)
sudo apt install -y python3 python3-pip
pip3 install cryptography

# Verify installations
gcc --version
clamscan --version
```

---

## 📁 Transfer Cod Sursă

### Opțiunea 1: Shared Folder (Recomandat)

1. **În VirtualBox Manager:**
   - VM → Settings → Shared Folders
   - Add folder: `C:\Users\pascb\OneDrive\Desktop\PCD`
   - Mount point: `/home/student/PCD`
   - Auto-mount: ✅

2. **În VM Linux:**
   ```bash
   # Install VirtualBox Guest Additions
   sudo apt install -y virtualbox-guest-additions-iso
   
   # Mount shared folder
   sudo mkdir -p /mnt/shared
   sudo mount -t vboxsf PCD /mnt/shared
   
   # Copy to local directory
   cp -r /mnt/shared ~/antivirus-project
   cd ~/antivirus-project
   ```

### Opțiunea 2: SCP/SFTP

```bash
# Pe Windows (în PowerShell)
scp -r C:\Users\pascb\OneDrive\Desktop\PCD student@192.168.56.101:~/antivirus-project
```

### Opțiunea 3: Git Repository

```bash
# În VM Linux
git clone <repository-url> ~/antivirus-project
cd ~/antivirus-project
```

---

## 🔨 Compilarea Proiectului

### În VM Linux:

```bash
cd ~/antivirus-project

# Verifică dependințele
make info

# Compilează toate componentele
make all

# Verifică executabilele
ls -la bin/
```

**Output așteptat:**
```
bin/antivirus_server
bin/admin_client  
bin/ordinary_client
```

---

## 🚀 Rularea Demo-ului Complet

### Setup 1: Server + Admin + Client în VM

#### Terminal 1 - Server
```bash
cd ~/antivirus-project
make run-server
```

#### Terminal 2 - Admin Client  
```bash
cd ~/antivirus-project
make run-admin
```

#### Terminal 3 - Ordinary Client
```bash
cd ~/antivirus-project
make run-client
```

### Setup 2: Client Python pe Windows Host

Pe **Windows host**, dacă ai Python instalat:

```powershell
# În directorul proiectului
cd C:\Users\pascb\OneDrive\Desktop\PCD\src\windows_client

# Instalează dependințele
pip install tkinter cryptography

# Modifică adresa serverului în windows_client.py
# Schimbă server_host de la "localhost" la IP-ul VM-ului (ex: 192.168.56.101)

# Rulează clientul GUI
python windows_client.py
```

---

## 🧪 Scenarii de Testare

### Test 1: Funcționalitate de Bază

```bash
# În VM Linux, terminal client
cd ~/antivirus-project

# Creează fișiere de test
echo "This is a clean file" > test_clean.txt
echo 'X5O!P%@AP[4\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*' > test_virus.txt

# Testează upload
./bin/ordinary_client
```

Comenzi în client:
```
upload test_clean.txt
status 1
result 1
upload test_virus.txt  
status 2
result 2
quit
```

### Test 2: Multiple Clienți

**Terminal 4 - Al doilea client:**
```bash
cd ~/antivirus-project
./bin/ordinary_client
```

**Terminal 5 - Al treilea client:**
```bash
cd ~/antivirus-project  
./bin/ordinary_client
```

### Test 3: Cross-Platform Testing

- **Server:** Linux VM
- **Admin:** Linux VM  
- **Client 1:** Linux VM
- **Client 2:** Windows host (Python GUI)

---

## 🔍 Troubleshooting

### Problemă: Server nu pornește

```bash
# Verifică porturile
sudo netstat -tulpn | grep 8080

# Verifică socket-ul admin
ls -la /tmp/antivirus_admin.sock

# Verifică log-urile
tail -f logs/server.log
```

### Problemă: ClamAV nu funcționează

```bash
# Verifică serviciul
sudo systemctl status clamav-freshclam

# Update manual
sudo freshclam

# Test manual
clamscan --version
clamscan test_virus.txt
```

### Problemă: Clientul Python nu se conectează

1. **Verifică IP-ul VM:**
   ```bash
   ip addr show
   ```

2. **Modifică IP în `windows_client.py`:**
   ```python
   self.server_host = "192.168.56.101"  # IP-ul VM-ului
   ```

3. **Verifică firewall:**
   ```bash
   sudo ufw allow 8080
   ```

---

## 📊 Configurare Avansată VM

### Pentru Performanță Optimă:

1. **Setări VirtualBox:**
   ```
   System → Motherboard → Base Memory: 6144 MB
   System → Processor → Processors: 4
   System → Acceleration → Enable VT-x/AMD-V + Nested Paging
   Display → Video Memory: 128 MB
   ```

2. **În Linux VM:**
   ```bash
   # Optimizări pentru compilare
   export MAKEFLAGS="-j$(nproc)"
   
   # Monitorizare resurse
   htop
   iotop
   ```

### Network Setup pentru Testare Avansată:

```bash
# VM cu 2 adaptoare network:
# Adapter 1: NAT (pentru internet)  
# Adapter 2: Host-only (pentru comunicare cu host)

# Configurare manuală IP static
sudo nano /etc/netplan/01-network-manager-all.yaml
```

```yaml
network:
  version: 2
  ethernets:
    enp0s8:
      dhcp4: false
      addresses: [192.168.56.101/24]
```

```bash
sudo netplan apply
```

---

## 🎯 Demo Script Complet pentru VirtualBox

Creez un script automat pentru demo:

```bash
#!/bin/bash
# demo_virtualbox.sh

echo "=== Antivirus Server Demo în VirtualBox ==="

# Start server în background
./bin/antivirus_server &
SERVER_PID=$!
sleep 3

echo "Server started (PID: $SERVER_PID)"
echo "IP VM: $(hostname -I | awk '{print $1}')"
echo ""
echo "Pentru a testa:"
echo "1. Rulează './bin/admin_client' în alt terminal"
echo "2. Rulează './bin/ordinary_client' în alt terminal"  
echo "3. Conectează clientul Python din Windows la IP-ul afișat"
echo ""
echo "Press Enter pentru a opri serverul..."
read

# Cleanup
kill $SERVER_PID
echo "Demo terminated."
```

---

## 📝 Checklist Final

### ✅ **Verificări înainte de demo:**

- [ ] VM Linux funcțional cu Ubuntu
- [ ] Toate dependințele instalate
- [ ] Codul compilat cu succes (`make all`)
- [ ] ClamAV functional (`clamscan --version`)
- [ ] Network configurat pentru comunicare cu host
- [ ] Fișiere de test create
- [ ] IP-ul VM notat pentru clientul Python

### ✅ **Demonstrație completă:**

- [ ] Server pornit și funcțional
- [ ] Admin client conectat și operațional
- [ ] Client ordinar Linux funcțional
- [ ] Client Python Windows conectat
- [ ] Test upload fișier clean
- [ ] Test upload fișier EICAR (virus test)
- [ ] Verificare criptare E2E
- [ ] Test funcții administrative

---

## 🎓 Avantajele Setup-ului VirtualBox

1. **Izolare:** Testare sigură fără afectarea sistemului host
2. **Portabilitate:** VM poate fi exportat/importat ușor
3. **Flexibilitate:** Testare cross-platform reală
4. **Reproducibilitate:** Mediu consistent pentru demonstrație
5. **Scalabilitate:** Posibilitatea de a crea multiple VM-uri pentru testare distribuită

**Succes cu proiectul!** 🚀 