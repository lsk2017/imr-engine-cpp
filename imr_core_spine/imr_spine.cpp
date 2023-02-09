#include "imr_spine.h"
#include <spine/Extension.h>
#include <iostream>
#include <assert.h>

spine::SpineExtension* spine::getDefaultExtension() {
	return new spine::DefaultSpineExtension();
}

namespace
{
	class imr_texture_loader : public spine::TextureLoader
	{
	public:
		void load(spine::AtlasPage& page, const spine::String& path) override
		{
			auto [res, texture] = imr::load_texture(path.buffer());
			if (imr::succeed(res))
			{
				texture->set_name(path.buffer());
				_textures[texture->name()] = texture;
				page.setRendererObject(texture.get());
				page.width = texture->width();
				page.height = texture->height();
			}
		}

		void unload(void* texture) override
		{
			imr::Itexture_info* tex = (imr::Itexture_info*)texture;
			if (tex)
			{
				_textures.erase(tex->name());
			}
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<imr::Itexture_info>> _textures = {};
	};

	class context
	{
	public:
		~context()
		{
			spine_data_set = {};
			texture_loader = {};
		}
		std::unordered_map<std::string, std::shared_ptr<imr::spine::spine_data>> spine_data_set = {};
		std::shared_ptr<imr_texture_loader> texture_loader = std::make_shared<imr_texture_loader>();
	};

	std::shared_ptr<context> g_ctx = {};
}

namespace imr::spine
{
	result initialize()
	{
		g_ctx = std::make_shared<context>();
		return {};
	}

	result deinitialize()
	{
		g_ctx = {};
		return {};
	}

	std::tuple<result, std::shared_ptr<spine_data>> load(const char* atlas_path, const char* binary_path)
	{
		auto ret = std::make_shared<spine_data>();

		// Load the texture atlas
		ret->atlas.reset(new ::spine::Atlas(atlas_path, g_ctx->texture_loader.get()));
		if (ret->atlas->getPages().size() == 0) {
			return { {.type = imr::result_type::fail, .error_code = 1, .msg = "failed to load atlas"}, ret };
		}

		// Load the skeleton data
		::spine::SkeletonBinary binary(ret->atlas.get());
		binary.setScale(0.5f);
		auto raw_data = imr::load_data(binary_path);
		ret->skeleton.reset(binary.readSkeletonData((const unsigned char*)raw_data.data(), (int)raw_data.size()));
		if (!ret->skeleton) {
			return { {.type = imr::result_type::fail, .error_code = 2, .msg = "failed to load skeleton data"}, ret };
		}

		// Setup mix times
		ret->animation_state.reset(new ::spine::AnimationStateData(ret->skeleton.get()));
		ret->animation_state->setDefaultMix(0.5f);

		return { {}, ret };
	}

	result regist_spine_data(const std::string& name, std::shared_ptr<spine_data> data)
	{
		g_ctx->spine_data_set[name] = data;
		return {};
	}

	result unregist_spine_data(const std::string& name)
	{
		g_ctx->spine_data_set.erase(name);
		return {};
	}

	std::tuple<result, std::shared_ptr<spine_instance>> instance(const std::string& name)
	{
		auto data = g_ctx->spine_data_set[name];
		if (data == nullptr)
		{
			return { {.type = fail, .error_code = 1, .msg = "not exist spine data"}, {} };
		}

		auto ret = std::make_shared<spine_instance>();
		ret->data = data;
		ret->skeleton = std::make_shared<::spine::Skeleton>(data->skeleton.get());
		ret->animation_state = std::make_shared<::spine::AnimationState>(data->animation_state.get());
		return { {}, ret };
	}

	void spine_instance::set_position(const float2& pos)
	{
		skeleton->setX(pos.x);
		skeleton->setY(-pos.y);
	}

	void spine_instance::set_animation(size_t track_idx, const char* name, bool loop)
	{
		animation_state->setAnimation(track_idx, name, loop);
	}

	void spine_instance::update(float dt)
	{
		animation_state->update(dt);
		animation_state->apply(*skeleton.get());
		skeleton->updateWorldTransform();
	}

