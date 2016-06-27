/* $Id: unit_animation.cpp 47268 2010-10-29 14:37:49Z boucman $ */
/*
   Copyright (C) 2006 - 2010 by Jeremy Rosen <jeremy.rosen@enst-bretagne.fr>
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

#include "animation.hpp"

#include "display.hpp"
#include "base_unit.hpp"
#include "controller_base.hpp"
#include "halo.hpp"
#include "map.hpp"
#include "rose_config.hpp"

#include <boost/foreach.hpp>
#include <algorithm>

#include "animated.i"

template class animated< image::locator >;
template class animated< std::string >;
template class animated< unit_frame >;

animation::animation(int start_time, const unit_frame & frame, const std::string& event, const int variation, const frame_builder & builder) 
	: unit_anim_(start_time, builder)
	, sub_anims_()
	, type_(anim_map)
	, src_()
	, dst_()
	, invalidated_(false)
	, play_offscreen_(true)
	, area_mode_(false)
	, layer_(0)
	, cycles_(false)
	, started_(false)
	, overlaped_hex_()
{
	add_frame(frame.duration(), frame, !frame.does_not_change());
}

animation::animation(const config& cfg, const std::string& frame_string ) 
	: unit_anim_(cfg,frame_string)
	, sub_anims_()
	, type_(anim_map)
	, src_()
	, dst_()
	, invalidated_(false)
	, play_offscreen_(true)
	, area_mode_(false)
	, layer_(0)
	, cycles_(false)
	, started_(false)
	, overlaped_hex_()
{
	BOOST_FOREACH (const config::any_child &fr, cfg.all_children_range()) {
		if (fr.key == frame_string) continue;
		if (fr.key.find("_frame", fr.key.size() - 6) == std::string::npos) continue;
		if (sub_anims_.find(fr.key) != sub_anims_.end()) continue;
		sub_anims_[fr.key] = particular(cfg, fr.key.substr(0, fr.key.size() - 5));
	}

	play_offscreen_ = cfg["offscreen"].to_bool(true);

	area_mode_ = cfg["area_mode"].to_bool();
	layer_ = cfg["layer"].to_int(); // use to outer anination
}


void animation::particular::override(int start_time
		, int duration
		, const std::string& highlight
		, const std::string& blend_ratio
		, Uint32 blend_color
		, const std::string& offset
		, const std::string& layer
		, const std::string& modifiers)
{
	set_begin_time(start_time);
	parameters_.override(duration,highlight,blend_ratio,blend_color,offset,layer,modifiers);

	if (get_animation_duration() < duration) {
		const unit_frame & last_frame = get_last_frame();
		add_frame(duration -get_animation_duration(), last_frame);
	} else if(get_animation_duration() > duration) {
		set_end_time(duration);
	}

}

bool animation::particular::need_update() const
{
	if (animated<unit_frame>::need_update()) return true;
	if (get_current_frame().need_update()) return true;
	if (parameters_.need_update()) return true;
	return false;
}

bool animation::particular::need_minimal_update() const
{
	if (get_current_frame_begin_time() != last_frame_begin_time_ ) {
		return true;
	}
	return false;
}

animation::particular::particular(
	const config& cfg, const std::string& frame_string ) :
		animated<unit_frame>(),
		accelerate(true),
		cycles(false),
		parameters_(),
		halo_id_(0),
		last_frame_begin_time_(0)
{
	config::const_child_itors range = cfg.child_range(frame_string+"frame");
	starting_frame_time_=INT_MAX;
	if(cfg[frame_string+"start_time"].empty() &&range.first != range.second) {
		BOOST_FOREACH (const config &frame, range) {
			starting_frame_time_ = std::min(starting_frame_time_, frame["begin"].to_int());
		}
	} else {
		starting_frame_time_ = cfg[frame_string+"start_time"];
	}

	cycles = cfg[frame_string + "cycles"].to_bool();

	BOOST_FOREACH (const config &frame, range)
	{
		unit_frame tmp_frame(frame);
		add_frame(tmp_frame.duration(),tmp_frame,!tmp_frame.does_not_change());
	}
	parameters_ = frame_parsed_parameters(frame_builder(cfg,frame_string),get_animation_duration());
	if(!parameters_.does_not_change()  ) {
			force_change();
	}
}

bool animation::need_update() const
{
	if(unit_anim_.need_update()) return true;
	std::map<std::string,particular>::const_iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		if(anim_itor->second.need_update()) return true;
	}
	return false;
}

bool animation::need_minimal_update() const
{
	if (!play_offscreen_) {
		return false;
	}
	if(unit_anim_.need_minimal_update()) return true;
	std::map<std::string,particular>::const_iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		if(anim_itor->second.need_minimal_update()) return true;
	}
	return false;
}

bool animation::animation_finished() const
{
	if(!unit_anim_.animation_finished()) return false;
	std::map<std::string,particular>::const_iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		if(!anim_itor->second.animation_finished()) return false;
	}
	return true;
}

bool animation::animation_finished_potential() const
{
	if(!unit_anim_.animation_finished_potential()) return false;
	std::map<std::string,particular>::const_iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		if(!anim_itor->second.animation_finished_potential()) return false;
	}
	return true;
}

void animation::update_last_draw_time()
{
	double acceleration = unit_anim_.accelerate ? display::get_singleton()->turbo_speed() : 1.0;
	unit_anim_.update_last_draw_time(acceleration);
	std::map<std::string,particular>::iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		anim_itor->second.update_last_draw_time(acceleration);
	}
}

int animation::get_end_time() const
{
	int result = unit_anim_.get_end_time();
	std::map<std::string,particular>::const_iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		result= std::max<int>(result,anim_itor->second.get_end_time());
	}
	return result;
}

int animation::get_begin_time() const
{
	int result = unit_anim_.get_begin_time();
	std::map<std::string,particular>::const_iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		result= std::min<int>(result,anim_itor->second.get_begin_time());
	}
	return result;
}

void animation::start_animation(int start_time
		, const map_location &src
		, const map_location &dst
		, bool cycles
		, const std::string& text
		, const Uint32 text_color
		, const bool accelerate)
{
	started_ = true;
	unit_anim_.accelerate = accelerate;
	src_ = src;
	dst_ = dst;
	unit_anim_.start_animation(start_time, cycles || unit_anim_.cycles);
	cycles_ = cycles || unit_anim_.cycles;
	if(!text.empty()) {
		particular crude_build;
		crude_build.add_frame(1,frame_builder());
		crude_build.add_frame(1,frame_builder().text(text,text_color),true);
		sub_anims_["_add_text"] = crude_build;
	}
	std::map<std::string,particular>::iterator anim_itor =sub_anims_.begin();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		anim_itor->second.accelerate = accelerate;
		anim_itor->second.start_animation(start_time, cycles || anim_itor->second.cycles);
		cycles_ = cycles_ || anim_itor->second.cycles;
	}
}

void animation::update_parameters(const map_location &src, const map_location &dst)
{
	src_ = src;
	dst_ = dst;
}
void animation::pause_animation()
{

	std::map<std::string,particular>::iterator anim_itor =sub_anims_.begin();
	unit_anim_.pause_animation();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		anim_itor->second.pause_animation();
	}
}
void animation::restart_animation()
{

	std::map<std::string,particular>::iterator anim_itor =sub_anims_.begin();
	unit_anim_.restart_animation();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		anim_itor->second.restart_animation();
	}
}
void animation::redraw(frame_parameters& value)
{

	invalidated_=false;
	overlaped_hex_.clear();
	std::map<std::string,particular>::iterator anim_itor =sub_anims_.begin();
	value.primary_frame = t_true;
	unit_anim_.redraw(value,src_,dst_);
	value.primary_frame = t_false;
	for ( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		anim_itor->second.redraw( value,src_,dst_);
	}
}

void animation::redraw(surface&, const SDL_Rect&)
{
	frame_parameters params;
	params.area_mode = area_mode_;
	redraw(params);
}

void animation::require_release()
{
	if (callback_finish_) {
		callback_finish_(this);
	}
}

void animation::replace_image_name(const std::string& part, const std::string& src, const std::string& dst)
{
	if (part.empty()) {
		unit_anim_.replace_image_name(src, dst);
	} else {
		std::map<std::string, particular>::iterator it = sub_anims_.find(part + "_frame");
		if (it != sub_anims_.end()) {
			it->second.replace_image_name(src, dst);
		}
	}
}

void animation::replace_image_mod(const std::string& part, const std::string& src, const std::string& dst)
{
	if (part.empty()) {
		unit_anim_.replace_image_mod(src, dst);
	} else {
		std::map<std::string, particular>::iterator it = sub_anims_.find(part + "_frame");
		if (it != sub_anims_.end()) {
			it->second.replace_image_mod(src, dst);
		}
	}
}

void animation::replace_progressive(const std::string& part, const std::string& name, const std::string& src, const std::string& dst)
{
	if (part.empty()) {
		unit_anim_.replace_progressive(name, src, dst);
	} else {
		std::map<std::string, particular>::iterator it = sub_anims_.find(part + "_frame");
		if (it != sub_anims_.end()) {
			it->second.replace_progressive(name, src, dst);
		}
	}
}

void animation::replace_static_text(const std::string& part, const std::string& src, const std::string& dst)
{
	if (part.empty()) {
		unit_anim_.replace_static_text(src, dst);
	} else {
		std::map<std::string, particular>::iterator it = sub_anims_.find(part + "_frame");
		if (it != sub_anims_.end()) {
			it->second.replace_static_text(src, dst);
		}
	}
}

void animation::replace_int(const std::string& part, const std::string& name, int src, int dst)
{
	if (part.empty()) {
		unit_anim_.replace_int(name, src, dst);
	} else {
		std::map<std::string, particular>::iterator it = sub_anims_.find(part + "_frame");
		if (it != sub_anims_.end()) {
			it->second.replace_int(name, src, dst);
		}
	}
}

void animation::clear_haloes()
{

	std::map<std::string,particular>::iterator anim_itor = sub_anims_.begin();
	unit_anim_.clear_halo();
	for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
		anim_itor->second.clear_halo();
	}
}
bool animation::invalidate(const surface&, frame_parameters& value)
{
	if(invalidated_) return false;
	display* disp = display::get_singleton();
	bool complete_redraw =disp->tile_nearly_on_screen(src_) || disp->tile_nearly_on_screen(dst_) || !src_.valid();
	if (overlaped_hex_.empty()) {
		if (complete_redraw) {
			std::map<std::string,particular>::iterator anim_itor =sub_anims_.begin();
			value.primary_frame = t_true;
			overlaped_hex_ = unit_anim_.get_overlaped_hex(value,src_,dst_);
			value.primary_frame = t_false;
			for( /*null*/; anim_itor != sub_anims_.end() ; ++anim_itor) {
				std::set<map_location> tmp = anim_itor->second.get_overlaped_hex(value,src_,dst_);
				overlaped_hex_.insert(tmp.begin(),tmp.end());
			}
		} else {
			// off screen animations only invalidate their own hex, no propagation,
			// but we stil need this to play sounds
			overlaped_hex_.insert(src_);
		}

	}
	if (complete_redraw) {
		if( need_update()) {
			disp->invalidate(overlaped_hex_);
			invalidated_ = true;
			return true;
		} else {
			invalidated_ = disp->propagate_invalidation(overlaped_hex_);
			return invalidated_;
		}
	} else {
		if (need_minimal_update()) {
			disp->invalidate(overlaped_hex_);
			invalidated_ = true;
			return true;
		} else {
			return false;
		}
	}
}

