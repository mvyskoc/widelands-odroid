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

#include "event_reveal_objective.h"

#include "logic/game.h"
#include "logic/game_data_error.h"
#include "i18n.h"
#include "wui/interactive_base.h"
#include "logic/map.h"
#include "profile/profile.h"
#include "wexception.h"

#define EVENT_VERSION 2

namespace Widelands {

Event_Reveal_Objective::Event_Reveal_Objective
	(Section & s, Editor_Game_Base & egbase)
	: Event(s), m_objective(0)
{
	try {
		uint32_t const packet_version = s.get_safe_positive("version");
		if (packet_version <= EVENT_VERSION) {
			char const * const objective_name = s.get_safe_string("objective");
			if (Objective * const obj = egbase.map().mom()[objective_name])
				set_objective(obj);
			else
				throw wexception("objective \"%s\" does not exist", objective_name);
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error
			("Event (Reveal Objective) %s: %s", name().c_str(), e.what());
	}
}

void Event_Reveal_Objective::Write
	(Section & s, Editor_Game_Base const &, Map_Map_Object_Saver const &) const
{
	assert(m_objective);
	s.set_string("type",      "reveal_objective");
	s.set_int   ("version",   EVENT_VERSION);
	s.set_string("objective", m_objective->name());
}


Event::State Event_Reveal_Objective::run(Game &) {
	assert(m_objective);

	m_objective->set_is_visible(true);

	return m_state = DONE;
}

}
