#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <iostream>
#include <sstream>
#include "steam/steam_api.h"
#include "pipe.h"

#define COMMAND_DELIMITER ' '

using namespace std;

void ParseRequest() {
	stringstream ss(pipe_buffer);
	string token;
	if (getline(ss, token, COMMAND_DELIMITER)) {
		if (token == "unlock_achievement") {
			if (getline(ss, token, COMMAND_DELIMITER)) {
				if (!SteamUserStats()->SetAchievement(token.c_str())) {
					cout << "Error setting achievement\n";
				}
				if (!SteamUserStats()->StoreStats()) {
					cout << "Error storing stats\n";
				}
			}
		}
		else if (token == "update_stat") {
			if (getline(ss, token, COMMAND_DELIMITER)) {
				char statName[1024];
				strncpy(statName, token.c_str(), 1024);
				if (getline(ss, token, COMMAND_DELIMITER)) {
					int value = atoi(token.c_str());
					if (!SteamUserStats()->SetStat(statName, value)) {
						cout << "Error setting stat\n";
					}
					if (!SteamUserStats()->StoreStats()) {
						cout << "Error storing stats\n";
					}
				}
			}
		}
	}
}

#endif