/* $Id: handler.cpp 54604 2012-07-07 00:49:45Z loonycyborg $ */
/*
   Copyright (C) 2009 - 2012 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/auxiliary/event/handler.hpp"

#include "clipboard.hpp"
#include "gui/auxiliary/event/dispatcher.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/auxiliary/log.hpp"
#include "gui/widgets/helper.hpp"
#include "gui/widgets/widget.hpp"
#include "gui/widgets/window.hpp"
#include "hotkeys.hpp"
#include "video.hpp"
#include "display.hpp"
#include "gui/widgets/settings.hpp"
#include "preferences.hpp"
#include "wml_exception.hpp"
#include "posix.h"

#include <boost/foreach.hpp>

#include <cassert>

/**
 * @todo The items below are not implemented yet.
 *
 * - Tooltips have a fixed short time until showing up.
 * - Tooltips are shown until the widget is exited.
 * - Help messages aren't shown yet.
 *
 * @note it might be that tooltips will be shown independent of a window and in
 * their own window, therefore the code will be cleaned up after that has been
 * determined.
 */

namespace gui2 {

tpoint revise_screen_size(const int width, const int height)
{
	tpoint landscape_size = twidget::orientation_swap_size(width, height);
	if (landscape_size.x < preferences::min_allowed_width()) {
		landscape_size.x = preferences::min_allowed_width();
	}
	if (landscape_size.y < preferences::min_allowed_height()) {
		landscape_size.y = preferences::min_allowed_height();
	}

	tpoint normal_size = twidget::orientation_swap_size(landscape_size.x, landscape_size.y);
	return normal_size;
}

namespace event {

/***** Static data. *****/
class thandler;
static thandler* handler = NULL;
static events::event_context* event_context = NULL;

/***** thandler class. *****/

/**
 * This singleton class handles all events.
 *
 * It's a new experimental class.
 */
class thandler: public events::handler, public base_finger
{
	friend bool gui2::is_in_dialog();
	friend void gui2::async_draw();
	friend void gui2::absolute_draw();
	friend std::vector<twindow*> gui2::connectd_window();
public:
	thandler();

	~thandler();

	/** Inherited from events::handler. */
	void handle_event(const SDL_Event& event);

	/**
	 * Connects a dispatcher.
	 *
	 * @param dispatcher              The dispatcher to connect.
	 */
	void connect(tdispatcher* dispatcher);

	/**
	 * Disconnects a dispatcher.
	 *
	 * @param dispatcher              The dispatcher to disconnect.
	 */
	void disconnect(tdispatcher* dispatcher);

	/** The dispatcher that captured the mouse focus. */
	tdispatcher* mouse_focus;

private:
	// override base_finger
	void handle_swipe(int x, int y, int dx, int dy);
	void handle_mouse_down(const SDL_MouseButtonEvent& button);
	void handle_mouse_up(const SDL_MouseButtonEvent& button);
	void handle_mouse_motion(const SDL_MouseMotionEvent& motion);
	void handle_mouse_wheel(const SDL_MouseWheelEvent& wheel, int x, int y, Uint8 mouse_flags);

	/**
	 * Reinitializes the state of all dispatchers.
	 *
	 * This is needed when the application gets activated, to make sure the
	 * state of mainly the mouse is set properly.
	 */
	void activate();

	/***** Handlers *****/

	/** Fires a draw event. */
	void draw(const bool force);

	/**
	 * Fires a video resize event.
	 *
	 * @param new_size               The new size of the window.
	 */
	void video_resize(const tpoint& new_size);

	/**
	 * Fires a generic mouse event.
	 *
	 * @param event                  The event to fire.
	 * @param position               The position of the mouse.
	 */
	void mouse(const tevent event, const tpoint& position);

	/**
	 * Fires a mouse button up event.
	 *
	 * @param position               The position of the mouse.
	 * @param button                 The SDL id of the button that caused the
	 *                               event.
	 */
	void mouse_button_up(const tpoint& position, const Uint8 button);

	/**
	 * Fires a mouse button down event.
	 *
	 * @param position               The position of the mouse.
	 * @param button                 The SDL id of the button that caused the
	 *                               event.
	 */
	void mouse_button_down(const tpoint& position, const Uint8 button);

