/* $Id: mp_login.cpp 50955 2011-08-30 19:41:15Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/dialogs/design.hpp"

#include "SDL_image.h"
#include "display.hpp"
#include "rose_config.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/scroll_label.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/track.hpp"
#include "gui/widgets/window.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "integrate.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "wml_separators.hpp"

#include <boost/bind.hpp>

struct tcrop
{
	tcrop(const SDL_Rect& r, const std::string& name)
		: r(r)
		, name(name)
		, alphamask()
	{}

	SDL_Rect r;
	std::string name;
	std::string alphamask;
};

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_mp_login
 *
 * == Multiplayer connect ==
 *
 * This shows the dialog to log in to the MP server
 *
 * @begin{table}{dialog_widgets}
 *
 * user_name & & text_box & m &
 *         The login user name. $
 *
 * password & & text_box & m &
 *         The password. $
 *
 * remember_password & & toggle_button & o &
 *         A toggle button to offer to remember the password in the
 *         preferences. $
 *
 * password_reminder & & button & o &
 *         Request a password reminder. $
 *
 * change_username & & button & o &
 *         Use a different username. $
 *
 * login_label & & button & o &
 *         Displays the information received from the server. $
 *
 * @end{table}
 */

REGISTER_DIALOG(design)

std::map<tdesign::tstyle, std::string> tdesign::styles;

tdesign::tdesign(display& disp)
	: disp_(disp)
	, style_(style_min)
{
	if (design_dir.empty()) {
		design_dir = game_config::preferences_dir_utf8 + "/editor";
		input_png = "input.png";
	}

	if (styles.empty()) {
		styles.insert(std::make_pair(style_transition, "design/style-transition.png"));
		styles.insert(std::make_pair(style_stuff, "design/style-stuff.png"));
		styles.insert(std::make_pair(style_nine, "design/style-nine.png"));
	}
}

const size_t max_text_size = 30;

void tdesign::pre_show(CVideo& /*video*/, twindow& window)
{
	window.set_canvas_variable("border", variant("default-border"));

	std::stringstream strstr;

	tlabel* label = find_widget<tlabel>(&window, "title", false, true);
	strstr.str("");
	strstr << _("Design") << "(" << design_dir << ")";
	label->set_label(strstr.str());

	ttrack* portrait = find_widget<ttrack>(&window, "portrait", false, true);
	portrait->set_canvas_variable("background_image", variant(null_str));
	portrait->set_callback_timer(boost::bind(&tdesign::portrait_callback_timer, this, _1, _2, _3, _4, _5));
	portrait->set_enable_drag_draw_coordinate(false);

	ttext_box* user_widget = find_widget<ttext_box>(&window, "first_input", false, true);
	user_widget->set_maximum_length(max_text_size);

	user_widget = find_widget<ttext_box>(&window, "second_input", false, true);
	user_widget->set_maximum_length(max_text_size);

	strstr.str("");
	label = find_widget<tlabel>(&window, "remark", false, true);
	strstr << _("Input filename") << ": " << input_png;
	label->set_label(strstr.str());

	refresh_according_to_style(window);

	connect_signal_mouse_left_click(
		find_widget<tbutton>(&window, "style", false)
		, boost::bind(
		&tdesign::style
		, this
		, boost::ref(window)));
	find_widget<tbutton>(&window, "style", false).set_label(tintegrate::generate_img(styles.find(style_)->second));

	connect_signal_mouse_left_click(
		find_widget<tbutton>(&window, "execute", false)
		, boost::bind(
		&tdesign::execute
		, this
		, boost::ref(window)));
}

void tdesign::post_show(twindow& window)
{
}

void tdesign::refresh_according_to_style(twindow& window)
{
	std::stringstream strstr;

	// first tag/input
	strstr.str("");
	if (style_ == style_transition) {
		strstr << _("Transition filename prefix(Include '-' if necessary");
	} else if (style_ == style_stuff) {
		strstr << _("Stuff filename prefix(Include '-' if necessary");
	}
	tlabel* label = find_widget<tlabel>(&window, "first_tag", false, true);
	label->set_label(strstr.str());

	ttext_box* user_widget = find_widget<ttext_box>(&window, "first_input", false, true);
	strstr.str("");
	if (style_ == style_transition) {
		strstr << "green-abrupt-";
	} else if (style_ == style_stuff) {
		strstr << "green-stuff-";
	} else if (style_ == style_nine) {
		strstr << "thermometer2-";
	}
	user_widget->set_label(strstr.str());

	// second tag/input
	strstr.str("");
	label = find_widget<tlabel>(&window, "second_tag", false, true);
	if (style_ == style_transition) {
		strstr << _("Terrain filename(Exclude '.png')");
		label->set_label(strstr.str());
		label->set_visible(twidget::VISIBLE);
	} else if (style_ == style_stuff) {
		label->set_visible(twidget::INVISIBLE);
	} else if (style_ == style_nine) {
		strstr << _("corner(w,h),mid(w,h)");
		label->set_label(strstr.str());
		label->set_visible(twidget::VISIBLE);
	}

	user_widget = find_widget<ttext_box>(&window, "second_input", false, true);
	strstr.str("");
	if (style_ == style_transition) {
		strstr << "green";
		user_widget->set_label(strstr.str());
		user_widget->set_visible(twidget::VISIBLE);
	} else if (style_ == style_stuff) {
		user_widget->set_visible(twidget::INVISIBLE);
	} else if (style_ == style_nine) {
		strstr << "12,12,64,64";
		user_widget->set_label(strstr.str());
		user_widget->set_visible(twidget::VISIBLE);
	}
}

