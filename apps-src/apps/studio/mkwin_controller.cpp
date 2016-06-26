/* $Id: editor_controller.cpp 47755 2010-11-29 12:57:31Z shadowmaster $ */
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

#include "mkwin_controller.hpp"
#include "mkwin_display.hpp"

#include "gettext.hpp"
#include "integrate.hpp"
#include "formula_string_utils.hpp"
#include "preferences.hpp"
#include "sound.hpp"
#include "filesystem.hpp"
#include "hotkeys.hpp"
#include "config_cache.hpp"
#include "preferences_display.hpp"
#include "gui/dialogs/mkwin_theme.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/system.hpp"
#include "gui/dialogs/row_setting.hpp"
#include "gui/dialogs/column_setting.hpp"
#include "gui/dialogs/window_setting.hpp"
#include "gui/dialogs/control_setting.hpp"
#include "gui/dialogs/linked_group.hpp"
#include "gui/dialogs/drawing_setting.hpp"
#include "gui/dialogs/grid_setting.hpp"
#include "gui/dialogs/browse.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/listbox.hpp"
#include "serialization/parser.hpp"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

const std::string noise_str = "_magic5a5anoise";
std::string noise_config_key(const std::string& key)
{
	return key + noise_str;
}

std::string denoise_config_key(const std::string& noised)
{
	const size_t noise_size = noise_str.size();
	size_t noised_size = noised.size();

	if (noised_size > noise_size && noised.find(noise_str) == noised_size - noise_size) {
		return noised.substr(0, noised_size - noise_size);
	}
	return null_str;
}

bool is_noised_config_key(const std::string& noised)
{
	const size_t noise_size = noise_str.size();
	size_t noised_size = noised.size();

	return noised_size > noise_size && noised.find(noise_str) == noised_size - noise_size;
}


void square_fill_frame(t_translation::t_map& tiles, size_t start, const t_translation::t_terrain& terrain, bool front, bool back)
{
	if (front) {
		// first column(border)
		for (size_t n = start; n < tiles[0].size() - start; n ++) {
			tiles[start][n] = terrain;
		}
		// first line(border)
		for (size_t n = start + 1; n < tiles.size() - start; n ++) {
			tiles[n][start] = terrain;
		}
	}

	if (back) {
		// last column(border)
		for (size_t n = start; n < tiles[0].size() - start; n ++) {
			tiles[tiles.size() - start - 1][n] = terrain;
		}
		// last line(border)
		for (size_t n = start; n < tiles.size() - start - 1; n ++) {
			tiles[n][tiles[0].size() - start - 1] = terrain;
		}
	}
}

std::string generate_map_data(int width, int height, bool colorful)
{
	VALIDATE(width > 0 && height > 0, "map must not be empty!");

	const t_translation::t_terrain normal = t_translation::read_terrain_code("Gg");
	const t_translation::t_terrain border = t_translation::read_terrain_code("Gs");
	const t_translation::t_terrain control = t_translation::read_terrain_code("Gd");
	const t_translation::t_terrain forbidden = t_translation::read_terrain_code("Gll");

	t_translation::t_map tiles(width + 2, t_translation::t_list(height + 2, normal));
	if (colorful) {
		square_fill_frame(tiles, 0, border, true, true);
		square_fill_frame(tiles, 1, control, true, false);

		const size_t border_size = 1;
		tiles[border_size][border_size] = forbidden;
	}

	// tiles[border_size][tiles[0].size() - border_size - 1] = forbidden;

	// tiles[tiles.size() - border_size - 1][border_size] = forbidden;
	// tiles[tiles.size() - border_size - 1][tiles[0].size() - border_size - 1] = forbidden;

	std::string str = gamemap::default_map_header + t_translation::write_game_map(t_translation::t_map(tiles));

	return str;
}

std::string generate_anchor_rect(int x, int y, int w, int h)
{
	std::stringstream ss;
	ss << x << "," << y << "," << x + w << "," << y + h;
	return ss.str();
}

tmenu2::~tmenu2()
{
	for (std::vector<titem>::iterator it = items.begin(); it != items.end(); ++ it) {
		titem& item = *it;
		if (item.submenu) {
			delete item.submenu;
			item.submenu = NULL;
		}
	}
}

tmenu2* tmenu2::top_menu()
{
	tmenu2* result = this;
	while (result->parent) {
		result = result->parent;
	}
	return result;
}

bool tmenu2::id_existed(const std::string& id) const
{
	for (std::vector<titem>::const_iterator it = items.begin(); it != items.end(); ++ it) {
		const titem& item = *it;
		if (item.id == id) {
			return true;
		}
		if (item.submenu) {
			if (item.submenu->id_existed(id)) {
				return true;
			}
		}
	}
	return false;
}

void tmenu2::submenus(std::vector<tmenu2*>& result) const
{
	for (std::vector<titem>::const_iterator it = items.begin(); it != items.end(); ++ it) {
		const titem& item = *it;
		if (item.submenu) {
			result.push_back(item.submenu);
			item.submenu->submenus(result);
		}
	}
}

void tmenu2::generate(config& cfg) const
{
	if (items.empty()) {
		return;
	}

	std::vector<tmenu2*> submenus;
	std::stringstream ss;
	for (std::vector<titem>::const_iterator it = items.begin(); it != items.end(); ++ it) {
		const titem& item = *it;
		if (!ss.str().empty()) {
			ss << ", ";
		}
		ss << item.id;
		if (item.submenu) {
			ss << "_m";
			submenus.push_back(item.submenu);
		}
		if (item.hide || item.param) {
			ss << "|";
			if (item.hide) {
				ss << "h";
			}
			if (item.param) {
				ss << "p";
			}
		}
	}
	if (!report.empty()) {
		cfg["report"] = report;
	}
	cfg[id] = ss.str();

	for (std::vector<tmenu2*>::const_iterator it = submenus.begin(); it != submenus.end(); ++ it) {
		const tmenu2& menu = **it;
		menu.generate(cfg);
	}
}

mkwin_controller::ttheme_top_lock::ttheme_top_lock(mkwin_controller& controller)
	: controller_(controller)
	, require_clear_(!!controller.current_unit_)
{
	VALIDATE(controller_.theme_, "Must in theme!");
	controller_.top_.units = controller_.form_top_units();
}

mkwin_controller::ttheme_top_lock::~ttheme_top_lock()
{
	controller_.top_.units.clear();
}

const int mkwin_controller::default_child_width = 5;
const int mkwin_controller::default_child_height = 4;
const int mkwin_controller::default_dialog_width = 48;
const int mkwin_controller::default_dialog_height = 24;

std::string mkwin_controller::widget_template_cfg()
{
	return game_config::path + "/data/gui/default/macros/template.cfg";
}

mkwin_controller::mkwin_controller(const config &top_config, CVideo& video, bool theme)
	: controller_base(SDL_GetTicks(), top_config, video)
	, gui_(NULL)
	, map_(top_config, null_str)
	, units_(*this, map_, !theme)
	, current_unit_(NULL)
	, copied_unit_(NULL)
	, last_unit_(NULL)
	, file_()
	, top_()
	, theme_top_()
	, theme_(theme)
	, theme_widget_start_(0)
	, top_rect_cfg_(NULL)
	, modes_()
	, context_menus_(1, tmenu2(null_str, "main", NULL))
	, callback_sstr_()
	, do_quit_(false)
	, quit_mode_(EXIT_ERROR)
{
	gui2::init_layout_mode();

	textdomains_.push_back("rose-lib");
	textdomains_.push_back(game_config::app + "-lib");

	reset_modes();
	tadjust::init_fields();

	VALIDATE(default_dialog_width >= default_child_width, null_str);
	VALIDATE(default_dialog_height >= default_child_height, null_str);

	int original_width = default_child_width;
	int original_height = default_child_height;
	if (theme_) {
		original_width = 1 + ceil(1.0 * theme::XDim / image::tile_size);
		original_height = 1 + ceil(1.0 * theme::YDim / image::tile_size);
	}
	map_ = gamemap(top_config, generate_map_data(original_width, original_height, theme_));

	units_.create_coor_map(map_.w(), map_.h());

	init_gui(video);
	init_music(top_config);

	fill_spacer(NULL, -1);
	
	if (theme_) {
		theme_widget_start_ = original_width + original_height - 1;
		int zoom = gui_->hex_width();
		SDL_Rect rect = create_rect(zoom, zoom, zoom * 2, zoom * 2);
		unit2* tmp = new unit2(*this, *gui_, units_, std::make_pair("main_map", gui2::tcontrol_definition_ptr()), current_unit_, rect);
		gui2::tcell_setting& cell = tmp->cell();
		
		cell.id = theme::id_main_map;
		cell.rect_cfg = tadjust::generate_empty_rect_cfg();
		cell.rect_cfg["ref"] = theme::id_screen;
		cell.rect_cfg["rect"] = "=,=,+144,+144";
		cell.rect_cfg["xanchor"] = gui2::horizontal_anchor.find(theme::TOP_ANCHORED)->second.id;
		cell.rect_cfg["yanchor"] = gui2::vertical_anchor.find(theme::TOP_ANCHORED)->second.id;
		units_.insert2(*gui_, tmp);
	}
	units_.save_map_to(top_, false);
	if (theme_) {
		top_.units.clear();
	}

	set_status();
	gui_->show_context_menu();

}

