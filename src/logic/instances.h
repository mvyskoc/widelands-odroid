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

#ifndef INSTANCES_H
#define INSTANCES_H

#include "cmd_queue.h"

#include "ref_cast.h"

#include <map>
#include <string>
#include <cstring>
#include <vector>

struct DirAnimations;
struct RenderTarget;
namespace UI {struct Tab_Panel;}

namespace Widelands {

struct Path;
struct Player;
struct Map_Map_Object_Loader;

/**
 * Base class for descriptions of worker, files and so on. This must just
 * link them together
 */
struct Map_Object_Descr {
	friend struct ::DirAnimations;
	typedef uint8_t Index;
	Map_Object_Descr(char const * const _name, char const * const _descname)
		: m_name(_name), m_descname(_descname)
	{}
	virtual ~Map_Object_Descr() {m_anims.clear();}

	std::string const &     name() const throw () {return m_name;}
	std::string const & descname() const throw () {return m_descname;}
	struct Animation_Nonexistent {};
	uint32_t get_animation(char const * const anim) const {
		std::map<std::string, uint32_t>::const_iterator it = m_anims.find(anim);
		if (it == m_anims.end())
			throw Animation_Nonexistent();
		return it->second;
	}

	uint32_t main_animation() const throw () {
		return m_anims.begin() != m_anims.end() ? m_anims.begin()->second : 0;
	}

	std::string get_animation_name(uint32_t) const; ///< needed for save, debug
	bool has_attribute(uint32_t) const throw ();
	static uint32_t get_attribute_id(std::string const & name);
	static std::string get_attribute_name(uint32_t const & id);

	bool is_animation_known(const char * name) const;
	void add_animation(const char * name, uint32_t anim);

	protected:
		void add_attribute(uint32_t attr);



private:
	typedef std::map<std::string, uint32_t> Anims;
	typedef std::map<std::string, uint32_t> AttribMap;
	typedef std::vector<uint32_t>           Attributes;

	Map_Object_Descr & operator= (Map_Object_Descr const &);
	explicit Map_Object_Descr    (Map_Object_Descr const &);

	std::string const m_name;
	std::string const m_descname;       ///< Descriptive name
	Attributes        m_attributes;
	Anims             m_anims;
	static uint32_t   s_dyn_attribhigh; ///< highest attribute ID used
	static AttribMap  s_dyn_attribs;

};

/**
 * dummy instance because Map_Object needs a description
 * \todo move this to another header??
 */
extern Map_Object_Descr g_flag_descr;

/**
 * \par Notes on Map_Object
 *
 * Map_Object is the base class for everything that can be on the map:
 * buildings, animals, decorations, etc... most of the time, however, you'll
 * deal with one of the derived classes, BaseImmovable or Bob.
 *
 * Every Map_Object has a unique serial number. This serial number is used as
 * key in the Object_Manager map, and in the safe Object_Ptr.
 *
 * Unless you're perfectly sure about when an object can be destroyed you
 * should use an Object_Ptr or, better yet, the type safe OPtr template.
 * This is not necessary when the relationship and lifetime between objects
 * is well-defined, such as in the relationship between Building and Flag.
 *
 * Map_Objects can also have attributes. They are mainly useful for finding
 * objects of a given type (e.g. trees) within a certain radius.
 *
 * \warning DO NOT allocate/free Map_Objects directly. Use the appropriate
 * type-dependent create() function for creation, and call die() for removal.
 *
 * \note Convenient creation functions are defined in class Game.
 *
 * When you do create a new object yourself (i.e. when you're implementing one
 * of the create() functions), you need to allocate the object using new,
 * potentially set it up by calling basic functions like set_position(),
 * set_owner(), etc. and then call init(). After that, the object is supposed to
 * be fully created.
*/

/// If you find a better way to do this that doesn't cost a virtual function
/// or additional member variable, go ahead
#define MO_DESCR(type) \
public: const type & descr() const { \
      return ref_cast<type const, Map_Object_Descr const>(*m_descr);          \
   }                                                                          \

class Map_Object {
	friend struct Object_Manager;
	friend struct Object_Ptr;

	MO_DESCR(Map_Object_Descr);

public:
	enum {
		AREAWATCHER,
		BOB,  //  class Bob

		WARE, //  class WareInstance
		BATTLE,

		// everything below is at least a BaseImmovable
		IMMOVABLE,

