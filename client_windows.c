/*
 * Distributed Task Execution - Worker Client (Windows)
 * Client bertindak sebagai worker yang memproses pekerjaan dari master
 * Platform: Windows
 * Menggunakan TCP socket untuk komunikasi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

// Fungsi untuk inisialisasi Winsock
int init_winsock() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}

// Fungsi untuk cleanup Winsock
void cleanup_winsock() {
    WSACleanup();
}

// Fungsi untuk memproses task sum
long long process_sum_task(int *array, int size) {
    long long sum = 0;
    
    printf("Processing sum of %d elements...\n", size);
    
    // Simulasi komputasi dengan progress indicator
    int progress_step = size / 10;
    if (progress_step == 0) progress_step = 1;
    
    clock_t start = clock();
    
    for (int i = 0; i < size; i++) {
        sum += array[i];
        
        // Tampilkan progress setiap 10%
        if (progress_step > 0 && (i + 1) % progress_step == 0) {
            int progress = ((i + 1) * 100) / size;
            printf("Progress: %d%%\r", progress);
            fflush(stdout);
        }
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Progress: 100%%\n");
    printf("Computation completed in %.6f seconds\n", time_taken);
    
    return sum;
}

int main(int argc, char *argv[]) {
    SOCKET sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char *server_ip;
    int server_port;
    
    printf("=== Distributed Task Execution - Worker Client ===\n");
    printf("Platform: Windows\n");
    
    // Parse command line arguments
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        printf("Example: %s 192.168.1.100 8080\n", argv[0]);
        return 1;
    }
    
    server_ip = argv[1];
    server_port = atoi(argv[2]);
    
    printf("Server: %s:%d\n\n", server_ip, server_port);
    
    // Inisialisasi Winsock
    if (init_winsock() != 0) {
        return 1;
    }
    
    // Buat socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        cleanup_winsock();
        return 1;
    }
    
    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    // Convert IP address
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid address or address not supported\n");
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    // Connect ke server
    printf("Connecting to server...\n");
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    printf("Connected to server successfully!\n");
    printf("Waiting for task...\n\n");
    
    // Terima task dari server
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received <= 0) {
        printf("Failed to receive task from server\n");
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    buffer[bytes_received] = '\0';
    printf("Received task: %s\n", buffer);
    
    // Parse task: "TASK SUM <start> <end>"
    char task_type[20], operation[20];
    int start_idx, end_idx;
    
    if (sscanf(buffer, "%s %s %d %d", task_type, operation, &start_idx, &end_idx) != 4) {
        printf("Invalid task format\n");
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    if (strcmp(task_type, "TASK") != 0 || strcmp(operation, "SUM") != 0) {
        printf("Unknown task type or operation\n");
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    printf("Task: Calculate SUM from index %d to %d\n", start_idx, end_idx);
    
    // Terima jumlah elemen
    int num_elements;
    bytes_received = recv(sock, (char*)&num_elements, sizeof(int), 0);
    if (bytes_received != sizeof(int)) {
        printf("Failed to receive array size\n");
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    printf("Number of elements to process: %d\n", num_elements);
    
    // Alokasi memori untuk array
    int *array = (int*)malloc(num_elements * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation failed\n");
        closesocket(sock);
        cleanup_winsock();
        return 1;
    }
    
    // Terima array data
    printf("Receiving array data...\n");
    int total_bytes = num_elements * sizeof(int);
    int bytes_read = 0;
    
    while (bytes_read < total_bytes) {
        int bytes = recv(sock, ((char*)array) + bytes_read, total_bytes - bytes_read, 0);
        if (bytes <= 0) {
            printf("Failed to receive array data. Error Code: %d\n", WSAGetLastError());
            free(array);
            closesocket(sock);
            cleanup_winsock();
            return 1;
        }
        bytes_read += bytes;
    }
    
    printf("Array data received successfully!\n\n");
    
    // Proses task
    printf("=== Starting Computation ===\n");
    long long result = process_sum_task(array, num_elements);
    printf("Result: %lld\n\n", result);
    
    // Kirim hasil ke server
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "RESULT %lld", result);
    
    printf("Sending result to server...\n");
    send(sock, buffer, strlen(buffer), 0);
    
    // Tunggu acknowledgment
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        if (strcmp(buffer, "ACK") == 0) {
            printf("Server acknowledged the result\n");
        }
    }
    
    printf("\n=== Task Completed Successfully ===\n");
    
    // Cleanup
    free(array);
    closesocket(sock);
    cleanup_winsock();
    
    printf("\nPress Enter to exit...");
    getchar();
    
    return 0;
}