void mkwin_controller::init_gui(CVideo& video)
{
	const config &theme_cfg = gui2::get_theme("mkwin");
	gui_ = new mkwin_display(*this, units_, video, map_, theme_cfg, config());
}

void mkwin_controller::init_music(const config& game_config)
{
	const config &cfg = game_config.child("editor_music");
	if (!cfg) {
		return;
	}
	BOOST_FOREACH (const config &i, cfg.child_range("music")) {
		sound::play_music_config(i);
	}
	sound::commit_music_changes();
}


mkwin_controller::~mkwin_controller()
{
	if (gui_) {
		delete gui_;
		gui_ = NULL;
	}
}

void mkwin_controller::reset_modes()
{
	modes_.clear();
	insert_mode(tmode::def_id);
}

EXIT_STATUS mkwin_controller::main_loop()
{
	try {
		while (!do_quit_) {
			play_slice();
		}
	} catch (twml_exception& e) {
		e.show(gui());
	}
	return quit_mode_;
}

events::mouse_handler_base& mkwin_controller::get_mouse_handler_base()
{
	return *this;
}

void mkwin_controller::execute_command2(int command, const std::string& sparam)
{
	using namespace gui2;
	unit* u = units_.find_unit(selected_hex_);

	switch (command) {
		case tmkwin_theme::HOTKEY_SELECT:
			click_widget(gui_->spacer.first, gui_->spacer.second->id);
			break;
		case tmkwin_theme::HOTKEY_GRID:
			click_widget("grid", null_str);
			break;

		case tmkwin_theme::HOTKEY_RCLICK:
			do_right_click();
			break;

		case tmkwin_theme::HOTKEY_SWITCH:
			if (!theme_) {
				break;
			}
			if (!current_unit_) {
				if (u && u->has_child()) {
					theme_into_widget(u);
				}
			} else {
				theme_goto_top(true);
			}
			break;

		case tmkwin_theme::HOTKEY_INSERT_TOP:
			insert_row(u, true);
			break;
		case tmkwin_theme::HOTKEY_INSERT_BOTTOM:
			insert_row(u, false);
			break;
		case tmkwin_theme::HOTKEY_ERASE_ROW:
			erase_row(u);
			break;

		case tmkwin_theme::HOTKEY_INSERT_LEFT:
			insert_column(u, true);
			break;
		case tmkwin_theme::HOTKEY_INSERT_RIGHT:
			insert_column(u, false);
			break;
		case tmkwin_theme::HOTKEY_ERASE_COLUMN:
			erase_column(u);
			break;

		case tmkwin_theme::HOTKEY_SETTING:
			widget_setting(false);
			break;
		case tmkwin_theme::HOTKEY_LINKED_GROUP:
			linked_group_setting();
			break;
		case tmkwin_theme::HOTKEY_SPECIAL_SETTING:
			widget_setting(true);
			break;

		case HOTKEY_COPY:
			copy_widget(u, false);
			break;
		case HOTKEY_CUT:
			copy_widget(u, true);
			break;
		case HOTKEY_PASTE:
			paste_widget();
			break;
		case tmkwin_theme::HOTKEY_ERASE:
			erase_widget();
			break;

		case tmkwin_theme::HOTKEY_INSERT_CHILD:
			u->insert_child(default_child_width, default_child_height);
			if (!in_theme_top()) {
				layout_dirty();
			}
			gui_->show_context_menu();
			break;
		case tmkwin_theme::HOTKEY_ERASE_CHILD:
			u->parent().u->erase_child(u->parent().number);
			if (!in_theme_top()) {
				layout_dirty();
			}
			gui_->show_context_menu();
			break;

		case tmkwin_theme::HOTKEY_UNPACK:
			unpack_widget_tpl(u);
			break;

		case tmkwin_theme::HOTKEY_BUILD:
			{
				gui2::tmkwin_theme* theme = dynamic_cast<gui2::tmkwin_theme*>(gui_->get_theme());
				theme->do_build();
			}
			break;

		case tmkwin_theme::HOTKEY_RUN:
			run();
			break;
		case HOTKEY_SYSTEM:
			system();
			break;

		default:
			controller_base::execute_command2(command, sparam);
	}
}

void mkwin_controller::insert_row(unit* u, bool top)
{
	const unit::tparent& parent = u->parent();
	unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	int row = u->get_location().y - child.window->get_location().y - 1;
	if (!top) {
		row ++;
	}
	int w = (int)child.cols.size();
	std::vector<unit*> v;
	for (int x = 0; x < w; x ++) {
		v.push_back(new unit(*this, *gui_, units_, gui_->spacer, parent.u, parent.number));
	}
	std::vector<unit*>::iterator it = child.units.begin();
	if (row) {
		std::advance(it, row * w);
	}
	child.units.insert(it, v.begin(), v.end());

	it = child.rows.begin();
	if (row) {
		std::advance(it, row);
	}
	child.rows.insert(it, new unit(*this, *gui_, units_, unit::ROW, parent.u, parent.number));

	layout_dirty();
	gui_->show_context_menu();
}

void mkwin_controller::erase_row(unit* u)
{
	const unit::tparent& parent = u->parent();
	unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	int row = u->get_location().y - child.window->get_location().y - 1;
	int w = (int)child.cols.size();
	std::vector<unit*>::iterator it;
	for (int x = 0; x < w; x ++) {
		unit* u = child.units[row * w];
		units_.erase(u->get_location());

		it = child.units.begin();
		std::advance(it, row * w);
		child.units.erase(it);
	}
	units_.erase(u->get_location());

	it = child.rows.begin();
	if (row) {
		std::advance(it, row);
	}
	child.rows.erase(it);

	layout_dirty();

	if (row >= (int)child.rows.size()) {
		selected_hex_.y = child.window->get_location().y + child.rows.size();
	}
	gui_->show_context_menu();
}

void mkwin_controller::insert_column(unit* u, bool left)
{
	const unit::tparent& parent = u->parent();
	unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	int col = u->get_location().x - child.window->get_location().x - 1;
	if (!left) {
		col ++;
	}
	int w = (int)child.cols.size();
	int h = (int)child.rows.size();
	
	std::vector<unit*>::iterator it;
	for (int y = 0; y < h; y ++) {
		unit* u = new unit(*this, *gui_, units_, gui_->spacer, parent.u, parent.number);

		it = child.units.begin();
		std::advance(it, y * w + col + y);
		child.units.insert(it, u);
	}
	
	it = child.cols.begin();
	if (col) {
		std::advance(it, col);
	}
	child.cols.insert(it, new unit(*this, *gui_, units_, unit::COLUMN, parent.u, parent.number));

	layout_dirty();
	gui_->show_context_menu();
}

void mkwin_controller::erase_column(unit* u)
{
	const unit::tparent& parent = u->parent();
	unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	int col = u->get_location().x - child.window->get_location().x - 1;
	int w = (int)child.cols.size();
	int h = (int)child.rows.size();
	std::vector<unit*>::iterator it;
	for (int y = 0; y < h; y ++) {
		unit* u = child.units[y * w + col - y];
		units_.erase(u->get_location());

		it = child.units.begin();
		std::advance(it, y * w + col - y);
		child.units.erase(it);
	}
	units_.erase(u->get_location());

	it = child.cols.begin();
	if (col) {
		std::advance(it, col);
	}
	child.cols.erase(it);

	layout_dirty();
	if (col >= (int)child.cols.size()) {
		selected_hex_.x = child.window->get_location().x + child.cols.size();
	}
	gui_->show_context_menu();
}

void mkwin_controller::fill_object_list()
{
	gui2::tmkwin_theme* theme = dynamic_cast<gui2::tmkwin_theme*>(gui_->get_theme());
	theme->fill_object_list(units_);
}

void mkwin_controller::theme_into_widget(unit* u)
{
	gui_->set_grid(false);

	VALIDATE(top_.units.empty(), "units in top_ must be empty!");
	theme_top_ = top_;
	theme_top_.units = units_.form_units();

	current_unit_ = u;
	top_.clear(false);

	units_.set_consistent(true);
	layout_dirty(true);

	gui2::tcontrol* widget = dynamic_cast<gui2::tcontrol*>(gui_->get_theme_object("switch"));
	widget->set_label("misc/return.png");
	widget->set_tooltip(_("Return"));

	selected_hex_ = map_location();

	fill_object_list();

	gui_->show_context_menu();
}

void mkwin_controller::theme_goto_top(bool auto_select)
{
	unit* u = units_.find_unit(map_location(0, 0))->parent().u;
	if (auto_select) {
		selected_hex_ = u->get_location();
	} else {
		selected_hex_ = map_location();
	}

	gui_->set_grid(true);
	top_ = theme_top_;

	current_unit_ = NULL;
	units_.zero_map();

	units_.set_consistent(false);
	reload_map(top_.cols.size() + 1, top_.rows.size() + 1, true);
	units_.restore_map_from(top_, 0, 0, true);

	// normal, theme not use top_.units
	top_.units.clear();

	gui2::tcontrol* widget = dynamic_cast<gui2::tcontrol*>(gui_->get_theme_object("switch"));
	widget->set_label("misc/into.png");
	widget->set_tooltip(_("Go into"));

	if (selected_hex_.valid()) {
		gui_->scroll_to_xy(u->get_rect().x, u->get_rect().y, display::ONSCREEN);
	}

	fill_object_list();

	gui_->show_context_menu();
}

