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

#include <timing.h>

#include <array>
#include <cassert>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map> // Unordered associative containers: "Rehashing invalidates iterators, <...> but does not invalidate pointers or references to elements".

namespace
{
	constexpr char MM[][4]{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	constexpr unsigned TIME_STAMP()
	{	auto c2d = [](const char *b, std::size_t n) { return b[n] == ' ' ? 0 : b[n] - '0'; };
		auto date_c2d = [c2d](std::size_t n) { return c2d(__DATE__, n); };
		auto time_c2d = [c2d](std::size_t n) { return c2d(__TIME__, n); };
		unsigned result = 0U;
		
		enum {
		//__DATE__ "MMM DD YYYY"
			M1 = 0, M2, M3, D1 = 4, D2, Y1 = 7, Y2, Y3, Y4,
		//__TIME__ "hh:mm:ss"
			h1 = 0, h2, m1 = 3, m2, s1 = 6, s2,
		};
		result += date_c2d(Y3) * 10 + date_c2d(Y4);

		result *= 100;
		for (unsigned n = 0; n < std::size(MM); ++n)
		{	auto const p = MM[n];
			if (p[M1] == __DATE__[M1] && p[M2] == __DATE__[M2] && p[M3] == __DATE__[M3])
			{	result += n + 1;
				break;
			}
		}

		result *= 100;
		result += date_c2d(D1) * 10 + date_c2d(D2);

		result *= 100;
		result += time_c2d(h1) * 10 + time_c2d(h2);

		result *= 100;
		result += time_c2d(m1) * 10 + time_c2d(m2);

		return result;
	}

	struct item_t
	{	vi_tmAtomicTicks_t total_ = 0U;
		std::size_t counter_ = 0U;
		std::size_t calls_cnt_ = 0U;
		void clear() noexcept
		{	total_ = counter_ = calls_cnt_ = 0U;
		};
	};

	constexpr auto MAX_LOAD_FACTOR = 0.7F;
	int storage_capacity = 64;
	using storage_t = std::unordered_map<std::string, item_t>;

} // namespace

struct vi_tmInstance_t
{	std::mutex storage_guard_;
	storage_t storage_;
	vi_tmAtomicTicks_t total_dummy_ = 0U;

	explicit vi_tmInstance_t(int reserve)
	{	storage_.max_load_factor(MAX_LOAD_FACTOR);
		storage_.reserve(reserve >= 0 ? reserve : storage_capacity);
	}

	static vi_tmInstance_t& global()
	{	static vi_tmInstance_t inst{ storage_capacity };
		return inst;
	}

	void init(int reserve)
	{	if (reserve >= 0)
		{	std::lock_guard lock{ storage_guard_ };
			storage_.reserve(reserve);
		}
	}

	vi_tmAtomicTicks_t& total(const char *name, std::size_t cnt)
	{	if (name)
		{	std::lock_guard lock{ storage_guard_ };
			auto &item = storage_[name];
			item.calls_cnt_ += 1;
			item.counter_ += cnt;
			return item.total_;
		}

		return total_dummy_;
	}

	int results(vi_tmLogRAW_t fn, void *data)
	{	std::lock_guard lock{ storage_guard_ };
		for (const auto &[name, item] : storage_)
		{	assert(item.counter_ >= item.calls_cnt_ && ((0 == item.total_) == (0 == item.calls_cnt_)));
			if (!name.empty() && 0 == fn(name.c_str(), item.total_, item.counter_, item.calls_cnt_, data))
			{	return 0;
			}
		}
		return -1;
	}
	
	void clear(const char *name)
	{	std::lock_guard lock{ storage_guard_ };
		if (!name)
		{	total_dummy_ = 0U;
			for (auto &[_, item] : storage_)
			{	item.clear();
			}
		}
		else if (const auto [it, b] = storage_.try_emplace(name); !b)
		{	it->second.clear();
		}
	}

	friend vi_tmInstance_t& from_handle(vi_tmInstance_t *p)
	{	return p? *p: global();
	}
}; // struct vi_tmInstance_t

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

void VI_TM_CALL vi_tmInit(int reserve)
{	if (reserve >= 0)
	{	storage_capacity = reserve;
	}
	vi_tmInstance_t::global().init(-1);
}

void VI_TM_CALL vi_tmFinit()
{	vi_tmInstance_t::global().clear(nullptr);
}

VI_TM_HANDLE VI_TM_CALL vi_tmCreate(int reserve)
{	return new vi_tmInstance_t{ reserve };
}

void VI_TM_CALL vi_tmClose(VI_TM_HANDLE h)
{	delete h;
}

vi_tmAtomicTicks_t* VI_TM_CALL vi_tmTotalTicks(VI_TM_HANDLE h, const char* name, std::size_t amount) noexcept
{	return &from_handle(h).total(name, amount);
}

void VI_TM_CALL vi_tmClear(VI_TM_HANDLE h, const char* name) noexcept
{	from_handle(h).clear(name);
}

int VI_TM_CALL vi_tmResults(VI_TM_HANDLE h, vi_tmLogRAW_t fn, void *data)
{	return from_handle(h).results(fn, data);
}

const void* VI_TM_CALL vi_tmInfo(vi_tmInfo_e info)
{	const void *result = nullptr;
	switch (info)
	{
		case VI_TM_INFO_VER:
		{	static constexpr std::intptr_t ver = VI_TM_VERSION;
			static_assert(sizeof(result) == sizeof(ver));
			std::memcpy(&result, &ver, sizeof(result));
		} break;

		case VI_TM_INFO_BUILDNUMBER:
		{	static constexpr std::intptr_t ver = TIME_STAMP();
			static_assert(sizeof(result) == sizeof(ver));
			std::memcpy(&result, &ver, sizeof(result));
		} break;

		case VI_TM_INFO_VERSION:
		{	static const char *const version = []
				{	static_assert(VI_TM_VERSION_MAJOR < 100 && VI_TM_VERSION_MINOR < 1'000 && VI_TM_VERSION_PATCH < 1'000); //-V590
#	ifdef VI_TM_SHARED
					static constexpr char type[] = "shared";
#	else
					static constexpr char type[] = "static";
#	endif
					static std::array<char, std::size("99.999.9999 b.YYMMDDHHmm") - 1 + std::size(type)> buff;
					const auto sz = snprintf(buff.data(), buff.size(), VI_TM_VERSION_STR " b.%u %s", TIME_STAMP(), type);
					assert(sz < buff.size());
					return buff.data();
				}();
			result = version;
		} break;

		case VI_TM_INFO_BUILDTIME:
		{	result = VI_STR(__DATE__) " " VI_STR(__TIME__);
		} break;

		case VI_TM_INFO_BUILDTYPE:
		{	
#ifdef NDEBUG
			result = "Release";
#else
			result = "Debug";
#endif
		} break;

		default:
		{	assert(false);
		} break;
	}
	return result;
}

//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