bool animation::invalidate(const surface& screen)
{
	frame_parameters params;
	params.area_mode = area_mode_;
	return invalidate(screen, params);
}

void animation::particular::redraw(const frame_parameters& value,const map_location &src, const map_location &dst)
{
	const unit_frame& current_frame= get_current_frame();
	const frame_parameters default_val = parameters_.parameters(get_animation_time() -get_begin_time());
	if(get_current_frame_begin_time() != last_frame_begin_time_ ) {
		last_frame_begin_time_ = get_current_frame_begin_time();
		current_frame.redraw(get_current_frame_time(),true,src,dst,&halo_id_,default_val,value);
	} else {
		current_frame.redraw(get_current_frame_time(),false,src,dst,&halo_id_,default_val,value);
	}
}

void animation::particular::replace_image_name(const std::string& src, const std::string& dst)
{
	for (std::vector<frame>::iterator fr = frames_.begin(); fr != frames_.end(); ++ fr) {
		unit_frame& T = fr->value_;
		T.replace_image_name(src, dst);
	}
}

void animation::particular::replace_image_mod(const std::string& src, const std::string& dst)
{
	for (std::vector<frame>::iterator fr = frames_.begin(); fr != frames_.end(); ++ fr) {
		unit_frame& T = fr->value_;
		T.replace_image_mod(src, dst);
	}
}

