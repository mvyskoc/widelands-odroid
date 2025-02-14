/*
 * Copyright (C) 2002-2004, 2008-2009 by the Widelands Development Team
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

#ifndef EDITOR_MAIN_MENU_H
#define EDITOR_MAIN_MENU_H

#include "ui_basic/button.h"
#include "ui_basic/unique_window.h"

struct Editor_Interactive;

/**
 * This represents the main menu
*/
struct Editor_Main_Menu : public UI::UniqueWindow {
	Editor_Main_Menu(Editor_Interactive &, UI::UniqueWindow::Registry &);

private:
	Editor_Interactive & eia();
	UI::Callback_Button<Editor_Main_Menu> m_button_new_map;
	UI::Callback_Button<Editor_Main_Menu> m_button_new_random_map;
	UI::Callback_Button<Editor_Main_Menu> m_button_load_map;
	UI::Callback_Button<Editor_Main_Menu> m_button_save_map;
	UI::Callback_Button<Editor_Main_Menu> m_button_map_options;
	UI::Callback_Button<Editor_Main_Menu> m_button_view_readme;
	UI::Callback_Button<Editor_Main_Menu> m_button_exit_editor;

	UI::UniqueWindow::Registry m_window_readme;

	void exit_btn       ();
	void load_btn       ();
	void save_btn       ();
	void new_map_btn    ();
	void new_random_map_btn    ();
	void map_options_btn();
	void readme_btn     ();
};


#endif
