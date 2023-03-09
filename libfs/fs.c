#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"


#define NUM_ENTRIES_FAT_BLOCK 2048
#define FAT_EOC 0xFFFF
#define NUM_ENTRIES_ROOT_DIRECTORY 128

/* Superblock data structure */
struct __attribute__((__packed__)) superblock{
	uint8_t 	signature[8];
	uint16_t 	total_blocks;
	uint16_t 	root_dir_index;
	uint16_t 	data_block_index;
	uint16_t 	total_data_blocks;
	uint8_t 	fat_blocks;
	uint8_t		padding[4079];

};

/* FAT block data structure */
struct __attribute__((__packed__)) fat_block{
	uint16_t 	fat_entries[NUM_ENTRIES_FAT_BLOCK];
};

/* Root directory data structure */
struct __attribute__((__packed__)) root_directory_entry{
	uint8_t  	filename[FS_FILENAME_LEN];
	uint32_t 	file_size;
	uint16_t 	first_data_block_index;
	uint8_t 	padding[10];

};

struct file_descriptor{
	size_t offset;
	int file_descriptor;	
};

struct superblock sb;
struct fat_block *fat;
struct root_directory_entry rd[NUM_ENTRIES_ROOT_DIRECTORY];

int mounted = 0;

int fs_format_check(void)
{
	char sig[8] = {'E', 'C', 'S', '1', '5', '0', 'F', 'S'};
	for (int index = 0; index < 8; index++){
		if (sb.signature[index] != sig[index]){
			perror("incorrect signature");
			return -1;
		}
	}

	int total_amt_blocks = 1 + sb.fat_blocks + 1 + sb.total_data_blocks;
	if (total_amt_blocks != block_disk_count()){
		perror("incorrect total amount of blocks");
		return -1;
	}
		
	return 0;
}

int fs_mount(const char *diskname)
{
	// Open Virtual Disk
	if (block_disk_open(diskname) == -1)
		return -1;

	// Read Metadata - Superblock
	if (block_read(0, &sb) == -1)
		return -1;
	
	// Read Metadata - File Allocation Table
	fat = malloc(sizeof(struct fat_block) * sb.fat_blocks);
	for (int index = 1; index <= sb.fat_blocks; index++)
		if (block_read(index, &fat[index-1]) == -1)
			return -1;

	// Read Metadata - Root Directory
	if (block_read(sb.fat_blocks+1 , &rd) == -1)
		return -1;

	// Check for Proper Format 
	if (fs_format_check() == -1)
		return -1;

	mounted = 1;

	return 0;
}

int fs_umount(void)
{
	// Write Metadata to Disk - Superblock
	if (block_write(0, &sb) == -1)
		return -1;

	// Write Metadata to Disk - File Allocation Table
	for (int index = 1; index <= sb.fat_blocks; index++)
		if (block_write(index, &fat[index-1]) == -1)
			return -1;

	// Write Metadata to Disk - Root Directory
	if (block_write(sb.fat_blocks+1, &rd) == -1)
		return -1;

	// Also need to check if still open FDs, but not implemented as of Phase 1
	if ( !mounted || block_disk_close() == -1)
		return -1;

	mounted = 0;

	return 0;
}

/* Returns sum of free entries in File Allocation Tree. */
int fat_free(void)
{
	int count = 0;

	for (uint8_t fatIndex = 0; fatIndex < sb.fat_blocks; fatIndex++){
		for (int entryIndex = 0; entryIndex < NUM_ENTRIES_FAT_BLOCK; entryIndex++)
			if (fat[fatIndex].fat_entries[entryIndex] == 0)
				count ++;
	} 

	return count;
}

/* Returns either sum of free entries in root directory or first free entry
   depending on value of bool 'total'. */
int rdir_free(bool sum)
{
	int count = 0;

	for (int entry = 0; entry < NUM_ENTRIES_ROOT_DIRECTORY; entry++){
		if (rd[entry].filename[0] == '\0'){
			if (sum == false)
				return entry;
			count++;
		}	
	}

	if (sum == false)
		return -1;	
	
	return count;
}

