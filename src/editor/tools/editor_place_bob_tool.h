/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#ifndef EDITOR_PLACE_BOB_TOOL_H
#define EDITOR_PLACE_BOB_TOOL_H

#include "multi_select.h"
#include "editor_delete_bob_tool.h"

/// Places bobs on the map.
struct Editor_Place_Bob_Tool : public Editor_Tool, public MultiSelect {
	Editor_Place_Bob_Tool(Editor_Delete_Bob_Tool & tool)
		: Editor_Tool(tool, tool)
	{}

	int32_t handle_click_impl
		(Widelands::Map &, Widelands::Node_and_Triangle<>, Editor_Interactive &);
	char const * get_sel_impl() const {return "pics/fsel_editor_place_bob.png";}
};

#endif