void animation::particular::replace_progressive(const std::string& name, const std::string& src, const std::string& dst)
{
	does_not_change_ = true;
	for (std::vector<frame>::iterator fr = frames_.begin(); fr != frames_.end(); ++ fr) {
		unit_frame& T = fr->value_;
		T.replace_progressive(name, src, dst);
		if (does_not_change_) {
			does_not_change_ = T.does_not_change();
		}
	}
}

void animation::particular::replace_static_text(const std::string& src, const std::string& dst)
{
	for (std::vector<frame>::iterator fr = frames_.begin(); fr != frames_.end(); ++ fr) {
		unit_frame& T = fr->value_;
		T.replace_static_text(src, dst);
	}
}

void animation::particular::replace_int(const std::string& name, int src, int dst)
{
	for (std::vector<frame>::iterator fr = frames_.begin(); fr != frames_.end(); ++ fr) {
		unit_frame& T = fr->value_;
		T.replace_int(name, src, dst);
	}
}

void animation::particular::clear_halo()
{
	if(halo_id_ != halo::NO_HALO) {
		halo::remove(halo_id_);
		halo_id_ = halo::NO_HALO;
	}
}
std::set<map_location> animation::particular::get_overlaped_hex(const frame_parameters& value,const map_location &src, const map_location &dst)
{
	const unit_frame& current_frame= get_current_frame();
	const frame_parameters default_val = parameters_.parameters(get_animation_time() -get_begin_time());
	return current_frame.get_overlaped_hex(get_current_frame_time(), src, dst, default_val, value);
}