void mkwin_controller::widget_setting(bool special)
{
	if (!selected_hex_.valid()) {
		return;
	}

	unit* u = units_.find_unit(selected_hex_);
	const config original_rect_cfg = u->cell().rect_cfg;

	gui2::tsetting_dialog *dlg = NULL;
	if (special) {
		if (u->is_drawing()) {
			dlg = new gui2::tdrawing_setting(*gui_, *u);
		}

	} else if (u->type() == unit::WIDGET) {
		dlg = new gui2::tcontrol_setting(*gui_, *this, *u, textdomains_, linked_groups_);

	} else if (u->type() == unit::ROW) {
		dlg = new gui2::trow_setting(*gui_, *u);

	} else if (u->type() == unit::COLUMN) {
		dlg = new gui2::tcolumn_setting(*gui_, *u);

	} else if (u->type() == unit::WINDOW) {
		if (!u->parent().u) {
			dlg = new gui2::twindow_setting(*gui_, *this, *u, textdomains_);
		} else {
			const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget = u->parent().u->widget();
			dlg = new gui2::tgrid_setting(*this, *gui_, units_, *u, widget.first);
		}
	}

	dlg->show(gui_->video());
	int res = dlg->get_retval();
	if (res != gui2::twindow::OK) {
		delete dlg;
		return;
	}
	u->set_cell(dlg->cell());
	delete dlg;

	if (original_rect_cfg != u->cell().rect_cfg) {
		rect_changed(u);
	}
}

void mkwin_controller::rect_changed(unit* u)
{
	if (!in_theme_top()) {
		// second layer must not effect top layer's rect.
		return;
	}

	std::vector<unit*> top_units = form_top_units();
	config theme_cfg = generate_theme_cfg(top_units);
/*
	int screen_w = theme::XDim * gui2::twidget::hdpi_scale;
	int screen_h = theme::YDim * gui2::twidget::hdpi_scale;
*/
	int screen_w = theme::XDim;
	int screen_h = theme::YDim;

	std::string patch = gui_->get_theme_patch();
	config theme_cfg2;
	const config* theme_current_cfg = theme::set_resolution(theme_cfg.child("theme"), screen_w, screen_h, patch, theme_cfg2);

	std::vector<const unit*> mistaken_ids;
	double factor = gui_->get_zoom_factor();
	int zoom = gui_->zoom();

	// set_rect maybe result sort
	int index = 0;

	// Only check top layer.
	BOOST_FOREACH (const config::any_child& v, theme_current_cfg->all_children_range()) {
		if (is_theme_reserved(v.key)) {
			continue;
		}

		unit* u = top_units[index ++];
		SDL_Rect rect = theme::calculate_relative_loc(v.cfg, screen_w, screen_h);
		if (!rect.w || !rect.h) {
			mistaken_ids.push_back(u);
			break;
		} else {
			rect = create_rect(rect.x * factor, rect.y * factor, rect.w * factor, rect.h * factor);
			rect.x += zoom;
			rect.y += zoom;
			u->set_rect(rect);
		}
	}
	if (!mistaken_ids.empty()) {
		const unit* u = mistaken_ids[0];

		utils::string_map symbols;
		const std::string id = u->cell().id;
		symbols["id"] = tintegrate::generate_format(id.empty()? "--": id, "red");
		symbols["widget"] = tintegrate::generate_format(u->widget_tag(), "red");

		const std::string err = vgettext2("widget($widget) has rect, must set id!", symbols);
		gui2::show_message(gui_->video(), "", err);
	}

	fill_object_list();
}

void mkwin_controller::linked_group_setting()
{
	gui2::tlinked_group2 dlg(*gui_, linked_groups_);
	dlg.show(gui_->video());
	int res = dlg.get_retval();
	if (res != gui2::twindow::OK) {
		return;
	}
	linked_groups_ = dlg.linked_groups();
}

void mkwin_controller::copy_widget(unit* u, bool cut)
{
	cut_ = cut;
	set_copied_unit(u);

	gui_->show_context_menu();
}

bool mkwin_controller::paste_widget2(const unit& from, const map_location to_loc)
{
	bool require_layout_dirty = false;

	unit* to = units_.find_unit(to_loc);
	unit::tparent parent = to->parent();
	units_.erase(to_loc);
	unit* clone = new unit(from);
	// from and to maybe in different page.
	clone->set_parent(parent.u, parent.number);	
	units_.insert(to_loc, clone);

	to = units_.find_unit(to_loc);
	replace_child_unit(to);
	if (to->has_child()) {
		require_layout_dirty = true;
	}
	return require_layout_dirty;
}

void mkwin_controller::paste_widget()
{
	if (!selected_hex_.valid() || !copied_unit_) {
		return;
	}

	unit* u = units_.find_unit(selected_hex_);
	bool require_layout_dirty = false;
	std::vector<const unit*> require_cut_widgets;

	if (copied_unit_->type() == unit::WIDGET) {
		require_layout_dirty = paste_widget2(*copied_unit_, selected_hex_);
		require_cut_widgets.push_back(copied_unit_);

	} else if (copied_unit_->type() == unit::ROW) {
		unit::tparent from_parent = copied_unit_->parent();
		unit::tparent to_parent = u->parent();

		const unit::tchild& from_child = from_parent.u? from_parent.u->child(from_parent.number): top_;
		const unit::tchild& to_child = to_parent.u? to_parent.u->child(to_parent.number): top_;

		int from_row = copied_unit_->get_location().y - from_child.window->get_location().y - 1;
		int to_row = u->get_location().y - to_child.window->get_location().y - 1;
		int w = (int)from_child.cols.size();
		for (int x = 0; x < w; x ++) {
			const unit* from = from_child.units[from_row * w + x];
			const unit* to = to_child.units[to_row * w + x];
			require_layout_dirty |= paste_widget2(*from, to->get_location());
			require_cut_widgets.push_back(from);
		}

	} else if (copied_unit_->type() == unit::COLUMN) {
		unit::tparent from_parent = copied_unit_->parent();
		unit::tparent to_parent = u->parent();

		const unit::tchild& from_child = from_parent.u? from_parent.u->child(from_parent.number): top_;
		const unit::tchild& to_child = to_parent.u? to_parent.u->child(to_parent.number): top_;

		int from_col = copied_unit_->get_location().x - from_child.window->get_location().x - 1;
		int to_col = u->get_location().x - to_child.window->get_location().x - 1;
		int from_w = (int)from_child.cols.size();
		int to_w = (int)to_child.cols.size();
		int h = (int)from_child.rows.size();
		for (int y = 0; y < h; y ++) {
			const unit* from = from_child.units[y * from_w + from_col];
			const unit* to = to_child.units[y * to_w + to_col];
			require_layout_dirty |= paste_widget2(*from, to->get_location());
			require_cut_widgets.push_back(from);
		}

	}
	if (cut_) {
		for (std::vector<const unit*>::const_iterator it = require_cut_widgets.begin(); it != require_cut_widgets.end(); ++ it) {
			const unit* erasing = *it;
			const map_location loc = erasing->get_location();
			unit::tparent parent = erasing->parent();
			units_.erase(loc);
			VALIDATE(!copied_unit_ || copied_unit_->type() != unit::WIDGET, null_str);

			units_.insert(loc, new unit(*this, *gui_, units_, gui_->spacer, parent.u, parent.number));
			unit* u = units_.find_unit(loc);
			replace_child_unit(u);
		}
	}
	if (require_layout_dirty) {
		layout_dirty();
	}

	gui_->show_context_menu();
}

