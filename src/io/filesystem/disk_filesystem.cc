/*
 * Copyright (C) 2006-2010 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "disk_filesystem.h"

#include "filesystem_exceptions.h"
#include "wexception.h"
#include "zip_filesystem.h"
#include "log.h"

#include <sys/stat.h>

#include <cassert>
#include <cerrno>

#ifdef WIN32
#include <windows.h>
#include <dos.h>
#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#define S_ISDIR(x) ((x&_S_IFDIR)?1:0)
#endif
#else
#include <sys/mman.h>
#include <glob.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for O_NOATIME
#endif
#endif

#include "io/streamread.h"
#include "io/streamwrite.h"

struct FileSystemPath: public std::string
{
	bool m_exists;
	bool m_isDirectory;

	FileSystemPath(std::string const & path)
	: std::string(path)
	{
		struct stat st;

		m_exists = (stat(c_str(), &st) != -1);
		m_isDirectory = m_exists and S_ISDIR(st.st_mode);
	}
};

/**
 * Initialize the real file-system
 */
RealFSImpl::RealFSImpl(std::string const & Directory)
: m_directory(Directory)
{
	// TODO: check OS permissions on whether the directory is writable!
#ifdef WIN32
	m_root = Directory;
#else
	m_root = "";
	m_root = FS_CanonicalizeName(Directory);
#endif
}


/**
 * Return true if this directory is writable.
 */
bool RealFSImpl::IsWritable() const {
	return true; // should be checked in constructor

	//fweber: no, should be checked here, because the ondisk state might have
	//changed since then
}

/**
 * Returns the number of files found, and stores the filenames (without the
 * pathname) in the results. There doesn't seem to be an even remotely
 * cross-platform way of doing this
 */
// note: the Win32 version may be broken, feel free to fix it
int32_t RealFSImpl::FindFiles
	(const std::string & path,
	 const std::string & pattern,
	 filenameset_t     * results,
	 uint32_t)
{
#ifdef _WIN32
	std::string buf;
	struct _finddata_t c_file;
	long hFile;

	if (path.size())
		buf = m_directory + '\\' + path + '\\' + pattern;
	else
		buf = m_directory + '\\' + pattern;

	hFile = _findfirst(buf.c_str(), &c_file);
	if (hFile == -1)
		return 0;

	std::string realpath = path;

	// Check if path is relative - both \ and / are possibly used depending on the file we work on,
	// so we have to check for both.
	if ((not pattern.compare(0, 3, "../")) || (not pattern.compare(0, 3, "..\\"))) {
		// Workaround: If pattern is a relative we need to fix the path
		std::string m_root_save(m_root); // save orginal m_root
		m_root = FS_CanonicalizeName(path);
		realpath = FS_CanonicalizeName(pattern);
		realpath = realpath.substr(0, realpath.rfind('\\'));
		m_root = m_root_save; // reset m_root
	}

	if (!realpath.empty()) realpath.append("\\");
	do {
		results->insert(realpath + c_file.name);
	} while (_findnext(hFile, &c_file) == 0);

	_findclose(hFile);

	return results->size();
#else
	std::string buf;
	glob_t gl;
	int32_t i, count;
	int32_t ofs;

	if (path.size()) {
		if (pathIsAbsolute(path)) {
			buf = path + '/' + pattern;
			ofs = 0;
		} else {
			buf = m_directory + '/' + path + '/' + pattern;
			ofs = m_directory.length() + 1;
		}
	} else {
		buf = m_directory + '/' + pattern;
		ofs = m_directory.length() + 1;
	}
	if (glob(buf.c_str(), 0, 0, &gl))
		return 0;

	count = gl.gl_pathc;

	for (i = 0; i < count; ++i) {
		results->insert(&gl.gl_pathv[i][ofs]);
	}

	globfree(&gl);

	return count;
#endif
}

/**
 * Returns true if the given file exists, and false if it doesn't.
 * Also returns false if the pathname is invalid (obviously, because the file
 * \e can't exist then)
 * \todo Can this be rewritten to just using exceptions? Should it?
 */