		// everything below is at least a PlayerImmovable
		BUILDING,
		FLAG,
		ROAD
	};
	/// Some default, globally valid, attributes.
	/// Other attributes (such as "harvestable corn") could be
	/// allocated dynamically (?)
	enum Attribute {
		CONSTRUCTIONSITE = 1, ///< assume BUILDING
		WORKER,               ///< assume BOB
		SOLDIER,              ///<  assume WORKER
		RESI,                 ///<  resource indicator, assume IMMOVABLE

		HIGHEST_FIXED_ATTRIBUTE
	};

	struct LogSink {
		virtual void log(std::string str) = 0;
		virtual ~LogSink() {}
	};

	virtual void load_finish(Editor_Game_Base &) {}

protected:
	Map_Object(Map_Object_Descr const * descr);
	virtual ~Map_Object() {}

public:
	virtual int32_t get_type() const throw () = 0;
	virtual char const * type_name() const throw () {return "map object";}

	Serial serial() const {return m_serial;}

	/**
	 * Attributes are fixed boolean properties of an object.
	 * An object either has a certain attribute or it doesn't.
	 * See the \ref Attribute enume.
	 *
	 * \return whether this object has the given attribute
	 */
	bool has_attribute(uint32_t const attr) const {
		return descr().has_attribute(attr);
	}

	/**
	 * \return the value of the given \ref tAttribute. -1 if this object
	 * doesn't have this kind of attribute.
	 * The default behaviour returns \c -1 for all attributes.
	 */
	virtual int32_t get_tattribute(uint32_t attr) const;

	void remove(Editor_Game_Base &);
	virtual void destroy(Editor_Game_Base &);

	//  The next functions are really only needed in games, not in the editor.
	void schedule_destroy(Game &);
	uint32_t schedule_act(Game &, uint32_t tdelta, uint32_t data = 0);
	virtual void act(Game &, uint32_t data);

	// implementation is in game_debug_ui.cc
	virtual void create_debug_panels
		(Editor_Game_Base const & egbase, UI::Tab_Panel & tabs);

	LogSink * get_logsink() {return m_logsink;}
	void set_logsink(LogSink *);

	/// Called when a new logsink is set. Used to give general information.
	virtual void log_general_info(Editor_Game_Base const &);

	// saving and loading
	/**
	 * Header bytes to distinguish between data packages for the different
	 * Map_Object classes.
	 *
	 * Be careful in changing those, since they are written to files.
	 */
	enum {
		header_Map_Object = 1,
		header_Immovable = 2,
		header_Legacy_Battle = 3,
		header_Legacy_AttackController = 4,
		header_Battle = 5
	};

	/**
	 * Static load functions of derived classes will return a pointer to
	 * a Loader class. The caller needs to call the virtual functions
	 * \ref load for all instances loaded that way, after that call
	 * \ref load_pointers for all instances loaded that way and finally
	 * call \ref load_finish for all instances loaded that way.
	 * Those are the three phases of loading. After the last phase,
	 * all Loader objects should be deleted.
	 */
	class Loader {
		Editor_Game_Base      * m_egbase;
		Map_Map_Object_Loader * m_mol;
		Map_Object            * m_object;

	protected:
		Loader() : m_egbase(0), m_mol(0), m_object(0) {}

	public:
		virtual ~Loader() {}

		void init
			(Editor_Game_Base & e, Map_Map_Object_Loader & m, Map_Object & object)
		{
			m_egbase = &e;
			m_mol    = &m;
			m_object = &object;
		}

		Editor_Game_Base      & egbase    () {return *m_egbase;}
		Map_Map_Object_Loader & mol   () {return *m_mol;}
		Map_Object            * get_object() {return m_object;}
		template<typename T> T & get() {
			return ref_cast<T, Map_Object>(*m_object);
		}

	protected:
		virtual void load(FileRead &, uint8_t version);

	public:
		virtual void load_pointers();
		virtual void load_finish();
	};

	/// This is just a fail-safe guard for the time until we fully transition
	/// to the new Map_Object saving system
	virtual bool has_new_save_support() {return false;}

	virtual void save(Editor_Game_Base &, Map_Map_Object_Saver &, FileWrite &);
	// Pure Map_Objects cannot be loaded

protected:
	/// Called only when the oject is logically created in the simulation. If
	/// called again, such as when the object is loaded from a savegame, it will
	/// cause bugs.
	virtual void init(Editor_Game_Base &);

	virtual void cleanup(Editor_Game_Base &);

