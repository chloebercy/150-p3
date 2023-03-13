#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs.h>

#define ASSERT(cond, func)                               \
do {                                                     \
	if (!(cond)) {                                       \
		printf("'%s' FAILED\n", func); 					 \
		exit(1);     									 \
	} else {											 \
		printf("'%s' SUCCESS\n", func); 				 \
	}                                 					 \
} while (0)

int main(int argc, char *argv[])
{
	int ret;
	char *diskname;
	int fd;
	char data[FS_OPEN_MAX_COUNT+1][FS_FILENAME_LEN] = {"a", "b", "c", "d", "e",
		"f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", 
		"t", "u", "v", "w", "x", "y", "z", "1", "2", "3", "4", "5", "6", "7"};

	if (argc < 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");


	/*----------fs_create() Testing Coverage [Currently 4/6]------------------*/
	printf("----------fs_create() Testing----------\n");

	/* Error 1 */
	ret = fs_create("");
	ASSERT(ret == -1, "filename invalid handling");

	/* Error 2 */
	ret = fs_create("1234567890123456");
	ASSERT(ret == -1, "filename too long handling");

	/* Error 3 TODO: Null terminated check*/

	/* Error 4 TODO: space in root directory check*/

	/* Create */
	ret = fs_create("file1");
	ASSERT(ret == 0, "fs_create");

	/* Error 5*/
	ret = fs_create("file1");
	ASSERT(ret == -1, "filename repeat handling");

	/*----------fs_open() Testing Coverage [Currently 3/4]------------------*/
	printf("----------fs_open() Testing----------\n");

	/* Error 1 TODO: FD Table full */
	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if ((ret = fs_create(data[i])) == -1){printf("internal error\n");}
		if ((ret = fs_open(data[i])) == -1){printf("internal error\n");}
	}
	if ((ret = fs_create(data[FS_OPEN_MAX_COUNT])) == -1){printf("internal error\n");}

	fd = fs_open(data[FS_OPEN_MAX_COUNT]);

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if ((ret = fs_close(i)) == -1){printf("internal error\n");}
		if ((ret = fs_delete(data[i])) == -1){printf("internal error\n");}
	}

	ASSERT(fd == -1, "fdtable full handling");

	/* Error 2 */
	fd = fs_open("");
	ASSERT(fd == -1, "filename invalid handling");

	/* Error 3 */
	fd = fs_open("file2");
	ASSERT(fd == -1, "filename invalid 2 handling");

	/* Open */
	fd = fs_open("file1");
	ASSERT(fd == 0, "fs_open");

	/*----------fs_stat() Testing Coverage [Currently 4/4]------------------*/
	printf("----------fs_stat() Testing----------\n");

	/* Error 1 */
	ret = fs_stat(33);
	ASSERT(ret == -1, "fd invalid handling");

	/* Error 2 */
	ret = fs_stat(-2);
	ASSERT(ret == -1, "fd invalid 2 handling");

	/* Error 3 */
	ret = fs_stat(2);
	ASSERT(ret == -1, "fd invalid 3 handling");

	/* Get Size */
	ret = fs_stat(fd);
	ASSERT(!ret, "fs_stat");

	/*----------fs_lseek() Testing Coverage [Currently 4/4]------------------*/
	printf("----------fs_lseek() Testing----------\n");

	/* Error 1 */
	ret = fs_lseek(33, 0);
	ASSERT(ret == -1, "fd invalid handling");

	/* Error 2 */
	ret = fs_lseek(-2, 0);
	ASSERT(ret == -1, "fd invalid 2 handling");

	/* Error 3 */
	ret = fs_lseek(2, 0);
	ASSERT(ret == -1, "fd invalid 3 handling");

	/* Error 4 */
	ret = fs_lseek(fd, 1);
	ASSERT(ret == -1, "@offset too large handling");

	/* Change Offset */
	ret = fs_lseek(fd, 0);
	ASSERT(!ret, "fs_lseek");

	/*----------fs_close() Testing Coverage [Currently 4/4]------------------*/
	printf("----------fs_close() Testing----------\n");

	/* Error 1 */
	ret = fs_close(32);
	ASSERT(ret == -1, "fd invalid handling");

	/* Error 2 */
	ret = fs_close(-1);
	ASSERT(ret == -1, "fd invalid 2 handling");

	/* Error 3 */
	ret = fs_close(2);
	ASSERT(ret == -1, "fd invalid 3 handling");

	/* Close */
	ret = fs_close(fd);
	ASSERT(ret == 0, "fd_close");

	/*----------fs_delete() Testing Coverage [Currently 5/5]------------------*/
	printf("----------fs_delete() Testing----------\n");

	/* Error 1 */
	ret = fs_delete("");
	ASSERT(ret == -1, "filename invalid handling");

	/* Error 2*/
	ret = fs_delete("file2");
	ASSERT(ret == -1, "filename invalid 2 handling");

	/* Error 3 */
	fs_create("file2");
	fd = fs_open("file2");
	ret = fs_delete("file2");
	ASSERT(ret == -1, "filename open handling");
	fs_close(fd);
	fs_delete("file2");

	/* Delete */
	ret = fs_delete("file1");
	ASSERT(ret == 0, "fs_delete");

	/* Unmount */
	ret = fs_umount();
	ASSERT(!ret, "fs_unmount");

	return 0;
}
