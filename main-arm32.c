/*
 * Gather data points of data access time (in nanoseconds)
 * on a ARM 32-Bit system using random array index accesses
*/
#include <time.h> 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define OUT_FILE "output.csv"
#define RUNS 1

#define DATA_SIZE_BYTES 1000000
#define INT_COUNT (DATA_SIZE_BYTES / sizeof(int))

#ifndef __ARM_NR_cacheflush
#define __ARM_NR_cacheflush (0x900000 + 2)
#endif

/*
 * Get Time in Nanoseconds
 * Uses CLOCK_MONOTONIC for ARMv7 compatibility.
 */
static inline uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/*
 * Uses kernel syscall to flush the stack memory range.
 */
static inline void flush_memory_range(void *addr, size_t size) {
    syscall(__ARM_NR_cacheflush, addr, (char *)addr + size, 0);
}

/*
 * Perform a memory access and return the time (in nanoseconds)
 * of how long the access took
*/
uint64_t access_memory(volatile int *addr, int index) {
    uint64_t start, end;

    start = get_time_ns();

    // The actual memory access
    int value = addr[index];
    (void)value; 

    end = get_time_ns();

    return (end - start);
}

/*
 * Write test records to file
*/
void write_records_to_file(char** records, int size_of_records) {
    FILE *fp = fopen(OUT_FILE, "a");
    
    if (fp == NULL) {
        fprintf(stderr, "Error: Failed to open file %s\n", OUT_FILE);
        return;
    }
    
    for (int i = 0; i < size_of_records; i++) {
        fprintf(fp, "%s", records[i]);
    }
    fclose(fp);
}

/*
 * The run_through_array function randomly accesses memory.
 */
char** run_through_array(int* arr, int size, int memory_accesses) {
    uint64_t duration;
    
    char** records = malloc(memory_accesses * sizeof(char*));
    if (records == NULL) {
        perror("Failed to allocate memory for records array");
        exit(1);
    }

    for (int i = 0; i < memory_accesses; i++) {
        int num = rand() % size; 
        
        duration = access_memory(arr, num);

        records[i] = malloc(64 * sizeof(char)); 
        if (records[i] == NULL) {
             perror("Failed to allocate memory for string");
             exit(1);
        }

        snprintf(records[i], 64, "arr[%d],%llu\n", num, (unsigned long long)duration);
    }

    return records;
}

/*
 * Setup test CSV file
*/
void init_file() {
    remove(OUT_FILE);
    FILE *fp = fopen(OUT_FILE, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Failed to open file %s\n", OUT_FILE);
        return; 
    }
    fprintf(fp, "access-location,nanoseconds\n");
    fclose(fp);
}

/*
 * Deallocate memory for records
*/
void free_records(char** records, int size) {
    for (int i = 0; i < size; i++) {
        free(records[i]);
    }
    free(records);
}

int main() {
    srand(time(NULL));

    int data[INT_COUNT];
    
    for (int i = 0; i < INT_COUNT; i++) {
        data[i] = 0;
    }

    init_file();

    for (int i = 0; i < RUNS; i++) {
        // Flush the stack array from cache
        flush_memory_range(data, DATA_SIZE_BYTES);

        char** results = run_through_array(data, INT_COUNT, INT_COUNT);
        
        write_records_to_file(results, INT_COUNT);
        
        free_records(results, INT_COUNT);
    }
    
    return 0;
}