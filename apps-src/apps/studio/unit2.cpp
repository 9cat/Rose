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

#include "unit2.hpp"
#include "gettext.hpp"
#include "mkwin_display.hpp"
#include "mkwin_controller.hpp"
#include "gui/dialogs/mkwin_theme.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/report.hpp"
#include "gui/auxiliary/window_builder/helper.hpp"
#include "game_config.hpp"

#include <boost/bind.hpp>

unit2::unit2(mkwin_controller& controller, mkwin_display& disp, unit_map& units, const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget, unit* parent, const SDL_Rect& rect)
	: unit(controller, disp, units, widget, parent, -1)
{
	rect_ = rect;
}

unit2::unit2(mkwin_controller& controller, mkwin_display& disp, unit_map& units, int type, unit* parent, const SDL_Rect& rect)
	: unit(controller, disp, units, type, parent, -1)
{
	set_rect(rect);
}

unit2::unit2(const unit2& that)
	: unit(that)
{
}

void unit2::redraw_unit()
{
	if (!loc_.valid()) {
		return;
	}

	if (refreshing_) {
		return;
	}
	trefreshing_lock lock(*this);

	const int xsrc = disp_.map_area().x + disp_.get_scroll_pixel_x(rect_.x);
	const int ysrc = disp_.map_area().y + disp_.get_scroll_pixel_y(rect_.y);

	surface surf = create_neutral_surface(rect_.w, rect_.h);

	if (type_ == WIDGET) {
		redraw_widget(xsrc, ysrc, rect_.w, rect_.h);

	} else if (type_ == WINDOW) {
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, 
			loc_, xsrc, ysrc, image::get_image(image(), image::SCALED_TO_ZOOM));

		if (!cell_.id.empty()) {
			surface text_surf = font::get_rendered_text2(cell_.id, -1, 10 * gui2::twidget::hdpi_scale, font::BLACK_COLOR);
			disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, loc_, xsrc, ysrc + rect_.w / 2, text_surf);
		}
		
	} else if (type_ == COLUMN) {
		int x = (loc_.x - 1) * image::tile_size;
		blit_integer_surface(x, surf, 0, 0);

	} else if (type_ == ROW) {
		int y = (loc_.y - 1) * image::tile_size;
		blit_integer_surface(y, surf, 0, 0);
	}

	blit_integer_surface(map_index_, surf, 0, 12);

	if (type_ == WINDOW || type_ == WIDGET) {
		//
		// draw rectangle
		//
		Uint32 border_color = 0x00ff00ff;
		if (loc_ == controller_.selected_hex()) {
			border_color = 0x0000ffff;
		} else if (this == controller_.last_unit()) {
			border_color = 0xff0000ff;
		}
		surface_lock locker(surf);

		// draw the border
		for (unsigned i = 0; i < 2; i ++) {
			const unsigned left = 0 + i;
			const unsigned right = left + rect_.w - (i * 2) - 1;
			const unsigned top = 0 + i;
			const unsigned bottom = top + rect_.h - (i * 2) - 1;

			// top horizontal (left -> right)
			draw_line(surf, border_color, left, top, right, top, true);

			// right vertical (top -> bottom)
			draw_line(surf, border_color, right, top, right, bottom, true);

			// bottom horizontal (left -> right)
			draw_line(surf, border_color, left, bottom, right, bottom, true);

			// left vertical (top -> bottom)
			draw_line(surf, border_color, left, top, left, bottom, true);
		}
	}

	disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, loc_, xsrc, ysrc, surf);
}

bool unit2::sort_compare(const base_unit& that_base) const
{
	const unit2* that = dynamic_cast<const unit2*>(&that_base);
	if (type_ != WIDGET) {
		if (that->type() == WIDGET) {
			return true;
		}
		return unit::sort_compare(that_base);
	}
	if (that->type() != WIDGET) {
		return false;
	}
	if (rect_.x != that->rect_.x || rect_.y != that->rect_.y) {
		return rect_.y < that->rect_.y || (rect_.y == that->rect_.y && rect_.x < that->rect_.x);
	}
	return unit::sort_compare(that_base);
}

