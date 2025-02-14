/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#include "editor_player_menu.h"

#include "editor/editorinteractive.h"
#include "editor/tools/editor_set_starting_pos_tool.h"
#include "graphic/graphic.h"
#include "i18n.h"
#include "logic/map.h"
#include "wui/overlay_manager.h"
#include "logic/player.h"
#include "logic/tribe.h"
#include "logic/warehouse.h"
#include "wexception.h"

#include "ui_basic/editbox.h"
#include "ui_basic/messagebox.h"
#include "ui_basic/textarea.h"


Editor_Player_Menu::Editor_Player_Menu
	(Editor_Interactive & parent, UI::UniqueWindow::Registry & registry)
	:
	UI::UniqueWindow(&parent, &registry, 340, 400, _("Player Options")),
	m_add_player
		(this,
		 5, 5, 20, 20,
		 g_gr->get_picture(PicMod_UI, "pics/but1.png"),
		 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
		 &Editor_Player_Menu::clicked_add_player, *this,
		 _("Add player"),
		 parent.egbase().map().get_nrplayers() < MAX_PLAYERS),
	m_remove_last_player
		(this,
		 get_inner_w() - 5 - 20, 5, 20, 20,
		 g_gr->get_picture(PicMod_UI, "pics/but1.png"),
		 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
		 &Editor_Player_Menu::clicked_remove_last_player, *this,
		 _("Remove last player"),
		 1 < parent.egbase().map().get_nrplayers())
{
	int32_t const spacing = 5;
	int32_t const width   = 20;
	int32_t       posy    = 0;

	Widelands::Tribe_Descr::get_all_tribenames(m_tribes);

	set_inner_size(375, 135);

	UI::Textarea * ta = new UI::Textarea(this, 0, 0, _("Number of Players"));
	ta->set_pos(Point((get_inner_w() - ta->get_w()) / 2, posy + 5));
	posy += spacing + width;

	m_nr_of_players_ta = new UI::Textarea(this, 0, 0, "5");
	m_nr_of_players_ta->set_pos
		(Point((get_inner_w() - m_nr_of_players_ta->get_w()) / 2, posy + 5));
	posy += width + spacing + spacing;

	m_posy = posy;

	for (Widelands::Player_Number i = 0; i < MAX_PLAYERS; ++i) {
		m_plr_names          [i] = 0;
		m_plr_set_pos_buts   [i] = 0;
		m_plr_set_tribes_buts[i] = 0;
	}
	update();

	set_think(true);
}

/**
 * Think function. Some things may change while this window
 * is open
 */
void Editor_Player_Menu::think() {
	update();
}

