/* $Id: control.cpp 54604 2012-07-07 00:49:45Z loonycyborg $ */
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

#include "control.hpp"

#include "font.hpp"
#include "formula_string_utils.hpp"
#include "gui/auxiliary/iterator/walker_widget.hpp"
#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/event/message.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/auxiliary/window_builder/control.hpp"
#include "marked-up_text.hpp"
#include "display.hpp"
#include "hotkeys.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <iomanip>

#define LOG_SCOPE_HEADER "tcontrol(" + get_control_type() + ") [" \
		+ id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

bool tcontrol::force_add_to_dirty_list = false;

tcontrol::tcontrol(const unsigned canvas_count)
	: definition_("default")
	, label_()
	, label_size_(std::make_pair(0, tpoint(0, 0)))
	, text_editable_(false)
	, pre_anims_()
	, post_anims_()
	, integrate_(NULL)
	, integrate_default_color_(font::BLACK_COLOR)
	, use_tooltip_on_label_overflow_(true)
	, tooltip_()
	, canvas_(canvas_count)
	, config_(NULL)
	, text_maximum_width_(0)
	, text_alignment_(PANGO_ALIGN_LEFT)
	, shrunken_(false)
	, drag_detect_started_(false)
	, first_drag_coordinate_(0, 0)
	, last_drag_coordinate_(0, 0)
	, enable_drag_draw_coordinate_(true)
	, draw_offset_(0, 0)
	, best_width_("")
	, best_height_("")
	, hdpi_off_width_(false)
	, hdpi_off_height_(false)
{
	connect_signal<event::SHOW_TOOLTIP>(boost::bind(
			  &tcontrol::signal_handler_show_tooltip
			, this
			, _2
			, _3
			, _5));

	connect_signal<event::NOTIFY_REMOVE_TOOLTIP>(boost::bind(
			  &tcontrol::signal_handler_notify_remove_tooltip
			, this
			, _2
			, _3));
}

tcontrol::~tcontrol()
{
	while (!pre_anims_.empty()) {
		erase_animation(pre_anims_.front());
	}
	while (!post_anims_.empty()) {
		erase_animation(post_anims_.front());
	}
	if (integrate_) {
		delete integrate_;
		integrate_ = NULL;
	}
}

void tcontrol::set_members(const string_map& data)
{
	/** @todo document this feature on the wiki. */
	/** @todo do we need to add the debug colors here as well? */
	string_map::const_iterator itor = data.find("id");
	if(itor != data.end()) {
		set_id(itor->second);
	}

	itor = data.find("linked_group");
	if(itor != data.end()) {
		set_linked_group(itor->second);
	}

	itor = data.find("label");
	if(itor != data.end()) {
		set_label(itor->second);
	}

	itor = data.find("tooltip");
	if(itor != data.end()) {
		set_tooltip(itor->second);
	}

	itor = data.find("editable");
	if (itor != data.end()) {
		set_text_editable(utils::string_bool(itor->second));
	}
}

bool tcontrol::disable_click_dismiss() const
{
	return get_visible() == twidget::VISIBLE && get_active();
}

iterator::twalker_* tcontrol::create_walker()
{
	return new iterator::walker::twidget(*this);
}

tpoint tcontrol::get_config_default_size() const
{
	assert(config_);

	tpoint result(config_->default_width, config_->default_height);

	DBG_GUI_L << LOG_HEADER << " result " << result << ".\n";
	return result;
}

tpoint tcontrol::get_config_maximum_size() const
{
	assert(config_);

	tpoint result(config_->max_width, config_->max_height);
/*
	if (!result.x) {
		result.x = settings::screen_width;
	}
	if (!result.y) {
		result.y = settings::screen_height;
	}
*/
	DBG_GUI_L << LOG_HEADER << " result " << result << ".\n";
	return result;
}

unsigned tcontrol::get_characters_per_line() const
{
	return 0;
}

