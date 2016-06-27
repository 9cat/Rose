/* $Id: spacer.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#include "gui/widgets/spacer.hpp"

#include "gui/auxiliary/widget_definition/spacer.hpp"
#include "gui/auxiliary/window_builder/spacer.hpp"
#include "gui/widgets/settings.hpp"

#include <boost/bind.hpp>

namespace gui2 {

tspacer* create_spacer(const std::string& id, int width, int height)
{
	config cfg;
	if (!id.empty()) {
		cfg["id"] = id;
	}
	cfg["definition"] = "default";
	if (width) {
		cfg["width"] = width;
	}
	if (height) {
		cfg["height"] = height;
	}

	implementation::tbuilder_spacer builder(cfg);
	return dynamic_cast<tspacer*>(builder.build());
}

REGISTER_WIDGET(spacer)

void tspacer::set_best_size2(int width, int height)
{
	hdpi_off_width_ = hdpi_off_height_ = true;
	set_best_size(str_cast(width), str_cast(height), null_str);
}

const std::string& tspacer::get_control_type() const
{
	static const std::string type = "spacer";
	return type;
}

} // namespace gui2


