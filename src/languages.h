/*
 * Copyright (C) 2002-2008 by the Widelands Development Team
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

#ifndef LANGUAGES_H
#define LANGUAGES_H

#include <string>
#include <cstring>

/*
 * This file simply contains the languages which are available as
 * translations in widelands.
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!!! NOTE This file must be saved as utf-8 encoded file !!!!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

struct Languages {
	std::string name;
	std::string abbrev;
};


/*
 * Add your language below in alphabetical order (as far as possible). But keep
 * NONE as first entry. Also increase the NR_LANGUAGES variable by one
 */
#ifdef DEFINE_LANGUAGES  // defined in fullscreen_menu_options.cc
#define NR_LANGUAGES 31
static Languages available_languages[NR_LANGUAGES] = {
	{"Default system language", ""},
// EXTRACT BEGIN (leaves this line untouched)
	{"العربية",             "ar_SA.UTF-8"},
	{"Català",              "ca_ES.UTF-8"},
	{"česky",               "cs_CZ.UTF-8"},
	{"Dansk",               "da_DK.UTF-8"},
	{"Deutsch",             "de_DE.UTF-8"},
	{"English",             "en_US.UTF-8"},
	{"British English",     "en_GB.UTF-8"},
	{"Esperanto",           "eo_EO.UTF-8"},
	{"Español",             "es_ES.UTF-8"},
	{"Euskara",             "eu_ES.UTF-8"},
	{"Suomi",               "fi_FI.UTF-8"},
	{"Français",            "fr_FR.UTF-8"},
	{"Galego",              "gl_ES.UTF-8"},
	{"עברית",                "he_IL.UTF-8"},
	{"Magyar",              "hu_HU.UTF-8"},
	{"Interlingua",         "ia_IA.UTF-8"},
	{"Bahasa Indonesia",    "id_ID.UTF-8"},
	{"Italiano",            "it_IT.UTF-8"},
	{"Japanese",            "ja_JP.UTF-8"},
	{"Latin",               "la_LA.UTF-8"},
	{"Nederlands",          "nl_NL.UTF-8"},
	{"Nynorsk",             "nn_NO.UTF-8"},
	{"Polski",              "pl_PL.UTF-8"},
	{"Português do Brasil", "pt_BR.UTF-8"},
	{"Русский",             "ru_RU.UTF-8"},
	{"Sinhala",             "si_SI.UTF-8"},
	{"Slovensky",           "sk_SK.UTF-8"},
	{"Slovenščina",         "sl_SI.UTF-8"},
	{"Srpski",              "sr_ME.UTF-8"},
	{"Svenska",             "sv_SE.UTF-8"},
// EXTRACT END (leave this line untouched)
};
#endif

#endif
