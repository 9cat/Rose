/* $Id: title_screen.cpp 48740 2011-03-05 10:01:34Z mordante $ */
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

#include "gui/dialogs/rose.hpp"

#include "display.hpp"
#include "game_config.hpp"
#include "preferences.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/browse.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/tree_view.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/toggle_panel.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/track.hpp"
#include "gui/widgets/window.hpp"
#include "preferences_display.hpp"
#include "help.hpp"
#include "version.hpp"
#include "filesystem.hpp"
#include "loadscreen.hpp"
#include <hero.hpp>
#include <time.h>
#include "sound.hpp"

#include <boost/bind.hpp>

#include <algorithm>

namespace gui2 {

REGISTER_DIALOG(rose)

trose::trose(display& disp, hero& player_hero)
	: disp_(disp)
	, player_hero_(player_hero)
	, window_(NULL)
	, select_all_(false)
	, back_2_path_(false)
	, current_copier_(NULL)
{
}

trose::~trose()
{
	if (task_thread_) {
		exit_task_ = true;
		delete task_thread_;
		task_thread_ = NULL;
		exit_task_ = false;
	}
}

static const char* menu_items[] = {
	"edit_dialog",
	"player",
	"edit_theme",
	"language",
	"chat",
	"preferences",
	"design"
};
static int nb_items = sizeof(menu_items) / sizeof(menu_items[0]);

static std::vector<int> ids;
void trose::pre_show(CVideo& video, twindow& window)
{
	ids.clear();
	window_ = &window;

	set_restore(false);
	window.set_escape_disabled(true);

	// Set the version number
	tcontrol* control = find_widget<tcontrol>(&window, "revision_number", false, false);
	if (control) {
		control->set_label(_("V") + game_config::version);
	}
	window.canvas()[0].set_variable("revision_number", variant(_("Version") + std::string(" ") + game_config::version));
	window.canvas()[0].set_variable("background_image",	variant("misc/white-background.png"));

	/***** Set the logo *****/
	tcontrol* logo = find_widget<tcontrol>(&window, "logo", false, false);
	if (logo) {
		logo->set_label(game_config::logo_png);
	}

	tbutton* b;
	for (int item = 0; item < nb_items; item ++) {
		b = find_widget<tbutton>(&window, menu_items[item], false, false);
		if (!b) {
			continue;
		}
		std::string str;
		if (!strcmp(menu_items[item], "player")) {
			str = "hero-256/100.png";

		} else {
			str = std::string("icons/") + menu_items[item] + ".png";
		}

		for (int i = 0; i < 4; i ++) {
			b->canvas()[i].set_variable("image", variant(str));
		}
	}

	for (int item = 0; item < nb_items; item ++) {
		std::string id = menu_items[item];
		int retval = twindow::NONE;
		if (id == "edit_dialog") {
			retval = EDIT_DIALOG;
		} else if (id == "player") {
			retval = PLAYER;
		} else if (id == "edit_theme") {
			retval = EDIT_THEME;
		} else if (id == "language") {
			retval = CHANGE_LANGUAGE;
		} else if (id == "chat") {
			retval = MESSAGE;
		} else if (id == "preferences") {
			retval = EDIT_PREFERENCES;
		} else if (id == "design") {
			retval = DESIGN;
		}

		connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, id, false)
			, boost::bind(
				&trose::set_retval
				, this
				, boost::ref(window)
				, retval));

		if (retval == PLAYER) {
			find_widget<tbutton>(&window, id, false).set_visible(twidget::INVISIBLE);
		}
	}

	tlobby::thandler::join();

	ttree_view* explorer = find_widget<ttree_view>(&window, "explorer", false, true);
	ttree_view_node& root = explorer->get_root_node();
	ttree_view_node& htvi = add_explorer_node(game_config::path, root, file_name(game_config::path), true);
	::walk_dir(editor_.working_dir(), false, boost::bind(
				&trose::walk_dir2
				, this
				, _1, _2, &htvi));
	htvi.sort_children(boost::bind(&trose::compare_sort, this, _1, _2));
	htvi.unfold();
    explorer->invalidate_layout(true);

	pre_base(window);
}