std::string tdesign::text_box_str(twindow& window, const std::string& id, const std::string& name, int min, int max, bool allow_empty)
{
	std::stringstream err;
	utils::string_map symbols;

	ttext_box* widget = find_widget<ttext_box>(&window, id, false, true);
	std::string str = widget->label();

	if (!allow_empty && str.empty()) {
		symbols["key"] = tintegrate::generate_format(name, "red");
		
		err << vgettext2("Invalid '$key' value, not accept empty", symbols);
		gui2::show_message(disp_.video(), "", err.str());
		return str;
	} else if ((int)str.size() < min || (int)str.size() > max) {
		symbols["min"] = tintegrate::generate_format(min, "yellow");
		symbols["max"] = tintegrate::generate_format(max, "yellow");
		symbols["key"] = tintegrate::generate_format(name, "red");
		
		if (min != max) {
			err << vgettext2("'$key' value must combine $min to $max characters", symbols);
		} else {
			err << vgettext2("'$key' value must be $min characters", symbols);
		}
		gui2::show_message(disp_.video(), "", err.str());
		return null_str;

	}
	return str;
}

void tdesign::style(twindow& window)
{
	std::vector<tval_str> items;

	for (std::map<tstyle, std::string>::const_iterator it = styles.begin(); it != styles.end(); ++ it) {
		items.push_back(tval_str(it->first, tintegrate::generate_img(it->second)));
	}

	gui2::tcombo_box dlg(items, style_);
	dlg.show(disp_.video());

	style_ = (tstyle)dlg.selected_val();

	tcontrol* label = find_widget<tcontrol>(&window, "style", false, true);
	label->set_label(tintegrate::generate_img(styles.find(style_)->second));

	refresh_according_to_style(window);
}

void tdesign::portrait_callback_timer(ttrack& widget, surface& frame_buffer, const tpoint& offset, int state2, bool from_timer)
{
	if (!frame_buffer || from_timer) {
		return;
	}

	SDL_Rect widget_rect = widget.get_rect();

	const int xsrc = offset.x + widget_rect.x;
	const int ysrc = offset.y + widget_rect.y;
	SDL_Rect dst = empty_rect;

	int offset_x, offset_y = 0;

	const std::string input_png2 = design_dir + "/" + input_png;
	surface surf = image::get_image(input_png2);
	if (!surf) {
		surf = font::get_rendered_text2(_("Can not find input.png"), -1, 12 * twidget::hdpi_scale, font::BLACK_COLOR);
	}
	offset_x = (widget_rect.w - surf->w) / 2;
	offset_y = (widget_rect.h - surf->h) / 2;
	dst = ::create_rect(xsrc + offset_x, ysrc + offset_y, 0, 0);
	sdl_blit(surf, NULL, frame_buffer, &dst);
}

