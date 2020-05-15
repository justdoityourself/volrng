/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once


#include <string>
#include <string_view>

#include "d8u/cmd.hpp"
#include "d8u/buffer.hpp"

namespace volrng
{
	namespace win
	{
		using namespace d8u; 

		class ISCSIClient
		{
		public:
			template < typename F > static void EnumerateTargets(std::string_view ip, F && f)
			{
				cmd("net start MSiSCSI");
				if(ip.size())
					cmd(std::string("iscsicli QAddTargetPortal ") + std::string(ip));

				auto _result = execute("iscsicli ListTargets");

				Helper result(_result);
				
				bool capture = false;
				while (result.size())
				{
					auto line = result.GetLine2();

					if (line.StartsWith(std::string_view("The operation")))
						continue;

					switch (switch_t(line))
					{
					case switch_t("Targets List:"):
						capture = true;
						break;
					default:
						if (capture) 
							f(line.Trim(" \t\r\n"));
						break;
					}
				}
			}

			template < typename F > static void EnumerateSessions(std::string_view ip, F&& f)
			{
				cmd("net start MSiSCSI");
				if(ip.size())
					cmd(std::string("iscsicli QAddTargetPortal ") + std::string(ip));

				auto _result = execute("iscsicli sessionlist");

				Helper result(_result);

				std::string_view session;
				std::string_view target;
				std::string_view device;

				std::map<std::string_view, std::string_view> map;

				bool capture = false;
				while (result.size())
				{
					auto line = result.GetLine2();

					if (line.StartsWith(std::string_view("The operation")))
						continue;

					if (line.StartsWith(std::string_view("Total of ")))
					{
						capture = true;
						continue;
					}

					if (capture)
					{
						auto k = line.GetWord(':').Cleanup(" \t\r\n"), v = line.Cleanup(" \t\r\n");

						if (v.size())
						{
							map[k] = v;

							if (k.StartsWith(std::string_view("Device Instance")))
							{
								f(map);
								map.clear();
							}
						}
					}
				}
			}

			static void Partition(std::string_view disk, std::string_view letter)
			{
				partition(disk, letter);
			}

			template < typename F > static void EnumerateMappings(std::string_view ip, F&& f)
			{
				cmd("net start MSiSCSI");

				if(ip.size()) 
					cmd(std::string("iscsicli QAddTargetPortal ") + std::string(ip));

				auto _result = execute("iscsicli reporttargetmappings");

				Helper result(_result);

				std::string_view session;
				std::string_view target;
				std::string_view device;

				bool capture = false;
				while (result.size())
				{
					auto line = result.GetLine2();

					if (line.StartsWith(std::string_view("The operation")))
						continue;

					if (line.StartsWith(std::string_view("Total of ")))
					{
						capture = true;
						continue;
					}

					if (capture)
					{
						line.Trim(" \t\r\n");

						if (line.StartsWith(std::string_view("Session Id")))
							session = line.SkipWord(':').Trim(" ");

						if (line.StartsWith(std::string_view("Target Name")))
							target = line.SkipWord(':').Trim(" ");

						if (line.StartsWith(std::string_view("Initiator Scsi Device")))
							device = line.SkipWord(':').Trim(" ");

						if(!line.size())
							f(session, target,device);
					}
				}
			}

			static void AddTarget(std::string_view ip, std::string_view name)
			{
				if(ip.size())
					cmd(std::string("iscsicli QAddTargetPortal ") + std::string(ip));
				cmd(std::string("iscsicli QAddTarget ") + std::string(name));
			}

			static void Login(std::string_view ip, std::string_view name)
			{
				if (ip.size())
					cmd(std::string("iscsicli QAddTargetPortal ") + std::string(ip));
				cmd(std::string("iscsicli qlogintarget ") + std::string(name));
			}

			static void DirectLogin(std::string_view name, std::string_view ip)
			{
				cmd(std::string("iscsicli qaddtarget ") + std::string(name) + " " + std::string(ip));
				cmd(std::string("iscsicli qlogintarget ") + std::string(name));
			}

			static void Logout(std::string_view ip, std::string_view session)
			{
				if (ip.size())
					cmd(std::string("iscsicli QAddTargetPortal ") + std::string(ip));
				cmd(std::string("iscsicli logouttarget ") + std::string(session));
			}

			static void ResetPortal(std::string_view ip)
			{
				cmd(std::string("iscsicli RefreshTargetPortal ") + std::string(ip) + " 3260");
			}

			static void LogoutAll(std::string_view ip)
			{
				volrng::win::ISCSIClient::EnumerateMappings(ip, [](std::string_view session, std::string_view target, std::string_view device)
				{
					cmd(std::string("iscsicli logouttarget ") + std::string(session));
				});
			}
		};
	}
}