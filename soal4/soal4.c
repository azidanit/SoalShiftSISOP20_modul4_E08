#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/xattr.h>

static const char *dirpath = "/home/daffa/Downloads";
static const char *logpath = "/home/daffa/fs.log";


void Log(char *level, char *cmd_desc)
{
  FILE * fp;
  fp = fopen (logpath, "a+");

  time_t rawtime = time(NULL);
  
  struct tm *info = localtime(&rawtime);
  
  char time[100];
  strftime(time, 100, "%y%m%d-%H:%M:%S", info);

  char log[100];
  sprintf(log, "%s::%s::%s\n", level, time, cmd_desc);
  fputs(log, fp);

  fclose(fp);
}


static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char fpath[1000];

	sprintf(fpath, "%s%s", dirpath, path);
	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "LS::%s%s", dirpath, path);
	}
	//Log("INFO",fpath);
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	int res;
	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	char fpath[1000];
	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	char desc[100];
	sprintf(desc, "MKDIR::%s", fpath);
	Log("INFO",fpath);
	int res;

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	char desc[100];
	sprintf(desc, "REMOVE::%s", fpath);
	Log("WARNING", desc);
	int res;

	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	int res;
	char desc[100];
	sprintf(desc, "REMOVE::%s", fpath);
	Log("WARNING", desc);
	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;
	char new_from[1000];
	char new_to[1000];
	sprintf(new_from,"%s%s",dirpath,from);
	sprintf(new_to,"%s%s",dirpath,to);
	res = symlink(new_from, new_to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;
	char new_from[1000];
	char new_to[1000];
	sprintf(new_from,"%s%s",dirpath,from);
	sprintf(new_to,"%s%s",dirpath,to);
	char desc[100];
	sprintf(desc, "RENAME::%s::%s", new_from,new_to);
	Log("INFO", desc);
	res = rename(new_from, new_to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;
	char new_from[1000];
	char new_to[1000];
	sprintf(new_from,"%s%s",dirpath,from);
	sprintf(new_to,"%s%s",dirpath,to);
	res = link(new_from, new_to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	char desc[100];
	sprintf(desc, "CHMOD::%s", fpath);
	Log("INFO", desc);
	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	char desc[100];
	sprintf(desc, "CHOWN::%s", fpath);
	Log("INFO", desc);
	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	char desc[100];
	sprintf(desc, "CAT::%s", fpath);
	Log("INFO", desc);
	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	(void) fi;
	fd = open(fpath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

	(void) fi;
	char fpath[1000];

	if (!strcmp(path, "/"))
	{
		path = dirpath;
		sprintf(fpath, "%s", path);
	}
	else
	{
		sprintf(fpath, "%s%s", dirpath, path);
	}
	char desc[100];
	sprintf(desc, "Create::%s", fpath);
	Log("INFO", desc);
	int res;
	res = creat(fpath, mode);
	if(res == -1)
		return -errno;

	close(res);

	return 0;
}


static struct fuse_operations xmp_oper = {
	.getattr        = xmp_getattr,
	.access         = xmp_access,
	.readlink       = xmp_readlink,
	.readdir        = xmp_readdir,
	.mknod          = xmp_mknod,
	.mkdir          = xmp_mkdir,
	.symlink        = xmp_symlink,
	.unlink         = xmp_unlink,
	.rmdir          = xmp_rmdir,
	.rename         = xmp_rename,
	.link           = xmp_link,
	.chmod          = xmp_chmod,
	.chown          = xmp_chown,
	.truncate       = xmp_truncate,
	.utimens        = xmp_utimens,
	.open           = xmp_open,
	.read           = xmp_read,
	.write          = xmp_write,
	.statfs         = xmp_statfs,
	.create         = xmp_create,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}