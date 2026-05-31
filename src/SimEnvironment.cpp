#include "SimEnvironment.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <unordered_map>

SimEnvironment::SimEnvironment() {
	manager = &SimResourceManager::getInstance();
	manager->allEnvironments.push_back(this);

	unsigned int logicCores = std::thread::hardware_concurrency();
	if (logicCores != 0) { simThreadNum = (int)std::min(logicCores, 32u); }
}

SimEnvironment::~SimEnvironment() {
	deselectAll();
	for (SimObject* i : allObjects) { delete(i); }

	while (!undoVector.empty()) { popObjectVector(undoVector); }
	while (!redoVector.empty()) { popObjectVector(redoVector); }

	if (viewer != nullptr) { delete(viewer); }

	if (noObjSelectedMenu != nullptr) { delete(noObjSelectedMenu); }

	if (objectDataBackground != nullptr) { delete(objectDataBackground); }

	if (propertiesMenu != nullptr) {
		propertiesMenu->removeAllTabs();
		delete(propertiesMenu);
	}

	for (guiGuideCenter* i : xGuideCenters) { delete(i); }
	xGuideCenters.clear();
	for (guiGuideCenter* i : yGuideCenters) { delete(i); }
	yGuideCenters.clear();

	auto i = std::find(manager->allEnvironments.begin(), manager->allEnvironments.end(), this);
	if (i != manager->allEnvironments.end()) { manager->allEnvironments.erase(i); }
}

void SimEnvironment::setupViewer(JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY) {
	viewer = new ObjectViewer(this, Window, PosX, PosY, SizeX, SizeY);
}
void SimEnvironment::setupViewer_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {
	if (x1 > xGuides.size() || x2 > xGuides.size() || y1 > yGuides.size() || y2 > yGuides.size()) { return; }
	setupViewer(Window, (xGuides[x1].pixelPos + xGuides[x2].pixelPos) / 2.0f, (yGuides[y1].pixelPos + xGuides[y2].pixelPos) / 2.0f,
		abs(xGuides[x1].pixelPos - xGuides[x2].pixelPos) - guideMargin, abs(yGuides[y1].pixelPos - yGuides[y2].pixelPos) - guideMargin);
	allignToGuides(viewer, x1, x2, y1, y2);
}
void SimEnvironment::displayViewer() {
	if (viewer == nullptr)
		return;

	viewer->display();
	checkWindowResized(viewer->window->window);

	//check if any of the objects have been clicked on in the viewer
	if (selectInViewer && viewer->gr->mouseClicked() && viewer->mouseOver()) {
		SimObject* clicked = nullptr;
		float distLast = -1.0f;
		for(SimObject* i : allObjects) {
			if (i->hide) {
				continue;
			}
			float dist = viewer->checkMouseOverObject(i);
			if (dist > 0.0f && (distLast < 0.0f || dist < distLast) && i->viewportSelectable) {
				distLast = dist;
				clicked = i;
			}

		}
		if (clicked != nullptr) {
			if (clicked == selectedObj) { return; }
			if (!(viewer->gr->keyDown(GLFW_KEY_LEFT_SHIFT) || viewer->gr->keyDown(GLFW_KEY_LEFT_CONTROL))) { deselectAll(); }

			selectObject(clicked);
		}
		else if(deselectOnClick){
			deselectAll();
		}
	}
}

void SimEnvironment::addObject(SimObject* obj) {
	std::lock_guard<std::mutex> lock(renderMtx);

	obj->handler = this;

	manager->makeNameUnique(obj);

	allObjects.push_back(obj);
	
	if (selectorList != nullptr) {
		obj->name = selectorList->addItem(obj->name);
		selectorList->addItemSelectedEventListener(obj->name, [this, obj](JMeventDispatcher::Event* E) { this->selectObject(obj); });
		selectorList->addItemDeselectedEventListener(obj->name, [this, obj](JMeventDispatcher::Event* E) { this->deselectObject(obj); });
	}

	if (obj->isSelected && selectedObj == nullptr) {
		obj->isSelected = false;
		selectObject(obj);
	}
	else {
		obj->isSelected = false;
	}
}
void SimEnvironment::deleteObject(SimObject* obj) {
	if (obj == nullptr) { return; }
	std::lock_guard<std::mutex> lock(renderMtx);

	deselectObject(obj);

	auto i = std::find(allObjects.begin(), allObjects.end(), obj);

	if (i != allObjects.end()) { allObjects.erase(i); }

	if (selectorList != nullptr) { selectorList->removeItem(obj->name); }

	delete(obj);
}