	/**
	 * Gets the dispatcher that wants to receive the keyboard input.
	 *
	 * @returns                   The dispatcher.
	 * @retval NULL               No dispatcher found.
	 */
	tdispatcher* keyboard_dispatcher();

	/**
	 * Handles a hat motion event.
	 *
	 * @param event                  The SDL joystick hat event triggered.
	 */
	void hat_motion(const SDL_JoyHatEvent& event);


	/**
	 * Handles a joystick button down event.
	 *
	 * @param event                  The SDL joystick button event triggered.
	 */
	void button_down(const SDL_JoyButtonEvent& event);


	/**
	 * Fires a key down event.
	 *
	 * @param event                  The SDL keyboard event triggered.
	 */
	void key_down(const SDL_KeyboardEvent& event);

	/**
	 * Fires a text input event.
	 *
	 * @param event                  The SDL textinput event triggered.
	 */
	void text_input(const SDL_TextInputEvent& event);

	/**
	 * Handles the pressing of a hotkey.
	 *
	 * @param key                 The hotkey item pressed.
	 *
	 * @returns                   True if the hotkey is handled false otherwise.
	 */
	bool hotkey_pressed(const hotkey::hotkey_item& key);

	/**
	 * Fires a key down event.
	 *
	 * @param key                    The SDL key code of the key pressed.
	 * @param modifier               The SDL key modifiers used.
	 * @param unicode                The unicode value for the key pressed.
	 */
	void key_down(const SDLKey key
			, const SDLMod modifier
			, const Uint16 unicode);

	/**
	 * Fires a keyboard event which has no parameters.
	 *
	 * This can happen for example when the mouse wheel is used.
	 *
	 * @param event                  The event to fire.
	 */
	void keyboard(const tevent event);

private:
	/**
	 * The dispatchers.
	 *
	 * The order of the items in the list is also the z-order the front item
	 * being the one completely in the background and the back item the one
	 * completely in the foreground.
	 */
	std::vector<tdispatcher*> dispatchers_;

	/**
	 * Needed to determine which dispatcher gets the keyboard events.
	 *
	 * NOTE the keyboard events aren't really wired in yet so doesn't do much.
	 */
	tdispatcher* keyboard_focus_;
	friend void capture_keyboard(tdispatcher*);
};

thandler::thandler()
	: events::handler(false)
	, mouse_focus(NULL)
	, dispatchers_()
	, keyboard_focus_(NULL)
{
	if (SDL_WasInit(SDL_INIT_TIMER) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_TIMER) == -1) {
			assert(false);
		}
	}
}

thandler::~thandler()
{
}

void thandler::handle_swipe(int x, int y, int dx, int dy)
{
	int abs_dx = abs(dx);
	int abs_dy = abs(dy);
	if (abs_dx <= FINGER_HIT_THRESHOLD && abs_dy <= FINGER_HIT_THRESHOLD) {
		return;
	}

	if (abs_dx >= abs_dy && abs_dx >= FINGER_MOTION_THRESHOLD) {
		// x axis
		if (dx > 0) {
			mouse(SDL_WHEEL_LEFT, tpoint(x, y));
		} else {
			mouse(SDL_WHEEL_RIGHT, tpoint(x, y));
		}

	} else if (abs_dx < abs_dy && abs_dy >= FINGER_MOTION_THRESHOLD) {
		// y axis
		if (dy > 0) {
			mouse(SDL_WHEEL_UP, tpoint(x, y));
		} else {
			mouse(SDL_WHEEL_DOWN, tpoint(x, y));
		}
	}
}

void thandler::handle_mouse_down(const SDL_MouseButtonEvent& button)
{
	mouse_button_down(tpoint(button.x, button.y), button.button);
}

void thandler::handle_mouse_up(const SDL_MouseButtonEvent& button)
{
	if (!multi_gestures()) {
		mouse_button_up(tpoint(button.x, button.y), button.button);
	} else {
		mouse_button_up(tpoint(twidget::npos, twidget::npos), button.button);
	}
}

