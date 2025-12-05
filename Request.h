#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <optional>

#include <windows.h>
#include <shlobj.h>

#include "steam/steam_api.h"
#include "Pipe.h"
#include "Downloader.h"
#include "Static.h"
#include "NetPipe.h"
#include "SteamProtocol.h"

#define COMMAND_DELIMITER ' '

using namespace std;

std::optional<std::string> FindSingleBspFilename(const std::filesystem::path& folder)
{
	namespace fs = std::filesystem;

	std::error_code ec;
	if (!fs::exists(folder, ec) || !fs::is_directory(folder, ec)) {
		return std::nullopt;
	}

	std::optional<std::string> found;

	for (const fs::directory_entry& de : fs::directory_iterator(folder, ec)) {
		if (ec) return std::nullopt;

		if (!de.is_regular_file(ec)) continue;

		std::string ext = de.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (ext == ".bsp") {
			if (found.has_value()) {
				return std::nullopt;
			}
			found = de.path().filename().string(); 
		}
	}

	return found;
}

bool ShellCopyFile(const std::filesystem::path& fromRel,
	const std::filesystem::path& toRel)
{
	namespace fs = std::filesystem;

	fs::path fromAbs = fs::absolute(fromRel);
	fs::path toAbs = fs::absolute(toRel);

	if (!fs::is_directory(fromAbs)) {
		std::wstring fromStr = fromAbs.c_str();
		std::vector<wchar_t> fromBuf(fromStr.size() + 2, L'\0');
		std::copy(fromStr.begin(), fromStr.end(), fromBuf.begin());

		std::wstring toStr = toAbs.c_str();

		SHFILEOPSTRUCTW op{};
		op.wFunc = FO_COPY;
		op.pFrom = fromBuf.data();
		op.pTo = toStr.c_str();
		op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

		return SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
	}

	fs::path starPattern = fromAbs / L"*";
	std::wstring fromStar = starPattern.c_str();

	std::vector<wchar_t> fromBuf(fromStar.size() + 2, L'\0');
	std::copy(fromStar.begin(), fromStar.end(), fromBuf.begin());

	std::wstring toStr = toAbs.c_str();

	SHFILEOPSTRUCTW op{};
	op.wFunc = FO_COPY;
	op.pFrom = fromBuf.data();
	op.pTo = toStr.c_str();
	op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

	return SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
}

template <typename IntT>
static std::optional<IntT> TryParseInt(const std::string& s, int base = 10) {
	try {
		size_t idx = 0;
		long long v = std::stoll(s, &idx, base);
		if (idx == s.size()) return static_cast<IntT>(v);
	}
	catch (...) {}
	return std::nullopt;
}

static std::string ToIso8601UTC(std::time_t t) {
	std::tm gmt{};
#ifdef _WIN32
	gmtime_s(&gmt, &t);
#else
	gmt = *std::gmtime(&t);
#endif
	std::ostringstream oss;
	oss << std::put_time(&gmt, "%Y-%m-%d");
	return oss.str();
}

void PumpPipe() {
	if (Pipe_AvailableBytes() == 0 || !Pipe_Read()) {
		return;
	}
	std::stringstream ss(pipe_buffer);
	std::string token;
	if (!std::getline(ss, token, COMMAND_DELIMITER)) {
		return;
	}
	if (token == "unlock_achievement") {
		std::string ach;
		if (std::getline(ss, ach, COMMAND_DELIMITER)) {
			if (!SteamUserStats()->SetAchievement(ach.c_str())) {
				ShowError("Error setting achievement");
			}
			if (!SteamUserStats()->StoreStats()) {
				ShowError("Error storing stats");
			}
		}
	}
	else if (token == "update_stat") {
		std::string statName;
		if (std::getline(ss, statName, COMMAND_DELIMITER)) {
			std::string valueStr;
			if (std::getline(ss, valueStr, COMMAND_DELIMITER)) {
				auto val = TryParseInt<int>(valueStr);
				if (!val.has_value()) {
					ShowError("Invalid stat value");
				}
				else {
					if (!SteamUserStats()->SetStat(statName.c_str(), *val)) {
						ShowError("Error setting stat");
					}
					if (!SteamUserStats()->StoreStats()) {
						ShowError("Error storing stats");
					}
				}
			}
		}
	}
	else if (token == "workshop_install") {
		std::string idStr;
		if (std::getline(ss, idStr, COMMAND_DELIMITER)) {
			auto value = TryParseInt<long long>(idStr);
			if (!value.has_value()) {
				Pipe_Write("\x04");
				return;
			}
			Downloader downloader(STEAM_APP_ID);
			downloader.Init();
			downloader.EnsureSubscribedAndDownload((PublishedFileId_t)*value, false);
			downloader.WaitUntilInstalled((PublishedFileId_t)*value);
			std::string path;
			if (downloader.IsItemInstalled(*value, &path)) {
				std::filesystem::path gameDir = "workshop_" + idStr;
				std::filesystem::path outDir = gameDir;
				if (ShellCopyFile(path, outDir.string())) {
					Pipe_Write(gameDir.string().c_str());
					std::filesystem::path mapDir = outDir / "maps";
					auto map = FindSingleBspFilename(mapDir);
					if (map) {
						Pipe_Write(map->c_str());
					}
					else {
						Pipe_Write("\x04");
					}
				}
				else {
					Pipe_Write("\x04");
				}
			}
			else {
				Pipe_Write("\x04");
			}
		}
	}
	else if (token == "workshop_mods") {
		Downloader downloader(STEAM_APP_ID);
		downloader.Init();
		std::vector<WorkshopItemInfo> mods = downloader.EnumerateAll(
			k_EUGCQuery_RankedByPublicationDate,
			k_EUGCMatchingUGCType_Items,
			200
		);
		for (WorkshopItemInfo& mod : mods) {
			if (mod.banned) {
				continue;
			}
			Pipe_Write("%llu", mod.id);
			Pipe_Write("%s", mod.title.c_str());
			Pipe_Write("%s", mod.ownerPersonaName.c_str());
			Pipe_Write("%s", mod.description.c_str());
			Pipe_Write("%s", ToIso8601UTC(mod.timeCreated).c_str());
			Pipe_Write("%s", mod.previewURL.c_str());
			Pipe_Write(mod.subscribed ? "1" : "0");
		}
		Pipe_Write("\x04");
	}
	else if (token == "languages") {
		for (auto& kvp : languageMap) {
			Pipe_Write("%s", kvp.first.c_str());
			Pipe_Write("localization/%s", kvp.second.c_str());
		}
		Pipe_Write("\x04");
	}
	else if (token == "host") {
		std::string commandLine = std::format("+toggleconsole +connect steam-conn|{}", static_cast<unsigned long long>(SteamUser()->GetSteamID().ConvertToUint64()));
		SteamFriends()->SetRichPresence("connect", commandLine.c_str());
		SteamFriends()->SetRichPresence("status", "In match");
		SteamFriends()->SetRichPresence("steam_display", "#Status_InMatch");
	}
	else if (token == "unhost") {
		SteamFriends()->ClearRichPresence();
	}
}