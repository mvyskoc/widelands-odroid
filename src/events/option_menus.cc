/*
 * Copyright (C) 2008-2010 by the Widelands Development Team
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

/// \file
/// This is a place where the user interface links into the game logic code.

#include "editor/ui_menus/event_player_worker_types_option_menu.h"
#include "editor/ui_menus/event_player_building_types_option_menu.h"
#include "editor/ui_menus/event_expire_message_option_menu.h"
#include "editor/ui_menus/event_conquer_area_option_menu.h"
#include "editor/ui_menus/event_message_box_option_menu.h"
#include "editor/ui_menus/event_move_view_option_menu.h"
#include "editor/ui_menus/event_set_player_frontier_style_option_menu.h"
#include "editor/ui_menus/event_set_player_flag_style_option_menu.h"
#include "editor/ui_menus/event_set_timer_option_menu.h"
#include "editor/ui_menus/event_unhide_area_option_menu.h"

#include "event_player_worker_types.h"
#include "event_player_building_types.h"
#include "event_building.h"
#include "event_conquer_area.h"
#include "event_flag.h"
#include "event_immovable.h"
#include "event_message_box.h"
#include "event_move_view.h"
#include "event_reveal_campaign.h"
#include "event_reveal_objective.h"
#include "event_reveal_scenario.h"
#include "event_road.h"
#include "event_set_timer.h"
#include "event_set_player_frontier_style.h"
#include "event_set_player_flag_style.h"
#include "event_unhide_area.h"
#include "event_player_allowed_retreat_change.h"
#include "event_player_retreat_change.h"
#include "event_message.h"
#include "event_expire_message.h"

namespace Widelands {

int32_t Event_Player_Worker_Types       ::option_menu(Editor_Interactive & eia)
{Event_Player_Worker_Types_Option_Menu          m(eia, *this); return m.run();}

int32_t Event_Player_Building_Types     ::option_menu(Editor_Interactive & eia)
{Event_Player_Building_Types_Option_Menu        m(eia, *this); return m.run();}

int32_t Event_Building                  ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Conquer_Area              ::option_menu(Editor_Interactive & eia)
{Event_Conquer_Area_Option_Menu                 m(eia, *this); return m.run();}

int32_t Event_Flag                      ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Immovable                 ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Message_Box               ::option_menu(Editor_Interactive & eia)
{Event_Message_Box_Option_Menu                  m(eia, *this); return m.run();}

int32_t Event_Reveal_Campaign           ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Reveal_Objective          ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Reveal_Scenario           ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Road                      ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Set_Timer                 ::option_menu(Editor_Interactive & eia)
{Event_Set_Timer_Option_Menu                    m(eia, *this); return m.run();}

int32_t Event_Set_Player_Frontier_Style ::option_menu(Editor_Interactive & eia)
{Event_Set_Player_Frontier_Style_Option_Menu    m(eia, *this); return m.run();}

int32_t Event_Set_Player_Flag_Style     ::option_menu(Editor_Interactive & eia)
{Event_Set_Player_Flag_Style_Option_Menu        m(eia, *this); return m.run();}

int32_t Event_Move_View                 ::option_menu(Editor_Interactive & eia)
{Event_Move_View_Option_Menu                    m(eia, *this); return m.run();}

int32_t Event_Unhide_Area               ::option_menu(Editor_Interactive & eia)
{Event_Unhide_Area_Option_Menu                  m(eia, *this); return m.run();}

int32_t
Event_Player_Allowed_Retreat_Change     ::option_menu(Editor_Interactive &)
{throw;}

int32_t
Event_Player_Retreat_Change             ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Message                   ::option_menu(Editor_Interactive &)
{throw;}

int32_t Event_Expire_Message            ::option_menu(Editor_Interactive & eia)
{Event_Expire_Message_Option_Menu               m(eia, *this); return m.run();}
}
