#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <assert.h>
#include <box2d/box2d.h>

#include "memory_pool.h"

namespace imr::game
{
	class gameobject;
	class gameworld;

	struct Igame_context
	{
	private:
		std::unordered_map<std::type_index, std::shared_ptr<Imemory_pool>> _pool = {};

	public:
		virtual ~Igame_context() = default;
		template<class T>
		inline memory_pool<T>* pool()
		{
			auto& ret = _pool[typeid(T)];
			if (ret == nullptr)
			{
				ret = std::make_shared<memory_pool<T>>();
			}
			return ret->get<T>();
		}

		inline Imemory_pool* raw_pool(std::type_index ti)
		{
			auto& ret = _pool[ti];
			return ret.get();
		}

		const std::unordered_map<std::type_index, std::shared_ptr<Imemory_pool>>& pools() const
		{
			return _pool;
		}
	};

	class Iworld_system
	{
	public:
		virtual ~Iworld_system() = default;
		virtual void installed(gameworld* world) = 0;
		virtual void fixed_update(gameworld* world, float dt) = 0;
		virtual void update(gameworld* world, float dt) = 0;
		virtual void render(gameworld* world) = 0;
		virtual void ui(gameworld* world) = 0;
	};

	class systems
	{
	public:
		~systems();

		void set_game_context(Igame_context* ctx)
		{
			_game_context = ctx;
		}

		template<class T>
		void add_world_system()
		{
			T* system = _game_context->pool<T>()->create();
			_systems.push_back(system);
		}

		template<class T>
		void remove_world_system()
		{
			_systems.erase([this](auto* i)
				{
					auto same = typeid(T) == typeid(*i);
					if (same)
					{
						_game_context->raw_pool(typeid(T))->destroy((void**)&i);
					}
					return same;
				});
		}

		void installed(gameworld* world);
		void fixed_update(gameworld* world, float fdt);
		void update(gameworld* world, float dt);
		void render(gameworld* world);
		void ui(gameworld* world);

	private:
		std::vector<Iworld_system*> _systems = {};
		Igame_context* _game_context = {};
	};

	class universe;

	class Iuniverse_command
	{
	public:
		virtual ~Iuniverse_command() = default;
		virtual void run(universe*) = 0;
		/*void undo();*/
		void on_reused() {};
		void on_free() {};
	};

	class Icommand
	{
	public:
		virtual ~Icommand() = default;
		virtual void run(gameworld*) = 0;
		/*void undo();*/
	};

	using world_id = int;
	using go_id = unsigned int;

	class universe : b2ContactListener
	{
		friend gameworld;
	public:
		universe();
		~universe();
		gameworld* create_world(world_id id);
		gameworld* get_world(world_id id) { return &_gameworlds.at(id); }
		bool has_world(world_id id) { return _gameworlds.contains(id); }
		void add_command(Iuniverse_command* command);
		void run_command(Iuniverse_command* command);
		inline float time() { return _time; }
		float fixed_fps = 30.0f;
		inline float fixed_delta() { return 1.0f / fixed_fps; }
		void fixed_update();
		void update(float dt);
		void render();
		void ui();
		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;
		template<class T>
		constexpr T* get_game_context() const
		{
			return dynamic_cast<T*>(_game_context);
		}
		template<class T>
		T* set_game_context(T* ctx)
		{
			_game_context = ctx;
			return get_game_context<T>();
		}

	private:
		std::shared_ptr<b2World> _physics_world = std::make_shared<b2World>(b2Vec2{ 0, 0 });
		std::unordered_map<world_id, gameworld> _gameworlds = {};
		float _time = {};
		float _fixed_elapsed = {};
		go_id _gameobject_id_stack = 1;
		Igame_context* _game_context = {};

		std::queue<Iuniverse_command*> _commands = {};
	};

	class gameworld
	{
		friend universe;
		friend gameobject;
	public:
		~gameworld();

		bool is_enabled() const { return _enabled; }
		void enable(bool flag);
		void add_command(Icommand* command);
		void run_command(Icommand* command);
		gameobject* create_gameobject();
		void destroy_gameobject(gameobject** go);
		void destroy_gameobject_by_id(go_id goid);

		template<class T>
		const std::unordered_set<gameobject*>& get_gameobjects()
		{
			return _gameobjects_by_compoent[typeid(T)];
		}
		
