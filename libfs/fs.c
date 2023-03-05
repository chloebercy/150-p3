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
struct __attribute__((__packed__)) root_directory{
	char 		filename[FS_FILENAME_LEN];
	uint32_t 	file_size;
	uint16_t 	first_data_block_index;
	uint8_t 	padding[10];

};

struct superblock sb;
// Dynamically allocate FAT later because we don't know it until superblock
struct fat_block* fat;
struct root_directory rd;

// To track whether FS is currently mounted
int mounted = 0;

int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname) == -1){
		return -1;
	}

	// Fill out Superblock
	block_read(0, &sb);

	/* !!!Need to implement error checking here I think!!!*/

	// Allocate FAT array and fill it out
	fat = (struct fat_block*)malloc(sizeof(struct fat_block) * sb.fat_blocks);
	int fatIndex = 1;
	while (fatIndex <= sb.fat_blocks){
		block_read(fatIndex, &fat[fatIndex-1]);
		fatIndex++;
	}

	// Fill out Root directory
	block_read(fatIndex, &rd);


	/* ! I feel like there is definitely more to do here that I am missing ! */


	mounted = 1;
	return 0;
}

int fs_umount(void)
{
	// Also need to check if still open FDs, but not implemented as of Phase 1
	if (!mounted || block_disk_close() == -1){
		return -1;
	}


	mounted = 0;
	return 0;
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

	/* Not implemented */
	printf("fat_free_ratio=%d/%d\n", 0, 0);
	printf("rdir_free_ratio=%d/%d\n", 0, 0);

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

