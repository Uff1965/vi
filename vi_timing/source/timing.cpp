// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/********************************************************************\
'vi_timing' is a compact library designed for measuring the execution time of
code in C and C++.

Copyright (C) 2024 A.Prograamar

This library was developed for experimental and educational purposes.
Please temper your expectations accordingly. If you encounter any bugs or have
suggestions for improvements, kindly contact me at programmer.amateur@proton.me.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. 
If not, see <https://www.gnu.org/licenses/gpl-3.0.html#license-text>.
\********************************************************************/

//-V:assert:2570

#include <timing.h>

#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map> // Unordered associative containers: "Rehashing invalidates iterators, <...> but does not invalidate pointers or references to elements".

namespace
{
	struct item_t
	{	vi_tmAtomicTicks_t amount_ = 0U;
		std::size_t counter_ = 0U;
		std::size_t calls_cnt_ = 0U;
		void clear() noexcept
		{	amount_ = counter_ = calls_cnt_ = 0U;
		};
	};

	constexpr auto MAX_LOAD_FACTOR = 0.7F;
	using storage_t = std::unordered_map<std::string, item_t>;

	class Instance_t
	{	std::mutex storage_guard_;
		storage_t storage_;
	protected:
		vi_tmAtomicTicks_t amount_dummy_ = 0U;
		Instance_t(std::size_t reserve = 64U)
		{	storage_.max_load_factor(MAX_LOAD_FACTOR);
			storage_.reserve(reserve);
		}

		class guard_t
		{	std::scoped_lock<std::mutex> lock_;
			Instance_t &inst_;
		public:
			guard_t() = delete;
			guard_t(const guard_t &) = delete;
			guard_t &operator=(const guard_t &) = delete;
			guard_t(Instance_t &inst)
			:	lock_{ inst.storage_guard_ },
				inst_{ inst }
			{
			}
			//auto& operator[](const char *key) const & { return inst_.storage_[key]; }
			auto& operator*() const & { return inst_.storage_; }
			auto& operator*() const && = delete;
			auto* operator->() const & { return &inst_.storage_; }
			auto* operator->() const && = delete;
		};

		auto storage() { return guard_t{ *this }; }
	};

} // namespace

struct vi_tmInstance_t: public Instance_t
{
	using Instance_t::Instance_t;

	static vi_tmInstance_t *create(size_t reserve)
	{	return new vi_tmInstance_t{ reserve };
	}

	void init(size_t reserve)
	{	auto guard = storage();
		guard->reserve(reserve);
	}

	vi_tmAtomicTicks_t *item(const char *name, std::size_t amount)
	{	
		if (name)
		{	auto guard = storage();
			auto &item = (*guard)[name];
			item.calls_cnt_ += 1;
			item.counter_ += amount;
			return &item.amount_;
		}

		return &amount_dummy_;
	}

	int results(vi_tmLogRAW_t fn, void *data)
	{	int result = -1;
		auto guard = storage();
		for (const auto& [name, item] : *guard)
		{	assert(item.counter_ >= item.calls_cnt_);
			assert((0 == item.amount_) == (0 == item.calls_cnt_));
			if (!name.empty() && 0 == fn(name.c_str(), item.amount_, item.counter_, item.calls_cnt_, data))
			{	result = 0;
				break;
			}
		}
		return result;
	}
	
	void clear(const char *name)
	{	if (auto guard = storage(); !name)
		{	for (auto &[_, item] : *guard)
			{	item.clear();
			}
			amount_dummy_ = 0U;
		}
		else if (const auto [it, b] = guard->try_emplace(name); !b)
		{	it->second.clear();
		}
	}

	static vi_tmInstance_t &instance()
	{	static vi_tmInstance_t inst{ 64 };
		return inst;
	}

	static vi_tmInstance_t *from_handle(vi_tmInstance_t *p)
	{	return p? p: &vi_tmInstance_t::instance();
	}
};

void VI_TM_CALL vi_tmInit(std::size_t n)
{	vi_tmInstance_t::instance().init(n);
}

void VI_TM_CALL vi_tmFinit()
{	vi_tmInstance_t::instance().clear(nullptr);
}

vi_tmAtomicTicks_t* VI_TM_CALL vi_tmItem(VI_TM_HANDLE h, const char* name, std::size_t amount) noexcept
{	return vi_tmInstance_t::from_handle(h)->item(name, amount);
}

void VI_TM_CALL vi_tmClear(VI_TM_HANDLE h, const char* name) noexcept
{	vi_tmInstance_t::from_handle(h)->clear(name);
}

int VI_TM_CALL vi_tmResults(VI_TM_HANDLE h, vi_tmLogRAW_t fn, void *data)
{	return vi_tmInstance_t::from_handle(h)->results(fn, data);
}
