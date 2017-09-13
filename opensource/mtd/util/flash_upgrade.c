
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>

int main(int argc,char *argv[])
{
	const char *filename = NULL,*devname = NULL;
	int i;
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	struct stat filestat;
	unsigned long erase_begin,erase_sect_begin;
	unsigned long erase_end,erase_sect_end;
	unsigned long preImageSize,postImageSize;
	int dev_fd,file_fd;
	char *preImage = NULL, *postImage = NULL, *fileImage = NULL;
	int blocks = 0;

	if(argc != 5){
		printf("Usage : flash_upgrade device srcFile flStartLoc flEndLoc\n");
		return 1;
	}

	devname = argv[1];
	dev_fd = open(devname,O_SYNC | O_RDWR);
	if(dev_fd < 0){
		printf("The device %s could not be opened\n",devname);
		return 1;
	}

	/* get some info about the flash device */
	if (ioctl (dev_fd,MEMGETINFO,&mtd) < 0){
		printf("%s This doesn't seem to be a valid MTD flash device!\n",devname);
		return 1;
	}

	filename = argv[2];
	file_fd = open(filename,O_SYNC | O_RDONLY);
	if(file_fd < 0){
		printf("The file %s could not be opened\n",filename);
		return 1;
	}

	/* does it fit into the device/partition? */
	fstat (file_fd,&filestat);

	/* does it fit into the device/partition? */
	if (filestat.st_size > mtd.size)
	{
		printf("%s won't fit into %s!\n",filename,devname);
		return 1;
	}

	erase_begin = strtoul(argv[3],NULL,16);
	erase_end = strtoul(argv[4],NULL,16);
	if(erase_begin >= mtd.size || erase_end >= mtd.size || erase_end <= erase_begin){
		printf("Erase begin 0x%08lx or Erase end 0x%08lx are out of boundary of mtd size 0x%08lx",erase_begin,erase_end,mtd.size);
		return 1;
	}

	erase_sect_begin = erase_begin & ~(mtd.erasesize - 1);
	erase_sect_end = erase_end & ~(mtd.erasesize - 1);
	erase_sect_end = erase_sect_end + mtd.erasesize;

	preImageSize = erase_begin - erase_sect_begin - 1;
	if(preImageSize > 0){
		printf("Saving %d data as erase_begin 0x%08lx is not on sector boundary 0x%08lx\n",preImageSize,erase_begin,erase_sect_begin);
		preImage = (char *)calloc(preImageSize + 1,sizeof(char));
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_sect_begin,SEEK_CUR);
		read(dev_fd,preImage,preImageSize);
	}

	postImageSize = erase_sect_end	- erase_end;
	if(postImageSize > 0){
		printf("Saving %d data as erase_end 0x%08lx is not on sector boundary 0x%08lx\n",postImageSize,erase_end,erase_sect_end);
		postImage = (char *)calloc(postImageSize + 1,sizeof(char));
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_end,SEEK_CUR);
		read(dev_fd,postImage,postImageSize);
	}
#if 1
	blocks = (erase_sect_end - erase_sect_begin) / mtd.erasesize;
	erase.length = mtd.erasesize;
	erase.start = erase_sect_begin;
	for(i = 0; i < blocks; i++){
		printf ("Erasing blocks: %d/%d \n",i,blocks);
		if (ioctl (dev_fd,MEMERASE,&erase) < 0)
		{
			printf ("While erasing blocks 0x%.8x-0x%.8x on %s: %m\n",
					(unsigned int) erase.start,(unsigned int) (erase.start + erase.length),devname);
			return 1;
		}
		erase.start += mtd.erasesize;
	}


	if(preImageSize > 0){
		printf("Writing back %d data as erase_begin 0x%08lx is not on sector boundary 0x%08lx\n",preImageSize,erase_begin,erase_sect_begin);
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_sect_begin,SEEK_CUR);
		preImageSize = write(dev_fd,preImage,preImageSize);
		printf("Wrote back at 0x%08lx size %d\n",erase_sect_begin,preImageSize);
		free(preImage);
	}

	fileImage = (char *)calloc(filestat.st_size + 1,sizeof(char));
	read(file_fd,fileImage,filestat.st_size);
	lseek(dev_fd,0L,SEEK_SET);
	lseek(dev_fd,erase_begin,SEEK_CUR);
	write(dev_fd,fileImage,filestat.st_size);
	free(fileImage);

	if(postImageSize > 0){
		printf("Writing back %d data as erase_end 0x%08lx is not on sector boundary 0x%08lx\n",postImageSize,erase_end,erase_sect_end);
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_end,SEEK_CUR);
		postImageSize = write(dev_fd,postImage,postImageSize);
		printf("Wrote back at 0x%08lx size %d\n",erase_end,postImageSize);
		free(postImage);
	}
#else
	free(preImage);
	free(postImage);
#endif
	close(dev_fd);
	close(file_fd);
	return 0;
}
