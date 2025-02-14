/*
 * Copyright (C) 2002, 2006, 2008-2009 by the Widelands Development Team
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

#include "radiobutton.h"

#include "checkbox.h"

namespace UI {

struct Radiobutton : public Statebox {
	friend struct Radiogroup;

	Radiobutton
		(Panel * parent, Point, PictureID picid, Radiogroup &, int32_t id);
	~Radiobutton();

private:
	void clicked();

	Radiobutton * m_nextbtn;
	Radiogroup  & m_group;
	int32_t           m_id;
};

/**
 * Initialize the radiobutton and link it into the group's linked list
*/
Radiobutton::Radiobutton
	(Panel      * const parent,
	 Point        const p,
	 PictureID    const picid,
	 Radiogroup &       group,
	 int32_t      const id)
	:
	Statebox (parent, p, picid),
	m_nextbtn(group.m_buttons),
	m_group  (group),
	m_id     (id)
{
	group.m_buttons = this;
}

/**
 * Unlink the radiobutton from its group
 */
Radiobutton::~Radiobutton()
{
	for (Radiobutton * * pp = &m_group.m_buttons; *pp; pp = &(*pp)->m_nextbtn) {
		if (*pp == this) {
			*pp = m_nextbtn;
			break;
		}
	}
}

/**
 * Inform the radiogroup about the click; the group is responsible of setting
 * button states.
 */
void Radiobutton::clicked()
{
	m_group.set_state(m_id);
	play_click();
}


/*
==============================================================================

Radiogroup

==============================================================================
*/

/**
 * Initialize an empty radiogroup
 */
Radiogroup::Radiogroup()
{
	m_buttons = 0;
	m_highestid = -1;
	m_state = -1;
}

/**
 * Free all associated buttons.
 */
Radiogroup::~Radiogroup() {while (m_buttons) delete m_buttons;}


/**
 * Create a new radio button with the given attributes
 * Returns the ID of the new button.
*/
int32_t Radiogroup::add_button
	(Panel      * const parent,
	 Point        const p,
	 PictureID    const picid,
	 char const * const tooltip)
{
	++m_highestid;
	(new Radiobutton(parent, p, picid, *this, m_highestid))->set_tooltip
		(tooltip);
	return m_highestid;
}


/**
 * Change the state and set button states to reflect the change.
 *
 * Args: state  the ID of the checked button (-1 means don't check any button)
 */
void Radiogroup::set_state(int32_t const state) {
	if (state == m_state) {
		clicked.call();
		return;
	}

	for (Radiobutton * btn = m_buttons; btn; btn = btn->m_nextbtn)
		btn->set_state(btn->m_id == state);
	m_state = state;
	changed.call();
	changedto.call(state);
}

}
