#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"


#define NUM_ENTRIES_FAT_BLOCK 2048
#define FAT_EOC 0xFFFF

bool DEBUG = false;

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
	int offset;
	struct root_directory_entry *file;
};

struct superblock sb;
struct fat_block *fat;
struct root_directory_entry rd[FS_FILE_MAX_COUNT];
struct file_descriptor fdTable[FS_OPEN_MAX_COUNT];

int mounted = 0;

/* Phase 1 */

int fs_format_check(void)
{
	char sig[8] = {'E', 'C', 'S', '1', '5', '0', 'F', 'S'};
	for (int index = 0; index < 8; index++)
		if (sb.signature[index] != sig[index]){
			perror("incorrect signature");
			return -1;
		}

	int total_amt_blocks = 1 + sb.fat_blocks + 1 + sb.total_data_blocks;
	if (total_amt_blocks != block_disk_count()){
		perror("incorrect total amount of blocks");
		return -1;
	}
		
	return 0;
}

/* Initalize all file descriptor *file pointers to NULL. */
void fdtable_init(void)
{
	for (int fd = 0; fd < FS_OPEN_MAX_COUNT; fd++)
		fdTable[fd].file = NULL;
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

	fdtable_init();

	mounted = 1;

	if (DEBUG)
		fs_info();

	return 0;
}

int fs_umount(void)
{
	if (DEBUG)
		fs_info();

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
	
	// Check if FS not mounted or disk cannot be closed
	if ( !mounted || block_disk_close() == -1)
		return -1;

	// Check if FDs are still open
	for (int fd = 0; fd < FS_OPEN_MAX_COUNT; fd++){
		if (fdTable[fd].file != NULL)
			return -1;
	}


	mounted = 0;

	return 0;
}

/* Returns sum of free entries in File Allocation Tree or first free entry
	depending on value bool 'sum'. */
int fat_free(bool sum)
{
	int count = 0;
	int block_count = 0;

	for (uint8_t fatIndex = 0; fatIndex < sb.fat_blocks; fatIndex++){
		for (int entryIndex = 0; entryIndex < NUM_ENTRIES_FAT_BLOCK; entryIndex++){
			
			// Ignores extra unused FAT blocks
			if (block_count >= sb.total_data_blocks)
				break;
			
			if (fat[fatIndex].fat_entries[entryIndex] == 0){
				if (sum == false)
					return entryIndex + (fatIndex * NUM_ENTRIES_FAT_BLOCK);
				count++;
			}

			block_count++;
		}

	}
	return count;
}

/* Returns either sum of free entries in root directory or first free entry
   depending on value of bool 'sum'. */
int rdir_free(bool sum)
{
	int count = 0;

	for (int entry = 0; entry < FS_FILE_MAX_COUNT; entry++)
		if (rd[entry].filename[0] == '\0'){
			if (sum == false)
				return entry;
			count++;
		}	

	if (sum == false)
		return -1;	
	
	return count;
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
	printf("fat_free_ratio=%d/%d\n", fat_free(true), sb.total_data_blocks);
	printf("rdir_free_ratio=%d/%d\n", rdir_free(true), FS_FILE_MAX_COUNT);

	return 0;
}

/* Phase 2 */

/* Searches for entry in root directory with specific 'filename'. */
int rdir_search(const char *filename)
{
	for (int entry = 0; entry < FS_FILE_MAX_COUNT; entry++)
		if (strcmp((const char *)rd[entry].filename, filename) == 0)
			return entry;	

	return -1;
}

int fs_create(const char *filename)
{
	// No FS currently mounted
	if (!mounted)
		return -1;


	// File name @filename is invalid
	if (strlen(filename) == 0 || strcmp(filename,"\0") == 0)
		return -1;
	
	// File named @filename already exists
	if (rdir_search(filename) != -1)
		return -1;
	
	// String @filename is too long (strlen does not include '\0')
	if (strlen(filename) >= FS_FILENAME_LEN)
		return -1;

	// Filename is not NULL-terminated
	if (filename[strlen(filename)] != '\0')
		return -1;

	// Root directory file limit reached, also setting index if not
	int index;
	if ((index = rdir_free(false)) == -1)
		return -1;

	memcpy(rd[index].filename, filename, FS_FILENAME_LEN);

	rd[index].file_size = 0;
	rd[index].first_data_block_index = FAT_EOC;

	return 0;	
}

int fdtable_search(const char *filename){
	for (int fd = 0; fd < FS_OPEN_MAX_COUNT; fd++)
		if(fdTable[fd].file != NULL && 
		strcmp((const char *)fdTable[fd].file->filename, filename) == 0)
			return fd;

	return -1;
}

