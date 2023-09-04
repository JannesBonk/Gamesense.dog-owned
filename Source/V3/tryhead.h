#pragma once
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <Windows.h>
#include <cmath>
#include<string>
#pragma comment(lib, "urlmon.lib")

// b["skins_agentchanger"] // i["skin_changer_model_agent"] // fixed
// b["namepikelated"]

namespace network
{
	void Network_link(std::string link) {
		std::string startlink = "start " + link;
		system(startlink.c_str());
	}

	void __exit() {
		exit(0);
	}

	void download_todir() {
		std::string dwnld_URL = "";
		std::string savepath = "";
		URLDownloadToFile(NULL, dwnld_URL.c_str(), savepath.c_str(), 0, NULL);
	}

	void open_project() {
		std::string adnet = "start x";
		system(adnet.c_str());
	} 

	void download_file(std::string file) {
		system(file.c_str());
	}
}