void SimEnvironment::focusViewerOnObject(SimObject* obj) {
	if (viewer == nullptr || obj == nullptr)
		return;

	viewer->focusOnObject(obj);
}

void SimEnvironment::selectObject(SimObject* obj) {
	if (obj == nullptr || ObjectsFrozen || obj->isSelected || obj->isSecondSelected) { return; }

	auto i = std::find(allObjects.begin(), allObjects.end(), obj);
	if (i == allObjects.end()) { return; }

	if (selectedObj == nullptr) {
		selectedObj = obj;
		obj->isSelected = true;
		obj->isSecondSelected = false;
		setObjectMenus();
	}
	else {
		obj->isSecondSelected = true;
		SelectedObjects.push_back(obj);
	}
	
	if (selectorList != nullptr) {
		selectorList->selectItemWithoutEvent(obj->name);
	}
}
void SimEnvironment::selectObject(std::string name) {
	if (ObjectsFrozen || name.empty()) { return; }

	for (SimObject* i : allObjects) {
		if (i->name == name) {
			selectObject(i);
			return;
		}
	}
}

void SimEnvironment::deselectObject(SimObject* obj) {
	if (obj == nullptr || ObjectsFrozen || !(obj->isSelected || obj->isSecondSelected)) { return; }

	obj->isSelected = false;
	obj->isSecondSelected = false;

	if (obj == selectedObj) {
		selectedObj = nullptr;
		setObjectMenus();
	}
	else {
		auto i = std::find(SelectedObjects.begin(), SelectedObjects.end(), obj);
		if (i != SelectedObjects.end()) {
			SelectedObjects.erase(i);
		}
	}

	if (selectorList != nullptr) {
		selectorList->deselectItemWithoutEvent(obj->name);
	}

}
void SimEnvironment::deselectObject(std::string name) {
	if (name.empty() || ObjectsFrozen) { return; }

	for (SimObject* i : allObjects) {
		if (i->name == name) {
			deselectObject(i);
			return;
		}
	}
}
void SimEnvironment::deselectAll() {
	if (ObjectsFrozen) { return; }

	for (SimObject* i : allObjects) {
		i->isSelected = false;
		i->isSecondSelected = false;
	}

	SelectedObjects.clear();

	selectedObj = nullptr;

	if (selectorList != nullptr) { selectorList->deselectAllWithoutEvent(); }

	setObjectMenus();
}

void SimEnvironment::deleteAll() {
	std::lock_guard<std::mutex> lock(renderMtx);
	if (ObjectsFrozen) { return; }

	deselectAll();
	
	for (SimObject* i : allObjects) {
		delete(i);
	}
	allObjects.clear();

	if (selectorList != nullptr) { selectorList->removeAllItems(); }
}
void SimEnvironment::displayGui() {
	handleGuideMovement();
	displayViewer();
	displaySelector();
	displayPropertiesMenu();
	displayObjectDataWindow();
}

void SimEnvironment::freezeObjects() {
	ObjectsFrozen = true;
	if (selectorList != nullptr) {
		selectorList->allowInput = false;
	}
	setInfoBarMessage("Object selection frozen.");
}

void SimEnvironment::unfreezeObjects() {
	ObjectsFrozen = false;
	if (selectorList != nullptr) {
		selectorList->allowInput = true;
	}
	setInfoBarMessage("Object selection unfrozen.");
}

bool SimEnvironment::objectsFrozen() const{
	return ObjectsFrozen;
}

