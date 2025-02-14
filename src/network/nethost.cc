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

#include "nethost.h"

#include "build_info.h"
#include "chat.h"
#include "computer_player.h"
#include "ui_fsmenu/launchgame.h"
#include "logic/game.h"
#include "wui/game_tips.h"
#include "i18n.h"
#include "io/fileread.h"
#include "io/filesystem/layered_filesystem.h"
#include "wui/interactive_dedicated_server.h"
#include "wui/interactive_player.h"
#include "wui/interactive_spectator.h"
#include "md5.h"
#include "network_ggz.h"
#include "network_lan_promotion.h"
#include "network_protocol.h"
#include "network_system.h"
#include "logic/player.h"
#include "logic/playercommand.h"
#include "profile/profile.h"
#include "logic/tribe.h"
#include "wexception.h"
#include "wlapplication.h"

#include "ui_basic/progresswindow.h"



struct HostGameSettingsProvider : public GameSettingsProvider {
	HostGameSettingsProvider(NetHost * const _h) : h(_h) {}

	virtual void setScenario(bool) {}; //  FIXME no scenario for multiplayer

	virtual GameSettings const & settings() {return h->settings();}

	virtual bool canChangeMap() {return true;}
	virtual bool canChangePlayerState(uint8_t const number) {
		return number != settings().playernum;
	}
	virtual bool canChangePlayerTribe(uint8_t const number) {
		if (number == settings().playernum)
			return true;
		if (number >= settings().players.size())
			return false;
		return
			settings().players.at(number).state == PlayerSettings::stateComputer;
	}
	virtual bool canChangePlayerInit(uint8_t const number) {
		return number < settings().players.size();
	}

	virtual bool canLaunch() {return h->canLaunch();}

	virtual void setMap
		(std::string const &       mapname,
		 std::string const &       mapfilename,
		 uint32_t            const maxplayers,
		 bool                const savegame = false)
	{
		h->setMap(mapname, mapfilename, maxplayers, savegame);
	}
	virtual void setPlayerState
		(uint8_t const number, PlayerSettings::State const state)
	{
		if (number >= settings().players.size())
			return;

		h->setPlayerState(number, state);
	}
	virtual void nextPlayerState(uint8_t const number) {
		if
			(number == settings().playernum ||
			 number >= settings().players.size())
			return;

		PlayerSettings::State newstate = PlayerSettings::stateClosed;
		switch (h->settings().players.at(number).state) {
		case PlayerSettings::stateClosed:
			newstate = PlayerSettings::stateOpen;
			break;
		case PlayerSettings::stateOpen:
		case PlayerSettings::stateHuman:
		case PlayerSettings::stateComputer:
			Computer_Player::ImplementationVector const & impls =
				Computer_Player::getImplementations();
			Computer_Player::ImplementationVector::const_iterator it =
				impls.begin();
			if (h->settings().players.at(number).ai.empty()) {
				setPlayerAI(number, (*it)->name);
				newstate = PlayerSettings::stateComputer;
				break;
			}
			do {
				++it;
				if ((*(it - 1))->name == h->settings().players.at(number).ai)
					break;
			} while (it != impls.end());
			if (it == impls.end()) {
				setPlayerAI(number, std::string());
				newstate = PlayerSettings::stateOpen;
			} else {
				setPlayerAI(number, (*it)->name);
				newstate = PlayerSettings::stateComputer;
			}
			break;
		}

		h->setPlayerState(number, newstate, true);
	}

	virtual void setPlayerTribe(uint8_t const number, std::string const & tribe)
	{
		if (number >= h->settings().players.size())
			return;

		if
			(number == settings().playernum ||
			 settings().players.at(number).state == PlayerSettings::stateComputer)
			h->setPlayerTribe(number, tribe);
	}

	virtual void setPlayerInit(uint8_t const number, uint8_t const index) {
		if (number >= h->settings().players.size())
			return;

		h->setPlayerInit(number, index);
	}

	virtual void setPlayerAI(uint8_t number, std::string const & name) {
		h->setPlayerAI(number, name);
	}

	virtual void setPlayerName(uint8_t const number, std::string const & name) {
		if (number >= h->settings().players.size())
			return;
		h->setPlayerName(number, name);
	}

	virtual void setPlayer(uint8_t const number, PlayerSettings const ps) {
		if (number >= h->settings().players.size())
			return;
		h->setPlayer(number, ps);
	}

	virtual void setPlayerReady
		(uint8_t const number, bool const ready)
	{
		if (number >= h->settings().players.size())
			return;

		if (number == settings().playernum)
			h->setPlayerReady(number, ready);
	}

	virtual bool getPlayerReady(uint8_t const number) {
		if (number >= h->settings().players.size())
			return false;
		return h->getPlayerReady(number);
	}

	virtual void setPlayerNumber(uint8_t const number) {
		if
			(number == UserSettings::none() or
			 number < h->settings().players.size())
			h->setPlayerNumber(number);
	}

private:
	NetHost * h;
};

struct HostChatProvider : public ChatProvider {
	HostChatProvider(NetHost * const _h) : h(_h) {}

	void send(std::string const & msg) {
		ChatMessage c;
		c.time = time(0);
		c.playern = h->getLocalPlayerposition();
		c.sender = h->getLocalPlayername();
		c.msg = msg;
		if (c.msg.size() && *c.msg.begin() == '@') {
			// Personal message
			std::string::size_type const space = c.msg.find(' ');
			if (space >= c.msg.size() - 1)
				return;
			c.recipient = c.msg.substr(1, space - 1);
			c.msg = c.msg.substr(space + 1);
		}
		h->send(c);
	}

	std::vector<ChatMessage> const & getMessages() const {
		return messages;
	}

	void receive(ChatMessage const & msg) {
		messages.push_back(msg);
		ChatProvider::send(msg);
	}

private:
	NetHost                * h;
	std::vector<ChatMessage> messages;
};

struct Client {
	TCPsocket sock;
	Deserializer deserializer;
	uint8_t playernum;
	uint8_t usernum;
	std::string build_id;
	md5_checksum syncreport;
	bool syncreport_arrived;
	int32_t time; // last time report
	uint32_t desiredspeed;
};

struct NetHostImpl {
	GameSettings settings;
	std::string localplayername;
	uint32_t localdesiredspeed;
	HostChatProvider chat;

	LAN_Game_Promoter * promoter;
	TCPsocket svsock;
	SDLNet_SocketSet sockset;

	/// List of connected clients. Note that clients are not in the same
	/// order as players. In fact, a client must not be assigned to a player.
	std::vector<Client> clients;

	/// The game itself; only non-null while game is running
	Widelands::Game * game;

	/// If we were to send out a plain networktime packet, this would be the
	/// time. However, we have not yet committed to this networktime.
	int32_t pseudo_networktime;
	int32_t last_heartbeat;

