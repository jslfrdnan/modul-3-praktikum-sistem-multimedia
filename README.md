# Distributed Task Execution System (Master-Worker)

Sistem terdistribusi sederhana untuk eksekusi task paralel menggunakan arsitektur Master-Worker.

## 📋 Deskripsi Proyek

Sistem ini mengimplementasikan distributed computing dengan:
- **Server (Master)**: Membagi pekerjaan dan menggabungkan hasil
- **Client (Worker)**: Memproses sebagian pekerjaan secara paralel
- **Protokol**: TCP untuk komunikasi yang reliable
- **Bahasa**: C untuk performa optimal

## 🎯 Konsep Arsitektur Komputer

1. **Parallel Computation**: Pembagian komputasi ke multiple workers
2. **Load Balancing**: Distribusi beban kerja yang merata
3. **Memory Hierarchy**: Optimasi penggunaan memori
4. **Data Partitioning**: Pembagian data secara efisien

## 🖥️ Spesifikasi Sistem

### Server
- **OS**: Linux
- **Port**: 8080
- **Fungsi**: Master node yang mengkoordinasi workers

### Client
- **Client 1**: Windows
- **Client 2**: Linux (Virtual Machine)
- **Fungsi**: Worker nodes yang memproses task

## 📦 Struktur File

```
Proyek 2 New/
├── server.c                      # Program server (master) - Linux
├── client_linux.c                # Program client (worker) - Linux
├── client_windows.c              # Program client (worker) - Windows
├── client.c                      # Program client (worker) - Legacy/Cross-platform
├── compile_server.sh             # Script kompilasi server (Linux)
├── compile_client_linux.sh       # Script kompilasi client (Linux)
├── compile_client_windows.bat    # Script kompilasi client (Windows)
├── README.md                     # Dokumentasi ini
└── LAPORAN.md                    # Laporan lengkap proyek
```

## 🔧 Cara Kompilasi

### Server (Linux)

```bash
chmod +x compile_server.sh
./compile_server.sh
```

Atau manual:
```bash
gcc server.c -o server -lpthread -Wall
```

### Client Linux

```bash
chmod +x compile_client_linux.sh
./compile_client_linux.sh
```

Atau manual:
```bash
gcc client_linux.c -o client -Wall
```

### Client Windows

Jalankan file batch:
```cmd
compile_client_windows.bat
```

Atau manual (menggunakan MinGW):
```cmd
gcc client_windows.c -o client.exe -lws2_32 -Wall
```

**Note**: Untuk Windows, pastikan MinGW atau GCC sudah terinstall.

## 🚀 Cara Menjalankan

### 1. Jalankan Server (di Linux)

```bash
./server
```

Server akan meminta 2 input:
1. **Jumlah elemen array**: Masukkan angka (contoh: 1000000 untuk 1 juta)
2. **Jumlah worker**: Masukkan berapa worker yang diinginkan (contoh: 2 atau 3)

Server kemudian akan:
- Generate array random
- Listen pada port 8080
- Menunggu hingga jumlah worker yang diminta terhubung semua
- Otomatis mulai processing setelah semua worker connect

Contoh interaksi:
```
=== Distributed Task Execution - Master Server ===
Port: 8080
Maximum Workers: 10

=== Configuration ===
Enter the number of array elements to process: 1000000
Array size set to: 1000000 elements
Estimated memory usage: 3.81 MB

Enter the number of workers needed: 2
Waiting for 2 worker(s) to connect...

=== Generating Array ===
Generating random array...
Array generated successfully!

=== Server Ready ===
Listening on port 8080...
Waiting for 2 worker(s) to connect...
Connect your worker clients now...
```

### 2. Jalankan Client (di Windows)

```cmd
client.exe <server_ip> 8080
```

Contoh:
```cmd
client.exe 192.168.1.100 8080
```

### 3. Jalankan Client (di Linux VM)

```bash
./client <server_ip> 8080
```

Contoh:
```bash
./client 192.168.1.100 8080
```

### 4. Observasi Hasil

Server akan:
- Membagi array menjadi bagian-bagian untuk setiap worker
- Mengirim task ke masing-masing worker
- Menerima hasil dari workers
- Menggabungkan hasil
- Memverifikasi dengan sequential computation
- Menampilkan statistik performa

## 📊 Contoh Output

