/*
 * Copyright (C) 2002-2004, 2006, 2008-2009 by the Widelands Development Team
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

#ifndef EDITOR_MAIN_MENU_LOAD_MAP_H
#define EDITOR_MAIN_MENU_LOAD_MAP_H

#include "io/filesystem/filesystem.h"

#include "ui_basic/window.h"

struct Editor_Interactive;
namespace UI {
struct EditBox;
template <typename T> struct Listselect;
struct Textarea;
struct Multiline_Textarea;
template <typename T> struct Callback_Button;
};

/**
 * Choose a filename and save your brand new created map
*/
struct Main_Menu_Load_Map : public UI::Window {
	Main_Menu_Load_Map(Editor_Interactive &);

private:
	void clicked_ok();
	void selected      (uint32_t);
	void double_clicked(uint32_t);

	void fill_list();

	UI::Textarea * m_name, * m_author, * m_size, * m_world, * m_nrplayers;
	UI::Multiline_Textarea * m_descr;
	UI::Listselect<const char *> * m_ls;
	UI::Callback_Button<Main_Menu_Load_Map> * m_ok_btn;

	std::string m_basedir;
	std::string m_curdir;
	std::string m_parentdir;
	filenameset_t m_mapfiles;
};

#endif