	/// The networktime we committed to by sending it across the network.
	int32_t committed_networktime;

	/// This is the time for local simulation
	NetworkTime time;

	/// Whether we're waiting for all clients to report back.
	bool waiting;
	int32_t lastframe;

	/**
	 * The speed, in milliseconds per second, that is effective as long
	 * as we're not \ref waiting.
	 */
	uint32_t networkspeed;

	/// All currently running computer players, *NOT* in one-one correspondence
	/// with \ref Player objects
	std::vector<Computer_Player *> computerplayers;

	/// \c true if a syncreport is currently in flight
	bool syncreport_pending;
	int32_t syncreport_time;
	md5_checksum syncreport;
	bool syncreport_arrived;

	NetHostImpl(NetHost * const h) : chat(h) {}
};

NetHost::NetHost (std::string const & playername, bool ggz)
: d(new NetHostImpl(this)), use_ggz(ggz)
{
	log("[Host] starting up.\n");

	if (ggz) {
#if HAVE_GGZ
		NetGGZ::ref().launch();
#endif
	}

	d->localplayername = playername;

	// create a listening socket
	IPaddress myaddr;
	SDLNet_ResolveHost (&myaddr, 0, WIDELANDS_PORT);
	d->svsock = SDLNet_TCP_Open(&myaddr);

	d->sockset = SDLNet_AllocSocketSet(16);
	d->promoter = new LAN_Game_Promoter();
	d->game = 0;
	d->pseudo_networktime = 0;
	d->waiting = true;
	d->networkspeed = 1000;
	d->localdesiredspeed = 1000;
	d->syncreport_pending = false;
	d->syncreport_time = 0;

	Widelands::Tribe_Descr::get_all_tribe_infos(d->settings.tribes);
	setMultiplayerGameSettings();
	d->settings.playernum = UserSettings::none();
	d->settings.usernum = 0;
	UserSettings hostuser;
	hostuser.name = playername;
	hostuser.position = UserSettings::none();
	d->settings.users.push_back(hostuser);
	file = 0; //  Initialize as 0 pointer - unfortunately needed in struct.
}

NetHost::~NetHost ()
{
	clearComputerPlayers();

	while (d->clients.size() > 0) {
		disconnectClient(0, _("Server has left the game."));
		reaper();
	}

	SDLNet_FreeSocketSet (d->sockset);

	// close all open sockets
	if (d->svsock != 0)
		SDLNet_TCP_Close (d->svsock);

	delete d->promoter;
	delete d;
}

std::string const & NetHost::getLocalPlayername() const
{
	return d->localplayername;
}

int16_t NetHost::getLocalPlayerposition()
{
	return d->settings.users.at(0).position;
}

void NetHost::clearComputerPlayers()
{
	for (uint32_t i = 0; i < d->computerplayers.size(); ++i)
		delete d->computerplayers.at(i);
	d->computerplayers.clear();
}

void NetHost::initComputerPlayer(Widelands::Player_Number p)
{
	d->computerplayers.push_back
		(Computer_Player::getImplementation(d->game->get_player(p)->getAI())
		 ->instantiate(*d->game, p));
}

void NetHost::initComputerPlayers()
{
	const Widelands::Player_Number nr_players = d->game->map().get_nrplayers();
	iterate_players_existing(p, nr_players, *d->game, plr) {
		if (p == d->settings.playernum + 1)
			continue;

		uint32_t client;
		for (client = 0; client < d->clients.size(); ++client)
			if (d->clients.at(client).playernum + 1 == p)
				break;

		if (client >= d->clients.size())
			initComputerPlayer(p);
	}
}

void NetHost::run(bool const autorun)
{
	HostGameSettingsProvider hp(this);
	{
		Fullscreen_Menu_LaunchGame lgm(&hp, this, autorun);
		lgm.setChatProvider(d->chat);
		const int32_t code = lgm.run();

		if (code <= 0)
			return;
	}

#if HAVE_GGZ
	// if this is a ggz game, tell the metaserver that the game started
	if (use_ggz)
		NetGGZ::ref().send_game_playing();
#endif

	for (uint32_t i = 0; i < d->clients.size(); ++i) {
		if (d->clients.at(i).playernum == UserSettings::notConnected())
			disconnectClient
				(i, _("The game has started just after you tried to connect."));
	}

	SendPacket s;
	s.Unsigned8(NETCMD_LAUNCH);
	broadcast(s);

	Widelands::Game game;
#ifdef DEBUG
	game.set_write_syncstream(true);
#endif

	try {
		UI::ProgressWindow loaderUI("pics/progress.png");
		std::vector<std::string> tipstext;
		tipstext.push_back("general_game");
		tipstext.push_back("multiplayer");
		try {
			tipstext.push_back(hp.getPlayersTribe());
		} catch (GameSettingsProvider::No_Tribe) {
		}
		GameTips tips (loaderUI, tipstext);

		loaderUI.step(_("Preparing game"));

		uint8_t const pn = d->settings.playernum + 1;
		d->game = &game;
		game.set_game_controller(this);
		Interactive_GameBase * igb;
		if (0 < pn and pn <= UserSettings::highestPlayernum())
			igb =
				new Interactive_Player
					(game, g_options.pull_section("global"), pn, false, true);
		else if (!autorun)
			igb =
				new Interactive_Spectator
					(*d->game, g_options.pull_section("global"), true);
		else
			igb =
				new Interactive_DServer
					(*d->game, g_options.pull_section("global"));
		igb->set_chat_provider(d->chat);
		game.set_ibase(igb);
		if (!d->settings.savegame) // new game
			game.init_newgame(loaderUI, d->settings);
		else                      // savegame
			game.init_savegame(loaderUI, d->settings);
		d->pseudo_networktime = game.get_gametime();
		d->time.reset(d->pseudo_networktime);
		d->lastframe = WLApplication::get()->get_time();
		d->last_heartbeat = d->lastframe;

		d->committed_networktime = d->pseudo_networktime;

		for (uint32_t i = 0; i < d->clients.size(); ++i)
			d->clients.at(i).time = d->committed_networktime - 1;

		// The call to checkHungClients ensures that the game leaves the
		// wait mode when there are no clients
		checkHungClients();
		initComputerPlayers();
		game.run
			(loaderUI,
			 d->settings.savegame ?
			 Widelands::Game::Loaded : Widelands::Game::NewNonScenario);
#if HAVE_GGZ
		// if this is a ggz game, tell the metaserver that the game is done.
		if (use_ggz)
			NetGGZ::ref().send_game_done();
#endif
		clearComputerPlayers();
	} catch (...) {
		WLApplication::emergency_save(game);
		clearComputerPlayers();
		d->game = 0;

		while (d->clients.size() > 0) {
			disconnectClient
				(0, _("Server has crashed and performed an emergency save."));
			reaper();
		}
		throw;
	}
	d->game = 0;
}

