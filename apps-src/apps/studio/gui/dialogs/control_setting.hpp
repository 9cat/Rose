/* $Id: campaign_difficulty.hpp 49603 2011-05-22 17:56:17Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_CONTROL_SETTING_HPP_INCLUDED
#define GUI_DIALOGS_CONTROL_SETTING_HPP_INCLUDED

#include "gui/dialogs/cell_setting.hpp"
#include "gui/dialogs/mode_navigate.hpp"
#include "unit.hpp"

class display;
class unit;
class mkwin_controller;

namespace gui2 {

struct tlinked_group;
class tstacked_widget;

class tcontrol_setting : public tsetting_dialog, public tmode_navigate
{
public:
	static const std::string default_ref;
	enum {BASE_PAGE, RECT_PAGE, SIZE_PAGE, ADVANCED_PAGE};

	explicit tcontrol_setting(display& disp, mkwin_controller& controller, unit& u, const std::vector<std::string>& textdomains, const std::vector<tlinked_group>& linkeds);

protected:
	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	void update_title(twindow& window);

	void save(twindow& window, bool& handled, bool& halt);

	void set_horizontal_layout(twindow& window);
	void set_vertical_layout(twindow& window);
	
	void set_horizontal_layout_label(twindow& window);
	void set_vertical_layout_label(twindow& window);

	void set_linked_group(twindow& window);
	void set_linked_group_label(twindow& window);

	void set_textdomain(twindow& window, bool label);
	void set_textdomain_label(twindow& window, bool label);

	bool pre_toggle_tabbar(twidget* widget, twidget* previous);
	void toggle_report(twidget* widget);

	void pre_base(twindow& window);
	void pre_rect(twindow& window);
	void pre_size(twindow& window);
	void pre_advanced(twindow& window);

	bool save_base(twindow& window);
	bool save_rect(twindow& window);
	bool save_size(twindow& window);
	bool save_advanced(twindow& window);

	//
	// rect page
	// 
	void set_ref(twindow& window);
	void set_ref_label(twindow& window, const config& cfg);
	void set_rect_label(twindow& window, const config& cfg, int n);
	bool collect_ref(const unit::tchild& child, std::vector<std::string>& refs) const;

	int calculate_rect_coor_op(const std::string& val) const;
	std::string calculate_rect_coor(twindow& window, int n) const;
	void set_rect_coor(twindow& window, int n);
	void set_xanchor(twindow& window);
	void set_xanchor_label(twindow& window, const config& cfg) const;
	void set_yanchor(twindow& window);
	void set_yanchor_label(twindow& window, const config& cfg) const;

	void rect_toggled(twidget* widget);
	void custom_rect_toggled(twidget* widget);
	bool rect_pre_toggle_tabbar(twidget* widget, twidget* previous);
	void rect_toggle_tabbar(twidget* widget);

	void switch_rect_cfg(twindow& window, const config& cfg);

	std::string form_tab_label(treport& navigate, int at) const;
	bool require_show_flag(treport& navigate, int index) const;

	//
	// advanced page
	//
	bool advanced_pre_toggle_tabbar(twidget* widget, twidget* previous);
	void advanced_toggle_tabbar(twidget* widget);
	
	void set_horizontal_mode(twindow& window);
	void set_horizontal_mode_label(twindow& window);
	void set_vertical_mode(twindow& window);
	void set_vertical_mode_label(twindow& window);
	void set_advanced_misc(twindow& window, const config& cfg, bool enable);
	void set_definition(twindow& window);
	void set_multi_line(twindow& window);
	void set_report_attribute(twindow& window, const config& cfg, bool enable);
	void set_text_box2_text_box(twindow& window);
	void switch_advanced_cfg(twindow& window, const config& cfg, bool enable);

	config get_derived_res_change_cfg(int current_tab, bool rect) const;

protected:
	display& disp_;
	mkwin_controller& controller_;
	unit& u_;
	const std::vector<std::string>& textdomains_;
	const std::vector<tlinked_group>& linkeds_;
	treport* bar_;
	tstacked_widget* page_panel_;

	treport* rect_navigate_;
	int rect_current_tab_;
	treport* advanced_navigate_;
	int advanced_current_tab_;

	//
	// rect page
	//
	std::map<int, std::vector<std::string> > candidate_rect_coor_ops_;
	std::vector<int> rect_coor_ops_;
	bool must_use_rect_;
	config current_rect_cfg_;

	//
	// advanced page
	//
	config current_advanced_cfg_;
};


}


#endif /* ! GUI_DIALOGS_CAMPAIGN_DIFFICULTY_HPP_INCLUDED */