void tcontrol::layout_init(const bool full_initialization)
{
	// Inherited.
	twidget::layout_init(full_initialization);

	if(full_initialization) {
		shrunken_ = false;
	}
}

tpoint tcontrol::best_size_from_builder() const
{
	tpoint result(npos, npos);
	const twindow* window = NULL;
	if (best_width_.has_formula2()) {
		window = get_window();
		result.x = best_width_(window->variables());
		if (!hdpi_off_width_) {
			result.x *= twidget::hdpi_scale;
		}
	}
	if (best_height_.has_formula2()) {
		if (!window) {
			window = get_window();
		}
		result.y = best_height_(window->variables());
		if (!hdpi_off_height_) {
			result.y *= twidget::hdpi_scale;
		}
	}
	return result;
}

tpoint tcontrol::calculate_best_size() const
{
	if (id() == "login") {
		int ii = 0;
	}

	VALIDATE(config_, null_str);
	
	// if has width/height field, use them. or calculate.
	tpoint result = best_size_from_builder();
	if (result.x != npos && result.y != npos) {
		return result;
	}

	tpoint result2(npos, npos);
	if (label_.empty()) {
		result2 = get_config_default_size();
	} else {
		const tpoint minimum = get_config_default_size();
		tpoint maximum = get_config_maximum_size();
		if (!maximum.x) {
			maximum.x = settings::screen_width;
			maximum.x -= config_->text_extra_width;
		}
		if (!maximum.y) {
			maximum.y = settings::screen_height;
		}
		if (text_maximum_width_ && maximum.x > text_maximum_width_) {
			maximum.x = text_maximum_width_;
		}

		/**
			* @todo The value send should subtract the border size
			* and read it after calculation to get the proper result.
			*/
		result2 = get_best_text_size(minimum, maximum);
	}
	if (result.x == npos) {
		result.x = result2.x;
	}
	if (result.y == npos) {
		result.y = result2.y;
	}

	return result;
}

tpoint tcontrol::request_reduce_width(const unsigned maximum_width)
{
	return get_best_size();
}

void tcontrol::calculate_integrate()
{
	if (!text_editable_) {
		return;
	}
	if (integrate_) {
		delete integrate_;
	}
	int max = get_text_maximum_width();
	if (max > 0) {
		// before place, w_ = 0. it indicate not ready.
		integrate_ = new tintegrate(label_, get_text_maximum_width(), -1, config()->text_font_size, integrate_default_color_, text_editable_);
		if (!locator_.empty()) {
			integrate_->fill_locator_rect(locator_, true);
		}
	}
}

void tcontrol::set_integrate_default_color(const SDL_Color& color)
{
	integrate_default_color_ = color;
}

void tcontrol::refresh_locator_anim(std::vector<tintegrate::tlocator>& locator)
{
	if (!text_editable_) {
		return;
	}
	locator_.clear();
	if (integrate_) {
		integrate_->fill_locator_rect(locator, true);
	} else {
		locator_ = locator;
	}
}

void tcontrol::set_surface(const surface& surf, int w, int h)
{
	size_t count = canvas_.size();

	surfs_.clear();
	surfs_.resize(count);
	if (!surf || !count) {
		return;
	}

	surface normal = scale_surface(surf, w, h);
	for (size_t n = 0; n < count; n ++) {
		surfs_[n] = normal;
	}

	set_dirty();
}

void tcontrol::place(const tpoint& origin, const tpoint& size)
{
	SDL_Rect previous_rect = ::create_rect(x_, y_, w_, h_);

	// resize canvasses
	BOOST_FOREACH(tcanvas& canvas, canvas_) {
		canvas.set_width(size.x);
		canvas.set_height(size.y);
	}

	// inherited
	twidget::place(origin, size);

	calculate_integrate();

	// update the state of the canvas after the sizes have been set.
	update_canvas();

	if (callback_place_) {
		callback_place_(this, previous_rect);
	}
}