void mkwin_controller::erase_widget()
{
	if (!selected_hex_.valid()) {
		return;
	}

	unit* erasing = units_.find_unit(selected_hex_);
	unit::tparent parent = erasing->parent();

	if (erasing->type() == unit::WINDOW && parent.u && parent.u->is_grid()) {
		selected_hex_ = parent.u->get_location();
		erase_widget();
		return;
	}

	bool has_child = erasing->has_child();
	if (has_child) {
		bool all_spacer = true;
		const std::vector<unit::tchild>& children = erasing->children();
		for (std::vector<unit::tchild>::const_iterator it = children.begin(); it != children.end(); ++ it) {
			if (!it->is_all_spacer()) {
				all_spacer = false;
				break;
			}
		}
		if (!all_spacer) {
			utils::string_map symbols;
			std::stringstream ss;
			
			symbols["id"] = tintegrate::generate_format(erasing->cell().id.empty()? "---": erasing->cell().id, "red");
			ss << vgettext2("$id holds grid that not all spacer. Do you want to delete?", symbols);
			int res = gui2::show_message(gui_->video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
			if (res != gui2::twindow::OK) {
				return;
			}
		}
	}
	units_.erase(selected_hex_);
	if (in_theme_top()) {
		selected_hex_ = map_location();

	} else {
		units_.insert(selected_hex_, new unit(*this, *gui_, units_, gui_->spacer, parent.u, parent.number));
		unit* u = units_.find_unit(selected_hex_);
		replace_child_unit(u);

		if (has_child) {
			layout_dirty();
		}
	}

	fill_object_list();
	gui_->show_context_menu();
}

void mkwin_controller::layout_dirty(bool force_change_map)
{
	const int default_expand_size = 4;
	
	VALIDATE(!in_theme_top(), "Must on be in !theme or theme's second layer!");

	bool require_change = false;
	int w = map_.w(), h = map_.h();
	int new_w, new_h;
	if (theme_) {
		gui2::tpoint ret = units_.restore_map_from(current_unit_->children(), false);
		new_w = ret.x;
		new_h = ret.y;
	} else {
		units_.recalculate_size(top_);
		new_w = top_.window->cell().window.cover_width;
		new_h = top_.window->cell().window.cover_height;
	}
	if (force_change_map || w < new_w) {
		require_change = true;
		w = new_w + default_expand_size;
	} else if (w > default_dialog_width && w > new_w + default_expand_size) {
		require_change = true;
		w = std::max(new_w + default_expand_size, default_dialog_width);
	}

	if (force_change_map || h < new_h) {
		require_change = true;
		h = new_h + default_expand_size;
	} else if (h > default_dialog_height && h > new_h + default_expand_size) {
		require_change = true;
		h = std::max(new_h + default_expand_size, default_dialog_height);
	}

	if (require_change) {
		reload_map(w, h, in_theme_top());
	}

	units_.layout(top_);
	if (require_change) {
		gui_->recalculate_minimap();
	} else {
		gui_->redraw_minimap();
	}
}

void mkwin_controller::set_status()
{
	gui2::tcontrol* widget = dynamic_cast<gui2::tcontrol*>(gui_->get_theme_object("status"));
	int w = widget->get_width();
	int h = widget->get_height();
	surface result = make_neutral_surface(image::get_image("misc/background45.png"));
	result = scale_surface(result, w, h);
	if (copied_unit_) {
		SDL_Rect dst_clip = empty_rect;
		std::string png = cut_? "buttons/cut.png": "buttons/copy.png";
		surface fg = scale_surface(image::get_image(png), w / 2, h / 2);
		sdl_blit(fg, NULL, result, &dst_clip);

		const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget = copied_unit_->widget();
		png = copied_unit_->image();
		fg = scale_surface(image::get_image(png), w / 2, h / 2);
		if (fg) {
			dst_clip.x = w / 2;
			dst_clip.y = 0;
			sdl_blit(fg, NULL, result, &dst_clip);
		}
	}
	gui_->widget_set_surface("status", result);
	gui_->redraw_minimap();
}

void mkwin_controller::set_copied_unit(unit* u)
{
	copied_unit_ = u;
	if (gui_) {
		set_status();
	}
}

void mkwin_controller::fill_spacer(unit* parent, int number)
{
	int width = map_.w();
	int height = map_.h();

	int zoom = gui_->hex_width();
	for (int y = 0; y < height; y ++) {
		for (int x = 0; x < width; x ++) {
			map_location loc = map_location(x, y);
			if (!units_.find_base_unit(loc, true)) {
				if (x && y) {
					if (!in_theme_top()) {
						units_.insert(loc, new unit(*this, *gui_, units_, gui_->spacer, parent, number));
					}
				} else {
					int type = unit::NONE;
					if (!x && !y) {
						type = unit::WINDOW;
					} else if (x) {
						type = unit::COLUMN;
					} else if (y) {
						type = unit::ROW;
					}
					if (!in_theme_top()) {
						units_.insert(loc, new unit(*this, *gui_, units_, type, parent, number));
					} else {
						SDL_Rect rect = create_rect(x * zoom, y * zoom, zoom, zoom);
						units_.insert2(*gui_, new unit2(*this, *gui_, units_, type, parent, rect));
					}
				}
			}
		}
	}
}

class tclick_dismiss
{
public:
	tclick_dismiss(gui2::tcell_setting& cell)
		: cell_(cell)
	{
		original_ = cell.window.click_dismiss;
		cell_.window.click_dismiss = true;
	}
	~tclick_dismiss()
	{
		cell_.window.click_dismiss = original_;
	}

private:
	gui2::tcell_setting& cell_;
	bool original_;
};

void mkwin_controller::run()
{
	if (!window_has_valid(true)) {
		return;
	}

	std::string msg = _("Check successfully!");
	gui2::show_message(gui_->video(), "", msg);
}

unit* mkwin_controller::get_window() const
{
	unit* window = NULL;
	if (current_unit_) {
		window = theme_top_.window;
	} else {
		window = units_.find_unit(map_location(0, 0));
	}
	return window;
}

bool mkwin_controller::callback_valid_id(const unit* u, bool show_error)
{
	std::string err;
	std::stringstream ss;
	utils::string_map symbols;

	const std::string id = u->cell().id;
	symbols["id"] = tintegrate::generate_format(id.empty()? "--": id, "red");
	symbols["widget"] = tintegrate::generate_format(u->widget_tag(), "red");

	bool is_top_unit = !u->parent().u;
	if (theme_) {
		if (is_top_unit) {
			if (!id.empty()) {
				if (callback_sstr_.find(id) == callback_sstr_.end()) {
					callback_sstr_.insert(id);
				} else {
					err = vgettext2("On theme top, duplicated '$id' value!", symbols);
				}
			}
			if (!u->fix_rect()) {
				err = vgettext2("widget($widget) is on theme top, must set rect!", symbols);
			}
		}
		if (u->fix_rect() && id.empty()) {
			err = vgettext2("widget($widget) has rect, must set id!", symbols);
		}
/*
		if (u->fix_rect() && u->cell().rect_cfg["ref"].empty()) {
			err = vgettext2("widget($widget) has rect, must set ref!", symbols);
		}
*/
		if (!err.empty()) {
			if (show_error) {
				if (is_top_unit) {
					if (!in_theme_top()) {
						err = _("Widget on top layer has error. Be back top to get error detail.");
					}
				} else {
					const unit* parent = u->parent_at_top();
					if (in_theme_top()) {
						const unit* parent = u->parent_at_top();
						symbols["widget"] = tintegrate::generate_format(parent->widget_tag(), "red");
						err = vgettext2("Widget on second layer has error. Go into $widget to get error detail.", symbols);
					} else if (parent != current_unit_) {
						err = _("Another widget on top layer has error. Be back top to get error detail.");
					}
				}
				gui2::show_message(gui_->video(), "", err);
			}
			return false;
		}
	}

	if (u->is_stacked_widget()) {
		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it = children.begin(); it != children.end(); ++ it) {
			const unit::tchild& child = *it;
			if (child.is_all_spacer()) {
				err = vgettext2("widget($widget) has all spacer layer!", symbols);
				if (show_error) {
					gui2::show_message(gui_->video(), "", err);
				}
				return false;
			}
		}
	}
	return true;
}

bool mkwin_controller::enumerate_child2(const unit::tchild& child, bool show_error) const
{
	if (!fcallback_(child.window, show_error)) {
		return false;
	}

	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		const unit* u = *it;
		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			if (!enumerate_child2(*it2, show_error)) {
				return false;
			}
		}
		if (!fcallback_(u, show_error)) {
			return false;
		}
	}
	return true;
}

bool mkwin_controller::enumerate_child(const std::vector<unit*>& top_units, bool show_error) const
{
	for (std::vector<unit*>::const_iterator it = top_units.begin(); it != top_units.end(); ++ it) {
		const unit* u = *it;
		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			if (!enumerate_child2(*it2, show_error)) {
				return false;
			}
		}
		if (!fcallback_(u, show_error)) {
			return false;
		}
	}
	return true;
}

bool mkwin_controller::window_has_valid(bool show_error)
{
	std::stringstream err;
	err << _("Window isn't ready!") << "\n\n";

	const unit* window = get_window();
	if (!window) {
		return false;
	}

	if (window->cell().id.empty() || window->cell().id == gui2::untitled) {
		if (show_error) {
			gui2::show_id_error(*gui_, "id", err.str());
		}
		return false;
	}

	if (window->cell().window.description.empty()) {
		if (show_error) {
			gui2::show_id_error(*gui_, "description", err.str());
		}
		return false;
	}
	if (window->cell().window.textdomain.empty()) {
		if (show_error) {
			gui2::show_id_error(*gui_, "textdomain", err.str());
		}
		return false;
	}

	std::vector<unit*> top_units = form_top_units();
	{
		tenumerate_lock lock(*this, boost::bind(&mkwin_controller::callback_valid_id, this, _1, _2));
		if (!enumerate_child(top_units, show_error)) {
			return false;
		}
	}
	return true;
}

void mkwin_controller::system()
{
	enum {_LOAD, _SAVE, _SAVE_AS, _PREFERENCES, _QUIT};

	int retval;
	std::vector<gui2::tsystem::titem> items;
	std::vector<int> rets;

	const config top = theme_? generate_theme_cfg(form_top_units()).child("theme"): generate_window_cfg().child("window");
	const unit* window = get_window();
	bool window_dirty = original_.first != window->cell().window.textdomain || original_.second != top;

	{
		std::string str = _("Save");

		items.push_back(gui2::tsystem::titem(_("Load")));
		rets.push_back(_LOAD);

		items.push_back(gui2::tsystem::titem(str, window_has_valid(false) && window_dirty && !file_.empty()));
		rets.push_back(_SAVE);

		items.push_back(gui2::tsystem::titem(_("Save As"), window_has_valid(false)));
		rets.push_back(_SAVE_AS);

		items.push_back(gui2::tsystem::titem(_("Preferences")));
		rets.push_back(_PREFERENCES);

		items.push_back(gui2::tsystem::titem(_("Quit")));
		rets.push_back(_QUIT);

		gui2::tsystem dlg(items);
		try {
			dlg.show(gui_->video());
			retval = dlg.get_retval();
		} catch(twml_exception& e) {
			e.show(*gui_);
			return;
		}
		if (retval == gui2::twindow::OK) {
			return;
		}
	}
	bool require_show_context_menu = false;
	if (rets[retval] == _LOAD) {
		load_window();	

	} else if (rets[retval] == _SAVE) {
		save_window(false);
		require_show_context_menu = true;

	} else if (rets[retval] == _SAVE_AS) {
		save_window(true);
		require_show_context_menu = true;

	} else if (rets[retval] == _PREFERENCES) {
		preferences::show_preferences_dialog(*gui_, true);
		gui_->redraw_everything();

	} else if (rets[retval] == _QUIT) {
		quit_confirm(EXIT_NORMAL, window_dirty);
	}

	if (require_show_context_menu) {
		gui_->show_context_menu();
	}
}

