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
	//char data[26];

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

	/* Error 2 */
	fd = fs_open("");
	ASSERT(fd == -1, "filename invalid handling");

	/* Error 3 */
	fd = fs_open("file2");
	ASSERT(fd == -1, "filename invalid 2 handling");

	/* Open */
	fd = fs_open("file1");
	ASSERT(fd == 0, "fs_open");

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

	/*----------fs_delete() Testing Coverage [Currently 4/5]------------------*/
	printf("----------fs_delete() Testing----------\n");

	/* Error 1 */
	ret = fs_delete("");
	ASSERT(ret == -1, "filename invalid handling");

	/* Error 2*/
	ret = fs_delete("file2");
	ASSERT(ret == -1, "fs_delete filename invalid 2 handling");

	/* Error 3 TODO: File currently open*/
	/*fs_create("file2");
	fd = fs_open("file2");
	fs_close(fd);
	ret = fs_delete("file2");
	ASSERT(ret == -1, "filename open handling");*/

	/* Delete */
	ret = fs_delete("file1");
	ASSERT(ret == 0, "fs_delete");

	/* Unmount */
	ret = fs_umount();
	ASSERT(!ret, "fs_unmount");

	return 0;
}