		inline float time() { return _universe->time(); }
		void fixed_update();
		void update(float dt);
		void render();
		void ui();
		inline b2World* get_physics_world() { return _universe->_physics_world.get(); }

		template<class T>
		inline memory_pool<T>* pool()
		{
			return _game_context->pool<T>();
		}

		inline Imemory_pool* raw_pool(std::type_index ti)
		{
			return _game_context->raw_pool(ti);
		}

		template<class T>
		constexpr T* get_game_context() const
		{
			return dynamic_cast<T*>(_game_context);
		}

		Igame_context* get_raw_game_context() const
		{
			return _game_context;
		}

		template<class T>
		T* set_game_context(T* ctx)
		{
			_game_context = ctx;
			return get_game_context<T>();
		}

		gameobject* get_gameobject(go_id goid) const
		{
			if (_gameobjects.contains(goid))
			{
				return _gameobjects.at(goid);
			}
			return nullptr;
		}

		void set_systems(systems* sys)
		{
			_systems = sys;
			_systems->installed(this);
		}

		static void translate(gameworld* prev, gameworld* new_world, gameobject* target);
		universe* get_universe() const { return _universe; }

	private:
		universe* _universe = {};
		std::queue<Icommand*> _commands = {};
		systems* _systems = {};
		std::unordered_map<go_id, gameobject*> _gameobjects = {};
		std::unordered_map<std::type_index, std::unordered_set<gameobject*>> _gameobjects_by_compoent = {};
		void _add_gameobject_by_component(std::type_index ti, gameobject* go)
		{
			_gameobjects_by_compoent[ti].insert(go);
		}

		void _remove_gameobject_by_component(std::type_index ti, gameobject* go)
		{
			_gameobjects_by_compoent[ti].erase(go);
		}

		Igame_context* _game_context = {};
		bool _enabled = true;
	};

	struct Icomponent
	{
		virtual ~Icomponent() = default;
		virtual void enable(bool flag) = 0;
		virtual void added(gameobject* go, gameworld* world) = 0;
		virtual void removed(gameobject* go, gameworld* world) = 0;
		gameobject* owner = {};
	};

	struct Icontactable
	{
		friend universe;
		friend gameworld;
		virtual ~Icontactable() = default;
		virtual gameobject* get_gameobject() = 0;
	protected:
		std::unordered_map<b2Fixture*, std::unordered_set<Icontactable*>> contact_list = {};
		virtual void on_begin_contact(Icontactable* other) = 0;
		virtual void on_end_contact(Icontactable* other) = 0;
	};

	class gameobject
	{
		friend gameworld;
	public:
		template<class T>
		constexpr T* add_component()
		{
			assert(!has_component<T>());
			T* ret = _world->pool<T>()->create();
			assert(ret != nullptr);
			_components[typeid(T)] = ret;
			_world->_add_gameobject_by_component(typeid(T), this);
			ret->owner = this;
			ret->added(this, _world);
			return ret;
		}

		void enable(bool flag)
		{
			for (auto& c : _components)
			{
				c.second->enable(flag);
			}
		}

		template<class T>
		void remove_component()
		{
			remove_component(typeid(T));
		}

		void remove_component(std::type_index ti)
		{
			assert(_components.contains(ti));
			auto c = _components[ti];
			assert(c != nullptr);
			_components.erase(ti);
			c->removed(this, _world);
			c->owner = nullptr;
			_world->raw_pool(ti)->destroy((void**)&c);
			_world->_remove_gameobject_by_component(ti, this);
			assert(c == nullptr);
		}

		template<class T>
		constexpr T* get_component()
		{
			T* ret = {};
			if (_components.contains(typeid(T)))
				ret = (T*)_components[typeid(T)];
			return ret;
		}

		template<class T>
		constexpr bool has_component()
		{
			return _components.contains(typeid(T));
		}

		void on_reused()
		{
		}

		void on_free()
		{
			auto comp = _components;
			for (auto& p : comp)
			{
				remove_component(p.first);
			}
			_components.clear();
		}

		constexpr go_id get_gameobject_id() const { return _id; }
		gameworld* get_world() const { return _world; }

	private:
		std::unordered_map<std::type_index, Icomponent*> _components = {};
		gameworld* _world = {};
		go_id _id = {};
	};
}