void NetHost::think()
{
	handle_network();

	if (d->game) {
		int32_t curtime = WLApplication::get()->get_time();
		int32_t delta = curtime - d->lastframe;
		d->lastframe = curtime;

		if (!d->waiting) {
			int32_t diff = (delta * d->networkspeed) / 1000;
			d->pseudo_networktime += diff;
		}

		d->time.think(realSpeed()); // must be called even when d->waiting

		if (d->pseudo_networktime != d->committed_networktime)
		{
			if (d->pseudo_networktime - d->committed_networktime < 0) {
				d->pseudo_networktime = d->committed_networktime;
			} else if (curtime - d->last_heartbeat >= SERVER_TIMESTAMP_INTERVAL) {
				d->last_heartbeat = curtime;

				SendPacket s;
				s.Unsigned8(NETCMD_TIME);
				s.Signed32(d->pseudo_networktime);
				broadcast(s);

				committedNetworkTime(d->pseudo_networktime);

				checkHungClients();
			}
		}

		for (uint32_t i = 0; i < d->computerplayers.size(); ++i)
			d->computerplayers.at(i)->think();
	}
}

void NetHost::sendPlayerCommand(Widelands::PlayerCommand & pc)
{
	pc.set_duetime(d->committed_networktime + 1);

	SendPacket s;
	s.Unsigned8(NETCMD_PLAYERCOMMAND);
	s.Signed32(pc.duetime());
	pc.serialize(s);
	broadcast(s);
	d->game->enqueue_command(&pc);

	committedNetworkTime(d->committed_networktime + 1);
}

/**
 * All chat messages go through this function.
 * If it is a normal message it is sent to clients as needed, and it is
 * forwarded to our local \ref ChatProvider.
 * If it is a personal message it will only be send to the recipient and to
 * the sender (to show that the message was actually sent).
 */
void NetHost::send(ChatMessage msg)
{
	if (msg.msg.empty())
		return;

	// Make sure that msg is free of richtext formation tags. Such tags could not
	// just be abused by the user, but could also break the whole text formation.
	// FIXME It would be better to escape < as &lt; and then render that as <
	// FIXME instead of replacing < with { in chat messages.
	container_iterate(std::string, msg.msg, i)
		if (*i.current == '<')
			*i.current = '{';

	if (msg.recipient.empty()) {
		SendPacket s;
		s.Unsigned8(NETCMD_CHAT);
		s.Signed16(msg.playern);
		s.String(msg.sender);
		s.String(msg.msg);
		s.Unsigned8(0);
		broadcast(s);

		d->chat.receive(msg);

		log("[Host]: chat: %s\n", msg.toPlainString().c_str());
	} else { //  personal messages
		SendPacket s;
		s.Unsigned8(NETCMD_CHAT);

		// Is this pm for the host player?
		if (d->localplayername == msg.recipient) {
			d->chat.receive(msg);
			// Write the SendPacket - will be used below to show that the message
			// was received.
			s.Signed16(msg.playern);
			s.String(msg.sender);
			s.String(msg.msg);
			s.Unsigned8(1);
			s.String(msg.recipient);
		} else { //find the recipient
			uint32_t i = 0;
			for (; i < d->settings.users.size(); ++i) {
				UserSettings const & user = d->settings.users.at(i);
				if (user.name == msg.recipient)
					break;
			}
			if (i < d->settings.users.size()) {
				for (wl_const_range<std::vector<Client> > j(d->clients);; ++j)
					if (j.empty()) {
						//  Better no wexception; it would break the whole game.
						log
							("WARNING: user was found but no client is connected to "
							 "it!\n");
						break;
					} else if (j->usernum == i) {
						s.Signed16(msg.playern);
						s.String(msg.sender);
						s.String(msg.msg);
						s.Unsigned8(1);
						s.String(msg.recipient);
						s.send(j->sock);
						break;
					}

				log
					("[Host]: personal chat: from %s to %s\n",
					 msg.sender.c_str(), msg.recipient.c_str());
			} else {
				std::string fail = "Failed to send message: Recipient \"";
				fail += msg.recipient + "\" could not be found!";

				// is host the sender?
				if (d->localplayername == msg.sender) {
					ChatMessage err;
					err.playern = -2; // System message
					err.sender = "";
					err.msg = fail;
					err.recipient = "";
					d->chat.receive(err);
					return; // nothing left to do!
				}
				s.Signed16(-2); // System message
				s.String("");
				s.String(fail);
				s.Unsigned8(0);
			}
		}

		if (msg.sender == msg.recipient) //  he sent himself a private message
			return; //  do not deliver it to him twice

		//Now find the sender and send either the message or the failure notice
		else if (d->localplayername == msg.sender)
			d->chat.receive(msg);
		else { // host is not the sender -> get sender
			uint32_t i = 0;
			for (; i < d->settings.users.size(); ++i) {
				UserSettings const & user = d->settings.users.at(i);
				if (user.name == msg.sender)
					break;
			}
			if (i < d->settings.users.size()) {
				uint32_t j = 0;
				for (; j < d->clients.size(); ++j)
					if (d->clients.at(j).usernum == i)
						break;
				if (j < d->clients.size())
					s.send(d->clients.at(j).sock);
				else
					// Better no wexception it would break the whole game
					log
						("WARNING: user was found but no client is connected to "
						 "it!\n");
			} else
				// Better no wexception it would break the whole game
				log("WARNING: sender could not be found!");
		}
	}
}

void NetHost::sendSystemChat(char const * const fmt, ...)
{
	char buffer[500];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	ChatMessage c;
	c.time = time(0);
	c.msg = buffer;
	c.playern = UserSettings::none(); //  == System message
	// c.sender remains empty to indicate a system message
	send(c);
}

int32_t NetHost::getFrametime()
{
	return d->time.time() - d->game->get_gametime();
}

std::string NetHost::getGameDescription()
{
	char buf[200];
	snprintf
		(buf, sizeof(buf),
		 "network player %i (host)", d->settings.users.at(0).position);
	return buf;
}

GameSettings const & NetHost::settings()
{
	return d->settings;
}

bool NetHost::canLaunch()
{
	if (d->settings.mapname.empty())
		return false;
	if (d->settings.players.size() < 1)
		return false;
	for (size_t i = 0; i < d->settings.players.size(); ++i) {
		if (!getPlayerReady(i)) {
			return false;
		}
	}
	return true;
}