#include "loadscreen.hpp"

config window_cfg_from_gui_bin()
{
	config cfg;
	wml_config_from_file(game_config::path + "/xwml/" + "gui.bin", cfg);
	const config& gui_cfg = cfg.find_child("gui", "id", "default");
	return gui_cfg.find_child("window", "id", "chat2");
	// return gui_cfg.find_child("window", "id", "recruit");
}

const config& mkwin_controller::find_rect_cfg(const std::string& widget, const std::string& id) const
{
	return find_rect_cfg2(*top_rect_cfg_, widget, id);
}

const config& mkwin_controller::find_rect_cfg2(const config& cfg, const std::string& widget, const std::string& id) const
{
	config::all_children_itors itors = cfg.all_children_range();
	for (config::all_children_iterator i = itors.first; i != itors.second; ++ i) {
		const config& icfg = i->cfg;
		if (i->cfg["id"] == id && i->key == widget) {
			return icfg;
		}

		// Recursively look in children.
		const config& c = find_rect_cfg2(icfg, widget, id);
		if (!c.empty()) {
			return c;
		}
	}

	return null_cfg;
}

std::vector<std::string> mkwin_controller::generate_textdomains(const std::string& file, bool theme) const
{
	std::string dir = directory_name(file);
	static const std::string dir_separators = "\\/";

	std::string::size_type pos = dir.find_last_of(dir_separators);
	if (pos == dir.size() - 1) {
		dir = dir.substr(0, dir.size() - 1);
	}

	std::string name = file_name(dir);
	if (theme) {
		if (name != "theme") {
			return textdomains_;
		}
	} else if (name != "window") {
		return textdomains_;
	}

	dir = directory_name(dir);
	if (dir.size() < 2) { // 0, 1
		return textdomains_;
	}
	dir = dir.substr(0, dir.size() - 1);
	name = file_name(dir);
	const std::string app = game_config::extract_app_from_app_dir(name);

	std::vector<std::string> res;
	res.push_back("rose-lib");
	if (!app.empty()) {
		res.push_back(app + "-lib");
	}

	return res;
}

void mkwin_controller::load_window()
{
	bool browse_cfg = true;
	std::stringstream err;

	game_config::config_cache& cache = game_config::config_cache::instance();
	config window_cfg;
	if (browse_cfg) {
		std::string initial = game_config::preferences_dir_utf8 + "/editor/maps";
		gui2::tbrowse::tparam param(gui2::tbrowse::TYPE_FILE, true, initial, _("Choose a Window to Open"));
		if (theme_) {
			param.extra = gui2::tbrowse::tentry(game_config::path + "/data/gui/default/theme", _("gui/theme"), "misc/dir-res.png");
		} else {
			param.extra = gui2::tbrowse::tentry(game_config::path + "/data/gui/default/window", _("gui/window"), "misc/dir-res.png");
		}
		gui2::tbrowse dlg(*gui_, param);
		dlg.show(gui_->video());
		int res = dlg.get_retval();
		if (res != gui2::twindow::OK) {
			return;
		}
		std::string file = param.result;

		config cfg;

		{
			game_config::config_cache_transaction main_transaction;
			cache.get_config(widget_template_cfg(), cfg);
			cfg.clear();

			topen_unicode_lock lock(true);
			cache.get_config(file, cfg);
		}

		if (theme_) {
			const config& window_cfg1 = cfg.child("theme");
			if (!window_cfg1) {
				err << _("Invalid theme cfg file!");
				gui2::show_message(gui_->video(), "", err.str());
				return;
			}
			window_cfg = window_cfg1;

		} else {
			const config& window_cfg1 = cfg.child("window");
			if (!window_cfg1) {
				err << _("Invalid dialog cfg file!");
				gui2::show_message(gui_->video(), "", err.str());
				return;
			}
			window_cfg = window_cfg1;
		}
		
		file_ = file;

		refresh_title();

		textdomains_ = generate_textdomains(file_, theme_);

	} else {
		window_cfg = window_cfg_from_gui_bin();
	}

	const config& res_cfg = window_cfg.child("resolution");
	if (!res_cfg) {
		return;
	}

	tused_widget_tpl_lock tpl_lock(*this);
	if (theme_) {
		if (current_unit_) {
			theme_goto_top(false);
		}
		// don't erase window, cols, rows, they are shared.
		for (std::vector<unit*>::iterator it = top_.units.begin(); it != top_.units.end(); ) {
			unit* u = *it;
			units_.erase(u->get_location());
			it = top_.units.erase(it);
		}

		reset_modes();
/*
		int screen_w = theme::XDim * gui2::twidget::hdpi_scale;
		int screen_h = theme::YDim * gui2::twidget::hdpi_scale;
*/
		int screen_w = theme::XDim;
		int screen_h = theme::YDim;

		std::string patch = gui_->get_theme_patch();
		config theme_cfg2;
		const config* theme_current_cfg = theme::set_resolution(window_cfg, screen_w, screen_h, patch, theme_cfg2);

		reload_map(top_.cols.size() + 1, top_.rows.size() + 1, true);
		units_.restore_map_from(top_, 0, 0, true);

		{
			tfind_rect_cfg_lock lock(*this, res_cfg);
			top_.from(*this, *gui_, units_, NULL, -1, *theme_current_cfg);
		
			top_.window->from(window_cfg);
			theme_load_patches(top_, window_cfg);

			// in theme, don't use top_.units
			top_.units.clear();
		}
		form_linked_groups(res_cfg);
		form_context_menus(res_cfg);

		gui_->recalculate_minimap();

	} else {
		const config& top_grid = res_cfg.child("grid");
		if (!top_grid) {
			return;
		}

		top_.erase(units_);
		
		top_.from(*this, *gui_, units_, NULL, -1, top_grid);
		top_.window->from(window_cfg);
		form_linked_groups(res_cfg);
		form_context_menus(res_cfg);

		layout_dirty();
	}

	if (!used_widget_tpl_.empty()) {
		// erase [linked_group] belong to widget template.
		std::set<std::string> erase_ids;
		{
			game_config::config_cache_transaction main_transaction;

			config cfg;
			cache.get_config(widget_template_cfg(), cfg);

			preproc_map& map = cache.make_copy_map();

			for (std::set<const config*>::const_iterator it = used_widget_tpl_.begin(); it != used_widget_tpl_.end(); ++ it ) {
				const config& tpl_cfg = **it;
				preproc_map::const_iterator it2 = map.find(tpl_cfg["linked_group"]);
				if (it2 != map.end()) {
					const preproc_define& define = it2->second;
					::read(cfg, define.value);
					BOOST_FOREACH (const config& linked, cfg.child_range("linked_group")) {
						erase_ids.insert(linked["id"]);
					}
				}
			}
		}
		
		for (std::vector<gui2::tlinked_group>::iterator it = linked_groups_.begin(); it != linked_groups_.end(); ) {
			const std::string& id = it->id;
			if (erase_ids.find(id) != erase_ids.end()) {
				it = linked_groups_.erase(it);
			} else {
				++ it;
			}
		}
	}

	fill_object_list();
	selected_hex_ = map_location();

	original_.first = top_.window->cell().window.textdomain;
	// window_cfg is format from *.cfg directly that parsed macro.
	// here requrie format that is tpl_widget.
	if (theme_) {
		original_.second = generate_theme_cfg(form_top_units()).child("theme");
	} else {
		original_.second = generate_window_cfg().child("window");
	}
}