	void spine_instance::draw()
	{
		static ::spine::Vector<float> worldVertices = {};
		::spine::Vector<unsigned short> quadIndices;
		quadIndices.add(0);
		quadIndices.add(1);
		quadIndices.add(2);
		quadIndices.add(2);
		quadIndices.add(3);
		quadIndices.add(0);
		imr::Itexture_info* texture = {};
		imr::Itexture_info* prev_texture = {};
		imr::float4 color = {};
		for (unsigned i = 0; i < skeleton->getSlots().size(); ++i) {
			::spine::Slot& slot = *skeleton->getDrawOrder()[i];
			::spine::Attachment* attachment = slot.getAttachment();
			if (!attachment) continue;

			// Early out if the slot color is 0 or the bone is not active
			if (slot.getColor().a == 0 || !slot.getBone().isActive()) {
				clipper->clipEnd(slot);
				continue;
			}

			size_t verticesCount = 0;
			size_t indicesCount = 0;
			::spine::Vector<float>* vertices = &worldVertices;
			::spine::Vector<float>* uvs = NULL;
			::spine::Vector<unsigned short>* indices;
			::spine::Color* attachmentColor;

			if (attachment->getRTTI().isExactly(::spine::RegionAttachment::rtti)) {
				::spine::RegionAttachment* regionAttachment = (::spine::RegionAttachment*)attachment;
				attachmentColor = &regionAttachment->getColor();

				// Early out if the slot color is 0
				if (attachmentColor->a == 0) {
					clipper->clipEnd(slot);
					continue;
				}

				worldVertices.setSize(8, 0);
				regionAttachment->computeWorldVertices(slot, worldVertices, 0, 2);
				verticesCount = 4;
				uvs = &regionAttachment->getUVs();
				indices = &quadIndices;
				indicesCount = 6;
				texture = (imr::Itexture_info*)((::spine::AtlasRegion*)regionAttachment->getRendererObject())->page->getRendererObject();

			}
			else if (attachment->getRTTI().isExactly(::spine::MeshAttachment::rtti)) {
				::spine::MeshAttachment* mesh = (::spine::MeshAttachment*)attachment;
				attachmentColor = &mesh->getColor();

				// Early out if the slot color is 0
				if (attachmentColor->a == 0) {
					clipper->clipEnd(slot);
					continue;
				}

				worldVertices.setSize(mesh->getWorldVerticesLength(), 0);
				mesh->computeWorldVertices(slot, 0, mesh->getWorldVerticesLength(), worldVertices.buffer(), 0, 2);
				texture = (imr::Itexture_info*)((::spine::AtlasRegion*)mesh->getRendererObject())->page->getRendererObject();
				verticesCount = mesh->getWorldVerticesLength() >> 1;
				uvs = &mesh->getUVs();
				indices = &mesh->getTriangles();
				indicesCount = indices->size();

			}
			else if (attachment->getRTTI().isExactly(::spine::ClippingAttachment::rtti)) {
				::spine::ClippingAttachment* clip = (::spine::ClippingAttachment*)slot.getAttachment();
				clipper->clipStart(slot, clip);
				continue;
			}
			else
				continue;

			color.x = skeleton->getColor().r * slot.getColor().r * attachmentColor->r;
			color.y = skeleton->getColor().g * slot.getColor().g * attachmentColor->g;
			color.z = skeleton->getColor().b * slot.getColor().b * attachmentColor->b;
			color.w = skeleton->getColor().a * slot.getColor().a * attachmentColor->a;

			if (clipper->isClipping()) {
				clipper->clipTriangles(worldVertices, *indices, *uvs, 2);
				vertices = &clipper->getClippedVertices();
				verticesCount = clipper->getClippedVertices().size() >> 1;
				uvs = &clipper->getClippedUVs();
				indices = &clipper->getClippedTriangles();
				indicesCount = clipper->getClippedTriangles().size();
			}

			switch (slot.getData().getBlendMode()) {
			case ::spine::BlendMode_Normal:
				break;
			case ::spine::BlendMode_Multiply:
				break;
			case ::spine::BlendMode_Additive:
				break;
			case ::spine::BlendMode_Screen:
				break;
			}

			if (texture != prev_texture /*|| blend != prev_blend*/)
			{
				prev_texture = texture;
				imr::mesh::end();
				imr::mesh::begin();
				imr::mesh::use_program(imr::MESH_PROGRAM_NAME);
				imr::mesh::set_texture(0, texture);
				imr::mesh::vertex_attrib_pointer(0, 4, 8, 0);
				imr::mesh::vertex_attrib_pointer(1, 4, 8, 4);
			}
			std::vector<imr::mesh::vertex> verts = {};
			for (auto i = 0, j = 0; i < verticesCount; ++i, j += 2)
			{
				auto& v = verts.emplace_back();
				v.position.x = vertices->operator[](j);
				v.position.y = -vertices->operator[](j + 1);
				v.uv.x = uvs->operator[](j);
				v.uv.y = 1.0f - uvs->operator[](j + 1);
				v.color = color;
			}
			auto stride = sizeof(imr::mesh::vertex) / sizeof(float);
			imr::mesh::push_meshes((float*)verts.data(), static_cast<int>(stride), verts.size() * stride, indices->buffer(), indicesCount);

			clipper->clipEnd(slot);
		}
		clipper->clipEnd();
		imr::mesh::end();
	}
}