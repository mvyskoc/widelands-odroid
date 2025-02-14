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

#ifndef BUILDING_H
#define BUILDING_H

#include "ai/ai_hints.h"
#include "immovable.h"
#include "soldier_counts.h"
#include "workarea_info.h"
#include "writeHTML.h"
#include "ware_types.h"
#include "widelands.h"

#include "graphic/picture_id.h"
#include "io/filewrite.h"

#include <string>
#include <cstring>
#include <vector>

namespace UI {struct Window;}
struct BuildingHints;
struct EncodeData;
struct Interactive_GameBase;
struct Profile;

namespace Widelands {

struct Flag;
struct Message;
struct Tribe_Descr;
struct WaresQueue;

class Building;

#define LOW_PRIORITY             2
#define DEFAULT_PRIORITY         4
#define HIGH_PRIORITY            8

/*
 * Common to all buildings!
 */
struct Building_Descr : public Map_Object_Descr {
	Building_Descr
		(char const * _name, char const * _descname,
		 std::string const & directory, Profile &, Section & global_s,
		 Tribe_Descr const &, EncodeData const *);

	bool is_buildable   () const {return m_buildable;}
	bool is_destructible() const {return m_destructible;}
	bool is_enhanced    () const {return m_enhanced_building;}
	bool global() const {return m_global;}
	typedef std::map<Ware_Index, uint8_t> Buildcost;
	Buildcost const & buildcost() const throw () {return m_buildcost;}
	PictureID get_buildicon() const {return m_buildicon;}
	int32_t get_size() const throw () {return m_size;}
	bool get_ismine() const {return m_mine;}
	virtual uint32_t get_ui_anim() const {return get_animation("idle");}

	typedef std::set<Building_Index> Enhancements;
	Enhancements const & enhancements() const throw () {return m_enhancements;}
	void add_enhancement(const Building_Index & i) {
		assert(not m_enhancements.count(i));
		m_enhancements.insert(i);
	}

	/// Create a building of this type in the game. Calls init, which does
	/// different things for different types of buildings (such as conquering
	/// land and requesting things). Therefore this must not be used to allocate
	/// a building during savegame loading. (It would cause many bugs.)
	///
	/// Does not perform any sanity checks.
	/// If old != 0 this is an enhancing.
	Building & create
		(Editor_Game_Base &,
		 Player &,
		 Coords,
		 bool                   construct,
		 uint32_t       const * ware_counts    = 0,
		 uint32_t       const * worker_counts  = 0,
		 Soldier_Counts const * soldier_counts = 0,
		 Building_Descr const * old = 0,
		 bool                   loading = false)
		const;
#ifdef WRITE_GAME_DATA_AS_HTML
	void writeHTML(::FileWrite &) const;
#endif
	virtual void load_graphics();

	virtual uint32_t get_conquers() const;
	virtual uint32_t vision_range() const throw ();

	const Tribe_Descr & tribe() const throw () {return m_tribe;}
	Workarea_Info m_workarea_info;

	virtual int32_t suitability(Map const &, FCoords) const;
	BuildingHints const & hints() const {return m_hints;}

protected:
	virtual Building & create_object() const = 0;
	Building & create_constructionsite(Building_Descr const * old) const;

private:
	const Tribe_Descr & m_tribe;
	bool          m_buildable;       // the player can build this himself
	bool          m_destructible;    // the player can destruct this himself
	Buildcost     m_buildcost;
	PictureID     m_buildicon;       // if buildable: picture in the build dialog
	std::string   m_buildicon_fname; // filename for this icon
	int32_t       m_size;            // size of the building
	bool          m_mine;
	Enhancements  m_enhancements;
	bool          m_enhanced_building; // if it is one, it is bulldozable
	BuildingHints m_hints;             // hints (knowledge) for computer players
	bool          m_global;            // whether this is a "global" building

	// for migration, 0 is the default, meaning get_conquers() + 4
	uint32_t m_vision_range;
};


class Building : public PlayerImmovable {
	friend struct Building_Descr;
	friend struct Map_Buildingdata_Data_Packet;

	MO_DESCR(Building_Descr)

public:
	// Player capabilities: which commands can a player issue for this building?
	enum {
		PCap_Bulldoze = 0, // can bulldoze/remove this buildings
		PCap_Enhancable = 3, // can be enhanced to something
	};

public:
	Building(const Building_Descr &);
	virtual ~Building();

