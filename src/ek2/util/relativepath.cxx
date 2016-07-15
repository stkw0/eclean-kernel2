/* eclean-kernel2
 * (c) 2016 Michał Górny
 * 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "ek2/util/relativepath.h"

#include "ek2/util/error.h"

#include <stdexcept>

#include <cassert>
#include <cerrno>

extern "C"
{
#	include <fcntl.h>
#	include <unistd.h>
};

OpenFD::OpenFD(int fd)
	: fd_(fd)
{
}

OpenFD::~OpenFD()
{
	if (fd_ != -1)
		close(fd_);
}

OpenFD::OpenFD(OpenFD&& other)
	: fd_(other.fd_)
{
	other.fd_ = -1;
}

OpenFD& OpenFD::operator=(OpenFD&& other)
{
	fd_ = other.fd_;
	other.fd_ = -1;
	return *this;
}

OpenFD::operator int() const
{
	return fd_;
}

RelativePath::RelativePath(std::shared_ptr<DirectoryStream> dir,
			std::string&& filename)
	: dir_(dir), filename_(filename), file_fd_(-1)
{
	assert(!dir_->path_.empty());
}

std::string RelativePath::filename() const
{
	return filename_;
}

std::string RelativePath::path() const
{
	return dir_->path_ + '/' + filename_;
}

int RelativePath::file_fd(int flags)
{
	if (file_fd_ != -1 && flags != open_mode_)
		throw Error("Reopening not implemented yet");

	if (file_fd_ == -1)
	{
#if defined(HAVE_OPENAT)
		assert(dir_->dir_);
		file_fd_ = openat(dirfd(dir_->dir_), filename_.c_str(), flags);
#else
		file_fd_ = ::open(path().c_str(), flags);
#endif

		if (file_fd_ == -1)
			throw IOError("Unable to open " + path(), errno);

		open_mode_ = flags;
	}

	return file_fd_;
}

struct stat RelativePath::stat() const
{
	int ret;
	struct stat buf;

	if (file_fd_ != -1)
		ret = fstat(file_fd_, &buf);
	else
	{
#if defined(HAVE_FSTATAT)
		assert(dir_->dir_);
		ret = fstatat(dirfd(dir_->dir_), filename_.c_str(), &buf,
				AT_SYMLINK_NOFOLLOW);
#elif defined(HAVE_LSTAT)
		ret = lstat(path().c_str(), &buf);
#else
		ret = ::stat(path().c_str(), &buf);
#endif
	}

	if (ret == -1)
		throw IOError("Unable to stat " + path(), errno);

	return buf;
}