int fs_delete(const char *filename)
{
	int rdirIndex;

	// No FS currently mounted
	if (!mounted)
		return -1;
	
	// File name @filename is invalid
	if (strlen(filename) == 0 || strcmp(filename,"\0") == 0)
		return -1;

	// Filename is not NULL-terminated
	if (filename[strlen(filename)] != '\0')
		return -1;

	// File does not exist, also setting rdirIndex if not
	if ((rdirIndex = rdir_search(filename)) == -1)
		return -1;

	// File currently open
	if (fdtable_search(filename) != -1)
		return -1;

	// Delete entries in FAT blocks
	uint16_t blockNum, index, content;
	content = rd[rdirIndex].first_data_block_index;
	while (content != FAT_EOC){
		blockNum = content / (NUM_ENTRIES_FAT_BLOCK+1); // used to be FS_FILE_MAX_COUNT? not sure what i was thinkin
		index = content % (NUM_ENTRIES_FAT_BLOCK+1);

		content = fat[blockNum].fat_entries[index];
		fat[blockNum].fat_entries[index] = 0;
	}

	rd[rdirIndex].filename[0] = '\0';
	rd[rdirIndex].file_size = 0;
	rd[rdirIndex].first_data_block_index = FAT_EOC;

	return 0;
}

int fs_ls(void)
{
	// No FS currently mounted
	if (!mounted)
		return -1;
		
	printf("FS Ls:\n");

	for (int entry = 0; entry < FS_FILE_MAX_COUNT; entry++)
		if(rd[entry].filename[0] != '\0')
			printf("file: %s, size: %d, data_blk: %d\n", rd[entry].filename,
			rd[entry].file_size, 
			rd[entry].first_data_block_index);

	return 0;
}

/* Phase 3 */

/* Searches for free entry in fd table. */
int fdtable_free(void)
{
	for (int fd = 0; fd < FS_OPEN_MAX_COUNT; fd++)
		if(fdTable[fd].file == NULL)
			return fd;

	return -1;
}

int fs_open(const char *filename)
{
	// No FS currently mounted
	if (!mounted)
		return -1;

	int fdNum, rdirIndex;

	// No more than 32 open file descriptors
	if ((fdNum = fdtable_free()) == -1)
		return -1;

	// File name @filename is invalid
	if (strlen(filename) == 0 || strcmp(filename,"\0") == 0)
		return -1;

	// Check File Exists
	if ((rdirIndex = rdir_search(filename)) == -1)
		return -1;

	fdTable[fdNum].offset = 0;
	fdTable[fdNum].file = &rd[rdirIndex];

	return fdNum;
}

int fd_is_valid(int fd)
{
	if (fd >= FS_OPEN_MAX_COUNT || fd < 0 || fdTable[fd].file == NULL)
		return 0;
	return 1;
	
}

int fs_close(int fd)
{
	// No FS currently mounted OR fd invalid
	if (!mounted || !fd_is_valid(fd))
		return -1;

	fdTable[fd].file = NULL;

	return 0;
}

int fs_stat(int fd)
{
	// No FS currently mounted OR fd invalid
	if (!mounted || !fd_is_valid(fd))
		return -1;

	return fdTable[fd].file->file_size;
}

int fs_lseek(int fd, size_t offset)
{
	// No FS currently mounted OR fd invalid
	if (!mounted || !fd_is_valid(fd))
		return -1;

	// Check if @offset is larger than the current file size
	if (offset > fdTable[fd].file->file_size)
		return -1; 

	fdTable[fd].offset = offset;

	return 0;
}


/* Phase 4 */
int fd_offset(int fd)
{
	// No FS currently mounted OR fd invalid
	if (!mounted || !fd_is_valid(fd))
		return -1;

	return fdTable[fd].offset;
}

/* Return index of newly allocated Data block*/
int allocate_block(int *flag)
{
	*flag = 1;

	int freeBlock, FATblockNum, entry;
	
	freeBlock = fat_free(false);
	FATblockNum = freeBlock / (NUM_ENTRIES_FAT_BLOCK); 
	entry = freeBlock % NUM_ENTRIES_FAT_BLOCK;

	fat[FATblockNum].fat_entries[entry] = FAT_EOC;

	return freeBlock;
}

