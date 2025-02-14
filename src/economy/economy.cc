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

#include "economy.h"

// Package includes
#include "flag.h"
#include "route.h"
#include "cmd_call_economy_balance.h"
#include "router.h"

#include "logic/game.h"
#include "logic/player.h"
#include "request.h"
#include "logic/tribe.h"
#include "upcast.h"
#include "logic/warehouse.h"
#include "warehousesupply.h"
#include "wexception.h"

namespace Widelands {

Economy::Economy(Player & player) :
	m_owner(player),
	m_rebuilding     (false),
	m_request_timerid(0)
{
	Tribe_Descr const & tribe = player.tribe();
	Ware_Index const nr_wares   = tribe.get_nrwares();
	Ware_Index const nr_workers = tribe.get_nrworkers();
	m_wares.set_nrwares(nr_wares);
	m_workers.set_nrwares(nr_workers);

	player.add_economy(*this);

	m_ware_target_quantities   = new Target_Quantity[nr_wares  .value()];
	for (Ware_Index i = Ware_Index::First(); i < nr_wares; ++i) {
		Target_Quantity tq;
		tq.temporary = tq.permanent =
			tribe.get_ware_descr(i)->default_target_quantity();
		tq.last_modified = 0;
		m_ware_target_quantities[i.value()] = tq;
	}
	m_worker_target_quantities = new Target_Quantity[nr_workers.value()];
	for (Ware_Index i = Ware_Index::First(); i < nr_workers; ++i) {
		Target_Quantity tq;
		tq.temporary = tq.permanent =
			tribe.get_worker_descr(i)->default_target_quantity();
		tq.last_modified = 0;
		m_worker_target_quantities[i.value()] = tq;
	}

	m_router = new Router();
}

Economy::~Economy()
{
	assert(!m_rebuilding);

	m_owner.remove_economy(*this);

	if (m_requests.size())
		log("Warning: Economy still has requests left on destruction\n");
	if (m_flags.size())
		log("Warning: Economy still has flags left on destruction\n");
	if (m_warehouses.size())
		log("Warning: Economy still has warehouses left on destruction\n");

	delete[] m_ware_target_quantities;
	delete[] m_worker_target_quantities;

	delete m_router;
}


/**
 * \return an arbitrary flag in this economy, or 0 if no flag exists
 */
Flag & Economy::get_arbitrary_flag()
{
	assert(m_flags.size());
	return *m_flags[0];
}

/**
 * Two flags have been connected; check whether their economies should be
 * merged.
 * Since we could merge into both directions, we preserve the economy that is
 * currently bigger (should be more efficient).
*/
void Economy::check_merge(Flag & f1, Flag & f2)
{
	Economy * e1 = f1.get_economy();
	Economy * e2 = f2.get_economy();
	if (e1 != e2) {
		if (e1->get_nrflags() < e2->get_nrflags())
			std::swap(e1, e2);
		e1->_merge(*e2);
	}
}

/// If the two flags can no longer reach each other (pathfinding!), the economy
/// gets split.
///
/// Should we create the new economy starting at f1 or f2? Ideally, we'd split
/// off in a way that the new economy will be relatively small.
///
/// Unfortunately, there's no easy way to tell in advance which of the two
/// resulting economies will be smaller (the problem is even NP-complete), so
/// we use a heuristic.
/// NOTE There is a way; parallel counting. If for example one has size 100 and
/// NOTE the other has size 1, we start counting (to 1) in the first. Then we
/// NOTE switch to the second and count (to 1) there. Then we switch to the
/// NOTE first and count (to 2) there. Then we switch to the second and have
/// NOTE nothing more to count. We are done and know that the second is not
/// NOTE larger than the first.
/// NOTE
/// NOTE We have not done more than n * (s + 1) counting operations, where n is
/// NOTE the number of parallel entities (2 in this example) and s is the size
/// NOTE of the smallest entity (1 in this example). So instead of risking to
/// NOTE make a bad guess and change 100 entities, we count 4 and change 1.
/// NOTE                                                                --sigra
///
/// Using f2 is just a guess, but if anything f2 is probably best: it will be
/// the end point of a road. Since roads are typically built from the center of
/// a country outwards, and since splits are more likely to happen outwards,
/// the economy at the end point is probably smaller in average. It's all just
/// guesswork though ;)
/// NOTE Many roads are built when a new building has just been placed. For
/// NOTE those cases, the guess is bad because the user typically builds from
/// NOTE the new building's flag to some existing flag (at the headquarter or
/// NOTE somewhere in his larger road network). This is also what the user
/// NOTE interface makes the player do when it enters roadbuilding mode after
/// NOTE placing a flag that is not connected with roads.               --sigra
void Economy::check_split(Flag & f1, Flag & f2)
{
	assert(&f1 != &f2);
	assert(f1.get_economy() == f2.get_economy());

	Economy & e = *f1.get_economy();

	if (not e.find_route(f1, f2, 0, false))
		e._split(f2);
}


/**
 * Calcaluate a route between two flags.
 *
 * This functionality has been moved to Router(). This is currently
 * merely a delegator.
*/
bool Economy::find_route
	(Flag & start, Flag & end,
	 Route * const route,
	 bool    const wait,
	 int32_t const cost_cutoff)
{
	assert(start.get_economy() == this);
	assert(end  .get_economy() == this);

	Map & map = owner().egbase().map();

	std::vector<RoutingNode *> & nodes =
		*reinterpret_cast<std::vector<RoutingNode *> *>(&m_flags);

	return
		m_router->find_route(start, end, route, wait, cost_cutoff, map, nodes);
}


/**
 * Add a flag to the flag array.
 * Only call from Flag init and split/merger code!
*/
void Economy::add_flag(Flag & flag)
{
	assert(flag.get_economy() == 0);

	m_flags.push_back(&flag);
	flag.set_economy(this);

	flag.reset_path_finding_cycle();
}

/**
 * Remove a flag from the flag array.
 * Only call from Flag cleanup and split/merger code!
*/
void Economy::remove_flag(Flag & flag)
{
	assert(flag.get_economy() == this);

	_remove_flag(flag);

	// automatically delete the economy when it becomes empty.
	if (m_flags.empty())
		delete this;
}

/**
 * Remove the flag, but don't delete the economy automatically.
 * This is called from the merge code.
*/
void Economy::_remove_flag(Flag & flag)
{
	flag.set_economy(0);

	// fast remove
	container_iterate(Flags, m_flags, i)
		if (*i.current == &flag) {
			*i.current = *(i.get_end() - 1);
			return m_flags.pop_back();
		}
	throw wexception("trying to remove nonexistent flag");
}

/**
 * Set the target quantities for the given Ware_Index to the
 * numbers given in permanent and temporary. Also update the last
 * modification time.
 *
 * This is called from Cmd_ResetTargetQuantity and Cmd_SetTargetQuantity
 */
void Economy::set_ware_target_quantity
	(Ware_Index const ware_type,
	 uint32_t   const permanent,
	 uint32_t   const temporary,
	 Time       const mod_time)
{
	Target_Quantity & tq = m_ware_target_quantities[ware_type.value()];
	tq.temporary = temporary;
	tq.permanent = permanent;
	tq.last_modified = mod_time;
}


void Economy::set_worker_target_quantity
	(Ware_Index const ware_type,
	 uint32_t   const permanent,
	 uint32_t   const temporary,
	 Time       const mod_time)
{
	Target_Quantity & tq = m_worker_target_quantities[ware_type.value()];
	tq.temporary = temporary;
	tq.permanent = permanent;
	tq.last_modified = mod_time;
}


/**
 * Call this whenever some entity created a ware, e.g. when a lumberjack
 * has felled a tree.
 * This is also called when a ware is added to the economy through trade or
 * a merger.
*/
void Economy::add_wares(Ware_Index const id, uint32_t const count)
{
	//log("%p: add(%i, %i)\n", this, id, count);

	m_wares.add(id, count);

	// TODO: add to global player inventory?
}
void Economy::add_workers(Ware_Index const id, uint32_t const count)
{
	//log("%p: add(%i, %i)\n", this, id, count);

	m_workers.add(id, count);

	// TODO: add to global player inventory?
}

/**
 * Call this whenever a ware is destroyed or consumed, e.g. food has been
 * eaten or a warehouse has been destroyed.
 * This is also called when a ware is removed from the economy through trade or
 * a split of the Economy.
*/
void Economy::remove_wares(Ware_Index const id, uint32_t const count)
{
	assert(id < m_owner.tribe().get_nrwares());
	//log("%p: remove(%i, %i) from %i\n", this, id, count, m_wares.stock(id));

	m_wares.remove(id, count);

	Target_Quantity & tq = m_ware_target_quantities[id.value()];
	tq.temporary =
		tq.temporary <= tq.permanent + count ?
		tq.permanent : tq.temporary - count;

	// TODO: remove from global player inventory?
}

/**
 * Call this whenever a worker is destroyed.
 * This is also called when a worker is removed from the economy through
 * a split of the Economy.
 */
void Economy::remove_workers(Ware_Index const id, uint32_t const count)
{
	//log("%p: remove(%i, %i) from %i\n", this, id, count, m_workers.stock(id));

	m_workers.remove(id, count);

	Target_Quantity & tq = m_worker_target_quantities[id.value()];
	tq.temporary =
		tq.temporary <= tq.permanent + count ?
		tq.permanent : tq.temporary - count;

	// TODO: remove from global player inventory?
}

/**
 * Add the warehouse to our list of warehouses.
 * This also adds the wares in the warehouse to the economy. However, if wares
 * are added to the warehouse in the future, add_wares() must be called.
*/
void Economy::add_warehouse(Warehouse & wh)
{
	m_warehouses.push_back(&wh);
}

/**
 * Remove the warehouse and its wares from the economy.
*/
void Economy::remove_warehouse(Warehouse & wh)
{
	for (size_t i = 0; i < m_warehouses.size(); ++i)
		if (m_warehouses[i] == &wh) {
			m_warehouses[i] = *m_warehouses.rbegin();
			m_warehouses.pop_back();
			return;
		}


	//  This assert was modified, since on loading, warehouses might try to
	//  remove themselves from their own economy, though they weren't added
	//  (since they weren't initialized)
	assert(m_warehouses.empty());
}

/**
 * Consider the request, try to fulfill it immediately or queue it for later.
 * Important: This must only be called by the \ref Request class.
*/
void Economy::add_request(Request & req)
{
	assert(req.is_open());
	assert(!_has_request(req));

	assert(&owner());

	m_requests.push_back(&req);

	// Try to fulfill the request
	_start_request_timer();
}

/**
 * \return true if the given Request is registered with the \ref Economy, false
 * otherwise
*/
bool Economy::_has_request(Request & req)
{
	return
		std::find(m_requests.begin(), m_requests.end(), &req)
		!=
		m_requests.end();
}

/**
 * Remove the request from this economy.
 * Important: This must only be called by the \ref Request class.
*/
void Economy::remove_request(Request & req)
{
	RequestList::iterator const it =
		std::find(m_requests.begin(), m_requests.end(), &req);

	if (it == m_requests.end()) {
		log("WARNING: remove_request(%p) not in list\n", &req);
		return;
	}

	*it = *m_requests.rbegin();

	m_requests.pop_back();
}

/**
 * Add a supply to our list of supplies.
*/
void Economy::add_supply(Supply & supply)
{
	m_supplies.add_supply(supply);
	_start_request_timer();
}


/**
 * Remove a supply from our list of supplies.
*/
void Economy::remove_supply(Supply & supply)
{
	m_supplies.remove_supply(supply);
}


bool Economy::needs_ware(Ware_Index const ware_type) const {
	size_t const nr_supplies = m_supplies.get_nrsupplies();
	uint32_t const t = ware_target_quantity(ware_type).temporary;
	uint32_t quantity = 0;
	for (size_t i = 0; i < nr_supplies; ++i)
		if (upcast(WarehouseSupply const, warehouse_supply, &m_supplies[i])) {
			quantity += warehouse_supply->stock_wares(ware_type);
			if (t <= quantity)
				return false;
		}
	return true;
}


bool Economy::needs_worker(Ware_Index const worker_type) const {
	size_t const nr_supplies = m_supplies.get_nrsupplies();
	uint32_t const t = worker_target_quantity(worker_type).temporary;
	uint32_t quantity = 0;
	for (size_t i = 0; i < nr_supplies; ++i)
		if (upcast(WarehouseSupply const, warehouse_supply, &m_supplies[i])) {
			quantity += warehouse_supply->stock_workers(worker_type);
			if (t <= quantity)
				return false;
		}
	return true;
}


/**
 * Add e's flags to this economy.
 *
 * Also transfer all wares and wares request. Try to resolve the new ware
 * requests if possible.
*/
void Economy::_merge(Economy & e)
{
	for (Ware_Index::value_t i = m_owner.tribe().get_nrwares().value(); i;) {
		--i;
		Target_Quantity other_tq = e.m_ware_target_quantities[i];
		Target_Quantity & this_tq = m_ware_target_quantities[i];
		if (this_tq.last_modified < other_tq.last_modified)
			this_tq = other_tq;
	}
	for (Ware_Index::value_t i = m_owner.tribe().get_nrworkers().value(); i;) {
		--i;
		Target_Quantity other_tq = e.m_worker_target_quantities[i];
		Target_Quantity & this_tq = m_worker_target_quantities[i];
		if (this_tq.last_modified < other_tq.last_modified)
			this_tq = other_tq;
	}

	//  If the options window for e is open, but not the one for *this, the user
	//  should still have an options window after the merge. Create an options
	//  window for *this where the options window for e is, to give the user
	//  some continuity.
	if
		(e.m_optionswindow_registry.window and
		 not m_optionswindow_registry.window)
	{
		m_optionswindow_registry.x = e.m_optionswindow_registry.x;
		m_optionswindow_registry.x = e.m_optionswindow_registry.x;
		show_options_window();
	}


	m_rebuilding = true;

	// Be careful around here. The last e->remove_flag() will cause the other
	// economy to delete itself.
	for (std::vector<Flag *>::size_type i = e.get_nrflags() + 1; --i;) {
		assert(i == e.get_nrflags());

		Flag & flag = *e.m_flags[0];

		e._remove_flag(flag);
		add_flag(flag);
	}

	// Fix up Supply/Request after rebuilding
	m_rebuilding = false;

	// implicitly delete the economy
	delete &e;
}

/// Flag initial_flag and all its direct and indirect neighbours are put into a
/// new economy.
void Economy::_split(Flag & initial_flag)
{
	Economy & e = *new Economy(m_owner);

	for (Ware_Index::value_t i = m_owner.tribe().get_nrwares  ().value(); i;) {
		--i;
		e.m_ware_target_quantities[i] = m_ware_target_quantities[i];
	}
	for (Ware_Index::value_t i = m_owner.tribe().get_nrworkers().value(); i;) {
		--i;
		e.m_worker_target_quantities[i] = m_worker_target_quantities[i];
	}

	m_rebuilding = true;
	e.m_rebuilding = true;

	// Use a vector instead of a set to ensure parallel simulation
	std::vector<Flag *> open;

	open.push_back(&initial_flag);
	while (open.size()) {
		Flag & flag = **open.rbegin();
		open.pop_back();

		if (flag.get_economy() != this)
			continue;

		// move this flag to the new economy
		remove_flag(flag);
		e.add_flag(flag);

		//  check all neighbours; if they aren't in the new economy yet, add
		//  them to the list (note: roads and buildings are reassigned via
		//  Flag::set_economy)
		RoutingNodeNeighbours neighbours;
		flag.get_neighbours(neighbours);

		for (uint32_t i = 0; i < neighbours.size(); ++i) {
			/// \todo the next line shouldn't need any casts at all
			Flag & n = *dynamic_cast<Flag *>(neighbours[i].get_neighbour());

			if (n.get_economy() == this)
				open.push_back(&n);
		}
	}

	// Fix Supply/Request after rebuilding
	m_rebuilding = false;
	e.m_rebuilding = false;

	// As long as rebalance commands are tied to specific flags, we
	// need this, because the flag that rebalance commands for us were
	// tied to might have been moved into the other economy
	_start_request_timer();
}

/**
 * Make sure the request timer is running.
 */
void Economy::_start_request_timer(int32_t const delta)
{
	if (upcast(Game, game, &m_owner.egbase()))
		game->cmdqueue().enqueue
			(new Cmd_Call_Economy_Balance
			 	(game->get_gametime() + delta, this, m_request_timerid));
}


/**
 * Find the supply that is best suited to fulfill the given request.
 * \return 0 if no supply is found, the best supply otherwise
*/
Supply * Economy::_find_best_supply
	(Game & game, Request const & req, int32_t & cost)
{
	assert(req.is_open());

	Route buf_route0, buf_route1;
	Supply * best_supply = 0;
	Route  * best_route  = 0;
	int32_t  best_cost   = -1;
	Flag & target_flag = req.target_flag();

	for (size_t i = 0; i < m_supplies.get_nrsupplies(); ++i) {
		Supply & supp = m_supplies[i];

		// idle requests only get active supplies
		if (req.is_idle() and not supp.is_active()) {
			/* unless the warehouse REALLY needs the supply */
			if (req.get_priority(0) > 100) { //  100 is the 'real idle' priority
				//check if the supply is at current target
				if (&target_flag == &supp.get_position(game)->base_flag()) {
					//assert(false);
					continue;
				}
			} else if (not supp.is_active()) {
				continue;
			}
		}

		// Check requirements
		if (!supp.nr_supplies(game, req))
			continue;

		Route * const route =
			best_route != &buf_route0 ? &buf_route0 : &buf_route1;
		// will be cleared by find_route()

		if
			(!
			 find_route
			 	(supp.get_position(game)->base_flag(),
			 	 target_flag,
			 	 route,
			 	 false,
			 	 best_cost))
		{
			if (!best_route)
				throw wexception
					("Economy::find_best_supply: COULD NOT FIND A ROUTE!");
			continue;
		}

		best_supply = &supp;
		best_route = route;
		best_cost = route->get_totalcost();
	}

	if (!best_route)
		return 0;

	cost = best_cost;
	return best_supply;
}

struct RequestSupplyPair {
	bool is_item;
	bool is_worker;
	Ware_Index ware;
	TrackPtr<Request> request;
	TrackPtr<Supply>  supply;
	int32_t priority;

