/*
 * Copyright (C) 2003, 2006-2008 by the Widelands Development Team
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

#ifndef WARESDISPLAY_H
#define WARESDISPLAY_H

#include "logic/warelist.h"

#include "ui_basic/textarea.h"

#include <vector>

namespace UI {struct Textarea;}

namespace Widelands {
struct Tribe_Descr;
struct WareList;
}

/*
class WaresDisplay
------------------
Panel that displays the contents of a WareList.
*/
struct WaresDisplay : public UI::Panel {
	enum {
		Width = 5 * 24 + 4 * 4, //  (5 wares icons) + space in between

		WaresPerRow = 5,
	};

	enum wdType {
		WORKER,
		WARE
	};

public:
	WaresDisplay
		(UI::Panel * const parent,
		 int32_t const x, int32_t const y, Widelands::Tribe_Descr const &);
	virtual ~WaresDisplay();

	bool handle_mousemove
		(Uint8 state, int32_t x, int32_t y, int32_t xdiff, int32_t ydiff);

	void add_warelist(Widelands::WareList const &, wdType);
	void remove_all_warelists();

protected:
	virtual void draw(RenderTarget &);
	virtual void draw_ware
		(RenderTarget &,
		 Point,
		 Widelands::Ware_Index,
		 uint32_t stock,
		 bool     is_worker);

private:
	typedef std::vector<Widelands::WareList const *> vector_type;

	Widelands::Tribe_Descr const & m_tribe;
	UI::Textarea        m_curware;
	wdType              m_type;
	vector_type         m_warelists;
};

#endif