int fs_write(int fd, void *buf, size_t count)
{
	// No FS currently mounted || fd invalid || buf is NULL
	if (!mounted || !fd_is_valid(fd) || buf == NULL)
		return -1;

	// Nothing to Write
	if (count == 0)
		return count;

	if (DEBUG) { printf("starting Write:"); fs_ls(); }

	// Initalize bounce array
	uint8_t bounce[BLOCK_SIZE];

	// Number of bytes written into data blocks (rename to bytesWritten?)
	int bufIndex = 0;

	// Variables
	int increaseSizeFlag = 0;
	uint16_t offset, blockStart, currCount;
	uint16_t FATblockNum, index, content;

	offset = fd_offset(fd);
	blockStart = offset / BLOCK_SIZE;
	currCount = count;
	content = fdTable[fd].file->first_data_block_index;

	if (DEBUG) {printf("offset = %d, blockStart = %d, currCount = %d\n", offset, blockStart, currCount);}

	// If first block is unallocated (file not created yet)
	if (content == FAT_EOC){
		content = allocate_block(&increaseSizeFlag);
		fdTable[fd].file->first_data_block_index = content;
	}

	// Find offset location in data blocks to start write
	for (int i = 0; i < blockStart; i++){
		FATblockNum = content / (NUM_ENTRIES_FAT_BLOCK+1);
		index = content % (NUM_ENTRIES_FAT_BLOCK+1);

		content = fat[FATblockNum].fat_entries[index];
	}

	if (DEBUG) {printf("staring index offset location is : content = %d\n", content);}

	while (currCount != 0){
		FATblockNum = content / (NUM_ENTRIES_FAT_BLOCK+1); // ?? idk if +1 needed
		index = content % (NUM_ENTRIES_FAT_BLOCK+1);

		if (DEBUG) {printf("I am writing to block pointed to by fat[%d].entry[%d]\n", FATblockNum, index);}
		
		// Calculate much to write into this block
		int size = currCount;
		size = (size <= BLOCK_SIZE) ? size : BLOCK_SIZE - (offset % BLOCK_SIZE);

		// Calculate index of data block we will be accessing
		int block = (FATblockNum*4096) + index + sb.data_block_index;

		// Perform write
		if (size == BLOCK_SIZE){
			if (DEBUG) { printf("ABT TO Write1 @block=%d!\n", block); }
			block_write(block, ((uint8_t*)buf)+bufIndex);
		} else {
			if (DEBUG) {  printf("ABT TO Write2 @block=%d!\n", block); }
			block_read(block, &bounce);
			memcpy(((uint8_t*)&bounce)+(offset%BLOCK_SIZE), ((uint8_t*)buf)+bufIndex, size);
			block_write(block, &bounce);
			if (DEBUG) { printf("I wrote : bounce =%s\n", bounce); }
		}
		
		// Update offset, currCount, bufIndex, and size of file
		offset += size;
		currCount -= size;
		bufIndex += size;
		if (increaseSizeFlag == 1) 
			fdTable[fd].file->file_size += size;

		// No more space in disk
		if (fat_free(true) == 0){
			fs_lseek(fd, offset);
			return bufIndex;
		}
				
		// Get next data block number
		if (content == FAT_EOC)
			content = allocate_block(&increaseSizeFlag);
		else 
			content = fat[FATblockNum].fat_entries[index];	
	}

	fs_lseek(fd, offset);
	return bufIndex;	
}

int fs_read(int fd, void *buf, size_t count)
{
	// No FS currently mounted || fd invalid || buf is NULL
	if (!mounted || !fd_is_valid(fd) || buf == NULL)
		return -1;

	// Nothing to Read
	if (count == 0)
		return count;

	if (DEBUG) { printf("starting read:"); fs_ls(); }

	// Initalize bounce array
	uint8_t bounce[BLOCK_SIZE];

	// Number of bytes read into buf (rename to bytesRead?)
	int bufIndex = 0;

	// Variables
	uint16_t offset, blockStart, numBlockRead, currCount;
	uint16_t FATblockNum, index, content;

	offset = fd_offset(fd);
	blockStart = offset / BLOCK_SIZE;
	numBlockRead = ceil((double)count / BLOCK_SIZE);
	currCount = count;
	content = fdTable[fd].file->first_data_block_index;

	if (DEBUG) { printf("offset = %d, blockStart = %d, numBlockRead = %d, currCount = %d\n", offset, blockStart, numBlockRead, currCount); }

	// Find offset location in data blocks to start read
	for (int i = 0; i < blockStart; i++){
		FATblockNum = content / (NUM_ENTRIES_FAT_BLOCK+1);
		index = content % (NUM_ENTRIES_FAT_BLOCK+1);

		content = fat[FATblockNum].fat_entries[index];
	}

	if (DEBUG) { printf("staring index offset loc is : content = %d\n", content); }

	for (int i = 0; i < numBlockRead; i++){
		FATblockNum = content / (NUM_ENTRIES_FAT_BLOCK+1);
		index = content % (NUM_ENTRIES_FAT_BLOCK+1);

		if (DEBUG) { printf("I am reading block pointed to by fat[%d].entry[%d]\n", FATblockNum, index);
		printf("this index is : content = %d\n", content); }

		// Calculate much to read in this block
		int size = currCount;
		size = (size <= BLOCK_SIZE) ? size : BLOCK_SIZE - (offset % BLOCK_SIZE); 

		// Calculate index of data block we will be accessing
		int block = (FATblockNum*4096) + index + sb.data_block_index;

		// Perform read
		if (size == BLOCK_SIZE){
			if (DEBUG) { printf("ABT TO READ1 @index=%d!\n", block); }
			block_read(block, ((uint8_t*)buf)+bufIndex);
		} else {
			if (DEBUG) {  printf("ABT TO READ2 @index=%d!\n", block); }
			block_read(block, &bounce);
			memcpy(((uint8_t*)buf)+bufIndex, ((uint8_t*)&bounce)+(offset%BLOCK_SIZE), size);
		}
		
		// Update offset and currCount
		offset  += size;
		currCount -= size;
		bufIndex += size;

		// Reach EOF before end of buf
		if (content == FAT_EOC){
			fs_lseek(fd, offset);
			return bufIndex;
		}

		// Get next data block number
		content = fat[FATblockNum].fat_entries[index];
	}

	fs_lseek(fd, offset);
	return bufIndex;
}