/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string_view>
#include <string>
#include <stdexcept>
#include <filesystem>

#include <windows.h>
#include <atlbase.h>
#include <initguid.h>
#include <virtdisk.h>
#pragma comment(lib, "VirtDisk.lib")

#include "util.hpp"

using namespace std;

namespace volrng
{
	using namespace util;

	namespace win
	{
		void diskpart(string command)
		{
			string_as_file("system_command.txt",command);

			system("diskpart /s system_command.txt");

			filesystem::remove("system_command.txt");
		}

		class VHD
		{
		public:
			VHD()
			{
				hvhd = nullptr;
			}

			VHD(string_view vhd_file, string_view drive_letter,bool no_attach = false) :
				hvhd(NULL)
			{
				Open(vhd_file, drive_letter,no_attach);
				Path();

				if (drive_letter.size())
					Mount(id, drive_letter);
			}

			VHD(string_view vhd_file, unsigned long long length_bytes, string_view drive_letter) :
				hvhd(NULL)
			{
				if(!filesystem::exists(vhd_file))
					Create(vhd_file, length_bytes);
				Open(vhd_file, drive_letter);

				Path();

				if (drive_letter.size())
					Partition(id, drive_letter);
			}

			~VHD()
			{
				if (hvhd && INVALID_HANDLE_VALUE != hvhd)
				{
					Detatch();
					CloseHandle(hvhd);
					hvhd = nullptr;
				}
			}

			void Release()
			{
				CloseHandle(hvhd);
				hvhd = nullptr;
			}

			string Device()
			{
				return string("\\\\.\\PhysicalDrive" + id);
			}

			unsigned int PhysicalNumber()
			{
				return stoi(id);
			}

			static void Dismount(string_view path_and_file)
			{
				VHD vhd(path_and_file, "",true);
			}

		protected:
			void Create(string_view path_and_file, unsigned long long length_bytes)
			{
				CREATE_VIRTUAL_DISK_PARAMETERS  params;
				VIRTUAL_DISK_ACCESS_MASK        mask;
				VIRTUAL_STORAGE_TYPE vst = { VIRTUAL_STORAGE_TYPE_DEVICE_VHD, VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT };

				params.Version1.UniqueId = GUID_NULL;
				params.Version1.BlockSizeInBytes = 0;
				params.Version1.MaximumSize = length_bytes;
				params.Version1.ParentPath = NULL;
				params.Version1.SourcePath = NULL;
				params.Version1.SectorSizeInBytes = 512;
				params.Version = CREATE_VIRTUAL_DISK_VERSION_1;
				mask = VIRTUAL_DISK_ACCESS_CREATE;

				auto result = CreateVirtualDisk(&vst, to_wide(path_and_file).c_str(), mask, NULL, CREATE_VIRTUAL_DISK_FLAG_NONE, 0, &params, NULL, &hvhd);

				if(result != ERROR_SUCCESS || hvhd == nullptr || hvhd == INVALID_HANDLE_VALUE)
					throw std::runtime_error("Failed to create VHD " + to_string(result));

				CloseHandle(hvhd);
				hvhd = INVALID_HANDLE_VALUE;
			}

			void Open(string_view file, string_view letter,bool no_attach = false)
			{
				OPEN_VIRTUAL_DISK_PARAMETERS oparams;
				ATTACH_VIRTUAL_DISK_PARAMETERS iparams;
				VIRTUAL_STORAGE_TYPE vst = { VIRTUAL_STORAGE_TYPE_DEVICE_VHD, VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT };

				oparams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
				oparams.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

				iparams.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

				auto result = OpenVirtualDisk(&vst, to_wide(file).c_str(), VIRTUAL_DISK_ACCESS_ATTACH_RW | VIRTUAL_DISK_ACCESS_GET_INFO | VIRTUAL_DISK_ACCESS_DETACH | VIRTUAL_DISK_ACCESS_ALL, OPEN_VIRTUAL_DISK_FLAG_NONE, &oparams, &hvhd);

				if (result != ERROR_SUCCESS || hvhd == nullptr || hvhd == INVALID_HANDLE_VALUE)
					throw std::runtime_error("Failed to open VHD " + to_string(result));

				if(!no_attach)
					Attach();
			}

			void Attach()
			{
				ATTACH_VIRTUAL_DISK_PARAMETERS iparams;
				iparams.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

				auto result = AttachVirtualDisk(hvhd, NULL, ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME, 0, &iparams, NULL);

				if (result != ERROR_SUCCESS)
					throw std::runtime_error("Failed to attach VHD " + to_string(result));
			}

			void Path()
			{
				WCHAR tmp[1024] = { 0 };
				ULONG size = 1024;

				auto result = GetVirtualDiskPhysicalPath(hvhd, &size, tmp);

				if (result != ERROR_SUCCESS)
					throw std::runtime_error("Failed to read VHD path " + to_string(result));

				path = to_narrow(tmp);
				id = path.back();
			}

			void Detatch()
			{
				auto result = DetachVirtualDisk(hvhd, DETACH_VIRTUAL_DISK_FLAG_NONE, 0);

				if (result != ERROR_SUCCESS)
					throw std::runtime_error("Failed to detach VHD " + to_string(result));
			}

			void Mount(string_view disknumber, string_view driveletter)
			{
				return diskpart(string("select disk ") + string(disknumber) + "\nselect partition 1\nassign letter=" + string(driveletter) + "\n");
			}

			void Partition(string_view disknumber, string_view driveletter)
			{
				return diskpart(string("select disk ") + string(disknumber) + "\ncreate partition primary\nselect partition 1\nassign letter=" + string(driveletter) + "\nselect volume=" + string(driveletter) + "\nformat quick\n");
			}

			HANDLE hvhd;
			string path;
			string id;
		};
	}
}