	/**
	 * pairid is an explicit tie-breaker for comparison.
	 *
	 * Without it, the pair priority queue would use an implicit, system
	 * dependent tie breaker, which in turn causes desyncs.
	 */
	uint32_t pairid;

	struct Compare {
		bool operator()
			(RequestSupplyPair const & p1, RequestSupplyPair const & p2)
		{
			return
				p1.priority == p2.priority ? p1.pairid < p2.pairid :
				p1.priority <  p2.priority;
		}
	};
};

typedef
	std::priority_queue
	<RequestSupplyPair,
	std::vector<RequestSupplyPair>,
	RequestSupplyPair::Compare>
	RSPairQueue;

struct RSPairStruct {
	RSPairQueue queue;
	uint32_t pairid;
	int32_t nexttimer;

	RSPairStruct() : pairid(0) {}
};

/**
 * Walk all Requests and find potential transfer candidates.
*/
void Economy::_process_requests(Game & game, RSPairStruct & s)
{
	//  TODO This function should be called from time to time.
	_create_requested_workers (game);

	container_iterate_const(RequestList, m_requests, i) {
		Request & req = **i.current;

		// We somehow get desynced request lists that don't trigger desync
		// alerts, so add info to the sync stream here.
		{
			::StreamWrite & ss = game.syncstream();
			ss.Unsigned8 (req.get_type  ());
			ss.Unsigned8 (req.get_index ().value());
			ss.Unsigned32(req.target    ().serial());
		}

		int32_t cost; // estimated time in milliseconds to fulfill Request
		Supply * const supp = _find_best_supply(game, req, cost);

		if (!supp)
			continue;

		if (!req.is_idle() and not supp->is_active()) {
			// Calculate the time the building will be forced to idle waiting
			// for the request
			int32_t const idletime =
				game.get_gametime() + 15000 + 2 * cost - req.get_required_time();
			// If the building wouldn't have to idle, we wait with the request
			if (idletime < -200) {
				if (s.nexttimer < 0 || s.nexttimer > -idletime)
					s.nexttimer = -idletime;

				continue;
			}
		}

		int32_t const priority = req.get_priority (cost);
		if (priority < 0)
			continue;

		// Otherwise, consider this request/supply pair for queueing
		RequestSupplyPair rsp;

		rsp.is_item = false;
		rsp.is_worker = false;

		switch (req.get_type()) {
		case Request::WARE:    rsp.is_item    = true; break;
		case Request::WORKER:  rsp.is_worker  = true; break;
		default:
			assert(false);
		}

		rsp.ware = req.get_index();
		rsp.request  = &req;
		rsp.supply = supp;
		rsp.priority = priority;
		rsp.pairid = ++s.pairid;

		s.queue.push(rsp);
	}
}


/**
 * Walk all Requests and find requests of workers than aren't supplied. Then
 * try to create the worker at warehouses.
*/
void Economy::_create_requested_workers(Game & game)
{
	/*
		Find the request of workers that can not be supplied
	*/
	if (warehouses().size()) {
		Tribe_Descr const & tribe = owner().tribe();
		container_iterate_const(RequestList, m_requests, j) {
			Request const & req = **j.current;

			if (!req.is_idle() && req.get_type() == Request::WORKER) {
				Ware_Index const index = req.get_index();
				Worker_Descr const & w_desc = *tribe.get_worker_descr(index);

				for (size_t i = 0; i < m_supplies.get_nrsupplies(); ++i)
					if (m_supplies[i].nr_supplies(game, req))
						goto requested_worker_exists;

				// If there aren't enough supplies...
				if (owner().is_worker_type_allowed(index)) {
					if (not w_desc.is_buildable()) {
						log
							("Economy::_create_requested_workers: ERROR: "
							 "attempting to create worker of non-buildable type "
							 "%s\n",
							 w_desc.descname().c_str());
					}
					assert(w_desc.is_buildable());
					bool created_worker = false;
					for (uint32_t n_wh = 0; n_wh < warehouses().size(); ++n_wh) {
						if (m_warehouses[n_wh]->can_create_worker(game, index)) {
							m_warehouses[n_wh]->create_worker(game, index);
							created_worker = true;
							break;
						} // if (m_warehouses[n_wh]
					}
					if (! created_worker) {
						uint32_t nth_wh = 0;
						if (warehouses().size() > 1) {
							// Find nearest warehouse!
							// NOTE  Just a dummy implementation to ensure that each
							// NOTE  call of this function sets an request for the same
							// NOTE  warehouse - should of coures be improved further.
							Coords const tac = req.target_flag().get_position();
							Coords whc = m_warehouses[0]->base_flag().get_position();
							int32_t current = (tac.x - whc.x) * (tac.y - whc.y);
							current = current < 1 ? (- current) : current;
							for (uint32_t i = 0; i < warehouses().size(); ++i) {
								whc = m_warehouses[i]->base_flag().get_position();
								int32_t cost = (tac.x - whc.x) * (tac.y - whc.y);
								cost = cost < 0 ? (- cost) : cost;
								if (current > cost) {
									current = cost;
									nth_wh = i;
								}
							}
						}
						Warehouse & nearest = *m_warehouses[nth_wh];
						Worker_Descr::Buildcost const & cost = w_desc.buildcost();
						container_iterate_const
							(Worker_Descr::Buildcost, cost, bc_it)
							if
								(Ware_Index const w_id =
								 	tribe.ware_index(bc_it.current->first.c_str()))
								nearest.set_needed(w_id, bc_it.current->second);
					}
				} //else
					//log
						//("Economy::_create_requested_workers: Could not create %s "
						 //"for player %u because it is forbidden\n",
						 //w_desc.descname().c_str(), owner().player_number());
			} // if (req->is_open())
			requested_worker_exists:;
		}
	}
}

/**
 * Balance Requests and Supplies by collecting and weighing pairs, and
 * starting transfers for them.
*/
void Economy::balance_requestsupply(uint32_t const timerid)
{
	if (m_request_timerid != timerid)
		return;
	++m_request_timerid;

	RSPairStruct rsps;
	rsps.nexttimer = -1;

	//  Try to fulfill non-idle Requests.
	Game & game = ref_cast<Game, Editor_Game_Base>(owner().egbase());
	_process_requests(game, rsps);

	//  Now execute request/supply pairs.
	while (rsps.queue.size()) {
		RequestSupplyPair rsp = rsps.queue.top();

		rsps.queue.pop();

		if
			(!rsp.request                ||
			 !rsp.supply                 ||
			 !_has_request(*rsp.request) ||
			 !rsp.supply->nr_supplies(game, *rsp.request))
		{
			rsps.nexttimer = 200;
			continue;
		}

		rsp.request->start_transfer(game, *rsp.supply);
		rsp.request->set_last_request_time(game.get_gametime());

		//  for multiple wares
		if (rsp.request && _has_request(*rsp.request))
			rsps.nexttimer = 200;
	}

	if (rsps.nexttimer > 0) //  restart the timer, if necessary
		_start_request_timer(rsps.nexttimer);
}

}

