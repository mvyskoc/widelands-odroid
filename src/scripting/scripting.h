/*
 * Copyright (C) 2006-2010 by the Widelands Development Team
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

#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <stdexcept>
#include <string>

#include <stdint.h>

namespace Widelands {
	struct Game;
}

struct LuaError : public std::runtime_error {
	LuaError(std::string const & reason) : std::runtime_error(reason) {}
};
struct LuaValueError : public LuaError {
	LuaValueError(std::string const & wanted) :
		LuaError("Variable not of expected type: " + wanted)
	{}
};

/*
 * Class LuaCoroutine.
 *
 * Easy handling of LuaCoroutines
 */
struct LuaCoroutine {
	virtual ~LuaCoroutine() {}

	virtual int get_status() = 0;
	virtual int resume    () = 0;
};

/*
 * This class mainly provides convenient wrappers to access
 * variables that come from lua
 */
struct LuaState {
	virtual void push_int32(int32_t) = 0;
	virtual int32_t pop_int32() = 0;
	virtual void push_uint32(uint32_t) = 0;
	virtual uint32_t pop_uint32() = 0;
	virtual void push_double(double) = 0;
	virtual double pop_double() = 0;
	virtual void push_string(std::string &) = 0;
	virtual std::string pop_string() = 0;
	virtual LuaCoroutine * pop_coroutine() = 0;
};

/*
 * This is the thin class that is used to execute code
 */
struct LuaInterface {
	virtual ~LuaInterface() {}

	virtual LuaState * interpret_string(std::string) = 0;
	virtual LuaState * interpret_file(std::string) = 0;
	virtual std::string const & get_last_error() const = 0;
};

LuaInterface * create_lua_interface(Widelands::Game *);

#endif
