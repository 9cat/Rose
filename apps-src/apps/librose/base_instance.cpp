/* $Id: mkwin_display.cpp 47082 2010-10-18 00:44:43Z shadowmaster $ */
/*
   Copyright (C) 2008 - 2010 by Tomasz Sniatowski <kailoran@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#define GETTEXT_DOMAIN "rose-lib"

#include "base_instance.hpp"
#include "gettext.hpp"
#include "builder.hpp"
#include "language.hpp"
#include "preferences_display.hpp"
#include "loadscreen.hpp"
#include "cursor.hpp"
#include "map.hpp"
#include "version.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/language_selection.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/widgets/window.hpp"
#include "ble.hpp"
#include <iostream>
#include <clocale>

base_instance* instance = NULL;

void base_instance::regenerate_heros(hero_map& heros, bool allow_empty)
{
	const std::string hero_data_path = game_config::path + "/xwml/" + "hero.dat";
	heros.map_from_file(hero_data_path);
	if (!heros.map_from_file(hero_data_path)) {
		if (allow_empty) {
			// allow no hero.dat
			heros.realloc_hero_map(HEROS_MAX_HEROS);
		} else {
			std::stringstream err;
			err << _("Can not find valid hero.dat in <data>/xwml");
			throw game::error(err.str());
		}
	}
	hero_map::map_size_from_dat = heros.size();
	hero player_hero(hero_map::map_size_from_dat);
	if (!preferences::get_hero(player_hero, heros.size())) {
		// write [hero] to preferences
		preferences::set_hero(heros, player_hero);
	}

	group.reset();
	heros.add(player_hero);
	hero& leader = heros[heros.size() - 1];
	group.set_leader(leader);
	leader.set_uid(preferences::uid());
	group.set_noble(preferences::noble());
	group.set_coin(preferences::coin());
	group.set_score(preferences::score());
	group.interior_from_str(preferences::interior());
	group.signin_from_str(preferences::signin());
	group.reload_heros_from_string(heros, preferences::member(), preferences::exile());
	group.associate_from_str(preferences::associate());
	// group.set_layout(preferences::layout()); ?????
	group.set_map(preferences::map());

	group.set_city(heros[hero::number_local_player_city]);
	group.city().set_name(preferences::city());

	other_group.clear();
}

extern void preprocess_res_explorer();
static void handle_app_event(Uint32 type, void* param)
{
	base_instance* instance = reinterpret_cast<base_instance*>(param);
	instance->handle_app_event(type);
}

base_instance::base_instance(int argc, char** argv, int sample_rate, size_t sound_buffer_size)
	: thread_manager()
	, icon_()
	, video_()
	, force_bpp_(-1)
	, font_manager_()
	, prefs_manager_()
	, image_manager_()
	, main_event_context_()
	, music_thinker_()
	, paths_manager_()
	, cursor_manager_(NULL)
	, loadscreen_manager_(NULL)
	, gui2_event_manager_(NULL)
	, heros_(game_config::path)
	, disp_(NULL)
	, game_config_()
	, game_config_core_()
	, old_defines_map_()
	, cache_(game_config::config_cache::instance())
	, foreground_(true) // normally app cannnot receive first DIDFOREGROUND.
	, terminating_(false)
{
	VALIDATE(game_config::path.empty(), null_str);
#ifdef _WIN32
	std::string exe_dir = get_exe_dir();
	if (!exe_dir.empty() && file_exists(exe_dir + "/data/_main.cfg")) {
		game_config::path = exe_dir;
	}

	// windows is debug os. allow pass command line.
	for (int arg_ = 1; arg_ != argc; ++ arg_) {
		const std::string option(argv[arg_]);
		if (option.empty()) {
			continue;
		}

		if (option == "--res-dir") {
			const std::string val = argv[++ arg_];

			// Overriding data directory
			if (val.c_str()[1] == ':') {
				game_config::path = val;
			} else {
				game_config::path = get_cwd() + '/' + val;
			}
			
			if (!is_directory(game_config::path)) {
				posix_print("path(%s) of res directory not found\n", game_config::path.empty()? "nul": game_config::path.c_str());
				VALIDATE(false, "path of res directory not found");
			}
		}
	}
#elif defined(__APPLE__)
	// ios, macosx
	game_config::path = get_cwd();

#elif defined(ANDROID)
	// on android, use asset.
	game_config::path = "res";

#else
	// linux, etc
	game_config::path = get_cwd();
#endif

#if defined(_WIN32)
	// all characters of game_config::path must be ascii.
	for (std::string::const_iterator it = game_config::path.begin(); it != game_config::path.end(); ++ it) {
		if (*it & 0x80) {
			std::stringstream err;
			std::string path = game_config::path;
			std::replace(path.begin(), path.end(), '/', '\\');

			err << path << " include invalid character! Please use english path.";
			MessageBox(NULL, err.str().c_str(), "Error", MB_OK);
			throw game::error(err.str());
		}
	}
#endif

	std::replace(game_config::path.begin(), game_config::path.end(), '\\', '/');
	VALIDATE(file_exists(game_config::path + "/data/_main.cfg"), "game_config::path must be valid!");

	posix_print("Data directory: %s\n", game_config::path.c_str());
	preprocess_res_explorer();

	sound::set_play_params(sample_rate, sound_buffer_size);

	bool no_music = false;
	bool no_sound = false;

	// if allocate static, iOS may be not align 4! 
	// it necessary to ensure align great equal than 4.
	game_config::savegame_cache = (unsigned char*)malloc(game_config::savegame_cache_size);

	font_manager_.update_font_path();
	heros_.set_path(game_config::path);

#if defined(_WIN32) && defined(_DEBUG)
	// sound::init_sound make no memory leak output.
	// By this time, I doesn't find what result it, to easy, don't call sound::init_sound. 
	no_sound = true;
#endif

	// disable sound in nosound mode, or when sound engine failed to initialize
	if (no_sound || ((preferences::sound_on() || preferences::music_on() ||
	                  preferences::turn_bell() || preferences::UI_sound_on()) &&
	                 !sound::init_sound())) {

		preferences::set_sound(false);
		preferences::set_music(false);
		preferences::set_turn_bell(false);
		preferences::set_UI_sound(false);
	} else if (no_music) { // else disable the music in nomusic mode
		preferences::set_music(false);
	}

	game_config::reserve_players.insert("");
	game_config::reserve_players.insert("kingdom");
	game_config::reserve_players.insert("Player");
	game_config::reserve_players.insert(_("Player"));


	//
	// initialize player group
	//
	upgrade::fill_require();
	regenerate_heros(heros_, true);

	SDL_SetAppEventHandler(::handle_app_event, this);
}

base_instance::~base_instance()
{
	close();

	if (icon_) {
		icon_.get()->refcount --;
	}
	if (game_config::savegame_cache) {
		free(game_config::savegame_cache);
		game_config::savegame_cache = NULL;
	}
	terrain_builder::release_heap();
	sound::close_sound();

	clear_anims();
	game_config::path.clear();
}

/**
 * I would prefer to setup locale first so that early error
 * messages can get localized, but we need the game_controller
 * initialized to have get_intl_dir() to work.  Note: setlocale()
 * does not take GUI language setting into account.
 */
