#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    bool valid;
    unsigned long long tag;
    int lru_counter;
} cache_line_t;

typedef struct {
    int s; // set index bits
    int E; // Associativity
    int b; // block bits
    cache_line_t** lines;
} cache_t;

void usage(char* argv[]);
void init_cache(cache_t* cache, int s, int E, int b);
void free_cache(cache_t* cache, int S);
void process_trace_file(cache_t* cache, char* trace_file, bool verbose, int* hits, int* misses, int* evictions);
bool access_cache(cache_t* cache, unsigned long long address, int* hits, int* misses, int* evictions);

int main(int argc, char* argv[]) {
    int opt;
    char* trace_file = NULL;
    bool verbose = false;
    int s = 0, E = 0, b = 0;
    int hits = 0, misses = 0, evictions = 0;
    
    while ((opt = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        switch (opt) {
            case 'v':
                verbose = true;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                usage(argv);
                exit(EXIT_FAILURE);
        }
    }
    
    if (s <= 0 || E <= 0 || b <= 0 || trace_file == NULL) {
        usage(argv);
        exit(EXIT_FAILURE);
    }
    
    cache_t cache;
    init_cache(&cache, s, E, b);
    
    process_trace_file(&cache, trace_file, verbose, &hits, &misses, &evictions);
    
    printSummary(hits, misses, evictions);
    
    int S = 1 << s; // sets
    free_cache(&cache, S);
    
    return 0;
}

void usage(char* argv[]) {
    printf("Usage: %s [-v] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
    printf("  -v: Optional verbose flag\n");
    printf("  -s <s>: Number of set index bits\n");
    printf("  -E <E>: Associativity (number of lines per set)\n");
    printf("  -b <b>: Number of block bits\n");
    printf("  -t <tracefile>: Name of the trace file\n");
}

void init_cache(cache_t* cache, int s, int E, int b) {
    int S = 1 << s; //sets
    
    cache->s = s;
    cache->E = E;
    cache->b = b;
    
    cache->lines = (cache_line_t**)malloc(S * sizeof(cache_line_t*));
    for (int i = 0; i < S; i++) {
        cache->lines[i] = (cache_line_t*)malloc(E * sizeof(cache_line_t));
        for (int j = 0; j < E; j++) {
            cache->lines[i][j].valid = false;
            cache->lines[i][j].tag = 0;
            cache->lines[i][j].lru_counter = 0;
        }
    }
}

void free_cache(cache_t* cache, int S) {
    for (int i = 0; i < S; i++) {
        free(cache->lines[i]);
    }
    free(cache->lines);
}

void process_trace_file(cache_t* cache, char* trace_file, bool verbose, int* hits, int* misses, int* evictions) {
    FILE* fp = fopen(trace_file, "r");
    if (fp == NULL) {
        printf("Error: Cannot open trace file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    char operation;
    unsigned long long address;
    int size;
    char buffer[100];
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (buffer[0] == 'I' || buffer[0] == '\n') {
            continue;
        }
        
        sscanf(buffer, " %c %llx,%d", &operation, &address, &size);
        
        if (verbose) {
            printf("%c %llx,%d ", operation, address, size);
        }
        
        bool hit = access_cache(cache, address, hits, misses, evictions);
        
        if (verbose) {
            if (hit) {
                printf("hit ");
            } else {
                printf("miss ");
            }
        }
        
        if (operation == 'M') {
            hit = access_cache(cache, address, hits, misses, evictions);
            if (verbose) {
                if (hit) {
                    printf("hit ");
                } else {
                    printf("miss ");
                }
            }
        }
        
        if (verbose) {
            printf("\n");
        }
    }
    
    fclose(fp);
}

bool access_cache(cache_t* cache, unsigned long long address, int* hits, int* misses, int* evictions) {
    int s = cache->s;
    int b = cache->b;
    int E = cache->E;
    
    unsigned long long set_index = (address >> b) & ((1 << s) - 1);
    unsigned long long tag = address >> (s + b);
    
    int max_lru = -1;
    int max_lru_index = -1;
    int empty_line = -1;
    
    for (int i = 0; i < E; i++) {
        if (cache->lines[set_index][i].valid) {
            if (cache->lines[set_index][i].lru_counter > max_lru) {
                max_lru = cache->lines[set_index][i].lru_counter;
                max_lru_index = i;
            }
            
            if (cache->lines[set_index][i].tag == tag) {
                (*hits)++;
                cache->lines[set_index][i].lru_counter = 0;
                
                for (int j = 0; j < E; j++) {
                    if (cache->lines[set_index][j].valid && j != i) {
                        cache->lines[set_index][j].lru_counter++;
                    }
                }
                
                return true; // Hit
            }
        } else {
            if (empty_line == -1) {
                empty_line = i;
            }
        }
    }
    
    (*misses)++;
    
    if (empty_line != -1) {
        cache->lines[set_index][empty_line].valid = true;
        cache->lines[set_index][empty_line].tag = tag;
        cache->lines[set_index][empty_line].lru_counter = 0;
        
        for (int j = 0; j < E; j++) {
            if (cache->lines[set_index][j].valid && j != empty_line) {
                cache->lines[set_index][j].lru_counter++;
            }
        }
    } else {
        (*evictions)++;
        cache->lines[set_index][max_lru_index].tag = tag;
        cache->lines[set_index][max_lru_index].lru_counter = 0;
        
        for (int j = 0; j < E; j++) {
            if (j != max_lru_index) {
                cache->lines[set_index][j].lru_counter++;
            }
        }
    }
    
    return false; // Miss
}