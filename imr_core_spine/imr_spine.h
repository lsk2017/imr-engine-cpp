#pragma once

#include "imr_core.h"
#include <spine/spine.h>

//auto [ret, spine_data] = imr::spine::load("spineboy.atlas", "spineboy.skel");
//imr::spine::regist_spine_data("spineboy", spine_data);
//auto spine_instance = imr::spine::instance("spineboy");
//spine_instance->animate("walk");
//spine_instance->update(dt);
//spine_instance->draw();
//imr::spine::unregist_spine_data("spineboy");
namespace imr::spine
{
	class spine_data
	{
	public:
		std::string name = {};
		std::shared_ptr<::spine::Atlas> atlas = {};
		std::shared_ptr<::spine::SkeletonData> skeleton = {};
		std::shared_ptr<::spine::AnimationStateData> animation_state = {};
	};

	class spine_instance
	{
	public:
		std::shared_ptr<spine_data> data = {};
		std::shared_ptr<::spine::AnimationState> animation_state = {};
		std::shared_ptr<::spine::Skeleton> skeleton = {};
		std::shared_ptr<::spine::SkeletonClipping> clipper = std::make_shared<::spine::SkeletonClipping>();

		void set_position(const float2& pos);
		void set_animation(size_t track_idx, const char* name, bool loop);
		void update(float dt);
		void draw();
	};

	result initialize();
	result deinitialize();
	std::tuple<result, std::shared_ptr<spine_data>> load(const char* atlas_path, const char* binary_path);
	result regist_spine_data(const std::string& name, std::shared_ptr<spine_data> data);
	result unregist_spine_data(const std::string& name);
	std::tuple<result, std::shared_ptr<spine_instance>> instance(const std::string& name);
}