void tdesign::execute(twindow& window)
{
	std::string first_str = text_box_str(window, "first_input", _("First edit box input"), 2, 28, false);
	if (first_str.empty()) {
		return;
	}

	std::string second_str = text_box_str(window, "second_input", _("Second edit box input"), 2, 19, false);
	
	std::vector<tcrop> items;
	const std::string input_png2 = design_dir + "/" + input_png;
	surface surf = image::get_image(input_png2);
	if (!surf) {
		return;
	}

	if (style_ == style_transition) {
		if (surf->w != 180 && surf->h != 216) {
			gui2::show_message(disp_.video(), "", "input png must be 180x216");
			return;
		}
		if (second_str.empty()) {
			return;
		}
		const std::string transiton_prefix = design_dir + "/" + first_str;
		const std::string base = design_dir + "/" + second_str + ".png";

		items.push_back(tcrop(::create_rect(54, 0, 72, 72), transiton_prefix + "s.png"));
		items.push_back(tcrop(::create_rect(108, 36, 72, 72), transiton_prefix + "sw.png"));
		items.push_back(tcrop(::create_rect(108, 108, 72, 72), transiton_prefix + "nw.png"));
		items.push_back(tcrop(::create_rect(54, 144, 72, 72), transiton_prefix + "n.png"));
		items.push_back(tcrop(::create_rect(0, 108, 72, 72), transiton_prefix + "ne.png"));
		items.push_back(tcrop(::create_rect(0, 36, 72, 72), transiton_prefix + "se.png"));
		items.push_back(tcrop(::create_rect(54, 72, 72, 72), base));

	} else if (style_ == style_stuff) {
		if (surf->w != 300 && surf->h != 300) {
			gui2::show_message(disp_.video(), "", "input png must be 300x300");
			return;
		}
		const std::string stuff_prefix = design_dir + "/" + first_str;

		items.push_back(tcrop(::create_rect(0, 0, -1, -1), stuff_prefix + "n.png"));
		items.back().alphamask = "design/alphamask-n.png";
		items.push_back(tcrop(::create_rect(0, 0, -1, -1), stuff_prefix + "ne.png"));
		items.back().alphamask = "design/alphamask-ne.png";
		items.push_back(tcrop(::create_rect(0, 0, -1, -1), stuff_prefix + "se.png"));
		items.back().alphamask = "design/alphamask-se.png";
		items.push_back(tcrop(::create_rect(0, 0, -1, -1), stuff_prefix + "s.png"));
		items.back().alphamask = "design/alphamask-s.png";
		items.push_back(tcrop(::create_rect(0, 0, -1, -1), stuff_prefix + "sw.png"));
		items.back().alphamask = "design/alphamask-sw.png";
		items.push_back(tcrop(::create_rect(0, 0, -1, -1), stuff_prefix + "nw.png"));
		items.back().alphamask = "design/alphamask-nw.png";

	} else if (style_ == style_nine) {
		std::vector<std::string> vstr = utils::split(second_str);
		if (vstr.size() != 4) {
			return;
		}
		int corner_width = lexical_cast_default<int>(vstr[0]);
		int corner_height = lexical_cast_default<int>(vstr[1]);
		int mid_width = lexical_cast_default<int>(vstr[2]);
		int mid_height = lexical_cast_default<int>(vstr[3]);

		if (corner_width <= 0 || corner_height <= 0 || mid_width <= 0 || mid_height < 0) {
			return;
		}
		if (2 * corner_width + mid_width > surf->w || 2 * corner_height + mid_height > surf->h) {
			return;
		}
		const std::string transiton_prefix = design_dir + "/" + first_str;
		items.push_back(tcrop(::create_rect(0, 0, corner_width, corner_height), transiton_prefix + "topleft.png"));
		items.push_back(tcrop(::create_rect(surf->w - corner_width, 0, corner_width, corner_height), transiton_prefix + "topright.png"));
		items.push_back(tcrop(::create_rect(0, surf->h - corner_height, corner_width, corner_height), transiton_prefix + "botleft.png"));
		items.push_back(tcrop(::create_rect(surf->w - corner_width, surf->h - corner_height, corner_width, corner_height), transiton_prefix + "botright.png"));
		items.push_back(tcrop(::create_rect(corner_width, 0, mid_width, corner_height), transiton_prefix + "top.png"));
		items.push_back(tcrop(::create_rect(corner_width, surf->h - corner_height, mid_width, corner_height), transiton_prefix + "bottom.png"));
		if (mid_height) {
			items.push_back(tcrop(::create_rect(0, corner_height, corner_width, mid_height), transiton_prefix + "left.png"));
			items.push_back(tcrop(::create_rect(surf->w - corner_width, corner_height, corner_width, mid_height), transiton_prefix + "right.png"));
			items.push_back(tcrop(::create_rect(corner_width, corner_height, mid_width, mid_height), transiton_prefix + "background.png"));
		}
	}

	for (std::vector<tcrop>::const_iterator it = items.begin(); it != items.end(); ++ it) {
		const tcrop& crop = *it;
		surface middle;
		if (crop.r.w > 0) {
			middle = cut_surface(surf, crop.r);
		} else {
			middle = surf;
		}
		if (style_ != style_nine) {
			surface masksurf = crop.alphamask.empty()? image::get_hexmask(): image::get_image(crop.alphamask);
			middle = mask_surface(middle, masksurf);
		}
		IMG_SavePNG(middle, crop.name.c_str());
	}

	gui2::show_message(disp_.video(), "", "Execute finished!");
}

} // namespace gui2