void trose::pre_base(twindow& window)
{
	tbuild::pre_show(find_widget<ttrack>(&window, "task_status", false));

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "browse", false)
			, boost::bind(
				&trose::set_working_dir
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "refresh", false)
			, boost::bind(
				&trose::do_refresh
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "build", false)
			, boost::bind(
				&trose::do_build
				, this
				, boost::ref(window)));

	reload_mod_configs(disp_, mod_copiers, app_copiers);
	fill_items(window);
}

ttree_view_node& trose::add_explorer_node(const std::string& dir, ttree_view_node& parent, const std::string& name, bool isdir)
{
	string_map tree_group_field;
	std::map<std::string, string_map> tree_group_item;

	tree_group_field["label"] = name;
	tree_group_item["text"] = tree_group_field;
	tree_group_field["label"] = isdir? "misc/dir.png": "misc/file.png";
	tree_group_item["type"] = tree_group_field;
	ttree_view_node& htvi = parent.add_child("node", tree_group_item, twidget::npos, isdir);
	htvi.icon()->set_label("fold-common");

	cookies_.insert(std::make_pair(&htvi, tcookie(dir, name, isdir)));
	if (isdir) {
		htvi.icon()->set_callback_state_change(boost::bind(&trose::icon_toggled, this, _1));
	}
	htvi.connect_signal<event::RIGHT_BUTTON_CLICK>(boost::bind(
		&trose::right_click_explorer, this, boost::ref(htvi), _3, _4));
	htvi.connect_signal<event::RIGHT_BUTTON_CLICK>(boost::bind(
		&trose::right_click_explorer, this, boost::ref(htvi), _3, _4), event::tdispatcher::back_post_child);

	return htvi;
}

bool trose::compare_sort(const ttree_view_node& a, const ttree_view_node& b)
{
	const tcookie& a2 = cookies_.find(&a)->second;
	const tcookie& b2 = cookies_.find(&b)->second;

	if (a2.isdir && !b2.isdir) {
		// tvi1 is directory, tvi2 is file
		return true;
	} else if (!a2.isdir && b2.isdir) {
		// tvi1 is file, tvi2 if directory
		return false;
	} else {
		// both lvi1 and lvi2 are directory or file, use compare string.
		return SDL_strcasecmp(a2.name.c_str(), b2.name.c_str()) <= 0? true: false;
	}
}

bool trose::walk_dir2(const std::string& dir, const SDL_dirent2* dirent, ttree_view_node* root)
{
	bool isdir = SDL_DIRENT_DIR(dirent->mode);
	add_explorer_node(dir, *root, dirent->name, isdir);

	return true;
}

void trose::icon_toggled(twidget* widget)
{
	ttoggle_button* toggle = dynamic_cast<ttoggle_button*>(widget);
	ttree_view_node& node = ttree_view_node::node_from_icon(*toggle);
	if (!toggle->get_value() && node.empty()) {
		const tcookie& cookie = cookies_.find(&node)->second;
		::walk_dir(cookie.dir + '/' + cookie.name, false, boost::bind(
				&trose::walk_dir2
				, this
				, _1, _2, &node));
		node.sort_children(boost::bind(&trose::compare_sort, this, _1, _2));
	}
}

bool trose::handle(int tag, tsock::ttype type, const config& data)
{
	if (tag != tlobby::tag_chat) {
		return false;
	}

	if (type != tsock::t_data) {
		return false;
	}
	if (const config& c = data.child("whisper")) {
		tbutton* b = find_widget<tbutton>(window_, "message", false, false);
		if (b->label().empty()) {
			b->set_label("misc/red-dot12.png");
		}
		sound::play_UI_sound(game_config::sounds::receive_message);
	}
	return false;
}

void trose::set_retval(twindow& window, int retval)
{
	if (build_ctx_.status != tbuild_ctx::stopped) {
		return;
	}
	window.set_retval(retval);
}

