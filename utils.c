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
