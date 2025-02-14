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

#include "multilineeditbox.h"

#include "scrollbar.h"
#include "constants.h"
#include "font_handler.h"
#include "graphic/rendertarget.h"
#include "helper.h"
#include "wlapplication.h"

#include <SDL_keysym.h>

#include <algorithm>

namespace UI {
/**
 * Initialize an editbox that supports multiline strings.
*/
Multiline_Editbox::Multiline_Editbox
	(Panel * parent,
	 int32_t x, int32_t y, uint32_t w, uint32_t h,
	 const char * text)
	:
	Multiline_Textarea(parent, x, y, w, h, text, Align_Left, true),
	m_cur_pos         (get_text().size()),
	m_maxchars        (0xffff),
	m_needs_update    (false)
{
	set_scrollmode(ScrollNormal);
	set_handle_mouse(true);
	set_can_focus(true);
	set_think(false);
}


/**
 * A key event must be handled
*/
bool Multiline_Editbox::handle_key(bool const down, SDL_keysym const code) {
	m_needs_update = true;

	if (down) {
		std::string txt =
			UI::g_fh->word_wrap_text
				(m_fontname, m_fontsize, get_text(), get_eff_w());
		assert(m_cur_pos <= txt.size());
		switch (code.sym) {

		case SDLK_DELETE:
			if (m_cur_pos < txt.size()) {
				do {
					++m_cur_pos;
				} while
						(m_cur_pos < txt.size() &&
						 ((txt.at(m_cur_pos) & 0xc0) == 0x80));
				// fallthrough - handle it like backspace
			} else
				break;

		case SDLK_BACKSPACE:
			if (txt.size() and m_cur_pos) {
				while ((txt.at(--m_cur_pos) & 0xc0) == 0x80) {
					txt.erase(txt.begin() + m_cur_pos);
					if (m_cur_pos == 0)
						break;
				}
				txt.erase(txt.begin() + m_cur_pos);
				set_text(txt.c_str());
			}
			break;

		case SDLK_LEFT:
			if (0 < m_cur_pos) {
				while ((txt.at(--m_cur_pos) & 0xc0) == 0x80) {};
				if (code.mod & (KMOD_LCTRL | KMOD_RCTRL))
					for (uint32_t new_cur_pos = m_cur_pos;; m_cur_pos = new_cur_pos)
						if (0 == new_cur_pos or isspace(txt.at(--new_cur_pos)))
							break;
			}
			break;

		case SDLK_RIGHT:
			if (m_cur_pos < txt.size()) {
				do {
					++m_cur_pos;
					if (m_cur_pos >= txt.size()) {
						break;
					}

				} while ((txt.at(m_cur_pos) & 0xc0) == 0x80);

				if (code.mod & (KMOD_LCTRL | KMOD_RCTRL))
					for (uint32_t new_cur_pos = m_cur_pos;; ++new_cur_pos) {
						assert ((new_cur_pos - 1) < txt.size());
						if
							(new_cur_pos == txt.size()
							 or
							 isspace(txt.at(new_cur_pos - 1)))
						{
							m_cur_pos = new_cur_pos;
							break;
						}
					}
			}
			break;

		case SDLK_DOWN:
			if (m_cur_pos < txt.size()) {
				uint32_t begin_of_line = m_cur_pos;

				assert(begin_of_line < txt.size());
				if (txt.at(begin_of_line) == '\n')
					--begin_of_line;
				while (begin_of_line > 0 && txt.at(begin_of_line) != '\n')
					--begin_of_line;
				if (begin_of_line)
					++begin_of_line;
				uint32_t begin_of_next_line = m_cur_pos;
				while
					(begin_of_next_line < txt.size()
					 &&
					 txt.at(begin_of_next_line) != '\n')
					++begin_of_next_line;
				begin_of_next_line += begin_of_next_line == txt.size() ? -1 : 1;
				uint32_t end_of_next_line = begin_of_next_line;
				while
					(end_of_next_line < txt.size() &&
					 txt.at(end_of_next_line) != '\n')
					++end_of_next_line;
				m_cur_pos =
					begin_of_next_line + m_cur_pos - begin_of_line
					>
					end_of_next_line ? end_of_next_line :
					begin_of_next_line + m_cur_pos - begin_of_line;
				// Care about unicode letters
				while (m_cur_pos < txt.size() && (txt.at(m_cur_pos) & 0xc0) == 0x80)
					++m_cur_pos;
			}
			break;

		case SDLK_UP:
			if (m_cur_pos > 0) {
				uint32_t begin_of_line = m_cur_pos;

				if (begin_of_line >= txt.size()) {
					begin_of_line = txt.size() - 1;
				}
				assert (begin_of_line < txt.size());
				if (txt.at(begin_of_line) == '\n')
					--begin_of_line;
				while (begin_of_line > 0 && txt.at(begin_of_line) != '\n')
					--begin_of_line;
				if (begin_of_line)
					++begin_of_line;
				uint32_t end_of_last_line = begin_of_line;
				if (begin_of_line)
					--end_of_last_line;
				uint32_t begin_of_lastline = end_of_last_line;
				assert(begin_of_lastline < txt.size());
				if (begin_of_lastline > 0 && txt.at(begin_of_lastline) == '\n')
					--begin_of_lastline;
				while (begin_of_lastline > 0 && txt.at(begin_of_lastline) != '\n')
					--begin_of_lastline;
				if (begin_of_lastline)
					++begin_of_lastline;
				m_cur_pos =
					begin_of_lastline + (m_cur_pos - begin_of_line)
					>
					end_of_last_line ? end_of_last_line :
					begin_of_lastline + (m_cur_pos - begin_of_line);
				// Care about unicode letters
				while (m_cur_pos < txt.size() && (txt.at(m_cur_pos) & 0xc0) == 0x80)
					++m_cur_pos;
			}
			break;

		case SDLK_HOME:
			if (code.mod & (KMOD_LCTRL | KMOD_RCTRL))
				m_cur_pos = 0;
			else
				while (0 < m_cur_pos) {
					uint32_t const preceding_cur_pos = m_cur_pos - 1;
					if (txt.at(preceding_cur_pos) == '\n')
						break;
					else
						m_cur_pos = preceding_cur_pos;
				}
			break;

		case SDLK_END:
			if (code.mod & (KMOD_LCTRL | KMOD_RCTRL))
				m_cur_pos = txt.size();
			else
				while (m_cur_pos < txt.size() and txt.at(m_cur_pos) != '\n')
					++m_cur_pos;
			break;

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (txt.size() < m_maxchars) {
				txt.insert(txt.begin() + m_cur_pos++, 1, '\n');
				set_text(txt.c_str());
			}
			break;

		default:
			// Nullbytes happen on MacOS X when entering Multiline Chars, like for
			// example ~ + o results in a o with a tilde over it. The ~ is reported
			// as a 0 on keystroke, the o then as the unicode character. We simply
			// ignore the 0.
			if (is_printable(code) and code.unicode and txt.size() < m_maxchars) {
				if (code.unicode < 0x80)         // 1 byte char
					txt.insert(txt.begin() + m_cur_pos++, 1, code.unicode);
				else if (code.unicode < 0x800) { // 2 byte char
					txt.insert
						(txt.begin() + m_cur_pos++,
						 (((code.unicode & 0x7c0) >> 6) | 0xc0));
					txt.insert
						(txt.begin() + m_cur_pos++,
						 ((code.unicode & 0x3f) | 0x80));
				} else {                         // 3 byte char
					txt.insert
						(txt.begin() + m_cur_pos++,
						 (((code.unicode & 0xf000) >> 12) | 0xe0));
					txt.insert
						(txt.begin() + m_cur_pos++,
						 (((code.unicode & 0xfc0) >> 6) | 0x80));
					txt.insert
						(txt.begin() + m_cur_pos++,
						 ((code.unicode & 0x3f) | 0x80));
				}
			}
			set_text(txt.c_str());
			break;
		}
		set_text(txt.c_str());
		changed.call();
		return true;
	}

	return false;
}

/**
 * Handle mousebutton events
 */
bool Multiline_Editbox::handle_mousepress
		(const Uint8 btn, int32_t x, int32_t y)
{
	if (btn == SDL_BUTTON_LEFT and not has_focus()) {
		focus();
		Multiline_Textarea::set_text(get_text().c_str());
		changed.call();
		return true;
	}
	return Multiline_Textarea::handle_mousepress(btn, x, y);
}
bool Multiline_Editbox::handle_mouserelease(const Uint8, int32_t, int32_t)
{
	return false;
}

/**
 * Redraw the Editbox
*/
void Multiline_Editbox::draw(RenderTarget & dst)
{
	//  make the whole area a bit darker
	dst.brighten_rect(Rect(Point(0, 0), get_w(), get_h()), ms_darken_value);
	if (get_text().size()) {
		UI::g_fh->draw_string
			(dst,
			 m_fontname,
			 m_fontsize,
			 m_fcolor,
			 RGBColor(107, 87, 55),
			 Point(get_halign(), 0 - m_textpos),
			 get_text(),
			 m_align,
			 get_eff_w(),
			 m_cache_mode,
			 m_cache_id,
			 //  explicit cast is necessary to avoid a compiler warning
			 has_focus() ? static_cast<int32_t>(m_cur_pos) :
			 std::numeric_limits<uint32_t>::max());
		draw_scrollbar();

		uint32_t w; // just to run g_fh->get_size_from_cache
		UI::g_fh->get_size_from_cache(m_cache_id, w, m_textheight);

		m_cache_mode = Widget_Cache_Use;
	}
}

/**
 * Set text function needs to take care of the current
 * position
 */
void Multiline_Editbox::set_text(char const * const str)
{
	CalcLinePos();

	Multiline_Textarea::set_text(str);
}

/**
 * Calculate the height position of the cursor and write it to m_textpos
 * so the scrollbar can follow the cursor.
 */
void Multiline_Editbox::CalcLinePos()
{
	if (m_textheight < static_cast<uint32_t>(get_h())) {
		m_textpos = 0;
		return;
	}

	std::string const & str = get_text().c_str();
	size_t leng = str.size();
	uint32_t lbtt = 0; // linebreaks to top
	uint32_t lbtb = 0; // linebreaks to bottom

	for
		(size_t i = 0;
		 i < std::min(m_cur_pos, static_cast<uint32_t>(str.size() - 1));
		 ++i)
		if (str.at(i) == '\n')
			++lbtt;
	for (size_t i = m_cur_pos; i < leng; ++i)
		if (str.at(i) == '\n')
			++lbtb;

	m_textpos = (lbtt == 0) & (lbtb == 0) ?
		0 : (m_textheight - get_h()) * lbtt / (lbtb + lbtt);
}

}