bool RealFSImpl::FileExists(std::string const & path) {
	return FileSystemPath(FS_CanonicalizeName(path)).m_exists;
}

/**
 * Returns true if the given file is a directory, and false if it isn't.
 * Also returns false if the pathname is invalid (obviously, because the file
 * \e can't exist then)
 */
bool RealFSImpl::IsDirectory(std::string const & path) {
	return FileSystemPath(FS_CanonicalizeName(path)).m_isDirectory;
}

/**
 * Create a sub filesystem out of this filesystem
 */
FileSystem & RealFSImpl::MakeSubFileSystem(std::string const & path) {
	FileSystemPath fspath(FS_CanonicalizeName(path));
	assert(fspath.m_exists); //TODO: throw an exception instead
	//printf("RealFSImpl MakeSubFileSystem path %s fullname %s\n",
	//path.c_str(), fspath.c_str());

	if (fspath.m_isDirectory)
		return *new RealFSImpl   (fspath);
	else
		return *new ZipFilesystem(fspath);
}

/**
 * Create a sub filesystem out of this filesystem
 */
FileSystem & RealFSImpl::CreateSubFileSystem
	(std::string const & path, Type const fs)
{
	FileSystemPath fspath(FS_CanonicalizeName(path));
	if (fspath.m_exists)
		throw wexception
			("path %s already exists, can not create a filesystem from it",
			 path.c_str());

	if (fs == FileSystem::DIR) {
		EnsureDirectoryExists(path);
		return *new RealFSImpl(fspath);
	} else
		return *new ZipFilesystem(fspath);
}

/**
 * Remove a number of files
 */
void RealFSImpl::Unlink(std::string const & file) {
	FileSystemPath fspath(FS_CanonicalizeName(file));
	if (!fspath.m_exists)
		return;

	if (fspath.m_isDirectory)
		m_unlink_directory(file);
	else
		m_unlink_file(file);
}

/**
 * Remove a single directory or file
 */
void RealFSImpl::m_unlink_file(std::string const & file) {
	FileSystemPath fspath(FS_CanonicalizeName(file));
	assert(fspath.m_exists);  //TODO: throw an exception instead
	assert(!fspath.m_isDirectory); //TODO: throw an exception instead

#ifndef WIN32
	unlink(fspath.c_str());
#else
	DeleteFile(fspath.c_str());
#endif
}

/**
 * Recursively remove a directory
 */
void RealFSImpl::m_unlink_directory(std::string const & file) {
	FileSystemPath fspath(FS_CanonicalizeName(file));
	assert(fspath.m_exists);  //TODO: throw an exception instead
	assert(fspath.m_isDirectory);  //TODO: throw an exception instead

	filenameset_t files;

	FindFiles(file, "*", &files);

	for
		(filenameset_t::iterator pname = files.begin();
		 pname != files.end();
		 ++pname)
	{
		std::string filename = FS_Filename(pname->c_str());
		if (filename == "..")
			continue;
		if (filename == ".")
			continue;

		if (IsDirectory(*pname))
			m_unlink_directory(*pname);
		else
			m_unlink_file(*pname);
	}

	// NOTE: this might fail if this directory contains CVS dir,
	// so no error checking here
#ifndef WIN32
	rmdir(fspath.c_str());
#else
	RemoveDirectory(fspath.c_str());
#endif
}

/**
 * Create this directory if it doesn't exist, throws an error
 * if the dir can't be created or if a file with this name exists
 */
void RealFSImpl::EnsureDirectoryExists(const std::string & dirname)
{
	try {
		std::string::size_type it = 0;
		while (it < dirname.size()) {
			it = dirname.find(m_filesep, it);

			FileSystemPath fspath(FS_CanonicalizeName(dirname.substr(0, it)));
			if (fspath.m_exists and !fspath.m_isDirectory)
				throw wexception
					("%s exists and is not a directory",
					 dirname.substr(0, it).c_str());
			if (!fspath.m_exists)
				MakeDirectory(dirname.substr(0, it));

			if (it == std::string::npos)
				break;
			++it;
		}
	} catch (const std::exception & e) {
		throw wexception
			("RealFSImpl::EnsureDirectoryExists(%s): %s",
			 dirname.c_str(), e.what());
	}
}