std::vector<SDL_Rect> animation::particular::get_overlaped_rect(const frame_parameters& value,const map_location &src, const map_location &dst)
{
	const unit_frame& current_frame = get_current_frame();
	const frame_parameters default_val = parameters_.parameters(get_animation_time() - get_begin_time());
	return current_frame.get_overlaped_rect_area_mode(get_current_frame_time(), src, dst, default_val, value);
}

animation::particular::~particular()
{
	halo::remove(halo_id_);
	halo_id_ = halo::NO_HALO;
}

void animation::particular::start_animation(int start_time, bool cycles)
{
	halo::remove(halo_id_);
	halo_id_ = halo::NO_HALO;
	parameters_.override(get_animation_duration());
	animated<unit_frame>::start_animation(start_time,cycles);
	last_frame_begin_time_ = get_begin_time() -1;
}


void base_animator::add_animation2(base_unit* animated_unit
		, const animation* anim
		, const map_location &src
		, bool with_bars
		, bool cycles
		, const std::string& text
		, const Uint32 text_color)
{
	if (!animated_unit) {
		return;
	}

	anim_elem tmp;
	tmp.my_unit = animated_unit;
	tmp.text = text;
	tmp.text_color = text_color;
	tmp.src = src;
	tmp.with_bars= with_bars;
	tmp.cycles = cycles;
	tmp.anim = anim;
	if (!tmp.anim) {
		return;
	}

	start_time_ = std::max<int>(start_time_, tmp.anim->get_begin_time());
	animated_units_.push_back(tmp);
}

