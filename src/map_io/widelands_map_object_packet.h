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

#ifndef WIDELANDS_MAP_OBJECT_PACKET_H
#define WIDELANDS_MAP_OBJECT_PACKET_H

#include "logic/instances.h"

#include <set>

struct FileSystem;

namespace Widelands {

struct Editor_Game_Base;
struct Map_Map_Object_Loader;
struct Map_Map_Object_Saver;

/**
 * This data packet contains all \ref Map_Object and derived instances.
 *
 * \note Right now, only those Map_Objects not covered by other objects
 * are in this packet.
 */
struct Map_Object_Packet {
	typedef std::set<Map_Object::Loader *> LoaderSet;
	LoaderSet loaders;

	~Map_Object_Packet();

	void Read (FileSystem &, Editor_Game_Base &, Map_Map_Object_Loader &);

	void LoadFinish();

	void Write(FileSystem &, Editor_Game_Base &, Map_Map_Object_Saver  &);
};

}

#endif