void mkwin_controller::save_window(bool as)
{
	VALIDATE(as || !file_.empty(), "When save, file_ must not be empty!");

	const unit* u = get_window();
	std::string fname;
	if (!as) {
		fname = file_;
	} else {
		fname = game_config::preferences_dir_utf8 + "/editor/maps/";

		gui2::tbrowse::tparam param(gui2::tbrowse::TYPE_FILE, false, fname, _("Choose a Window to Save"), _("Save"));
		if (theme_) {
			param.extra = gui2::tbrowse::tentry(game_config::path + "/data/themes", _("data/themes"), "misc/dir-res.png");
		} else {
			param.extra = gui2::tbrowse::tentry(game_config::path + "/data/gui/default/window", _("gui/window"), "misc/dir-res.png");
		}

		gui2::tbrowse dlg(*gui_, param);
		dlg.show(gui_->video());
		int res = dlg.get_retval();
		if (res != gui2::twindow::OK) {
			return;
		}
		fname = param.result;
	}

	std::stringstream ss;

	ss << "#textdomain " << u->cell().window.textdomain << "\n";
	ss << "\n";

	tused_widget_tpl_lock tpl_lock(*this);
	config top;
	if (!theme_) {
		top = generate_window_cfg();
		if (!as) {
			original_.second = top.child("window");
		}

		if (!used_widget_tpl_.empty()) {
			config& res_cfg = top.child("window").child("resolution");
			std::string key = noise_config_key("linked_group");
			for (std::set<const config*>::const_iterator it = used_widget_tpl_.begin(); it != used_widget_tpl_.end(); ++ it ) {
				const config& tpl_cfg = **it;
				res_cfg[key] = tpl_cfg["linked_group"].str();
			}
		}
		
	} else {
		top = generate_theme_cfg(form_top_units());
		if (!as) {
			original_.second = top.child("theme");
		}
	}
	::write(ss, top, 0, u->cell().window.textdomain);
	std::string data = ss.str();
	if (!used_widget_tpl_.empty()) {
		for (std::set<const config*>::const_iterator it = used_widget_tpl_.begin(); it != used_widget_tpl_.end(); ++ it ) {
			const config& tpl_cfg = **it;
			std::stringstream src, dst;

			std::string key = noise_config_key("widget");
			src << key << "=\"" << tpl_cfg["widget"].str() << "\"";
			dst << "{" << tpl_cfg["widget"].str() << "}";

			size_t pos = 0;
			while ((pos = data.find(src.str(), pos)) != std::string::npos) {
				data.replace(pos, src.str().size(), dst.str());
				pos += dst.str().size();
			}

			src.str("");
			key = noise_config_key("linked_group");
			src << key << "=\"" << tpl_cfg["linked_group"].str() << "\"";
			dst.str("");
			dst << "{" << tpl_cfg["linked_group"].str() << "}";
			pos = 0;
			while ((pos = data.find(src.str(), pos)) != std::string::npos) {
				data.replace(pos, src.str().size(), dst.str());
				pos += dst.str().size();
			}
		}
	}
	write_file(fname, data.c_str(), data.size(), true);

	if (!as) {
		original_.first = u->cell().window.textdomain;
	}
}

void mkwin_controller::quit_confirm(EXIT_STATUS mode, bool dirty)
{
	std::string message = _("Do you really want to quit?");
	if (dirty) {
		utils::string_map symbols;
		symbols["window"] = theme_? _("Theme"): _("Dialog");
		std::string str = vgettext2("Changes in the $window since the last save will be lost.", symbols);
		message += "\n\n" + tintegrate::generate_format(str, "red");
	}
	const int res = gui2::show_message(gui().video(), _("Quit"), message, gui2::tmessage::yes_no_buttons);
	if (res != gui2::twindow::CANCEL) {
		do_quit_ = true;
		quit_mode_ = mode;
	}
}

void mkwin_controller::theme_load_patches(const unit::tchild& top, const config& theme_cfg)
{
	BOOST_FOREACH (const config::any_child& c, theme_cfg.all_children_range()) {
		if (c.key == "resolution") {
			continue;
		}
		if (c.key == "partialresolution") {
			int width = c.cfg["width"].to_int();
			int height = c.cfg["height"].to_int();
			tmode patch(tmode::def_id, width, height);
			load_patch_res(top, patch, c.cfg);

		} else {
			insert_mode(c.key);
			BOOST_FOREACH (const config::any_child& c2, c.cfg.all_children_range()) {
				std::vector<std::string> vstr = utils::split(c2.key, 'x');
				VALIDATE(vstr.size() == 2, "Name of patch resolution is error!");

				int width = lexical_cast<int>(vstr[0]);
				int height = lexical_cast<int>(vstr[1]);
				tmode patch(c.key, width, height);
				load_patch_res(top, patch, c2.cfg);
			}
		}
	}
}

void mkwin_controller::load_patch_res(const unit::tchild& top, const tmode& patch, const config& cfg)
{
	BOOST_FOREACH (const config::any_child& c, cfg.all_children_range()) {
		if (c.key == "change") {
			const std::string& id = c.cfg["id"].str();
			unit* u = top.find_unit(id);
			if (u) {
				config cfg2 = c.cfg;
				cfg2.remove_attribute("id");

				u->insert_adjust(tadjust(tadjust::CHANGE, patch, cfg2));
			}

		} else if (c.key == "remove") {
			std::string id = c.cfg["id"].str();
			bool remove_unit = is_noised_config_key(id);
			if (remove_unit) {
				id = denoise_config_key(id);
			}
			if (id.empty()) {
				continue;
			}
			if (remove_unit) {
				unit* u = top.find_unit(id);
				if (u) {
					u->insert_adjust(tadjust(tadjust::REMOVE, patch, null_cfg));
				}
			} else {
				std::pair<unit*, int> ret = top.find_layer(id);
				if (ret.first) {
					unit* u = ret.first;
					config adjust_cfg;

					adjust_cfg[str_cast(ret.second)] = true;
					u->insert_adjust(tadjust(tadjust::REMOVE2, patch, adjust_cfg));
				}
			}

		}
	}
}

void mkwin_controller::mouse_motion(int x, int y, const bool browse)
{
	mouse_handler_base::mouse_motion(x, y, browse);

	if (mouse_handler_base::mouse_motion_default(x, y)) return;
	map_location hex_clicked = gui_->hex_clicked_on(x, y);
	last_hex_ = hex_clicked;
	last_unit_ = units_.unit_clicked_on(x, y, last_hex_);

	gui_->highlight_hex(hex_clicked);

	if (cursor::get() != cursor::WAIT) {
		if (current_unit_ && !last_hex_.x && !last_hex_.y) {
			// cursor::set(cursor::ILLEGAL);
			cursor::set(cursor::NORMAL);
			gui_->set_mouseover_hex_overlay(NULL);

		} else if (!last_hex_.x || !last_hex_.y || !map_.on_board(last_hex_)) {
			// cursor::set(cursor::INTERIOR);
			cursor::set(cursor::NORMAL);
			gui_->set_mouseover_hex_overlay(NULL);

		} else {
			// no selected unit or we can't move it
			cursor::set(cursor::NORMAL);
			gui_->resume_mouseover_hex_overlay();
		}
	}
}

void mkwin_controller::select_unit(unit* u)
{
	gui_->scroll_to_tile(*u->get_draw_locations().begin(), display::ONSCREEN, true, true);

	selected_hex_ = u->get_location();
	gui_->show_context_menu();
}

