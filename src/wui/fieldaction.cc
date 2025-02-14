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

#include "fieldaction.h"

#include "attack_box.h"
#include "logic/attackable.h"
#include "logic/cmd_queue.h"
#include "economy/economy.h"
#include "economy/flag.h"
#include "economy/road.h"
#include "editor/editorinteractive.h"
#include "game_debug_ui.h"
#include "graphic/graphic.h"
#include "i18n.h"
#include "interactive_player.h"
#include "logic/maphollowregion.h"
#include "logic/militarysite.h"
#include "overlay_manager.h"
#include "logic/player.h"
#include "logic/soldier.h"
#include "logic/tribe.h"
#include "logic/warehouse.h"
#include "military_box.h"
#include "watchwindow.h"

#include "ui_basic/box.h"
#include "ui_basic/button.h"
#include "ui_basic/icongrid.h"
#include "ui_basic/tabpanel.h"
#include "ui_basic/textarea.h"
#include "ui_basic/unique_window.h"

#include "upcast.h"

namespace Widelands {struct Building_Descr;}
using Widelands::Building;
using Widelands::Editor_Game_Base;
using Widelands::Game;

#define BG_CELL_WIDTH  34 // extents of one cell
#define BG_CELL_HEIGHT 34


// The BuildGrid presents a selection of buildable buildings
struct BuildGrid : public UI::Icon_Grid {
	BuildGrid
		(UI::Panel                    * parent,
		 Widelands::Tribe_Descr const & tribe,
		 const int32_t x, const int32_t y,
		 int32_t cols);

	UI::Signal1<Widelands::Building_Index::value_t> buildclicked;
	UI::Signal1<Widelands::Building_Index::value_t> buildmouseout;
	UI::Signal1<Widelands::Building_Index::value_t> buildmousein;

	void add(Widelands::Building_Index::value_t);

private:
	void clickslot(int32_t idx);
	void mouseoutslot(int32_t idx);
	void mouseinslot(int32_t idx);

private:
	Widelands::Tribe_Descr const & m_tribe;
};


BuildGrid::BuildGrid
	(UI::Panel                    * parent,
	 Widelands::Tribe_Descr const & tribe,
	 int32_t const x, int32_t const y,
	 int32_t                        cols)
:
	UI::Icon_Grid
		(parent, x, y, BG_CELL_WIDTH, BG_CELL_HEIGHT, Grid_Horizontal, cols),
	m_tribe(tribe)
{
	clicked.set(this, &BuildGrid::clickslot);
	mouseout.set(this, &BuildGrid::mouseoutslot);
	mousein.set(this, &BuildGrid::mouseinslot);
}


/*
===============
Add a new building to the list of buildable buildings
===============
*/
void BuildGrid::add(Widelands::Building_Index::value_t const id)
{
	Widelands::Building_Descr const & descr =
		*m_tribe.get_building_descr(Widelands::Building_Index(id));
	UI::Icon_Grid::add
		(descr.get_buildicon(), reinterpret_cast<void *>(id), descr.descname());
}