void NetHost::setMap
	(std::string const &       mapname,
	 std::string const &       mapfilename,
	 uint32_t            const maxplayers,
	 bool                const savegame)
{
	d->settings.mapname = mapname;
	d->settings.mapfilename = mapfilename;
	d->settings.savegame = savegame;

	std::vector<PlayerSettings>::size_type oldplayers =
		d->settings.players.size();

	SendPacket s;

	while (oldplayers > maxplayers) {
		--oldplayers;
		for (uint32_t i = 1; i < d->settings.users.size(); ++i)
			if (d->settings.users.at(i).position == oldplayers) {
				d->settings.users.at(i).position = UserSettings::none();

				// for local settings
				uint32_t j = 0;
				for (; j < d->clients.size(); ++j)
					if (d->clients.at(j).usernum == i)
						break;
				d->clients.at(j).playernum = UserSettings::none();

				// Broadcast change
				s.reset();
				s.Unsigned8(NETCMD_SETTING_USER);
				s.Unsigned32(i);
				writeSettingUser(s, i);
				broadcast(s);
			}
	}

	d->settings.players.resize(maxplayers);

	while (oldplayers < maxplayers) {
		PlayerSettings & player = d->settings.players.at(oldplayers);
		player.state                = PlayerSettings::stateOpen;
		player.tribe                = d->settings.tribes.at(0).name;
		player.initialization_index = 0;
		++oldplayers;
	}

	// Broadcast new map info
	s.reset();
	s.Unsigned8(NETCMD_SETTING_MAP);
	writeSettingMap(s);
	broadcast(s);

	// Broadcast new player settings
	s.reset();
	s.Unsigned8(NETCMD_SETTING_ALLPLAYERS);
	writeSettingAllPlayers(s);
	broadcast(s);

	// If possible, offer the map / savegame as transfer
	// TODO not yet able to handle directory type maps / savegames
	if (!g_fs->IsDirectory(mapfilename)) {
		// Read in the file
		FileRead fr;
		fr.Open(*g_fs, mapfilename.c_str());
		if (file)
			delete file;
		file = new NetTransferFile();
		file->filename = mapfilename;
		uint32_t leftparts = file->bytes = fr.GetSize();
		while (leftparts > 0) {
			uint8_t readout
				= (leftparts > NETFILEPARTSIZE) ? NETFILEPARTSIZE : leftparts;
			FilePart fp;
			memcpy(fp.part, fr.Data(readout), readout);
			file->parts.push_back(fp);
			leftparts -= readout;
		}
		std::vector<char> complete(file->bytes);
		fr.SetFilePos(0);
		fr.DataComplete(&complete[0], file->bytes);
		MD5Checksum<FileRead> md5sum;
		md5sum.Data(&complete[0], file->bytes);
		md5sum.FinishChecksum();
		file->md5sum = md5sum.GetChecksum().str();
	}

	s.reset();
	if (writeMapTransferInfo(s, mapfilename))
		broadcast(s);
}

void NetHost::setPlayerState
	(uint8_t const number, PlayerSettings::State const state, bool const host)
{
	if (number >= d->settings.players.size())
		return;

	PlayerSettings & player = d->settings.players.at(number);

	if (player.state == state)
		return;

	SendPacket s;

	if (player.state == PlayerSettings::stateHuman)
		//  0 is host and has no client
		for (uint8_t i = 1; i < d->settings.users.size(); ++i)
			if (d->settings.users.at(i).position == number) {
				d->settings.users.at(i).position = UserSettings::none();
				if (host) //  Did host send the user to lobby?
					sendSystemChat
						(_("Host sent player %s to the lobby!"),
						 d->settings.users.at(i).name.c_str());

				//  for local settings
				for (std::vector<Client>::iterator j = d->clients.begin();; ++j) {
					assert(j != d->clients.end());
					if (j->usernum == i) {
						j->playernum = UserSettings::none();
						break;
					}
				}

				//  broadcast change
				s.Unsigned8(NETCMD_SETTING_USER);
				s.Unsigned32(i);
				writeSettingUser(s, i);
				broadcast(s);

				break;
			}

	player.state = state;

	if (player.state == PlayerSettings::stateComputer)
		player.name = getComputerPlayerName(number);

	// Broadcast change
	s.reset();
	s.Unsigned8(NETCMD_SETTING_PLAYER);
	s.Unsigned8(number);
	writeSettingPlayer(s, number);
	broadcast(s);
}


void NetHost::setPlayerTribe(uint8_t const number, std::string const & tribe)
{
	if (number >= d->settings.players.size())
		return;

	PlayerSettings & player = d->settings.players.at(number);

	if (player.tribe == tribe)
		return;

	container_iterate_const(std::vector<TribeBasicInfo>, d->settings.tribes, i)
		if (i.current->name == player.tribe) {
			player.tribe = tribe;
			if (i.current->initializations.size() <= player.initialization_index)
				player.initialization_index = 0;

			//  broadcast changes
			SendPacket s;
			s.Unsigned8(NETCMD_SETTING_PLAYER);
			s.Unsigned8(number);
			writeSettingPlayer(s, number);
			broadcast(s);
			return;
		}
	log
		("Player %u attempted to change to tribe %s; not a valid tribe\n",
		 number, tribe.c_str());
}

void NetHost::setPlayerInit(uint8_t const number, uint8_t const index)
{
	if (number >= d->settings.players.size())
		return;

	PlayerSettings & player = d->settings.players.at(number);

	if (player.initialization_index == index)
		return;

	container_iterate_const(std::vector<TribeBasicInfo>, d->settings.tribes, i)
		if (i.current->name == player.tribe) {
			if (index < i.current->initializations.size()) {
				player.initialization_index = index;

				//  broadcast changes
				SendPacket s;
				s.Unsigned8(NETCMD_SETTING_PLAYER);
				s.Unsigned8(number);
				writeSettingPlayer(s, number);
				broadcast(s);
				return;
			} else
				log
					("Attempted to change to out-of-range initialization index %u "
					 "for player %u.\n", index, number);
			return;
		}
	assert(false);
}


void NetHost::setPlayerAI(uint8_t number, std::string const & name)
{
	if (number >= d->settings.players.size())
		return;

	PlayerSettings & player = d->settings.players.at(number);
	player.ai = name;

	// Broadcast changes
	SendPacket s;
	s.Unsigned8(NETCMD_SETTING_PLAYER);
	s.Unsigned8(number);
	writeSettingPlayer(s, number);
	broadcast(s);
}


void NetHost::setPlayerName(uint8_t const number, std::string const & name)
{
	if (number >= d->settings.players.size())
		return;

	PlayerSettings & player = d->settings.players.at(number);

	if (player.name == name)
		return;

	player.name = name;

	// Broadcast changes
	SendPacket s;
	s.Unsigned8(NETCMD_SETTING_PLAYER);
	s.Unsigned8(number);
	writeSettingPlayer(s, number);
	broadcast(s);
}


void NetHost::setPlayer(uint8_t const number, PlayerSettings const ps)
{
	if (number >= d->settings.players.size())
		return;

	PlayerSettings & player = d->settings.players.at(number);
	player = ps;

	// Broadcast changes
	SendPacket s;
	s.Unsigned8(NETCMD_SETTING_PLAYER);
	s.Unsigned8(number);
	writeSettingPlayer(s, number);
	broadcast(s);
}

