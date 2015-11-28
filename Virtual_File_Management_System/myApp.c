#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 80
char tempBuf[BUFFER_SIZE];

//#include "p6cyuan.c"

int i;
size_t nbyte;
off_t offset;
off_t length;
int fda, fdb, fdc, fdd, fde; // 5 file descriptors

int main() {

  make_fs("mydk");
  mount_fs("mydk");

  fs_create("fa");
  fda = fs_open("fa");
  char bufa[] = "The goal of this project";
  nbyte = 24;
  fs_write(fda,bufa,nbyte);

  fs_create("fb");
  fdb = fs_open("fb");
  char bufb[] = "CS6456 is fun";
  nbyte = 13;
  fs_write(fdb,bufb,nbyte);

  fs_create("fc");
  fdc = fs_open("fc");
  char bufc[] = "I love computer programming because I can ask computers to do something for me";
  nbyte = 78;
  fs_write(fdc,bufc,nbyte);

  fs_create("fd");
  fdd = fs_open("fd");
  char bufd[] = "Hi, there";
  nbyte = 9;
  fs_write(fdd,bufd,nbyte);

  fs_create("fe");

  length = fs_get_filesize(fdc);
  printf("file size of file fc = %d\n", (int) length); // file size of file fc = 78

  nbyte = 20;

  offset = -24;
  fs_lseek(fda, offset);
  fs_read(fda, tempBuf, nbyte); 
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // The goal of this pro
  printf("\n");

  offset = -58;
  fs_lseek(fdc, offset);   
  nbyte = 10;
  fs_read(fdc, tempBuf, nbyte);
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // ramming be
  printf("\n");

  length = 30;
  fs_truncate(fdc, length);
  nbyte = 30;
  fs_read(fdc, tempBuf, nbyte); 
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // I love computer programming be
  printf("\n");
  
  fs_close(fdb);

  fde = fs_open("fe");
  char bufe[] = "I am glad this is the last project for the semester";
  nbyte = 51;
  fs_write(fde,bufe,nbyte);

  fs_delete("fb");

  dismount_fs("mydk");

  mount_fs("mydk");

  fda = fs_open("fa");
  fdb = fs_open("fb");
  printf("status of file fb = %d\n",fdb); // status of file fb = -1
  fdc = fs_open("fc");
  fdd = fs_open("fd");
  fde = fs_open("fe");

  nbyte = 24;
  fs_read(fda, tempBuf, nbyte);
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // The goal of this project
  printf("\n");

  nbyte = 30;
  fs_read(fdc, tempBuf, nbyte);
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // I love computer programming be
  printf("\n");

  nbyte = 9;
  fs_read(fdd, tempBuf, nbyte);
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // Hi, there
  printf("\n");

  nbyte = 51;
  fs_read(fde, tempBuf, nbyte);
  for (i = 0; i < nbyte; i++)
	  printf("%c", tempBuf[i]); // I am glad this is the last project for the semester
  printf("\n");

  dismount_fs("mydk");
 
  return 0;
}

 