void thandler::handle_mouse_motion(const SDL_MouseMotionEvent& motion)
{
	if (!multi_gestures()) {
		mouse(SDL_MOUSE_MOTION, tpoint(motion.x, motion.y));
	} else {
		mouse(SDL_MOUSE_MOTION, tpoint(twidget::npos, twidget::npos));
	}
}

void thandler::handle_mouse_wheel(const SDL_MouseWheelEvent& wheel, int x, int y, Uint8 mouse_flags)
{
	int abs_dx, abs_dy;

	abs_dx = abs(wheel.x);
	abs_dy = abs(wheel.y);
	if (abs_dx <= MOUSE_HIT_THRESHOLD && abs_dy <= MOUSE_HIT_THRESHOLD) {
		return;
	}
	mouse(SDL_MOUSE_MOTION, tpoint(x, y));
	if (abs_dx >= abs_dy && abs_dx >= MOUSE_MOTION_THRESHOLD) {
		// x axis
		if (wheel.x > 0) {
			mouse(SDL_WHEEL_LEFT, tpoint(x, y));
		} else {
			mouse(SDL_WHEEL_RIGHT, tpoint(x, y));
		}
	} else if (abs_dx < abs_dy && abs_dy >= MOUSE_MOTION_THRESHOLD) {
		// y axis
		if (wheel.y > 0) {
			mouse(SDL_WHEEL_UP, tpoint(x, y));
		} else {
			mouse(SDL_WHEEL_DOWN, tpoint(x, y));
		}
	}
}

void thandler::handle_event(const SDL_Event& event)
{
	/** No dispatchers drop the event. */
	if (dispatchers_.empty()) {
		return;
	}

	int x = 0, y = 0;

	switch(event.type) {
	case SDL_FINGERDOWN:
	case SDL_FINGERMOTION:
	case SDL_FINGERUP:
	case SDL_MULTIGESTURE:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEMOTION:
	case SDL_MOUSEWHEEL:
		base_finger::process_event(event);
		break;

	case SHOW_HELPTIP_EVENT:
		mouse(SHOW_HELPTIP, get_mouse_position());
		break;

	case HOVER_REMOVE_POPUP_EVENT:
//			remove_popup();
		break;

	case TIMER_EVENT:
		execute_timer(reinterpret_cast<long>(event.user.data1));
		break;

	case CLOSE_WINDOW_EVENT:
		{
			/** @todo Convert this to a proper new style event. */
			DBG_GUI_E << "Firing " << CLOSE_WINDOW << ".\n";

			twindow* window = twindow::window_instance(event.user.code);
			if(window) {
				window->set_retval(twindow::AUTO_CLOSE);
			}
		}
		break;

	case SDL_JOYBUTTONDOWN:
		button_down(event.jbutton);
		break;

	case SDL_JOYBUTTONUP:
		break;

	case SDL_JOYAXISMOTION:
		break;

	case SDL_JOYHATMOTION:
		hat_motion(event.jhat);
		break;

	case SDL_KEYDOWN:
		key_down(event.key);
		break;

	case SDL_TEXTINPUT:
        text_input(event.text);
		break;

	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
			Uint8 mouse_flags = SDL_GetMouseState(&x, &y);
			if (mouse_flags & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				// mouse had leave window, simulate mouse motion and mouse up.
				mouse(SDL_MOUSE_MOTION, tpoint(twidget::npos, twidget::npos));
				mouse_button_up(tpoint(twidget::npos, twidget::npos), SDL_BUTTON_LEFT);
			}

		} else if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
			// draw(true);

		} else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
			posix_print("SDL_WINDOWEVENT_RESIZED, %ix%i\n", event.window.data1, event.window.data2);
			video_resize(revise_screen_size(event.window.data1, event.window.data2));

		} else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED || event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
			activate();
		}
		break;

#if defined(_X11) && !defined(__APPLE__)
	case SDL_SYSWMEVENT: {
		DBG_GUI_E << "Event: System event.\n";
		//clipboard support for X11
		handle_system_event(event);
		break;
	}
#endif

	// Silently ignored events.
	case SDL_KEYUP:
	case DOUBLE_CLICK_EVENT:
		break;

	default:
		WRN_GUI_E << "Unhandled event "
				<< static_cast<Uint32>(event.type) << ".\n";
		break;
	}
}