void NetHost::setPlayerNumber(uint8_t const number)
{
	if (number != UserSettings::none() and d->settings.players.size() <= number)
		return;

	d->settings.playernum = number;
	d->settings.users.at(0).position = number;

	// Broadcast changes
	SendPacket s;
	s.Unsigned8(NETCMD_SETTING_USER);
	s.Unsigned32(0);
	writeSettingUser(s, 0);
	broadcast(s);

	setPlayerReady(number, false);
}

void NetHost::setPlayerReady
	(uint8_t const number, bool const ready)
{
	if (number >= d->settings.players.size())
		return;
	d->settings.players.at(number).ready = ready;

	// broadcast changes
	SendPacket s;
	s.Unsigned8(NETCMD_SETTING_PLAYER);
	s.Unsigned8(number);
	writeSettingPlayer(s, number);
	broadcast(s);
	return;
}

bool NetHost::getPlayerReady(uint8_t const number)
{
	return
		d->settings.players.at(number).state == PlayerSettings::stateClosed ||
		d->settings.players.at(number).state == PlayerSettings::stateComputer ||
		(d->settings.players.at(number).state == PlayerSettings::stateHuman &&
		 d->settings.players.at(number).ready);
}

void NetHost::setMultiplayerGameSettings()
{
	d->settings.scenario = false;
	d->settings.multiplayer = true;
}


uint32_t NetHost::realSpeed()
{
	if (d->waiting)
		return 0;
	return d->networkspeed;
}

uint32_t NetHost::desiredSpeed()
{
	return d->localdesiredspeed;
}

void NetHost::setDesiredSpeed(uint32_t const speed)
{
	if (speed != d->localdesiredspeed) {
		d->localdesiredspeed = speed;
		updateNetworkSpeed();
	}
}


// Send the packet to all properly connected clients
void NetHost::broadcast(SendPacket & packet)
{
	container_iterate_const(std::vector<Client>, d->clients, i)
		if (i.current->playernum != UserSettings::notConnected())
			packet.send(i.current->sock);
}

void NetHost::writeSettingMap(SendPacket & packet)
{
	packet.String(d->settings.mapname);
	packet.String(d->settings.mapfilename);
	packet.Unsigned8(d->settings.savegame ? 1 : 0);
}

void NetHost::writeSettingPlayer(SendPacket & packet, uint8_t const number)
{
	PlayerSettings & player = d->settings.players.at(number);
	packet.Unsigned8(static_cast<uint8_t>(player.state));
	packet.String(player.name);
	packet.String(player.tribe);
	packet.Unsigned8(player.initialization_index);
	packet.String(player.ai);
	packet.Unsigned8(static_cast<uint8_t>(player.ready));
}

void NetHost::writeSettingAllPlayers(SendPacket & packet)
{
	packet.Unsigned8(d->settings.players.size());
	for (uint8_t i = 0; i < d->settings.players.size(); ++i)
		writeSettingPlayer(packet, i);
}

void NetHost::writeSettingUser(SendPacket & packet, uint32_t const number)
{
	packet.String(d->settings.users.at(number).name);
	packet.Signed32(d->settings.users.at(number).position);
}

void NetHost::writeSettingAllUsers(SendPacket & packet)
{
	packet.Unsigned8(d->settings.users.size());
	for (uint32_t i = 0; i < d->settings.users.size(); ++i)
		writeSettingUser(packet, i);
}


/**
* If possible, this function writes the MapTransferInfo to SendPacket & s
*
* \returns true if the data was written, else false
*/
bool NetHost::writeMapTransferInfo(SendPacket & s, std::string mapfilename) {
	// TODO not yet able to handle directory type maps / savegames
	if (g_fs->IsDirectory(mapfilename)) {
		log("Map/Save is a directory! No way for making it available a.t.m.!\n");
		return false;
	}

	// Write the new map/save file information, so client can decide whether it
	// needs the file.
	s.Unsigned8(NETCMD_NEW_FILE_AVAILABLE);
	s.String(mapfilename);
	s.Unsigned32(file->bytes);
	s.String(file->md5sum);
	return true;
}


/**
 *
 * \return a name for the given player.
 */
std::string NetHost::getComputerPlayerName(uint8_t const playernum)
{
	std::string name;
	uint32_t suffix = playernum;
	do {
		char buf[200];
		snprintf(buf, sizeof(buf), _("Computer %u"), ++suffix);
		name = buf;
	} while (haveUserName(name, playernum));
	return name;
}


/**
 * Checks whether a user with the given name exists already.
 *
 * If \p ignoreplayer < UserSettings::highestPlayernum(), the user with this
 * number will be ignored.
 */
bool NetHost::haveUserName(std::string const & name, uint8_t ignoreplayer) {
	for (uint32_t i = 0; i < d->settings.users.size(); ++i)
		if (i != ignoreplayer and d->settings.users.at(i).name == name)
			return true;

	// Computer players are not handled like human users,
	// so make sure no cp owns this name.
	if (ignoreplayer < d->settings.users.size())
		ignoreplayer = d->settings.users.at(ignoreplayer).position;
	for (uint32_t i = 0; i < d->settings.players.size(); ++i)
		if (i != ignoreplayer and d->settings.players.at(i).name == name)
			return true;

	return false;
}


