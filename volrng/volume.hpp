/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <map>
#include <array>

#include "util.hpp"
#include "block_db.hpp"
#include "random_path.hpp"

#include "../mio.hpp"

using namespace std;

namespace volrng
{
	using namespace util;
	using namespace blocks;
	using namespace path;

	namespace volume
	{
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
				New(size,mount);
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
					ifstream meta(root + "\\latest", ios::binary);
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

						buffer = (Random(5) == 0 && remaining ==S) ? (dup += S, block_db.Duplicate()) : block_db.Allocate(remaining);
						file_handle.write((const char*)buffer.data(), buffer.size());

						current += (uint64_t)buffer.size();
					}
				}

				id_file(file, id);
			}

			/*void ModifyTestFile(string_view file, uint64_t size, file_id& id)
			{
				{
					//append one block modify one block
					ofstream file_handle(file, ios::out | ios::binary);
				}

				unsigned long long file_size = file_handle.FileSize();
				unsigned long long offset = 0;
				if (file_size)
					offset = (rand() * _kb(10)) % file_size;

				exBuffer buffer;
				buffer.resize(size);
				AllocateBlock(buffer, true);

				modified_bytes += size;

				file_handle.Seek(offset);
				file_handle.Write(buffer);

				id_file(file, id);
			}*/

			void New(uint64_t size, string_view mount)
			{
				DISK disk(string(root) + "/disk.img", _gb(50), mount);

				float small_r = 0.55f, medium_r = 0.25f, large_r = 0.2f;
				auto n = now();

				ofstream db(root + '\\' + n + ".db", ofstream::binary);
				ofstream meta(root + '\\' + n + ".meta", ofstream::binary);

				{
					ofstream lf(root + "\\latest");
					lf << n;
				}

				uint64_t small_file_size = size * small_r;
				uint64_t medium_file_size = size * medium_r;
				uint64_t large_file_size = size * large_r;

				uint64_t ss = size * small_r;
				uint64_t ms = size * medium_r;
				uint64_t ls = size * large_r;

				RandomPath<P> pathrng(mount);

				uint64_t dup = 0;

				while (large_file_size)
				{
					auto seed = pathrng.Seed();
					auto path = pathrng.Path();
					auto file = pathrng.Iterate();
					filesystem::create_directories(path);

					auto step = [](auto& s, auto t)
					{
						uint64_t size = Random(t);
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

					db.write((const char*)seed.data(),sizeof(seed));
					db.write((const char*)id.data(), sizeof(id));
				}

				meta << ss;
				meta << ms;
				meta << ls;
				meta << dup;
			}

		//	void IterateDisk(unsigned long long size, unsigned long long modified_bytes, float small_r = 0.65f, float medium_r = 0.25f, float large_r = 0.1f)
		//	{
		//		string path_to_disk;
		//		path_to_disk = working_directory;
		//		path_to_disk += id.AsString();
		//		path_to_disk += L"\\";
		//		exBuffer filename_record;

		//		exFile::MakePath(path_to_disk);

		//		exFile history_file(EXS(path_to_disk, ex_int_to_string(iteration), L"_history.db"));
		//		exBuffer existing_files;
		//		existing_files.FromFile(EXS(path_to_disk, L"file.db"));
		//		vector < wchar_t* > filename_strings;
		//		existing_files.stringrray(filename_strings);
		//		exFile files_file(EXS(path_to_disk, ex_int_to_string(iteration), L"_files.db"));
		//		history_file.End();
		//		files_file.End();

		//		exBuffer modified_files;

		//		unsigned long long actual_modified = 0;

		//		unsigned int progress_bondary = 10;

		//		while (false /*modified_bytes*/)
		//		{
		//			unsigned long long size;
		//			string_view path_and_file, ssize;
		//			string random_file;
		//			random_file = filename_strings[rand() % filename_strings.size()];
		//			string_view s = random_file;

		//			s.SplitForward(path_and_file, ssize, L':', true, 1);
		//			ModifyTestFile(path_and_file, size, cid);

		//			if (size > modified_bytes)
		//				modified_bytes = 0;
		//			else
		//				modified_bytes -= size;

		//			if (exfilelist.find(random_file) == exfilelist.end())
		//				modified_files += EXS(random_file, L":", ex_int_to_string(size), L":", id_ref.AsString());

		//			actual_modified += size;
		//		}

		//		unsigned long long small_file_size = size * small_r;
		//		unsigned long long medium_file_size = size * medium_r;
		//		unsigned long long large_file_size = size * large_r;

		//		unsigned long long small_actual = 0;
		//		unsigned long long medium_actual = 0;
		//		unsigned long long large_actual = 0;

		//		while (small_file_size || medium_file_size || large_file_size)
		//		{
		//			unsigned int directory_depth = rand() % 5;

		//			string path = drive_letter;
		//			path += L":\\";

		//			bool directory_excluded = ((rand() % 15) == 0);
		//			bool first = true;

		//			while (directory_depth)
		//			{
		//				path += ex_single < exEnglishWordsMinimal >().RandomEnglishWord();
		//				path += L"\\";
		//				ex_fatal(exFile::MakePath(path))

		//					if (directory_excluded)
		//					{
		//						excluded_directories++;

		//						if (first == true)
		//							exdirlist.push_back(path);
		//					}
		//				first = false;

		//				unsigned int file_count = rand() % 3;
		//				unsigned long long size;
		//				if (directory_depth == 1)
		//					file_count = rand() % 30;

		//				while (file_count)
		//				{
		//					string filename;
		//					string justfile;
		//					filename = path;
		//					ex_random_filename(justfile);
		//					filename += justfile;
		//				retry:
		//					if (small_file_size == 0 && medium_file_size == 0 && large_file_size == 0)
		//						break;

		//					switch (rand() % 3)
		//					{
		//					case 0:
		//						if (!small_file_size)
		//							goto retry;

		//						size = RandomFileSize(0);
		//						if (size > small_file_size)
		//							size = small_file_size;

		//						CreateTestFile(filename, size, 0, cid);

		//						if (size > small_file_size)
		//							small_file_size = 0;
		//						else
		//							small_file_size -= size;
		//						small_actual += size;

		//						if (IsTypeExcluded(filename))
		//						{
		//							excluded_by_type++;
		//							effective_excluded_files++;
		//						}
		//						else if (directory_excluded)
		//						{
		//							effective_excluded_files++;
		//						}
		//						else if ((rand() % 20) == 0)
		//						{
		//							excluded_files++;
		//							effective_excluded_files++;
		//							exfilelist[filename] = 1;
		//						}
		//						else
		//						{
		//							filename_record += EXS(filename, L":", ex_int_to_string(size), L":", id_ref.AsString());
		//							current_snapshot_record[justfile] = std::pair < unsigned long long, unsigned long long >(size, 0);
		//						}

		//						ex_library_progress_handle.main.progress += size;

		//						break;
		//					case 1:

		//						if (!medium_file_size)
		//							goto retry;

		//						size = RandomFileSize(1);
		//						if (size > medium_file_size)
		//							size = medium_file_size;

		//						ex_fatal(CreateTestFile(filename, size, 1, cid))

		//							if (size > medium_file_size)
		//								medium_file_size = 0;
		//							else
		//								medium_file_size -= size;

		//						medium_actual += size;

		//						if (IsTypeExcluded(filename))
		//						{
		//							excluded_by_type++;
		//							effective_excluded_files++;
		//						}
		//						else if (directory_excluded)
		//						{
		//							effective_excluded_files++;
		//						}
		//						else if ((rand() % 20) == 0)
		//						{
		//							excluded_files++;
		//							effective_excluded_files++;
		//							exfilelist[filename] = 1;
		//						}
		//						else
		//						{
		//							filename_record += EXS(filename, L":", ex_int_to_string(size), L":", id_ref.AsString());
		//							current_snapshot_record[justfile] = std::pair < unsigned long long, unsigned long long >(size, 0);
		//						}

		//						ex_library_progress_handle.main.progress += size;

		//						break;
		//					case 2:
		//						if (!large_file_size)
		//							goto retry;

		//						size = RandomFileSize(2);
		//						if (size > large_file_size)
		//							size = large_file_size;

		//						ex_fatal(CreateTestFile(filename, size, 2, cid))

		//							if (size > large_file_size)
		//								large_file_size = 0;
		//							else
		//								large_file_size -= size;

		//						large_actual += size;

		//						if (IsTypeExcluded(filename))
		//						{
		//							excluded_by_type++;
		//							effective_excluded_files++;
		//						}
		//						else if (directory_excluded)
		//						{
		//							effective_excluded_files++;
		//						}
		//						else if ((rand() % 20) == 0)
		//						{
		//							excluded_files++;
		//							effective_excluded_files++;
		//							exfilelist[filename] = 1;
		//						}
		//						else
		//						{
		//							filename_record += EXS(filename, L":", ex_int_to_string(size), L":", id_ref.AsString());
		//							current_snapshot_record[justfile] = std::pair < unsigned long long, unsigned long long >(size, 0);
		//						}

		//						ex_library_progress_handle.main.progress += size;

		//						break;
		//					}

		//					file_count--;
		//				}

		//				directory_depth--;
		//			}
		//		}

		//		history_file.Write(actual_modified);
		//		history_file.Write(size);
		//		history_file.Write(small_actual);
		//		history_file.Write(medium_actual);
		//		history_file.Write(large_actual);

		//		files_file.Write(filename_record);

		//		filename_record.ToFile(EXS(path_to_disk, L"file.db"), true);

		//		modified_files.ToFile(EXS(path_to_disk, ex_int_to_string(iteration), L"_modified.db"), true);
		//	}
		//
		};
	}
}