void thandler::connect(tdispatcher* dispatcher)
{
	assert(std::find(dispatchers_.begin(), dispatchers_.end(), dispatcher)
			== dispatchers_.end());

	if (dispatchers_.empty()) {
		event_context = new events::event_context();
		join();
	}

	dispatchers_.push_back(dispatcher);
}

void thandler::disconnect(tdispatcher* dispatcher)
{
	/***** Validate pre conditions. *****/
	std::vector<tdispatcher*>::iterator itor =
			std::find(dispatchers_.begin(), dispatchers_.end(), dispatcher);
	assert(itor != dispatchers_.end());

	/***** Remove dispatcher. *****/
	dispatchers_.erase(itor);

	if(dispatcher == mouse_focus) {
		mouse_focus = NULL;
	}
	if(dispatcher == keyboard_focus_) {
		keyboard_focus_ = NULL;
	}

	/***** Set proper state for the other dispatchers. *****/
	BOOST_FOREACH(tdispatcher* dispatcher, dispatchers_) {
		dynamic_cast<twidget&>(*dispatcher).set_dirty();
	}

	activate();

	/***** Validate post conditions. *****/
	assert(std::find(dispatchers_.begin(), dispatchers_.end(), dispatcher)
			== dispatchers_.end());

	if (dispatchers_.empty()) {
		leave();
		delete event_context;
		event_context = NULL;
	}
}

void thandler::activate()
{
	BOOST_FOREACH(tdispatcher* dispatcher, dispatchers_) {
		dispatcher->fire2(SDL_ACTIVATE, dynamic_cast<twidget&>(*dispatcher));
	}
}

void thandler::draw(const bool force)
{
	// Don't display this event since it floods the screen
	//DBG_GUI_E << "Firing " << DRAW << ".\n";

	/*
	 * In normal draw mode the first window in not forced to be drawn the
	 * others are. So for forced mode we only need to force the first window to
	 * be drawn the others are already handled.
	 * Cannot draw topest window only, topest maybe "help" window, for example tooltip.
	 */
	bool first = !force;

	/**
	 * @todo Need to evaluate which windows really to redraw.
	 *
	 * For now we use a hack, but would be nice to rewrite it for 1.9/1.11.
	 */
	std::vector<bool> require_draw;
	int last_area = 0;
	for (std::vector<tdispatcher*>::const_reverse_iterator rit = dispatchers_.rbegin(); rit != dispatchers_.rend(); ++ rit) {
		{
			// only draw topest window.
			require_draw.insert(require_draw.begin(), require_draw.empty());
			continue;
		}

		twindow* widget = dynamic_cast<twindow*>(*rit);
		if (widget->is_theme()) {
			require_draw.insert(require_draw.begin(), false);
			continue;
		}
		int area = widget->get_width() * widget->get_height();
		require_draw.insert(require_draw.begin(), area >= last_area * 4);
		last_area = area;
	}

	std::vector<bool>::const_iterator require_draw_it = require_draw.begin();
	BOOST_FOREACH(tdispatcher* dispatcher, dispatchers_) {
		if (!*require_draw_it) {
			++ require_draw_it;
			continue;
		}
		++ require_draw_it;

		if (!first) {
			/*
			 * This leaves glitches on window borders if the window beneath it
			 * has changed, on the other hand invalidating twindown::restorer_
			 * causes black borders around the window. So there's the choice
			 * between two evils.
			 */
			dynamic_cast<twidget&>(*dispatcher).set_dirty();
		} else {
			first = false;
		}

		dispatcher->fire(DRAW, dynamic_cast<twidget&>(*dispatcher));
	}


	if (!dispatchers_.empty()) {
		display* disp = display::get_singleton();
		
		twindow* window = dynamic_cast<twindow*>(dispatchers_.back());
		CVideo& video = window->video();

		surface frame_buffer = video.getSurface();

		// tip's buff will used to other place, require remember first.
		window->draw_tooltip(frame_buffer);
		disp->draw_float_anim();
		cursor::draw(frame_buffer);

		video.flip();

		cursor::undraw(frame_buffer);
		disp->undraw_float_anim();
		window->undraw_tooltip(frame_buffer);
	}
}

