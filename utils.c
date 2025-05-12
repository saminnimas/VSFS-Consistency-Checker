#include "vsfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void read_block(FILE *fp, int block_num, void *buf) {
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fread(buf, BLOCK_SIZE, 1, fp);
}

void write_block(FILE *fp, int block_num, void *buf) {
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(buf, BLOCK_SIZE, 1, fp);
}

void read_inode(FILE *fp, int inode_index, Inode *inode) {
    int block_offset = inode_index * INODE_SIZE;
    int block = INODE_TABLE_START + (block_offset / BLOCK_SIZE);
    int offset = block_offset % BLOCK_SIZE;
    uint8_t buf[BLOCK_SIZE];
    read_block(fp, block, buf);
    memcpy(inode, buf + offset, sizeof(Inode));
}


void load_bitmap(FILE *fp, int block_num, uint8_t *bitmap) {
    read_block(fp, block_num, bitmap);
}

uint32_t read_u32_be(uint8_t *buf, int offset) {
    return ((uint32_t)buf[offset] << 24) |
           ((uint32_t)buf[offset + 1] << 16) |
           ((uint32_t)buf[offset + 2] << 8) |
           ((uint32_t)buf[offset + 3]);
}

uint16_t read_u16_be(uint8_t *buf, int offset) {
    return ((uint16_t)buf[offset] << 8) |
           ((uint16_t)buf[offset + 1]);
}


void write_u32_be(uint8_t *buf, int offset, uint32_t value) {
    buf[offset] = (value >> 24) & 0xFF;
    buf[offset + 1] = (value >> 16) & 0xFF;
    buf[offset + 2] = (value >> 8) & 0xFF;
    buf[offset + 3] = value & 0xFF;
}

void write_u16_be(uint8_t *buf, int offset, uint16_t value) {
    buf[offset] = (value >> 8) & 0xFF;
    buf[offset + 1] = value & 0xFF;
}

void write_superblock(FILE *fp, Superblock *sb) {
    uint8_t buf[BLOCK_SIZE] = {0};

    write_u16_be(buf, 0, sb->magic);
    write_u32_be(buf, 2, sb->block_size);
    write_u32_be(buf, 6, sb->total_blocks);
    write_u32_be(buf, 10, sb->inode_bitmap_block);
    write_u32_be(buf, 14, sb->data_bitmap_block);
    write_u32_be(buf, 18, sb->inode_table_start);
    write_u32_be(buf, 22, sb->first_data_block);
    write_u32_be(buf, 26, sb->inode_size);
    write_u32_be(buf, 30, sb->inode_count);

    fseek(fp, SUPERBLOCK_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(buf, BLOCK_SIZE, 1, fp);
}
