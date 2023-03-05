#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */


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
	uint16_t 	data_block[NUM_ENTRIES_FAT_BLOCK];
};

/* Root directory data structure */
typedef struct __attribute__((__packed__)) root_directory_entry{
	char 		filename[FS_FILENAME_LEN];
	uint32_t 	file_size;
	uint16_t 	first_data_block_index;
	uint8_t 	padding[10];

} rdir_entry;

struct __attribute__((__packed__)) root_directory{
	rdir_entry	rdir_entries[NUM_ENTRIES_ROOT_DIRECTORY];
};

struct superblock sb;
struct fat_block *fat;
struct root_directory rd;

int mounted = 0;

int fs_mount(const char *diskname)
{
	// Open Virtual Disk
	if (block_disk_open(diskname) == -1){
		return -1;
	}

	// Read Metadata - Superblock
	if (block_read(0, &sb) == -1)
		return -1;
	
	// Read Metadata - File Allocation Table
	fat = malloc(sizeof(struct fat_block) * sb.fat_blocks);
	for (int index = 1; index <= sb.fat_blocks; index++){
		if (block_read(index, &fat[index-1]) == -1)
			return -1;
	}

	// Read Metadata - Root Directory
	if (block_read(sb.fat_blocks+1 , &rd) == -1)
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
	for (int index = 1; index <= sb.fat_blocks; index++){
		if (block_write(index, &fat[index-1]) == -1)
			return -1;
	} 

	// Write Metadata to Disk - Root Directory
	if (block_write(sb.fat_blocks+1, &rd) == -1)
		return -1;

	// Also need to check if still open FDs, but not implemented as of Phase 1
	if (!mounted || block_disk_close() == -1){
		return -1;
	}

	mounted = 0;

	return 0;
}

int fat_free(void)
{
	int count = 0;

	for (uint8_t fatIndex = 0; fatIndex < sb.fat_blocks; fatIndex++){
		for (int entryIndex = 0; entryIndex < NUM_ENTRIES_FAT_BLOCK; entryIndex++){
			if (fat[fatIndex].data_block[entryIndex] == 0)
				count ++;
		}
	} 

	return count;
}

int rdir_free(void)
{
	int count = 0;

	for (int entry = 0; entry < NUM_ENTRIES_ROOT_DIRECTORY; entry++){
		if (rd.rdir_entries[0].filename[0] == '\0')
			count++;
	}

	return count;
}

int fs_info(void)
{
	if (block_disk_count() == -1){
		return -1;
	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sb.total_blocks);
	printf("fat_blk_count=%d\n", sb.fat_blocks);
	printf("rdir_blk=%d\n", sb.fat_blocks + 1);
	printf("data_blk=%d\n", sb.fat_blocks + 2);
	printf("data_blk_count=%d\n", sb.total_data_blocks);
	printf("fat_free_ratio=%d/%d\n", fat_free(), sb.total_data_blocks);
	printf("rdir_free_ratio=%d/%d\n", rdir_free(), NUM_ENTRIES_ROOT_DIRECTORY);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */


	// temp code - just to compile
	if (filename)
		return 0;
	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */

	// temp code - just to compile
	if (filename)
		return 0;
	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */


	// temp code - just to compile
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

