/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <map>
#include <array>

#include "d8u/util.hpp"
#include "block_db.hpp"
#include "random_path.hpp"

#include "../mio.hpp"

namespace volrng
{
	using namespace d8u::util;
	using namespace blocks;
	using namespace path;

	namespace volume
	{
		using namespace std;

		template <typename DISK, size_t S = _kb(256),size_t D = _gb(50), size_t P = 5> class Test
		{
			string root;
			Database<S> block_db;
		public:

			Test(string_view _root)
				: block_db(string(_root) + "/block.db")
				, root(_root) {}

			~Test(){}

			void Run(uint64_t size, string_view mount)
			{
				if (!std::filesystem::exists(string(root) + "/disk.img"))
					New(size, mount);
				else
					Iterate(size, mount);
			}

			string Mount(string_view mount)
			{
				DISK disk(string(root) + "/disk.img", mount);
				disk.Release();

				return disk.Device();
			}

			void Dismount()
			{
				DISK::Dismount(string(root) + "/disk.img");
			}

			bool Validate(string_view mount)
			{
				string n;
				uint64_t ss;
				uint64_t ms;
				uint64_t ls;
				uint64_t dup;

				{
					ifstream lf(root + "\\latest");
					lf >> n;
				}

				{
					ifstream meta(root + '\\' + n + ".meta", ios::binary);
					meta >> ss;
					meta >> ms;
					meta >> ls;
					meta >> dup;
				}

				mio::mmap_source db(root + '\\' + n + ".db");
				
				RandomPath<P> pathrng(mount);

				for (size_t i = 0; i < db.size(); i += sizeof(file_id) + sizeof(RandomPath<P>::seed))
				{
					auto pseed = (const RandomPath<P>::seed*)(db.data() + i);
					auto pid = (const file_id*)(db.data() + i + sizeof(RandomPath<P>::seed));

					pathrng.SetSeed(*pseed);
					file_id id2;
					id_file(pathrng.File(),id2);

					if (*pid != id2)
						return false;
				}

				return true;
			}

		private:

			void CreateFile(string_view file, uint64_t size, file_id& id, uint64_t&dup)
			{
				{
					ofstream file_handle(file, ios::out | ios::binary);

					uint64_t current = 0;

					block buffer;
					while (current < size)
					{
						uint64_t remaining = size - current;
						if (remaining > S)
							remaining = S;

						buffer = (d8u::random::Integer(5) == 0 && remaining ==S) ? (dup += S, block_db.Duplicate()) : block_db.Allocate(remaining);
						file_handle.write((const char*)buffer.data(), buffer.size());

						current += (uint64_t)buffer.size();
					}
				}

				id_file(file, id);
			}

			void New(uint64_t size, string_view mount)
			{
				DISK disk(string(root) + "/disk.img", _gb(50), mount);
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));

				float small_r = 0.55f, medium_r = 0.25f, large_r = 0.2f;
				auto n = now();

				ofstream db(root + '\\' + n + ".db", ofstream::binary);
				ofstream meta(root + '\\' + n + ".meta", ofstream::binary);

				uint64_t small_file_size = size * small_r;
				uint64_t medium_file_size = size * medium_r;
				uint64_t large_file_size = size * large_r;

				uint64_t ss = size * small_r;
				uint64_t ms = size * medium_r;
				uint64_t ls = size * large_r;

				RandomPath<P> pathrng(mount);

				uint64_t dup = 0;

				while (large_file_size)
					file_core(pathrng, db, small_file_size, medium_file_size, large_file_size, dup);

				meta << ss << endl;
				meta << ms << endl;
				meta << ls << endl;
				meta << dup << endl;

				{
					ofstream lf(root + "\\latest");
					lf << n;
				}
			}

			void Iterate(uint64_t size, string_view mount)
			{
				DISK disk(string(root) + "/disk.img", _gb(50), mount);
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));

				float small_r = 0.55f, medium_r = 0.25f, large_r = 0.2f;
				auto n = now();

				string _n;
				uint64_t ss;
				uint64_t ms;
				uint64_t ls;
				uint64_t dup;

				{
					ifstream lf(root + "\\latest");
					lf >> _n;
				}

				{
					ifstream meta(root + '\\' + _n + ".meta", ios::binary);
					meta >> ss;
					meta >> ms;
					meta >> ls;
					meta >> dup;
				}

				std::filesystem::copy_file(root + '\\' + _n + ".db", root + '\\' + n + ".db");

				mio::mmap_sink pdb(root + '\\' + n + ".db");
				ofstream db(root + '\\' + n + ".db", ofstream::binary);
				ofstream meta(root + '\\' + n + ".meta", ofstream::binary);

				uint64_t small_file_size = size * small_r;
				uint64_t medium_file_size = size * medium_r;
				uint64_t large_file_size = size * large_r;

				ss += size * small_r;
				ms += size * medium_r;
				ls += size * large_r;

				RandomPath<P> pathrng(mount);
				RandomPath<P> updaterng(mount);

				size_t i = 0;
				while (large_file_size)
				{
					if (d8u::random::Flip() && d8u::random::Flip())
						file_update(updaterng, i, (uint8_t*)pdb.data(), small_file_size, medium_file_size, large_file_size, dup);
					else
						file_core(pathrng, db, small_file_size, medium_file_size, large_file_size, dup);
				}

				meta << ss << endl;
				meta << ms << endl;
				meta << ls << endl;
				meta << dup << endl;

				{
					ofstream lf(root + "\\latest");
					lf << n;
				}
			}

			void file_update(RandomPath<P>& updaterng, size_t & i, uint8_t* p, uint64_t& small_file_size, uint64_t& medium_file_size, uint64_t& large_file_size, uint64_t& dup)
			{
				auto pseed = (const RandomPath<P>::seed*)(p + i);
				auto pid = (file_id*)(p + i + sizeof(RandomPath<P>::seed));
				i += sizeof(file_id) + sizeof(RandomPath<P>::seed);

				updaterng.SetSeed(*pseed);
					
				auto file = updaterng.File();
				
				auto step = [](auto& s, auto t)
				{
					uint64_t size = d8u::random::Integer(t);
					if (size > s)
						size = s;
					s -= size;

					return size;
				};

				uint64_t size;
				if (small_file_size) size = step(small_file_size, S);
				else if (medium_file_size) size = step(medium_file_size, S * 10);
				else if (large_file_size) size = step(large_file_size, S * 100);

				CreateFile(file, size, *pid, dup);
			}

			void file_core(RandomPath<P> & pathrng, ofstream & db, uint64_t &small_file_size, uint64_t & medium_file_size ,uint64_t & large_file_size, uint64_t & dup)
			{
				auto seed = pathrng.Seed();
				auto path = pathrng.Path();
				auto file = pathrng.Iterate();

				bool valid_dir = true;
				try
				{
					filesystem::create_directories(path);
				}
				catch (...)
				{
					//Sometimes the RNG clashes
					valid_dir = false;
				}

				if (!valid_dir || std::filesystem::exists(file))
					return;

				auto step = [](auto& s, auto t)
				{
					uint64_t size = d8u::random::Integer(t);
					if (size > s)
						size = s;
					s -= size;

					return size;
				};

				uint64_t size;
				if (small_file_size) size = step(small_file_size, S);
				else if (medium_file_size) size = step(medium_file_size, S * 10);
				else if (large_file_size) size = step(large_file_size, S * 100);

				file_id id;

				CreateFile(file, size, id, dup);

				db.write((const char*)seed.data(), sizeof(seed));
				db.write((const char*)id.data(), sizeof(id));
			}
		};
	}
}