void tcontrol::load_config()
{
	if(!config()) {

		definition_load_configuration(get_control_type());

		load_config_extra();
	}
}

void tcontrol::set_definition(const std::string& definition)
{
	assert(!config());
	definition_ = definition;
	load_config();
	assert(config());
}

void tcontrol::clear_label_size_cache()
{
	label_size_.second.x = 0;
}

void tcontrol::set_best_size(const std::string& width, const std::string& height, const std::string& hdpi_off)
{
	best_width_ = tformula<unsigned>(width);
	best_height_ = tformula<unsigned>(height);
	if (!hdpi_off.empty() && (!width.empty() || !height.empty())) {
		std::vector<std::string> fields = utils::split(hdpi_off);
		if (!fields.empty()) {
			hdpi_off_width_ = utils::to_bool(fields[0]);
		}
		if (fields.size() >= 2) {
			hdpi_off_height_ = utils::to_bool(fields[1]);
		}
	}
}

void tcontrol::set_label(const std::string& label)
{
	if (label == label_) {
		return;
	}

	label_ = label;
	label_size_.second.x = 0;
	update_canvas();
	set_dirty();

	calculate_integrate();
}

void tcontrol::set_text_editable(bool editable)
{
	if (editable == text_editable_) {
		return;
	}

	text_editable_ = editable;
	update_canvas();
	set_dirty();
}

void tcontrol::set_text_alignment(const PangoAlignment text_alignment)
{
	if(text_alignment_ == text_alignment) {
		return;
	}

	text_alignment_ = text_alignment;
	update_canvas();
	set_dirty();
}

void tcontrol::update_canvas()
{
	const int max_width = get_text_maximum_width();
	const int max_height = get_text_maximum_height();

	// set label in canvases
	BOOST_FOREACH(tcanvas& canvas, canvas_) {
		canvas.set_variable("text", variant(label_));
		canvas.set_variable("text_alignment", variant(encode_text_alignment(text_alignment_)));
		canvas.set_variable("text_maximum_width", variant(max_width / twidget::hdpi_scale));
		canvas.set_variable("text_maximum_height", variant(max_height / twidget::hdpi_scale));
		canvas.set_variable("text_characters_per_line", variant(get_characters_per_line()));
	}
}

int tcontrol::get_text_maximum_width() const
{
	assert(config_);

	return get_width() - config_->text_extra_width;
}

int tcontrol::get_text_maximum_height() const
{
	assert(config_);
	return get_height() - config_->text_extra_height;
}

void tcontrol::set_text_maximum_width(int maximum)
{
	text_maximum_width_ = maximum - config_->text_extra_width;
}

bool tcontrol::exist_anim()
{
	if (!pre_anims_.empty() || !post_anims_.empty()) {
		return true;
	}
	if (integrate_ && integrate_->exist_anim()) {
		return true;
	}
	return !canvas_.empty() && canvas_[get_state()].exist_anim();
}

int tcontrol::insert_animation(const ::config& cfg, bool pre)
{
	int id = start_cycle_float_anim(*display::get_singleton(), cfg);
	if (id != INVALID_ANIM_ID) {
		std::vector<int>& anims = pre? pre_anims_: post_anims_;
		anims.push_back(id);
	}
	return id;
}

void tcontrol::erase_animation(int id)
{
	bool found = false;
	std::vector<int>::iterator find = std::find(pre_anims_.begin(), pre_anims_.end(), id);
	if (find != pre_anims_.end()) {
		pre_anims_.erase(find);
		found = true;

	} else {
		find = std::find(post_anims_.begin(), post_anims_.end(), id);
		if (find != post_anims_.end()) {
			post_anims_.erase(find);
			found = true;
		}
	}
	if (found) {
		display& disp = *display::get_singleton();
		disp.erase_area_anim(id);
		set_dirty();
	}
}

void tcontrol::set_canvas_variable(const std::string& name, const variant& value)
{
	BOOST_FOREACH(tcanvas& canvas, canvas_) {
		canvas.set_variable(name, value);
	}
	set_dirty();
}

