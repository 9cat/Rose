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
#define GETTEXT_DOMAIN "studio-lib"

#include "mkwin_display.hpp"
#include "mkwin_controller.hpp"
#include "unit_map.hpp"
#include "gui/dialogs/mkwin_theme.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/report.hpp"
#include "game_config.hpp"
#include "formula_string_utils.hpp"

#include <boost/bind.hpp>

static std::string miss_anim_err_str = "logic can only process map or canvas animation!";

gui2::tcontrol_definition_ptr mkwin_display::find_widget(display& disp, const std::string& type, const std::string& definition, const std::string& id)
{
	std::stringstream err;
	utils::string_map symbols;
	std::string definition2 = definition.empty()? "default": definition;

	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
	if (!controls.count(type)) {
		err << "Cannot find widget, type: " << type;
		VALIDATE(false, err.str());
	}

	const std::map<std::string, gui2::tcontrol_definition_ptr>& map = controls.find(type)->second;
	std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = map.find(definition2);
	if (it == map.end()) {
		symbols["id"] = tintegrate::generate_format(id, "red");
		symbols["type"] = tintegrate::generate_format(type, "red");
		symbols["definition"] = tintegrate::generate_format(definition, "red");
		symbols["default"] = tintegrate::generate_format("default", "green");

		err << vgettext2("Cannot find widget definition(id: $id, type: $type, definition: $definition)! Force to use definition $default.", symbols);
		gui2::show_message(disp.video(), "", err.str());

		it = map.find("default");
	}
	return it->second;
}


mkwin_display::mkwin_display(mkwin_controller& controller, unit_map& units, CVideo& video, const gamemap& map,
		const config& theme_cfg, const config& level)
	: display(game_config::tile_square, &controller, video, &map, theme_cfg, level, gui2::tmkwin_theme::NUM_REPORTS)
	, controller_(controller)
	, units_(units)
	, current_widget_type_()
	, scroll_header_(NULL)
{
	min_zoom_ = 48;
	max_zoom_ = 144;

	show_hover_over_ = false;

	// must use grid.
	set_grid(controller.theme());

	std::string patch = get_theme_patch();
	theme_current_cfg_ = theme::set_resolution(theme_cfg, w(), h(), patch, theme_cfg_);
	create_theme();
	
	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
	std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = controls.find("spacer")->second.find("default");
	spacer = std::make_pair("spacer", it->second);

	it = controls.find("toggle_button")->second.find("default");
	toggle_button = std::make_pair("toggle_button", it->second);

	it = controls.find("toggle_panel")->second.find("default");
	toggle_panel = std::make_pair("toggle_panel", it->second);

	widget_palette_ = dynamic_cast<gui2::treport*>(get_theme_object("widget-palette"));
	widget_palette_->multiline_init(false, "surface");
	reload_widget_palette();

	scroll_header_ = dynamic_cast<gui2::treport*>(get_theme_object("scroll-header"));
	scroll_header_->tabbar_init(true, "image_tab");
	scroll_header_->set_boddy(dynamic_cast<gui2::twidget*>(get_theme_object("palette_panel")));
	reload_scroll_header();

	click_widget(spacer.first, spacer.second->id);
}

std::string mkwin_display::get_theme_patch() const
{
	std::string id;
	if (!controller_.theme()) {
		id = "dialog";
	}
	return id;
}

gui2::ttheme* mkwin_display::create_theme_dlg(const config& cfg)
{
	return new gui2::tmkwin_theme(*this, controller_, cfg);
}

mkwin_display::~mkwin_display()
{
}

void mkwin_display::pre_change_resolution(std::map<const std::string, bool>& actives)
{
}

void mkwin_display::post_change_resolution(const std::map<const std::string, bool>& actives)
{
	widget_palette_ = dynamic_cast<gui2::treport*>(get_theme_object("widget-palette"));
	widget_palette_->multiline_init(false, "surface");
	reload_widget_palette();

	scroll_header_ = dynamic_cast<gui2::treport*>(get_theme_object("scroll-header"));
	scroll_header_->tabbar_init(true, "image_tab");
	scroll_header_->set_boddy(dynamic_cast<gui2::twidget*>(get_theme_object("palette_panel")));
	reload_scroll_header();

	controller_.post_change_resolution();
}

void mkwin_display::draw_hex(const map_location& loc)
{
	display::draw_hex(loc);
}

