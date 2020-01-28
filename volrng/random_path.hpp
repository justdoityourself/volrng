/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string>
#include <array>
#include <string_view>

#include "util.hpp"

using namespace std;

namespace volrng
{
	namespace path
	{
		template <size_t S, typename U = uint64_t> class RandomPath
		{
			string root;
		public:

			typedef array<U, S> seed;

			RandomPath(string_view _root)
				: root(_root)
			{
				for (size_t i = 0; i < S; i++)
					data[i] = /*util::Flip() ? 0 :*/ util::Random();
			}

			seed Seed() { return data; }

			void SetSeed(seed s) { data = s; }

			string Iterate()
			{
				auto result = Build();

				for (size_t i = 0; i < S; i++)
					data[i] = /*util::Flip() ? data[i] : (util::Flip() ? 0 :*/ util::Random();//);

				data[S - 1] = util::Random();

				return result;
			}

			string File() { return Build(); }

			string Path()
			{
				size_t c = 0;

				for (size_t i = 0; i < S; i++)
				{
					if (!data[i]) continue;

					c++;
				}

				if (!c)
					return root + '\\';

				string result = root;

				for (size_t i = 0; i < S; i++)
				{
					if (--c == 0) break;
					if (!data[i]) continue;

					result += '\\' + to_string(data[i]);
				}

				return result;
			}

		private:
			string Build()
			{
				string result = root;
				size_t c = 0;

				for (size_t i = 0; i < S; i++)
				{
					if (!data[i]) continue;
					
					c++;
					result += '\\' + to_string(data[i]);
				}

				if(!c)
					result += '\\' + to_string(Random());

				return result;
			}

			seed data;
		};
	}
}