void trose::fill_items(twindow& window)
{
	if (!file_exists(editor_.working_dir() + "/data/core/_main.cfg") || !is_directory(editor_.working_dir() + "/xwml")) {
		return;
	}

	editor_.reload_extendable_cfg();

	std::vector<editor::BIN_TYPE> system_bins;
	for (editor::BIN_TYPE type = editor::BIN_MIN; type <= editor::BIN_SYSTEM_MAX; type = (editor::BIN_TYPE)(type + 1)) {
		system_bins.push_back(type);
	}
	editor_.get_wml2bin_desc_from_wml(system_bins);
	const std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();

	bool enable_build = false;
	std::stringstream ss;
	tlistbox* list = find_widget<tlistbox>(&window, "items", false, true);
	list->set_row_align(false);
	list->clear();
	for (std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >::const_iterator it = descs.begin(); it != descs.end(); ++ it) {
		const editor::wml2bin_desc& desc = it->second;

		string_map list_item;
		std::map<std::string, string_map> list_item_item;

		ss.str("");
		if (desc.wml_nfiles == desc.bin_nfiles && desc.wml_sum_size == desc.bin_sum_size && desc.wml_modified == desc.bin_modified) {
			ss << tintegrate::generate_img("misc/ok-tip.png");
		} else {
			ss << tintegrate::generate_img("misc/alert-tip.png");
		}
		ss << desc.bin_name;
		list_item["label"] = ss.str();
		list_item_item.insert(std::make_pair("filename", list_item));

		list_item["label"] = desc.app;
		list_item_item.insert(std::make_pair("app", list_item));

		ss.str("");
		ss << "(" << desc.wml_nfiles << ", " << desc.wml_sum_size << ", " << desc.wml_modified << ")";
		list_item["label"] = ss.str();
		list_item_item.insert(std::make_pair("wml_checksum", list_item));

		ss.str("");
		ss << "(" << desc.bin_nfiles << ", " << desc.bin_sum_size << ", " << desc.bin_modified << ")";
		list_item["label"] = ss.str();
		list_item_item.insert(std::make_pair("bin_checksum", list_item));

		list->add_row(list_item_item);

		twidget* panel = list->get_row_panel(list->get_item_count() - 1);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		if (select_all_) {
			enable_build = true;

		} else if (it->first == editor::MAIN_DATA && (desc.wml_nfiles != desc.bin_nfiles || desc.wml_sum_size != desc.bin_sum_size || desc.wml_modified != desc.bin_modified)) {	
			enable_build = true;

		} else if (it->first == editor::SCENARIO_DATA && (desc.wml_nfiles != desc.bin_nfiles || desc.wml_sum_size != desc.bin_sum_size || desc.wml_modified != desc.bin_modified)) {
			const std::string id = file_main_name(desc.bin_name);
			if (editor_config::campaign_id.empty() || id == editor_config::campaign_id) {
				enable_build = true;
			}
		}
		if (enable_build) {
			prefix->set_value(true);
		}
		prefix->set_callback_state_change(boost::bind(&trose::check_build_toggled, this, _1));
	}

	list->invalidate_layout(true);

	find_widget<tbutton>(&window, "build", false).set_active(enable_build);
}

void trose::set_build_active(twindow& window)
{
	tlistbox* list = find_widget<tlistbox>(&window, "items", false, true);
	int count = list->get_item_count();
	for (int at = 0; at < count; at ++) {
		twidget* panel = list->get_row_panel(at);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		if (prefix->get_value()) {
			find_widget<tbutton>(&window, "build", false).set_active(true);
			return;
		}
	}
	find_widget<tbutton>(&window, "build", false).set_active(false);
}

void trose::check_build_toggled(twidget* widget)
{
	set_build_active(*window_);
}

