/* $Id: minimap.cpp 54007 2012-04-28 19:16:10Z mordante $ */
/*
   Copyright (C) 2008 - 2012 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/widgets/minimap.hpp"

#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/widget_definition/minimap.hpp"
#include "gui/auxiliary/window_builder/minimap.hpp"
#include "gui/widgets/settings.hpp"
#include "map.hpp"
#include "map_exception.hpp"
#include "../../image.hpp"
#include "../../minimap.hpp"
#include "rose_config.hpp"

#include <boost/bind.hpp>

#include <algorithm>

static lg::log_domain log_config("config");
#define ERR_CF LOG_STREAM_INDENT(err, log_config)

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

REGISTER_WIDGET(minimap)

/** Key type for the cache. */
struct tkey
{
	tkey(const int w, const int h, const std::string& map_data)
		: w(w)
		, h(h)
		, map_data(map_data)
	{
	}

	/** Width of the image. */
	const int w;

	/** Height of the image. */
	const int h;

	/** The data used to generate the image. */
	const std::string map_data;
};

static bool operator<(const tkey& lhs, const tkey& rhs)
{
	return lhs.w < rhs.w || (lhs.w == rhs.w
			&& (lhs.h < rhs.h || (lhs.h == rhs.h
				&& lhs.map_data < rhs.map_data)));
}

/** Value type for the cache. */
struct tvalue
{
	tvalue(const surface& surf)
		: surf(surf)
		, age(1)
	{
	}

	/** The cached image. */
	const surface surf;

	/**
	 * The age of the image.
	 *
	 * Every time an image is used its age is increased by one. Once the cache
	 * is full 25% of the cache is emptied. This is done by halving the age of
	 * the items in the cache and then erase the 25% with the lowest age. If
	 * items have the same age their order is unspecified.
	 */
	unsigned age;
};

	/**
	 * Maximum number of items in the cache (multiple of 4).
	 *
	 * No testing on the optimal number is done, just seems a nice number.
	 */
	static const size_t cache_max_size = 100;

	/**
	 * The terrain used to create the cache.
	 *
	 * If another terrain config is used the cache needs to be cleared, this
	 * normally doesn't happen a lot so the clearing of the cache is rather
	 * unusual.
	 */
	static const ::config* terrain = NULL;

	/** The cache. */
	typedef std::map<tkey, tvalue> tcache;
	static tcache cache;

static bool compare(const std::pair<unsigned, tcache::iterator>& lhs
		, const std::pair<unsigned, tcache::iterator>& rhs)
{
	return lhs.first < rhs.first;
}

static void shrink_cache()
{
	DBG_GUI_D << "Shrinking the minimap cache.\n";

	std::vector<std::pair<unsigned, tcache::iterator> > items;
	for(tcache::iterator itor = cache.begin(); itor != cache.end(); ++itor) {

		itor->second.age /= 2;
		items.push_back(std::make_pair(itor->second.age, itor));
	}

	std::partial_sort(items.begin()
			, items.begin() + cache_max_size / 4
			, items.end()
			, compare);

	for(std::vector<std::pair<unsigned, tcache::iterator> >::iterator
			  vitor = items.begin()
			; vitor < items.begin() + cache_max_size / 4
			; ++vitor) {

		cache.erase(vitor->second);
	}

}

tminimap::tminimap() 
	: tcontrol(COUNT)
	, type_(NONE)
	, map_data_()
	, terrain_(NULL)
{
	surfs_.resize(COUNT);
}

void tminimap::set_surface(const surface& surf, int w, int h)
{
	surfs_.resize(COUNT);
	if (!surf) {
		return;
	}

	// normal
	surface& normal = surfs_[NORMAL];
	normal = scale_surface(surf, w, h);

	type_ = SURFACE;

	set_dirty();
}

const surface tminimap::get_image(const int w, const int h)
{
	if (!terrain_) {
		return NULL;
	}

	if (terrain_ != terrain) {
		DBG_GUI_D << "Flushing the minimap cache.\n";

		terrain = terrain_;
		cache.clear();

	}

	const tkey key(w, h, map_data_);
	tcache::iterator itor = cache.find(key);

	if (itor != cache.end()) {
		itor->second.age++;
		return itor->second.surf;
	}

	if (cache.size() >= cache_max_size) {
		shrink_cache();
	}

	try {
		surface surf;
		if (type_ == IMG) {
			surf = image::get_image(map_data_);
		} else if (type_ == TILE_MAP) {
			image::ttile_switch_lock lock(game_config::tile_hex);
			const gamemap map(*terrain_, map_data_);
			surf = image::getMinimap(w, h, map, NULL);
		}
		if (type_ == IMG && !surf.null() && (surf.get()->w != w || surf.get()->h != h)) {
			surf = scale_surface(surf, w, h);
		}
		cache.insert(std::make_pair(key, tvalue(surf)));
		return surf;

	} catch (incorrect_map_format_error& e) {
		ERR_CF << "Error while loading the map: " << e.message << '\n';
	}
	return NULL;
}

void tminimap::impl_draw_background(
		  surface& frame_buffer
		, int x_offset
		, int y_offset)
{
	surface surf;
	if (type_ != SURFACE) {
		if (!terrain_) return;
		assert(terrain_);

		if (map_data_.empty()) {
			return;
		}
		SDL_Rect rect = get_rect();
		assert(rect.w > 0 && rect.h > 0);
		surf = get_image(rect.w, rect.h);

		surfs_[0] = surf;
	} else {
		// surf = surfs_[0];
	}
	
	// tshare_canvas_image_lock lock(&surf);
	tcontrol::impl_draw_background(frame_buffer, x_offset, y_offset);
}

const std::string& tminimap::get_control_type() const
{
	static const std::string type = "minimap";
	return type;
}

} // namespace gui2