/// Respond to a client's Hello message.
void NetHost::welcomeClient
	(uint32_t const number, std::string const & playername)
{
	assert(number < d->clients.size());

	Client & client = d->clients.at(number);

	assert(client.playernum == UserSettings::notConnected());
	assert(client.sock);

	// The client gets its own initial data set.
	client.playernum = UserSettings::none();
	client.usernum = d->settings.users.size();
	UserSettings newuser;
	d->settings.users.push_back(newuser);

	// Assign the player a name, preferably the name chosen by the client
	std::string effective_name = playername;

	if (effective_name.empty())
		effective_name = _("Player");

	if (haveUserName(effective_name, client.usernum)) {
		uint32_t i = 2;
		do {
			char buf[32];
			snprintf(buf, sizeof(buf), "%u", i++);
			effective_name = playername + buf;
		} while (haveUserName(effective_name, client.usernum));
	}

	d->settings.users.at(client.usernum).name = effective_name;
	d->settings.users.at(client.usernum).position = UserSettings::none();

	log("[Host]: client %u: welcome to usernum %u\n", number, client.usernum);

	SendPacket s;
	s.Unsigned8(NETCMD_HELLO);
	s.Unsigned8(NETWORK_PROTOCOL_VERSION);
	s.Unsigned32(client.usernum);
	s.send(client.sock);

	// even if the network protocol is the same, the data might be different.
	if (client.build_id != build_id())
		sendSystemChat
			(_("WARNING: %s uses version: %s, while Host uses version: %s"),
			 effective_name.c_str(), client.build_id.c_str(), build_id().c_str());

	// Send information about currently selected map / savegame
	s.reset();
	s.Unsigned8(NETCMD_SETTING_MAP);
	writeSettingMap(s);
	s.send(client.sock);

	// If possible, offer the map / savegame as transfer
	if (file) {
		s.reset();
		if (writeMapTransferInfo(s, file->filename))
			s.send(client.sock);
	}

	//  Send the tribe information to the new client.
	s.reset();
	s.Unsigned8(NETCMD_SETTING_TRIBES);
	s.Unsigned8(d->settings.tribes.size());
	for (uint8_t i = 0; i < d->settings.tribes.size(); ++i) {
		s.String(d->settings.tribes[i].name);
		size_t const nr_initializations =
			d->settings.tribes[i].initializations.size();
		s.Unsigned8(nr_initializations);
		for (uint8_t j = 0; j < nr_initializations; ++j)
			s.String(d->settings.tribes[i].initializations[j].first);
	}
	s.send(client.sock);

	s.reset();
	s.Unsigned8(NETCMD_SETTING_ALLPLAYERS);
	writeSettingAllPlayers(s);
	s.send(client.sock);

	s.reset();
	s.Unsigned8(NETCMD_SETTING_ALLUSERS);
	writeSettingAllUsers(s);
	s.send(client.sock);

	// Broadcast new information about the player to everybody
	s.reset();
	s.Unsigned8(NETCMD_SETTING_USER);
	s.Unsigned32(client.usernum);
	writeSettingUser(s, client.usernum);
	broadcast(s);

	sendSystemChat(_("%s has joined the game"), effective_name.c_str());
}

void NetHost::committedNetworkTime(int32_t const time)
{
	assert(time - d->committed_networktime > 0);

	d->committed_networktime = time;
	d->time.recv(time);

	if
		(!d->syncreport_pending &&
		 d->committed_networktime - d->syncreport_time >= SYNCREPORT_INTERVAL)
		requestSyncReports();
}

void NetHost::recvClientTime(uint32_t const number, int32_t const time)
{
	assert(number < d->clients.size());

	Client & client = d->clients.at(number);

	if (time - client.time < 0)
		throw DisconnectException
			(_("Client reports time to host that is running backwards."));
	if (d->committed_networktime - time < 0)
		throw DisconnectException
			(_("Client simulates beyond the game time allowed by the host."));
	if (d->syncreport_pending && !client.syncreport_arrived) {
		if (time - d->syncreport_time > 0)
			throw DisconnectException
				(_("Client did not submit sync report in time."));
	}

	client.time = time;
	log("[Host]: Client %i: Time %i\n", number, time);

	if (d->waiting) {
		log
			("[Host]: Client %i reports time %i (networktime = %i) during hang\n",
			 number, time, d->committed_networktime);
		checkHungClients();
	}
}


void NetHost::checkHungClients()
{
	int nrready = 0;
	int nrdelayed = 0;
	int nrhung = 0;

	for (uint32_t i = 0; i < d->clients.size(); ++i) {
		if (d->clients.at(i).playernum == UserSettings::notConnected())
			continue;

		int32_t const delta = d->committed_networktime - d->clients.at(i).time;

		if (delta == 0)
			++nrready;
		else {
			++nrdelayed;
			if
				(delta
				 >
				 (5                         *
				  CLIENT_TIMESTAMP_INTERVAL *
				  static_cast<int32_t>(d->networkspeed))
				 /
				 1000)
			{
				log
					("[Host]: Client %i (%s) hung\n",
					 i, d->settings.users.at(d->clients.at(i).usernum).name.c_str());
				++nrhung;
			}
		}
	}

	if (!d->waiting) {
		if (nrhung) {
			log("[Host]: %i clients hung. Entering wait mode\n", nrhung);

			// Brake and wait
			d->waiting = true;
			broadcastRealSpeed(0);

			SendPacket s;
			s.Unsigned8(NETCMD_WAIT);
			broadcast(s);
		}
	} else {
		if (nrdelayed == 0) {
			d->waiting = false;
			broadcastRealSpeed(d->networkspeed);
			if (!d->syncreport_pending)
				requestSyncReports();
		}
	}
}


void NetHost::broadcastRealSpeed(uint32_t const speed)
{
	assert(speed <= std::numeric_limits<uint16_t>::max());

	SendPacket s;
	s.Unsigned8(NETCMD_SETSPEED);
	s.Unsigned16(speed);
	broadcast(s);
}


/**
 * This is the algorithm that decides upon the effective network speed,
 * given the desired speed of all clients.
 *
 * This function is supposed to be the only code that ever changes
 * \ref NetHostImpl::networkspeed.
 *
 * The current implementation picks the median, or the average of
 * lower and upper median.
 */
void NetHost::updateNetworkSpeed()
{
	uint32_t const oldnetworkspeed = d->networkspeed;
	std::vector<uint32_t> speeds;

	speeds.push_back(d->localdesiredspeed);
	for (uint32_t i = 0; i < d->clients.size(); ++i) {
		if (d->clients.at(i).playernum <= UserSettings::highestPlayernum())
			speeds.push_back(d->clients.at(i).desiredspeed);
	}
	std::sort(speeds.begin(), speeds.end());

	d->networkspeed =
		speeds.size() % 2 ? speeds.at(speeds.size() / 2) :
		(speeds.at(speeds.size() / 2) + speeds.at((speeds.size() / 2) - 1)) / 2;

	if (d->networkspeed > std::numeric_limits<uint16_t>::max())
		d->networkspeed = std::numeric_limits<uint16_t>::max();

	if (d->networkspeed != oldnetworkspeed && !d->waiting)
		broadcastRealSpeed(d->networkspeed);
}


/**
 * Request sync reports from all clients at the next possible time.
 */
void NetHost::requestSyncReports()
{
	assert(!d->syncreport_pending);

	d->syncreport_pending = true;
	d->syncreport_arrived = false;
	d->syncreport_time = d->committed_networktime + 1;

	for (uint32_t i = 0; i < d->clients.size(); ++i)
		d->clients.at(i).syncreport_arrived = false;

	log("[Host]: Requesting sync reports for time %i\n", d->syncreport_time);

	SendPacket s;
	s.Unsigned8(NETCMD_SYNCREQUEST);
	s.Signed32(d->syncreport_time);
	broadcast(s);

	d->game->enqueue_command(new Cmd_NetCheckSync(d->syncreport_time, this));

	committedNetworkTime(d->syncreport_time);
}

/**
 * Check whether all sync reports have arrived, and if so, compare.
 */