void base_instance::init_locale() 
{
#ifdef _WIN32
    std::setlocale(LC_ALL, "English");
#else
	std::setlocale(LC_ALL, "C");
	std::setlocale(LC_MESSAGES, "");
#endif
	const std::string& intl_dir = get_intl_dir();
	bindtextdomain("rose-lib", intl_dir.c_str());
	bind_textdomain_codeset ("rose-lib", "UTF-8");
	
	bindtextdomain(def_textdomain, intl_dir.c_str());
	bind_textdomain_codeset(def_textdomain, "UTF-8");
	textdomain(def_textdomain);

	app_init_locale(intl_dir);
}

bool base_instance::init_language()
{
	if (!::load_language_list()) {
		return false;
	}

	if (!::set_language(get_locale())) {
		return false;
	}

	// hotkey::load_descriptions();

	return true;
}

void base_instance::handle_app_event(Uint32 type)
{
	// Notice! it isn't in main thread, except SDL_APP_LOWMEMORY.
	if (type == SDL_APP_TERMINATING || type == SDL_QUIT) {
		posix_print("handle_app_event, SDL_APP_TERMINATING(0x%x)\n", type);
		app_terminating();

		terminating_ = true;
#ifdef ANDROID
		// now android is default!

#elif defined __APPLE__
		// now iOS is default!
		// if call CVideo::quit when SDL_APP_WILLENTERBACKGROUND, it will success. Of couse, no help.
		throw CVideo::quit();
#endif

	} else if (type == SDL_APP_WILLENTERBACKGROUND) {
		posix_print("handle_app_event, SDL_APP_WILLENTERBACKGROUND\n");
		app_willenterbackground();
		// FIX SDL BUG! normally DIDENTERBACKGROUND should be called after WILLENTERBACKGROUND.
		// but on iOS, because SDL event queue, SDL-DIDENTERBACKGROUND is called, but app-DIDENTERBACKGROUND not!
		// app-DIDENTERBACKGROUND is call when WILLENTERFOREGROUND.

	} else if (type == SDL_APP_DIDENTERBACKGROUND) {
		posix_print("handle_app_event, SDL_APP_DIDENTERBACKGROUND\n");
		app_didenterbackground();

		foreground_ = false;
		
		tble* ble = tble::get_singleton();
		if (ble) {
			ble->handle_app_event(true);
		}

	} else if (type == SDL_APP_WILLENTERFOREGROUND) {
		posix_print("handle_app_event, SDL_APP_WILLENTERFOREGROUND\n");
		app_willenterforeground();

	}
	else if (type == SDL_APP_DIDENTERFOREGROUND) {
		posix_print("handle_app_event, SDL_APP_DIDENTERFOREGROUND\n");
		app_didenterforeground();
	
		foreground_ = true;
		tble* ble = tble::get_singleton();
		if (ble) {
			ble->handle_app_event(false);
		}

	} else if (type == SDL_APP_LOWMEMORY) {
		posix_print("handle_app_event, SDL_APP_LOWMEMORY\n");
		app_lowmemory();
	}
}