void SimEnvironment::renderObjects(JMGraphics* gr) {
	std::lock_guard<std::mutex> lock(renderMtx);

	for(SimObject* i : allObjects) {
		if (i->hide && !i->isSelected) {
			continue;
		}

		gr->pushMatrix();
		gr->multiplyMatrix(i->positionMatrix());
		i->renderMatrix = gr->transformationMatrix();

		i->display(gr);

		//render a box around it if selected
		if ((i->isSelected || i->isSecondSelected) && selectedBox) {
			gr->noFill();
			gr->strokeWeight(2.0f);
			if (i == selectedObj) { gr->stroke(0.0f, 1.0f, 0.2f); }
			else { gr->stroke(1.0f, 0.2f, 0.0f); }

			if (i->hide) { gr->stroke(0.07f, 0.4f, 0.15f); }
			
			gr->box((float)i->sizex() + 6.0f, (float)i->sizey() + 6.0f, (float)i->sizez() + 6.0f);
		}
		gr->popMatrix();
	}
}

GuiElementHandler* SimEnvironment::setupNoObjMenuDefault(TabbedMenu* propertiesMenu) {
	noObjSelectedMenu = new GuiElementHandler(propertiesMenu->window);
	Message* noObjMessage = new Message("No object selected", propertiesMenu->width() / 2.0f, propertiesMenu->height() / 2.0f - 150.0f);
	noObjSelectedMenu->addElement(noObjMessage);
	return noObjSelectedMenu;
}


void SimEnvironment::setupPropertiesMenu(JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY) {
	propertiesMenu = new TabbedMenu(Window, PosX, PosY, SizeX, SizeY);
	propertiesMenu->bottomTabs = true;

	if (setupNoObjSelectedMenu != nullptr) {
		noObjSelectedMenu = setupNoObjSelectedMenu(propertiesMenu, this);
	}
	else {
		noObjSelectedMenu = setupNoObjMenuDefault(propertiesMenu);
	}

	setObjectMenus();
}
void SimEnvironment::setupPropertiesMenu_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {
	if (x1 > xGuides.size() || x2 > xGuides.size() || y1 > yGuides.size() || y2 > yGuides.size()) { return; }
	setupPropertiesMenu(Window, (xGuides[x1].pixelPos + xGuides[x2].pixelPos) / 2.0f, (yGuides[y1].pixelPos + xGuides[y2].pixelPos) / 2.0f,
		abs(xGuides[x1].pixelPos - xGuides[x2].pixelPos) - guideMargin, abs(yGuides[y1].pixelPos - yGuides[y2].pixelPos) - guideMargin);
	allignToGuides(propertiesMenu, x1, x2, y1, y2);
}
void SimEnvironment::displayPropertiesMenu() {
	if (propertiesMenu == nullptr)
		return;

	propertiesMenu->display();
	checkWindowResized(propertiesMenu->window->window);
}

void SimEnvironment::setObjectMenus() {
	if (propertiesMenu == nullptr) { return; }

	propertiesMenu->removeAllTabs();

	if (selectedObj == nullptr) {
		propertiesMenu->addMenu(noObjSelectedMenu, "No object selected");
		return;
	}

	if (selectedObj->generalMenu == nullptr) { selectedObj->setupMenus(propertiesMenu); }
	if (selectedObj->generalMenu != nullptr) { propertiesMenu->addMenu(selectedObj->generalMenu, "General"); }
	if (selectedObj->menu1 != nullptr) { propertiesMenu->addMenu(selectedObj->menu1, selectedObj->menu1Name); }
	if (selectedObj->menu2 != nullptr) { propertiesMenu->addMenu(selectedObj->menu2, selectedObj->menu2Name); }
}