void NetHost::checkSyncReports()
{
	assert(d->syncreport_pending);

	if (!d->syncreport_arrived)
		return;

	for (uint32_t i = 0; i < d->clients.size(); ++i) {
		if
			(d->clients.at(i).playernum != UserSettings::notConnected() &&
			 !d->clients.at(i).syncreport_arrived)
			return;
	}

	d->syncreport_pending = false;
	log("[Host]: comparing syncreports for time %i\n", d->syncreport_time);

	for (uint32_t i = 0; i < d->clients.size(); ++i) {
		Client & client = d->clients.at(i);
		if (client.playernum == UserSettings::notConnected())
			continue;

		if (client.syncreport != d->syncreport) {
			log
				("[Host] lost synchronization with client %u!\n"
				 "I have:     %s\n"
				 "Client has: %s\n",
				 i, d->syncreport.str().c_str(), client.syncreport.str().c_str());

			d->game->save_syncstream(true);

			SendPacket s;
			s.Unsigned8(NETCMD_INFO_DESYNC);
			broadcast(s);

			disconnectClient(i, _("Client and host have become desynchronized."));
			// Pause the game, so that host and client have time to handle the
			// desync.
			d->networkspeed = 0;
			broadcastRealSpeed(d->networkspeed);
		}
	}
}

void NetHost::syncreport()
{
	assert(d->game->get_gametime() == d->syncreport_time);

	d->syncreport = d->game->get_sync_hash();
	d->syncreport_arrived = true;

	checkSyncReports();
}


void NetHost::handle_network ()
{
	TCPsocket sock;

	if (d->promoter != 0)
		d->promoter->run ();

	// Check for new connections.
	while (d->svsock != 0 && (sock = SDLNet_TCP_Accept(d->svsock)) != 0) {
		log("[Host] Received a connection request\n");

		SDLNet_TCP_AddSocket (d->sockset, sock);

		Client peer;
		peer.sock = sock;
		peer.playernum = UserSettings::notConnected();
		peer.syncreport_arrived = false;
		peer.desiredspeed = 1000;
		d->clients.push_back(peer);

		// Now we wait for the client to say Hi in the right language,
		// unless the game has already started
		if (d->game) {
			// the following lines are needed to avoid segfaults in
			// disconnectClient
			UserSettings newuser;
			newuser.name = _("New User"); // shown in later disconnect msg.
			d->clients.rbegin()->usernum = d->settings.users.size();
			d->settings.users.push_back(newuser);

			disconnectClient
				(d->clients.size() - 1, _("The game has already started."));
		}
	}

#if HAVE_GGZ
	// if this is a ggz game, handle the ggz network
	if (use_ggz)
		NetGGZ::ref().data();
#endif

	// Check if we hear anything from our clients
	while (SDLNet_CheckSockets(d->sockset, 0) > 0) {
		for (size_t i = 0; i < d->clients.size(); ++i) {
			try {
				Client & client = d->clients.at(i);

				while (client.sock && SDLNet_SocketReady(client.sock)) {
					if (!client.deserializer.read(client.sock)) {
						disconnectClient(i, _("Connection to client lost."), false);
						break;
					}

					//  Handle all available packets immediately after each read, so
					//  that we do not miss any commands (especially DISCONNECT...).
					while (client.sock && client.deserializer.avail()) {
						RecvPacket r(client.deserializer);
						handle_packet(i, r);
					}
				}
			} catch (DisconnectException const & e) {
				disconnectClient(i, e.what());
			} catch (std::exception const & e) {
				std::string reason = _("Client sent malformed commands: ");
				reason += e.what();
				disconnectClient(i, reason);
			}
		}
	}

	reaper();
}


/**
 * Handle a single received packet.
 *
 * The caller must catch exceptions and disconnect the client as appropriate.
 *
 * \param i the client number
 * \param r the received packet
 */