display& base_instance::disp()
{
	if (disp_.get() == NULL) {
		if (get_video_surface() == NULL) {
			throw CVideo::error();
		}
		disp_.assign(display::create_dummy_display(video_));
	}
	return *disp_.get();
}

surface icon2;
bool base_instance::init_video()
{
	load_config();

	std::pair<int,int> resolution;
	int bpp = 0;
	int video_flags = 0;

#if (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(ANDROID)
	// width/height from preferences.cfg must be landscape.
    SDL_Rect rc = video_.bound();

	posix_print("1. video_.bound(): (%i, %i, %i, %i)\n", rc.x, rc.y, rc.w, rc.h);

	if (rc.w < rc.h) {
		int tmp = rc.w;
		rc.w = rc.h;
		rc.h = tmp;
	}

#if (defined(__APPLE__) && TARGET_OS_IPHONE)
	if (gui2::twidget::hdpi) {
		rc.w *= gui2::twidget::hdpi_scale;
		rc.h *= gui2::twidget::hdpi_scale;
	}
#endif
	gui2::tpoint normal_size = gui2::twidget::orientation_swap_size(rc.w, rc.h);

	resolution = std::make_pair(normal_size.x, normal_size.y);
	bpp = 32;
	// on iphone/ipad, don't set SDL_WINDOW_FULLSCREEN, it will result cannot find orientation.
	// video_flags = SDL_WINDOW_FULLSCREEN;

#else
	video_flags = preferences::fullscreen() ? SDL_WINDOW_FULLSCREEN: 0;
	resolution = preferences::resolution();
	bpp = 32;
    
#endif

    posix_print("setting mode to %ix%ix%i\n", resolution.first, resolution.second, bpp);
	const int res = video_.setMode(resolution.first, resolution.second, bpp, video_flags);
	posix_print("using mode %ix%ix%i\n", video_.getx(), video_.gety(), bpp);
	video_.setBpp(bpp);

	if (res == 0) {
		std::cerr << "required video mode, " << resolution.first << "x"
		          << resolution.second << "x" << bpp << " is not supported\n";
		return false;
	}

	std::string wm_title_string = game_config::app_title;
	wm_title_string += " - " + game_config::wesnoth_version.str();
	SDL_SetWindowTitle(video_.getWindow(), wm_title_string.c_str());

#ifdef _WIN32
	icon2 = image::get_image("game-icon.png", image::UNSCALED);
	if (icon2) {
		SDL_SetWindowIcon(video_.getWindow(), icon2);
	}
#endif

	return true;
}

