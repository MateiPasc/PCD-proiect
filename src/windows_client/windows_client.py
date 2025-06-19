#!/usr/bin/env python3
"""
Windows Client for Antivirus Server
Implements GUI client for file scanning with E2E encryption
"""

import socket
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import threading
import time
import os
import struct
import hashlib
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
import base64

class AntivirusClient:
    def __init__(self):
        self.socket = None
        self.connected = False
        self.encryption_key = None
        self.server_host = "localhost"
        self.server_port = 8080
        
        # Create main window
        self.root = tk.Tk()
        self.root.title("Antivirus Scanner Client")
        self.root.geometry("800x600")
        self.root.resizable(True, True)
        
        self.setup_ui()
        
    def setup_ui(self):
        """Setup the user interface"""
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        
        # Connection frame
        conn_frame = ttk.LabelFrame(main_frame, text="Server Connection", padding="10")
        conn_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        conn_frame.columnconfigure(1, weight=1)
        
        ttk.Label(conn_frame, text="Server:").grid(row=0, column=0, sticky=tk.W)
        self.server_entry = ttk.Entry(conn_frame, width=30)
        self.server_entry.insert(0, f"{self.server_host}:{self.server_port}")
        self.server_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 10))
        
        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.connect_to_server)
        self.connect_btn.grid(row=0, column=2)
        
        self.status_label = ttk.Label(conn_frame, text="Status: Disconnected", foreground="red")
        self.status_label.grid(row=1, column=0, columnspan=3, sticky=tk.W, pady=(5, 0))
        
        # File selection frame
        file_frame = ttk.LabelFrame(main_frame, text="File Scanning", padding="10")
        file_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        file_frame.columnconfigure(1, weight=1)
        
        ttk.Label(file_frame, text="File:").grid(row=0, column=0, sticky=tk.W)
        self.file_entry = ttk.Entry(file_frame, width=50)
        self.file_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 5))
        
        ttk.Button(file_frame, text="Browse", command=self.browse_file).grid(row=0, column=2)
        
        # Buttons frame
        btn_frame = ttk.Frame(file_frame)
        btn_frame.grid(row=1, column=0, columnspan=3, pady=(10, 0))
        
        self.upload_btn = ttk.Button(btn_frame, text="Upload & Scan", command=self.upload_file, state=tk.DISABLED)
        self.upload_btn.pack(side=tk.LEFT, padx=(0, 5))
        
        self.check_status_btn = ttk.Button(btn_frame, text="Check Status", command=self.check_status, state=tk.DISABLED)
        self.check_status_btn.pack(side=tk.LEFT, padx=(0, 5))
        
        self.get_result_btn = ttk.Button(btn_frame, text="Get Result", command=self.get_result, state=tk.DISABLED)
        self.get_result_btn.pack(side=tk.LEFT)
        
        # Progress frame
        progress_frame = ttk.LabelFrame(main_frame, text="Progress", padding="10")
        progress_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        progress_frame.columnconfigure(0, weight=1)
        
        self.progress_var = tk.StringVar(value="Ready")
        self.progress_label = ttk.Label(progress_frame, textvariable=self.progress_var)
        self.progress_label.grid(row=0, column=0, sticky=tk.W)
        
        self.progress_bar = ttk.Progressbar(progress_frame, mode='indeterminate')
        self.progress_bar.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(5, 0))
        
        # Results frame
        results_frame = ttk.LabelFrame(main_frame, text="Results & Logs", padding="10")
        results_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        results_frame.columnconfigure(0, weight=1)
        results_frame.rowconfigure(0, weight=1)
        main_frame.rowconfigure(3, weight=1)
        
        self.results_text = scrolledtext.ScrolledText(results_frame, height=15, width=80)
        self.results_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Store current job ID
        self.current_job_id = None
        
        self.log("Antivirus Client initialized")
        
    def log(self, message):
        """Add message to results text area"""
        timestamp = time.strftime("%H:%M:%S")
        self.results_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.results_text.see(tk.END)
        self.root.update()
        
    def update_progress(self, message):
        """Update progress label"""
        self.progress_var.set(message)
        self.root.update()
        
    def browse_file(self):
        """Open file dialog to select file"""
        file_path = filedialog.askopenfilename(
            title="Select file to scan",
            filetypes=[("All Files", "*.*")]
        )
        if file_path:
            self.file_entry.delete(0, tk.END)
            self.file_entry.insert(0, file_path)
            
    def generate_encryption_key(self, shared_secret):
        """Generate encryption key from shared secret"""
        # Simple key derivation (for educational purposes)
        kdf = PBKDF2HMAC(
            algorithm=hashes.SHA256(),
            length=32,
            salt=b'antivirus_salt',
            iterations=100000,
        )
        key = base64.urlsafe_b64encode(kdf.derive(shared_secret.encode()))
        return Fernet(key)
        
    def perform_key_exchange(self):
        """Perform simple key exchange with server"""
        try:
            # Send a simple shared secret (for demo purposes)
            shared_secret = "antivirus_shared_secret_2024"
            self.socket.send(shared_secret.encode())
            
            # Receive server confirmation
            response = self.socket.recv(1024).decode()
            if response == "KEY_EXCHANGE_OK":
                self.encryption_key = self.generate_encryption_key(shared_secret)
                return True
            return False
        except Exception as e:
            self.log(f"Key exchange failed: {e}")
            return False
            
    def connect_to_server(self):
        """Connect to the antivirus server"""
        if self.connected:
            self.disconnect()
            return
            
        try:
            server_info = self.server_entry.get().strip()
            if ':' in server_info:
                self.server_host, port_str = server_info.split(':', 1)
                self.server_port = int(port_str)
            else:
                self.server_host = server_info
                
            self.update_progress("Connecting to server...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(10)
            self.socket.connect((self.server_host, self.server_port))
            
            # Perform key exchange
            if not self.perform_key_exchange():
                raise Exception("Key exchange failed")
                
            # Register client
            self.send_command("REGISTER_CLIENT")
            response = self.receive_response()
            
            if response.startswith("OK"):
                self.connected = True
                self.status_label.config(text="Status: Connected", foreground="green")
                self.connect_btn.config(text="Disconnect")
                self.upload_btn.config(state=tk.NORMAL)
                self.check_status_btn.config(state=tk.NORMAL)
                self.get_result_btn.config(state=tk.NORMAL)
                self.log(f"Connected to server at {self.server_host}:{self.server_port}")
                self.log("E2E encryption established")
                self.update_progress("Connected")
            else:
                raise Exception(f"Registration failed: {response}")
                
        except Exception as e:
            self.log(f"Connection failed: {e}")
            self.update_progress("Connection failed")
            if self.socket:
                self.socket.close()
                self.socket = None
                
    def disconnect(self):
        """Disconnect from server"""
        if self.socket:
            self.socket.close()
            self.socket = None
            
        self.connected = False
        self.status_label.config(text="Status: Disconnected", foreground="red")
        self.connect_btn.config(text="Connect")
        self.upload_btn.config(state=tk.DISABLED)
        self.check_status_btn.config(state=tk.DISABLED)
        self.get_result_btn.config(state=tk.DISABLED)
        self.log("Disconnected from server")
        self.update_progress("Disconnected")
        
    def send_command(self, command):
        """Send command to server"""
        if not self.connected or not self.socket:
            return False
            
        try:
            message = command + "\n"
            self.socket.send(message.encode())
            return True
        except Exception as e:
            self.log(f"Failed to send command: {e}")
            return False
            
    def receive_response(self):
        """Receive response from server"""
        if not self.connected or not self.socket:
            return ""
            
        try:
            response = self.socket.recv(4096).decode().strip()
            return response
        except Exception as e:
            self.log(f"Failed to receive response: {e}")
            return ""
            
    def encrypt_file(self, file_path):
        """Encrypt file for secure transmission"""
        try:
            if not self.encryption_key:
                raise Exception("No encryption key available")
                
            with open(file_path, 'rb') as f:
                file_data = f.read()
                
            encrypted_data = self.encryption_key.encrypt(file_data)
            
            # Save to temporary file
            temp_path = file_path + ".encrypted"
            with open(temp_path, 'wb') as f:
                f.write(encrypted_data)
                
            return temp_path
        except Exception as e:
            self.log(f"File encryption failed: {e}")
            return None
            
    def upload_file(self):
        """Upload and scan file"""
        file_path = self.file_entry.get().strip()
        if not file_path or not os.path.exists(file_path):
            messagebox.showerror("Error", "Please select a valid file")
            return
            
        if not self.connected:
            messagebox.showerror("Error", "Not connected to server")
            return
            
        def upload_thread():
            try:
                self.progress_bar.start()
                self.update_progress("Encrypting file...")
                
                # Encrypt file
                encrypted_path = self.encrypt_file(file_path)
                if not encrypted_path:
                    return
                    
                # Get file info
                filename = os.path.basename(file_path)
                file_size = os.path.getsize(encrypted_path)
                
                self.log(f"Uploading {filename} ({file_size} bytes encrypted)")
                self.update_progress("Uploading file...")
                
                # Send upload command
                upload_cmd = f"UPLOAD_FILE {filename} {file_size}"
                if not self.send_command(upload_cmd):
                    return
                    
                # Wait for ready response
                response = self.receive_response()
                if not response.startswith("OK"):
                    self.log(f"Upload failed: {response}")
                    return
                    
                # Send file data
                with open(encrypted_path, 'rb') as f:
                    sent = 0
                    while sent < file_size:
                        chunk = f.read(4096)
                        if not chunk:
                            break
                        self.socket.send(chunk)
                        sent += len(chunk)
                        
                        # Update progress
                        progress = (sent * 100) // file_size
                        self.update_progress(f"Uploading... {progress}%")
                        
                # Clean up encrypted file
                os.unlink(encrypted_path)
                
                # Get upload confirmation
                response = self.receive_response()
                if response.startswith("OK"):
                    # Extract job ID
                    if "Job ID:" in response:
                        self.current_job_id = response.split("Job ID:")[-1].strip()
                        self.log(f"File uploaded successfully. Job ID: {self.current_job_id}")
                        self.update_progress("Upload complete - scanning...")
                        
                        # Start monitoring
                        self.monitor_scan()
                    else:
                        self.log("File uploaded successfully")
                        self.update_progress("Upload complete")
                else:
                    self.log(f"Upload failed: {response}")
                    self.update_progress("Upload failed")
                    
            except Exception as e:
                self.log(f"Upload error: {e}")
                self.update_progress("Upload error")
            finally:
                self.progress_bar.stop()
                
        threading.Thread(target=upload_thread, daemon=True).start()
        
    def check_status(self):
        """Check scan status"""
        if not self.current_job_id:
            messagebox.showwarning("Warning", "No active scan job")
            return
            
        if not self.send_command(f"GET_SCAN_STATUS {self.current_job_id}"):
            return
            
        response = self.receive_response()
        self.log(f"Scan status: {response}")
        
    def get_result(self):
        """Get scan result"""
        if not self.current_job_id:
            messagebox.showwarning("Warning", "No active scan job")
            return
            
        if not self.send_command(f"GET_SCAN_RESULT {self.current_job_id}"):
            return
            
        response = self.receive_response()
        self.log(f"Scan result: {response}")
        
        # Show result in message box
        if "CLEAN" in response:
            messagebox.showinfo("Scan Result", "File is CLEAN - No threats detected")
        elif "INFECTED" in response:
            messagebox.showwarning("Scan Result", f"File is INFECTED!\n{response}")
        else:
            messagebox.showinfo("Scan Result", response)
            
    def monitor_scan(self):
        """Monitor scan progress asynchronously"""
        def monitor_thread():
            while self.connected and self.current_job_id:
                try:
                    time.sleep(2)
                    
                    if not self.send_command(f"GET_SCAN_STATUS {self.current_job_id}"):
                        break
                        
                    response = self.receive_response()
                    
                    if "COMPLETED" in response:
                        self.log("Scan completed!")
                        self.update_progress("Scan completed")
                        
                        # Get result
                        if self.send_command(f"GET_SCAN_RESULT {self.current_job_id}"):
                            result = self.receive_response()
                            self.log(f"Final result: {result}")
                            
                            # Show notification
                            if "CLEAN" in result:
                                self.log("✓ File is CLEAN")
                            elif "INFECTED" in result:
                                self.log("⚠ File is INFECTED!")
                                
                        break
                    elif "ERROR" in response:
                        self.log(f"Scan error: {response}")
                        self.update_progress("Scan error")
                        break
                    elif "PROCESSING" in response:
                        self.update_progress("Scanning in progress...")
                        
                except Exception as e:
                    self.log(f"Monitor error: {e}")
                    break
                    
        threading.Thread(target=monitor_thread, daemon=True).start()
        
    def run(self):
        """Start the GUI application"""
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.root.mainloop()
        
    def on_closing(self):
        """Handle window closing"""
        if self.connected:
            self.disconnect()
        self.root.destroy()

if __name__ == "__main__":
    try:
        app = AntivirusClient()
        app.run()
    except Exception as e:
        print(f"Application error: {e}")
        input("Press Enter to exit...") 