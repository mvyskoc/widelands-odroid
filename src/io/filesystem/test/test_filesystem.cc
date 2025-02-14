/*
 * Copyright (C) 2007-2008, 2010 by the Widelands Development Team
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

#include <exception>
#include <boost/test/unit_test.hpp>

#include "io/filesystem/disk_filesystem.h"

// BOOST_CHECK_EQUAL generates an old-style cast usage warning, so ignore
#pragma GCC diagnostic ignored "-Wold-style-cast"

BOOST_AUTO_TEST_SUITE(FileSystemTests)

#define TEST_CANONICALIZE_NAME(root, path, expected)                          \
   BOOST_CHECK_EQUAL(RealFSImpl(root).FS_CanonicalizeName(path), expected);   \

BOOST_AUTO_TEST_CASE(test_canonicalize_name) {
	setenv("HOME", "/home/test", 1);
	std::string cwd = RealFSImpl("").getWorkingDirectory();

	// RealFSImpl is constructed with a root directory...

	TEST_CANONICALIZE_NAME("", "path", cwd + "/path");
	TEST_CANONICALIZE_NAME(".", "path", cwd + "/path");

	TEST_CANONICALIZE_NAME("/home", "path", "/home/path");
	TEST_CANONICALIZE_NAME("/opt", "path", "/opt/path");
	TEST_CANONICALIZE_NAME("/opt/test", "path", "/opt/test/path");
	TEST_CANONICALIZE_NAME("/opt", "some/path", "/opt/some/path");

	// single dot is removed (root path)...

	TEST_CANONICALIZE_NAME("./home/me", "path", cwd + "/home/me/path");
	TEST_CANONICALIZE_NAME("/home/./you", "path", "/home/you/path");
	TEST_CANONICALIZE_NAME("/home/us/.", "path", "/home/us/path");

	// single dot is removed (file path)...

	TEST_CANONICALIZE_NAME("/opt", "./no/where", "/opt/no/where");
	TEST_CANONICALIZE_NAME("/opt", "some/./where", "/opt/some/where");
	TEST_CANONICALIZE_NAME("/opt", "any/where/.", "/opt/any/where");

	// empty path nodes are removed (root path)...

	TEST_CANONICALIZE_NAME("//usr/empty", "path", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr//empty", "path", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr/empty/", "path", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr/empty//", "path", "/usr/empty/path");

	// empty path nodes are removed (file path)...

	TEST_CANONICALIZE_NAME("/usr", "/empty/path", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr", "//empty/path", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr", "empty//path", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr", "empty/path/", "/usr/empty/path");
	TEST_CANONICALIZE_NAME("/usr", "empty/path//", "/usr/empty/path");

	// '..' moves up a directory in the path (root path)...

	TEST_CANONICALIZE_NAME("/usr/../home", "path", "/home/path");
	TEST_CANONICALIZE_NAME("/usr/../../home", "path", "/home/path");
	TEST_CANONICALIZE_NAME("/usr/test/..", "path", "/usr/path");
	TEST_CANONICALIZE_NAME("/usr/test/../..", "path", cwd + "/path");
	TEST_CANONICALIZE_NAME("/usr/test/../../..", "path", cwd + "/path");
	TEST_CANONICALIZE_NAME("/usr/one/../two/..", "path", "/usr/path");
	TEST_CANONICALIZE_NAME("/usr/one/../a/b/..", "path", "/usr/a/path");

	// '..' moves up a directory in the path (file path)...

	TEST_CANONICALIZE_NAME("/home/test", "../path", "/home/path");
	TEST_CANONICALIZE_NAME("/home/test", "../../path", "/path");
	TEST_CANONICALIZE_NAME("/home/test", "../../../path", "/path");

	TEST_CANONICALIZE_NAME("/home/test", "path/..", "/home/test");
	TEST_CANONICALIZE_NAME("/home/test", "path/../..", "/home");
	TEST_CANONICALIZE_NAME("/home/test", "path/../../..", "");
	TEST_CANONICALIZE_NAME("/home/test", "path/../../../..", "");

	TEST_CANONICALIZE_NAME("/home/test", "path/../one", "/home/test/one");
	TEST_CANONICALIZE_NAME("/home/test", "path/../../one", "/home/one");
	TEST_CANONICALIZE_NAME("/home/test", "path/../../../one", "/one");
	TEST_CANONICALIZE_NAME("/home/test", "path/../../../../one", "/one");

	// ...but not a '..' coming from two different strings...

	TEST_CANONICALIZE_NAME("/home/test/.", "./path", "/home/test/path");
}

// ~ gets expanded to $HOME
BOOST_AUTO_TEST_CASE(test_canonicalize_name_home_expansion) {
	setenv("HOME", "/my/home", 1);
	std::string cwd = RealFSImpl("").getWorkingDirectory();

	TEST_CANONICALIZE_NAME("~", "path", "/my/home/path");
	TEST_CANONICALIZE_NAME("~/test", "path", "/my/home/test/path");

	setenv("HOME", "/somewhere", 1);
	TEST_CANONICALIZE_NAME("~", "path", "/somewhere/path");

	// ~ at the start of the path overrides the root

	TEST_CANONICALIZE_NAME("~", "~", "/somewhere");
	TEST_CANONICALIZE_NAME("~/vanish", "~", "/somewhere");
	TEST_CANONICALIZE_NAME("~/fs", "~/sf", "/somewhere/sf");

	// ~ anywhere other than at the start of a path does not get expanded

	TEST_CANONICALIZE_NAME("/opt/~", "path", "/opt/~/path");
	TEST_CANONICALIZE_NAME("/opt/~/the", "path", "/opt/~/the/path");

	TEST_CANONICALIZE_NAME("/opt", "path/~", "/opt/path/~");
	TEST_CANONICALIZE_NAME("/opt", "path/~/here", "/opt/path/~/here");

	// ~ as part of a root-path spec segment name does not get expanded

	TEST_CANONICALIZE_NAME("~a", "path", cwd + "/~a/path");
	TEST_CANONICALIZE_NAME("a~", "path", cwd + "/a~/path");

	TEST_CANONICALIZE_NAME("/opt/~a", "path", "/opt/~a/path");
	TEST_CANONICALIZE_NAME("/opt/a~", "path", "/opt/a~/path");

	TEST_CANONICALIZE_NAME("~a/b", "path", cwd + "/~a/b/path");
	TEST_CANONICALIZE_NAME("a~/b", "path", cwd + "/a~/b/path");

	TEST_CANONICALIZE_NAME("/opt/~the/test", "path", "/opt/~the/test/path");
	TEST_CANONICALIZE_NAME("/opt/the~/test", "path", "/opt/the~/test/path");

	// ~ as part of a path spec segment name does not get expanded

	TEST_CANONICALIZE_NAME("/opt", "~path", "/opt/~path");
	TEST_CANONICALIZE_NAME("/opt", "path~", "/opt/path~");

	TEST_CANONICALIZE_NAME("/opt", "some/~path", "/opt/some/~path");
	TEST_CANONICALIZE_NAME("/opt", "some/path~", "/opt/some/path~");

	TEST_CANONICALIZE_NAME("/opt", "~path/here", "/opt/~path/here");
	TEST_CANONICALIZE_NAME("/opt", "path~/here", "/opt/path~/here");

	TEST_CANONICALIZE_NAME("/opt", "a/~path/here", "/opt/a/~path/here");
	TEST_CANONICALIZE_NAME("/opt", "a/path~/here", "/opt/a/path~/here");
}

BOOST_AUTO_TEST_SUITE_END()

