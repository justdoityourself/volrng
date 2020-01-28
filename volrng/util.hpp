/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <locale>
#include <codecvt>

#include <string>
#include <string_view>
#include <array>

#include <filesystem>
#include <fstream>
#include <random>
#include <time.h>
#include <chrono>

#include "../mio.hpp"
#include "../picosha2.hpp"
#include "../gsl-lite.hpp"

using namespace std;
using namespace gsl;


namespace volrng
{
	namespace util
	{
		string now()
		{
			chrono::system_clock::time_point p = chrono::system_clock::now();
			time_t t = chrono::system_clock::to_time_t(p);

			return to_string((uint64_t)t);
		}

		typedef array<uint8_t, 32> file_id;
		void id_file(string_view name, file_id& hash)
		{
			mio::mmap_source file(name);
			picosha2::hash256(file.begin(), file.end(), hash.begin(), hash.end());
		}

		size_t Random(size_t max = -1)
		{
			static default_random_engine e;

			size_t result = e();
			result <<= 32; //e() doesn't fill top dword of qword
			result += e();

			return result % max;
		}

		bool Flip() { return (bool)(Random() % 2); }

		template < typename T > vector<T> RandomVector(size_t size)
		{
			vector<T> result;
			result.resize(size);

			static default_random_engine e;

			for (size_t i = 0; i < size; i++)
				result[i] = (T)Random();

			return result;
		}

		template < typename T > void FactorExpand(const span<T>& poly, span<T> output)
		{
			auto const u = poly.size();
			vector<T> l(u + output.size());

			for (size_t i = 0; i < u - 2; i++)
				l[i] = 0;

			l[u - 2] = 1;

			for (size_t i = 0; i < output.size(); i++)
			{
				size_t sum = 0;

				for (size_t j = 0, k = u - 1; j < u - 1; j++, k--)
					sum += poly[k] * l[i + j];

				l[u + i - 1] = sum;
				output[i] = sum;
			}
		}

		constexpr size_t _mb(size_t s) { return s * 1024 * 1024; }
		constexpr size_t _kb(size_t s) { return s * 1024; }
		constexpr size_t _gb(size_t s) { return s * 1024 * 1024 * 1024; }

		wstring to_wide(string_view s)
		{
			wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
			return converter.from_bytes(string(s));
		}

		string to_narrow(wstring_view s)
		{
			wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
			return converter.to_bytes(wstring(s));
		}

		void string_as_file(string_view file,string_view data)
		{
			filesystem::remove(file);

			ofstream f(file);
			f << data;
		}

		array<uint8_t,16> unique_id()
		{
			//TODO Cross platform
			array<uint8_t, 16> ret;
			HRESULT hresult = CoCreateGuid((GUID*)ret.data());

			if(S_OK != hresult)
				throw std::runtime_error("Failed to create GUID");

			return ret;
		}
	}
}