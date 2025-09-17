#pragma once
#include <string>
#include <vector>
#include <fstream>  
#include <sstream>  
#include <stdexcept>
#include <utility>  

using namespace std;

vector<pair<string, string>> read_kv_pairs(const string& file_path) {
	vector<pair<string, string>> result;
	ifstream file(file_path);
	if (file) {
		string line;
		while (getline(file, line)) {
			istringstream ss(line);
			string key, value;
			if (getline(ss, key, ',') && getline(ss, value)) {
				result.emplace_back(key, value);
			}
		}
	}
	return result;
}

void write_kv_pairs(const string& file_path, const vector<pair<string, string>>& pairs)
{
	ofstream file(file_path, ios::trunc);
	if (!file) {
		throw runtime_error("Could not open file for writing: " + file_path);
	}
	for (const auto& [key, value] : pairs) {
		file << key << ',' << value << '\n';
	}
	file.flush();
}

string find_value_or_default(
	const vector<pair<string, string>>& pairs,
	const string& key,
	const string& default_value = "")
{
	for (const auto& kv : pairs) {
		if (kv.first == key)
			return kv.second;
	}
	return default_value; // fallback
}