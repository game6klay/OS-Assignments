

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

static const char *hello_str = "Operating Systems-101!\n1800_VK\n";
static const char *myproc_path = "/myproc";
#define BUFF_SIZE 100000

void dir_listing(const char *name, int level, void *buf, fuse_fill_dir_t filler);
int number_check(const char *p);
char* procInfo_read(const char *filename);

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, myproc_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	} else if (number_check(path)) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(procInfo_read(path));
	} 
	else
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{

	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	char *proc_path = "/proc/";
	dir_listing(proc_path, 0, buf, filler);

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	if (!number_check(path + 1)) return -ENOENT;
	
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

// to fetch the contents of the main directory
void dir_listing(const char *name, int level, void *buf, fuse_fill_dir_t filler) {
    DIR *dir;
    struct dirent *entDir;

    if (!(dir = opendir(name)))
        return;
    if (!(entDir = readdir(dir)))
        return;

    do {
        if (entDir->d_type == DT_DIR) {
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entDir->d_name);
            path[len] = 0;
            if (strcmp(entDir->d_name, ".") == 0 || strcmp(entDir->d_name, "..") == 0)
                continue;

	    // To fetch the  process directory
	    if (number_check(entDir->d_name)) {
		filler(buf, entDir->d_name, NULL, 0);
	    }
           
        }
       
    } while (entDir = readdir(dir));
    closedir(dir);
}

char* procInfo_read(const char *filename) {
    char full_path[100] = "";

    const char *path = "/proc";
    const char *sepreator = "/";
    const char *target = "status";

    strcat(full_path, path);
    strcat(full_path, filename);
    strcat(full_path, sepreator);
    strcat(full_path, target);

 
    char source[BUFF_SIZE + 1];

    FILE *fp = fopen(full_path, "r");
    if (fp != NULL) {
        size_t newLen = fread(source, sizeof(char), BUFF_SIZE, fp);
        if (newLen == 0) {
            fputs("Error reading file", stderr);
        } else {
            source[++newLen] = '\0';
			 /* for the safe side. */
        }

        fclose(fp);
    }

    return source;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	
	if(number_check(path + 1)) {
	    const char *p = procInfo_read(path);

	    len = strlen(p);
	    
	    if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, p + offset, size);
	  	
	    } 
	    else {
		size = 0;
	    }
	    
	    return size;
	}
	else {
	    return 0;
	}
	

}

static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
};


// To check if there are numbers in the string
int number_check(const char *p) {
    while(*p) {
	if (*p != '/' && (*p < '0' || *p > '9')) return 0;
	else p++;
    }
    return 1;
}



int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