	void load_finish(Editor_Game_Base &);

	Tribe_Descr const & tribe() const throw () {return descr().tribe();}

	virtual int32_t  get_type    () const throw ();
	char const * type_name() const throw () {return "building";}
	virtual int32_t  get_size    () const throw ();
	virtual bool get_passable() const throw ();
	virtual uint32_t get_ui_anim () const;

	virtual Flag & base_flag();
	virtual uint32_t get_playercaps() const throw ();
	virtual Coords get_position() const throw () {return m_position;}

	std::string const & name() const throw ();
	const std::string & descname() const throw () {return descr().descname();}

	std::string info_string(std::string const & format);
	virtual std::string get_statistics_string();

	/// \Returns the queue for a ware type or \throws _wexception.
	virtual WaresQueue & waresqueue(Ware_Index) __attribute__ ((noreturn));

	/// Fills the building with things that it would normally request
	/// (wares/workers/soldiers).
	virtual void prefill
		(Game &,
		 uint32_t       const * ware_counts,
		 uint32_t       const * worker_counts,
		 Soldier_Counts const * worker_types);
	virtual void postfill
		(Game &,
		 uint32_t       const * ware_counts,
		 uint32_t       const * worker_counts,
		 Soldier_Counts const * worker_types);

	virtual bool burn_on_destroy();
	virtual void destroy(Editor_Game_Base &);

	void show_options(Interactive_GameBase &);
	void hide_options();

	virtual bool fetch_from_flag(Game &);
	virtual bool get_building_work(Game &, Worker &, bool success)
		__attribute__ ((noreturn));

	bool leave_check_and_wait(Game &, Worker &);
	void leave_skip(Game &, Worker &);
	uint32_t get_conquers() const throw () {return descr().get_conquers();}
	virtual uint32_t vision_range() const throw () {
		return descr().vision_range();
	}

	int32_t get_base_priority() const {return m_priority;}
	virtual int32_t get_priority
		(int32_t type, Ware_Index, bool adjust = true) const;
	void set_priority(int32_t new_priority);
	void set_priority(int32_t type, Ware_Index ware_index, int32_t new_priority);

	void collect_priorities
		(std::map<int32_t, std::map<Ware_Index, int32_t> > & p) const;

	std::set<Building_Index> const & enhancements() const throw () {
		return descr().enhancements();
	}

	virtual void log_general_info(Editor_Game_Base const &);

	//  Use on training sites only.
	virtual void change_train_priority(uint32_t, int32_t) {};
	virtual void switch_train_mode () {};

	///  Stores the Player_Number of the player who has defeated this building.
	void set_defeating_player(Player_Number const player_number) {
		m_defeating_player = player_number;
	}

	void    add_worker(Worker &);
	void remove_worker(Worker &);

	/// Creates a message specific to this building. It will have the building's
	/// coordinates, and display a picture of the building in its description.
	///
	/// The message will be allocated with operator new and ownership of it is
	/// given to the caller.
	Message & create_message
		(std::string const & msgsender,
		 uint32_t            time,
		 Duration            duration,
		 std::string const & title,
		 std::string const & description)
		const;

protected:
	void start_animation(Editor_Game_Base &, uint32_t anim);

	virtual void init(Editor_Game_Base &);
	virtual void cleanup(Editor_Game_Base &);
	virtual void act(Game &, uint32_t data);

	virtual void draw(Editor_Game_Base const &, RenderTarget &, FCoords, Point);
	void draw_help(Editor_Game_Base const &, RenderTarget &, FCoords, Point);

	virtual void create_options_window
		(Interactive_GameBase &, UI::Window * & registry)
		= 0;

	UI::Window * m_optionswindow;
	Coords       m_position;
	Flag       * m_flag;

	uint32_t m_anim;
	int32_t  m_animstart;

	typedef std::vector<OPtr<Worker> > Leave_Queue;
	Leave_Queue m_leave_queue; //  FIFO queue of workers leaving the building
	uint32_t    m_leave_time;  //  when to wake the next one from leave queue
	Object_Ptr  m_leave_allow; //  worker that is allowed to leave now

	//  The player who has defeated this building.
	Player_Number           m_defeating_player;

	int32_t m_priority; // base priority
	std::map<Ware_Index, int32_t> m_ware_priorities;
};

}

#endif