void mkwin_controller::unpack_widget_tpl(unit* u)
{
	utils::string_map symbols;
	std::stringstream ss;
	symbols["id"] = tintegrate::generate_format(u->cell().id, "red");
	ss << vgettext2("Once unpake, can not be back to template. Do you want to unpake $id?", symbols);
	int res = gui2::show_message(gui_->video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
	if (res != gui2::twindow::OK) {
		return;
	}

	unit::tparent parent = u->parent();
	gui2::tgrid::tchild cell = u->cell().widget.cell;
	
	std::string tpl_id = unit::extract_from_tpl_widget_id(u->cell().id);
	const config& tpl_cfg = game_config_.find_child("tpl_widget", "id", tpl_id);
	
	config widget_cfg, linked_group_cfg;
	{
		game_config::config_cache& cache = game_config::config_cache::instance();
		game_config::config_cache_transaction main_transaction;

		config cfg;
		cache.get_config(widget_template_cfg(), cfg);

		preproc_map& map = cache.make_copy_map();
		
		preproc_map::const_iterator it = map.find(tpl_cfg["widget"]);
		if (it != map.end()) {
			const preproc_define& define = it->second;
			::read(widget_cfg, define.value);
		}

		it = map.find(tpl_cfg["linked_group"]);
		if (it != map.end()) {
			const preproc_define& define = it->second;
			::read(linked_group_cfg, define.value);
		}
	}

	units_.erase(selected_hex_);

	BOOST_FOREACH (const config& linked, linked_group_cfg.child_range("linked_group")) {
		linked_groups_.push_back(gui2::tlinked_group(linked));
	}

	BOOST_FOREACH (const config::any_child& c, widget_cfg.all_children_range()) {
		const std::string& type = c.key;
		const std::string& definition = c.cfg["definition"].str();
		gui2::tcontrol_definition_ptr ptr;
		if (type != "grid") {
			ptr = mkwin_display::find_widget(*gui_, type, definition, c.cfg["id"].str());
		}
		u = new unit(*this, *gui_, units_, std::make_pair(type, ptr), parent.u, parent.number);
		units_.insert(selected_hex_, u);
		u->from_widget(widget_cfg, true);
	}
	u->cell().widget.cell = cell;
	// modify id. can not be back to template when reload late.
	u->cell().id = tpl_id;

	replace_child_unit(u);
	if (u->has_child()) {
		layout_dirty();
	}
	gui_->show_context_menu();
}

void mkwin_controller::insert_used_widget_tpl(const config& tpl_cfg)
{
	used_widget_tpl_.insert(&tpl_cfg);
}

void mkwin_controller::derive_create(unit* u)
{
	int create_childs = 0;
	if (u->is_grid() || u->is_stacked_widget() || u->is_panel() || u->is_scrollbar_panel()) {
		create_childs = 1;

	} else if (u->is_listbox()) {
		u->insert_listbox_child(default_child_width, default_child_height);
		return;
	} else if (u->is_tree_view()) {
		u->insert_treeview_child();
		return;
	}
	for (int i = 0; i < create_childs; i ++) {
		u->insert_child(default_child_width, default_child_height);
	}
}

void mkwin_controller::replace_child_unit(unit* u)
{
	const unit::tparent& parent = u->parent();
	unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	const map_location& window_loc = child.window->get_location();
	int pitch = (u->get_location().y - 1 - window_loc.y) * child.cols.size();
	int at = pitch + u->get_location().x - 1 - window_loc.x;
	child.units[at] = u;
}

bool mkwin_controller::left_mouse_down(int x, int y, const bool browse)
{
	if (mouse_handler_base::left_mouse_down(x, y, browse)) {
		return false;
	}
	if (!map_.on_board(last_hex_)) {
		return false;
	}
	if (in_theme_top()) {
		int type = last_unit_? dynamic_cast<unit*>(last_unit_)->type(): unit::NONE;
		if (type == unit::WINDOW || type == unit::WIDGET) {
			selected_hex_ = last_unit_->get_location();
		} else {
			selected_hex_ = map_location::null_location;
		}
	} else {
		selected_hex_ = last_hex_;
	}
	
	unit* u = NULL;
	bool valid = false;
	if (last_hex_.x && last_hex_.y) {
		u = units_.find_unit(selected_hex_);
		if (in_theme_top()) {
			if (gui_->selected_widget() != gui_->spacer && !u) {
				int xmap = x;
				int ymap = y;
				gui_->pixel_screen_to_map(xmap, ymap);
				SDL_Rect rect = create_rect(xmap, ymap, gui_->hex_width() / 2, gui_->hex_width() / 2);
				u = new unit2(*this, *gui_, units_, gui_->selected_widget(), current_unit_, rect);
				gui2::tcell_setting& cell = u->cell();
				cell.rect_cfg = tadjust::generate_empty_rect_cfg();
				cell.rect_cfg["ref"] = theme::id_screen;
				cell.rect_cfg["rect"] = generate_anchor_rect(rect.x, rect.y, rect.w, rect.h);
				units_.insert2(*gui_, u);
				derive_create(u);

				selected_hex_ = u->get_location();

				valid = true;
			}

		} else if (u && gui_->selected_widget() != gui_->spacer && u->is_spacer()) {
			unit::tparent parent = u->parent();
			units_.erase(selected_hex_);

			units_.insert(selected_hex_, new unit(*this, *gui_, units_, gui_->selected_widget(), parent.u, parent.number));
			u = units_.find_unit(selected_hex_);
			derive_create(u);
			
			if (u->is_tpl()) {
				u->cell().id = unit::form_tpl_widget_id(unit::extract_from_widget_tpl(u->widget().first));
			}

			replace_child_unit(u);
			if (u->has_child()) {
				layout_dirty();
			}

			valid = true;
		}
	}

	if (valid) {
		fill_object_list();
	}
	gui_->show_context_menu();

	return true;
}

bool mkwin_controller::right_click(int x, int y, const bool browse)
{
	if (mouse_handler_base::right_click(x, y, browse)) {
		const SDL_Rect& rect = gui_->map_outside_area();
		if (!point_in_rect(x, y, rect)) {
			return false;
		}
	}

	do_right_click();
	
	return false;
}

void mkwin_controller::do_right_click()
{
	selected_hex_ = map_location();
	gui_->show_context_menu();
}

void mkwin_controller::post_change_resolution()
{
	set_status();

	unit* u = units_.find_unit(selected_hex_);
	if (u) {
		if (in_theme_top()) {
			gui_->scroll_to_xy(u->get_rect().x, u->get_rect().y, display::ONSCREEN);
		} else {
			gui_->scroll_to_tile(u->get_location(), display::ONSCREEN);
		}
	}

	gui_->show_context_menu();
}

void mkwin_controller::click_widget(const std::string& type, const std::string& definition)
{
	if (type != "grid") {
		gui_->click_widget(type, definition);
	} else {
		gui_->click_grid(type);
	}
}

bool mkwin_controller::toggle_report(gui2::twidget* widget)
{
	long index = reinterpret_cast<long>(widget->cookie());

	gui2::tmkwin_theme* theme = dynamic_cast<gui2::tmkwin_theme*>(gui_->get_theme());
	if (index == 0) {
		theme->load_widget_page();
	} else if (index == 1) {
		theme->load_object_page(units_);
	}
	return true;
}

void mkwin_controller::form_linked_groups(const config& res_cfg)
{
	linked_groups_.clear();

	BOOST_FOREACH (const config& linked, res_cfg.child_range("linked_group")) {
		linked_groups_.push_back(gui2::tlinked_group(linked));
	}
}

void mkwin_controller::generate_linked_groups(config& res_cfg) const
{
	config tmp;
	for (std::vector<gui2::tlinked_group>::const_iterator it = linked_groups_.begin(); it != linked_groups_.end(); ++ it) {
		const gui2::tlinked_group& linked = *it;

		tmp.clear();
		tmp["id"] = linked.id;
		if (linked.fixed_width) {
			tmp["fixed_width"] = true;
		}
		if (linked.fixed_height) {
			tmp["fixed_height"] = true;
		}
		res_cfg.add_child("linked_group", tmp);
	}
}

void mkwin_controller::form_context_menu(tmenu2& menu, const config& cfg, const std::string& menu_id)
{
	const std::string& items = cfg[menu_id].str();
	std::vector<std::string> vstr = utils::split(items);
	std::vector<std::string> vstr2;
	size_t flags;
	for (std::vector<std::string>::const_iterator it = vstr.begin(); it != vstr.end(); ++ it) {
		const std::string& id = *it;
		vstr2 = utils::split(id, '|');
		flags = 0;
		if (vstr2.size() >= 2) {
			flags = gui2::tcontext_menu::decode_flag_str(vstr[1]);
		}
		menu.items.push_back(tmenu2::titem(vstr2[0], NULL, flags & gui2::tcontext_menu::F_HIDE, flags & gui2::tcontext_menu::F_PARAM));
		size_t pos = id.rfind("_m");
		if (id.size() <= 2 || pos != id.size() - 2) {
			continue;
		}
		const std::string sub = id.substr(0, pos);
		menu.items.back().id = sub;
		menu.items.back().submenu = new tmenu2(null_str, sub, &menu);
		tmenu2& menu2 = *menu.items.back().submenu;
		form_context_menu(menu2, cfg, menu2.id);
	}
}

void mkwin_controller::form_context_menus(const config& res_cfg)
{
	context_menus_.clear();

	std::vector<std::string> vstr, vstr2;
	BOOST_FOREACH (const config &cfg, res_cfg.child_range("context_menu")) {
		const std::string& report = cfg["report"].str();
		const std::string& main = cfg["main"].str();
		if (report.empty() || main.empty()) {
			continue;
		}
		context_menus_.push_back(tmenu2(report, "main", NULL));

		tmenu2& menu = context_menus_.back();
		form_context_menu(menu, cfg, menu.id);
	}
	if (context_menus_.empty()) {
		context_menus_.push_back(tmenu2(null_str, "main", NULL));
	}
}

void mkwin_controller::generate_context_menus(config& res_cfg)
{
	for (std::vector<tmenu2>::const_iterator it = context_menus_.begin(); it != context_menus_.end(); ++ it) {
		const tmenu2& menu = *it;
		if (menu.items.empty()) {
			continue;
		}
		config& cfg = res_cfg.add_child("context_menu");
		menu.generate(cfg);
	}
}

config mkwin_controller::generate_window_cfg() const
{
	config top, tmp;
	config& window_cfg = top.add_child("window");
	units_.find_unit(map_location(0, 0))->generate(window_cfg);
	config& res_cfg = window_cfg.child("resolution");

	config& top_grid = res_cfg.add_child("grid");

	int rows = (int)top_.rows.size();
	int cols = (int)top_.cols.size();

	config unit_cfg;
	config* row_cfg = NULL;
	int current_y = -1;
	for (int y = 1; y < rows + 1; y ++) {
		for (int x = 1; x < cols + 1; x ++) {
			unit* u = units_.find_unit(map_location(x, y));
			if (u->get_location().y != current_y) {
				row_cfg = &top_grid.add_child("row");
				current_y = u->get_location().y;

				unit* row = units_.find_unit(map_location(0, y));
				row->generate(*row_cfg);
			}
			unit_cfg.clear();
			u->generate(unit_cfg);
			if (y == 1) {
				unit* column = units_.find_unit(map_location(x, 0));
				column->generate(unit_cfg);
			}
			row_cfg->add_child("column", unit_cfg);
		}
	}

	return top;
}

config mkwin_controller::generate_theme_cfg(const std::vector<unit*>& units)
{
	config top;
	config& theme_cfg = top.add_child("theme");
	get_window()->generate(theme_cfg);
	config& res_cfg = theme_cfg.child("resolution");

	// default(1024x768)
	for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
		const unit* u = *it;
		if (u->type() != unit::WIDGET) {
			continue;
		}
		u->generate(res_cfg);
	}
	
	std::stringstream ss;
	size_t size = modes_.size();
	config* parent_cfg = &theme_cfg;
	for (size_t n = 1; n < size; n ++) {
		const tmode& mode = modes_[n];
		config* cfg;
		std::string tag;
		if (n < tmode::res_count) {
			config& part = parent_cfg->add_child("partialresolution");
			ss.str("");
			ss << mode.res.width << "x" << mode.res.height;
			part["id"] = ss.str();
			ss.str("");
			const tres& res = modes_[n - 1].res;
			ss << res.width << "x" << res.height;
			tag = ss.str();
			part["inherits"] = ss.str();
			part["width"] = mode.res.width;
			part["height"] = mode.res.height;
			cfg = &part;
		} else {
			if (!(n % tmode::res_count)) {
				parent_cfg = &theme_cfg.add_child(mode.id);
			}
			ss.str("");
			ss << mode.res.width << "x" << mode.res.height;
			tag = ss.str();
			cfg = &parent_cfg->add_child(ss.str());
		}

		{
			ttheme_top_lock lock(*this);
			top_.generate_adjust(mode, *cfg);
		}

		if (n >= tmode::res_count && cfg->empty()) {
			parent_cfg->remove_child(tag, 0);
		}
	}
	
	return top;
}