void base_animator::start_animations()
{
	int begin_time = INT_MAX;
	std::vector<anim_elem>::iterator anim;
	
	for (anim = animated_units_.begin(); anim != animated_units_.end(); ++ anim) {
		if (anim->my_unit->get_animation()) {
			if(anim->anim) {
				begin_time = std::min<int>(begin_time, anim->anim->get_begin_time());
			} else  {
				begin_time = std::min<int>(begin_time, anim->my_unit->get_animation()->get_begin_time());
			}
		}
	}

	for (anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
		if (anim->anim) {
			// TODO: start_tick_ of particular is relative to last_update_tick_(reference to animated<T,T_void_value>::start_animation), 
			//       and last_update_tick generated by new_animation_frame().
			//       On the other hand, new_animation_frame() may be delayed, for example display gui2::dialog,
			//       once it occur, last_update_tick will indicate "past" time, result to start_tick_ "too early".
			//       ----Should displayed frame in this particular will not display!
			//       there need call new_animation_frame(), update last_update_tick to current tick.
			new_animation_frame();

			anim->my_unit->start_animation(begin_time, anim->anim,
				anim->with_bars, anim->cycles, anim->text, anim->text_color);
			anim->anim = NULL;
		} else {
			anim->my_unit->get_animation()->update_parameters(anim->src, anim->src.get_direction(anim->my_unit->facing()));
		}

	}
}

bool base_animator::would_end() const
{
	bool finished = true;
	for(std::vector<anim_elem>::const_iterator anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
		finished &= anim->my_unit->get_animation()->animation_finished_potential();
	}
	return finished;
}
void base_animator::wait_until(controller_base& controller, int animation_time) const
{
	display& disp = *display::get_singleton();

	double speed = disp.turbo_speed();
	controller.play_slice(false);
	int end_tick = animated_units_[0].my_unit->get_animation()->time_to_tick(animation_time);
	while (SDL_GetTicks() < static_cast<unsigned int>(end_tick)
				- std::min<int>(static_cast<unsigned int>(20/speed),20)) {

		disp.delay(std::max<int>(0,
			std::min<int>(10,
			static_cast<int>((animation_time - get_animation_time()) * speed))));
		controller.play_slice(false);
        end_tick = animated_units_[0].my_unit->get_animation()->time_to_tick(animation_time);
	}
	disp.delay(std::max<int>(0,end_tick - SDL_GetTicks() +5));
	new_animation_frame();
}

void base_animator::wait_for_end(controller_base& controller) const
{
	display& disp = *display::get_singleton();

	if (game_config::no_delay) return;
	bool finished = false;
	while(!finished) {
		controller.play_slice(false);
		disp.delay(10);
		finished = true;
		for (std::vector<anim_elem>::const_iterator anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
			finished &= anim->my_unit->get_animation()->animation_finished_potential();
		}
	}
}
int base_animator::get_animation_time() const
{
	return animated_units_[0].my_unit->get_animation()->get_animation_time() ;
}

int base_animator::get_animation_time_potential() const
{
	return animated_units_[0].my_unit->get_animation()->get_animation_time_potential() ;
}

int base_animator::get_end_time() const
{
	int end_time = INT_MIN;
	for (std::vector<anim_elem>::const_iterator anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
		if(anim->my_unit->get_animation()) {
			end_time = std::max<int>(end_time,anim->my_unit->get_animation()->get_end_time());
		}
	}
	return end_time;
}
void base_animator::pause_animation()
{
        for(std::vector<anim_elem>::iterator anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
	       if(anim->my_unit->get_animation()) {
                anim->my_unit->get_animation()->pause_animation();
	       }
        }
}
void base_animator::restart_animation()
{
    for (std::vector<anim_elem>::iterator anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
		if (anim->my_unit->get_animation()) {
			anim->my_unit->get_animation()->restart_animation();
		}
	}
}
void base_animator::set_all_standing()
{
	for(std::vector<anim_elem>::iterator anim = animated_units_.begin(); anim != animated_units_.end();++anim) {
		anim->my_unit->set_standing(true);
    }
}