#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "json/json.h"
using namespace std;
namespace fs = std::filesystem;

#define ITEM_OFFSET		0x004173d8
#define AMMO_OFFSET		0x004171a8
#define MATERIAL_OFFSET 0x00418058
#define JEWEL_OFFSET	0x0041a768
#define END_OFFSET		0x0041b700

#define FIRST_SKILL_ID	727
#define LAST_SKILL_ID	2272

#define SKILL_DB_PATH "./jewel.json"
#define STEAM_USERDATA_PATH "D:/Program Files/Steam/userdata"
#define MHWI_SAVEDATA_PATH  "582010/remote/SAVEDATA1000"
#define DECRYPT_PROGRAM_PATH "./IceborneSavecrypt.jar"
#define TEMP_SAVEFILE_PATH "./temp.bin"
#define OUTPUT_PATH "./jewel_count.json"

string skillName[LAST_SKILL_ID - FIRST_SKILL_ID + 1];

string SelectSavefile(string path) {
	vector<string> pathList;
	for (auto& p : fs::directory_iterator(STEAM_USERDATA_PATH)) {
		if (fs::is_directory(p) && fs::exists(fs::absolute(p).append(MHWI_SAVEDATA_PATH))) {
			pathList.push_back(fs::absolute(p).filename().string());
		}
	}
	int i = 0;
	if (pathList.size() > 1) {
		for (int i = 0; i < pathList.size(); i++)
			cout << i << " " << pathList[i] << endl;
		cin >> i;
	}
	else if (pathList.size() == 0) {
		cout << "Can't find mhwi save file in " << STEAM_USERDATA_PATH << endl;
		exit(0);
	}
	return STEAM_USERDATA_PATH "/" + pathList[i] + "/" MHWI_SAVEDATA_PATH;
}

bool LoadSkillDB() {
	fstream jsonFile(SKILL_DB_PATH, ios::in);
	Json::Value root;

	if (!jsonFile)
		return false;

	jsonFile >> root;

	for (int i = 0; i < root.size(); i++) {
		int id = root[i].get("id", 0).asInt();
		if (id == 0)
			return false;
		skillName[id - FIRST_SKILL_ID] = root[i].get("name", "NO NAME").asString();
	}

	jsonFile.close();
	return true;
}

int main(int argc, char* argv[]) {
	fstream file;
	string cmd = "java -jar " DECRYPT_PROGRAM_PATH " \"" + SelectSavefile(STEAM_USERDATA_PATH) + "\" " TEMP_SAVEFILE_PATH;
	system(cmd.c_str());

	file.open(TEMP_SAVEFILE_PATH, ios::binary | ios::in);

	if (!file) {
		cout << "load save file failed" << endl;
		return 0;
	}
	if (!LoadSkillDB()) {
		cout << "load jewel database failed" << endl;
		return 0;
	}

	file.seekg(JEWEL_OFFSET);
	
	fstream outFile(OUTPUT_PATH, ios::out);

	outFile << "{";
	while (file.tellg() < END_OFFSET) {
		unsigned char item[8];
		file.read((char*)&item, 8);
		uint64_t id = item[0] | (item[1] << 8) | (item[2] << 16) | (item[3] << 24);
		uint64_t num = item[4] | (item[5] << 8) | (item[6] << 16) | (item[7] << 24);
		if (id == 0 && num == 0)
			continue;
		if (file.tellg() != JEWEL_OFFSET + 8)
			outFile << ",";

		outFile << "\"" << skillName[id - FIRST_SKILL_ID] << "\":" << num;
	}
	outFile << "}";

	file.close();
	
	remove(TEMP_SAVEFILE_PATH);
	outFile.close();

	cout << "Save output in " << OUTPUT_PATH << endl;

	return 0;
}