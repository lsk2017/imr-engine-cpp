#pragma once

template<class T>
class memory_pool;

class Imemory_pool
{
public:
	virtual ~Imemory_pool() = default;
	virtual void clear() = 0;
	virtual void destroy(void** p) = 0;

	template<class T>
	memory_pool<T>* get()
	{
		return static_cast<memory_pool<T>*>(this);
	}

	virtual const size_t get_use_count() const = 0;
	virtual const size_t get_free_count() const = 0;
};

template<class T>
class memory_pool : public Imemory_pool
{
public:
	memory_pool()
	{
		_components.reserve(1);
	}

	T* create()
	{
		T* ret = {};
		if (_component_pool.size() > 0)
		{
			ret = &_components[_component_pool.top()];
			_component_pool.pop();
		}
		else
		{
			assert(_components.capacity() > _components.size());
			ret = &_components.emplace_back();
			_point_to_index[reinterpret_cast<uintptr_t>(ret)] = _components.size() - 1;
		}
		ret->on_reused();
#ifdef _DEBUG
		assert(_using[reinterpret_cast<uintptr_t>(ret)] == false);
		_using[reinterpret_cast<uintptr_t>(ret)] = true;
#endif
		return ret;
	}

	void destroy(T** comp)
	{

		assert(*comp != nullptr);
		uintptr_t key = reinterpret_cast<uintptr_t>(*comp);
#ifdef _DEBUG
		assert(_using[key]);
		_using[key] = false;
#endif
		(*comp)->on_free();
		assert(_point_to_index.contains(key));
		auto idx = _point_to_index[key];
		_component_pool.push(idx);
		assert(_components.size() >= _component_pool.size());
		*comp = nullptr;
	}

	void destroy(void** p) override
	{
		destroy((T**)p);
	}

	void clear() override
	{
		for (size_t i = 0; i < _components.size(); ++i)
		{
			if (_component_pool.size() > 0 && i == _component_pool.top())
			{
				_component_pool.pop();
				continue;
			}
			_components[i].on_free();
		}
		_components.clear();
		_component_pool = {};
		_point_to_index.clear();
#ifdef _DEBUG
		_using.clear();
#endif
	}

	void reserve(size_t capacity)
	{
		_components.reserve(capacity);
	}

	const size_t get_use_count() const override
	{
		return _components.size() - _component_pool.size();
	}

	const size_t get_free_count() const override
	{
		return _component_pool.size();
	}

private:
	std::vector<T> _components = {};
	std::stack<size_t> _component_pool = {};
	std::unordered_map<uintptr_t, size_t> _point_to_index = {};
#ifdef _DEBUG
	std::unordered_map<uintptr_t, bool> _using = {};
#endif
};
