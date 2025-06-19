#include "../../include/common.h"
#include <openssl/rand.h>

// Simple XOR-based encryption (for educational purposes)
// In a real implementation, use proper AES encryption

static void simple_xor_encrypt(const unsigned char* input, unsigned char* output, 
                              size_t length, const unsigned char* key, size_t key_length) {
    for (size_t i = 0; i < length; i++) {
        output[i] = input[i] ^ key[i % key_length];
    }
}

static void simple_xor_decrypt(const unsigned char* input, unsigned char* output, 
                              size_t length, const unsigned char* key, size_t key_length) {
    // XOR is symmetric
    simple_xor_encrypt(input, output, length, key, key_length);
}

void generate_key(crypto_key_t* key) {
    // Generate random key and IV
    for (int i = 0; i < 32; i++) {
        key->key[i] = rand() % 256;
    }
    for (int i = 0; i < 16; i++) {
        key->iv[i] = rand() % 256;
    }
}

int encrypt_file(const char* input_file, const char* output_file, const crypto_key_t* key) {
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        perror("fopen input file");
        return -1;
    }
    
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        perror("fopen output file");
        fclose(in);
        return -1;
    }
    
    // Write IV to beginning of encrypted file
    fwrite(key->iv, 1, 16, out);
    
    unsigned char buffer[BUFFER_SIZE];
    unsigned char encrypted_buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
        simple_xor_encrypt(buffer, encrypted_buffer, bytes_read, key->key, 32);
        fwrite(encrypted_buffer, 1, bytes_read, out);
    }
    
    fclose(in);
    fclose(out);
    return 0;
}

int decrypt_file(const char* input_file, const char* output_file, const crypto_key_t* key) {
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        perror("fopen input file");
        return -1;
    }
    
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        perror("fopen output file");
        fclose(in);
        return -1;
    }
    
    // Read IV from beginning of encrypted file
    unsigned char file_iv[16];
    if (fread(file_iv, 1, 16, in) != 16) {
        fprintf(stderr, "Failed to read IV from encrypted file\n");
        fclose(in);
        fclose(out);
        return -1;
    }
    
    // Verify IV matches (simple check)
    if (memcmp(file_iv, key->iv, 16) != 0) {
        fprintf(stderr, "IV mismatch - wrong key or corrupted file\n");
        fclose(in);
        fclose(out);
        return -1;
    }
    
    unsigned char buffer[BUFFER_SIZE];
    unsigned char decrypted_buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
        simple_xor_decrypt(buffer, decrypted_buffer, bytes_read, key->key, 32);
        fwrite(decrypted_buffer, 1, bytes_read, out);
    }
    
    fclose(in);
    fclose(out);
    return 0;
}

int send_encrypted_data(int socket_fd, const void* data, size_t size, const crypto_key_t* key) {
    unsigned char* encrypted_data = malloc(size);
    if (!encrypted_data) {
        return -1;
    }
    
    simple_xor_encrypt((const unsigned char*)data, encrypted_data, size, key->key, 32);
    
    int result = send(socket_fd, encrypted_data, size, 0);
    free(encrypted_data);
    
    return result;
}

int receive_encrypted_data(int socket_fd, void* data, size_t size, const crypto_key_t* key) {
    unsigned char* encrypted_data = malloc(size);
    if (!encrypted_data) {
        return -1;
    }
    
    int result = recv(socket_fd, encrypted_data, size, 0);
    if (result > 0) {
        simple_xor_decrypt(encrypted_data, (unsigned char*)data, result, key->key, 32);
    }
    
    free(encrypted_data);
    return result;
}

// Key exchange helpers (simplified Diffie-Hellman)
typedef struct {
    unsigned int p;  // prime
    unsigned int g;  // generator
    unsigned int private_key;
    unsigned int public_key;
} dh_key_t;

void generate_dh_keys(dh_key_t* dh) {
    // Simple DH parameters (for demo - use proper large primes in production)
    dh->p = 23;  // prime
    dh->g = 5;   // generator
    dh->private_key = (rand() % 20) + 1;  // private key (1-20)
    
    // Calculate public key: g^private_key mod p
    unsigned int result = 1;
    unsigned int base = dh->g;
    unsigned int exp = dh->private_key;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % dh->p;
        }
        base = (base * base) % dh->p;
        exp /= 2;
    }
    
    dh->public_key = result;
}

unsigned int calculate_shared_secret(const dh_key_t* dh, unsigned int other_public_key) {
    // Calculate shared secret: other_public_key^private_key mod p
    unsigned int result = 1;
    unsigned int base = other_public_key;
    unsigned int exp = dh->private_key;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % dh->p;
        }
        base = (base * base) % dh->p;
        exp /= 2;
    }
    
    return result;
}

int perform_key_exchange(int socket_fd, crypto_key_t* shared_key, int is_server) {
    dh_key_t dh;
    generate_dh_keys(&dh);
    
    unsigned int other_public_key;
    
    if (is_server) {
        // Server: receive client's public key first
        if (recv(socket_fd, &other_public_key, sizeof(other_public_key), 0) != sizeof(other_public_key)) {
            return -1;
        }
        
        // Send our public key
        if (send(socket_fd, &dh.public_key, sizeof(dh.public_key), 0) != sizeof(dh.public_key)) {
            return -1;
        }
    } else {
        // Client: send our public key first
        if (send(socket_fd, &dh.public_key, sizeof(dh.public_key), 0) != sizeof(dh.public_key)) {
            return -1;
        }
        
        // Receive server's public key
        if (recv(socket_fd, &other_public_key, sizeof(other_public_key), 0) != sizeof(other_public_key)) {
            return -1;
        }
    }
    
    // Calculate shared secret
    unsigned int shared_secret = calculate_shared_secret(&dh, other_public_key);
    
    // Derive encryption key from shared secret (simple method)
    memset(shared_key, 0, sizeof(crypto_key_t));
    
    // Use shared secret to seed key generation
    srand(shared_secret);
    for (int i = 0; i < 32; i++) {
        shared_key->key[i] = rand() % 256;
    }
    for (int i = 0; i < 16; i++) {
        shared_key->iv[i] = rand() % 256;
    }
    
    return 0;
} 