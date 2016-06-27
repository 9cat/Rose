/* $Id: events.cpp 46186 2010-09-01 21:12:38Z silene $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "clipboard.hpp"
#include "cursor.hpp"
#include "events.hpp"
#include "sound.hpp"
#include "video.hpp"
#include "game_end_exceptions.hpp"
#include "display.hpp"
#include "preferences.hpp"
#include "gui/widgets/settings.hpp"
#include "posix.h"
#include "base_instance.hpp"

#include "SDL.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <utility>
#include <vector>

base_finger::base_finger()
	: pinch_distance_(0)
	, mouse_motions_(0)
	, pinch_noisc_time_(100)
	, last_pinch_ticks_(0)
{}

#define PINCH_SQUARE_THRESHOLD		6400
void base_finger::process_event(const SDL_Event& event)
{
	int x, y, dx, dy;
	bool hit = false;
	Uint8 mouse_flags;
	Uint32 now = SDL_GetTicks();

	unsigned screen_width2 = gui2::settings::screen_width;
	unsigned screen_height2 = gui2::settings::screen_height;
#if (defined(__APPLE__) && TARGET_OS_IPHONE)
	if (gui2::twidget::hdpi) {
		screen_width2 /= gui2::twidget::hdpi_scale;
		screen_height2 /= gui2::twidget::hdpi_scale;
	}
#endif

	switch(event.type) {
	case SDL_FINGERDOWN:
		x = event.tfinger.x * screen_width2;
		y = event.tfinger.y * screen_height2;

		// posix_print("SDL_FINGERDOWN, (%i, %i)\n", x, y);

		if (!finger_coordinate_valid(x, y)) {
			return;
		}
		fingers_.push_back(tfinger(event.tfinger.fingerId, x, y, now));
		break;

	case SDL_FINGERMOTION:
		{
			int x1 = 0, y1 = 0, x2 = 0, y2 = 0, at = 0;
			x = event.tfinger.x * screen_width2;
			y = event.tfinger.y * screen_height2;
			dx = event.tfinger.dx * screen_width2;
			dy = event.tfinger.dy * screen_height2;

			for (std::vector<tfinger>::iterator it = fingers_.begin(); it != fingers_.end(); ++ it, at ++) {
				tfinger& finger = *it;
				if (finger.fingerId == event.tfinger.fingerId) {
					finger.x = x;
					finger.y = y;
					finger.active = now;
					hit = true;
				}
				if (at == 0) {
					x1 = finger.x;
					y1 = finger.y;
				} else if (at == 1) {
					x2 = finger.x;
					y2 = finger.y;
				}
			}
			if (!hit) {
				return;
			}
			if (!finger_coordinate_valid(x, y)) {
				return;
			}
			
			if (fingers_.size() == 1) {
				handle_swipe(x, y, dx, dy);

			} else if (fingers_.size() == 2) {
				// calculate distance between finger
				int distance = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
				if (pinch_distance_ != gui2::twidget::npos) {
					int diff = pinch_distance_ - distance;
					if (abs(diff) >= PINCH_SQUARE_THRESHOLD * gui2::twidget::hdpi_scale) {
						pinch_distance_ = distance;
						if (now - last_pinch_ticks_ > pinch_noisc_time_) {
							last_pinch_ticks_ = now;
							handle_pinch(x, y, diff > 0);
						}
					}
				} else {
					pinch_distance_ = distance;
				}
			}
		}
		break;

	case SDL_FINGERUP:
		for (std::vector<tfinger>::iterator it = fingers_.begin(); it != fingers_.end(); ) {
			const tfinger& finger = *it;
			if (finger.fingerId == event.tfinger.fingerId) {
				it = fingers_.erase(it);
			} else if (now > finger.active + 5000) {
				it = fingers_.erase(it);
			} else {
				++ it;
			}
		}
		break;

	case SDL_MULTIGESTURE:
		// Now I don't use SDL logic, process multi-finger myself. Ignore it.
		break;

	case SDL_MOUSEBUTTONDOWN:
		mouse_motions_ = 0;
		pinch_distance_ = gui2::twidget::npos;

		// posix_print("SDL_MOUSEBUTTONDOWN, (%i, %i)\n", event.button.x, event.button.y);
		handle_mouse_down(event.button);
		break;

	case SDL_MOUSEBUTTONUP:
		// 1. once one finger up, thank this finger end.
		// 2. solve mouse_up nest.
		fingers_.clear();

		handle_mouse_up(event.button);
		break;

	case SDL_MOUSEMOTION:
		mouse_motions_ ++;
		handle_mouse_motion(event.motion);
		break;

	case SDL_MOUSEWHEEL:
		if (event.wheel.which == SDL_TOUCH_MOUSEID) {
			break;
		}
		mouse_flags = SDL_GetMouseState(&x, &y);
		if (!mouse_wheel_coordinate_valid(x, y)) {
			return;
		}
#ifdef _WIN32
		if (mouse_flags & SDL_BUTTON(SDL_BUTTON_LEFT) && abs(event.wheel.y) >= MOUSE_MOTION_THRESHOLD) {
			// left mouse + wheel vetical ==> pinch
			mouse_motions_ ++;
			Uint32 now = SDL_GetTicks();
			if (now - last_pinch_ticks_ > pinch_noisc_time_) {
				last_pinch_ticks_ = now;
				handle_pinch(x, y, event.wheel.y > 0);
			}
			
		} else
#endif
		{
			handle_mouse_wheel(event.wheel, x, y, mouse_flags);
		}
		break;
	}
}

bool base_finger::multi_gestures() const
{
	if (fingers_.size() < 2) {
		return false;
	}
	return true;
}

namespace events
{

bool ignore_finger_event;

struct context
{
	context() :
		handlers(),
		focused_handler(-1)
	{
	}

	void add_handler(handler* ptr);
	bool remove_handler(handler* ptr);
	int cycle_focus();
	void set_focus(const handler* ptr);

	std::vector<handler*> handlers;
	int focused_handler;

	void delete_handler_index(size_t handler);
};

void context::add_handler(handler* ptr)
{
	handlers.push_back(ptr);
}

void context::delete_handler_index(size_t handler)
{
	if(focused_handler == static_cast<int>(handler)) {
		focused_handler = -1;
	} else if(focused_handler > static_cast<int>(handler)) {
		--focused_handler;
	}

	handlers.erase(handlers.begin()+handler);
}

bool context::remove_handler(handler* ptr)
{
	if(handlers.empty()) {
		return false;
	}

	static int depth = 0;
	++depth;

	//the handler is most likely on the back of the events array,
	//so look there first, otherwise do a complete search.
	if(handlers.back() == ptr) {
		delete_handler_index(handlers.size()-1);
	} else {
		const std::vector<handler*>::iterator i = std::find(handlers.begin(),handlers.end(),ptr);
		if(i != handlers.end()) {
			delete_handler_index(i - handlers.begin());
		} else {
			return false;
		}
	}

	--depth;

	if(depth == 0) {
		cycle_focus();
	} else {
		focused_handler = -1;
	}

	return true;
}

int context::cycle_focus()
{
	int index = focused_handler+1;
	for(size_t i = 0; i != handlers.size(); ++i) {
		if(size_t(index) == handlers.size()) {
			index = 0;
		}

		if(handlers[size_t(index)]->requires_event_focus()) {
			focused_handler = index;
			break;
		}
	}

	return focused_handler;
}

void context::set_focus(const handler* ptr)
{
	const std::vector<handler*>::const_iterator i = std::find(handlers.begin(),handlers.end(),ptr);
	if(i != handlers.end() && (**i).requires_event_focus()) {
		focused_handler = int(i - handlers.begin());
	}
}

}

// this object stores all the event handlers. It is a stack of event 'contexts'.
// a new event context is created when e.g. a modal dialog is opened, and then
// closed when that dialog is closed. Each context contains a list of the handlers
// in that context. The current context is the one on the top of the stack.
// it original place at namespace events, but when vs2008 debug, cannot watch.
std::deque<events::context> event_contexts;

namespace events {

std::vector<pump_monitor*> pump_monitors;

pump_monitor::pump_monitor(bool auto_join)
	: has_joined_(false)
{
	if (auto_join) {
		join();
	}
}

pump_monitor::~pump_monitor()
{
	pump_monitors.erase(
		std::remove(pump_monitors.begin(), pump_monitors.end(), this),
		pump_monitors.end());
}

void pump_monitor::join()
{
	if (has_joined_) {
		return;
	}
	pump_monitors.push_back(this);
	has_joined_ = true;
}

event_context::event_context()
{
	event_contexts.push_back(context());
}

event_context::~event_context()
{
	assert(event_contexts.empty() == false);
	event_contexts.pop_back();
}

handler::handler(const bool auto_join) : has_joined_(false)
{
	if(auto_join) {
		assert(!event_contexts.empty());
		event_contexts.back().add_handler(this);
		has_joined_ = true;
	}
}

handler::~handler()
{
	leave();
}

void handler::join()
{
	if(has_joined_) {
		leave(); // should not be in multiple event contexts
	}
	//join self
	event_contexts.back().add_handler(this);
	has_joined_ = true;

	//instruct members to join
	handler_vector members = handler_members();
	if(!members.empty()) {
		for(handler_vector::iterator i = members.begin(); i != members.end(); ++i) {
			(*i)->join();
		}
	}
}

void handler::leave()
{
	handler_vector members = handler_members();
	if(!members.empty()) {
		for(handler_vector::iterator i = members.begin(); i != members.end(); ++i) {
			(*i)->leave();
		}
	} else {
		assert(event_contexts.empty() == false);
	}
	for(std::deque<context>::reverse_iterator i = event_contexts.rbegin(); i != event_contexts.rend(); ++i) {
		if(i->remove_handler(this)) {
			break;
		}
	}
	has_joined_ = false;
}

void focus_handler(const handler* ptr)
{
	if(event_contexts.empty() == false) {
		event_contexts.back().set_focus(ptr);
	}
}

bool has_focus(const handler* hand, const SDL_Event* event)
{
	if(event_contexts.empty()) {
		return true;
	}

	if(hand->requires_event_focus(event) == false) {
		return true;
	}

	const int foc_i = event_contexts.back().focused_handler;

	//if no-one has focus at the moment, this handler obviously wants
	//focus, so give it to it.
	if(foc_i == -1) {
		focus_handler(hand);
		return true;
	}

	handler *const foc_hand = event_contexts.back().handlers[foc_i];
	if(foc_hand == hand){
		return true;
	} else if(!foc_hand->requires_event_focus(event)) {
		//if the currently focused handler doesn't need focus for this event
		//allow the most recent interested handler to take care of it
		int back_i = event_contexts.back().handlers.size() - 1;
		for(int i=back_i; i>=0; --i) {
			handler *const thief_hand = event_contexts.back().handlers[i];
			if(i != foc_i && thief_hand->requires_event_focus(event)) {
				//steal focus
				focus_handler(thief_hand);
				if(foc_i < back_i) {
					//position the previously focused handler to allow stealing back
					event_contexts.back().delete_handler_index(foc_i);
					event_contexts.back().add_handler(foc_hand);
				}
				return thief_hand == hand;
			}
		}
	}
	return false;
}

void dump_events(const std::vector<SDL_Event>& events)
{
	std::map<int, int> dump;
	for (std::vector<SDL_Event>::const_iterator it = events.begin(); it != events.end(); ++ it) {
		int type = it->type;

		std::map<int, int>::iterator find = dump.find(type);
		if (dump.find(type) != dump.end()) {
			find->second ++;
		} else {
			dump.insert(std::make_pair(type, 1));
		}
	}
	std::stringstream ss;
	for (std::map<int, int>::const_iterator it = dump.begin(); it != dump.end(); ++ it) {
		if (it != dump.begin()) {
			ss << "; ";
		}
		ss << "(type: " << it->first << ", count: " << it->second << ")";
	}
	posix_print("%s\n", ss.str().c_str());
}

void pump()
{
	if (instance->terminating()) {
		// let main thread throw quit exception.
		throw CVideo::quit();
	}

	SDL_PumpEvents();

	SDL_Event temp_event;
	int poll_count = 0;
	int begin_ignoring = 0;
	ignore_finger_event = false;

	std::vector<SDL_Event> events;
	// ignore user input events when receive SDL_WINDOWEVENT. include before and after.
	while (SDL_PollEvent(&temp_event)) {
		++ poll_count;
		if (!begin_ignoring && temp_event.type == SDL_WINDOWEVENT) {
			begin_ignoring = poll_count;
		} else if (begin_ignoring > 0 && temp_event.type >= INPUT_MASK_MIN && temp_event.type <= INPUT_MASK_MAX) {
			//ignore user input events that occurred after the window was activated
			continue;
		}
		events.push_back(temp_event);
	}

	if (events.size() > 10) {
		posix_print("------waring!! events.size(): %u, last_event: %i\n", events.size(), events.back().type);
		dump_events(events);
	}

	std::vector<SDL_Event>::iterator ev_it = events.begin();
	for (int i = 1; i < begin_ignoring; ++i) {
		if (ev_it->type >= INPUT_MASK_MIN && ev_it->type <= INPUT_MASK_MAX) {
			//ignore user input events that occurred before the window was activated
			ev_it = events.erase(ev_it);
		} else {
			++ev_it;
		}
	}

	std::vector<SDL_Event>::iterator ev_end = events.end();
	for (ev_it = events.begin(); ev_it != ev_end; ++ev_it){
		SDL_Event& event = *ev_it;
		switch (event.type) {
			case SDL_APP_TERMINATING:
			case SDL_APP_WILLENTERBACKGROUND:
			case SDL_APP_DIDENTERBACKGROUND:
			case SDL_APP_WILLENTERFOREGROUND:
			case SDL_APP_DIDENTERFOREGROUND:
			case SDL_QUIT:
				// instance->handle_app_event(event.type);
				VALIDATE(false, "this event should be processed by SDL_SetAppEventHandler.");
				break;

			case SDL_APP_LOWMEMORY:
				instance->handle_app_event(event.type);
				break;

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {

				} else if (event.window.event == SDL_WINDOWEVENT_ENTER || event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
					cursor::set_focus(true);

				} else if (event.window.event == SDL_WINDOWEVENT_LEAVE || event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					cursor::set_focus(false);

				} else if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
					// if the window must be redrawn, update the entire screen

				} else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					
				}
				break;

			case SDL_MOUSEMOTION: {
				//always make sure a cursor is displayed if the
				//mouse moves or if the user clicks
				cursor::set_focus(true);
				break;
			}

			case SDL_MOUSEBUTTONDOWN: {
				//always make sure a cursor is displayed if the
				//mouse moves or if the user clicks
				cursor::set_focus(true);
				if (event.button.button == SDL_BUTTON_LEFT && event.button.clicks == 2) {
					SDL_UserEvent user_event;
					user_event.type = DOUBLE_CLICK_EVENT;
					user_event.code = 0;
					user_event.data1 = reinterpret_cast<void*>(event.button.x);
					user_event.data2 = reinterpret_cast<void*>(event.button.y);
					::SDL_PushEvent(reinterpret_cast<SDL_Event*>(&user_event));
				}
				break;
			}

#if defined(_X11) && !defined(__APPLE__)
			case SDL_SYSWMEVENT: {
				//clipboard support for X11
				handle_system_event(event);
				break;
			}
#endif
		}

		if(event_contexts.empty() == false) {

			const std::vector<handler*>& event_handlers = event_contexts.back().handlers;

			std::deque<context>& event_contexts_b = event_contexts;
			//events may cause more event handlers to be added and/or removed,
			//so we must use indexes instead of iterators here.
			for (size_t i1 = 0, i2 = event_handlers.size(); i1 != i2 && i1 < event_handlers.size(); ++i1) {
				event_handlers[i1]->handle_event(event);
				if (display::require_change_resolution) {
					display* disp = display::get_singleton();
					disp->change_resolution();
				}
			}
		}
	}

	//inform the pump monitors that an events::pump() has occurred
	for (size_t i1 = 0, i2 = pump_monitors.size(); i1 != i2 && i1 < pump_monitors.size(); ++i1) {
		pump_monitors[i1]->monitor_process();
	}
}

void raise_process_event()
{
	if(event_contexts.empty() == false) {

		const std::vector<handler*>& event_handlers = event_contexts.back().handlers;

		//events may cause more event handlers to be added and/or removed,
		//so we must use indexes instead of iterators here.
		for(size_t i1 = 0, i2 = event_handlers.size(); i1 != i2 && i1 < event_handlers.size(); ++i1) {
			event_handlers[i1]->process_event();
		}
	}
}

void raise_draw_event()
{
	if (event_contexts.empty() == false) {
		const std::vector<handler*>& event_handlers = event_contexts.back().handlers;

		// events may cause more event handlers to be added and/or removed,
		// so we must use indexes instead of iterators here.
		for (size_t i1 = 0, i2 = event_handlers.size(); i1 != i2 && i1 < event_handlers.size(); ++i1) {
			event_handlers[i1]->draw();
		}
	}
}

int discard(Uint32 event_mask_min, Uint32 event_mask_max)
{
	int discard_count = 0;
	SDL_Event temp_event;
	std::vector< SDL_Event > keepers;
	SDL_Delay(10);
	while (SDL_PollEvent(&temp_event) > 0) {
		if (temp_event.type >= event_mask_min && temp_event.type <= event_mask_max) {
			keepers.push_back( temp_event );
		} else {
			++ discard_count;
		}
	}

	//FIXME: there is a chance new events are added before kept events are replaced
	for (unsigned int i = 0; i < keepers.size(); ++i) {
		if (SDL_PushEvent(&keepers[i]) <= 0) {
			posix_print("failed to return an event to the queue.");
		}
	}

	return discard_count;
}

} //end events namespace