/**
 * Create this directory, throw an error if it already exists or
 * if a file is in the way or if the creation fails.
 *
 * Pleas note, this function does not honor parents,
 * MakeDirectory("onedir/otherdir/onemoredir") will fail
 * if either onedir or otherdir is missing
 */
void RealFSImpl::MakeDirectory(std::string const & dirname) {
	FileSystemPath fspath(FS_CanonicalizeName(dirname));
	if (fspath.m_exists)
		throw wexception
			("a file with the name \"%s\" already exists", dirname.c_str());

	if
		(mkdir
#ifdef WIN32
		 	(fspath.c_str())
#else
		 	(fspath.c_str(), 0x1FF)
#endif
		 ==
		 -1)
		throw DirectoryCannotCreate_error
			("could not create directory %s: %s",
			 dirname.c_str(), strerror(errno));
}

/**
 * Read the given file into alloced memory; called by FileRead::Open.
 * Throws an exception if the file couldn't be opened.
 */
void * RealFSImpl::Load(const std::string & fname, size_t & length) {
	const std::string fullname = FS_CanonicalizeName(fname);

	FILE * file = 0;
	void * data = 0;

	try {
		//debug info
		//printf("------------------------------------------\n");
		//printf("RealFSImpl::Load():\n");
		//printf("     fname       = %s\n", fname.c_str());
		//printf("     m_directory = %s\n", m_directory.c_str());
		//printf("     fullname    = %s\n", fullname.c_str());
		//printf("------------------------------------------\n");

		file = fopen(fullname.c_str(), "rb");
		if (not file)
			throw File_error("RealFSImpl::Load", fullname.c_str());

		// determine the size of the file (rather quirky, but it doesn't require
		// potentially unportable functions)
		fseek(file, 0, SEEK_END);
		size_t size;
		{
			const int32_t ftell_pos = ftell(file);
			if (ftell_pos < 0)
				throw wexception
					("RealFSImpl::Load: error when loading \"%s\" (\"%s\"): file "
					 "size calculation yieded negative value %i",
					 fname.c_str(), fullname.c_str(), ftell_pos);
			size = ftell_pos;
		}
		fseek(file, 0, SEEK_SET);

		// allocate a buffer and read the entire file into it
		data = malloc(size + 1); //  FIXME memory leak!
		int result = fread(data, size, 1, file);
		if (size and (result != 1)) {
			assert(false);
			throw wexception
				("RealFSImpl::Load: read failed for %s (%s) with size %lu",
				 fname.c_str(), fullname.c_str(),
				 static_cast<long unsigned int>(size));
		}
		static_cast<int8_t *>(data)[size] = 0;

		fclose(file);
		file = 0;

		length = size;
	} catch (...) {
		if (file)
			fclose(file);
		free(data);
		throw;
	}

	return data;
}

void * RealFSImpl::fastLoad
	(const std::string & fname, size_t & length, bool & fast)
{
#ifdef _WIN32
	fast = false;
	return Load(fname, length);
#else
	const std::string fullname = FS_CanonicalizeName(fname);
	int file = 0;
	void * data = 0;

#ifdef __APPLE__
	file = open(fullname.c_str(), O_RDONLY);
#elif defined (__FreeBSD__) || defined (__OpenBSD__)
	file = open(fullname.c_str(), O_RDONLY);
#else
	file = open(fullname.c_str(), O_RDONLY|O_NOATIME);
#endif
	length = lseek(file, 0, SEEK_END);
	lseek(file, 0, SEEK_SET);

	data = mmap(0, length, PROT_READ, MAP_PRIVATE, file, 0);

	//if mmap doesn't work for some strange reason try the old way
#pragma GCC diagnostic warning "-Wold-style-cast"
	if (data == MAP_FAILED) {
#pragma GCC diagnostic error "-Wold-style-cast"
		return Load(fname, length);
	}

	fast = true;

	assert(data);
#pragma GCC diagnostic warning "-Wold-style-cast"
	assert(data != MAP_FAILED);
#pragma GCC diagnostic error "-Wold-style-cast"

	close(file);

	return data;
#endif
}


