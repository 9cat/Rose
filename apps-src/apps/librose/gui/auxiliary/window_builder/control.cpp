/* $Id: control.cpp 54322 2012-05-28 08:21:28Z mordante $ */
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

#include "gui/auxiliary/window_builder/control.hpp"

#include "config.hpp"
#include "formatter.hpp"
#include "gettext.hpp"
#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/window_builder/helper.hpp"
#include "gui/widgets/control.hpp"
#include "gui/widgets/settings.hpp"

namespace theme {
extern SDL_Rect calculate_relative_loc(const config& cfg, int screen_w, int screen_h);
}

namespace gui2 {

namespace implementation {

tbuilder_control::tbuilder_control(const config& cfg)
	: tbuilder_widget(cfg)
	, definition(cfg["definition"])
	, label(cfg["label"].t_str())
	, tooltip(cfg["tooltip"].t_str())
	, help(cfg["help"].t_str())
	, drag(get_drag(cfg["drag"]))
	, use_tooltip_on_label_overflow(true)
	, fix_rect(empty_rect)
{
	if (definition.empty()) {
		definition = "default";
	}

	if (cfg.has_attribute("rect")) {
		fix_rect = theme::calculate_relative_loc(cfg, settings::screen_width, settings::screen_height);
	}

	VALIDATE_WITH_DEV_MESSAGE(help.empty() || !tooltip.empty()
			, _("Found a widget with a helptip and without a tooltip.")
			, (formatter() << "id '" << id
				<< "' label '" << label
				<< "' helptip '" << help << "'.").str());


	DBG_GUI_P << "Window builder: found control with id '"
			<< id << "' and definition '" << definition << "'.\n";
}

void tbuilder_control::init_control(tcontrol* control) const
{
	assert(control);

	control->set_id(id);
	control->set_definition(definition);
	control->set_linked_group(linked_group);
	control->set_label(label);
	control->set_tooltip(tooltip);
	control->set_use_tooltip_on_label_overflow(use_tooltip_on_label_overflow);
	if (fix_rect.w && fix_rect.h) {
		control->set_fix_rect(fix_rect);
	}
	control->set_drag(drag);
}

twidget* tbuilder_control::build(const treplacements& /*replacements*/) const
{
	return build();
}

} // namespace implementation

} // namespace gui2

/*WIKI
 * @page = GUIWidgetInstanceWML
 * @order = 1_widget
 *
 * = Widget =
 * @begin{parent}{name="generic/"}
 * @begin{tag}{name="widget_instance"}{min="0"}{max="-1"}
 * All widgets placed in the cell have some values in common:
 * @begin{table}{config}
 *     id & string & "" &              This value is used for the engine to
 *                                     identify 'special' items. This means that
 *                                     for example a text_box can get the proper
 *                                     initial value. This value should be
 *                                     unique or empty. Those special values are
 *                                     documented at the window definition that
 *                                     uses them. NOTE items starting with an
 *                                     underscore are used for composed widgets
 *                                     and these should be unique per composed
 *                                     widget. $
 *
 *     definition & string & "default" &
 *                                     The id of the widget definition to use.
 *                                     This way it's possible to select a
 *                                     specific version of the widget e.g. a
 *                                     title label when the label is used as
 *                                     title. $
 *
 *     linked_group & string & "" &    The linked group the control belongs
 *                                     to. $
 *
 *     label & t_string & "" &          Most widgets have some text associated
 *                                     with them, this field contain the value
 *                                     of that text. Some widgets use this value
 *                                     for other purposes, this is documented
 *                                     at the widget. E.g. an image uses the
 *                                     filename in this field. $
 *
 *     tooltip & t_string & "" &        If you hover over a widget a while (the
 *                                     time it takes can differ per widget) a
 *                                     short help can show up.This defines the
 *                                     text of that message. This field may not
 *                                     be empty when 'help' is set. $
 *
 *
 *     help & t_string & "" &           If you hover over a widget and press F10
 *                                     (or the key the user defined for the help
 *                                     tip) a help message can show up. This
 *                                     help message might be the same as the
 *                                     tooltip but in general (if used) this
 *                                     message should show more help. This
 *                                     defines the text of that message. $
 *
 *    use_tooltip_on_label_overflow & bool & true &
 *                                     If the text on the label is truncated and
 *                                     the tooltip is empty the label can be
 *                                     used for the tooltip. If this variable is
 *                                     set to true this will happen. $
 *
 *
 *   size_text & t_string & "" &       Sets the minimum width of the widget
 *                                     depending on the text in it. (Note not
 *                                     implemented yet.) $
 * @end{table}
 * @end{tag}{name="widget_instance"}
 * @end{parent}{name="generic/"}
 */