void thandler::video_resize(const tpoint& new_size)
{
	posix_print("thandler::video_resize, new_size: %ix%i\n", new_size.x, new_size.y);

	if (dispatchers_.size() > 1) {
		// when tow or more, forbidden scale.
		// It is bug, require deal with in the feature.

		// I cannot change SDL_VIDEO_RESIZE runtime. replace with recovering window size.
		display* disp = display::get_singleton();
		disp->video().sdl_set_window_size(settings::screen_width, settings::screen_height);
		return;
	}

	BOOST_FOREACH(tdispatcher* dispatcher, dispatchers_) {
		dispatcher->fire(SDL_VIDEO_RESIZE, dynamic_cast<twidget&>(*dispatcher), new_size);
	}
}

void thandler::mouse(const tevent event, const tpoint& position)
{
	DBG_GUI_E << "Firing: " << event << ".\n";

	if (mouse_focus) {
		mouse_focus->fire(event, dynamic_cast<twidget&>(*mouse_focus), position);
	} else {

		for(std::vector<tdispatcher*>::reverse_iterator ritor =
				dispatchers_.rbegin(); ritor != dispatchers_.rend(); ++ritor) {

			if((**ritor).get_mouse_behaviour() == tdispatcher::all) {
				(**ritor).fire(event, dynamic_cast<twidget&>(**ritor), position);
				break;
			}
			if((**ritor).get_mouse_behaviour() == tdispatcher::none) {
				continue;
			}
			if((**ritor).is_at(position)) {
				(**ritor).fire(event, dynamic_cast<twidget&>(**ritor), position);
				break;
			}
		}
	}
}

void thandler::mouse_button_up(const tpoint& position, const Uint8 button)
{
	switch(button) {
		case SDL_BUTTON_LEFT :
			mouse(SDL_LEFT_BUTTON_UP, position);
			break;
		case SDL_BUTTON_MIDDLE :
			mouse(SDL_MIDDLE_BUTTON_UP, position);
			break;
		case SDL_BUTTON_RIGHT :
			mouse(SDL_RIGHT_BUTTON_UP, position);
			break;

		default:
			WRN_GUI_E << "Unhandled 'mouse button up' event for button "
					<< static_cast<Uint32>(button) << ".\n";
			break;
	}
}

void thandler::mouse_button_down(const tpoint& position, const Uint8 button)
{
	switch(button) {
		case SDL_BUTTON_LEFT :
			mouse(SDL_LEFT_BUTTON_DOWN, position);
			break;
		case SDL_BUTTON_MIDDLE :
			mouse(SDL_MIDDLE_BUTTON_DOWN, position);
			break;
		case SDL_BUTTON_RIGHT :
			mouse(SDL_RIGHT_BUTTON_DOWN, position);
			break;
		default:
			WRN_GUI_E << "Unhandled 'mouse button down' event for button "
					<< static_cast<Uint32>(button) << ".\n";
			break;
	}
}

tdispatcher* thandler::keyboard_dispatcher()
{
	if(keyboard_focus_) {
		return keyboard_focus_;
	}

	for(std::vector<tdispatcher*>::reverse_iterator ritor =
			dispatchers_.rbegin(); ritor != dispatchers_.rend(); ++ritor) {

		if((**ritor).get_want_keyboard_input()) {
			return *ritor;
		}
	}

	return NULL;
}

void thandler::hat_motion(const SDL_JoyHatEvent& event)
{
/*	const hotkey::hotkey_item& hk = hotkey::get_hotkey(event);
	bool done = false;
	if(!hk.null()) {
		done = hotkey_pressed(hk);
	}
	if(!done) {
		//TODO fendrin think about handling hat motions that are not bound to a hotkey.
	} */
}

void thandler::button_down(const SDL_JoyButtonEvent& event)
{
/*	const hotkey::hotkey_item& hk = hotkey::get_hotkey(event);
	bool done = false;
	if(!hk.null()) {
		done = hotkey_pressed(hk);
	}
	if(!done) {
		//TODO fendrin think about handling button down events that are not bound to a hotkey.
	} */
}