void mkwin_controller::generate_theme_adjust(const tmode& mode, config& cfg) const
{
	std::vector<unit*> units = form_top_units();
	for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
		unit* u = *it;
		u->generate_adjust(mode, cfg);

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			it2->generate_adjust(mode, cfg); 
		}
	}
}

std::vector<unit*> mkwin_controller::form_top_units() const
{
	std::vector<unit*> result;

	if (!current_unit_) {
		if (theme_) {
			VALIDATE(top_.units.empty(), "units in top_ must be empty!");
			result = units_.form_units();

		} else {
			result = top_.units;
		}

	} else {
		result = theme_top_.units;
	}

	return result;
}

void mkwin_controller::reload_map(int w, int h, bool redraw)
{
	map_ = gamemap(game_config_, generate_map_data(w, h, in_theme_top()));
	gui_->reload_map();
	units_.zero_map();
	units_.create_coor_map(map_.w(), map_.h());
}

bool same_parent(const unit* a, const unit* b)
{
	const unit* a_parent = a;
	const unit* b_parent = b;

	while (a_parent->parent().u) {
		a_parent = a_parent->parent().u;
		if (a_parent == b) {
			return true;
		}
	}
	while (b_parent->parent().u) {
		b_parent = b_parent->parent().u;
		if (b_parent == a) {
			return true;
		}
	}
	return false;
}

bool mkwin_controller::can_copy(const unit* u, bool cut) const
{
	if (!u) {
		return false;
	}
	if (u == copied_unit_) {
		if (cut_) {
			return !cut;
		} else {
			return cut;
		}
	}
	if (u->type() == unit::WINDOW) {
		return false;
	}
	if (u->is_main_map()) {
		return false;
	}
	return true;
}

bool mkwin_controller::can_paste(const unit* u) const
{
	if (!u || !copied_unit_ || u == copied_unit_) {
		return false;
	}
	if (u->type() != copied_unit_->type()) {
		return false;
	}

	unit::tparent this_parent = u->parent();
	unit::tparent that_parent = copied_unit_->parent();

	if (u->type() == unit::ROW) {
		int this_cols = this_parent.u? this_parent.u->child(this_parent.number).cols.size(): top_.cols.size(); 
		int that_cols = that_parent.u? that_parent.u->child(that_parent.number).cols.size(): top_.cols.size();
		if (this_cols != that_cols) {
			return false;
		}
		
	} else if (u->type() == unit::COLUMN) {
		int this_rows = this_parent.u? this_parent.u->child(this_parent.number).rows.size(): top_.rows.size(); 
		int that_rows = that_parent.u? that_parent.u->child(that_parent.number).rows.size(): top_.rows.size();
		if (this_rows != that_rows) {
			return false;
		}

	} else if (!u->is_spacer()) {
		return false;
	}

	return !same_parent(u, copied_unit_);
}

bool mkwin_controller::can_adjust_row(const unit* u) const
{
	const unit::tparent& parent = u->parent();
	if (parent.u) {
		return !parent.u->is_listbox() || parent.number != 1;
	}
	return true;
}

bool mkwin_controller::can_adjust_column(const unit* u) const
{
	const unit::tparent& parent = u->parent();
	if (parent.u) {
		return !parent.u->is_listbox() || parent.number != 1;
	}
	return true;
}

bool mkwin_controller::can_erase(const unit* u) const
{
	if (!u) {
		return false;
	}

	const unit::tparent& parent = u->parent();

	if (u->type() == unit::WINDOW) {
		return parent.u && parent.u->is_grid();
	}
	if (u->type() != unit::WIDGET) {
		return false;
	}
	if (u->is_spacer() || u->is_main_map()) {
		return false;
	}
	if (parent.u) {
		return !parent.u->is_listbox() || parent.number != 1;
	}
	return true;
}

bool mkwin_controller::in_context_menu(const std::string& id) const
{
	using namespace gui2;
	std::pair<std::string, std::string> item = gui2::tcontext_menu::extract_item(id);
	int command = hotkey::get_hotkey(item.first).get_id();

	const unit* u = units_.find_unit(selected_hex_, true);

	switch(command) {
	// idle section
	case tmkwin_theme::HOTKEY_BUILD:
	{
		gui2::tmkwin_theme* theme = dynamic_cast<gui2::tmkwin_theme*>(gui_->get_theme());
		return !selected_hex_.valid() && theme->require_build();
	}

	case tmkwin_theme::HOTKEY_RUN:
	case HOTKEY_ZOOM_IN:
	case HOTKEY_ZOOM_OUT:
	case HOTKEY_SYSTEM:
		return !selected_hex_.valid();

	case tmkwin_theme::HOTKEY_SETTING: // setting
		return u && (u->type() != unit::WINDOW || (!u->parent().u || u->parent().u->is_stacked_widget()));
	case tmkwin_theme::HOTKEY_SPECIAL_SETTING:
		return u && u->has_special_setting();

	case HOTKEY_COPY:
		return can_copy(u, false);
	case HOTKEY_CUT:
		return can_copy(u, true);
	case HOTKEY_PASTE:
		return can_paste(u);

	// unit
	case tmkwin_theme::HOTKEY_ERASE: // erase
		return can_erase(u);

	// window
	case tmkwin_theme::HOTKEY_LINKED_GROUP:
		return u && u->type() == unit::WINDOW && !u->parent().u;

	// row
	case tmkwin_theme::HOTKEY_INSERT_TOP:
	case tmkwin_theme::HOTKEY_INSERT_BOTTOM:
	case tmkwin_theme::HOTKEY_ERASE_ROW:
		return !in_theme_top() && u && u->type() == unit::ROW && can_adjust_row(u);

	// column
	case tmkwin_theme::HOTKEY_INSERT_LEFT:
	case tmkwin_theme::HOTKEY_INSERT_RIGHT:
	case tmkwin_theme::HOTKEY_ERASE_COLUMN:
		return !in_theme_top() && u && u->type() == unit::COLUMN && can_adjust_column(u);

	case tmkwin_theme::HOTKEY_INSERT_CHILD: // add a page
		return u && u->is_stacked_widget();
	case tmkwin_theme::HOTKEY_ERASE_CHILD: // erase this page
		return u && u->type() == unit::WINDOW && u->parent().u && u->parent().u->is_stacked_widget() && u->parent().u->children().size() >= 2;

	case tmkwin_theme::HOTKEY_UNPACK:
		return u && u->is_tpl();

	default:
		return false;
	}

	return false;
}

bool mkwin_controller::actived_context_menu(const std::string& id) const
{
	using namespace gui2;
	std::pair<std::string, std::string> item = gui2::tcontext_menu::extract_item(id);
	int command = hotkey::get_hotkey(item.first).get_id();

	const unit* u = units_.find_unit(selected_hex_, true);
	if (!u) {
		return true;
	}
	const unit::tparent& parent = u->parent();
	const unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	switch(command) {
	// row
	case tmkwin_theme::HOTKEY_ERASE_ROW:
		return child.rows.size() >= 2;

	// column
	case tmkwin_theme::HOTKEY_ERASE_COLUMN:
		return child.cols.size() >= 2;

	}
	return true;
}

bool mkwin_controller::verify_new_mode(const std::string& str)
{
	std::stringstream err;
	for (std::vector<tmode>::const_iterator it = modes_.begin(); it != modes_.end(); ++ it) {
		if (it->id == str) {
			err << _("Duplicate mode!");
			gui2::show_message(gui_->video(), "", err.str());
			return false;
		}
	}
	return true;
}

void mkwin_controller::insert_mode(const std::string& id)
{
	modes_.push_back(tmode(id, 1024, 768));
	modes_.push_back(tmode(id, 640, 480));
	modes_.push_back(tmode(id, 480, 320));
}

// parameter(id) maybe reference of desire erase's id.
void mkwin_controller::erase_patch(const std::string id)
{
	VALIDATE(id != tmode::def_id, "Must not erase reserved patch!");

	for (std::vector<tmode>::iterator it = modes_.begin(); it != modes_.end(); ) {
		const tmode& mode = *it;
		if (mode.id == id) {
			it = modes_.erase(it);
		} else {
			++ it;
		}
	}
}

// parameter(form) maybe reference of desire erase's id.
void mkwin_controller::rename_patch(const std::string from, const std::string& to)
{
	VALIDATE(from != tmode::def_id, "Must not rename reserved patch!");

	for (std::vector<tmode>::iterator it = modes_.begin(); it != modes_.end(); ++ it) {
		tmode& mode = *it;
		if (mode.id == from) {
			mode.id = to;
		}
	}
}

void mkwin_controller::refresh_title()
{
	std::stringstream ss;
	gui2::tcontrol* label = dynamic_cast<gui2::tcontrol*>(gui_->get_theme_object("title"));
	if (theme_) {
		ss << _("Theme");
	} else {
		ss << _("Dialog");
	}
	ss << " (";

	std::string file = "---";
	if (!file_.empty()) {
		file = file_name(file_);
	}
	ss << tintegrate::generate_format(file, "blue");
	ss << ")";

	label->set_label(ss.str());
}