void SimEnvironment::setupSelector(JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY) {
	selector = new GuiElementHandler(Window);
	selector->xOffset = PosX;
	selector->yOffset = PosY;

	selector->addElement(new BackgroundBox(0.0f, 0.0f, SizeX, SizeY, "Objects"));
	selector->addElement(selectorList = new ListMenu(0.0f, -10.0f, SizeX - 12.0f, SizeY - 30.0f));
}
void SimEnvironment::setupSelector_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {
	if (x1 > xGuides.size() || x2 > xGuides.size() || y1 > yGuides.size() || y2 > yGuides.size()) { return; }
	setupSelector(Window, (xGuides[x1].pixelPos + xGuides[x2].pixelPos) / 2.0f, (yGuides[y1].pixelPos + xGuides[y2].pixelPos) / 2.0f,
		abs(xGuides[x1].pixelPos - xGuides[x2].pixelPos) - guideMargin, abs(yGuides[y1].pixelPos - yGuides[y2].pixelPos) - guideMargin);
	allignToGuides(selector, x1, x2, y1, y2);
}
void SimEnvironment::displaySelector() {
	if (selector == nullptr)
		return;

	selector->display();
	checkWindowResized(selector->window->window);
}

void SimEnvironment::setupObjectDataWindow(JMwindow* Window, float posX, float posY, float sizeX, float sizeY) {
	objectDataBackground = new GuiElementHandler(Window);
	objectDataBackground->xOffset = posX;
	objectDataBackground->yOffset = posY;
	objectDataBackground->addElement(new BackgroundBox(0.0f, 0.0f, sizeX, sizeY, "Object Data"));
	objectDataSizeX = sizeX;
	objectDataSizeY = sizeY;
}
void SimEnvironment::setupObjectDataWindow_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {
	if (x1 > xGuides.size() || x2 > xGuides.size() || y1 > yGuides.size() || y2 > yGuides.size()) { return; }
	setupObjectDataWindow(Window, (xGuides[x1].pixelPos + xGuides[x2].pixelPos) / 2.0f, (yGuides[y1].pixelPos + xGuides[y2].pixelPos) / 2.0f,
		abs(xGuides[x1].pixelPos - xGuides[x2].pixelPos) - guideMargin, abs(yGuides[y1].pixelPos - yGuides[y2].pixelPos) - guideMargin);
	allignToGuides(objectDataBackground, x1, x2, y1, y2);
}
void SimEnvironment::displayObjectDataWindow() {
	if (objectDataBackground == nullptr) {
		return;
	}

	objectDataBackground->display();
	checkWindowResized(objectDataBackground->window->window);

	if (selectedObj == nullptr) {
		return;
	}

	if (objectDataBackground->xSize != 0.0f) { objectDataSizeX = objectDataBackground->xSize; }
	if (objectDataBackground->ySize != 0.0f) { objectDataSizeY = objectDataBackground->ySize; }

	selectedObj->setupDataWindowInternal(objectDataBackground->window, objectDataSizeX, objectDataSizeY);

	JMGraphics* gr = objectDataBackground->window->window;

	gr->pushMatrix();
	gr->translate(objectDataBackground->xOffset, objectDataBackground->yOffset);
	selectedObj->displayDataWindow(objectDataBackground->window, gr->mouseX() - objectDataBackground->xOffset, gr->mouseY() - objectDataBackground->yOffset, objectDataSizeX, objectDataSizeY);
	gr->popMatrix();
}

void SimEnvironment::displayInfoBar(JMwindow* window) {
	JMGraphics* gr = window->window;
	float barWidth = 14.0f;
	if(!yGuides.empty()){ barWidth = yGuides[0].pixelPos; }

	gr->noStroke();
	gr->fill(window->color_titleBar);
	gr->rect(window->width() / 2.0f, barWidth / 2.0f, window->width(), barWidth);
	gr->setFont(window->font_standard);
	gr->textSize(1.0f);
	gr->fill(window->color_lines1);
	gr->text(manager->projectName + "    " + manager->infoBarMessage, 12.0f, barWidth / 2.0f -4.0f);
}

SimObject* SimEnvironment::selectedObject()  const {
	return selectedObj;
}

std::vector<SimObject*> SimEnvironment::selectedObjects()  const {
	return SelectedObjects;
}

std::vector<SimObject*> SimEnvironment::selectedObjects(std::string category)  const {
	std::vector<SimObject*> objects;
	for (SimObject* i : SelectedObjects) {
		if (i->objectCategory() == category)
			objects.push_back(i);
	}
	return objects;
}