void thandler::key_down(const SDL_KeyboardEvent& event)
{
	const hotkey::hotkey_item& hk = hotkey::get_hotkey(event);
	bool done = false;
	// let return go through on android
	if (event.keysym.sym != SDLK_RETURN && !hk.null()) {
		done = hotkey_pressed(hk);
	}
	if(!done) {
		SDLMod mod = (SDLMod)event.keysym.mod;
		key_down(event.keysym.sym, mod, event.keysym.unused);
	}
}

void thandler::text_input(const SDL_TextInputEvent& event)
{
	if (tdispatcher* dispatcher = keyboard_dispatcher()) {
		dispatcher->fire(SDL_TEXT_INPUT, dynamic_cast<twidget&>(*dispatcher)	, 0, event.text);
	}
}

bool thandler::hotkey_pressed(const hotkey::hotkey_item& key)
{
	tdispatcher* dispatcher = keyboard_dispatcher();

	if(!dispatcher) {
		return false;
	}

	return dispatcher->execute_hotkey(key.get_id());
}

void thandler::key_down(const SDLKey key
		, const SDLMod modifier
		, const Uint16 unicode)
{
	DBG_GUI_E << "Firing: " << SDL_KEY_DOWN << ".\n";

	if(tdispatcher* dispatcher = keyboard_dispatcher()) {
		dispatcher->fire(SDL_KEY_DOWN, dynamic_cast<twidget&>(*dispatcher), key, modifier, unicode);
	}
}

void thandler::keyboard(const tevent event)
{
	DBG_GUI_E << "Firing: " << event << ".\n";

	if(tdispatcher* dispatcher = keyboard_dispatcher()) {
		dispatcher->fire(event, dynamic_cast<twidget&>(*dispatcher));
	}
}

/***** tmanager class. *****/

tmanager::tmanager()
{
	handler = new thandler();
}

tmanager::~tmanager()
{
	delete handler;
	handler = NULL;
}

/***** free functions class. *****/

void connect_dispatcher(tdispatcher* dispatcher)
{
	assert(handler);
	assert(dispatcher);
	handler->connect(dispatcher);
}

void disconnect_dispatcher(tdispatcher* dispatcher)
{
	assert(handler);
	assert(dispatcher);
	handler->disconnect(dispatcher);
}

void init_mouse_location()
{
	tpoint mouse = get_mouse_position();

	SDL_Event event;
	event.type = SDL_MOUSEMOTION;
	event.motion.type = SDL_MOUSEMOTION;
	event.motion.x = mouse.x;
	event.motion.y = mouse.y;

	SDL_PushEvent(&event);
}

void capture_mouse(tdispatcher* dispatcher)
{
	assert(handler);
	assert(dispatcher);
	handler->mouse_focus = dispatcher;
}

void release_mouse(tdispatcher* dispatcher)
{
	assert(handler);
	assert(dispatcher);
	if(handler->mouse_focus == dispatcher) {
		handler->mouse_focus = NULL;
	}
}

void capture_keyboard(tdispatcher* dispatcher)
{
	assert(handler);
	assert(dispatcher);
	assert(dispatcher->get_want_keyboard_input());

	handler->keyboard_focus_ = dispatcher;
}

