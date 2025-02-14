/*
 * Copyright (C) 2002, 2006-2008 by the Widelands Development Team
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

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "panel.h"

namespace UI {
/**
 * Windows are cached by default.
 *
 * The graphics (see m_pic_*) are used in the following manner: (Example)
 *
 * top:
 *  <--20leftmostpixel_of_top-->
 *     <60Pixels as often as possible to reach window with from top>
 *  <20rightmost pixel of top>
 * site:
 *  ^
 *  20 topmost pixels of l_border                       <--- > same for r_border
 *  as often as needed: 60 pixels of l_border           <--- > same for r_border
 *  20 bottom pixels of l_border                        <--- > same for r_border
 * bottom:
 *  <--20leftmostpixel_of_bot-->
 *     <60Pixels as often as possible to reach window with from bot>
 *  <20rightmost pixel of bot>
 *
 * So: the l_border and the r_border pics MUST have a height of 100, while the
 *     width must be 20 and the top and bot pics MUST have a width of 100, while
 *     the height must be 20
 *
 * A click with the middle mouse button (or STRG+LClick) minimizes a window.
 * Minimize means, that the window is only the caption bar, nothing inside.
 * Another click on this bar resizes the window again
 */
struct Window : public Panel {
	Window
		(Panel      * parent,
		 int32_t      x,
		 int32_t      y,
		 uint32_t     w,
		 uint32_t     h,
		 char const * title);

	void set_title(char const * text);

	void move_to_mouse();
	virtual void move_inside_parent();
	void center_to_parent();

	bool is_minimal() const throw () {return _is_minimal;}
	void restore ();
	void minimize();
	bool is_snap_target() const {return true;}

	// Drawing and event handlers
	void draw_border(RenderTarget &);

	void think();

	bool handle_mousepress  (Uint8 btn, int32_t mx, int32_t my);
	bool handle_mouserelease(Uint8 btn, int32_t mx, int32_t my);
	bool handle_mousemove
		(Uint8 state, int32_t mx, int32_t my, int32_t xdiff, int32_t ydiff);

private:
	void dock_left();
	void undock_left();
	void dock_right();
	void undock_right();
	void dock_bottom();
	void undock_bottom();
	bool _is_minimal;
	uint32_t _oldw, _oldh;  // if it is, these are the old formats
	bool _dragging, _docked_left, _docked_right, _docked_bottom;
	int32_t _drag_start_win_x, _drag_start_win_y;
	int32_t _drag_start_mouse_x, _drag_start_mouse_y;

	std::string m_title;

	PictureID m_pic_lborder;
	PictureID m_pic_rborder;
	PictureID m_pic_top;
	PictureID m_pic_bottom;
	PictureID m_pic_background;
};

}

#endif
