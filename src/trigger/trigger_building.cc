/*
 * Copyright (C) 2002-2004, 2006-2008, 2010 by the Widelands Development Team
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

#include "trigger_building.h"

#include "logic/building.h"
#include "editor/editorinteractive.h"
#include "logic/game.h"
#include "logic/game_data_error.h"
#include "i18n.h"
#include "logic/map.h"
#include "logic/player.h"
#include "profile/profile.h"

#include "upcast.h"

#define PACKET_VERSION 3

namespace Widelands {

Trigger_Building::Trigger_Building(char const * const Name, bool const set)
:
	Trigger_Player_Area(Name, set),
	m_building_type    (0)
{}


void Trigger_Building::Read(Section & s, Editor_Game_Base & egbase) {
	try {
		uint32_t const packet_version = s.get_safe_positive("version");
		if (packet_version <= PACKET_VERSION) {
			Trigger_Player_Area::Read(s, egbase);
			egbase.get_ibase()->reference_player_tribe
				(m_player_area.player_number, this);
			m_building_type =
				&s.get_safe_Building_Type
					("building", egbase, m_player_area.player_number);
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error(_("(building): %s"), e.what());
	}
}

void Trigger_Building::Write
	(Section                    & s,
	 Editor_Game_Base     const & egbase,
	 Map_Map_Object_Saver const & mos)
	const
{
	s.set_string       ("type",     "building");
	s.set_int          ("version",  PACKET_VERSION);
	Trigger_Player_Area::Write(s, egbase, mos);
	s.set_string("building", m_building_type->name());
}

/**
 * Check if trigger conditions are done
 */
void Trigger_Building::check_set_conditions(Game const & game) {
	Map const & map = game.map();

	uint32_t count = 0;
	MapRegion<Area<FCoords> > mr (map, m_player_area);
	do if
		(upcast(Building const, building, mr.location().field->get_immovable()))
		if
			(building->get_position() == mr.location()
			 and
			 building->owner().player_number() == m_player_area.player_number
			 and
			 &building->descr() == m_building_type)
			if (m_count <= ++count)
				return set_trigger(true);
	while (mr.advance(map));
}

}
