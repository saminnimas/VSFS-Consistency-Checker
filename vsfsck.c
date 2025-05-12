#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vsfs.h"

extern void read_block(FILE *, int, void *);
extern void write_block(FILE *, int, void *);
extern void read_inode(FILE *, int, Inode *);
extern void load_bitmap(FILE *, int, uint8_t *);
extern uint16_t read_u16_be(uint8_t *, int);
extern uint32_t read_u32_be(uint8_t *, int);
extern void write_superblock(FILE *, Superblock *);

int used_data_blocks[MAX_DATA_BLOCKS] = {0};

void check_superblock(FILE *fp, Superblock *sb, int fix_mode) {
    int modified = 0;
    printf("Superblock Check:\n");

    if (sb->magic != MAGIC_NUMBER) {
        printf(" - Invalid magic number (expected 0x%X, found 0x%X).", MAGIC_NUMBER, sb->magic);
        if (fix_mode) { sb->magic = MAGIC_NUMBER; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->block_size != BLOCK_SIZE) {
        printf(" - Invalid block size (%u).", sb->block_size);
        if (fix_mode) { sb->block_size = BLOCK_SIZE; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->total_blocks != TOTAL_BLOCKS) {
        printf(" - Total blocks incorrect (%u).", sb->total_blocks);
        if (fix_mode) { sb->total_blocks = TOTAL_BLOCKS; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->inode_bitmap_block != INODE_BITMAP_BLOCK) {
        printf(" - Wrong inode bitmap block (%u).", sb->inode_bitmap_block);
        if (fix_mode) { sb->inode_bitmap_block = INODE_BITMAP_BLOCK; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->data_bitmap_block != DATA_BITMAP_BLOCK) {
        printf(" - Wrong data bitmap block (%u).", sb->data_bitmap_block);
        if (fix_mode) { sb->data_bitmap_block = DATA_BITMAP_BLOCK; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->inode_table_start != INODE_TABLE_START) {
        printf(" - Wrong inode table start (%u).", sb->inode_table_start);
        if (fix_mode) { sb->inode_table_start = INODE_TABLE_START; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->first_data_block != DATA_BLOCK_START) {
        printf(" - Wrong data block start (%u).", sb->first_data_block);
        if (fix_mode) { sb->first_data_block = DATA_BLOCK_START; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->inode_size != INODE_SIZE) {
        printf(" - Inode size mismatch (%u).", sb->inode_size);
        if (fix_mode) { sb->inode_size = INODE_SIZE; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (sb->inode_count > MAX_INODES) {
        printf(" - Too many inodes (%u).", sb->inode_count);
        if (fix_mode) { sb->inode_count = MAX_INODES; printf(" Fixed.\n"); modified = 1; } else printf("\n");
    }

    if (fix_mode && modified) {
        write_superblock(fp, sb);
        printf("Superblock updated with fixes.\n");
    }

    printf("Superblock check completed.\n\n");
}

int is_data_block_valid(uint32_t block) {
    return block >= DATA_BLOCK_START && block < TOTAL_BLOCKS;
}

void check_and_fix_inodes(FILE *fp, uint8_t *inode_bitmap, uint8_t *data_bitmap, int fix_mode) {
    printf("Inode Consistency Check:\n");

    for (int i = 0; i < MAX_INODES; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) continue;

        Inode inode;
        read_inode(fp, i, &inode);

        int valid = inode.links > 0 && inode.delete_time == 0;
        if (!valid) {
            printf(" - Inode %d marked used but is invalid. ", i);
            if (fix_mode) {
                inode_bitmap[i / 8] &= ~(1 << (i % 8));
                printf("Fixed.\n");
            } else printf("\n");
            continue;
        }

        if (!is_data_block_valid(inode.direct)) {
            printf(" - Inode %d direct block is invalid: %u. ", i, inode.direct);
            if (fix_mode) {
                inode.direct = 0;
                fseek(fp, INODE_TABLE_START * BLOCK_SIZE + i * INODE_SIZE, SEEK_SET);
                fwrite(&inode, sizeof(Inode), 1, fp);
                printf("Fixed.\n");
            } else printf("\n");
            continue;
        }

        if (inode.direct) {
            int idx = inode.direct - DATA_BLOCK_START;
            if (used_data_blocks[idx]) {
                printf(" - Duplicate block usage: block %u. ", inode.direct);
                if (fix_mode) {
                    inode.direct = 0;
                    fseek(fp, INODE_TABLE_START * BLOCK_SIZE + i * INODE_SIZE, SEEK_SET);
                    fwrite(&inode, sizeof(Inode), 1, fp);
                    printf("Fixed.\n");
                } else printf("\n");
                continue;
            }

            used_data_blocks[idx]++;

            if (!(data_bitmap[idx / 8] & (1 << (idx % 8)))) {
                printf(" - Inode %d uses block %u not marked in bitmap. ", i, inode.direct);
                if (fix_mode) {
                    data_bitmap[idx / 8] |= (1 << (idx % 8));
                    printf("Fixed.\n");
                } else printf("\n");
            }
        }
    }

    for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
        if ((data_bitmap[i / 8] & (1 << (i % 8))) && used_data_blocks[i] == 0) {
            printf(" - Block %d marked used but not referenced. ", DATA_BLOCK_START + i);
            if (fix_mode) {
                data_bitmap[i / 8] &= ~(1 << (i % 8));
                printf("Fixed.\n");
            } else printf("\n");
        }
    }

    printf("Inode and data block check completed.\n\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s [-f] vsfs.img\n", argv[0]);
        return 1;
    }

    int fix_mode = 0;
    char *filename = NULL;
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        fix_mode = 1;
        filename = argv[2];
    } else {
        filename = argv[1];
    }

    FILE *fp = fopen(filename, "rb+");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    uint8_t buf[BLOCK_SIZE];
    read_block(fp, SUPERBLOCK_BLOCK, buf);

    Superblock sb;
    sb.magic = read_u16_be(buf, 0);
    sb.block_size = read_u32_be(buf, 2);
    sb.total_blocks = read_u32_be(buf, 6);
    sb.inode_bitmap_block = read_u32_be(buf, 10);
    sb.data_bitmap_block = read_u32_be(buf, 14);
    sb.inode_table_start = read_u32_be(buf, 18);
    sb.first_data_block = read_u32_be(buf, 22);
    sb.inode_size = read_u32_be(buf, 26);
    sb.inode_count = read_u32_be(buf, 30);

    check_superblock(fp, &sb, fix_mode);

    uint8_t inode_bitmap[BLOCK_SIZE], data_bitmap[BLOCK_SIZE];
    load_bitmap(fp, INODE_BITMAP_BLOCK, inode_bitmap);
    load_bitmap(fp, DATA_BITMAP_BLOCK, data_bitmap);

    check_and_fix_inodes(fp, inode_bitmap, data_bitmap, fix_mode);

    if (fix_mode) {
        write_block(fp, INODE_BITMAP_BLOCK, inode_bitmap);
        write_block(fp, DATA_BITMAP_BLOCK, data_bitmap);
        // printf("\n%X\n", sb.magic);
        printf("All fixes applied and written to image.\n\n");
        printf("Re-checking filesystem after fixes:\n\n");
        char *recheck_argv[] = { argv[0], filename, NULL };
        execv(argv[0], recheck_argv);
        perror("Re-execution failed");
    }

    fclose(fp);
    return 0;
}