/*
===============
BuildGrid::clickslot [private]

The icon with the given index has been clicked. Figure out which building it
belongs to and trigger signal buildclicked.
===============
*/
void BuildGrid::clickslot(int32_t const idx)
{
	buildclicked.call
		(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}


/*
===============
BuildGrid::mouseoutslot [private]

The mouse pointer has left the icon with the given index. Figure out which
building it belongs to and trigger signal buildmouseout.
===============
*/
void BuildGrid::mouseoutslot(int32_t idx)
{
	buildmouseout.call
		(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}


/*
===============
BuildGrid::mouseinslot [private]

The mouse pointer has entered the icon with the given index. Figure out which
building it belongs to and trigger signal buildmousein.
===============
*/
void BuildGrid::mouseinslot(int32_t idx)
{
	buildmousein.call
		(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}



/*
==============================================================================

FieldActionWindow IMPLEMENTATION

==============================================================================
*/
struct FieldActionWindow : public UI::UniqueWindow {
	FieldActionWindow
		(Interactive_Base           * ibase,
		 Widelands::Player          * plr,
		 UI::UniqueWindow::Registry * registry);
	~FieldActionWindow();

	Interactive_Base & ibase() {
		return ref_cast<Interactive_Base, UI::Panel>(*get_parent());
	}

	virtual void think();

	void init();
	void add_buttons_auto();
	void add_buttons_build(int32_t buildcaps);
	void add_buttons_road(bool flag);
	void add_buttons_attack();

	void act_watch();
	void act_show_census();
	void act_show_statistics();
	void act_debug();
	void act_buildflag();
	void act_configure_economy();
	void act_ripflag();
	void act_buildroad();
	void act_abort_buildroad();
	void act_removeroad();
	void act_build              (Widelands::Building_Index::value_t);
	void building_icon_mouse_out(Widelands::Building_Index::value_t);
	void building_icon_mouse_in (Widelands::Building_Index::value_t);
	void act_geologist();
	void act_attack();      /// Launch the attack

	/// Total number of attackers available for a specific enemy flag
	uint32_t get_max_attackers();

private:
	uint32_t add_tab
		(const char * picname,
		 UI::Panel * panel,
		 const std::string & tooltip_text = std::string());
	void add_button
		(UI::Box *,
		 char const * picname,
		 void (FieldActionWindow::*fn)(),
		 std::string const & tooltip_text,
		 bool repeating = false);
	void okdialog();

	Widelands::Player * m_plr;
	Widelands::Map    * m_map;
	Overlay_Manager & m_overlay_manager;

	Widelands::FCoords  m_node;

	UI::Tab_Panel      m_tabpanel;
	bool m_fastclick; // if true, put the mouse over first button in first tab
	uint32_t m_best_tab;
	Overlay_Manager::Job_Id m_workarea_preview_job_id;
	PictureID workarea_cumulative_picid[NUMBER_OF_WORKAREA_PICS];

	/// Variables to use with attack dialog.
	AttackBox * m_attack_box;
};

static char const * const pic_tab_buildroad  = "pics/menu_tab_buildroad.png";
static char const * const pic_tab_watch      = "pics/menu_tab_watch.png";
static char const * const pic_tab_military   = "pics/menu_tab_military.png";
static char const * const pic_tab_buildhouse[] = {
	"pics/menu_tab_buildsmall.png",
	"pics/menu_tab_buildmedium.png",
	"pics/menu_tab_buildbig.png"
};
static const std::string tooltip_tab_build[] = {
	_("Build small buildings"),
	_("Build medium buildings"),
	_("Build large buildings")
};

static char const * const pic_tab_buildmine  = "pics/menu_tab_buildmine.png";

static char const * const pic_buildroad      = "pics/menu_build_way.png";
static char const * const pic_remroad        = "pics/menu_rem_way.png";
static char const * const pic_buildflag      = "pics/menu_build_flag.png";
static char const * const pic_ripflag        = "pics/menu_rip_flag.png";
static char const * const pic_watchfield     = "pics/menu_watch_field.png";
static char const * const pic_showcensus     = "pics/menu_show_census.png";
static char const * const pic_showstatistics = "pics/menu_show_statistics.png";
static char const * const pic_debug          = "pics/menu_debug.png";
static char const * const pic_abort          = "pics/menu_abort.png";
static char const * const pic_geologist      = "pics/menu_geologist.png";

static char const * const pic_tab_attack     = "pics/menu_tab_attack.png";
static char const * const pic_attack         = "pics/menu_attack.png";


/*
===============
Initialize a field action window, creating the appropriate buttons.
===============
*/
FieldActionWindow::FieldActionWindow
	(Interactive_Base           * const ib,
	 Widelands::Player          * const plr,
	 UI::UniqueWindow::Registry * const registry)
:
	UI::UniqueWindow(ib, registry, 68, 34, _("Action")),
	m_plr(plr),
	m_map(&ib->egbase().map()),
	m_overlay_manager(*m_map->get_overlay_manager()),
	m_node(ib->get_sel_pos().node, &(*m_map)[ib->get_sel_pos().node]),
	m_tabpanel(this, 0, 0, g_gr->get_picture(PicMod_UI, "pics/but1.png")),
	m_fastclick(true),
	m_best_tab(0),
	m_workarea_preview_job_id(Overlay_Manager::Job_Id::Null()),
	m_attack_box(0)
{
	ib->set_sel_freeze(true);

	m_tabpanel.set_snapparent(true);

	char filename[] = "pics/workarea0cumulative.png";
	compile_assert(NUMBER_OF_WORKAREA_PICS <= 9);
	for (Workarea_Info::size_type i = 0; i < NUMBER_OF_WORKAREA_PICS; ++i) {
		++filename[13];
		workarea_cumulative_picid[i] = g_gr->get_picture(PicMod_Game, filename);
	}
}


FieldActionWindow::~FieldActionWindow()
{
	if (m_workarea_preview_job_id)
		m_overlay_manager.remove_overlay(m_workarea_preview_job_id);
	ibase().set_sel_freeze(false);
	delete m_attack_box;
}


void FieldActionWindow::think() {
	if
		(m_plr and m_plr->vision(m_node.field - &ibase().egbase().map()[0]) <= 1
		 and not m_plr->see_all())
		die();
}


/*
===============
Initialize after buttons have been registered.
This mainly deals with mouse placement
===============
*/
void FieldActionWindow::init()
{
	m_tabpanel.resize();

	center_to_parent(); // override UI::UniqueWindow position

	// Move the window away from the current mouse position, i.e.
	// where the field is, to allow better view
	const Point mouse = get_mouse_position();
	if
		(0 <= mouse.x and mouse.x < get_w()
		 and
		 0 <= mouse.y and mouse.y < get_h())
	{
		set_pos
			(Point(get_x(), get_y())
			 +
			 Point
			 	(0, (mouse.y < get_h() / 2 ? 1 : -1)
			 	 *
			 	 get_h()));
		move_inside_parent();
	}

	// Now force the mouse onto the first button
	// TODO: should be on first tab button if we're building
	set_mouse_pos
		(Point(17 + BG_CELL_WIDTH * m_best_tab, m_fastclick ? 51 : 17));
}


/*
===============
Add the buttons you normally get when clicking on a field.
===============
*/
void FieldActionWindow::add_buttons_auto()
{
	UI::Box * buildbox = 0;
	UI::Box & watchbox = *new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

	// Add road-building actions
	Interactive_GameBase const & igbase =
		ref_cast<Interactive_GameBase, Interactive_Base>(ibase());
	Widelands::Player_Number const owner = m_node.field->get_owned_by();
	if (igbase.can_see(owner)) {

		Widelands::BaseImmovable * const imm = m_map->get_immovable(m_node);

		// The box with road-building buttons
		buildbox = new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

		if (upcast(Widelands::Flag, flag, imm)) {
			// Add flag actions
			bool const can_act = igbase.can_act(owner);
			if (can_act) {
				add_button
					(buildbox,
					 pic_buildroad,
					 &FieldActionWindow::act_buildroad,
					 _("Build road"));

				Building * const building = flag->get_building();

				if
					(!building
					 ||
					 building->get_playercaps() & (1 << Building::PCap_Bulldoze))
					add_button
						(buildbox,
						 pic_ripflag,
						 &FieldActionWindow::act_ripflag,
						 _("Destroy this flag"));
			}

			if (dynamic_cast<Game const *>(&ibase().egbase())) {
				add_button
					(buildbox,
					 "pics/genstats_nrwares.png",
					 &FieldActionWindow::act_configure_economy,
					 _("Configure economy"));
				if (can_act)
					add_button
						(buildbox,
						 pic_geologist,
						 &FieldActionWindow::act_geologist,
						 _("Send geologist to explore site"));
			}
		} else {
			int32_t const buildcaps = m_plr ? m_plr->get_buildcaps(m_node) : 0;

			// Add house building
			if
				((buildcaps & Widelands::BUILDCAPS_SIZEMASK)
				 ||
				 (buildcaps & Widelands::BUILDCAPS_MINE))
				add_buttons_build(buildcaps);

			// Add build actions
			if ((m_fastclick = buildcaps & Widelands::BUILDCAPS_FLAG))
				add_button
					(buildbox,
					 pic_buildflag,
					 &FieldActionWindow::act_buildflag,
					 _("Put a flag"));

			if (dynamic_cast<Widelands::Road const *>(imm))
				add_button
					(buildbox,
					 pic_remroad,
					 &FieldActionWindow::act_removeroad,
					 _("Destroy a road"));
		}
	} else if
		(m_plr and
		 1
		 <
		 m_plr->vision
		 	(Widelands::Map::get_index
		 	 	(m_node, ibase().egbase().map().get_width())))
		add_buttons_attack ();

	//  Watch actions, only when game (no use in editor) same for statistics.
	//  census is ok
	if (dynamic_cast<Game const *>(&ibase().egbase())) {
		add_button
			(&watchbox,
			 pic_watchfield,
			 &FieldActionWindow::act_watch,
			 _("Watch field in a separate window"));
		add_button
			(&watchbox,
			 pic_showstatistics,
			 &FieldActionWindow::act_show_statistics,
			 _("Toggle building statistics display"));
	}
	add_button
		(&watchbox,
		 pic_showcensus,
		 &FieldActionWindow::act_show_census,
		 _("Toggle building label display"));

	if (ibase().get_display_flag(Interactive_Base::dfDebug))
		add_button
			(&watchbox,
			 pic_debug,
			 &FieldActionWindow::act_debug,
			 _("Debug window"));

	MilitaryBox * militarybox =
		m_plr ? new MilitaryBox(&m_tabpanel, m_plr, 0, 0) : 0;

	// Add tabs
	if (buildbox && buildbox->get_nritems()) {
		buildbox->resize();
		add_tab(pic_tab_buildroad, buildbox, _("Build roads"));
	}

	watchbox.resize();
	add_tab(pic_tab_watch, &watchbox, _("Watch"));

	if (militarybox) {
		if (militarybox->allowed_change()) {
			militarybox->resize();
			add_tab(pic_tab_military, militarybox, _("Military settings"));
		} else
			delete militarybox;
	}
}

void FieldActionWindow::add_buttons_attack ()
{
	UI::Box & a_box = *new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

	if
		(m_plr and
		 m_plr->player_number() != m_node.field->get_owned_by())
	{
		if
			(upcast
			 	(Widelands::Attackable, attackable, m_map->get_immovable(m_node)))
			if (attackable->canAttack()) {
				m_attack_box = new AttackBox(&a_box, m_plr, &m_node, 0, 0);
				a_box.add(m_attack_box, UI::Box::AlignTop);

				add_button
					(&a_box,
					 pic_attack,
					 &FieldActionWindow::act_attack,
					 _("Start attack"));
			}
	}

	if (a_box.get_nritems()) { //  add tab
		m_attack_box->resize();
		a_box.resize();
		add_tab(pic_tab_attack, &a_box, _("Attack"));
	}
}

/*
===============
Add buttons for house building.
===============
*/
void FieldActionWindow::add_buttons_build(int32_t const buildcaps)
{
	if (not m_plr)
		return;
	BuildGrid * bbg_house[3] = {0, 0, 0};
	BuildGrid * bbg_mine = 0;

	Widelands::Tribe_Descr const & tribe = m_plr->tribe();

	m_fastclick = false;

	Widelands::Building_Index const nr_buildings = tribe.get_nrbuildings();
	for
		(Widelands::Building_Index id = Widelands::Building_Index::First();
		 id < nr_buildings;
		 ++id)
	{
		Widelands::Building_Descr const & descr = *tribe.get_building_descr(id);
		BuildGrid * * ppgrid;

		//  Some building types cannot be built (i.e. construction site) and not
		//  allowed buildings.
		if (dynamic_cast<Game const *>(&ibase().egbase())) {
			if (!descr.is_buildable() || !m_plr->is_building_type_allowed(id))
				continue;
		} else if (!descr.is_buildable() && !descr.is_enhanced())
			continue;

		// Figure out if we can build it here, and in which tab it belongs
		if (descr.get_ismine()) {
			if (!(buildcaps & Widelands::BUILDCAPS_MINE))
				continue;

			ppgrid = &bbg_mine;
		} else {
			int32_t size = descr.get_size() - Widelands::BaseImmovable::SMALL;

			if ((buildcaps & Widelands::BUILDCAPS_SIZEMASK) < size + 1)
				continue;

			ppgrid = &bbg_house[size];
		}

		// Allocate the tab's grid if necessary
		if (!*ppgrid) {
			*ppgrid = new BuildGrid(&m_tabpanel, tribe, 0, 0, 5);
			(*ppgrid)->buildclicked.set(this, &FieldActionWindow::act_build);
			(*ppgrid)->buildmouseout.set
				(this, &FieldActionWindow::building_icon_mouse_out);

			(*ppgrid)->buildmousein.set
				(this, &FieldActionWindow::building_icon_mouse_in);
		}

		// Add it to the grid
		(*ppgrid)->add(id.value());
	}

	// Add all necessary tabs
	for (int32_t i = 0; i < 3; ++i)
		if (bbg_house[i])
			m_tabpanel.activate
				(m_best_tab = add_tab
				 	(pic_tab_buildhouse[i],
				 	 bbg_house[i],
				 	 i18n::translate(tooltip_tab_build[i])));

	if (bbg_mine)
		m_tabpanel.activate
			(m_best_tab = add_tab(pic_tab_buildmine, bbg_mine, _("Build mines")));
}


/*
===============
Buttons used during road building: Set flag here and Abort
===============
*/
void FieldActionWindow::add_buttons_road(bool const flag)
{
	UI::Box & buildbox = *new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

	if (flag)
		add_button
			(&buildbox,
			 pic_buildflag, &FieldActionWindow::act_buildflag, _("Build flag"));

	add_button
		(&buildbox,
		 pic_abort, &FieldActionWindow::act_abort_buildroad, _("Cancel road"));

	// Add the box as tab
	buildbox.resize();
	add_tab(pic_tab_buildroad, &buildbox, _("Build roads"));
}


/*
===============
Convenience function: Adds a new tab to the main tab panel
===============
*/
uint32_t FieldActionWindow::add_tab
	(char const * picname, UI::Panel * panel, std::string const & tooltip_text)
{
	return
		m_tabpanel.add
			(g_gr->get_picture(PicMod_Game, picname), panel, tooltip_text);
}


void FieldActionWindow::add_button
	(UI::Box           * const box,
	 char        const * const picname,
	 void (FieldActionWindow::*fn)(),
	 std::string const & tooltip_text,
	 bool                const repeating)
{
	UI::Callback_Button<FieldActionWindow> & button =
		*new UI::Callback_Button<FieldActionWindow>
			(box,
			 0, 0, 34, 34,
			 g_gr->get_picture(PicMod_UI, "pics/but2.png"),
			 g_gr->get_picture(PicMod_Game, picname),
			 fn, *this, tooltip_text);
	button.set_repeating(repeating);
	box->add
		(&button, UI::Box::AlignTop);
}

/*
===============
Call this from the button handlers.
It resets the mouse to its original position and closes the window
===============
*/
void FieldActionWindow::okdialog()
{
	ibase().warp_mouse_to_node(m_node);
	die();
}

/*
===============
Open a watch window for the given field and delete self.
===============
*/
void FieldActionWindow::act_watch()
{
	show_watch_window
		(ref_cast<Interactive_GameBase, Interactive_Base>(ibase()), m_node);
	okdialog();
}


/*
===============
Toggle display of census and statistics for buildings, respectively.
===============
*/
void FieldActionWindow::act_show_census()
{
	ibase().set_display_flag
		(Interactive_Base::dfShowCensus,
		 !ibase().get_display_flag(Interactive_Base::dfShowCensus));
	okdialog();
}

void FieldActionWindow::act_show_statistics()
{
	ibase().set_display_flag
		(Interactive_Base::dfShowStatistics,
		 !ibase().get_display_flag(Interactive_Base::dfShowStatistics));
	okdialog();
}


/*
===============
Show a debug widow for this field.
===============
*/
void FieldActionWindow::act_debug()
{
	show_field_debug(ibase(), m_node);
}


/*
===============
Build a flag at this field
===============
*/
void FieldActionWindow::act_buildflag()
{
	ref_cast<Game, Editor_Game_Base>(ibase().egbase()).send_player_build_flag
		(m_plr->player_number(), m_node);
	if (ibase().is_building_road())
		ibase().finish_build_road();
	else
		ref_cast<Interactive_Player, Interactive_Base>(ibase())
			.set_flag_to_connect(m_node);
	okdialog();
}


void FieldActionWindow::act_configure_economy()
{
	if (upcast(Widelands::Flag const, flag, m_node.field->get_immovable()))
		flag->get_economy()->show_options_window();
}


/*
===============
Remove the flag at this field
===============
*/
void FieldActionWindow::act_ripflag()
{
	okdialog();
	Widelands::Editor_Game_Base & egbase = ibase().egbase();
	if (upcast(Widelands::Flag, flag, m_node.field->get_immovable())) {
		if (Building * const building = flag->get_building()) {
			if (building->get_playercaps() & (1 << Building::PCap_Bulldoze))
				show_bulldoze_confirm
					(ref_cast<Interactive_Player, Interactive_Base>(ibase()),
					 *building,
					 flag);
		} else {
			ref_cast<Game, Editor_Game_Base>(egbase).send_player_bulldoze
					(*flag, get_key_state(SDLK_LCTRL) or get_key_state(SDLK_RCTRL));
			ibase().need_complete_redraw();
		}
	}
}


/*
===============
Start road building.
===============
*/
void FieldActionWindow::act_buildroad()
{
	//if we area already building a road just ignore this
	if (!ibase().is_building_road()) {
		ibase().start_build_road(m_node, m_plr->player_number());
		okdialog();
	}
}

/*
===============
Abort building a road.
===============
*/
void FieldActionWindow::act_abort_buildroad()
{
	if (!ibase().is_building_road())
		return;

	ibase().abort_build_road();
	okdialog();
}

/*
===============
Remove the road at the given field
===============
*/
void FieldActionWindow::act_removeroad()
{
	Widelands::Editor_Game_Base & egbase = ibase().egbase();
	if (upcast(Widelands::Road, road, egbase.map().get_immovable(m_node)))
		ref_cast<Game, Editor_Game_Base>(egbase).send_player_bulldoze
			(*road, get_key_state(SDLK_LCTRL) or get_key_state(SDLK_RCTRL));
	ibase().need_complete_redraw();
	okdialog();
}


/*
===============
Start construction of the building with the give description index
===============
*/
void FieldActionWindow::act_build(Widelands::Building_Index::value_t const idx)
{
	Widelands::Game & game = ref_cast<Game, Editor_Game_Base>(ibase().egbase());
	game.send_player_build
		(ref_cast<Interactive_Player, Interactive_Base>(ibase()).player_number(),
		 m_node,
		 Widelands::Building_Index(idx));
	ibase().reference_player_tribe
		(m_plr->player_number(), &m_plr->tribe());
	ref_cast<Interactive_Player, Interactive_Base>(ibase()).set_flag_to_connect
		(game.map().br_n(m_node));
	okdialog();
}


void FieldActionWindow::building_icon_mouse_out
	(Widelands::Building_Index::value_t)
{
	if (m_workarea_preview_job_id) {
		m_overlay_manager.remove_overlay(m_workarea_preview_job_id);
		m_workarea_preview_job_id = Overlay_Manager::Job_Id::Null();
	}
}


void FieldActionWindow::building_icon_mouse_in
	(Widelands::Building_Index::value_t const idx)
{
	if (ibase().m_show_workarea_preview) {
		assert(not m_workarea_preview_job_id);
		m_workarea_preview_job_id = m_overlay_manager.get_a_job_id();
		Widelands::HollowArea<> hollow_area(Widelands::Area<>(m_node, 0), 0);
		const Workarea_Info & workarea_info =
			m_plr->tribe().get_building_descr(Widelands::Building_Index(idx))
			->m_workarea_info;
		Workarea_Info::const_iterator it = workarea_info.begin();
		for
			(Workarea_Info::size_type i =
			 	std::min(workarea_info.size(), NUMBER_OF_WORKAREA_PICS);
			 i;
			 ++it)
		{
			--i;
			hollow_area.radius = it->first;
			assert(hollow_area.radius);
			assert(hollow_area.hole_radius < hollow_area.radius);
			Widelands::MapHollowRegion<> mr(*m_map, hollow_area);
			do
				m_overlay_manager.register_overlay
					(mr.location(),
					 workarea_cumulative_picid[i],
					 0,
					 Point::invalid(),
					 m_workarea_preview_job_id);
			while (mr.advance(*m_map));
			hollow_area.hole_radius = hollow_area.radius;
		}

#if 0
		//  This is debug output.
		//  Improvement suggestion: add to sign explanation window instead.
		container_iterate_const(Workarea_Info, workarea_info, i) {
			log("Radius: %i\n", i.current->first);
			container_iterate_const(std::set<std::string>, i.current->second, j)
				log("        %s\n", j.current->c_str());
		}
#endif

	}
}


/*
===============
Call a geologist on this flag.
===============
*/
void FieldActionWindow::act_geologist()
{
	Game & game = ref_cast<Game, Editor_Game_Base>(ibase().egbase());
	if (upcast(Widelands::Flag, flag, game.map().get_immovable(m_node)))
		game.send_player_flagaction (*flag);

	okdialog();
}

/**
 * Here there are a problem: the sender of an event is always the owner of
 * were is done this even. But for attacks, the owner of an event is the
 * player who start an attack, so is needed to get an extra parameter to
 * the send_player_enemyflagaction, the player number
 */
void FieldActionWindow::act_attack ()
{
	Game & game = ref_cast<Game, Editor_Game_Base>(ibase().egbase());

	assert(m_attack_box);
	if (upcast(Building, building, game.map().get_immovable(m_node)))
		if (m_attack_box->soldiers() > 0)
			game.send_player_enemyflagaction
				(building->base_flag(),
				 ref_cast<Interactive_Player const, Interactive_Base const>
				 	(ibase())
				 .player_number(),
				 m_attack_box->soldiers(), //  number of soldiers
				 m_attack_box->retreat());
	okdialog();
}

/*
===============
show_field_action

Perform a field action (other than building options).
Bring up a field action window or continue road building.
===============
*/
void show_field_action
	(Interactive_Base           * const ibase,
	 Widelands::Player          * const player,
	 UI::UniqueWindow::Registry * const registry)
{
	// Force closing of old fieldaction windows. This is necessary because
	// show_field_action() does not always open a FieldActionWindow (e.g.
	// connecting the road we are building to an existing flag)
		delete registry->window;
	*registry = UI::UniqueWindow::Registry();

	if (!ibase->is_building_road()) {
		FieldActionWindow & w = *new FieldActionWindow(ibase, player, registry);
		w.add_buttons_auto();
		return w.init();
	}

	Widelands::Map const & map = player->egbase().map();

	// we're building a road right now
	Widelands::FCoords const target =
		map.get_fcoords(ibase->get_sel_pos().node);

	// if user clicked on the same field again, build a flag
	if (target == ibase->get_build_road_end()) {
		FieldActionWindow & w = *new FieldActionWindow(ibase, player, registry);
		w.add_buttons_road
			(target != ibase->get_build_road_start()
			 and
			 player->get_buildcaps(target) & Widelands::BUILDCAPS_FLAG);
		return w.init();
	}

	// append or take away from the road
	if (!ibase->append_build_road(target)) {
		FieldActionWindow & w = *new FieldActionWindow(ibase, player, registry);
		w.add_buttons_road(false);
		w.init();
		return;
	}

	// did he click on a flag or a road where a flag can be built?

	if (upcast(Widelands::PlayerImmovable const, i, map.get_immovable(target)))
	{
		bool finish = false;
		if      (dynamic_cast<Widelands::Flag const *>(i))
			finish = true;
		else if (dynamic_cast<Widelands::Road const *>(i))
			if (player->get_buildcaps(target) & Widelands::BUILDCAPS_FLAG) {
				ref_cast<Game, Editor_Game_Base>(player->egbase())
					.send_player_build_flag(player->player_number(), target);
				finish = true;
			}
		if (finish)
			ibase->finish_build_road();
	}
}
