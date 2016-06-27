/* $Id: lobby_player_info.cpp 48440 2011-02-07 20:57:31Z mordante $ */
/*
   Copyright (C) 2009 - 2011 by Tomasz Sniatowski <kailoran@gmail.com>
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

#include "gui/dialogs/helper.hpp"
#include "gui/dialogs/theme.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/scroll_label.hpp"
#include "gui/widgets/report.hpp"
#include "formula_string_utils.hpp"
#include "font.hpp"
#include "gettext.hpp"
#include "help.hpp"
#include "display.hpp"
#include "controller_base.hpp"
#include "hotkeys.hpp"
#include "rose_config.hpp"

#include <boost/bind.hpp>

namespace theme {

const int XDim = 1024;
const int YDim = 768;

const std::string id_screen = "screen";
const std::string id_main_map = "main-map";

typedef struct { size_t x1, y1, x2, y2; } _rect;
_rect ref_rect = { 0, 0, 0, 0 };

static size_t compute(std::string expr, size_t ref1, size_t ref2 = 0 ) 
{
	size_t ref = 0;
	if (expr[0] == '=') {
		ref = ref1;
		expr = expr.substr(1);
	} else if ((expr[0] == '+') || (expr[0] == '-')) {
		ref = ref2;
	}
	return ref + atoi(expr.c_str());
}

// If x2 or y2 are not specified, use x1 and y1 values
static _rect read_rect(const std::string& str) 
{
	_rect rect = { 0, 0, 0, 0 };
	std::vector<std::string> items = utils::split(str);
	if (items.size() >= 1) {
		rect.x1 = atoi(items[0].c_str());
	}

	if (items.size() >= 2) {
		rect.y1 = atoi(items[1].c_str());
	}

	if (items.size() >= 3) {
		rect.x2 = atoi(items[2].c_str());
	} else {
		rect.x2 = rect.x1;
	}

	if (items.size() >= 4) {
		rect.y2 = atoi(items[3].c_str());
	} else {
		rect.y2 = rect.y1;
	}

	return rect;
}

SDL_Rect read_sdl_rect(const std::string& str) 
{
	SDL_Rect sdlrect;
	const _rect rect = read_rect(str);
	sdlrect.x = rect.x1;
	sdlrect.y = rect.y1;
	sdlrect.w = (rect.x2 > rect.x1) ? (rect.x2 - rect.x1) : 0;
	sdlrect.h = (rect.y2 > rect.y1) ? (rect.y2 - rect.y1) : 0;

	return sdlrect;
}

static std::string resolve_rect(const std::string& rect_str) {
		_rect rect = { 0, 0, 0, 0 };
		std::stringstream resolved;
		const std::vector<std::string> items = utils::split(rect_str.c_str());
		if(items.size() >= 1) {
			rect.x1 = compute(items[0], ref_rect.x1, ref_rect.x2);
			resolved << rect.x1;
		}
		if(items.size() >= 2) {
			rect.y1 = compute(items[1], ref_rect.y1, ref_rect.y2);
			resolved << "," << rect.y1;
		}
		if(items.size() >= 3) {
			rect.x2 = compute(items[2], ref_rect.x2, rect.x1);
			resolved << "," << rect.x2;
		}
		if(items.size() >= 4) {
			rect.y2 = compute(items[3], ref_rect.y2, rect.y1);
			resolved << "," << rect.y2;
		}

		ref_rect = rect;
		return resolved.str();
	}

static config &find_ref(const std::string &id, config &cfg, bool remove = false)
{
	static config empty_config;

	config::all_children_itors itors = cfg.all_children_range();
	for (config::all_children_iterator i = itors.first; i != itors.second; ++i)
	{
		config &icfg = const_cast<config &>(i->cfg);
		if (i->cfg["id"] == id) {
			if (remove) {
				cfg.erase(i);
				return empty_config;
			} else {
				return icfg;
			}
		}

		// Recursively look in children.
		config &c = find_ref(id, icfg, remove);
		if (&c != &empty_config) {
			return c;
		}
	}

	// Not found.
	return empty_config;
}

void full_rect_cfg(const config& cfg, config& result)
{
	static std::vector<std::string> rect_fields;
	if (rect_fields.empty()) {
		rect_fields.push_back("rect");
		rect_fields.push_back("ref");
		rect_fields.push_back("xanchor");
		rect_fields.push_back("yanchor");
	}
	for (std::vector<std::string>::const_iterator it = rect_fields.begin(); it != rect_fields.end(); ++ it) {
		const std::string& key = *it;
		if (cfg.has_attribute(key)) {
			result[key] = cfg[key];

		} else {
			result[key] = null_str;
		}
	}
}

config& fill_rect_cfg(std::map<std::string, config>& cache, config& main_res, const std::string& id, const config& chg)
{
	config chg2;
	std::map<std::string, config>::iterator it = cache.find(id);
	if (it == cache.end()) {
		// has been no data, fill data from main_res.
		const config& target = find_ref(id, main_res);
		if (!target.empty()) {
			full_rect_cfg(target, chg2);
			chg2["id"] = id;
		}
		cache.insert(std::make_pair(id, chg2));
		it = cache.find(id);
	}
	if (!it->second.empty()) {
		// verlay with current cfg
		it->second.merge_attributes(chg);
	}
	return it->second;
}

static const config& modify_top_cfg_according_to_mode(const std::string& patch, const config& top_cfg, config& tmp)
{
	if (patch.empty()) {
		return top_cfg;
	}
	const config& sub = top_cfg.child(patch);
	if (!sub) {
		return top_cfg;
	}

	std::map<std::string, config> change_cache;

	std::vector<std::string> res_ids;
	res_ids.push_back("1024x768");
	res_ids.push_back("640x480");
	res_ids.push_back("480x320");

	config* main_res = NULL;
	tmp = top_cfg;
	for (std::vector<std::string>::const_iterator it = res_ids.begin(); it != res_ids.end(); ++ it) {
		const std::string& key = *it;
		const config& cfg = sub.child(key);

		bool is_resolution = it == res_ids.begin();
		config* find = NULL;
		if (is_resolution) {
			find = &tmp.find_child("resolution", "id", key);
			main_res = find;
		} else {
			find = &tmp.find_child("partialresolution", "id", key);
		}

		if (!*find) {
			VALIDATE(!is_resolution, "Theme must define 1024x768!");
			continue;
		}
		if (cfg) {
			BOOST_FOREACH (const config &rm, cfg.child_range("remove")) {
				if (is_resolution) {
					find_ref(rm["id"], *find, true);
				} else {
					config& find2 = find->find_child("remove", "id", rm["id"]);
					if (!find2) {
						find->add_child("remove", rm);
					}
				}
			}

			BOOST_FOREACH (const config &chg, cfg.child_range("change")) {
				const std::string& id = chg["id"];
				const config& chg2 = fill_rect_cfg(change_cache, *main_res, id, chg);

				if (is_resolution) {
					config& target = find_ref(id, *find);
					if (!target.empty()) {
						target.merge_attributes(chg2);
					}

				} else {
					config& find2 = find->find_child("change", "id", id);
					if (find2) {
						find2.merge_attributes(chg2);
					} else {
						find->add_child("change", chg2);
					}
				}
			}

			BOOST_FOREACH (const config &add, cfg.child_range("add")) {
				if (!is_resolution) {
					continue;
				}
				const std::string parent = add["id"].str();
				config& target = parent.empty()? *find: find_ref(parent, *find);
				BOOST_FOREACH (const config::any_child &j, add.all_children_range()) {
					target.add_child(j.key, j.cfg);
				}
			}
		} else {
			for (std::map<std::string, config>::const_iterator it2 = change_cache.begin(); it2 != change_cache.end(); ++ it2) {
				config& find2 = find->find_child("change", "id", it2->first);
				if (find2) {
					find2.merge_attributes(it2->second);
				} else {
					find->add_child("change", it2->second);
				}
			}
		}
	}
	return tmp;
}

// I make sure there is a 480x320 [resolution] in [theme]. so:
// 1. Don't resolve partialresolution when it is 480x320.
// 2. Don't save other resolution except 480x320, when current is tiny_gui mode.
static void expand_partialresolution(const std::string& patch, config& dst_cfg, const config& _top_cfg)
{
	config tmp;
	const config& top_cfg = modify_top_cfg_according_to_mode(patch, _top_cfg, tmp);

	std::vector<config> res_cfgs_;
	std::string theme_name = top_cfg["name"].str();
	// resolve all the partialresolutions
	BOOST_FOREACH (const config &part, top_cfg.child_range("partialresolution")) {
		// follow the inheritance hierarchy and push all the nodes on the stack
		std::vector<const config*> parent_stack(1, &part);
		const config *parent;
		std::string parent_id = part["inherits"];
		while (!*(parent = &top_cfg.find_child("resolution", "id", parent_id)))
		{
			parent = &top_cfg.find_child("partialresolution", "id", parent_id);
			if (!*parent)
				throw config::error("[partialresolution] refers to non-existent [resolution] " + parent_id);
			parent_stack.push_back(parent);
			parent_id = (*parent)["inherits"].str();
		}
		// find parent of this part, parent must is full resolution, interval may be not only one era
		// 1024x768 
		//   800x600
		//     800x480
		// also parent of 800x480 is 800x600, but 800x600 isnpt full resolution, find up until to 1024x768


		// Add the parent resolution and apply all the modifications of its children
		res_cfgs_.push_back(*parent);
		while (!parent_stack.empty()) {
			//override attributes
			res_cfgs_.back().merge_attributes(*parent_stack.back());
			BOOST_FOREACH (const config &rm, parent_stack.back()->child_range("remove")) {
				find_ref(rm["id"], res_cfgs_.back(), true);
			}

			BOOST_FOREACH (const config &chg, parent_stack.back()->child_range("change"))
			{
				config &target = find_ref(chg["id"], res_cfgs_.back());
				target.merge_attributes(chg);
			}

			// cannot add [status] sub-elements, but who cares
			if (const config &c = parent_stack.back()->child("add"))
			{
				BOOST_FOREACH (const config::any_child &j, c.all_children_range()) {
					res_cfgs_.back().add_child(j.key, j.cfg);
				}
			}

			parent_stack.pop_back();
		}
	}
	// Add all the resolutions
	BOOST_FOREACH (const config &res, top_cfg.child_range("resolution")) {
		dst_cfg.add_child("resolution", res);
	}
	// Add all the resolved resolutions
	for(std::vector<config>::const_iterator k = res_cfgs_.begin(); k != res_cfgs_.end(); ++k) {
		dst_cfg.add_child("resolution", (*k));
	}
	return;
}

static void do_resolve_rects(const config& cfg, config& resolved_config, config* resol_cfg = NULL) 
{
	// copy all key/values
	resolved_config.merge_attributes(cfg);

	// override default reference rect with "ref" parameter if any
	if (!cfg["ref"].empty()) {
		if (resol_cfg == NULL) {
			// Use of ref= outside a [resolution] block
		} else {
			const config ref = find_ref (cfg["ref"], *resol_cfg);

			if (ref["id"].empty()) {
				// Reference to non-existent rect id cfg["ref"]
			} else {
				const std::string& rect = ref["rect"].str();
				if (rect.empty()) {
					// Reference to id cfg["ref"] which does not have a rect
				} else {
					ref_rect = read_rect(rect);
				}
			}
		}
	}
	// resolve the rect value to absolute coordinates
	if (!cfg["rect"].empty()) {
		resolved_config["rect"] = resolve_rect(cfg["rect"]);
	}

	// recursively resolve children
	BOOST_FOREACH (const config::any_child &value, cfg.all_children_range()) {
		config &childcfg = resolved_config.add_child(value.key);
		do_resolve_rects(value.cfg, childcfg,
			value.key == "resolution" ? &childcfg : resol_cfg);
	}
}

tborder::tborder() :
	size(0.0),
	view_rectange_color(help::string_to_color("white")),
	background_image(),
	tile_image(),
	corner_image_top_left(),
	corner_image_bottom_left(),
	corner_image_top_right_odd(),
	corner_image_top_right_even(),
	corner_image_bottom_right_odd(),
	corner_image_bottom_right_even(),
	border_image_left(),
	border_image_right(),
	border_image_top_odd(),
	border_image_top_even(),
	border_image_bottom_odd(),
	border_image_bottom_even()
{
}

tborder::tborder(const config& cfg) :
	size(cfg["border_size"].to_double()),
	view_rectange_color(help::string_to_color(cfg["view_rectangle_color"].str())),

	background_image(cfg["background_image"]),
	tile_image(cfg["tile_image"]),

	corner_image_top_left(cfg["corner_image_top_left"]),
	corner_image_bottom_left(cfg["corner_image_bottom_left"]),

	corner_image_top_right_odd(cfg["corner_image_top_right_odd"]),
	corner_image_top_right_even(cfg["corner_image_top_right_even"]),

	corner_image_bottom_right_odd(cfg["corner_image_bottom_right_odd"]),
	corner_image_bottom_right_even(cfg["corner_image_bottom_right_even"]),

	border_image_left(cfg["border_image_left"]),
	border_image_right(cfg["border_image_right"]),

	border_image_top_odd(cfg["border_image_top_odd"]),
	border_image_top_even(cfg["border_image_top_even"]),

	border_image_bottom_odd(cfg["border_image_bottom_odd"]),
	border_image_bottom_even(cfg["border_image_bottom_even"])
{
	VALIDATE(size >= 0.0 && size <= 0.5, _("border_size should be between 0.0 and 0.5."));
}

ANCHORING read_anchor(const std::string& str)
{
	static const std::string top_anchor = "top", left_anchor = "left",
	                         bot_anchor = "bottom", right_anchor = "right",
							 fixed_anchor = "fixed", proportional_anchor = "proportional";
	if (str == top_anchor || str == left_anchor) {
		return TOP_ANCHORED;
	} else if (str == bot_anchor || str == right_anchor) {
		return BOTTOM_ANCHORED;
	} else if (str == proportional_anchor) {
		return PROPORTIONAL;
	} else {
		return FIXED;
	}
}

SDL_Rect calculate_relative_loc(const config& cfg, int screen_w, int screen_h)
{
	if (gui2::twidget::hdpi) {
		screen_w /= gui2::twidget::hdpi_scale;
		screen_h /= gui2::twidget::hdpi_scale;
	}

	SDL_Rect relative_loc;
	ANCHORING xanchor_ = read_anchor(cfg["xanchor"]);
	ANCHORING yanchor_ = read_anchor(cfg["yanchor"]);
	SDL_Rect loc_ = read_sdl_rect(cfg["rect"].str());

	switch (xanchor_) {
	case FIXED:
		relative_loc.x = loc_.x;
		relative_loc.w = loc_.w;
		break;
	case TOP_ANCHORED:
		relative_loc.x = loc_.x;
		relative_loc.w = screen_w - std::min<size_t>(XDim - loc_.w,screen_w);
		break;
	case BOTTOM_ANCHORED:
		relative_loc.x = screen_w - std::min<size_t>(XDim - loc_.x,screen_w);
		relative_loc.w = loc_.w;
		break;
	case PROPORTIONAL:
		relative_loc.x = (loc_.x*screen_w)/XDim;
		relative_loc.w = (loc_.w*screen_w)/XDim;
		break;
	default:
		VALIDATE(false, null_str);
	}

	switch(yanchor_) {
	case FIXED:
		relative_loc.y = loc_.y;
		relative_loc.h = loc_.h;
		break;
	case TOP_ANCHORED:
		relative_loc.y = loc_.y;
		relative_loc.h = screen_h - std::min<size_t>(YDim - loc_.h,screen_h);
		break;
	case BOTTOM_ANCHORED:
		relative_loc.y = screen_h - std::min<size_t>(YDim - loc_.y,screen_h);
		relative_loc.h = loc_.h;
		break;
	case PROPORTIONAL:
		relative_loc.y = (loc_.y*screen_h)/YDim;
		relative_loc.h = (loc_.h*screen_h)/YDim;
		break;
	default:
		VALIDATE(false, null_str);
	}

	relative_loc.x = std::min<int>(relative_loc.x, screen_w);
	relative_loc.w = std::min<int>(relative_loc.w, screen_w - relative_loc.x);
	relative_loc.y = std::min<int>(relative_loc.y, screen_h);
	relative_loc.h = std::min<int>(relative_loc.h, screen_h - relative_loc.y);

	// when MOD wirte [theme] mistake, will result to relative_loc_.x/y/w.h less than 0. 
	// it is invalid, there set to 0 in order to avoid ACCESS Excepetion. MOD should correct them.
	if (relative_loc.x < 0) relative_loc.x = 0;
	if (relative_loc.y < 0) relative_loc.y = 0;
	if (relative_loc.w < 0) relative_loc.w = THEME_NO_SIZE;
	if (relative_loc.h < 0) relative_loc.h = THEME_NO_SIZE;


	if (gui2::twidget::hdpi) {
		relative_loc = create_rect(relative_loc.x * gui2::twidget::hdpi_scale, relative_loc.y * gui2::twidget::hdpi_scale,
			relative_loc.w * gui2::twidget::hdpi_scale, relative_loc.h * gui2::twidget::hdpi_scale);
	}

	return relative_loc;
}

const config* set_resolution(const config& cfg, int screen_w, int screen_h, const std::string& patch, config& resolved_config)
{
	config tmp;
	expand_partialresolution(patch, tmp, cfg);
	// character of tmp expand_partialresolution generated:
	// 1. there is no attrubute in root
	// 2. only one block: [resolution], block count equal to count of [resolution] + count of [partialresolution]
	do_resolve_rects(tmp, resolved_config);

	return set_resolution2(resolved_config, screen_w, screen_h);
}

const config* set_resolution2(const config& cfg, int screen_w, int screen_h)
{
	if (gui2::twidget::hdpi) {
		screen_w /= gui2::twidget::hdpi_scale;
		screen_h /= gui2::twidget::hdpi_scale;
	}

	const config* current_cfg_ = NULL;
	int current_rating = 1000000;
	BOOST_FOREACH (const config &i, cfg.child_range("resolution")) {
		int width = i["width"];
		int height = i["height"];
		if (screen_w >= width && screen_h >= height) {
			current_cfg_ = &i;
			break;
		}

		const int rating = width * height;
		if (rating < current_rating) {
			current_cfg_ = &i;
			current_rating = rating;
		}
	}
	return current_cfg_;
}

std::map<std::string, config> known_themes;
void set_known_themes(const config* cfg)
{
	known_themes.clear();
	if (!cfg)
		return;

	BOOST_FOREACH (const config &thm, cfg->child_range("theme"))
	{
		std::string thm_name = thm["name"];
		if (thm_name != "null" && thm_name != "editor")
			known_themes[thm_name] = thm;
	}
}

}


namespace gui2 {

gui2::tbutton* create_context_button(display& disp, const std::string& main, const std::string& id, size_t flags, int width, int height)
{
	gui2::tbutton* widget = gui2::create_surface_button(id, NULL);
	const hotkey::hotkey_item& hotkey = hotkey::get_hotkey(id);
	if (!hotkey.null()) {
		widget->set_tooltip(hotkey.get_description());
	}

	connect_signal_mouse_left_click(
		*widget
		, boost::bind(
			&display::click_context_menu
			, &disp
			, main
			, id
			, flags));

	widget->set_visible(gui2::twidget::INVISIBLE);

	std::stringstream file;
	file << "buttons/" << id << ".png";
	// set surface
	surface surf = image::get_image(file.str());
	if (surf) {
		widget->set_surface(surf, width, height);
	}
	return widget;
}

size_t tcontext_menu::decode_flag_str(const std::string& str)
{
	size_t result = 0;
	const char* cstr = str.c_str();
	if (strchr(cstr, 'h')) {
		result |= F_HIDE;
	}
	if (strchr(cstr, 'p')) {
		result |= F_PARAM;
	}
	return result;
}

void tcontext_menu::load(display& disp, const config& cfg)
{
	gui2::tpoint unit_size = report->get_unit_size();

	std::string str = cfg["main"].str();
	std::vector<std::string> main_items = utils::split(str, ',');
	VALIDATE(!main_items.empty(), "Must define main menu!");

	std::vector<std::string> vstr, main_items2;
	size_t flags;
	for (std::vector<std::string>::const_iterator it = main_items.begin(); it != main_items.end(); ++ it) {
		const std::string& item = *it;
		vstr = utils::split(item, '|');
		flags = 0;
		if (vstr.size() >= 2) {
			flags = decode_flag_str(vstr[1]);
		}
		main_items2.push_back(vstr[0]);

		gui2::tbutton* widget = create_context_button(disp, main_id, vstr[0], flags, unit_size.x, unit_size.y);
		report->insert_child(*widget);
	}
	menus.insert(std::make_pair(main_id, main_items2));

	std::vector<std::string> subitems;
	for (std::vector<std::string>::const_iterator it = main_items2.begin(); it != main_items2.end(); ++ it) {
		const std::string& item = *it;
		size_t pos = item.rfind("_m");
		if (item.size() <= 2 || pos != item.size() - 2) {
			continue;
		}
		const std::string sub = item.substr(0, pos);
		str = cfg[sub].str();
		std::vector<std::string> vstr2 = utils::split(str, ',');
		VALIDATE(!vstr.empty(), "must define submenu!");
		subitems.clear();
		for (std::vector<std::string>::const_iterator it2 = vstr2.begin(); it2 != vstr2.end(); ++ it2) {
			vstr = utils::split(*it2, '|');
			flags = 0;
			if (vstr.size() >= 2) {
				flags = decode_flag_str(vstr[1]);
			}
			str = sub + ":" + vstr[0];
			subitems.push_back(str);

			gui2::tbutton* widget = create_context_button(disp, main_id, str, flags, unit_size.x, unit_size.y);
			report->insert_child(*widget);
		}
		menus.insert(std::make_pair(sub, subitems));
	}
}

void tcontext_menu::show(const display& disp, const controller_base& controller, const std::string& id) const
{
	size_t start_child = -1;
	std::string adjusted_id = id;
	if (id.empty()) {
		adjusted_id = main_id;
		start_child = 0;
	}
	if (adjusted_id.empty()) {
		return;
	}
	if (!menus.count(adjusted_id)) {
		return;
	}

	const gui2::tgrid::tchild* children = report->child_begin();
	size_t size = report->childs();
	if (start_child == -1) {
		// std::string prefix = adjusted_id + "_c:";
		std::string prefix = adjusted_id + ":";
		for (size_t n = 0; n < size; n ++ ) {
			const std::string& id = children[n].widget_->id();
			if (id.find(prefix) != std::string::npos) {
				start_child = n;
				break;
			}
		}
	}
	if (start_child == -1) {
		return;
	}

	gui2::tpoint unit_size = report->get_unit_size();
	const std::vector<std::string>& items = menus.find(adjusted_id)->second;
	for (size_t n = 0; n < size; n ++) {
		gui2::tbutton* widget = dynamic_cast<gui2::tbutton*>(children[n].widget_);
		if (n >= start_child && n < start_child + items.size()) {
			const std::string& item = items[n - start_child];
			// Remove commands that can't be executed or don't belong in this type of menu
			if (controller.in_context_menu(item)) {
				report->set_child_visible(n, true);
				widget->set_active(controller.actived_context_menu(item));
				controller.prepare_show_menu(*widget, item, unit_size.x, unit_size.y);
			} else {
				report->set_child_visible(n, false);
			}
		} else {
			report->set_child_visible(n, false);
		}
	}
	report->replacement_children();
}

void tcontext_menu::hide() const
{
	report->hide_children();
}

std::pair<std::string, std::string> tcontext_menu::extract_item(const std::string& id)
{
	size_t pos = id.find(":");
	if (pos == std::string::npos) {
		return std::make_pair(id, null_str);
	}

	std::string major = id.substr(0, pos);
	std::string minor = id.substr(pos + 1);
	return std::make_pair(major, minor);
}

REGISTER_DIALOG(theme);

ttheme::ttheme(display& disp, controller_base& controller, const config& cfg)
	: disp_(disp)
	, controller_(controller)
	, cfg_(cfg)
	, window_(NULL)
	, cached_widgets_()
	, log_(NULL)
{
}

ttheme::~ttheme()
{
}

void ttheme::pre_show(CVideo& video, twindow& window)
{
	window_ = &window;
	join();

	hotkey::insert_hotkey(HOTKEY_ZOOM_IN, "zoomin", _("Zoom In"));
	hotkey::insert_hotkey(HOTKEY_ZOOM_OUT, "zoomout", _("Zoom Out"));
	hotkey::insert_hotkey(HOTKEY_ZOOM_DEFAULT, "zoomdefault", _("Default Zoom"));
	hotkey::insert_hotkey(HOTKEY_SCREENSHOT, "screenshop", _("Screenshot"));
	hotkey::insert_hotkey(HOTKEY_MAP_SCREENSHOT, "mapscreenshop", _("Map Screenshot"));
	hotkey::insert_hotkey(HOTKEY_CHAT, "chat", _("Chat"));
	hotkey::insert_hotkey(HOTKEY_UNDO, "undo", _("Undo"));
	hotkey::insert_hotkey(HOTKEY_REDO, "redo", _("Redo"));
	hotkey::insert_hotkey(HOTKEY_COPY, "copy", _("Copy"));
	hotkey::insert_hotkey(HOTKEY_CUT, "cut", _("Cut"));
	hotkey::insert_hotkey(HOTKEY_PASTE, "paste", _("Paste"));
	hotkey::insert_hotkey(HOTKEY_HELP, "help", _("Help"));
	hotkey::insert_hotkey(HOTKEY_SYSTEM, "system", _("System"));

	app_pre_show();

	utils::string_map symbols;
	std::string no_widget_msgid = "Cannot find report widget: $id.";
	std::string not_fix_msgid = "Report widget($id) must use fix rect.";
		
	for (std::map<int, const std::string>::const_iterator it = reports_.begin(); it != reports_.end(); ++ it) {
		twidget* widget = get_object(it->second);
		symbols["id"] = it->second;
		if (!widget) {
			// some widget maybe remove. for example undo in tower mode.
			// VALIDATE(false, vgettext2("Cannot find report widget: $id.", symbols));
			continue;
		}
		if (!widget->fix_width() || !widget->fix_height()) {
			VALIDATE(false, vgettext2("Report widget($id) must use fix rect.", symbols));
		}
	}
}

void ttheme::post_layout()
{
	utils::string_map symbols;

	BOOST_FOREACH (const config &cfg, cfg_.child_range("context_menu")) {
		const std::string id = cfg["report"].str();
		if (id.empty()) {
			continue;
		}
		symbols["id"] = tintegrate::generate_format(id, "yellow");
		treport* report = dynamic_cast<treport*>(get_object(id));
		VALIDATE(report, vgettext2("Defined $id conext menu, but not define same id report widget!", symbols));

		context_menus_.push_back(tcontext_menu(id));
		tcontext_menu& m = context_menus_.back();
		m.report = report;
		report->tabbar_init(false, "surface");
		m.load(disp_, cfg);
	}
}

void ttheme::click_generic_handler(tcontrol& widget, const std::string& sparam)
{
	const hotkey::hotkey_item& hotkey = hotkey::get_hotkey(widget.id());
	if (!hotkey.null()) {
		widget.set_tooltip(hotkey.get_description());
	}

	connect_signal_mouse_left_click(
		widget
		, boost::bind(
			&controller_base::execute_command
			, &controller_
			, hotkey.get_id()
			, sparam));
}

void ttheme::toggle_report(twidget* widget)
{
	bool conti = controller_.toggle_report(widget);
	if (conti) {
		tdialog::toggle_report(widget);
	}
}

bool ttheme::click_report(twidget* widget)
{
	return controller_.click_report(widget);
}

void ttheme::destruct_widget(const twidget* widget)
{
	for (std::map<std::string, twidget*>::iterator it = cached_widgets_.begin(); it != cached_widgets_.end(); ++ it) {
		if (widget == it->second) {
			cached_widgets_.erase(it);
			return;
		}
	}
}

twidget* ttheme::get_object(const std::string& id) const
{
	std::map<std::string, twidget*>::const_iterator it = cached_widgets_.find(id);
	if (it != cached_widgets_.end()) {
		return it->second;
	}
	twidget* widget = find_widget<twidget>(window_, id, false, false);
	cached_widgets_.insert(std::make_pair(id, widget));
	return widget;
}

void ttheme::set_object_active(const std::string& id, bool active) const
{
	tcontrol* widget = dynamic_cast<tcontrol*>(get_object(id));
	if (widget) {
		widget->set_active(active);
	}
}

void ttheme::set_object_visible(const std::string& id, const twidget::tvisible visible) const
{
	twidget* widget = get_object(id);
	if (widget) {
		widget->set_visible(visible);
	}
}

void ttheme::set_object_surface(const std::string& id, const surface& surf) const
{
	tcontrol* widget = dynamic_cast<tcontrol*>(get_object(id));
	if (widget) {
		widget->set_surface(surf, widget->fix_width(), widget->fix_height());
	}
}

twidget* ttheme::get_report(int num) const
{
	std::map<int, const std::string>::const_iterator it = reports_.find(num);
	if (it == reports_.end()) {
		return NULL;
	}
	return get_object(it->second);
}

void ttheme::set_report_label(int num, const std::string& label) const
{
	tcontrol* widget = dynamic_cast<tcontrol*>(get_report(num));
	if (widget) {
		widget->set_label(label);
	}
}

void ttheme::set_report_surface(int num, const surface& surf) const
{
	tcontrol* widget = dynamic_cast<tcontrol*>(get_report(num));
	if (widget) {
		const SDL_Rect& rect = widget->fix_rect();
		widget->set_surface(surf, rect.w, rect.h);
	}
}

tcontext_menu* ttheme::context_menu(const std::string& id)
{
	for (std::vector<tcontext_menu>::iterator it = context_menus_.begin(); it != context_menus_.end(); ++ it) {
		return &*it;
	}
	return NULL;
}

void ttheme::handle(const tsock& sock, const std::string& msg)
{
	// log_->set_label(lobby.format_log_str());
	// log_->scroll_vertical_scrollbar(tscrollbar_::END);
}

} //end namespace gui2