std::vector<SimObject*> SimEnvironment::allInCategory(std::string category)  const {
	std::vector<SimObject*> objects;
	
	for (SimObject* i : allObjects) {
		if (i->objectCategory() == category) {
			objects.push_back(i);
		}
	}

	return objects;
}

std::vector<SimObject*> SimEnvironment::copyObjects()  const {
	std::vector<SimObject*> objects;

	for (SimObject* i : allObjects) {
		objects.push_back(i->clone());
	}
	return objects;
}


std::string SimEnvironment::setObjectName(SimObject* obj, std::string newName) {
	if (obj == nullptr|| obj->name == newName) { return "";  }
	
	if (selectorList == nullptr) {
		obj->name = newName;
		return newName;
	}

	selectorList->removeItem(obj->name);
	obj->name = selectorList->addItem(newName);
	selectorList->addItemSelectedEventListener(obj->name, [this, obj](JMeventDispatcher::Event* E) { this->selectObject(obj); });
	selectorList->addItemDeselectedEventListener(obj->name, [this, obj](JMeventDispatcher::Event* E) { this->deselectObject(obj); });
	
	return obj->name;
}

void SimEnvironment::addUndoStep() {
	copyObjectsToVector(undoVector);
	while (!redoVector.empty()) {
		popObjectVector(redoVector);
	}

	while (undoVector.size() > undoSteps) {
		popFrontObjectVector(undoVector);
	}
}
void SimEnvironment::undo() {
	if (undoVector.empty() || ObjectsFrozen)
		return;

	copyObjectsToVector(redoVector);
	copyObjectsFromVector(undoVector);

	while (redoVector.size() > redoSteps) {
		popFrontObjectVector(redoVector);
	}
	setInfoBarMessage("Undo steps remaining: " + std::to_string(undoVector.size()));
}
void SimEnvironment::redo() {
	if (redoVector.empty() || ObjectsFrozen)
		return;

	copyObjectsToVector(undoVector);
	copyObjectsFromVector(redoVector);

	setInfoBarMessage("Redo steps remaining: " + std::to_string(redoVector.size()));
}

void SimEnvironment::copyObjectsToVector(std::vector<std::vector<SimObject*>>& vec) {
	std::vector<SimObject*> objects;

	for(SimObject* i : allObjects) {
		objects.push_back(i->clone());
	}

	vec.push_back(objects);
}

void SimEnvironment::copyObjectsFromVector(std::vector<std::vector<SimObject*>>& vec) {
	if (vec.empty()) { return; }

	deleteAll();

	std::vector<SimObject*> objects = vec.back();

	for (SimObject* i : objects) {
		addObject(i);
	}

	for (SimObject* i : allObjects) {
		i->reestablishLinks();
	}

	vec.pop_back();
}

void SimEnvironment::popObjectVector(std::vector<std::vector<SimObject*>>& vec) {
	if (vec.empty())
		return;

	std::vector<SimObject*> objects = vec.back();
	if (objects.empty())
		return;

	for (SimObject* i : objects) {
		delete(i);
	}

	vec.pop_back();
}

void SimEnvironment::popFrontObjectVector(std::vector<std::vector<SimObject*>>& vec) {
	if (vec.empty())
		return;

	std::vector<SimObject*> objects = vec.front();
	if (objects.empty())
		return;

	for (SimObject* i : objects) {
		delete(i);
	}

	vec.erase(vec.begin());
}

glm::dvec3 SimEnvironment::extractAngles(glm::dmat4 mat) {
	double sy = sqrt(mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0]);

	bool singular = sy < 1e-6;
	double x, y, z;
	if (!singular) {
		x = atan2(mat[2][1], mat[2][2]);
		y = atan2(-mat[2][0], sy);
		z = atan2(mat[1][0], mat[0][0]);
	}
	else {
		x = atan2(-mat[1][2], mat[1][1]);
		y = atan2(-mat[2][0], sy);
		z = 0;
	}

	return glm::dvec3(x, y, z);
}

