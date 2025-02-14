/*
 * Copyright (C) 2007-2009 by the Widelands Development Team
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

#include "loadreplay.h"

#include "logic/game.h"
#include "game_io/game_loader.h"
#include "game_io/game_preload_data_packet.h"
#include "i18n.h"
#include "io/filesystem/layered_filesystem.h"
#include "logic/replay.h"

Fullscreen_Menu_LoadReplay::Fullscreen_Menu_LoadReplay() :
	Fullscreen_Menu_Base("choosemapmenu.jpg"),

// Values for alignment and size
	m_butw (m_xres / 4),
	m_buth (m_yres * 19 / 400),
	m_fs   (fs_small()),
	m_fn   (ui_fn()),

// Buttons
	m_back
		(this,
		 m_xres * 71 / 100, m_yres * 17 / 20, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
		 &Fullscreen_Menu_LoadReplay::end_modal, *this, 0,
		 _("Back"), std::string(), true, false,
		 m_fn, m_fs),
	m_ok
		(this,
		 m_xres * 71 / 100, m_yres * 9 / 10, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but2.png"),
		 &Fullscreen_Menu_LoadReplay::clicked_ok, *this,
		 _("OK"), std::string(), false, false,
		 m_fn, m_fs),

// Replay list
	m_list
		(this,
		 m_xres *  47 / 2500, m_yres * 3417 / 10000,
		 m_xres * 711 / 1250, m_yres * 6083 / 10000),

// Text area
	m_title
		(this,
		 m_xres / 2, m_yres * 3 / 20, _("Choose a replay!"),
		 UI::Align_HCenter)
{
	m_title.set_font(m_fn, fs_big(), UI_FONT_CLR_FG);
	m_list .set_font(m_fn, m_fs);
	m_list .selected.set(this, &Fullscreen_Menu_LoadReplay::replay_selected);
	m_list .double_clicked.set
		(this, &Fullscreen_Menu_LoadReplay::double_clicked);
	fill_list();
}


void Fullscreen_Menu_LoadReplay::clicked_ok()
{
	m_filename = m_list.get_selected();
	end_modal(1);
}

void Fullscreen_Menu_LoadReplay::double_clicked(uint32_t)
{
	clicked_ok();
}


void Fullscreen_Menu_LoadReplay::replay_selected(uint32_t const n)
{
	// TODO: Extract quick info about the replay
	m_ok.set_enabled
		(n != UI::Listselect<Fullscreen_Menu_LoadReplay>::no_selection_index());
}


/**
 * Fill the file list by simply fetching all files that end with the
 * replay suffix and have a valid associated savegame.
 */
void Fullscreen_Menu_LoadReplay::fill_list()
{
	filenameset_t files;

	g_fs->FindFiles(REPLAY_DIR, "*" REPLAY_SUFFIX, &files, 1);

	for
		(filenameset_t::iterator pname = files.begin();
		 pname != files.end();
		 ++pname)
	{
		std::string savename = *pname + WLGF_SUFFIX;

		if (!g_fs->FileExists(savename))
			continue;

		try {
			Widelands::Game_Preload_Data_Packet gpdp;
			Widelands::Game game;
			Widelands::Game_Loader gl(savename, game);
			gl.preload_game(gpdp);

			m_list.add
				 (FileSystem::FS_FilenameWoExt(pname->c_str()).c_str(), *pname);
		} catch (_wexception const &) {} //  we simply skip illegal entries
	}

	if (m_list.size())
		m_list.select(0);
}