void unit2::generate_window(config& cfg) const
{
	cfg["id"] = cell_.id;
	cfg["description"] = t_string(cell_.window.description, cell_.window.textdomain);
	if (cell_.window.orientation != gui2::twidget::auto_orientation) {
		cfg["orientation"] = gui2::orientations.find(cell_.window.orientation)->second.id;
	}

	config& res_cfg = cfg.add_child("resolution");
	res_cfg["id"] = "1024x768";
	res_cfg["width"] = "1024";
	res_cfg["height"] = "768";

	config& screen_cfg = res_cfg.add_child("screen");
	screen_cfg["id"] = "screen";
	screen_cfg["rect"] = "0,0,1024,768";

	config& border_cfg = res_cfg.add_child("main_map_border");
	generate_main_map_border(border_cfg);

	controller_.generate_linked_groups(res_cfg);
	controller_.generate_context_menus(res_cfg);
}

void unit2::generate_main_map_border(config& cfg) const
{
	if (cell_.window.tile_shape == game_config::tile_hex) {
		cfg["border_size"] = 0.5;
		cfg["background_image"] = "terrain-hexagonal/off-map/background.png";

	} else {
		cfg["border_size"] = 0;
		cfg["view_rectangle_color"] = "blue";

		cfg["background_image"] = "terrain-square/off-map/background.png";
	}
	if (!cell_.window.tile_shape.empty()) {
		cfg["tile_shape"] = cell_.window.tile_shape;
	}
	cfg["tile_image"] = "off-map/alpha.png";

	cfg["corner_image_top_left"] = "terrain-hexagonal/off-map/fade_corner_top_left_editor.png";
	cfg["corner_image_bottom_left"] = "terrain-hexagonal/off-map/fade_corner_bottom_left_editor.png";

	// odd means the corner is on a tile with an odd x value,
	// the tile is the ingame tile not the odd in C++
	cfg["corner_image_top_right_odd"] = "terrain-hexagonal/off-map/fade_corner_top_right_odd_editor.png";
	cfg["corner_image_top_right_even"] = "terrain-hexagonal/off-map/fade_corner_top_right_even_editor.png";

	cfg["corner_image_bottom_right_odd"] = "terrain-hexagonal/off-map/fade_corner_bottom_right_odd_editor.png";
	cfg["corner_image_bottom_right_even"] = "terrain-hexagonal/off-map/fade_corner_bottom_right_even_editor.png";

	cfg["border_image_left"] = "terrain-hexagonal/off-map/fade_border_left_editor.png";
	cfg["border_image_right"] = "terrain-hexagonal/off-map/fade_border_right_editor.png";

	cfg["border_image_top_odd"] = "terrain-hexagonal/off-map/fade_border_top_odd_editor.png";
	cfg["border_image_top_even"] = "terrain-hexagonal/off-map/fade_border_top_even_editor.png";

	cfg["border_image_bottom_odd"] = "terrain-hexagonal/off-map/fade_border_bottom_odd_editor.png";
	cfg["border_image_bottom_even"] = "terrain-hexagonal/off-map/fade_border_bottom_even_editor.png";
}

void unit2::from_window(const config& cfg)
{
	// [theme] <= cfg
	// [/theme]

	cell_.id = cfg["id"].str();
	t_string description = cfg["description"].t_str();
	split_t_string(description, cell_.window.textdomain, cell_.window.description);

	cell_.window.orientation = gui2::implementation::get_orientation(cfg["orientation"]);

	const config& main_map_cfg = cfg.child("resolution").child("main_map_border");
	cell_.window.tile_shape = main_map_cfg["tile_shape"].str();

	std::set<std::string> shapes;
	const config& core_config = controller_.core_config();
	BOOST_FOREACH (const config &c, core_config.child_range("tb")) {
		const std::string& id = c["id"].str();
		shapes.insert(id);
	}

	if (shapes.find(cell_.window.tile_shape) == shapes.end()) {
		cell_.window.tile_shape = game_config::tile_square;
	}
}