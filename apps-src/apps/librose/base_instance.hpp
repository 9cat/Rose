/* $Id: editor_display.hpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
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

#ifndef LIBROSE_BASE_INSTANCE_HPP_INCLUDED
#define LIBROSE_BASE_INSTANCE_HPP_INCLUDED

#include "rose_config.hpp"
#include "display.hpp"
#include "hero.hpp"
#include "preferences.hpp"
#include "sound.hpp"
#include "filesystem.hpp"
#include "serialization/preprocessor.hpp"
#include "config_cache.hpp"
#include "cursor.hpp"
#include "loadscreen.hpp"

class animation;
typedef tlobby* (* fcreate_lobby)();

class base_instance
{
public:
	static void prefix_create(const std::string& _app, const std::string& title, const std::string& channel, bool hdpi, bool landscape);

	base_instance(int argc, char** argv, int sample_rate = 44100, size_t sound_buffer_size = 4096);
	virtual ~base_instance();

	void initialize(fcreate_lobby create_lobby);
	virtual void close();

	loadscreen::global_loadscreen_manager& loadscreen_manager() const { return *loadscreen_manager_; }

	bool init_language();
	bool init_video();
	virtual bool init_config(const bool force);

	display& disp();
	const config& game_config() const { return game_config_; }
	bool is_loading() { return false; }

	bool change_language();
	virtual int show_preferences_dialog(display& disp, bool first);

	virtual void regenerate_heros(hero_map& heros, bool allow_empty);
	hero_map& heros() { return heros_; }

	virtual void fill_anim_tags(std::map<const std::string, int>& tags) {};
	virtual void fill_anim(int at, const std::string& id, bool area, bool tpl, const config& cfg);

	void clear_anims();
	const std::multimap<std::string, const config>& utype_anim_tpls() const { return utype_anim_tpls_; }
	const animation* anim(int at) const;

	void init_locale();
	void handle_app_event(Uint32 type);

	bool foreground() const { return foreground_; }
	bool terminating() const { return terminating_; }

protected:
	virtual void load_config() {}
	virtual void load_config2(const config& cfg) {}
	virtual void load_game_cfg(const bool force);

private:
	virtual void app_init_locale(const std::string& intl_dir) {}

	virtual void app_terminating() {}
	virtual void app_willenterbackground() {}
	virtual void app_didenterbackground() {}
	virtual void app_willenterforeground() {}
	virtual void app_didenterforeground() {}
	virtual void app_lowmemory() {}

protected:
	//this should get destroyed *after* the video, since we want
	//to clean up threads after the display disappears.
	const threading::manager thread_manager;

	surface icon_;
	CVideo video_;
	hero_map heros_;

	int force_bpp_;
	const font::manager font_manager_;
	const preferences::base_manager prefs_manager_;
	const image::manager image_manager_;
	const events::event_context main_event_context_;
	sound::music_thinker music_thinker_;
	binary_paths_manager paths_manager_;
	const cursor::manager* cursor_manager_; // it should create after video-subsystem.
	loadscreen::global_loadscreen_manager* loadscreen_manager_; // it should create after video-subsystem.
	const gui2::event::tmanager* gui2_event_manager_; // it should create after gui2-subsystem.

	util::scoped_ptr<display> disp_;

	config game_config_;
	config game_config_core_;
	preproc_map old_defines_map_;
	game_config::config_cache& cache_;

	bool foreground_;
	bool terminating_;
	std::map<int, animation*> anims_;
	std::multimap<std::string, const config> utype_anim_tpls_;
};

extern base_instance* instance;

// C++ Don't call virtual function during class construct function.
template<class T>
class instance_manager
{
public:
	instance_manager(int argc, char** argv, const std::string& app, const std::string& title, const std::string& channel, bool hdpi, bool landscape, fcreate_lobby create_lobby)
	{
		// if exception generated when construct, system don't call destructor.
		try {
			base_instance::prefix_create(app, title, channel, hdpi, landscape);
			instance = new T(argc, argv);
			instance->initialize(create_lobby);

		} catch(...) {
			if (instance) {
				delete instance;
				instance = NULL;
			}
			throw;
		}
	}
	
	~instance_manager()
	{
		if (instance) {
			delete instance;
			instance = NULL;
		}
	}
	T& get() { return *(dynamic_cast<T*>(instance)); }
};

#endif
