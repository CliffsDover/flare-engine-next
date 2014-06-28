/*
Copyright © 2011-2012 Clint Bellanger

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#include "CommonIncludes.h"
#include "ModManager.h"
#include "Settings.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

using namespace std;

Mod::Mod()
	: name("")
	, description("") {
}

Mod::~Mod() {
}

Mod::Mod(const Mod &mod)
	: name(mod.name)
	, description(mod.description)
	, depends(mod.depends) {
}

bool Mod::operator== (const Mod &mod) const {
	return this->name == mod.name;
}

bool Mod::operator!= (const Mod &mod) const {
	return !(*this == mod);
}

ModManager::ModManager() {
	loc_cache.clear();
	mod_dirs.clear();
	mod_list.clear();
	setPaths();

	vector<string> mod_dirs_other;
	getDirList(PATH_DEFAULT_DATA + "mods", mod_dirs);
	getDirList(PATH_DEFAULT_USER + "mods", mod_dirs_other);
	getDirList(PATH_DATA + "mods", mod_dirs_other);
	getDirList(PATH_USER + "mods", mod_dirs_other);

	for (unsigned i=0; i<mod_dirs_other.size(); ++i) {
		if (find(mod_dirs.begin(), mod_dirs.end(), mod_dirs_other[i]) == mod_dirs.end())
			mod_dirs.push_back(mod_dirs_other[i]);
	}

	loadModList();
	applyDepends();
}

/**
 * The mod list is in either:
 * 1. [PATH_CONF]/mods.txt
 * 2. [PATH_DATA]/mods/mods.txt
 * The mods.txt file shows priority/load order for mods
 *
 * File format:
 * One mod folder name per line
 * Later mods override previous mods
 */
void ModManager::loadModList() {
	ifstream infile;
	string line;
	string starts_with;
	bool found_any_mod = false;

	// Add the fallback mod by default
	// Note: if a default mod is not found in mod_dirs, the game will exit
	if (find(mod_dirs.begin(), mod_dirs.end(), FALLBACK_MOD) != mod_dirs.end()) {
		mod_list.push_back(loadMod(FALLBACK_MOD));
		found_any_mod = true;
	}

	// Add all other mods.
	string place1 = PATH_CONF + "mods.txt";
	string place2 = PATH_DATA + "mods/mods.txt";

	infile.open(place1.c_str(), ios::in);

	if (!infile.is_open()) {
		infile.open(place2.c_str(), ios::in);
	}
	if (!infile.is_open()) {
		fprintf(stderr, "Error during ModManager::loadModList() -- couldn't open mods.txt, to be located at \n");
		fprintf(stderr, "%s\n%s\n\n", place1.c_str(), place2.c_str());
	}

	while (infile.good()) {
		line = getLine(infile);

		// skip ahead if this line is empty
		if (line.length() == 0) continue;

		// skip comments
		starts_with = line.at(0);
		if (starts_with == "#") continue;

		// add the mod if it exists in the mods folder
		if (line != FALLBACK_MOD) {
			if (find(mod_dirs.begin(), mod_dirs.end(), line) != mod_dirs.end()) {
				mod_list.push_back(loadMod(line));
				found_any_mod = true;
			}
			else {
				fprintf(stderr, "Mod \"%s\" not found, skipping\n", line.c_str());
			}
		}
	}
	infile.close();
	if (!found_any_mod && mod_list.size() == 1) {
		fprintf(stderr, "Couldn't locate any Flare mod. ");
		fprintf(stderr, "Check if the game data are installed correctly. ");
		fprintf(stderr, "Expected to find the data in the $XDG_DATA_DIRS path, ");
		fprintf(stderr, "in /usr/local/share/flare/mods, ");
		fprintf(stderr, "or in the same folder as the executable. ");
		fprintf(stderr, "Try placing the mods folder in one of these locations.\n");
	}
}

/**
 * Find the location (mod file name) for this data file.
 * Use private loc_cache to prevent excessive disk I/O
 */
string ModManager::locate(const string& filename) {
	// if we have this location already cached, return it
	if (loc_cache.find(filename) != loc_cache.end()) {
		return loc_cache[filename];
	}

	// search through mods for the first instance of this filename
	string test_path;

	for (unsigned int i = mod_list.size(); i > 0; i--) {
		for (unsigned int j = 0; j < mod_paths.size(); j++) {
			test_path = mod_paths[j] + "mods/" + mod_list[i-1].name + "/" + filename;
			if (fileExists(test_path)) {
				loc_cache[filename] = test_path;
				return test_path;
			}
		}
	}

	// all else failing, simply return the filename
	return PATH_DATA + filename;
}

void amendPathToVector(const string &path, std::vector<std::string> &vec) {
	if (pathExists(path)) {
		if (isDirectory(path)) {
			getFileList(path, "txt", vec);
		}
		else {
			vec.push_back(path);
		}
	}
}

