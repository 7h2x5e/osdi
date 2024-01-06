#include <include/fat.h>
#include <include/list.h>
#include <include/xstr.h>
#include <include/types.h>
#include <include/stdio.h>
#include <include/string.h>
#include <include/slab.h>
#include <include/sd.h>

#define get_first_sector_of_cluster(sectors_per_cluster, first_data_sector, \
                                    cluster)                                \
    (((cluster - 2) * sectors_per_cluster) + first_data_sector)
#define FAT_LONG_NAME_LAST 0x40

static unsigned char longFilePos[] = {30, 28, 24, 22, 20, 18, 16,
                                      14, 9,  7,  5,  3,  1};

uint32_t bytesPerSector, sectorsPerCluster, bytesPerCluster, fatStart,
    rootCluster, dataStart;

uint32_t clusterAddress(uint32_t cluster, bool isRoot)
{
    if (!isRoot) {
        cluster -= 2;
    }
    uint32_t addr = (dataStart + bytesPerCluster * cluster);
    return addr;
}

static void parse_lfn(uint8_t *buffer, xstr_t *letters)
{
    lfn_t *lfn = (lfn_t *) buffer;

    if (lfn->attr != LFN_MARK || lfn->zero != 0x0000)
        return;

    if (lfn->order == FAT_LONG_NAME_LAST && lfn->order != ERASED) {
        xstr_clear(letters);
    }

    for (int i = 0; i < sizeof(longFilePos); i++) {
        uint8_t c = buffer[longFilePos[i]];
        uint8_t d = buffer[longFilePos[i] + 1];
        if (c != 0 && c != 0xff) {
            char tmp[2] = {0};
            if (d != 0x00) {
                tmp[0] = d;
                xstr_append(letters, tmp);
            }
            tmp[0] = c;
            xstr_append(letters, tmp);
        }
    }
}

static int isprint(int c)
{
    return (unsigned) c - 0x20 < 0x5f;
}

char *get_entry_short_filename(struct fat_entry entry)
{
    uint8_t *raw = entry.data;
    char *str = (char *) kmalloc(12);
    int i, j = 0;

    // unused space filled with '0x20'
    for (i = (str[0] == ERASED) ? 1 : 0; i < 8 && raw[i] != 0x20; i++) {
        str[j++] = raw[i];  // base
    }
    if (str[j] != 0x20) {
        str[j++] = '.';
        for (i = 8; i < 11 && raw[i] != 0x20; i++) {
            str[j++] = raw[i];  // ext
        }
    }
    str[j] = 0;
    return str;
}

static char *get_entry_long_filename(struct fat_entry entry)
{
    char *str = kmalloc(strlen(entry.long_name) + 1);
    strcpy(str, entry.long_name);
    return str;
}

static char *get_entry_filename(struct fat_entry entry)
{
    if (strlen(entry.long_name) != 0) {
        return get_entry_long_filename(entry);
    } else {
        return get_entry_short_filename(entry);
    }
}

static bool is_correct_entry(struct fat_entry entry)
{
    if (entry.attributes && !(entry.attributes & SUBDIRECTORY) &&
        !(entry.attributes & ARCHIVE)) {
        return false;
    }

    char *str = get_entry_filename(entry);
    if (entry.attributes == SUBDIRECTORY && entry.cluster == 0 &&
        strcmp(str, "..") != 0) {
        kfree(str);
        return false;
    }
    kfree(str);

    for (int i = 1; i < 11; i++) {
        if (isprint(entry.data[i])) {
            return true;
        }
    }
    return false;
}

void get_entries(uint8_t *cluster, uint32_t size, struct list_head *head)
{
    int local_found = 0;
    int local_bad_entries = 0;
    xstr_t *letters = xstr_create(0);
    INIT_LIST_HEAD(head);

    for (uint32_t i = 0; i < size; i += 32) {
        uint8_t *address = cluster + i;
        sfn_t *sfn = (sfn_t *) address;

        if (sfn->name[0] == NOT_IN_USE) {
            continue;
        } else if (sfn->attr == LFN_MARK) {
            parse_lfn(address, letters);
        } else {
            int i;
            bool valid = false;
            struct fat_entry *entry =
                (struct fat_entry *) kmalloc(sizeof(struct fat_entry));

            entry->attributes = sfn->attr;
            entry->address = address;
            entry->long_name = xstr_get(xstr_reverse(letters));
            entry->size = sfn->size;
            entry->cluster =
                (sfn->high_cluster_num << 16) | sfn->low_cluster_num;
            entry->creation_date = sfn->create_time;
            entry->change_date = sfn->last_modified_time;
            INIT_LIST_HEAD(&(entry->head));
            memcpy(entry->data, address, 32);

            for (i = 0; i < 32 && entry->data[i] == 0; i++)
                ;
            if (i != 32) {
                if (is_correct_entry(*entry)) {
                    list_add_tail(&(entry->head), head);
                    valid = true;
                    local_found++;
                } else {
                    local_bad_entries++;
                }
            }

            if (!valid) {
                kfree(entry->long_name);
                kfree(entry);
            }
        }
    }
    xstr_destroy(letters);
}

void readData(uint32_t address, char *buffer, int size)
{
    char tmp[SECTOR_SIZE];
    int idx, offset, toRead;
    do {
        idx = address / SECTOR_SIZE;
        offset = address % SECTOR_SIZE;
        toRead = MIN(SECTOR_SIZE - offset, size);

        readblock(idx, tmp);
        memcpy(buffer, tmp + offset, toRead);

        buffer += toRead;
        address += toRead;
        size -= toRead;
    } while (size > 0);
}

void writeData(uint32_t address, char *buffer, int size)
{
    char tmp[SECTOR_SIZE];
    int idx, offset, toWrite;
    do {
        idx = address / SECTOR_SIZE;
        offset = address % SECTOR_SIZE;
        toWrite = MIN(SECTOR_SIZE - offset, size);

        readblock(idx, tmp);
        memcpy(tmp + offset, buffer, toWrite);
        writeblock(idx, tmp);

        buffer += toWrite;
        address += toWrite;
        size -= toWrite;
    } while (size > 0);
}

unsigned int nextCluster(unsigned int cluster)
{
    /* Although FAT32 uses 32 bits per FAT entry, only the bottom 28 bits are
     * actually used to address clusters on the disk */
    uint32_t fatEntry;
    readData(fatStart + (32 * cluster) / 8, (char *) &fatEntry,
             sizeof(fatEntry));
    fatEntry &= 0x0fffffff;
    return (fatEntry >= 0x0ffffff0) ? FAT_LAST : fatEntry;
}