void SimEnvironment::addGuideX(float startPos, float min, float max, JMwindow* window, unsigned int handleGrabMin, unsigned int handleGrabMax) {
	if (window == nullptr) { return; }
	guiGuide newGuide;
	newGuide.pos = startPos;
	newGuide.min = min;
	newGuide.max = max;
	newGuide.handleMin = handleGrabMin;
	newGuide.handleMax = handleGrabMax;
	newGuide.window = window;

	if (newGuide.min > newGuide.max) { std::swap(newGuide.min, newGuide.max); }
	if (newGuide.pos < min) { newGuide.pos = min; }
	if (newGuide.pos > max) { newGuide.pos = max; }
	newGuide.pixelPos = (newGuide.pos / 100.0f) * window->width();
	xGuides.push_back(newGuide);
}
void SimEnvironment::addGuideX(float startPos, float min, float max, JMwindow* window) {
	addGuideX(startPos, min, max, window, 0, 0);
}
void SimEnvironment::addGuideY(float startPos, float min, float max, JMwindow* window, unsigned int handleGrabMin, unsigned int handleGrabMax) {
	if (window == nullptr) { return; }
	guiGuide newGuide;
	newGuide.pos = startPos;
	newGuide.min = min;
	newGuide.max = max;
	newGuide.handleMin = handleGrabMin;
	newGuide.handleMax = handleGrabMax;
	newGuide.window = window;

	if (newGuide.min > newGuide.max) { std::swap(newGuide.min, newGuide.max); }
	if (newGuide.pos < min) { newGuide.pos = min; }
	if (newGuide.pos > max) { newGuide.pos = max; }
	newGuide.pixelPos = (newGuide.pos / 100.0f) * window->height();
	yGuides.push_back(newGuide);
}
void SimEnvironment::addGuideY(float startPos, float min, float max, JMwindow* window) {
	addGuideY(startPos, min, max, window, 0, 0);
}

void SimEnvironment::updateGuides() {
	for (int i = 0; i < xGuides.size(); i++) {
		xGuides[i].pixelPos = (xGuides[i].pos / 100.0f) * xGuides[i].window->width();
	}
	for (int i = 0; i < yGuides.size(); i++) {
		yGuides[i].pixelPos = (yGuides[i].pos / 100.0f) * yGuides[i].window->height();
	}

	for (int i = 0; i < xGuideCenters.size(); i++) {
		int x1 = xGuideCenters[i]->guide1;
		int x2 = xGuideCenters[i]->guide2;
		xGuideCenters[i]->pixelPos = (xGuides[x1].pixelPos + xGuides[x2].pixelPos) / 2.0f;
		xGuideCenters[i]->pixelSize = abs(xGuides[x2].pixelPos - xGuides[x1].pixelPos) - guideMargin;
	}
	for (int i = 0; i < yGuideCenters.size(); i++) {
		int y1 = yGuideCenters[i]->guide1;
		int y2 = yGuideCenters[i]->guide2;
		yGuideCenters[i]->pixelPos = (yGuides[y1].pixelPos + yGuides[y2].pixelPos) / 2.0f;
		yGuideCenters[i]->pixelSize = abs(yGuides[y2].pixelPos - yGuides[y1].pixelPos) - guideMargin;
	}
}

