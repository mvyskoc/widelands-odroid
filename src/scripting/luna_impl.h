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

/*
 * This file is only included by luna.h. It hides the gory details
 * of implementation. Since most stuff is templated, we have to keep
 * it in a header file, not an implementation file
 */

#ifndef LUA_IMPL_H
#define LUA_IMPL_H

#include <lua.hpp>

/**
 * Descriptions for the Properties/Methods that should be available to Lua
 */
template <class T>
struct PropertyType {
	char const * name;
	int (T::*getter)(lua_State *);
	int (T::*setter)(lua_State *);
};
template <class T>
struct MethodType {
	char const * name;
	int (T::*method)(lua_State *);
};


// Forward declaration of public function, because we need it below
template <class T> int to_lua(lua_State * L, T * obj);

/*
 * The table must be the last on the stack. Our userdata is saved in
 * table[0]. This function fetches it and makes sure that it is the correct
 * userdata.
 */
template <class T>
T * * m_get_user_class_from_table(lua_State * const L) {
	//  GET table[0]
	lua_pushnumber(L, 0);
	lua_rawget    (L, 1);

	return static_cast<T * *>(luaL_checkudata(L, -1, T::className));
}

template <class T>
PropertyType<T> const * m_lookup_property_in_metatable(lua_State * const L) {
	// Look up the key in the metatable
	lua_getmetatable(L,  1);
	lua_pushvalue   (L,  2);
	lua_rawget      (L, -2);

	if (!lua_islightuserdata(L, -1))
		return 0;

	return static_cast<const PropertyType<T> *>(lua_touserdata(L, -1));
}

/**
 * Handler for properties. We first try to return the object if it is in
 * our table. If not, we check if this is a registered property in the
 * metatable. If so, we call our getter method and return the value.
 * Otherwise, returns nil.
 *
 * Stack at call:
 *   idx 2: string - name of variable requested
 *   idx 1: table  - current objects table
 */
template <class T>
int m_property_getter(lua_State * const L) {
	// Try a normal get on the table
	lua_pushvalue(L, 2);
	lua_rawget   (L, 1);

	if (!lua_isnil(L, -1)) {
		// Found in the table, we return it
		return 1;
	}


	const PropertyType<T>* list = m_lookup_property_in_metatable<T>(L);
	// Not in metatable?, return nil
	if (!list)
		return 1;

	T * * const obj = m_get_user_class_from_table<T>(L);
	// push value on top of the stack for the method call
	lua_pushvalue(L, 3);
	return ((*obj)->*(list->getter))(L);
}

template <class T>
int m_property_setter(lua_State * const L) {
	PropertyType<T> const * list = m_lookup_property_in_metatable<T>(L);

	if (!list) {
		// Not in metatable?
		// Pop the result(nil) and the metatable
		lua_pop(L, 2);

		// do a normal set on the table
		lua_rawset(L, 1);
		return 0;
	}

	T * * const obj = m_get_user_class_from_table<T>(L);
	// push value on top of the stack for the method call
	lua_pushvalue(L, 3);

	if (list->setter == 0)
		return report_error(L, "The property '%s' is read-only!\n", list->name);
	return ((*obj)->*(list->setter))(L);
}

/**
 * This function calls a lua method in our given object.  we can already be
 * sure that the called method is registered in our metatable, but we have to
 * make sure that the method is called correctly: obj.method(obj) or
 * obj:method(). We also check that we are not called with another object
 */
template <class T>
int m_method_dispatch(lua_State * const L) {
	// Check for invalid: obj.method()
	int const n = lua_gettop(L);
	if (!n)
		return report_error(L, "Method needs at least the object as argument!");

	// Check for invalid: obj.method(plainOldDatatype)
	luaL_checktype(L, 1, LUA_TTABLE);

	// Get the method pointer from the closure
	typedef int (T::* const * ConstMethod)(lua_State *);
	ConstMethod func = reinterpret_cast<ConstMethod>
		(lua_touserdata(L, lua_upvalueindex(1)));

	T * * const obj = m_get_user_class_from_table<T>(L);
	// pop userdata from the stack
	lua_pop(L, 1);

	// Call it on our instance
	return ((*obj)->*(*func))(L);
}

/**
 * Deletes a given object, as soon as Lua wants to get rid of it
 */
template <class T>
int m_garbage_collect(lua_State * const L) {
	T * * const obj = static_cast<T * *>(luaL_checkudata(L, -1, T::className));

	if (obj)
		delete *obj;

	return 0;
}

/**
 * Object creation from lua. The object needs a constructor that only
 * takes a lua_State. If direct creation of this object is not desired,
 * use report_error in the constructor to tell this the user
 */
template <class T>
int m_constructor(lua_State * const L) {
	return to_lua<T>(L, new T(L));
}

template <class T>
void m_add_constructor_to_lua(lua_State * const L) {
	lua_pushcfunction (L, &m_constructor<T>);
	lua_setfield(L, -2, T::className);
	lua_pop(L, 1);
}

template <class T>
int m_create_metatable_for_class(lua_State * const L) {
	luaL_newmetatable(L, T::className);
	int const metatable = lua_gettop(L);

	// OVERLOAD LUA TABLE FUNCTIONS
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, &m_garbage_collect<T>);
	lua_settable  (L, metatable);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, &m_property_getter<T>);
	lua_settable  (L, metatable);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, &m_property_setter<T>);
	lua_settable  (L, metatable);

	return metatable;
}

template <class T>
void m_register_properties_in_metatable
	(lua_State * const L, int const metatable)
{
	for (int i = 0; T::Properties[i].name; ++i) {
		// metatable[prop_name] = Pointer to getter setter
		lua_pushstring(L, T::Properties[i].name);
		lua_pushlightuserdata
			(L,
			 const_cast<void *>
			 	(reinterpret_cast<void const *>(&T::Properties[i])));
		lua_settable(L, metatable);
	}
}

template <class T, class PT>
void m_register_methods_in_metatable(lua_State * const L, int const metatable)
{
	// We add a lua C closure around the call, the closure gets the pointer to
	// the c method to call as its only argument. We can then use
	// method_dispatch as a one-fits-all caller
	typedef int (T::* const * ConstMethod)(lua_State *);

	for (int i = 0; PT::Methods[i].name; ++i) {
		lua_pushstring(L, PT::Methods[i].name);
		lua_pushlightuserdata
			(L,
			 const_cast<void *>
			 	(reinterpret_cast<void const *>(&PT::Methods[i].method)));
		lua_pushcclosure(L, &(m_method_dispatch<T>), 1);
		lua_settable    (L, metatable);
	}
}

#endif
