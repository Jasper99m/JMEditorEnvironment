#include "SimEnvironment.h"

SimResourceManager::SimResourceManager() {

}
SimResourceManager::~SimResourceManager() {

}


void SimResourceManager::save(JMwindow* window) {
	if (projectFilePath.empty()) {

		std::string tmpPath = window->saveFileDialog(fileExtension);
		if (tmpPath.empty()) { return; }

		std::string fileName = tmpPath;

		size_t slashPos = fileName.find_last_of("/\\");
		fileName = fileName.substr(slashPos + 1);

		if (fileName.size() >= fileExtension.size() && fileName.compare(fileName.size() - fileExtension.size(), fileExtension.size(), fileExtension) == 0) {
			projectName = fileName.erase(fileName.size() - fileExtension.size());
			projectFilePath = tmpPath;
		}
		else {
			projectName = fileName;
			projectFilePath = tmpPath + fileExtension;
		}
	}

	save(projectFilePath);
}

void SimResourceManager::saveAs(JMwindow* window) {
	projectFilePath = "";
	save(window);
}


void SimResourceManager::load(JMwindow* window) {

	std::string tmpPath = window->loadFileDialog(fileExtension);
	if (tmpPath.empty()) { return; }

	std::string fileName = tmpPath;

	size_t slashPos = fileName.find_last_of("/\\");
	fileName = fileName.substr(slashPos + 1);

	if (fileName.size() >= fileExtension.size() && fileName.compare(fileName.size() - fileExtension.size(), fileExtension.size(), fileExtension) == 0) {
		projectName = fileName.erase(fileName.size() - fileExtension.size());
		projectFilePath = tmpPath;
	}
	else {
		projectName = fileName;
		projectFilePath = tmpPath + fileExtension;
	}

	load(projectFilePath);
}

void SimResourceManager::save(std::string filepath) {
	auto saveThread = [this, filepath]() {
		infoBarMessage = "Saving to file path: " + filepath;
		std::ofstream file{ filepath, std::ios::binary };
		if (!file.is_open()) {
			infoBarMessage += "  Error: Unable to open file.";
			std::cerr << "SimResourceManager::save() Error: Failed to open file: " << projectFilePath << std::endl;
			return;
		}

		for (SimEnvironment* i : allEnvironments) {
			i->saveEnvironment(file);
		}

		file.close();
		infoBarMessage += "    Done.";
		};

	std::thread thread(saveThread);
	thread.detach();
}

void SimResourceManager::load(std::string filepath) {
	std::ifstream file{ filepath, std::ios::binary };
	infoBarMessage = "Loading from file path: " + filepath;
	if (!file.is_open()) {
		infoBarMessage = "	Error: Failed to open file.";
		std::cerr << "SimResourceManager::load() Error: Failed to open file: " << projectFilePath << std::endl;
		return;
	}

	for (SimEnvironment* i : allEnvironments) {
		i->loadEnvironment(file);
		file.ignore();
	}

	file.close();
	infoBarMessage += "    Done.";
}


void SimResourceManager::recoverBackup() {
	load(backupFilePath + fileExtension);
	projectFilePath = "";
	projectName = "Unsaved project";
}
void SimResourceManager::saveBackup() {
	save(backupFilePath + fileExtension);
}

void SimResourceManager::makeNameUnique(SimObject* obj) {

	std::string newName = obj->name;
	while (findObject(newName) != nullptr) {
		if (std::isdigit(newName.back())) {
			std::string newNum = "";
			while (!newName.empty() && std::isdigit(newName.back())) {
				newNum = newName.back() + newNum;
				newName.pop_back();
			}

			newNum = std::to_string(std::stoi(newNum) + 1);

			while (newNum.length() < 3) {
				newNum = "0" + newNum;
			}

			newName += newNum;
		}
		else {
			newName += " - 001";
		}
	}

	obj->name = newName;
}

SimObject* SimResourceManager::findObject(std::string name) {

	for (SimObject* i : allSimObjects()) {
		if (i->name == name) { return i; }
	}
	return nullptr;
}

std::vector<SimObject*> SimResourceManager::allSimObjects() {
	std::vector<SimObject*> allObjs;
	for (SimEnvironment* i : allEnvironments) {
		for (SimObject* j : i->allObjects) {
			allObjs.push_back(j);
		}
	}
	return allObjs;
}