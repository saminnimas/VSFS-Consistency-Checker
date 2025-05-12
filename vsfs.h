#ifndef VSFS_H
#define VSFS_H

#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 64

#define SUPERBLOCK_BLOCK 0
#define INODE_BITMAP_BLOCK 1
#define DATA_BITMAP_BLOCK 2
#define INODE_TABLE_START 3
#define INODE_TABLE_BLOCKS 5
#define DATA_BLOCK_START 8
#define INODE_SIZE 256
#define MAX_INODES ((INODE_TABLE_BLOCKS * BLOCK_SIZE) / INODE_SIZE)
#define MAX_DATA_BLOCKS (TOTAL_BLOCKS - DATA_BLOCK_START)

#define MAGIC_NUMBER 0xD34D

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t first_data_block;
    uint32_t inode_size;
    uint32_t inode_count;
    char reserved[4058];
} Superblock;

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t access_time;
    uint32_t creation_time;
    uint32_t mod_time;
    uint32_t delete_time;
    uint32_t links;
    uint32_t block_count;
    uint32_t direct;
    uint32_t single_indirect;
    uint32_t double_indirect;
    uint32_t triple_indirect;
    char reserved[156];
} Inode;

#pragma pack(pop)

#endif
