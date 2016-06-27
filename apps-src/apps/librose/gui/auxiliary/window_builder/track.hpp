/* $Id: button.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_AUXILIARY_WINDOW_BUILDER_TRACK_HPP_INCLUDED
#define GUI_AUXILIARY_WINDOW_BUILDER_TRACK_HPP_INCLUDED

#include "gui/auxiliary/window_builder/control.hpp"

namespace gui2 {

class tcontrol;

namespace implementation {

struct tbuilder_track
	: public tbuilder_control
{
public:
	explicit tbuilder_track(const config& cfg);

	twidget* build () const;

private:
	std::string width;
	std::string height;
	std::string hdpi_off;
};

} // namespace implementation

} // namespace gui2

#endif

