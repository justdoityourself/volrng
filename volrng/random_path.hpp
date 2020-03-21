/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string>
#include <array>
#include <string_view>

#include "d8u/util.hpp"

namespace volrng
{
	namespace path
	{
		using namespace std;
		using namespace d8u;

		template <size_t S, typename U = uint64_t> class RandomPath
		{
			string root;
		public:

			typedef array<U, S> seed;

			RandomPath(string_view _root)
				: root(_root)
			{
				for (size_t i = 1; i < S; i++)
					data[i] = random::Flip() ? 0 : random::Integer();
			}

			seed Seed() { return data; }

			void SetSeed(seed s) { data = s; }

			string Iterate()
			{
				auto result = Build();

				for (size_t i = 0; i < S; i++)
					data[i] = random::Flip() ? data[i] : (random::Flip() ? 0 : random::Integer());

				data[S - 1] = random::Integer();

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
					if (!data[i]) continue;
					if (--c == 0) break;

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
					result += '\\' + to_string(random::Integer());

				return result;
			}

			seed data;
		};
	}
}