/**
 * Write the given block of memory to the repository.
 * Throws an exception if it fails.
 */
void RealFSImpl::Write
	(std::string const & fname, void const * const data, int32_t const length)
{
	std::string fullname;

	fullname = FS_CanonicalizeName(fname);

	FILE * const f = fopen(fullname.c_str(), "wb");
	if (!f)
		throw wexception
			("could not open %s (%s) for writing",
			 fname.c_str(), fullname.c_str());

	size_t const c = fwrite(data, length, 1, f);
	fclose(f);

	if (length and c != 1) // data might be 0 blocks long
		throw wexception
			("Write to %s (%s) failed", fname.c_str(), fullname.c_str());
}

// rename a file or directory
void RealFSImpl::Rename
	(std::string const & old_name, std::string const & new_name)
{
	const std::string fullname1 = FS_CanonicalizeName(old_name);
	const std::string fullname2 = FS_CanonicalizeName(new_name);
	rename(fullname1.c_str(), fullname2.c_str());
}


/*****************************************************************************

Implementation of OpenStreamRead

*****************************************************************************/

namespace {

struct RealFSStreamRead : public StreamRead {
	RealFSStreamRead(std::string const & fname)
		: m_file(fopen(fname.c_str(), "rb"))
	{
		if (!m_file)
			throw wexception("could not open %s for reading", fname.c_str());
	}

	~RealFSStreamRead()
	{
		fclose(m_file);
	}

	size_t Data(void * const data, size_t const bufsize) {
		return fread(data, 1, bufsize, m_file);
	}

	bool EndOfFile() const
	{
		return feof(m_file);
	}

private:
	FILE * m_file;
};

};

StreamRead * RealFSImpl::OpenStreamRead(std::string const & fname) {
	const std::string fullname = FS_CanonicalizeName(fname);

	return new RealFSStreamRead(fullname);
}


/*****************************************************************************

Implementation of OpenStreamWrite

*****************************************************************************/

namespace {

struct RealFSStreamWrite : public StreamWrite {
	RealFSStreamWrite(std::string const & fname)
		: m_filename(fname)
	{
		m_file = fopen(fname.c_str(), "wb");
		if (!m_file)
			throw wexception("could not open %s for writing", fname.c_str());
	}

	~RealFSStreamWrite() {fclose(m_file);}

	void Data(const void * const data, const size_t size)
	{
		size_t ret = fwrite(data, 1, size, m_file);

		if (ret != size)
			throw wexception("Write to %s failed", m_filename.c_str());
	}

	void Flush()
	{
		fflush(m_file);
	}

private:
	std::string m_filename;
	FILE * m_file;
};

};

StreamWrite * RealFSImpl::OpenStreamWrite(std::string const & fname) {
	const std::string fullname = FS_CanonicalizeName(fname);

	return new RealFSStreamWrite(fullname);
}

unsigned long long RealFSImpl::DiskSpace() {
#ifdef WIN32
	ULARGE_INTEGER freeavailable;
	return
		GetDiskFreeSpaceEx
			(FS_CanonicalizeName(m_directory).c_str(), &freeavailable, 0, 0)
		?
		//if more than 2G free space report that much
		freeavailable.HighPart ? std::numeric_limits<unsigned long>::max() :
		freeavailable.LowPart : 0;
#else
	struct statvfs svfs;
	if (statvfs(FS_CanonicalizeName(m_directory).c_str(), &svfs) != -1) {
		return static_cast<unsigned long long>(svfs.f_bsize) * svfs.f_bavail;
	}
#endif
	return 0; //  can not check disk space
}