### Server Output:
```
=== Distributed Task Execution - Master Server ===
Port: 8080
Maximum Workers: 10

=== Configuration ===
Enter the number of array elements to process: 1000000
Array size set to: 1000000 elements
Estimated memory usage: 3.81 MB

Enter the number of workers needed: 2
Waiting for 2 worker(s) to connect...

=== Generating Array ===
Generating random array...
Array generated successfully!

=== Server Ready ===
Listening on port 8080...
Waiting for 2 worker(s) to connect...
Connect your worker clients now...

[Worker 0] Connected from 192.168.1.50:54321
[Worker 0] Assigned range: 0 to 499999 (500000 elements)
[Worker 0] Sent task: TASK SUM 0 499999
[Worker 0] Received result: 24751234

[Worker 1] Connected from 192.168.1.51:54322
[Worker 1] Assigned range: 500000 to 999999 (500000 elements)
[Worker 1] Sent task: TASK SUM 500000 999999
[Worker 1] Received result: 24748766

=== All workers completed their tasks ===

Parallel Computation Result: 49500000
Sequential Computation Result: 49500000

✓ VERIFICATION SUCCESSFUL! Results match.

=== Summary ===
Total Workers: 2
Array Size: 1000000 elements
Elements per Worker: 500000

Worker Results:
  Worker 0: 24751234
  Worker 1: 24748766
```

### Client Output:
```
=== Distributed Task Execution - Worker Client ===
Platform: Windows
Server: 192.168.1.100:8080

Connecting to server...
Connected to server successfully!
Waiting for task...

Received task: TASK SUM 0 499999
Task: Calculate SUM from index 0 to 499999
Number of elements to process: 500000
Receiving array data...
Array data received successfully!

=== Starting Computation ===
Processing sum of 500000 elements...
Progress: 100%
Computation completed in 0.002341 seconds
Result: 24751234

Sending result to server...
Server acknowledged the result

=== Task Completed Successfully ===
```

## 🔍 Cara Kerja Sistem

1. **Initialization**:
   - User input jumlah elemen array di server
   - User input jumlah worker yang diinginkan
   - Server validate dan generate array random
   - Server listen untuk koneksi dari workers

2. **Connection**:
   - Server menunggu hingga semua worker connect
   - Setiap worker yang connect ditampilkan notifikasi
   - Progress koneksi ditampilkan (misal: 1/2, 2/2)
   - Server tidak akan lanjut sampai semua worker terhubung

3. **Automatic Start**:
   - Workers connect ke server via TCP
   - Server assign worker ID untuk setiap client

3. **Data Partitioning**:
   - Server membagi array berdasarkan jumlah workers
   - Setiap worker mendapat range yang sama besar
   - Contoh: 2 workers → masing-masing 500,000 elemen

4. **Task Distribution**:
   - Server kirim task command: "TASK SUM <start> <end>"
   - Server kirim array data untuk range tersebut
   - Worker receive dan mulai processing

5. **Parallel Processing**:
   - Workers memproses bagian mereka secara simultan
   - Setiap worker kalkulasi sum dari rangenya
   - Processing berjalan secara parallel

6. **Result Collection**:
   - Workers kirim hasil: "RESULT <value>"
   - Server receive hasil dari semua workers
   - Server combine hasil menjadi total akhir

7. **Verification**:
   - Server jalankan sequential computation
   - Bandingkan hasil parallel vs sequential
   - Validasi kebenaran hasil

## 🛠️ Troubleshooting

### Port Already in Use
```bash
# Check port usage
sudo netstat -tulpn | grep 8080

# Kill process using port
sudo kill -9 <PID>
```

### Connection Refused
- Pastikan server sudah running dan menunggu koneksi
- Check IP address server dengan `ifconfig` atau `ipconfig`
- Pastikan firewall tidak blocking port 8080
- Verify koneksi jaringan antara client dan server

### Invalid Array Size Input
- Input harus berupa angka positif
- Maksimum: 100,000,000 elements
- Pastikan memory cukup (setiap 1 juta = ~4 MB)

### Compilation Error (Windows)
- Install MinGW: https://sourceforge.net/projects/mingw/
- Tambahkan MinGW ke PATH environment variable
- Restart terminal setelah install

### File Not Found Error
- Pastikan menggunakan file yang tepat:
  - Linux: `client_linux.c`
  - Windows: `client_windows.c`
- Atau gunakan `client.c` untuk cross-platform (legacy)

## 📈 Performa

Sistem ini mendemonstrasikan:
- **Speedup**: Peningkatan kecepatan dengan parallel processing
- **Scalability**: Dapat handle multiple workers (hingga 10)
- **Efficiency**: Load balancing yang merata
- **Reliability**: TCP menjamin data integrity

## Lisensi

Proyek ini dibuat untuk keperluan akademik - Tugas Arsitektur dan Organisasi Komputer.

Jasiel Fredinand Wowiling