vector<string> ModManager::list(const string &path, bool full_paths) {
	vector<string> ret;
	string test_path;

	for (unsigned int i = 0; i < mod_list.size(); ++i) {
		for (unsigned int j = mod_paths.size(); j > 0; j--) {
			test_path = mod_paths[j-1] + "mods/" + mod_list[i].name + "/" + path;
			amendPathToVector(test_path, ret);
		}
	}

	// we don't need to check for duplicates if there are no paths
	if (ret.empty()) return ret;

	if (!full_paths) {
		// reduce the each file path down to be relative to mods/
		for (unsigned i=0; i<ret.size(); ++i) {
			ret[i] = ret[i].substr(ret[i].rfind(path), ret[i].length());
		}

		// remove duplicates
		for (unsigned i = 0; i < ret.size(); ++i) {
			for (unsigned j = 0; j < i; ++j) {
				if (ret[i] == ret[j]) {
					ret.erase(ret.begin()+j);
					break;
				}
			}
		}
	}

	return ret;
}

void ModManager::setPaths() {
	// set some flags if directories are identical
	bool uniq_path_data = PATH_USER != PATH_DATA;
	bool uniq_path_default_user = PATH_USER != PATH_DEFAULT_USER && PATH_DATA != PATH_DEFAULT_USER;
	bool uniq_path_default_data = PATH_USER != PATH_DEFAULT_DATA && PATH_DATA != PATH_DEFAULT_DATA && PATH_DEFAULT_USER != PATH_DEFAULT_DATA;

	mod_paths.push_back(PATH_USER);
	if (uniq_path_data) mod_paths.push_back(PATH_DATA);
	if (uniq_path_default_user) mod_paths.push_back(PATH_DEFAULT_USER);
	if (uniq_path_default_data) mod_paths.push_back(PATH_DEFAULT_DATA);
}

Mod ModManager::loadMod(std::string name) {
	Mod mod;
	ifstream infile;
	std::string starts_with, line, key, val;

	mod.name = name;

	for (unsigned i=0; i<mod_paths.size(); ++i) {
		std::string path = mod_paths[i] + "mods/" + name + "/settings.txt";
		infile.open(path.c_str(), ios::in);

		while (infile.good()) {
			line = getLine(infile);
			key = "";
			val = "";

			// skip ahead if this line is empty
			if (line.length() == 0) continue;

			// skip comments
			starts_with = line.at(0);
			if (starts_with == "#") continue;

			parse_key_pair(line, key, val);

			if (key == "description") {
				mod.description = val;
			}
			else if (key == "requires") {
				std::string dep;
				val = val + ',';
				while ((dep = popFirstString(val, ',')) != "") {
					mod.depends.push_back(dep);
				}
			}
		}
		if (infile.good()) {
			infile.close();
			break;
		}
		else {
			infile.close();
		}
	}

	return mod;
}

void ModManager::applyDepends() {
	std::vector<Mod> new_mods;
	bool finished = true;

	for (unsigned i=0; i<mod_list.size(); i++) {
		// skip the mod if it's already in the new_mods list
		if (find(new_mods.begin(), new_mods.end(), mod_list[i]) != new_mods.end()) {
			continue;
		}

		bool depends_met = true;

		for (unsigned j=0; j<mod_list[i].depends.size(); j++) {
			bool found_depend = false;

			// try to add the dependecy to the new_mods list
			for (unsigned k=0; k<new_mods.size(); k++) {
				if (new_mods[k].name == mod_list[i].depends[j]) {
					found_depend = true;
				}
				if (!found_depend) {
					// if we don't already have this dependency, try to load it from the list of available mods
					if (find(mod_dirs.begin(), mod_dirs.end(), mod_list[i].depends[j]) != mod_dirs.end()) {
						Mod new_depend = loadMod(mod_list[i].depends[j]);
						if (find(new_mods.begin(), new_mods.end(), new_depend) == new_mods.end()) {
							printf("Mod \"%s\" requires the \"%s\" mod. Enabling \"%s\" now.\n", mod_list[i].name.c_str(), mod_list[i].depends[j].c_str(), mod_list[i].depends[j].c_str());
							new_mods.push_back(new_depend);
							finished = false;
							break;
						}
					}
					else {
						fprintf(stderr, "Could not find mod \"%s\", which is required by mod \"%s\". Disabling \"%s\" now.\n", mod_list[i].depends[j].c_str(), mod_list[i].name.c_str(), mod_list[i].name.c_str());
						depends_met = false;
						break;
					}
				}
			}
			if (!depends_met) break;
		}

		if (depends_met) {
			if (find(new_mods.begin(), new_mods.end(), mod_list[i]) == new_mods.end()) {
				new_mods.push_back(mod_list[i]);
			}
		}
	}

	mod_list = new_mods;

	// run recursivly until no more dependencies need to be met
	if (!finished)
		applyDepends();
}

bool ModManager::haveFallbackMod() {
	for (unsigned i=0; i<mod_list.size(); ++i) {
		if (mod_list[i].name == FALLBACK_MOD)
			return true;
	}
	return false;
}

ModManager::~ModManager() {
}
