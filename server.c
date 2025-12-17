/*
 * Distributed Task Execution - Master Server
 * Server bertindak sebagai master yang membagi pekerjaan ke worker clients
 * Menggunakan TCP socket untuk komunikasi
 * Sistem Operasi: Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Struktur untuk menyimpan informasi worker
typedef struct {
    int socket;
    int worker_id;
    struct sockaddr_in address;
    long long partial_result;
    int is_done;
} Worker;

// Global variables
Worker workers[MAX_CLIENTS];
int worker_count = 0;
int target_worker_count = 0;  // Jumlah worker yang diinginkan user
pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER;
int *global_array = NULL;
int ARRAY_SIZE = 0;  // Ukuran array akan diinput oleh user

// Fungsi untuk generate array random
void generate_array(int *array, int size) {
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 100;  // Nilai 0-99
    }
}

// Fungsi untuk kalkulasi hasil akhir
long long calculate_final_result() {
    long long total = 0;
    for (int i = 0; i < worker_count; i++) {
        total += workers[i].partial_result;
    }
    return total;
}

// Fungsi untuk verifikasi hasil (sequential computation)
long long verify_result(int *array, int size) {
    long long sum = 0;
    for (int i = 0; i < size; i++) {
        sum += array[i];
    }
    return sum;
}

// Fungsi untuk input dan validasi ukuran array
int input_array_size() {
    int size;
    
    printf("\n=== Configuration ===\n");
    printf("Enter the number of array elements to process: ");
    
    if (scanf("%d", &size) != 1) {
        printf("Invalid input! Please enter a valid number.\n");
        return -1;
    }
    
    // Validasi input
    if (size <= 0) {
        printf("Array size must be positive!\n");
        return -1;
    }
    
    if (size > 100000000) {  // Limit 100 juta elemen
        printf("Array size too large! Maximum: 100,000,000 elements\n");
        return -1;
    }
    
    printf("Array size set to: %d elements\n", size);
    
    // Estimasi memory usage
    double memory_mb = (size * sizeof(int)) / (1024.0 * 1024.0);
    printf("Estimated memory usage: %.2f MB\n", memory_mb);
    
    return size;
}

// Fungsi untuk input jumlah worker
int input_worker_count() {
    int count;
    
    printf("\nEnter the number of workers needed: ");
    
    if (scanf("%d", &count) != 1) {
        printf("Invalid input! Please enter a valid number.\n");
        return -1;
    }
    
    // Validasi input
    if (count <= 0) {
        printf("Worker count must be positive!\n");
        return -1;
    }
    
    if (count > MAX_CLIENTS) {
        printf("Too many workers! Maximum: %d workers\n", MAX_CLIENTS);
        return -1;
    }
    
    printf("Waiting for %d worker(s) to connect...\n", count);
    
    return count;
}

// Thread handler untuk setiap worker
void *handle_worker(void *arg) {
    int worker_id = *(int*)arg;
    free(arg);
    
    Worker *worker = &workers[worker_id];
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    printf("[Worker %d] Ready and waiting...\n", worker_id);
    
    // Hitung range untuk worker ini (data partitioning)
    int chunk_size = ARRAY_SIZE / worker_count;
    int start_idx = worker_id * chunk_size;
    int end_idx = (worker_id == worker_count - 1) ? ARRAY_SIZE - 1 : (start_idx + chunk_size - 1);
    
    printf("[Worker %d] Assigned range: %d to %d (%d elements)\n", 
           worker_id, start_idx, end_idx, end_idx - start_idx + 1);
    
    // Kirim task ke worker
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "TASK SUM %d %d", start_idx, end_idx);
    
    // Kirim data array ke worker
    send(worker->socket, buffer, strlen(buffer), 0);
    printf("[Worker %d] Sent task: %s\n", worker_id, buffer);
    
    // Kirim array data
    int elements_to_send = end_idx - start_idx + 1;
    send(worker->socket, &elements_to_send, sizeof(int), 0);
    send(worker->socket, &global_array[start_idx], elements_to_send * sizeof(int), 0);
    printf("[Worker %d] Sent %d array elements\n", worker_id, elements_to_send);
    
    // Terima hasil dari worker
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(worker->socket, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        // Parse hasil: "RESULT <nilai>"
        long long result;
        if (sscanf(buffer, "RESULT %lld", &result) == 1) {
            pthread_mutex_lock(&worker_mutex);
            worker->partial_result = result;
            worker->is_done = 1;
            pthread_mutex_unlock(&worker_mutex);
            
            printf("[Worker %d] Received result: %lld\n", worker_id, result);
            
            // Kirim acknowledgment
            snprintf(response, BUFFER_SIZE, "ACK");
            send(worker->socket, response, strlen(response), 0);
        } else {
            printf("[Worker %d] Invalid result format: %s\n", worker_id, buffer);
        }
    } else {
        printf("[Worker %d] Connection lost\n", worker_id);
    }
    
    close(worker->socket);
    printf("[Worker %d] Disconnected\n", worker_id);
    
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;
    
    printf("=== Distributed Task Execution - Master Server ===\n");
    printf("Port: %d\n", PORT);
    printf("Maximum Workers: %d\n", MAX_CLIENTS);
    
    // Input ukuran array dari user
    ARRAY_SIZE = input_array_size();
    if (ARRAY_SIZE <= 0) {
        printf("Failed to set array size. Exiting...\n");
        exit(EXIT_FAILURE);
    }
    
    // Input jumlah worker yang diinginkan
    target_worker_count = input_worker_count();
    if (target_worker_count <= 0) {
        printf("Failed to set worker count. Exiting...\n");
        exit(EXIT_FAILURE);
    }
    
    // Alokasi dan generate array
    printf("\n=== Generating Array ===\n");
    printf("Generating random array...\n");
    global_array = (int*)malloc(ARRAY_SIZE * sizeof(int));
    if (global_array == NULL) {
        perror("Failed to allocate memory for array");
        exit(EXIT_FAILURE);
    }
    generate_array(global_array, ARRAY_SIZE);
    printf("Array generated successfully!\n");
    
    // Buat socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options untuk reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    printf("\n=== Server Ready ===\n");
    printf("Listening on port %d...\n", PORT);
    printf("Waiting for %d worker(s) to connect...\n", target_worker_count);
    printf("Connect your worker clients now...\n\n");
    
    // Accept worker connections sampai target tercapai
    while (worker_count < target_worker_count) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Simpan informasi worker
        pthread_mutex_lock(&worker_mutex);
        int current_worker = worker_count;
        workers[current_worker].socket = client_socket;
        workers[current_worker].worker_id = current_worker;
        workers[current_worker].address = client_addr;
        workers[current_worker].partial_result = 0;
        workers[current_worker].is_done = 0;
        worker_count++;
        
        printf("✓ Worker %d connected from %s:%d\n", 
               current_worker,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        printf("  Progress: %d/%d workers connected\n", worker_count, target_worker_count);
        
        if (worker_count < target_worker_count) {
            printf("  Waiting for %d more worker(s)...\n\n", target_worker_count - worker_count);
        } else {
            printf("  All workers connected! Starting processing...\n\n");
        }
        
        pthread_mutex_unlock(&worker_mutex);
        
        // Buat thread untuk handle worker
        int *worker_id_ptr = malloc(sizeof(int));
        *worker_id_ptr = current_worker;
        if (pthread_create(&thread_id, NULL, handle_worker, worker_id_ptr) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            pthread_mutex_lock(&worker_mutex);
            worker_count--;
            pthread_mutex_unlock(&worker_mutex);
            free(worker_id_ptr);
        } else {
            pthread_detach(thread_id);
        }
        
        // Tunggu sebentar untuk stabilitas
        sleep(1);
    }
    
    // Semua worker sudah terhubung, mulai processing
    printf("\n=== Starting Computation ===\n");
    printf("Distributing tasks to %d workers...\n\n", worker_count);
    
    // Tunggu semua worker selesai
    while (1) {
        int all_done = 1;
        pthread_mutex_lock(&worker_mutex);
        for (int i = 0; i < worker_count; i++) {
            if (!workers[i].is_done) {
                all_done = 0;
                break;
            }
        }
        pthread_mutex_unlock(&worker_mutex);
        
        if (all_done) {
            break;
        }
        sleep(1);
    }
    
    printf("\n=== All workers completed their tasks ===\n\n");
    
    // Hitung hasil akhir (parallel result)
    long long parallel_result = calculate_final_result();
    printf("Parallel Computation Result: %lld\n", parallel_result);
    
    // Verifikasi dengan sequential computation
    printf("\nVerifying with sequential computation...\n");
    clock_t start = clock();
    long long sequential_result = verify_result(global_array, ARRAY_SIZE);
    clock_t end = clock();
    double sequential_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Sequential Computation Result: %lld\n", sequential_result);
    printf("Sequential Computation Time: %.6f seconds\n", sequential_time);
    
    // Validasi hasil
    if (parallel_result == sequential_result) {
        printf("\n✓ VERIFICATION SUCCESSFUL! Results match.\n");
    } else {
        printf("\n✗ VERIFICATION FAILED! Results don't match.\n");
        printf("Difference: %lld\n", parallel_result - sequential_result);
    }
    
    printf("\n=== Summary ===\n");
    printf("Total Workers: %d\n", worker_count);
    printf("Array Size: %d elements\n", ARRAY_SIZE);
    printf("Elements per Worker: ~%d\n", ARRAY_SIZE / worker_count);
    
    printf("\nWorker Results:\n");
    for (int i = 0; i < worker_count; i++) {
        printf("  Worker %d: %lld\n", i, workers[i].partial_result);
    }
    
    // Tunggu sebentar sebelum cleanup
    printf("\nPress Enter to shutdown server...");
    getchar();
    getchar();  // Clear buffer dari scanf sebelumnya
    
    // Cleanup
    free(global_array);
    close(server_socket);
    
    printf("Server shutting down...\n");
    
    return 0;
}
