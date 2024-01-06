#ifndef _FAT_H
#define _FAT_H

/* https://wiki.osdev.org/FAT */
// https://superuser.com/questions/1406112/in-fat-file-systems-why-does-the-data-area-start-at-cluster-2
// https://www.cs.fsu.edu/~cop4610t/lectures/project3/Week11/Slides_week11.pdf
#include <include/types.h>
#include <include/list.h>

#define LFN_MARK 0x0F
#define FAT_LAST (-1)
#define SECTOR_SIZE 512

enum sfn_desc {
    NOT_IN_USE = 0x00,
    INIT_CHARACTER = 0x05,
    DOT_ENTRY = 0x2e,
    ERASED = 0xe5
};

enum sfn_attr {
    READ_ONLY = 0x00,
    HIDDEN = 0x02,
    SYSTEM = 0x04,
    VOLUME_LABEL = 0x08,
    SUBDIRECTORY = 0x10,
    ARCHIVE = 0x20,
    DEVICE = 0x40,
    UNUSED = 0x80
};

struct hh_mm_ss {
    uint16_t hours : 5,  // 0-23
        minutes : 6,     // 0-59
        seconds : 5;     // 0-29
} __attribute__((packed));

struct yy_mm_dd {
    uint16_t year : 7,  // 0=1980, 127=2107
        month : 4,      // 1-12
        day : 5;        // 1-31
} __attribute__((packed));

struct date {
    struct hh_mm_ss hhmmss;
    struct yy_mm_dd yymmdd;
};

typedef struct _short_file_name {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t reserved[1];
    uint8_t ms;  // 10ms unit, for finer resolution
    struct date create_time;
    struct {
        struct yy_mm_dd yymmdd;
    } last_access_time;
    uint16_t high_cluster_num;
    struct date last_modified_time;
    uint16_t low_cluster_num;
    uint32_t size;
} __attribute__((packed)) sfn_t; /* 32 bytes */

typedef struct _long_file_name {
    uint8_t order;
    uint16_t name1[5];  // little endian
    uint8_t attr;       // always 0x0F, LFN attribute
    uint8_t type;       // zero for name entries
    uint8_t chksum;
    uint16_t name2[6];
    uint16_t zero;  // always 0x0000
    uint16_t name3[2];
} __attribute__((packed)) lfn_t; /* 32 bytes */

struct bios_parameter_block {
    uint8_t jmpInstruction[3];
    uint8_t oemIdentifier[8];
    uint16_t numBytesPerSector;
    uint8_t numSectorsPerCluster;
    uint16_t numReservedSectors;
    uint8_t numFats;
    uint16_t numRootDirectoryEntries;
    uint16_t numSectors;
    uint8_t mediaType;
    uint16_t numSectorsPerFat;  // FAT12/FAT16 only
    uint16_t numSectorsPerTrack;
    uint16_t numHeads;
    uint32_t numHiddenSectors;
    uint32_t numLargeSectors;
} __attribute__((packed)); /* 36 bytes */

struct extended_bios_parameter_block {
    uint32_t sectorsPerFat;
    uint16_t flags;
    uint16_t fatVersion;
    uint32_t clusterNumRootDirectory;
    uint16_t sectorNumFSInfo;
    uint16_t sectorNumBackupBoot;
    uint8_t reserved[12];
    uint8_t driveNum;
    uint8_t ntFlags[1];
    uint8_t signature;  // 0x28/0x29
    uint32_t volumeIdSerialNum;
    uint8_t volumeLabel[11];
    uint8_t systemIdentifier[8];  // "FAT32   "
} __attribute__((packed));        /* 54 bytes */

struct FSInfo {
    uint32_t leadSignature;
    uint8_t reserved1[480];
    uint32_t signature;  // 0x61417272
    uint32_t numFreeCluster;
    uint32_t clusterNumFreeCluster;
    uint8_t reserved2[12];
    uint32_t trailSignature;  // 0xAA550000
} __attribute__((packed));    /* 512 bytes */

struct boot_record {
    struct bios_parameter_block bpb;
    struct extended_bios_parameter_block ebpb;
    uint8_t boot_code[420];
    uint16_t partition_signature;  // 0xAA55
} __attribute__((packed));         /* 512 bytes */

struct fat_entry {
    char *long_name;
    void *address;
    uint8_t attributes;
    uint64_t size;
    uint32_t cluster;
    uint8_t data[32];
    struct date creation_date, change_date;
    struct list_head head;
};

extern uint32_t bytesPerSector, sectorsPerCluster, bytesPerCluster, fatStart,
    rootCluster, dataStart;
uint32_t clusterAddress(uint32_t cluster, bool isRoot);
unsigned int nextCluster(unsigned int cluster);
void readData(uint32_t address, char *buffer, int size);
void writeData(uint32_t address, char *buffer, int size);
void get_entries(uint8_t *cluster, uint32_t size, struct list_head *head);
char *get_entry_short_filename(struct fat_entry entry);

#endif