class tshare_canvas_integrate_lock
{
public:
	tshare_canvas_integrate_lock(tintegrate* integrate)
		: integrate_(share_canvas_integrate)
	{
		share_canvas_integrate = integrate;
	}
	~tshare_canvas_integrate_lock()
	{
		share_canvas_integrate = integrate_;
	}

private:
	tintegrate* integrate_;
};

void tcontrol::impl_draw_background(
		  surface& frame_buffer
		, int x_offset
		, int y_offset)
{
	const surface* surf = NULL;
	if (!surfs_.empty()) {
		surf = &surfs_[get_state()];
	}

	tshare_canvas_image_lock lock1(surf);
	tshare_canvas_integrate_lock lock2(integrate_);

	x_offset += draw_offset_.x;
	y_offset += draw_offset_.y;

	if (id() == "status") {
		int ii = 0;
	}

	canvas(get_state()).blit(frame_buffer, calculate_blitting_rectangle(x_offset, y_offset), get_dirty(), pre_anims_, post_anims_);
}

void tcontrol::child_populate_dirty_list(twindow& caller, const std::vector<twidget*>& call_stack)
{
	if (force_add_to_dirty_list && !canvas_.empty()) {
		caller.add_to_dirty_list(call_stack);
	}
}

void tcontrol::definition_load_configuration(const std::string& control_type)
{
	assert(!config());

	set_config(get_control(control_type, definition_));

	VALIDATE(canvas().size() == config()->state.size(), null_str);
	for (size_t i = 0; i < canvas().size(); ++i) {
		canvas(i) = config()->state[i].canvas;
		canvas(i).start_animation();
	}

	update_canvas();
}

void tcontrol::control_drag_detect(bool start, int x, int y, const tdrag_direction type)
{
	if (drag_ == drag_none || start == drag_detect_started_) {
		return;
	}

	if (start) {
		// ios sequence: SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN
		// it is called at SDL_MOUSEBUTTONDOWN! SDL_MOUSEMOTION before it is discard.
		first_drag_coordinate_.x = x;
		first_drag_coordinate_.y = y;

		last_drag_coordinate_.x = x;
		last_drag_coordinate_.y = y;
	} else if (enable_drag_draw_coordinate_) {
		set_draw_offset(0, 0);
	}
	drag_detect_started_ = start;

	bool require_dirty = true;
	if (callback_control_drag_detect_) {
		require_dirty = callback_control_drag_detect_(this, start, type);
	}
	// upcaller maybe update draw_offset.
	if (require_dirty) {
		set_dirty();
	}
}

void tcontrol::set_drag_coordinate(int x, int y)
{
	if (drag_ == drag_none || !drag_detect_started_) {
		return;
	}

	if (last_drag_coordinate_.x == x && last_drag_coordinate_.y == y) {
		return;
	}

	if (drag_ != drag_track) {
		// get rid of direct noise
		if (!(drag_ & (drag_left | drag_right))) {
			// concert only vertical
			if (abs(x - last_drag_coordinate_.x) > abs(y - last_drag_coordinate_.y)) {
				// horizontal variant is more than vertical, think no.
				return;
			}

		} else if (!(drag_ & (drag_up | drag_down))) {
			// concert only horizontal
			if (abs(x - last_drag_coordinate_.x) < abs(y - last_drag_coordinate_.y)) {
				// vertical variant is more than horizontal, think no.
				return;
			}
		}
	}

	last_drag_coordinate_.x = x;
	last_drag_coordinate_.y = y;

	if (enable_drag_draw_coordinate_) {
		set_draw_offset(last_drag_coordinate_.x - first_drag_coordinate_.x, 0);
	}

	bool require_dirty = true;
	if (callback_set_drag_coordinate_) {
		require_dirty = callback_set_drag_coordinate_(this, first_drag_coordinate_, last_drag_coordinate_);
	}

	if (require_dirty) {
		set_dirty();
	}
}