/* Searches for entry in root directory with specific 'filename'. */
int rdir_search(const char *filename)
{
	for (int entry = 0; entry < NUM_ENTRIES_ROOT_DIRECTORY; entry++)
		if (strcmp((const char *)rd[entry].filename, filename) == 0)
			return entry;	

	return -1;
}

int fs_info(void)
{
	if (block_disk_count() == -1)
		return -1;

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sb.total_blocks);
	printf("fat_blk_count=%d\n", sb.fat_blocks);
	printf("rdir_blk=%d\n", sb.fat_blocks + 1);
	printf("data_blk=%d\n", sb.fat_blocks + 2);
	printf("data_blk_count=%d\n", sb.total_data_blocks);
	printf("fat_free_ratio=%d/%d\n", fat_free(), sb.total_data_blocks);
	printf("rdir_free_ratio=%d/%d\n", rdir_free(true), NUM_ENTRIES_ROOT_DIRECTORY);

	return 0;
}

int fs_create(const char *filename)
{
	// No FS currently mounted
	if (!mounted){
		return -1;
	}
	// File named @filename already exists
	if (rdir_search(filename) != -1){
		return -1;
	}
	// String @filename is too long (strlen does not include '/0')
	if (strlen(filename) >= FS_FILENAME_LEN){
		return -1;
	}

	int index;
	// Root directory file limit reached, also setting index if not
	if ((index = rdir_free(false)) == -1){
		perror("no available space");
		return -1;
	}

	memcpy(rd[index].filename, filename, FS_FILENAME_LEN);

	rd[index].file_size = 0;
	rd[index].first_data_block_index = FAT_EOC;

	// Update the disk - Not sure if this may be needed?
	// return block_write(sb.root_dir_index, &rd);

	// return 0;	
}

int fs_delete(const char *filename)
{
	int rdirIndex;

	// No FS currently mounted
	if (!mounted){
		return -1;
	}
	// File @filename is currently open
	// I don't think this will work yet, we need P3 implemented first
	// if (){
	// 	return -1;
	// }

	// File does not exist, also setting rdirIndex if not
	if ((rdirIndex = rdir_search(filename)) == -1){
		perror("file does not exist");
		return -1;
	}	

	// Only need this portion if it's not already empty
	if (rd[rdirIndex].first_data_block_index != FAT_EOC){
		int dataIndex;
		uint16_t entry;
		uint16_t dataIndexNext;
		dataIndex = rd[rdirIndex].first_data_block_index / NUM_ENTRIES_FAT_BLOCK;
		entry = rd[rdirIndex].first_data_block_index % NUM_ENTRIES_FAT_BLOCK;
		while (fat[dataIndex].fat_entries[entry] != FAT_EOC){
			dataIndexNext = fat[dataIndex].fat_entries[entry];
			fat[entry].fat_entries[dataIndex] = 0;
			dataIndex = dataIndexNext / NUM_ENTRIES_FAT_BLOCK;
			entry = dataIndexNext % NUM_ENTRIES_FAT_BLOCK;
		}
	}

	rd[rdirIndex].filename[0] = '\0';
	rd[rdirIndex].file_size = 0;
	rd[rdirIndex].first_data_block_index = FAT_EOC;

	// Need to potentially do block_write again
	
	return 0;
}

int fs_ls(void)
{
	printf("FS Ls:\n");

	for (int entry = 0; entry < NUM_ENTRIES_ROOT_DIRECTORY; entry++){
		if(rd[entry].filename[0] != '\0')
			printf("file: %s, size: %d, data_blk: %d\n", rd[entry].filename,
			rd[entry].file_size, 
			rd[entry].first_data_block_index);
	}

	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */


	// temp code - just to compile
	if (filename)
		return 0;
	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */


	// temp code - just to compile
	if (fd)
		return 0;
	return 0;	
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */


	// temp code - just to compile
	if (fd)
		return 0; 
	return 0;	
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	

	// temp code - just to compile
	if (fd && offset)
		return 0;
	return 0;	
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	// temp code - just to compile
	if (fd && buf && count)
		return 0;
	return 0;	
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */


	// temp code - just to compile
	if (fd && buf && count)
		return 0; 
	return 0;
}

