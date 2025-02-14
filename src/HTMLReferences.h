/*
 * Copyright (C) 2008 by the Widelands Development Team
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

#ifndef HTML_REFERENCERS_H
#define HTML_REFERENCERS_H

#include "writeHTML.h"

#ifdef WRITE_GAME_DATA_AS_HTML

#include "logic/widelands.h"

#include <map>
#include <set>
#include <string>

struct HTMLReferences {
	enum Role {Input, Output, Madeof, Become, Employ, End};
	std::set<std::string> const & operator[] (size_t const i) const {
		assert(i < End);
		return references[i];
	}
	std::set<std::string>       & operator[] (size_t const i)       {
		assert(i < End);
		return references[i];
	}
	std::set<std::string> references[End]; /// indexed by Referencing_Role
};

#endif

#endif
