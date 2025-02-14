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

#include "widelands_map_objective_data_packet.h"

#include "logic/editor_game_base.h"
#include "logic/game_data_error.h"
#include "logic/map.h"
#include "profile/profile.h"
#include "trigger/trigger_time.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 1


void Map_Objective_Data_Packet::Read
	(FileSystem            &       fs,
	 Editor_Game_Base      &       egbase,
	 bool                    const skip,
	 Map_Map_Object_Loader &)
throw (_wexception)
{
	if (skip)
		return;

	Profile prof;
	try {prof.read("objective", 0, fs);} catch (...) {return;}
	Map & map = egbase.map();
	Manager<Objective> & mom = map.mom();
	Manager<Trigger>   & mtm = map.mtm();

	try {
		int32_t const packet_version =
			prof.get_safe_section("global").get_safe_int("packet_version");
		if (packet_version == CURRENT_PACKET_VERSION) {
			while (Section * const s = prof.get_next_section(0)) {
				char const * const         name = s->get_name();
				try {
					char const * const trigger_name = s->get_safe_string("trigger");
					Objective & objective = *new Objective();
					objective.set_name(name);
					try {
						mom.register_new(objective);
					} catch (Manager<Objective>::Already_Exists) {
						throw game_data_error("duplicated");
					}
					objective.set_visname    (s->get_string("name", name));
					objective.set_descr      (s->get_safe_string("descr"));
					objective.set_is_visible (s->get_safe_bool  ("visible"));
					if (Trigger * const trig = mtm[trigger_name])
						objective.set_trigger(trig);
					else
						throw game_data_error
							("references nonexistent trigger \"%s\"", trigger_name);
				} catch (_wexception const & e) {
					throw game_data_error(_("%s: %s"), name, e.what());
				}
			}
		} else
			throw game_data_error
				(_("unknown/unhandled version %i"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error(_("Objectives: %s"), e.what());
	}
}


void Map_Objective_Data_Packet::Write
	(FileSystem & fs, Editor_Game_Base & egbase, Map_Map_Object_Saver &)
throw (_wexception)
{
	Profile prof;
	prof.create_section("global").set_int
		("packet_version", CURRENT_PACKET_VERSION);

	Manager<Objective> const & mom = egbase.map().mom();
	Manager<Objective>::Index const nr_objectives = mom.size();
	for (Manager<Objective>::Index i = 0; i < nr_objectives; ++i) {
		Objective const & objective = mom[i];
		Section & s = prof.create_section(objective.name().c_str());
		s.set_string("name",     objective.visname());
		s.set_string("descr",    objective.descr());
		s.set_bool  ("visible",  objective.get_is_visible());
		s.set_string("trigger",  objective.get_trigger()->name());
	}

	prof.write("objective", false, fs);
}

}