void NetHost::handle_packet(uint32_t const i, RecvPacket & r)
{
	Client & client = d->clients.at(i);
	uint8_t const cmd = r.Unsigned8();

	if (cmd == NETCMD_DISCONNECT) {
		std::string reason = r.String();
		disconnectClient(i, reason, false);
		return;
	}

	if (client.playernum == UserSettings::notConnected()) {
		if (d->game)
			throw DisconnectException
				(_("Game is running already, but client has not connected fully"));
		if (cmd != NETCMD_HELLO)
			throw DisconnectException
				(_
				 	("First command sent by client is %u instead of HELLO. "
				 	 "Most likely the client is running an incompatible version."),
				 cmd);
		uint8_t version = r.Unsigned8();
		if (version != NETWORK_PROTOCOL_VERSION)
			throw DisconnectException
				(_("Server uses a different protocol version."));

		std::string playername = r.String();
		client.build_id = r.String();

		welcomeClient(i, playername);
		return;
	}

	switch (cmd) {
	case NETCMD_PONG:
		log("[Host] client %u: got pong\n", i);
		break;

	case NETCMD_SETTING_CHANGETRIBE:
		//  Do not be harsh about packets of this type arriving out of order -
		//  the client might just have had bad luck with the timing.
		if (!d->game) {
			std::string tribe = r.String();
			setPlayerTribe(client.playernum, tribe);
		}
		break;

	case NETCMD_SETTING_CHANGEREADY:
		if (!d->game) {
			setPlayerReady(client.playernum, static_cast<bool>(r.Unsigned8()));
		}
		break;

	case NETCMD_SETTING_CHANGEPOSITION:
		if (!d->game) {
			uint8_t const pos = r.Unsigned8();
			if (pos == UserSettings::none()) {
				setPlayerState(client.playernum, PlayerSettings::stateOpen);
				log("[Host] client %u went to lobby.\n", i);
			} else if (client.playernum == UserSettings::none()) {
				if (d->settings.players.size() <= pos)
					break;
				PlayerSettings position = d->settings.players.at(pos);
				if (position.state == PlayerSettings::stateOpen) {
					setPlayerState(pos, PlayerSettings::stateHuman);
					setPlayerName(pos, d->settings.users.at(client.usernum).name);
					setPlayerReady(pos, false);
					log("[Host] client %u switched to position %u.\n", i, pos);
				}
			} else {
				PlayerSettings position = d->settings.players.at(pos);
				PlayerSettings player   = d->settings.players.at(client.playernum);
				if
					((pos < d->settings.players.size()) &
					 (position.state == PlayerSettings::stateOpen))
				{
					const PlayerSettings oldOnPos = position;
					setPlayer(pos, player);
					setPlayer(client.playernum, oldOnPos);
					setPlayerReady(pos, false);
					log("[Host] client %u switched to position %i.\n", i, pos);
				} else
					break;
			}
			// local settings
			d->settings.users.at(client.usernum).position = pos;
			client.playernum = pos;

			// Send new user information
			SendPacket s;
			s.Unsigned8(NETCMD_SET_PLAYERNUMBER);
			s.Signed32(pos);
			s.send(client.sock);

			// Broadcast new information about the User to everybody
			s.reset();
			s.Unsigned8(NETCMD_SETTING_USER);
			s.Unsigned32(client.usernum);
			writeSettingUser(s, client.usernum);
			broadcast(s);
		}
		break;

	case NETCMD_TIME:
		if (!d->game)
			throw DisconnectException
				(_("Client sent TIME command even though game is not running."));
		recvClientTime(i, r.Signed32());
		break;

	case NETCMD_PLAYERCOMMAND: {
		if (!d->game)
			throw DisconnectException
				(_
				 	("Client sent PLAYERCOMMAND command even though game is not "
				 	 "running."));
		int32_t time = r.Signed32();
		Widelands::PlayerCommand & plcmd =
			*Widelands::PlayerCommand::deserialize(r);
		log
			("[Host] client %u (%u) sent player command %i for %i, time = %i\n",
			 i, client.playernum, plcmd.id(), plcmd.sender(), time);
		recvClientTime(i, time);
		if (plcmd.sender() != client.playernum + 1)
			throw DisconnectException
				(_
				 	("Client %u (%u) sent a playercommand (%i) for a different "
				 	 "player (%i)."),
				 i, client.playernum, plcmd.id(), plcmd.sender());
		sendPlayerCommand(plcmd);
	} break;

	case NETCMD_SYNCREPORT: {
		if (!d->game || !d->syncreport_pending || client.syncreport_arrived)
			throw DisconnectException
				(_("Client sent unexpected synchronization report."));
		int32_t time = r.Signed32();
		r.Data(client.syncreport.data, 16);
		client.syncreport_arrived = true;
		recvClientTime(i, time);
		checkSyncReports();
		break;
	}

	case NETCMD_CHAT: {
		ChatMessage c;
		c.time = time(0);
		c.playern = d->settings.users.at(client.usernum).position;
		c.sender = d->settings.users.at(client.usernum).name;
		c.msg = r.String();
		if (c.msg.size() && *c.msg.begin() == '@') {
			// Personal message
			std::string::size_type const space = c.msg.find(' ');
			if (space >= c.msg.size() - 1)
				break;
			c.recipient = c.msg.substr(1, space - 1);
			c.msg = c.msg.substr(space + 1);
		}
		send(c);
		break;
	}

	case NETCMD_SETSPEED: {
		client.desiredspeed = r.Unsigned16();
		updateNetworkSpeed();
		break;
	}

	case NETCMD_NEW_FILE_AVAILABLE: {
		if (!file) // Do we have a file for sending
			throw DisconnectException
				(_("Client requests file altough none is available to send."));
		sendSystemChat
			(_("Started to send file %s to %s!"),
			 file->filename.c_str(),
			 d->settings.users.at(client.usernum).name.c_str());
		sendFilePart(client.sock, 0);
		break;
	}

	case NETCMD_FILE_PART: {
		if (!file) // Do we have a file for sending
			throw DisconnectException
				(_("Client requests file altough none is available to send."));
		uint32_t part = r.Unsigned32();
		std::string x = r.String();
		if (x != file->md5sum) {
			log
				("[host] File transfer checksum missmatch %s != %s\n",
				 x.c_str(), file->md5sum.c_str());
			return; // Surely the file was changed, so we cancel here.
		}
		if (part >= file->parts.size())
			throw DisconnectException
				(_("Client requests file part that does not exist."));
		if (part == file->parts.size() - 1) {
			sendSystemChat
				(_("Completed transfer of file %s to %s"),
				 file->filename.c_str(),
				 d->settings.users.at(client.usernum).name.c_str());
			return;
		}
		++part;
		if (part % 100 == 0)
			sendSystemChat
				(_("Sending part %u/%u of file %s to %s"),
				 part, file->parts.size() + 1,
				 file->filename.c_str(),
				 d->settings.users.at(client.usernum).name.c_str());
		sendFilePart(client.sock, part);
		break;
	}

	default:
		throw DisconnectException
			(_("Client sent unknown command number %u"), cmd);
	}
}

void NetHost::sendFilePart(TCPsocket csock, uint32_t part) {
	assert(part < file->parts.size());

	uint32_t left = file->bytes - NETFILEPARTSIZE * part;
	uint8_t size = (left > NETFILEPARTSIZE) ? NETFILEPARTSIZE : left;

	// Send the part
	SendPacket s;
	s.Unsigned8(NETCMD_FILE_PART);
	s.Unsigned32(part);
	s.Unsigned8(size);
	s.Data(file->parts[part].part, size);
	s.send(csock);
}


void NetHost::disconnectPlayer
	(uint8_t const number, std::string const & reason, bool const sendreason)
{
	log("[Host]: disconnectPlayer(%u, %s)\n", number, reason.c_str());

	bool needai = false;

	for (uint32_t index = 0; index < d->clients.size(); ++index) {
		Client & client = d->clients.at(index);
		if (client.playernum != number)
			continue;

		client.playernum = UserSettings::notConnected();
		disconnectClient(index, reason, sendreason);

		setPlayerState(number, PlayerSettings::stateOpen);
		needai = true;
	}

	if (needai && d->game)
		initComputerPlayer(number + 1);
}

void NetHost::disconnectClient
	(uint32_t const number, std::string const & reason, bool const sendreason)
{
	assert(number < d->clients.size());

	Client & client = d->clients.at(number);
	if (client.playernum <= UserSettings::highestPlayernum()) {
		disconnectPlayer(client.playernum, reason, sendreason);
		// disconnectPlayer calls us recursively
		return;
	}

	sendSystemChat
		(_("%s has left the game (%s)"),
		 d->settings.users.at(client.usernum).name.c_str(),
		 reason.c_str());

	d->settings.users.at(client.usernum).name     = std::string();
	d->settings.users.at(client.usernum).position =
		UserSettings::notConnected();
	client.playernum = UserSettings::notConnected();


	// Broadcast the user changes to everybody
	SendPacket s;
	s.Unsigned8(NETCMD_SETTING_USER);
	s.Unsigned32(client.usernum);
	writeSettingUser(s, client.usernum);
	broadcast(s);

	log("[Host]: disconnectClient(%u, %s)\n", number, reason.c_str());

	if (client.sock) {
		if (sendreason) {
			s.reset();
			s.Unsigned8(NETCMD_DISCONNECT);
			s.String(reason);
			s.send(client.sock);
		}

		SDLNet_TCP_DelSocket (d->sockset, client.sock);
		SDLNet_TCP_Close (client.sock);
		client.sock = 0;
	}

	if (d->game)
		checkHungClients();
}

/**
 * The grim reaper. This finally erases disconnected clients from the clients
 * array.
 *
 * Calls this when you're certain that nobody is holding any client indices or
 * iterators, since this function will invalidate them.
 */
void NetHost::reaper()
{
	uint32_t index = 0;
	while (index < d->clients.size())
		if (d->clients.at(index).sock)
			++index;
		else
			d->clients.erase(d->clients.begin() + index);
}
