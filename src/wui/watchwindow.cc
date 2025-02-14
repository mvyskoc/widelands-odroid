/*
 * Copyright (C) 2002, 2004, 2006-2009 by the Widelands Development Team
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

#include "watchwindow.h"

#include "logic/bob.h"
#include "logic/game.h"
#include "graphic/graphic.h"
#include "i18n.h"
#include "interactive_gamebase.h"
#include "logic/map.h"
#include "mapview.h"
#include "mapviewpixelconstants.h"
#include "mapviewpixelfunctions.h"
#include "profile/profile.h"

#include "ui_basic/button.h"
#include "ui_basic/m_signal.h"
#include "ui_basic/window.h"

#include "upcast.h"

#include <vector>


#define NUM_VIEWS 5
#define REFRESH_TIME 5000

//Holds information for a view
struct WatchWindowView {
	Point view_point;
	Widelands::Object_Ptr tracking; //  if non-null, we're tracking a Bob
};

struct WatchWindow : public UI::Window {
	WatchWindow
		(Interactive_GameBase & parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 Widelands::Coords,
		 bool single_window = false);
	~WatchWindow();

	Widelands::Game & game() const {
		return ref_cast<Interactive_GameBase, UI::Panel>(*get_parent()).game();
	}

	UI::Signal1<Point> warp_mainview;

	void add_view(Widelands::Coords);
	void next_view(bool first = false);
	void show_view(bool first = false);
	Point calc_coords(Widelands::Coords);
	void save_coords();
	void close_cur_view();
	void toggle_buttons();

protected:
	virtual void think();
	void stop_tracking_by_drag(int32_t x, int32_t y);

private:
	Map_View mapview;
	uint32_t last_visit;
	bool     single_window;
	uint8_t  cur_index;


	/// If we are currently tracking, stop tracking.
	/// Otherwise, start tracking the nearest bob from our current position.
	struct Follow : public UI::Button {
		Follow(WatchWindow & parent, uint32_t const h) :
			UI::Button
				(&parent,
				 0, h - 34, 34, 34,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 g_gr->get_picture(PicMod_UI, "pics/menu_watch_follow.png"),
				 _("Follow"))
		{}
		void clicked() {
			WatchWindow     & parent =
				ref_cast<WatchWindow, UI::Panel>(*get_parent());
			Widelands::Game & game   = parent.game();
			if
				(Widelands::Map_Object const * const obj =
				 	parent.views[parent.cur_index].tracking.get(game))
				parent.views[parent.cur_index].tracking = 0;
			else {
				//  Find the nearest bob. Other object types can not move and are
				//  therefore not of interest.
				Map_View & mapview = parent.mapview;
				Point pos
					(mapview.get_viewpoint()
					 +
					 Point(mapview.get_w() / 2, mapview.get_h() / 2));
				Widelands::Map & map = game.map();
				MapviewPixelFunctions::normalize_pix(map, pos);
				std::vector<Widelands::Bob *> bobs;
				//  Scan progressively larger circles around the given position for
				//  suitable bobs.
				for
					(Widelands::Area<Widelands::FCoords> area
					 	(map.get_fcoords
					 	 	(MapviewPixelFunctions::calc_node_and_triangle
					 	 	 	(map, pos.x, pos.y)
					 	 	 .node),
					 	 2);
					 area.radius <= 32;
					 area.radius *= 2)
					if (map.find_bobs(area, &bobs))
						break;
				//  Find the bob closest to us
				int32_t closest_dist = -1;
				Widelands::Bob * closest = 0;
				for (uint32_t i = 0; i < bobs.size(); ++i) {
					Widelands::Bob * const bob = bobs[i];
					Point p;
					MapviewPixelFunctions::get_pix
						(map, bob->get_position(), p.x, p.y);
					p = bob->calc_drawpos(game, p);
					int32_t const dist =
						MapviewPixelFunctions::calc_pix_distance(map, p, pos);
					if (!closest || closest_dist > dist) {
						closest = bob;
						closest_dist = dist;
					}
				}
				parent.views[parent.cur_index].tracking = closest;
			}
		}
	} follow;

	/// Cause the main mapview to jump to our current position.
	struct Go_To : public UI::Button {
		Go_To(WatchWindow & parent, uint32_t const h) :
			UI::Button
				(&parent,
				 34, h - 34, 34, 34,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 g_gr->get_picture(PicMod_UI, "pics/menu_goto.png"),
				 _("Center mainview on this"))
		{}
		void clicked() {
			WatchWindow & parent  =
				ref_cast<WatchWindow, UI::Panel>(*get_parent());
			Map_View    & mapview = parent.mapview;
			parent.warp_mainview.call
				(mapview.get_viewpoint()
				 +
				 Point(mapview.get_w() / 2, mapview.get_h() / 2));
		}
	} go_to;

	/// Closes current view. Only visible with the single_watchwin option.
	struct Close : public UI::Button {
		Close
			(WatchWindow & parent,
			 uint32_t const w, uint32_t const h,
			 bool const visible)
			:
			UI::Button
				(&parent,
				 w - 34, h - 34, 34, 34,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 g_gr->get_picture(PicMod_UI, "pics/menu_abort.png"),
				 _("Close"),
				 visible)
		{
			set_visible(visible);
		}
		void clicked() {
			WatchWindow & parent =
				ref_cast<WatchWindow, UI::Panel>(*get_parent());
			if (parent.views.size() == 1) {
				delete &parent;
				return;
			}
			uint8_t const old_index = parent.cur_index;
			parent.next_view();
			std::vector<WatchWindowView>::iterator view_it = parent.views.begin();
			for (uint8_t i = 0; i < old_index; ++i)
				++view_it;
			parent.view_btns[parent.cur_index]->set_enabled(false);
			parent.views.erase(view_it);
			parent.toggle_buttons();
		}
	} close;

	/// Sets the current view to index and resets timeout.
	struct View_Button : public UI::Button {
		View_Button(WatchWindow & parent, uint8_t const index) :
			UI::Button
				(&parent,
				 74 + (17 * index), 200 - 34, 17, 34, g_gr->get_no_picture(),
				 "-", std::string(),
				 false),
			m_index   (index)
		{}
		void clicked() {
			WatchWindow & parent =
				ref_cast<WatchWindow, UI::Panel>(*get_parent());
			parent.save_coords();
			parent.cur_index = m_index;
			parent.last_visit = parent.game().get_gametime();
			parent.show_view();
		}
		uint8_t m_index;
	} * view_btns[NUM_VIEWS];
	std::vector<WatchWindowView> views;
};


static WatchWindow * g_watch_window = 0;

WatchWindow::WatchWindow
	(Interactive_GameBase &       parent,
	 int32_t const x, int32_t const y, uint32_t const w, uint32_t const h,
	 Widelands::Coords    const coords,
	 bool                   const _single_window)
:
	UI::Window(&parent, x, y, w, h, _("Watch")),
	mapview   (this, 0, 0, 200, 166, parent),
	last_visit(game().get_gametime()),
	single_window(_single_window),
	follow    (*this,            h),
	go_to     (*this,            h),
	close     (*this,         w, h, _single_window)
{
	if (_single_window)
		for (uint8_t i = 0; i < NUM_VIEWS; ++i)
			view_btns[i] = new View_Button(*this, i);
	mapview.fieldclicked.set(&parent, &Interactive_GameBase::node_action);
	mapview.warpview.set(this, &WatchWindow::stop_tracking_by_drag);
	warp_mainview.set(&parent, &Interactive_Base::move_view_to_point);

	add_view(coords);
	next_view(true);
	set_cache(false);
}

//Add a view to a watchwindow, if there is space left
void WatchWindow::add_view(Widelands::Coords const coords) {
	if (views.size() >= NUM_VIEWS)
		return;
	WatchWindowView view;

	view.tracking = 0;
	view.view_point = calc_coords(coords);

	views.push_back(view);
	if (single_window)
		toggle_buttons();
}

//Calc point on map from coords
Point WatchWindow::calc_coords(Widelands::Coords const coords) {
	// Initial positioning
	int32_t vx = coords.x * TRIANGLE_WIDTH;
	int32_t vy = coords.y * TRIANGLE_HEIGHT;


	return Point(vx - mapview.get_w() / 2, vy - mapview.get_h() / 2);
}

//Switch to next view
void WatchWindow::next_view(bool const first) {
	if (!first && views.size() == 1)
		return;
	if (!first)
		save_coords();
	if (first || (cur_index == views.size() - 1 && cur_index != 0))
		cur_index = 0;
	else if (cur_index < views.size() - 1)
		++cur_index;
	show_view(first);
}


//Saves the coordinates of a view if it was already shown (and possibly moved)
void WatchWindow::save_coords() {
	views[cur_index].view_point = mapview.get_viewpoint();
}


//Enables/Disables buttons for views
void WatchWindow::toggle_buttons() {
	for (uint32_t i = 0; i < NUM_VIEWS; ++i) {
		if (i < views.size()) {
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%i", i + 1);
			view_btns[i]->set_title(buffer);
			view_btns[i]->set_enabled(true);
		} else {
			view_btns[i]->set_title("-");
			view_btns[i]->set_enabled(false);
		}
	}
}

//Draws the current view
void WatchWindow::show_view(bool) {
	mapview.set_viewpoint(views[cur_index].view_point);
}

WatchWindow::~WatchWindow() {
	g_watch_window = 0;

	//  If we are destructed because our parent is destructed, our parent may
	//  not be an Interactive_GameBase any more (it may just be an UI::Panel).
	//  Then calling Interactive_GameBase::need_complete_redraw on our parent
	//  would be erroneous. Therefore this check is required. (As always, great
	//  care is required when destructors are misused to do anything else than
	//  releasing resources.)
	if (upcast(Interactive_GameBase, igbase, get_parent()))
		igbase->need_complete_redraw();
}


/*
===============
Update the mapview if we're tracking something.
===============
*/
void WatchWindow::think()
{
	UI::Window::think();

	if ((game().get_gametime() - last_visit) > REFRESH_TIME) {
		last_visit = game().get_gametime();
		next_view();
		return;
	}

	if (upcast(Widelands::Bob, bob, views[cur_index].tracking.get(game()))) {
		Point pos;

		MapviewPixelFunctions::get_pix
			(game().map(), bob->get_position(), pos.x, pos.y);
		pos = bob->calc_drawpos(game(), pos);

		mapview.set_viewpoint
			(pos - Point(mapview.get_w() / 2, mapview.get_h() / 2));
	}

	mapview.need_complete_redraw(); //  make sure that the view gets updated
}


/*
===============
When the user drags the mapview, we stop tracking.
===============
*/
void WatchWindow::stop_tracking_by_drag(int32_t, int32_t) {
	//Disable switching while dragging
	if (mapview.is_dragging()) {
		last_visit = game().get_gametime();
		views[cur_index].tracking = 0;
	}
}


/*
===============
show_watch_window

Open a watch window.
===============
*/
void show_watch_window
	(Interactive_GameBase & parent, Widelands::Coords const coords)
{
	if (g_options.pull_section("global").get_bool("single_watchwin", false)) {
		if (g_watch_window)
			g_watch_window->add_view(coords);
		else
			g_watch_window =
				new WatchWindow(parent, 250, 150, 200, 200, coords, true);
	} else
		new WatchWindow(parent, 250, 150, 200, 200, coords, false);
}
