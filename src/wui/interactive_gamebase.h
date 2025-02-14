/*
 * Copyright (C) 2002-2003, 2006-2010 by the Widelands Development Team
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

#ifndef INTERACTIVE_GAMEBASE_H
#define INTERACTIVE_GAMEBASE_H

#include "interactive_base.h"
#include "logic/game.h"
#include "graphic/graphic.h"
#include "ui_basic/widget_cache.h"

struct ChatProvider;

struct ChatDisplay : public UI::Panel {
	ChatDisplay(UI::Panel * parent, int32_t x, int32_t y, int32_t w, int32_t h);
	~ChatDisplay();

	void delete_all_left_message_pictures();

	void setChatProvider(ChatProvider &);
	virtual void draw(RenderTarget &);

private:
	ChatProvider * m_chat;
	std::vector<PictureID> m_cache_id;
	UI::Widget_Cache       m_cache_mode;
};

enum PlayerType {NONE, OBSERVER, PLAYING, VICTORIOUS, DEFEATED};

struct Interactive_GameBase : public Interactive_Base {
	struct Game_Main_Menu_Windows {
		UI::UniqueWindow::Registry loadgame;
		UI::UniqueWindow::Registry savegame;
		UI::UniqueWindow::Registry readme;
		UI::UniqueWindow::Registry keys;
		UI::UniqueWindow::Registry authors;
		UI::UniqueWindow::Registry license;
		UI::UniqueWindow::Registry sound_options;

		UI::UniqueWindow::Registry building_stats;
		UI::UniqueWindow::Registry general_stats;
		UI::UniqueWindow::Registry ware_stats;
		UI::UniqueWindow::Registry stock;
	};

	Interactive_GameBase
		(Widelands::Game &,
		 Section         & global_s,
		 PlayerType        pt          = NONE,
		 bool              chatenabled = false);
	Widelands::Game * get_game() const;
	Widelands::Game &     game() const;

	// Chat messages
	void set_chat_provider(ChatProvider &);
	ChatProvider * get_chat_provider();

	std::string const & building_census_format      () const {
		return m_building_census_format;
	}
	std::string const & building_statistics_format  () const {
		return m_building_statistics_format;
	}
	std::string const & building_tooltip_format     () const {
		return m_building_tooltip_format;
	}
	std::string const & building_window_title_format() const {
		return m_building_window_title_format;
	}

	virtual bool can_see(Widelands::Player_Number) const = 0;
	virtual bool can_act(Widelands::Player_Number) const = 0;
	virtual Widelands::Player_Number player_number() const = 0;

	virtual void node_action() = 0;
	const PlayerType & get_playertype()const {return m_playertype;}
	void set_playertype(const PlayerType & pt) {m_playertype = pt;}

protected:
	Game_Main_Menu_Windows m_mainm_windows;
	ChatProvider           * m_chatProvider;
	ChatDisplay            * m_chatDisplay;
	std::string              m_building_census_format;
	std::string              m_building_statistics_format;
	std::string              m_building_tooltip_format;
	std::string              m_building_window_title_format;
	bool                     m_chatenabled;

	PlayerType m_playertype;
	UI::UniqueWindow::Registry m_fieldaction;
};

#endif