/**
 * Update all
*/
void Editor_Player_Menu::update() {
	if (is_minimal())
		return;

	Widelands::Map & map =
		ref_cast<Editor_Interactive const, UI::Panel const>(*get_parent())
		.egbase().map();
	Widelands::Player_Number const nr_players = map.get_nrplayers();
	{
		assert(nr_players <= 99); //  2 decimal digits
		char text[3];
		if (char const nr_players_10 = nr_players / 10) {
			text[0] = '0' + nr_players_10;
			text[1] = '0' + nr_players % 10;
			text[2] = '\0';
		} else {
			text[0] = '0' + nr_players;
			text[1] = '\0';
		}
		m_nr_of_players_ta->set_text(text);
	}

	//  Now remove all the unneeded stuff.
	for (Widelands::Player_Number i = nr_players; i < MAX_PLAYERS; ++i) {
		delete m_plr_names          [i]; m_plr_names          [i] = 0;
		delete m_plr_set_pos_buts   [i]; m_plr_set_pos_buts   [i] = 0;
		delete m_plr_set_tribes_buts[i]; m_plr_set_tribes_buts[i] = 0;
	}
	int32_t       posy    = m_posy;
	int32_t const spacing =  5;
	int32_t const size    = 20;

	iterate_player_numbers(p, nr_players) {
		int32_t posx = spacing;
		if (!m_plr_names[p - 1]) {
			m_plr_names[p - 1] =
				new UI::EditBox
					(this, posx, posy, 140, size,
					 g_gr->get_picture(PicMod_UI, "pics/but0.png"), p - 1);
			m_plr_names[p - 1]->changedid.set
				(this, &Editor_Player_Menu::name_changed);
			posx += 140 + spacing;
			m_plr_names[p - 1]->setText(map.get_scenario_player_name(p));
		}

		if (!m_plr_set_tribes_buts[p - 1]) {
			m_plr_set_tribes_buts[p - 1] =
				new UI::Callback_IDButton
				<Editor_Player_Menu, Widelands::Player_Number const>
					(this,
					 posx, posy, 140, size,
					 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
					 &Editor_Player_Menu::player_tribe_clicked, *this, p - 1,
					 std::string());
			posx += 140 + spacing;
		}
		if (map.get_scenario_player_tribe(p) != "<undefined>")
			m_plr_set_tribes_buts[p - 1]->set_title
				(map.get_scenario_player_tribe(p).c_str());
		else {
			m_plr_set_tribes_buts[p - 1]->set_title(m_tribes[0].c_str());
			map.set_scenario_player_tribe(p, m_tribes[0]);
		}

		// Set default AI
		map.set_scenario_player_ai(p, "");

		//  Set Starting pos button.
		if (!m_plr_set_pos_buts[p - 1]) {
			m_plr_set_pos_buts[p - 1] =
				new UI::Callback_IDButton
				<Editor_Player_Menu, Widelands::Player_Number const>
					(this,
					 posx, posy, size, size,
					 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
					 g_gr->get_no_picture(), //  set below
					 &Editor_Player_Menu::set_starting_pos_clicked, *this, p,
					 std::string());
			posx += size + spacing;
		}
		char text[] = "pics/fsel_editor_set_player_00_pos.png";
		text[28] += p / 10;
		text[29] += p % 10;
		m_plr_set_pos_buts[p - 1]->set_pic(g_gr->get_picture(PicMod_Game, text));
		posy += size + spacing;
	}
	set_inner_size(get_inner_w(), posy + spacing);
}

void Editor_Player_Menu::clicked_add_player() {
	Editor_Interactive & menu =
		ref_cast<Editor_Interactive, UI::Panel>(*get_parent());
	Widelands::Map & map = menu.egbase().map();
	Widelands::Player_Number const nr_players = map.get_nrplayers() + 1;
	assert(nr_players <= MAX_PLAYERS);
	map.set_nrplayers(nr_players);
	{ //  register new default name for this players
		assert(nr_players <= 99); //  2 decimal digits
		std::string name = _("Player ");
		if (char const nr_players_10 = nr_players / 10)
			name += '0' + nr_players_10;
		name += '0' + nr_players % 10;
		map.set_scenario_player_name(nr_players, name);
	}
	map.set_scenario_player_tribe(nr_players, m_tribes[0]);
	menu.set_need_save(true);
	m_add_player        .set_enabled(nr_players < MAX_PLAYERS);
	m_remove_last_player.set_enabled(true);
	update();
}