bool base_instance::init_config(const bool force)
{
	VALIDATE(!game_config::app.empty(), _("Must set game_config::app"));
	VALIDATE(!game_config::app_title.empty(), _("Must set game_config::app_title"));
	VALIDATE(!game_config::app_channel.empty(), _("Must set game_config::app_channel"));

	cache_.clear_defines();

	load_game_cfg(force);

	const config& cfg = game_config().child("game_config");
	const config& cfg2 = cfg? cfg: config();
	game_config::load_config(&cfg2);
	load_config2(cfg2);

	paths_manager_.set_paths(game_config());
	return true;
}

#define BASENAME_DATA		"data.bin"

void base_instance::load_game_cfg(const bool force)
{
	// make sure that 'debug mode' symbol is set if command line parameter is selected
	// also if we're in multiplayer and actual debug mode is disabled
	if (!game_config_.empty() && !force && old_defines_map_ == cache_.get_preproc_map()) {
		return; // game_config already holds requested config in memory
	}
	old_defines_map_ = cache_.get_preproc_map();
	loadscreen::global_loadscreen_manager loadscreen_manager(disp().video());
	cursor::setter cur(cursor::WAIT);
	// The loadscreen will erase the titlescreen
	// NOTE: even without loadscreen, needed after MP lobby
	try {
		/**
		 * Read all game configs
		 * First we should load data/
		 * Then handle terrains so that they are last loaded from data/
		 * 2nd everything in userdata
		 **/
/*
		loadscreen::start_stage("verify cache");
		data_tree_checksum();
		loadscreen::start_stage("create cache");
*/
		// start transaction so macros are shared
		game_config::config_cache_transaction main_transaction;

		config tmpcfg;
		
		wml_config_from_file(game_config::path + "/xwml/" + BASENAME_DATA, game_config_);
		// once only duration one game running.
		// game_config_.clear_children("card");
		// game_config_.clear_children("card_anim");

		// set_unit_data(game_config_.child("units"));
		// game_config_.clear_children("units");

		anim2::fill_anims(game_config_.child("units"));
	

		const config::const_child_itors& terrains = game_config_.child_range("terrain_type");
		BOOST_FOREACH (const config &t, terrains) {
			gamemap::terrain_types.add_child("terrain_type", t);
		}
		game_config_.clear_children("terrain_type");

		// save this to game_config_core_
		game_config_core_ = game_config_;

		// cache_.get_config(game_config::path +"/data", game_config_);

		main_transaction.lock();

		/* Put the gfx rules aside so that we can prepend the add-on
		   rules to them. */
		// config core_terrain_rules;
		// core_terrain_rules.splice_children(game_config_, "terrain_graphics");

		// Extract the Lua scripts at toplevel.
		// extract_preload_scripts(game_config_);
		game_config_.clear_children("lua");

		// game_config_.merge_children("units");
		// game_config_.splice_children(core_terrain_rules, "terrain_graphics");

		config& hashes = game_config_.add_child("multiplayer_hashes");
		BOOST_FOREACH (const config &ch, game_config_.child_range("multiplayer")) {
			hashes[ch["id"]] = ch.hash();
		}

	} catch(game::error& e) {
		// ERR_CONFIG << "Error loading game configuration files\n";
		gui2::show_error_message(disp().video(), _("Error loading game configuration files: '") +
			e.message + _("' (The game will now exit)"));
		throw;
	}
}

bool base_instance::change_language()
{
	gui2::tlanguage_selection dlg;
	dlg.show(disp().video());
	if (dlg.get_retval() != gui2::twindow::OK) return false;

	std::string wm_title_string = game_config::app_title;
	wm_title_string += " - " + game_config::wesnoth_version.str();
	SDL_SetWindowTitle(disp().video().getWindow(), wm_title_string.c_str());
	return true;
}

