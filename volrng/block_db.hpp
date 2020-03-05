/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>

#include "d8u/util.hpp"

#include "../mio.hpp"

namespace volrng
{
	namespace blocks
	{
		using namespace std;

		typedef vector<uint8_t> block;
		template <size_t S> class Database
		{
			struct BlockRecord
			{
				BlockRecord(span<uint64_t> _seed,size_t _size)
				{
					copy(_seed.begin(), _seed.end(), seed.begin());
					size = _size;
				}

				array<uint8_t, 32> seed;
				size_t size;
			};


			string file;
		public:
			Database(string_view _file) : file(_file){}

			block Short()
			{
				return Allocate(Random(S));
			}

			block Allocate(size_t size = S)
			{
				auto seed = RandomVector<uint64_t>(4);
				block output(S);
				FactorExpand<uint64_t>(seed, span<uint64_t>((uint64_t*)output.data(),(uint64_t*)(output.data()+S)));

				ofstream fh(file, ofstream::binary | ofstream::app);

				BlockRecord br(seed,size);
				fh.write((const char * )&br, sizeof(br));

				output.resize(size);

				return output;
			}

			block Duplicate()
			{
				if (!filesystem::exists(file))
					return Allocate();

				mio::mmap_source file_map(file);

				if (!file_map.size())
					return Allocate();

				auto dx = util::Random(file_map.size() / sizeof(BlockRecord));

				auto* r = (BlockRecord*)file_map.data();
				r += dx;

				block output(S);
				FactorExpand<uint64_t>(span<uint64_t>((uint64_t*)r->seed.data(), (uint64_t*)(r->seed.data()+32)), span<uint64_t>((uint64_t*)output.data(), (uint64_t*)(output.data()+S)));

				output.resize(r->size);

				return output;
			}
		};
	}
}