void Editor_Player_Menu::clicked_remove_last_player() {
	Editor_Interactive & menu =
		ref_cast<Editor_Interactive, UI::Panel>(*get_parent());
	Widelands::Map & map = menu.egbase().map();
	Widelands::Player_Number const old_nr_players = map.get_nrplayers();
	Widelands::Player_Number const nr_players     = old_nr_players - 1;
	assert(1 <= nr_players);
	if (not menu.is_player_tribe_referenced(old_nr_players)) {
		if (const Widelands::Coords sp = map.get_starting_pos(old_nr_players)) {
			//  Remove starting position marker.
			char picsname[] = "pics/editor_player_00_starting_pos.png";
			picsname[19] += old_nr_players / 10;
			picsname[20] += old_nr_players % 10;
			map.overlay_manager().remove_overlay
				(sp, g_gr->get_picture(PicMod_Game, picsname));
		}
		std::string const & name  = map.get_scenario_player_name (nr_players);
		std::string const & tribe = map.get_scenario_player_tribe(nr_players);
		map.set_nrplayers(nr_players);
		map.set_scenario_player_name(nr_players, name);   //  ???
		map.set_scenario_player_tribe(nr_players, tribe); //  ???
		menu.set_need_save(true);
		m_add_player        .set_enabled(true);
		m_remove_last_player.set_enabled(1 < nr_players);
		if
			(&menu.tools.current() == &menu.tools.set_starting_pos
			 and
			 menu.tools.set_starting_pos.get_current_player() == old_nr_players)
			//  The starting position tool is the currently active editor tool and
			//  the sel picture is the one with the color of
			//  the player that is being removed. Make sure that it is fixed in
			//  that case by by switching the tool to the previous player and
			//  reselecting the tool.
			set_starting_pos_clicked(nr_players); //  This calls update().
		else
			update();
	} else {
		UI::WLMessageBox mmb
			(&menu,
			 _("Error!"),
			 _
			 	("Can not remove player. It is referenced in some place. Remove "
			 	 "all buildings, bobs, triggers and events that depend on this "
			 	 "player and try again."),
			 UI::WLMessageBox::OK);
		mmb.run();
	}
}


/**
 * Player Tribe Button clicked
 */
void Editor_Player_Menu::player_tribe_clicked(const Uint8 n) {
	Editor_Interactive & menu =
		ref_cast<Editor_Interactive, UI::Panel>(*get_parent());
	if (not menu.is_player_tribe_referenced(n + 1)) {
		std::string t = m_plr_set_tribes_buts[n]->get_title();
		if (!Widelands::Tribe_Descr::exists_tribe(t))
			throw wexception
				("Map defines tribe %s, but it does not exist!", t.c_str());
		uint32_t i;
		for (i = 0; i < m_tribes.size(); ++i)
			if (m_tribes[i] == t)
				break;
		t = i == m_tribes.size() - 1 ? m_tribes[0] : m_tribes[++i];
		menu.egbase().map().set_scenario_player_tribe(n + 1, t);
		menu.set_need_save(true);
	} else {
		UI::WLMessageBox mmb
			(&menu,
			 _("Error!"),
			 _
			 	("Can not change player tribe. It is referenced in some place. "
			 	 "Remove all bobs, triggers and events that depend on this tribe "
			 	 "and try again."),
			 UI::WLMessageBox::OK);
		mmb.run();
	}
	update();
}


/**
 * Set Current Start Position button selected
 */
void Editor_Player_Menu::set_starting_pos_clicked(const Uint8 n) {
	Editor_Interactive & menu =
		ref_cast<Editor_Interactive, UI::Panel>(*get_parent());
	//  jump to the current node
	Widelands::Map & map = menu.egbase().map();
	if (Widelands::Coords const sp = map.get_starting_pos(n))
		menu.move_view_to(sp);

	//  select tool set mplayer
	menu.select_tool(menu.tools.set_starting_pos, Editor_Tool::First);
	menu.tools.set_starting_pos.set_current_player(n);

	//  reselect tool, so everything is in a defined state
	menu.select_tool(menu.tools.current(), Editor_Tool::First);

	//  Register callback function to make sure that only valid locations are
	//  selected.
	map.overlay_manager().register_overlay_callback_function
		(&Editor_Tool_Set_Starting_Pos_Callback, &map);
	map.recalc_whole_map();
	update();
}

/**
 * Player name has changed
 */
void Editor_Player_Menu::name_changed(int32_t m) {
	//  Player name has been changed.
	std::string text = m_plr_names[m]->text();
	Editor_Interactive & menu =
		ref_cast<Editor_Interactive, UI::Panel>(*get_parent());
	Widelands::Map & map = menu.egbase().map();
	if (text == "") {
		text = map.get_scenario_player_name(m + 1);
		m_plr_names[m]->setText(text);
	}
	map.set_scenario_player_name(m + 1, text);
	m_plr_names[m]->setText(map.get_scenario_player_name(m + 1));
	menu.set_need_save(true);
}