int base_instance::show_preferences_dialog(display& disp, bool first)
{
#if (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(ANDROID)
	return gui2::twindow::OK;
#else
	std::vector<gui2::tval_str> items;

	int fullwindowed = preferences::fullscreen()? preferences::MAKE_WINDOWED: preferences::MAKE_FULLSCREEN;
	std::string str = preferences::fullscreen()? _("Exit fullscreen") : _("Enter fullscreen");
	items.push_back(gui2::tval_str(fullwindowed, str));
	items.push_back(gui2::tval_str(preferences::CHANGE_RESOLUTION, _("Change Resolution")));
	items.push_back(gui2::tval_str(gui2::twindow::OK, _("Close")));

	gui2::tcombo_box dlg(items, preferences::CHANGE_RESOLUTION);
	dlg.show(disp.video());

	return dlg.selected_val();
#endif
}

void base_instance::fill_anim(int at, const std::string& id, bool area, bool tpl, const config& cfg)
{
	if (area) {
		anims_.insert(std::make_pair(at, new animation(cfg)));
	} else {
		if (tpl) {
			utype_anim_tpls_.insert(std::make_pair(id, cfg));
		} else {
			anims_.insert(std::make_pair(at, new animation(cfg)));
		}
	}
}

void base_instance::clear_anims()
{
	utype_anim_tpls_.clear();

	for (std::map<int, animation*>::const_iterator it = anims_.begin(); it != anims_.end(); ++ it) {
		animation* anim = it->second;
		delete anim;
	}
	anims_.clear();
}

const animation* base_instance::anim(int at) const
{
	std::map<int, animation*>::const_iterator i = anims_.find(at);
	if (i != anims_.end()) {
		return i->second;
	}
	return NULL;
}

void base_instance::prefix_create(const std::string& app, const std::string& title, const std::string& channel, bool hdpi, bool landscape)
{
	std::stringstream err;
	// if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
	if (SDL_Init(SDL_INIT_TIMER) < 0) {
		err << "Couldn't initialize SDL: " << SDL_GetError();
		throw twml_exception(err.str(), null_str);
	}

	srand((unsigned int)time(NULL));
	game_config::init(app, title, channel, hdpi, landscape);
}

void base_instance::initialize(fcreate_lobby create_lobby)
{
	// if fail, use throw twml_exception.
	init_locale();
	if (create_lobby) {
		lobby = create_lobby();
	} else {
		lobby = new tlobby(new tlobby::tchat_sock(), new tlobby::thttp_sock(), new tlobby::ttransit_sock());
	}
	if (!lobby) {
		throw twml_exception("could not create lobby", null_str);
	}

	// do initialize fonts before reading the game config, to have game
	// config error messages displayed. fonts will be re-initialized later
	// when the language is read from the game config.
	bool res = font::load_font_config();
	if (!res) {
		throw twml_exception("could not initialize fonts", null_str);
	}

	res = init_language();
	if (!res) {
		throw twml_exception("could not initialize the language", null_str);
	}

	res = init_video();
	if (!res) {
		throw twml_exception("could not initialize display", null_str);
	}

	cursor_manager_ = new cursor::manager;
	cursor::set(cursor::WAIT);

	// in order to 
	loadscreen_manager_ = new loadscreen::global_loadscreen_manager(disp().video());
	loadscreen::start_stage("titlescreen");

	res = gui2::init();
	if (!res) {
		throw twml_exception("could not initialize gui2-subsystem", null_str);
	}
	gui2_event_manager_ = new gui2::event::tmanager;

	// load core config.
	res = init_config(false);
	if (!res) {
		throw twml_exception("could not initialize game config", null_str);
	}
}

void base_instance::close()
{
	// in reverse order compaire to initialize 
	if (gui2_event_manager_) {
		delete gui2_event_manager_;
		gui2_event_manager_ = NULL;
	}

	if (loadscreen_manager_) {
		delete loadscreen_manager_;
		loadscreen_manager_ = NULL;
	}

	if (cursor_manager_) {
		delete cursor_manager_;
		cursor_manager_ = NULL;
	}

	if (lobby) {
		delete lobby;
		lobby = NULL;
	}
}