void trose::extract_app(tapp_copier& copier)
{
	std::stringstream ss;
	utils::string_map symbols;

	if (!copier.valid()) {
		symbols["app"] = copier.app;
		ss.str("");
		ss << utf8_2_ansi(vgettext2("Can not get generate config of $app, extract $app package fail!", symbols)); 
		gui2::show_message(disp_.video(), null_str, ss.str());
		return;
	}

	symbols["app"] = copier.app;
	symbols["dst"] = copier.get_path(tapp_copier::app_src2_tag);
	ss.str("");
	ss << utf8_2_ansi(vgettext2("Do you want to extract $app package to $dst?", symbols).c_str()); 

	int res = gui2::show_message(disp_.video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
	if (res != gui2::twindow::OK) {
		return;
	}

	absolute_draw();
	bool fok = copier.opeate_file(disp_);
	if (!fok) {
		symbols["src"] = copier.get_path(tapp_copier::src2_tag);
		symbols["result"] = fok? "Success": "Fail";
		ss.str("");
		ss << utf8_2_ansi(vgettext2("Extract $app package from \"$src\" to \"$dst\", $result!", symbols)); 
		gui2::show_message(disp_.video(), null_str, ss.str());
		return;
	}

	current_copier_ = &copier;
	{
		// build
		tselect_all_lock lock(*this);
		on_change_working_dir(*window_, copier.get_path(tapp_copier::app_res_tag));
		do_build(*window_);
	}
}

void trose::on_change_working_dir(twindow& window, const std::string& dir)
{
	editor_.set_working_dir(dir);
	fill_items(window);
	task_status_->set_dirty();
}

bool trose::generate_kingdom_mod_res(tmod_copier& mod_config, const std::string& kingdom_res, const std::string& kingdom_star_patch, const std::string& kingdom_star)
{
	config copy_cfg = get_generate_cfg(editor_config::data_cfg, "copy");
	copy_cfg["path-res_mod"] = mod_config.get_path("res");
	tcopier copier(copy_cfg);
	tcallback_lock lock(false, boost::bind(&tcopier::do_delete_path, &copier, _1));

	if (!copier.make_path(disp_, "res_mod", true)) {
		return false;
	}

	if (!copier.do_copy(disp_, "res", "res_mod")) {
		return false;
	}
	
	if (!mod_config.opeate_file(disp_, true)) {
		return false;
	}

	lock.set_result(true);
	return true;
}

void trose::generate_mod(tmod_copier& current_mod)
{
/*
	std::stringstream ss;
	utils::string_map symbols;

	symbols["mod"] = current_mod.name();
	symbols["mod_res_path"] = current_mod.get_path(tmod_copier::res_tag);
	ss << vgettext2("Do you want to generate $mod resource package to $mod_res_path?", symbols);
	int res = gui2::show_message(disp_.video(), _("Confirm generate"), ss.str(), gui2::tmessage::yes_no_buttons);
	if (res == gui2::twindow::OK) {
		bool fok = generate_kingdom_mod_res(current_mod, gdmgr._menu_text, current_mod.get_path(tmod_copier::patch_tag), current_mod.get_path(tmod_copier::res_tag));
		symbols["src1"] = gdmgr._menu_text;
		symbols["src2"] = current_mod.get_path(tmod_copier::patch_tag);
		symbols["dst"] = current_mod.get_path(tmod_copier::res_tag);
		symbols["result"] = fok? "Success": "Fail";
		ss.str("");
		ss << vgettext2("Generate $mod resource package from \"$src1\" and \"$src2\" to \"$dst\", $result!", symbols);
		gui2::show_message(disp_.video(), null_str, ss.str());
	}
*/
}

void trose::right_click_explorer(ttree_view_node& node, bool& handled, bool& halt)
{
	if (!node.parent_node().is_root_node()) {
		return;
	}
	if (app_copiers.empty()) {
		return;
	}

	std::vector<gui2::tval_str> items;
	
	int at = 0;
	for (std::vector<tapp_copier>::const_iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
		const tapp_copier& app = *it;
		items.push_back(gui2::tval_str(at ++, app.app));
	}

	int x, y;
	SDL_GetMouseState(&x, &y);
	int selected;
	{
		gui2::tcombo_box dlg(items, 0);
		dlg.show(disp_.video(), 0, x, y);
		int retval = dlg.get_retval();
		if (dlg.get_retval() != gui2::twindow::OK) {
			return;
		}
		absolute_draw();
		selected = dlg.selected_val();
	}
	tapp_copier& current_app = app_copiers[selected];
	extract_app(current_app);

	handled = halt = true;
}

void trose::set_working_dir(twindow& window)
{
	std::string desire_dir;
	{
		gui2::tbrowse::tparam param(gui2::tbrowse::TYPE_DIR, true, null_str, _("Choose a Working Directory to Build"));
		gui2::tbrowse dlg(disp_, param);
		dlg.show(disp_.video());
		int res = dlg.get_retval();
		if (res != gui2::twindow::OK) {
			return;
		}
		desire_dir = param.result;
	}
	if (desire_dir == editor_.working_dir()) {
		return;
	}
	if (!check_res_folder(desire_dir)) {
		std::stringstream err;
		err << desire_dir << " isn't valid res directory";
		gui2::show_message(disp_.video(), null_str, err.str());
		return;
	}

	on_change_working_dir(window, desire_dir);
}

void trose::do_refresh(twindow& window)
{
	fill_items(window);
}

void trose::do_build(twindow& window)
{
	do_build2();
}

void trose::on_start_build()
{
	twindow& window = *window_;
	tlistbox& list = find_widget<tlistbox>(&window, "items", false);

	find_widget<tbutton>(&window, "refresh", false).set_active(false);
	find_widget<tbutton>(&window, "build", false).set_active(false);
	list.set_active(false);

	std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();
	int count = list.get_item_count();
	for (int at = 0; at < count; at ++) {
		twidget* panel = list.get_row_panel(at);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		descs[at].second.require_build = prefix->get_value();

		find_widget<tcontrol>(panel, "status", false).set_label(null_str);
	}

	if (select_all_) {
		back_2_path_ = true;
	}
}

void trose::on_stop_build()
{
	twindow& window = *window_;
	tbutton& refresh = find_widget<tbutton>(&window, "refresh", false);
	tbutton& build = find_widget<tbutton>(&window, "build", false);
	tlistbox& list = find_widget<tlistbox>(&window, "items", false);

	list.set_active(true);
	refresh.set_active(true);
	build.set_active(true);

	add_timer(1, boost::bind(&trose::build_finished, this, boost::ref(window)), false);
}

void trose::build_finished(twindow& window)
{
	if (!back_2_path_) {
		return;
	}
	back_2_path_ = false;

	on_change_working_dir(window, game_config::path);
	absolute_draw();

	// copy res to android/res
	current_copier_->copy_res_2_android_prj();

	std::stringstream ss;
	utils::string_map symbols;

	symbols["src"] = current_copier_->get_path(tapp_copier::src2_tag);
	symbols["dst"] = current_copier_->get_path(tapp_copier::app_src2_tag);
	symbols["result"] = "Success";
	ss.str("");
	ss << utf8_2_ansi(vgettext2("Extract $app package from \"$src\" to \"$dst\", $result!", symbols)); 
	gui2::show_message(disp_.video(), null_str, ss.str());

	int ii = 0;
}

void trose::handle_task()
{
	tlistbox& list = find_widget<tlistbox>(window_, "items", false);
	std::stringstream ss;
	std::string str;
	std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();
	for (std::vector<ttask>::iterator it = tasks_.begin(); it != tasks_.end(); ) {
		const ttask& task = *it;
		editor::wml2bin_desc& desc = descs[task.at].second;
		twidget* panel = list.get_row_panel(task.at);

		if (ttask::stopped) {
			tcontrol* filename = find_widget<tcontrol>(panel, "filename", false, true);
			ss.str("");
			ss << tintegrate::generate_img(task.ret? "misc/ok-tip.png": "misc/alert-tip.png");
			ss << desc.bin_name;
			filename->set_label(ss.str());

			tcontrol* wml_checksum = find_widget<tcontrol>(panel, "wml_checksum", false, true);
			ss.str("");
			ss << "(" << desc.wml_nfiles << ", " << desc.wml_sum_size << ", " << desc.wml_modified << ")";
			wml_checksum->set_label(ss.str());

			tcontrol* bin_checksum = find_widget<tcontrol>(panel, "bin_checksum", false, true);
			desc.refresh_checksum(editor_.working_dir());
			ss.str("");
			ss << "(" << desc.bin_nfiles << ", " << desc.bin_sum_size << ", " << desc.bin_modified << ")";
			bin_checksum->set_label(ss.str());
		}

		tcontrol* status = find_widget<tcontrol>(panel, "status", false, true);
		if (task.type == ttask::started) {
			str = "misc/operating.png";
		} else if (task.type == ttask::stopped) {
			str = task.ret? "misc/success.png": "misc/fail.png";
		}
		status->set_label(str);

		it = tasks_.erase(it);
	}
	list.invalidate_layout(true);
}

} // namespace gui2