int tcontrol::drag_satisfied()
{
	if (drag_ == drag_none || !drag_detect_started_) {
		return drag_none;
	}
	tdrag_direction ret = drag_none;

	const int drag_threshold_mdpi = 40;
	const int drag_threshold = hdpi? drag_threshold_mdpi * hdpi_scale: drag_threshold_mdpi;
	int w_threshold = w_ / 3;
	if (w_threshold > drag_threshold) {
		w_threshold = drag_threshold;
	}
	int h_threshold = h_ / 3;
	if (h_threshold > drag_threshold) {
		h_threshold = drag_threshold;
	}

	
	if (drag_ & drag_left) {
		if (last_drag_coordinate_.x < first_drag_coordinate_.x && first_drag_coordinate_.x - last_drag_coordinate_.x > w_threshold) {
			ret = drag_left;
		}
	} 
	if (drag_ & drag_right) {
		if (last_drag_coordinate_.x > first_drag_coordinate_.x && last_drag_coordinate_.x - first_drag_coordinate_.x > w_threshold) {
			ret = drag_right;
		}

	}
	if (drag_ & drag_up) {
		if (last_drag_coordinate_.y < first_drag_coordinate_.y && first_drag_coordinate_.y - last_drag_coordinate_.y > h_threshold) {
			ret = drag_up;
		}

	}
	if (drag_ & drag_down) {
		if (last_drag_coordinate_.y > first_drag_coordinate_.y && last_drag_coordinate_.y - first_drag_coordinate_.y > h_threshold) {
			ret = drag_down;
		}
	}

	// stop drag detect.
	control_drag_detect(false, npos, npos, ret);

	return ret;
}

void tcontrol::set_draw_offset(int x, int y) 
{
	draw_offset_.x = x; 
	draw_offset_.y = y; 
}

tpoint tcontrol::get_best_text_size(
		  const tpoint& minimum_size
		, const tpoint& maximum_size) const
{
	VALIDATE(!label_.empty(), null_str);

	if (label_.find("1.0.2") != std::string::npos) {
		int ii = 0;
	}

	if (!label_size_.second.x || (maximum_size.x != label_size_.first)) {
		// Try with the minimum wanted size.
		label_size_.first = maximum_size.x;
		label_size_.second = font::get_rendered_text_size(label_, maximum_size.x, config_->text_font_size, font::NORMAL_COLOR, text_editable_);
	}

	tpoint size = label_size_.second;

	if (size.x < minimum_size.x) {
		size.x = minimum_size.x;
	}

	if (size.y < minimum_size.y) {
		size.y = minimum_size.y;
	}
/*
	if (size.x > maximum_size.x) {
		if (label_.find("artifical.cpp:1766 in function") != std::string::npos) {
			int ii = 0;
		}
		size.x = maximum_size.x;
	}

	if (size.y > maximum_size.y) {
		if (label_.find("artifical.cpp:1766 in function") != std::string::npos) {
			int ii = 0;
		}
		size.y = maximum_size.y;
	}
*/
	const tpoint border(config_->text_extra_width, config_->text_extra_height);
	return size + border;
}

void tcontrol::signal_handler_show_tooltip(
		  const event::tevent event
		, bool& handled
		, const tpoint& location)
{
#if (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(ANDROID)
	return;
#endif

	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";
	if (!tooltip_.empty()) {
		std::string tip = tooltip_;
		event::tmessage_show_tooltip message(tip, *this, location);
		handled = fire(event::MESSAGE_SHOW_TOOLTIP, *this, message);
	}
}

void tcontrol::signal_handler_notify_remove_tooltip(
		  const event::tevent event
		, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	/*
	 * This makes the class know the tip code rather intimately. An
	 * alternative is to add a message to the window to remove the tip.
	 * Might be done later.
	 */
	get_window()->remove_tooltip();
	// tip::remove();

	handled = true;
}

} // namespace gui2

