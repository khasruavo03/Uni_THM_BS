#include "vmm.h"
#include "output_utility.h"

#include <stdio.h>
#include <stdbool.h>

/**
 * Initialized a statistics object to zero.
 * @param stats A pointer to an uninitialized statistics object.
 */

#define PAGE_SIZE 256
#define PAGE_ENTRIES 256
#define FRAME_COUNT 64
#define TLB_SIZE 16

typedef struct {
    int page;
    int frame;
    int valid;
} TLBentry;

static void statistics_initialize(Statistics *stats) {
    stats->tlb_hits = 0;
    stats->pagetable_hits = 0;
    stats->total_memory_accesses = 0;
}

Statistics simulate_virtual_memory_accesses(FILE *fd_addresses, FILE *fd_backing) {
    // Initialize statistics
    Statistics stats;
    statistics_initialize(&stats);

    // Physikalischer Speicher
    unsigned char physicalMemory[FRAME_COUNT][PAGE_SIZE];

    // Page Table
    int pageTable[PAGE_ENTRIES];

    // Welcher Frame gehört zu welcher Page?
    int frameToPage[FRAME_COUNT];

    // TLB
    TLBentry tlb[TLB_SIZE];

    // Initialisieren
    for(int i = 0; i < PAGE_ENTRIES; i++) {
        pageTable[i] = -1;
    }

    for (int i = 0; i < FRAME_COUNT; i++) {
        frameToPage[i] = -1;
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = false;
    }

    int nextFrame = 0;
    int nextTLB = 0;

    int logicalAddress;

    while (fscanf(fd_addresses, "%d", &logicalAddress) == 1) {
        stats.total_memory_accesses++;
        int virtualAddress = logicalAddress & 0xFFFF;
        int page = (virtualAddress >> 8) & 0xFF;
        int offset = virtualAddress & 0xFF;

        bool tlb_hit = false;
        bool pt_hit = false;

        int frame = -1;

        // TLB durchsuchen
        for (int i = 0; i < TLB_SIZE; i++) {
            if (tlb[i].valid && tlb[i].page == page) {
                frame = tlb[i].frame;
                tlb_hit = true;
                stats.tlb_hits++;
                stats.pagetable_hits++;

                break;
            }
        }

        // Page Table
        if (!tlb_hit) {
            if (pageTable[page] != -1) {
                frame = pageTable[page];
                pt_hit = true;
                stats.pagetable_hits++;
            } else {
                // Page Fault
                if (nextFrame < FRAME_COUNT) {
                    frame = nextFrame;
                    nextFrame++;
                } else {
                    // FIFO 
                    frame = nextFrame % FRAME_COUNT;
                    nextFrame++;

                    int oldPage = frameToPage[frame];
                    if (oldPage != -1) {
                        pageTable[oldPage] = -1;
                    }

                    // Löschen des alten Frame aus TLB
                    for(int i = 0; i < TLB_SIZE; i++) {
                        if (tlb[i].valid && tlb[i].frame == frame) {
                            tlb[i].valid = false;
                        }
                    }
                }

                // Seiten aus Backing Store laden
                fseek(fd_backing, page * PAGE_SIZE, SEEK_SET);
                fread(physicalMemory[frame], sizeof(unsigned char), PAGE_SIZE, fd_backing);
                pageTable[page] = frame;
                frameToPage[frame] = page;
            }

            // TLB aktualisieren
            tlb[nextTLB].page = page;
            tlb[nextTLB].frame = frame;
            tlb[nextTLB].valid = true;

            nextTLB = (nextTLB + 1) % TLB_SIZE;
        }

        // Physikalische Adressse
        int physicalAddress = frame * PAGE_SIZE + offset;
        unsigned char value = physicalMemory[frame][offset];

        print_access_results(virtualAddress, physicalAddress, value, tlb_hit, pt_hit);
    }
    return stats;
}