void mkwin_display::draw_sidebar()
{
	// Fill in the terrain report
	if (map_->on_board_with_border(mouseoverHex_)) {
		refresh_report(gui2::tmkwin_theme::POSITION, reports::report(reports::report::LABEL, lexical_cast<std::string>(mouseoverHex_), null_str));
	}

	std::stringstream ss;
	ss << zoom_ << "(" << int(get_zoom_factor() * 100) << "%)";
	refresh_report(gui2::tmkwin_theme::VILLAGES, reports::report(reports::report::LABEL, ss.str(), null_str));
	ss.str("");
	ss << selected_widget_.first;
	if (selected_widget_.second.get()) {
		ss << "(" << selected_widget_.second->id << ")";
	}
	refresh_report(gui2::tmkwin_theme::UPKEEP, reports::report(reports::report::LABEL, ss.str(), null_str));
}

gui2::tbutton* create_widget_button(mkwin_controller& controller, const t_string& tooltip, const std::string& type, const std::string& definition)
{
	gui2::tbutton* widget = gui2::create_surface_button(null_str, NULL);
	widget->set_tooltip(tooltip);

	connect_signal_mouse_left_click(
		*widget
		, boost::bind(
			&mkwin_controller::click_widget
			, &controller
			, type
			, definition));
	return widget;
}

void mkwin_display::reload_widget_palette()
{
	widget_palette_->erase_children();
	
	const gui2::tpoint& unit_size = widget_palette_->get_unit_size();
	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;

	static std::set<std::string> exclude;
	if (exclude.empty()) {
		exclude.insert(spacer.first);
		exclude.insert("window");
		exclude.insert("horizontal_scrollbar");
		exclude.insert("vertical_scrollbar");
		// exclude.insert("slider");
	}

	std::stringstream ss;
	surface default_surf(image::get_image(unit::widget_prefix + "bg.png"));
	SDL_Rect dst_rect = create_rect(0, 0, 0, 0);
	for (gui2::tgui_definition::tcontrol_definition_map::const_iterator it = controls.begin(); it != controls.end(); ++ it) {
		const std::string& type = it->first;
		if (exclude.find(type) != exclude.end()) {
			continue;
		}
		for (std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++ it2) {
			const std::string& definition = it2->first;
			if (definition != "default") {
				continue;
			}
			const std::string filename = unit::form_widget_png(type, definition);
			surface surf(image::get_image(filename));
			if (!surf) {
				surf = default_surf;
			}
			surf.assign(make_neutral_surface(surf));

			if (definition == "default") {
				// sdl_blit(image::get_image(unit::widget_prefix + "default.png"), NULL, surf, &dst_rect);
			}

			ss.str("");
			ss << definition << "(" << it2->second->description << ")";
			gui2::tbutton* widget = create_widget_button(controller_, ss.str(), type, definition);
			widget->set_surface(surf, unit_size.x, unit_size.y);
			widget_palette_->insert_child(*widget);
		}
	}

	const config& core_config = controller_.core_config();
	BOOST_FOREACH (const config& tpl, core_config.child_range("tpl_widget")) {
		const std::string& id = tpl["id"].str();
		const std::string filename = unit::form_widget_png(unit::tpl_type, id);
		surface surf(image::get_image(filename));
		if (!surf) {
			surf = default_surf;
		}

		ss.str("");
		ss << id << "(" << tpl["description"].str() << ")";
		gui2::tbutton* widget = create_widget_button(controller_, ss.str(), unit::tpl_type, id);
		widget->set_surface(surf, unit_size.x, unit_size.y);
		widget_palette_->insert_child(*widget);
	}

	widget_palette_->replacement_children();
	scroll_top(*widget_palette_);
}

gui2::ttoggle_button* create_sheet_button(mkwin_controller& controller, const t_string& tooltip, int index)
{
	gui2::ttoggle_button* widget = gui2::create_toggle_button(null_str, "tab", NULL);
	widget->set_tooltip(tooltip);
	widget->set_radio(true);

	return widget;
}

gui2::ttoggle_button* mkwin_display::scroll_header_widget(int index) const
{
	const gui2::tgrid::tchild* children = scroll_header_->content_grid()->children();
	return dynamic_cast<gui2::ttoggle_button*>(children[index].widget_);
}

void mkwin_display::reload_scroll_header()
{
	std::vector<std::string> images;
	images.push_back("buttons/widget-palette.png");
	images.push_back("buttons/object-list.png");

	scroll_header_->erase_children();
	
	int index = 0;
	for (std::vector<std::string>::const_iterator it = images.begin(); it != images.end(); ++ it, index ++) {
		gui2::tcontrol* widget = scroll_header_->create_child(null_str, null_str, reinterpret_cast<void*>(index));
		widget->set_label(*it);
		scroll_header_->insert_child(*widget, index);

	}
	scroll_header_->select(0);

	scroll_header_->replacement_children();
}