	void molog(char const * fmt, ...) const
		__attribute__((format(printf, 2, 3)));

protected:
	const Map_Object_Descr * m_descr;
	Serial                   m_serial;
	LogSink                * m_logsink;

private:
	Map_Object & operator= (Map_Object const &);
	explicit Map_Object    (Map_Object const &);
};

inline int32_t get_reverse_dir(int32_t const dir) {
	return 1 + ((dir - 1) + 3) % 6;
}


/**
 *
 * Keeps the list of all objects currently in the game.
 */
struct Object_Manager {
	typedef std::map<uint32_t, Map_Object *> objmap_t;

	Object_Manager() {m_lastserial = 0;}
	~Object_Manager();

	void cleanup(Editor_Game_Base &);

	Map_Object * get_object(Serial const serial) const {
		const objmap_t::const_iterator it = m_objects.find(serial);
		return it != m_objects.end() ? it->second : 0;
	}

	void insert(Map_Object &);
	void remove(Map_Object &);

	bool object_still_available(const Map_Object * const t) const {
		objmap_t::const_iterator it = m_objects.begin();
		while (it != m_objects.end()) {
			if (it->second == t)
				return true;
			++it;
		}
		return false;
	}

	/**
	 * Get the map of all objects for the purpose of iterating over it.
	 * Only provide a const version of the map!
	 */
	const objmap_t & get_objects() const throw () {return m_objects;}

private:
	Serial   m_lastserial;
	objmap_t m_objects;

	Object_Manager & operator= (Object_Manager const &);
	Object_Manager             (Object_Manager const &);
};

/**
 * Provides a safe pointer to a Map_Object
 */
struct Object_Ptr {
	Object_Ptr(Map_Object * const obj = 0) {m_serial = obj ? obj->m_serial : 0;}
	// can use standard copy constructor and assignment operator

	Object_Ptr & operator= (Map_Object * const obj) {
		m_serial = obj ? obj->m_serial : 0;
		return *this;
	}

	bool is_set() const {return m_serial;}

	// dammit... without a Editor_Game_Base object, we can't implement a
	// Map_Object* operator (would be _really_ nice)
	Map_Object * get(Editor_Game_Base const &);
	Map_Object const * get(Editor_Game_Base const & egbase) const;

	bool operator<  (const Object_Ptr & other) const throw () {
		return m_serial < other.m_serial;
	}
	bool operator== (const Object_Ptr & other) const throw () {
		return m_serial == other.m_serial;
	}
	bool operator!= (const Object_Ptr & other) const throw () {
		return m_serial != other.m_serial;
	}

	uint32_t serial() const {return m_serial;}

private:
	uint32_t m_serial;
};

template<class T>
struct OPtr {
	OPtr(T * const obj = 0) : m(obj) {}

	OPtr & operator= (T * const obj) {
		m = obj;
		return *this;
	}

	bool is_set() const {return m.is_set();}

	T       * get(Editor_Game_Base const &       egbase)       {
		return static_cast<T *>(m.get(egbase));
	}
	T       * get(Editor_Game_Base const * const egbase)       {
		return get(egbase);
	}
	T const * get(Editor_Game_Base const &       egbase) const {
		return static_cast<T const *>(m.get(egbase));
	}
	T const * get(Editor_Game_Base const * const egbase) const {
		return get(egbase);
	}

	bool operator<  (OPtr<T> const & other) const {return m <  other.m;}
	bool operator== (OPtr<T> const & other) const {return m == other.m;}
	bool operator!= (OPtr<T> const & other) const {return m != other.m;}

	Serial serial() const {return m.serial();}

private:
	Object_Ptr m;
};

struct Cmd_Destroy_Map_Object : public GameLogicCommand {
	Cmd_Destroy_Map_Object() : GameLogicCommand(0) {} ///< For savegame loading
	Cmd_Destroy_Map_Object (int32_t t, Map_Object &);
	virtual void execute (Game &);

	void Write(FileWrite &, Editor_Game_Base &, Map_Map_Object_Saver  &);
	void Read (FileRead  &, Editor_Game_Base &, Map_Map_Object_Loader &);

	virtual uint8_t id() const {return QUEUE_CMD_DESTROY_MAPOBJECT;}

private:
	Serial obj_serial;
};

struct Cmd_Act : public GameLogicCommand {
	Cmd_Act() : GameLogicCommand(0) {} ///< For savegame loading
	Cmd_Act (int32_t t, Map_Object &, int32_t a);

	virtual void execute (Game &);

	void Write(FileWrite &, Editor_Game_Base &, Map_Map_Object_Saver  &);
	void Read (FileRead  &, Editor_Game_Base &, Map_Map_Object_Loader &);

	virtual uint8_t id() const {return QUEUE_CMD_ACT;}

private:
	Serial obj_serial;
	int32_t arg;
};

}

#endif