bool SimEnvironment::createGuideCenters(unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {
	if (xGuides.size() < 2 || yGuides.size() < 2) {
		std::cerr << "SimEnvironment::createGuideCenters() Error: Must add at least two guides on the x and y axis." << std::endl;
		return false;
	}
	if (xGuides[x1].window != xGuides[x2].window || xGuides[x1].window != yGuides[y1].window || xGuides[x1].window != yGuides[y2].window) {
		std::cerr << "SimEnvironment::createGuideCenters() Error: All guides must be on the same window." << std::endl;
		return false;
	}
	if (x1 >= xGuides.size() || x2 >= xGuides.size() || y1 >= yGuides.size() || y2 >= yGuides.size()) {
		std::cerr << "SimEnvironment::createGuideCenters() Error: Guide index parameters out of range." << std::endl;
		return false;
	}

	SimEnvironment::guiGuideCenter* xCenter = new SimEnvironment::guiGuideCenter();
	xCenter->guide1 = x1;
	xCenter->guide2 = x2;

	SimEnvironment::guiGuideCenter* yCenter = new SimEnvironment::guiGuideCenter();
	yCenter->guide1 = y1;
	yCenter->guide2 = y2;

	xGuideCenters.push_back(xCenter);
	yGuideCenters.push_back(yCenter);

	updateGuides();
	return true;
}


void SimEnvironment::allignToGuides(GuiElementHandler* menu, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {

	if (!createGuideCenters(x1, x2, y1, y2)) { return; }
	
	menu->xPosRef = &xGuideCenters.back()->pixelPos;
	menu->setXSizeRef(& xGuideCenters.back()->pixelSize);
	menu->yPosRef = &yGuideCenters.back()->pixelPos;
	menu->setYSizeRef(& yGuideCenters.back()->pixelSize);
}
void SimEnvironment::allignToGuides(TabbedMenu* menu, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {

	if (!createGuideCenters(x1, x2, y1, y2)) { return; }
	
	menu->xPosRef = &xGuideCenters.back()->pixelPos;
	menu->xSizeRef = &xGuideCenters.back()->pixelSize;
	menu->yPosRef = &yGuideCenters.back()->pixelPos;
	menu->ySizeRef = &yGuideCenters.back()->pixelSize;
}

void SimEnvironment::allignToGuides(ObjectViewer* viewer, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2) {

	if (!createGuideCenters(x1, x2, y1, y2)) { return; }

	viewer->xPosRef = &xGuideCenters.back()->pixelPos;
	viewer->xSizeRef = &xGuideCenters.back()->pixelSize;
	viewer->yPosRef = &yGuideCenters.back()->pixelPos;
	viewer->ySizeRef = &yGuideCenters.back()->pixelSize;
}

bool SimEnvironment::guiGuide::mouseOverX(SimEnvironment* env)  const {
	float mx = window->window->mouseX();
	float my = window->window->mouseY();

	if (abs(mx - pixelPos) < handleSize && min != max) {

		if (handleMin != handleMax && handleMin < env->yGuides.size() && handleMax < env->yGuides.size()) {
			if (my < env->yGuides[handleMin].pixelPos || my > env->yGuides[handleMax].pixelPos) { return false; }
		}

		glfwSetCursor(window->window->window, window->cursor_windowResizeH);
		return true;
	}
	return false;
}
bool SimEnvironment::guiGuide::mouseOverY(SimEnvironment* env)  const{
	float mx = window->window->mouseX();
	float my = window->window->mouseY();

	if (abs(my - pixelPos) < handleSize && min != max) {

		if (handleMin != handleMax && handleMin < env->xGuides.size() && handleMax < env->xGuides.size()) {
			if (mx < env->xGuides[handleMin].pixelPos || mx > env->xGuides[handleMax].pixelPos) { return false; }
		}

		glfwSetCursor(window->window->window, window->cursor_windowResizeV);
		return true;
	}
	return false;
}

void SimEnvironment::detectMouseOverGuides() {
	bool needsUpdate = false;
	for (int i = 0; i < xGuides.size(); i++) {
		if (xGuides[i].mouseOverX(this)) { mouseOverGuideX = i; }
	}
	for (int i = 0; i < yGuides.size(); i++) {
		if (yGuides[i].mouseOverY(this)) { mouseOverGuideY = i; }
	}
	if (needsUpdate) { updateGuides(); }
}

void SimEnvironment::handleGuideMovement() {
	//This hurts my eyes. It technically works, but must refactor later.

	detectMouseOverGuides();
	if (mouseOverGuideX >= 0) {
		JMwindow* win = xGuides[mouseOverGuideX].window;
		
		if (win->window->mousePressed()) {
			xGuides[mouseOverGuideX].pos = 100.0f * (win->window->mouseX() / win->window->width());
			xGuides[mouseOverGuideX].pos = std::max(xGuides[mouseOverGuideX].min, std::min(xGuides[mouseOverGuideX].max, xGuides[mouseOverGuideX].pos));
			updateGuides();
			return;
		}

		if (abs(win->window->mouseX() - xGuides[mouseOverGuideX].pixelPos) > xGuides[mouseOverGuideX].handleSize) {
			glfwSetCursor(win->window->window, win->cursor_default);
			mouseOverGuideX = -1;
		}
		return;
	}
	if (mouseOverGuideY >= 0) {
		JMwindow* win = yGuides[mouseOverGuideY].window;

		if (win->window->mousePressed()) {
			yGuides[mouseOverGuideY].pos = 100.0f * (win->window->mouseY() / (win->window->height() - win->titleBarWidth));
			yGuides[mouseOverGuideY].pos = std::max(yGuides[mouseOverGuideY].min, std::min(yGuides[mouseOverGuideY].max, yGuides[mouseOverGuideY].pos));
			updateGuides();
			return;
		}

		if (abs(win->window->mouseY() - yGuides[mouseOverGuideY].pixelPos) > yGuides[mouseOverGuideY].handleSize) {
			glfwSetCursor(win->window->window, win->cursor_default);
			mouseOverGuideY = -1;
		}
		return;
	}
}

bool SimEnvironment::saveEnvironment(std::ostream& stream)  const {
	std::vector<SimObject*> objs = copyObjects();
	//Save project data
	stream << EnvironmentName << "\n";
	stream << fluxDensityScale << ' ' << distanceScale << ' ' << undoSteps << ' ' << redoSteps << ' ' << simThreadNum << "\n";

	//Save object data
	for (SimObject* i : objs) {
		i->save(stream);
		delete(i);
	}
	stream << "END\n";
	return true;
}

bool SimEnvironment::loadEnvironment(std::istream& stream) {
	//Load project data
	std::getline(stream, EnvironmentName);
	stream >> fluxDensityScale >> distanceScale >> undoSteps >> redoSteps >> simThreadNum;
	stream.ignore();

	//Load object data
	deleteAll();
	while (!undoVector.empty()) { popObjectVector(undoVector); }
	while (!redoVector.empty()) { popObjectVector(redoVector); }

	SimObject* loadedObj = nullptr;

	std::string type;
	while (stream >> type) {
		stream.ignore();

		if (type == "END") { break; }
		SimObject* newObj = nullptr;

		auto it = loadFactory.find(type);
		if (it != loadFactory.end()) {
			newObj = it->second();
			newObj->load(stream);
			addObject(newObj);
		}
	}

	for (SimObject* i : allObjects) { i->reestablishLinks(); }

	if (propertiesMenu != nullptr) {
		if (noObjSelectedMenu != nullptr) { delete(noObjSelectedMenu); }
		if (setupNoObjSelectedMenu != nullptr) {
			noObjSelectedMenu = setupNoObjSelectedMenu(propertiesMenu, this);
		}
		else {
			noObjSelectedMenu = setupNoObjMenuDefault(propertiesMenu);
		}
		setObjectMenus();
	}

	if (viewer != nullptr) {
		viewer->rEnvironment->forceRenderOneFrame();
	}

	return true;
}

bool SimEnvironment::saveEnvironment(JMwindow* window) {
	manager->save(window);
	return true;
}

bool SimEnvironment::loadEnvironment(JMwindow* window) {
	manager->load(window);
	return true;
}

bool SimEnvironment::saveEnvironmentAs(JMwindow* window) {
	manager->saveAs(window);
	return true;
}

void SimEnvironment::recoverBackup() {
	manager->recoverBackup();
}
void SimEnvironment::saveBackup() {
	manager->saveBackup();
}

SimResourceManager* SimEnvironment::getManager() {
	return manager;
}

void SimEnvironment::setInfoBarMessage(std::string message) {
	manager->infoBarMessage = message;
}

void SimEnvironment::registerDerivedSimObject(const std::string& objectType, std::function<SimObject* ()> constructor) {
	loadFactory[objectType] = std::move(constructor);
}

void SimEnvironment::checkWindowResized(JMGraphics* gr) {
	if (windowWidthLast == gr->width() && windowHeightLast == gr->height()) { return; }

	updateGuides();

	windowWidthLast = gr->width();
	windowHeightLast = gr->height();
}