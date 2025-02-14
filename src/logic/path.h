/*
 * Copyright (C) 2004, 2006-2010 by the Widelands Development Team
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

#ifndef S__PATH_H
#define S__PATH_H

#include <vector>

#include "widelands.h"
#include "widelands_geometry.h"

namespace Widelands {

/** class Path
 *
 * Represents a cross-country path found by Path::findpath, for example
 */
struct CoordPath;
struct Map;

struct Path {
	friend struct Map;

	Path() {}
	Path(const Coords & c) : m_start(c), m_end(c) {}
	Path(CoordPath &);

	void reverse();

	Coords get_start() const throw () {return m_start;}
	Coords get_end  () const throw () {return m_end;}

	typedef std::vector<Direction> Step_Vector;
	Step_Vector::size_type get_nsteps() const throw () {return m_path.size();}
	Direction operator[](Step_Vector::size_type const i) const {
		assert(i < m_path.size());
		return m_path[m_path.size() - i - 1];
	}

	void append(Map const & map, Direction);

	void reorigin(const Coords & new_origin, const Extent & extent) {
		m_start.reorigin(new_origin, extent);
		m_end  .reorigin(new_origin, extent);
	}

private:
	Coords m_start;
	Coords m_end;
	Step_Vector m_path;
};

// CoordPath is an extended path that also caches related Coords
struct CoordPath {
	CoordPath() {}
	CoordPath(Coords c) {m_coords.push_back(c);}
	CoordPath(const Map & map, const Path & path);

	Coords get_start() const throw () {return m_coords.front();}
	Coords get_end  () const throw () {return m_coords.back ();}
	const std::vector<Coords> &get_coords() const {return m_coords;}

	typedef std::vector<Direction> Step_Vector;
	Step_Vector::size_type get_nsteps() const throw () {return m_path.size();}
	Direction operator[](Step_Vector::size_type const i) const {
		assert(i < m_path.size());
		return m_path[i];
	}
	const Step_Vector & steps() const throw () {return m_path;}

	int32_t get_index(Coords field) const;

	void reverse();
	void truncate (const std::vector<char>::size_type after);
	void starttrim(const std::vector<char>::size_type before);
	void append(const Map & map, const Path & tail);
	void append(CoordPath const & tail);

private:
	Step_Vector          m_path;   //  directions
	std::vector<Coords>  m_coords; //  m_coords.size() == m_path.size() + 1
};

}

#endif