void mkwin_display::scroll_top(gui2::treport& widget)
{
	widget.scroll_vertical_scrollbar(gui2::tscrollbar_::BEGIN);
}

void mkwin_display::scroll_bottom(gui2::treport& widget)
{
	widget.scroll_vertical_scrollbar(gui2::tscrollbar_::END);
}

void mkwin_display::click_widget(const std::string& type, const std::string& definition)
{
	surface surf;
	if (type != unit::tpl_type) {
		const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
		const std::map<std::string, gui2::tcontrol_definition_ptr>& definitions = controls.find(type)->second;
		gui2::tcontrol_definition_ptr widget = definitions.find(definition)->second;

		if (selected_widget_.second != widget) {
			selected_widget_ = std::make_pair(type, widget);
			if (type != spacer.first) {
				surf = image::get_image(unit::form_widget_png(type, widget->id));
			}
		}
	} else {
		selected_widget_ = std::make_pair(unit::form_widget_tpl(definition), gui2::tcontrol_definition_ptr());
		surf = image::get_image(unit::form_widget_png(type, definition));
	}
	set_mouse_overlay(surf);
}

void mkwin_display::click_grid(const std::string& type)
{
	if (selected_widget_.first != type) {
		selected_widget_ = std::make_pair(type, gui2::tcontrol_definition_ptr());
        surface surf = image::get_image("buttons/grid.png");
		set_mouse_overlay(surf);
	}
}

void mkwin_display::set_mouse_overlay(surface& image_fg)
{
	if (!image_fg) {
		set_mouseover_hex_overlay(NULL);
		using_mouseover_hex_overlay_ = NULL;
		return;
	}

	// Create a transparent surface of the right size.
	surface image = create_compatible_surface(image_fg, image_fg->w, image_fg->h);
	sdl_fill_rect(image, NULL, SDL_MapRGBA(image->format, 0, 0, 0, 0));

	// For efficiency the size of the tile is cached.
	// We assume all tiles are of the same size.
	// The zoom factor can change, so it's not cached.
	// NOTE: when zooming and not moving the mouse, there are glitches.
	// Since the optimal alpha factor is unknown, it has to be calculated
	// on the fly, and caching the surfaces makes no sense yet.
	static const Uint8 alpha = 196;
	static const int half_size = zoom_ / 2;
	static const int offset = 2;
	static const int new_size = half_size - 2;

	// Blit left side
	image_fg = scale_surface(image_fg, new_size, new_size);
	SDL_Rect rcDestLeft = create_rect(offset, offset, 0, 0);
	sdl_blit ( image_fg, NULL, image, &rcDestLeft );

	// Add the alpha factor and scale the image
	image = adjust_surface_alpha(image, alpha);

	// Set as mouseover
	set_mouseover_hex_overlay(image);
	using_mouseover_hex_overlay_ = image;
}

