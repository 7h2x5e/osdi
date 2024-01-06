#include <include/mbr.h>
#include <include/types.h>
#include <include/string.h>
#include <include/stdio.h>
#include <include/kernel_log.h>

#define SECTOR_SIZE 512
#define MBR_SIGNATURE 0xAA55

__attribute__((unused)) static char get_boot_char(uint8_t b)
{
    switch (b) {
    case 0x00:
        return 'N';
    case 0x80:
        return 'Y';
    default:
        return '?';
    }
}

__attribute__((unused)) static void get_part_type(uint8_t type, char *buf)
{
    switch (type) {
    case 01:
        sprintf(buf, "FAT12");
        break;
    case 04:
        sprintf(buf, "FAT16<32M");
        break;
    case 05:
        sprintf(buf, "Extended");
        break;
    case 06:
        sprintf(buf, "FAT16");
        break;
    case 07:
        sprintf(buf, "NTFS");
        break;
    case 0x0B:
        sprintf(buf, "WIN95 FAT32");
        break;
    case 0x0C:
        sprintf(buf, "WIN95 FAT32 (LBA)");
        break;
    case 0x0E:
        sprintf(buf, "WIN95 FAT16 (LBA)");
        break;
    case 0x0F:
        sprintf(buf, "WIN95 Ext'd (LBA)");
        break;
    case 0x11:
        sprintf(buf, "Hidden FAT12");
        break;
    case 0x14:
        sprintf(buf, "Hidden FAT16<32M");
        break;
    case 0x16:
        sprintf(buf, "Hidden FAT16");
        break;
    case 0x17:
        sprintf(buf, "Hidden NTFS");
        break;
    case 0x1B:
        sprintf(buf, "Hidden WIN95 FAT32");
        break;
    case 0x1C:
        sprintf(buf, "Hidden WIN95 FAT32 (LBA)");
        break;
    case 0x1E:
        sprintf(buf, "Hidden WIN95 FAT16 (LBA)");
        break;
    case 0x82:
        sprintf(buf, "Linux Swap");
        break;
    case 0x83:
        sprintf(buf, "Linux");
        break;
    case 0x85:
        sprintf(buf, "Linux Ext'd");
        break;
    case 0x86:
    case 0x87:
        sprintf(buf, "NTFS Vol. Set");
        break;
    case 0x8E:
        sprintf(buf, "Linux LVM");
        break;
    case 0x9f:
        sprintf(buf, "BSD/OS");
        break;
    case 0xa5:
        sprintf(buf, "FreeBSD");
        break;
    case 0xa6:
        sprintf(buf, "OpenBSD");
        break;
    case 0xa9:
        sprintf(buf, "NetBSD");
        break;
    case 0xeb:
        sprintf(buf, "BeOS fs");
        break;
    case 0xee:
        sprintf(buf, "EFI GPT");
        break;
    case 0xef:
        sprintf(buf, "EFI FAT?");
        break;
    default:
        sprintf(buf, "?");
    }
}

__attribute__((unused)) static void get_human_size(float size, char *buf)
{
    if (!size) {
        buf[0] = 0;
        return;
    }

    char a = 'T';
    if (size < 1024)
        a = 'B';
    else if ((size /= 1024) < 1024.0)
        a = 'K';
    else if ((size /= 1024) < 1024.0)
        a = 'M';
    else if ((size /= 1024) < 1024.0)
        a = 'G';
    else
        size /= 1024.0;

    sprintf(buf, "%f %c", size, a);
}

int parse_mbr(struct mbr *buffer, uint32_t *lba)
{
    if (buffer->mbr_signature != MBR_SIGNATURE)
        return -1;

    struct mbr_partition *partition = &buffer->partition[0];
    if (partition->partition_type == 0x00)
        return -1;

    if (partition->partition_type == 0x05) {
        /* Extended Partition */
        return -1;
    }

    char strbuf[32] = {0};
    get_part_type(partition->partition_type, strbuf);
    KERNEL_LOG_INFO("Boot: %c (%x)\n", get_boot_char(partition->status),
                    partition->status);
    KERNEL_LOG_INFO("Type: %s (%x)\n", strbuf, partition->partition_type);
    KERNEL_LOG_INFO("LBA: 0x%x\n", partition->first_sector_lba);
    get_human_size((float) partition->sectors * 512, strbuf);
    KERNEL_LOG_INFO("Sectors: 0x%x (%s)\n", partition->sectors, strbuf);

    *lba = partition->first_sector_lba;
    return 0;
}