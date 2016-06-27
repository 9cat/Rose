/* $Id: config_cache.cpp 46186 2010-09-01 21:12:38Z silene $ */
/*
   Copyright (C) 2008 - 2010 by Pauli Nieminen <paniemin@cc.hut.fi>
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

#include "config_cache.hpp"
#include "filesystem.hpp"
#include "gettext.hpp"
#include "rose_config.hpp"
#include "log.hpp"
#include "marked-up_text.hpp"
#include "sha1.hpp"
#include "serialization/binary_or_text.hpp"
#include "serialization/parser.hpp"

#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

static lg::log_domain log_cache("cache");
#define ERR_CACHE LOG_STREAM(err, log_cache)
#define DBG_CACHE LOG_STREAM(debug, log_cache)

namespace game_config {

	config_cache& config_cache::instance()
	{
		static config_cache cache;
		return cache;
	}

	config_cache::config_cache() :
		force_valid_cache_(false),
		fake_invalid_cache_(false),
		defines_map_()
	{
		// To set-up initial defines map correctly
		clear_defines();
	}

	struct output {
		void operator()(const preproc_map::value_type& def)
		{
			DBG_CACHE << "key: " << def.first << " " << def.second << "\n";
		}
	};
	const preproc_map& config_cache::get_preproc_map() const
	{
		return defines_map_;
	}

	void config_cache::clear_defines()
	{
		defines_map_.clear();
		// set-up default defines map

#if defined(__APPLE__)
		defines_map_["APPLE"] = preproc_define();
#endif

	}

	void config_cache::get_config(const std::string& path, config& cfg)
	{
		load_configs(path, cfg);
	}

	preproc_map& config_cache::make_copy_map()
	{
		return config_cache_transaction::instance().get_active_map(defines_map_);
	}


	static bool compare_define(const preproc_map::value_type& a, const preproc_map::value_type& b)
	{
		if (a.first < b.first)
			return true;
		if (b.first < a.first)
			return false;
		if (a.second < b.second)
			return true;
		return false;
	}

	void config_cache::add_defines_map_diff(preproc_map& defines_map)
	{
		return config_cache_transaction::instance().add_defines_map_diff(defines_map);
	}


	void config_cache::read_configs(const std::string& path, config& cfg, preproc_map& defines_map)
	{
		//read the file and then write to the cache
		scoped_istream stream = preprocess_file(path, &defines_map);
		read(cfg, *stream);
	}

	void config_cache::load_configs(const std::string& path, config& cfg)
	{
		// Make sure that we have fake transaction if no real one is going on
		fake_transaction fake;

		preproc_map copy_map(make_copy_map());
		read_configs(path, cfg, copy_map);
		add_defines_map_diff(copy_map);
	}

	void config_cache::set_force_invalid_cache(bool force)
	{
		fake_invalid_cache_ = force;
	}

	void config_cache::set_force_valid_cache(bool force)
	{
		force_valid_cache_ = force;
	}

	void config_cache::recheck_filetree_checksum()
	{
		data_tree_checksum(true);
	}

	void config_cache::add_define(const std::string& define)
	{
		defines_map_[define] = preproc_define();
		if (config_cache_transaction::is_active())
		{
			// we have to add this to active map too
			config_cache_transaction::instance().get_active_map(defines_map_).insert(
					std::make_pair(define, preproc_define()));
		}

	}

	void config_cache::remove_define(const std::string& define)
	{
		DBG_CACHE << "removing define: " << define << "\n";
		defines_map_.erase(define);
		if (config_cache_transaction::is_active())
		{
			// we have to remove this from active map too
			config_cache_transaction::instance().get_active_map(defines_map_).erase(define);
		}
	}

	config_cache_transaction::state config_cache_transaction::state_ = FREE;
	config_cache_transaction* config_cache_transaction::active_ = 0;

	config_cache_transaction::config_cache_transaction()
		: define_filenames_()
		, active_map_()
	{
		assert(state_ == FREE);
		state_ = NEW;
		active_ = this;
	}

	config_cache_transaction::~config_cache_transaction()
	{
		state_ = FREE;
		active_ = 0;
	}

	void config_cache_transaction::lock()
	{
		state_ = LOCKED;
	}

	const config_cache_transaction::filenames& config_cache_transaction::get_define_files() const
	{
		return define_filenames_;
	}

	void config_cache_transaction::add_define_file(const std::string& file)
	{
		define_filenames_.push_back(file);
	}

	preproc_map& config_cache_transaction::get_active_map(const preproc_map& defines_map)
	{
		if(active_map_.empty())
		{
			std::copy(defines_map.begin(),
					defines_map.end(),
					std::insert_iterator<preproc_map>(active_map_, active_map_.begin()));
			if ( get_state() == NEW)
				state_ = ACTIVE;
		 }

		return active_map_;
	}

	void config_cache_transaction::add_defines_map_diff(preproc_map& new_map)
	{

		if (get_state() == ACTIVE)
		{
			preproc_map temp;
			std::set_difference(new_map.begin(),
					new_map.end(),
					active_map_.begin(),
					active_map_.end(),
					std::insert_iterator<preproc_map>(temp,temp.begin()),
					&compare_define);

			BOOST_FOREACH (const preproc_map::value_type &def, temp) {
				insert_to_active(def);
			}

			temp.swap(new_map);
		} else if (get_state() == LOCKED) {
			new_map.clear();
		}
	}

	void config_cache_transaction::insert_to_active(const preproc_map::value_type& def)
	{
		active_map_[def.first] = def.second;
	}
}