void mkwin_display::draw_minimap_units(surface& screen)
{
	static std::vector<SDL_Color> candidates;
	if (candidates.empty()) {
		candidates.push_back(font::TITLE_COLOR);
		candidates.push_back(font::BAD_COLOR);
		candidates.push_back(font::GRAY_COLOR);
		candidates.push_back(font::BLACK_COLOR);
		candidates.push_back(font::GOOD_COLOR);
	}
	
	double xscaling = 1.0 * minimap_location_.w / map_->w();
	double yscaling = 1.0 * minimap_location_.h / map_->h();

	if (controller_.theme()) {
		unit* current_unit = controller_.current_unit();
		if (!current_unit) {
			std::vector<SDL_Rect> rects;
			xscaling = 1.0 * minimap_location_.w / (map_->w() * hex_width());
			yscaling = 1.0 * minimap_location_.h / (map_->h() * hex_width());

			for (unit_map::const_iterator it = units_.begin(); it != units_.end(); ++ it) {
				const unit* u = dynamic_cast<const unit*>(&*it);
				
				if (u->type() != unit::WIDGET) {
					continue;
				}

				const SDL_Rect& rect = u->get_rect();
				double u_x = rect.x * xscaling;
				double u_y = rect.y * yscaling;
				double u_w = rect.w * xscaling;
				double u_h = rect.h * yscaling;

				SDL_Color col = font::GOOD_COLOR;
				if (u->cell().id != theme::id_main_map) {
					int level = 1;	// level = 1 is red.
					for (std::vector<SDL_Rect>::const_iterator it = rects.begin(); it != rects.end(); ++ it) {
						const SDL_Rect& that = *it;
						if (rects_overlap(that, rect)) {
							level ++;
						}
					}
					col = candidates[level % candidates.size()];;
				}
				rects.push_back(rect);

				const Uint32 mapped_col = SDL_MapRGB(screen->format, col.r, col.g, col.b);

				draw_rectangle(minimap_location_.x + round_double(u_x)
					, minimap_location_.y + round_double(u_y)
					, round_double(u_w)
					, round_double(u_h)
					, mapped_col, screen);

			}
		} else {
			std::vector<unit::tchild>& children = current_unit->children();
			for (std::vector<unit::tchild>::const_iterator it = children.begin(); it != children.end(); ++ it) {
				const unit::tchild& child = *it;
				child.draw_minimap_architecture(screen, minimap_location_, xscaling, yscaling, 1);
			}
		}

	} else if (controller_.top().window) {
		controller_.top().draw_minimap_architecture(screen, minimap_location_, xscaling, yscaling, 0);
	}

	const unit* copied_unit = controller_.copied_unit();
	if (copied_unit) {
		double u_x = copied_unit->get_location().x * xscaling;
		double u_y = copied_unit->get_location().y * yscaling;
		double u_w = xscaling;
		double u_h = yscaling;

		const SDL_Color col = font::GOOD_COLOR;
		const Uint32 mapped_col = SDL_MapRGB(screen->format, col.r, col.g, col.b);

		SDL_Rect r = create_rect(minimap_location_.x + round_double(u_x)
				, minimap_location_.y + round_double(u_y)
				, round_double(u_w)
				, round_double(u_h));

		sdl_fill_rect(screen, &r, mapped_col);
/*
		{
			surface_lock locker(screen);
			draw_circle(screen, mapped_col, r.x + r.w / 2, r.y + r.h / 2, r.w / 2 + 2, false);
		}
*/
	}
}

void mkwin_display::post_set_zoom(int last_zoom)
{
	if (!controller_.in_theme_top()) {
		return;
	}
	double factor = double(zoom_) / double(last_zoom);
	SDL_Rect new_rect;
	// don't use units_, set_rect maybe change map_'s sequence.
	std::vector<unit*> top_units;
	for (unit_map::const_iterator it = units_.begin(); it != units_.end(); ++ it) {
		unit* u = dynamic_cast<unit*>(&*it);
		top_units.push_back(u);
	}
	for (std::vector<unit*>::const_iterator it = top_units.begin(); it != top_units.end(); ++ it) {
		unit* u = *it;
		const SDL_Rect& rect = u->get_rect();
		new_rect = create_rect(rect.x * factor, rect.y * factor, rect.w * factor, rect.h * factor);
		u->set_rect(new_rect);
	}
}

void mkwin_display::pre_draw(rect_of_hexes& hexes)
{
	// generate all units in current win.
	draw_area_unit_size_ = units_.units_from_rect(draw_area_unit_, draw_area_rect_);
}

void mkwin_display::draw_invalidated()
{
	std::vector<map_location> unit_invals;

	for (size_t i = 0; i < draw_area_unit_size_; i ++) {
		const base_unit* u = draw_area_unit_[i];

		const std::set<map_location>& draw_locs = u->get_draw_locations();
		invalidate(draw_locs);

		const map_location& loc = u->get_location();
		// draw_area_val(loc.x, loc.y) = INVALIDATE_UNIT;
		unit_invals.push_back(loc);
	}

	display::draw_invalidated();
	redraw_units(unit_invals);
}

void mkwin_display::redraw_units(const std::vector<map_location>& invalidated_unit_locs)
{
	// Units can overlap multiple hexes, so we need
	// to redraw them last and in the good sequence.
	BOOST_FOREACH (const map_location& loc, invalidated_unit_locs) {
		base_unit* u = units_.find_base_unit(loc, false);
		if (u) {
			u->redraw_unit();
		}
		u = units_.find_base_unit(loc, true);
		if (u) {
			u->redraw_unit();
		}
	}
	for (std::map<int, animation*>::iterator it = area_anims_.begin(); it != area_anims_.end(); ++ it) {
		tanim_type type = it->second->type();
		VALIDATE(type == anim_map || type == anim_canvas, miss_anim_err_str);
		if (type != anim_map) {
			continue;
		}

		it->second->update_last_draw_time();
		anim2::rt.type = it->second->type();
		it->second->redraw(screen_.getSurface(), empty_rect);
	}
}