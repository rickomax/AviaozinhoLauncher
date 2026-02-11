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
#include <thread>

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
		SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 255);
	}
	else if (token == "lobby_update") {
		if (lobbyId == 0) {
			return;
		}
		std::string data;
		if (std::getline(ss, data, COMMAND_DELIMITER)) {
			SteamMatchmaking()->SetLobbyData(lobbyId, "name", data.c_str());
		}
		if (std::getline(ss, data, COMMAND_DELIMITER)) {
			SteamMatchmaking()->SetLobbyData(lobbyId, "map", data.c_str());
		}
		if (std::getline(ss, data, COMMAND_DELIMITER)) {
			SteamMatchmaking()->SetLobbyData(lobbyId, "clients", data.c_str());
		}
		if (std::getline(ss, data, COMMAND_DELIMITER)) {
			SteamMatchmaking()->SetLobbyData(lobbyId, "maxc", data.c_str());
		}
		//const char* name = SteamFriends()->GetPersonaName();
		//SteamMatchmaking()->SetLobbyData(lobbyId, "name", name);
	}
	else if (token == "server_list") {
		serverMap.clear();
		expectedLobbies = 0;
		pendingLobbies = 0;
		serverListRequestId++;
		serverListInProgress = true;
		SteamMatchmaking()->RequestLobbyList();
	}
	else if (token == "unhost") {
		if (lobbyId == 0) {
			return;
		}
		SteamFriends()->ClearRichPresence();
		SteamMatchmaking()->SetLobbyJoinable(lobbyId, false);
		SteamMatchmaking()->LeaveLobby(lobbyId);
		lobbyId = 0;
	}
}