std::ostream& operator<<(std::ostream& stream, const tevent event)
{
	switch(event) {
		case DRAW                   : stream << "draw"; break;
		case CLOSE_WINDOW           : stream << "close window"; break;
		case SDL_VIDEO_RESIZE       : stream << "SDL video resize"; break;
		case SDL_MOUSE_MOTION       : stream << "SDL mouse motion"; break;
		case MOUSE_ENTER            : stream << "mouse enter"; break;
		case MOUSE_LEAVE            : stream << "mouse leave"; break;
		case MOUSE_MOTION           : stream << "mouse motion"; break;
		case SDL_LEFT_BUTTON_DOWN   : stream << "SDL left button down"; break;
		case SDL_LEFT_BUTTON_UP     : stream << "SDL left button up"; break;
		case LEFT_BUTTON_DOWN       : stream << "left button down"; break;
		case LEFT_BUTTON_UP         : stream << "left button up"; break;
		case LEFT_BUTTON_CLICK      : stream << "left button click"; break;
		case LEFT_BUTTON_DOUBLE_CLICK
		                            : stream << "left button double click";
		                              break;
		case SDL_MIDDLE_BUTTON_DOWN : stream << "SDL middle button down"; break;
		case SDL_MIDDLE_BUTTON_UP   : stream << "SDL middle button up"; break;
		case MIDDLE_BUTTON_DOWN     : stream << "middle button down"; break;
		case MIDDLE_BUTTON_UP       : stream << "middle button up"; break;
		case MIDDLE_BUTTON_CLICK    : stream << "middle button click"; break;
		case MIDDLE_BUTTON_DOUBLE_CLICK
		                            : stream << "middle button double click";
		                              break;
		case SDL_RIGHT_BUTTON_DOWN  : stream << "SDL right button down"; break;
		case SDL_RIGHT_BUTTON_UP    : stream << "SDL right button up"; break;
		case RIGHT_BUTTON_DOWN      : stream << "right button down"; break;
		case RIGHT_BUTTON_UP        : stream << "right button up"; break;
		case RIGHT_BUTTON_CLICK     : stream << "right button click"; break;
		case RIGHT_BUTTON_DOUBLE_CLICK
		                            : stream << "right button double click";
		                              break;
		case SDL_WHEEL_LEFT         : stream << "SDL wheel left"; break;
		case SDL_WHEEL_RIGHT        : stream << "SDL wheel right"; break;
		case SDL_WHEEL_UP           : stream << "SDL wheel up"; break;
		case SDL_WHEEL_DOWN         : stream << "SDL wheel down"; break;
		case SDL_KEY_DOWN           : stream << "SDL key down"; break;

		case NOTIFY_REMOVAL         : stream << "notify removal"; break;
		case NOTIFY_MODIFIED        : stream << "notify modified"; break;
		case RECEIVE_KEYBOARD_FOCUS : stream << "receive keyboard focus"; break;
		case LOSE_KEYBOARD_FOCUS    : stream << "lose keyboard focus"; break;
		case SHOW_TOOLTIP           : stream << "show tooltip"; break;
		case NOTIFY_REMOVE_TOOLTIP  : stream << "notify remove tooltip"; break;
		case SDL_ACTIVATE           : stream << "SDL activate"; break;
		case MESSAGE_SHOW_TOOLTIP   : stream << "message show tooltip"; break;
		case SHOW_HELPTIP           : stream << "show helptip"; break;
		case REQUEST_PLACEMENT      : stream << "request placement"; break;
	}

	return stream;
}

} // namespace event

void async_draw()
{
	
	/* It is call by display::draw only.
	 * if there are tow or more dialog, in change resolution.
	 */
/*
	if (event::handler->dispatchers_.size() > 1) {
		return;
	}
*/
	bool first = true;
	BOOST_FOREACH(event::tdispatcher* dispatcher, event::handler->dispatchers_) {
		if (!first) {
			/*
			 * This leaves glitches on window borders if the window beneath it
			 * has changed, on the other hand invalidating twindown::restorer_
			 * causes black borders around the window. So there's the choice
			 * between two evils.
			 */
			dynamic_cast<twidget&>(*dispatcher).set_dirty();
		} else {
			first = false;
		}
		twindow* window = dynamic_cast<twindow*>(dispatcher);
		window->draw();
		// dispatcher->fire(DRAW, dynamic_cast<twidget&>(*dispatcher));
	}
}

void absolute_draw()
{
	event::handler->draw(false);
}

std::vector<twindow*> connectd_window()
{
	std::vector<twindow*> result;
	BOOST_FOREACH(event::tdispatcher* dispatcher, event::handler->dispatchers_) {
		twindow* window = dynamic_cast<twindow*>(dispatcher);
		result.push_back(window);
	}
	return result;
}

bool is_in_dialog()
{
	if (!event::handler || event::handler->dispatchers_.empty()) {
		return false;
	}
	if (event::handler->dispatchers_.size() == 1) {
		twindow* window = dynamic_cast<twindow*>(event::handler->dispatchers_.front());
		if (window->is_theme()) {
			return false;
		}
	}
	return true;
}

} // namespace gui2

