/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#ifndef __S__EVENT_MESSAGEBOX_H
#define __S__EVENT_MESSAGEBOX_H

#include "event.h"
#include "trigger/trigger_referencer.h"

#include <vector>

class Trigger_Null;
class Editor_Game_Base;
namespace UI {struct Panel;};

/*
 * This event shows a message box
 */
struct Event_Message_Box : public Event, public TriggerReferencer {
     Event_Message_Box();
      ~Event_Message_Box();

      // For trigger referenecer
	const char * get_type() const {return "Event:MessageBox";}
	const std::string & name() const {return Event::name();}

      // one liner functions
	const char * get_id() const {return "message_box";}

      State run(Game*);
      virtual void reinitialize(Game*);

      // File Functions
	void Write(Section &, const Editor_Game_Base &) const;
      void Read(Section*, Editor_Game_Base*);

      inline void set_text(const char* str) {m_text=str;}
	const char * get_text() const {return m_text.c_str();}
      inline void set_window_title(const char* str) {m_window_title=str;}
	const char * get_window_title() const {return m_window_title.c_str();}
      inline void set_is_modal(bool t) {m_is_modal=t;}
	bool get_is_modal() const {return m_is_modal;}
      inline void set_pos(int posx, int posy) {m_posx=posx; m_posy=posy;}
	int get_posx() const {return m_posx;}
	int get_posy() const {return m_posy;}
      inline void set_dimensions(int w, int h) {m_width = w; m_height = h;}
	int get_w() const {return m_width;}
	int get_h() const {return m_height;}
      void set_button_trigger(int i, Trigger_Null* t);
      Trigger_Null* get_button_trigger(int i);
      void set_button_name(int i, std::string);
      const char* get_button_name(int i);
      void set_nr_buttons(int i);
	int get_nr_buttons() const {return m_buttons.size();}

	enum {
         Right = 0,
         Left,
         Center_under,
         Center_over,
	};

private:
	struct Button_Descr {
         std::string name;
         Trigger_Null *trigger;
	};

      std::string m_text;
      std::string m_window_title;
      bool m_is_modal;

      std::vector<Button_Descr> m_buttons;
      UI::Panel*      m_window;
      int  m_posx, m_posy;
      int  m_width, m_height;
};



#endif
