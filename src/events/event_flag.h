/*
 * Copyright (C) 2008-2010 by the Widelands Development Team
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

#ifndef EVENT_FLAG_H
#define EVENT_FLAG_H

#include "logic/widelands.h"
#include "event.h"
#include "logic/widelands_geometry.h"

struct Event_Flag_Option_Menu;

namespace Widelands {

struct Event_Flag : Event {
	friend struct ::Event_Flag_Option_Menu;
	Event_Flag(char const * const Name, State const S)
		: Event(Name, S), m_position(Coords::Null()), m_player(1)
	{}
	Event_Flag(Section &, Editor_Game_Base &);

	bool has_option_menu() const {return false;}
	int32_t option_menu(Editor_Interactive &) __attribute__ ((noreturn));

	void Write
		(Section &, Editor_Game_Base const &, Map_Map_Object_Saver const &)
		const;

	State run(Game &);

	void reorigin(Coords const new_origin, Extent const extent) {
		m_position.reorigin(new_origin, extent);
	}

	void set_position(Coords        const c) {m_position = c;}
	void set_player  (Player_Number const p) {m_player   = p;}

private:
	Coords        m_position;
	Player_Number m_player;
};

}

#endif
