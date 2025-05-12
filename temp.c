#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsfs.h"

extern void read_block(FILE *, int, void *);
extern void read_inode(FILE *, int, Inode *);

void check_superblock(Superblock *sb) {
    printf("Superblock Check:\n");
    if (sb->magic != MAGIC_NUMBER) printf(" - Invalid magic number.\n");
    if (sb->block_size != BLOCK_SIZE) printf(" - Invalid block size.\n");
    if (sb->total_blocks != TOTAL_BLOCKS) printf(" - Total blocks incorrect.\n");
    if (sb->inode_bitmap_block != INODE_BITMAP_BLOCK) printf(" - Wrong inode bitmap block.\n");
    if (sb->data_bitmap_block != DATA_BITMAP_BLOCK) printf(" - Wrong data bitmap block.\n");
    if (sb->inode_table_start != INODE_TABLE_START) printf(" - Wrong inode table start.\n");
    if (sb->first_data_block != DATA_BLOCK_START) printf(" - Wrong data block start.\n");
    if (sb->inode_size != INODE_SIZE) printf(" - Inode size mismatch.\n");
    if (sb->inode_count > MAX_INODES) printf(" - Too many inodes.\n");
    printf("Superblock check completed.\n\n");
}

void load_bitmap(FILE *fp, int block_num, uint8_t *bitmap) {
    read_block(fp, block_num, bitmap);
}

int is_data_block_valid(uint32_t block) {
    return block >= DATA_BLOCK_START && block < TOTAL_BLOCKS;
}

void check_inodes(FILE *fp, uint8_t *inode_bitmap, uint8_t *data_bitmap) {
    printf("Inode Consistency Check:\n");

    int used_data_blocks[MAX_DATA_BLOCKS] = {0};

    for (int i = 0; i < MAX_INODES; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) continue;

        Inode inode;
        read_inode(fp, i, &inode);

        if (inode.links == 0 || inode.delete_time != 0) {
            printf(" - Inode %d is marked used but is invalid.\n", i);
            continue;
        }

        if (!is_data_block_valid(inode.direct)) {
            printf(" - Inode %d direct block is invalid: %u\n", i, inode.direct);
        } else {
            int idx = inode.direct - DATA_BLOCK_START;
            if (used_data_blocks[idx]) {
                printf(" - Duplicate block usage detected: block %u\n", inode.direct);
            }
            used_data_blocks[idx]++;
            if (!(data_bitmap[idx / 8] & (1 << (idx % 8)))) {
                printf(" - Block %u used in inode %d but not marked in data bitmap.\n", inode.direct, i);
            }
        }
    }

    for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
        if ((data_bitmap[i / 8] & (1 << (i % 8))) && !used_data_blocks[i]) {
            printf(" - Block %d marked used in data bitmap but not referenced.\n", DATA_BLOCK_START + i);
        }
    }

    printf("Inode and Data bitmap check completed.\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s vsfs.img\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    Superblock sb;
    read_block(fp, SUPERBLOCK_BLOCK, &sb);
    check_superblock(&sb);

    uint8_t inode_bitmap[BLOCK_SIZE], data_bitmap[BLOCK_SIZE];
    load_bitmap(fp, INODE_BITMAP_BLOCK, inode_bitmap);
    load_bitmap(fp, DATA_BITMAP_BLOCK, data_bitmap);

    check_inodes(fp, inode_bitmap, data_bitmap);

    fclose(fp);
    return 0;
}
