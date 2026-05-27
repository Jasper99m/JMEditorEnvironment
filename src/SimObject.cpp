#include "SimEnvironment.h"
#include "glm/gtx/euler_angles.hpp"

SimObject::SimObject() {
}
SimObject::~SimObject() {
	if (generalMenu != nullptr) { delete(generalMenu); }
	if (menu1 != nullptr) { delete(menu1); }
	if (menu2 != nullptr) { delete(menu2); }

	unlinkAll();
}

void SimObject::display(JMGraphics* gr) {
	gr->stroke(0.0f, 0.5f, 1.0f);
	gr->strokeWeight(1.5f);
	gr->fill(0.0f, 0.1f, 0.3f, 1.0f);
	gr->box((float)sizeX, (float)sizeY, (float)sizeZ);
}

double SimObject::sizex() const {
	return sizeX;
}
double SimObject::sizey()  const {
	return sizeY;
}
double SimObject::sizez()  const {
	return sizeZ;
}
glm::dvec3 SimObject::size()  const {
	return glm::dvec3(sizeX, sizeY, sizeZ);
}


double SimObject::posx()  const {
	return posX;
}
double SimObject::posy()  const {
	return posY;
}
double SimObject::posz()  const {
	return posZ;
}
glm::dvec3 SimObject::pos()  const {
	return glm::dvec3(posX, posY, posZ);
}

double SimObject::rotx()  const {
	return rotX;
}
double SimObject::roty()  const {
	return rotY;
}
double SimObject::rotz()  const {
	return rotZ;
}
glm::dvec3 SimObject::rot()  const {
	return glm::dvec3(rotX, rotY, rotZ);
}
glm::dquat SimObject::rotq()  const {
	return rotation;
}

glm::dmat4 SimObject::positionMatrix()  const {
	return positionMat;
}

bool SimObject::setSize(double x, double y, double z) {
	if (lockSize || (handler != nullptr && handler->objectsFrozen())) { return false; }


	sizeX = x;
	sizeY = y;
	sizeZ = z;

	if (sizeX < 0.0f)
		sizeX = 0.0f;
	if (sizeY < 0.0f)
		sizeY = 0.0f;
	if (sizeZ < 0.0f)
		sizeZ = 0.0f;

	if (tbSizeX != nullptr) { tbSizeX->setInputWithoutEvent((float)sizeX); }
	if (tbSizeY != nullptr) { tbSizeY->setInputWithoutEvent((float)sizeY); }
	if (tbSizeZ != nullptr) { tbSizeZ->setInputWithoutEvent((float)sizeZ); }

	return true;
}
bool SimObject::setSize(glm::dvec3 size) {
	return setSize(size.x, size.y, size.z);
}
bool SimObject::setPos(double x, double y, double z) {
	if (lockPos || (handler != nullptr && handler->objectsFrozen())) { return false; }

	posX = x;
	posY = y;
	posZ = z;

	if (tbPosX != nullptr) { tbPosX->setInputWithoutEvent((float)posX); }
	if (tbPosY != nullptr) { tbPosY->setInputWithoutEvent((float)posY); }
	if (tbPosZ != nullptr) { tbPosZ->setInputWithoutEvent((float)posZ); }

	positionMat = glm::translate(glm::dmat4(1.0), glm::dvec3(posX, posY, posZ)) * glm::mat4_cast(rotation);

	return true;
}
bool SimObject::setPos(glm::dvec3 pos) {
	return setPos(pos.x, pos.y, pos.z);
}
bool SimObject::setRot(double x, double y, double z) {
	if (lockRot || (handler != nullptr && handler->objectsFrozen())) { return false; }

	rotX = x;
	rotY = y;
	rotZ = z;

	if (tbRotX != nullptr) { tbRotX->setInputWithoutEvent(glm::degrees((float)rotX)); }
	if (tbRotY != nullptr) { tbRotY->setInputWithoutEvent(glm::degrees((float)rotY)); }
	if (tbRotZ != nullptr) { tbRotZ->setInputWithoutEvent(glm::degrees((float)rotZ)); }

	rotation =  glm::angleAxis(x, glm::dvec3(1.0, 0.0, 0.0)) *
				glm::angleAxis(y, glm::dvec3(0.0, 1.0, 0.0)) *
				glm::angleAxis(z, glm::dvec3(0.0, 0.0, 1.0));

	positionMat = glm::translate(glm::dmat4(1.0), glm::dvec3(posX, posY, posZ)) * glm::mat4_cast(rotation);

	return true;

}
bool SimObject::setRot(glm::dvec3 rot) {
	return setRot(rot.x, rot.y, rot.z);
}


bool SimObject::setRot(glm::dquat rot) {
	if (lockRot || (handler != nullptr && handler->objectsFrozen())) { return false; }

	rotation = rot;

	positionMat = glm::translate(glm::dmat4(1.0), glm::dvec3(posX, posY, posZ)) * glm::mat4_cast(rotation);

	//glm::dvec3 newEuler = glm::eulerAngles(rot);
	glm::dmat4 R = glm::mat4_cast(glm::normalize(rotation));
	glm::extractEulerAngleXYZ(R, rotX, rotY, rotZ);

	if (tbRotX != nullptr) { tbRotX->setInputWithoutEvent(glm::degrees((float)rotX)); }
	if (tbRotY != nullptr) { tbRotY->setInputWithoutEvent(glm::degrees((float)rotY)); }
	if (tbRotZ != nullptr) { tbRotZ->setInputWithoutEvent(glm::degrees((float)rotZ)); }

	return true;
}

SimObject* SimObject::clone() {
	SimObject* c = cloneInternal();
	c->posX = posX;
	c->posY = posY;
	c->posZ = posZ;
	c->rotX = rotX;
	c->rotY = rotY;
	c->rotZ = rotZ;
	c->sizeX = sizeX;
	c->sizeY = sizeY;
	c->sizeZ = sizeZ;
	c->lockPos = lockPos;
	c->lockRot = lockRot;
	c->lockSize = lockSize;
	c->name = name;
	c->ObjectCategory = ObjectCategory;
	c->hide = hide;
	c->viewportSelectable = viewportSelectable;
	c->positionMat = positionMat;
	c->rotation = rotation;

	c->generalMenu = nullptr;
	c->menu1 = nullptr;
	c->menu2 = nullptr;
	c->handler = nullptr;
	c->isSecondSelected = false;
	c->linkedObjects.clear();

	return c;
}

std::string SimObject::objectCategory()  const {
	return ObjectCategory;
}

void SimObject::setupMenus(TabbedMenu* tabbedMenu) {
	if (generalMenu != nullptr)
		delete(generalMenu);
	if (menu1 != nullptr)
		delete(menu1);
	if (menu2 != nullptr)
		delete(menu2);
	setupGeneralMenu(tabbedMenu);
	setupMenu1(tabbedMenu);
	setupMenu2(tabbedMenu);
}

void SimObject::setupGeneralMenu(TabbedMenu* tabbedMenu) {
	tabbedMenu->window->animatedStartup = false;
	generalMenu = new GuiElementHandler(tabbedMenu->window);
	//Filler graphics
	generalMenu->addElement(new DividingLine(0.0f, 0.0f, tabbedMenu->width() * 0.8f));
	generalMenu->addElement(new DividingLine(0.0f, -tabbedMenu->height() * 0.166f, tabbedMenu->width() * 0.8f));
	generalMenu->addElement(new DividingLine(0.0f, -tabbedMenu->height() * 0.333f, tabbedMenu->width() * 0.8f));
	//Name input
	TextBox* nameBox = new TextBox(-40.0f, tabbedMenu->height() / 2.0f - 40.0f, tabbedMenu->width() - 120.0f);
	generalMenu->addElement(nameBox);
	nameBox->allowEmpty = false;
	nameBox->setInput(name);
	auto setName = [this, nameBox](JMeventDispatcher::Event* E) {
		std::string newName = static_cast<TextBox::TextInputEvent*>(E)->input;
		if (name == newName)
			return;

		if (handler != nullptr)
			handler->setObjectName(this, newName);
		else
			name = newName;

		nameBox->setInput(name);
		handler->selectObject(this);
		};
	nameBox->event_textInput->addListener(setName);

	//Delete object button
	DropDown* deleteButton = new DropDown(tabbedMenu->width() / 2.0f - 60.0f, tabbedMenu->height() / 2.0f - 40.0f, 70.0f, "DELETE");
	deleteButton->addItem("Confirm");
	auto deleteObjFunc = [this](JMeventDispatcher::Event* E) {
		if (this->handler != nullptr) {
			this->handler->addUndoStep();
			this->handler->deleteObject(this);
		}
		else
			delete(this);
		};

	deleteButton->addItemEventListener("Confirm", deleteObjFunc);
	generalMenu->addElement(deleteButton);

	//Position input
	tbPosX = new TextBox(-tabbedMenu->width() * 0.3f, tabbedMenu->height() / 2.0f - 105.0f, tabbedMenu->width() / 3.0f - 25.0f, "X");
	tbPosX->allowEmpty = false;
	tbPosX->allowLetters = false;
	tbPosX->allowSymbols = false;
	tbPosX->setInput((float)posX);
	tbPosY = new TextBox(0.0f, tabbedMenu->height() / 2.0f - 105.0f, tabbedMenu->width() / 3.0f - 25.0f, "Y");
	tbPosY->allowEmpty = false;
	tbPosY->allowLetters = false;
	tbPosY->allowSymbols = false;
	tbPosY->setInput((float)posY);
	tbPosZ = new TextBox(tabbedMenu->width() * 0.3f, tabbedMenu->height() / 2.0f - 105.0f, tabbedMenu->width() / 3.0f - 25.0f, "Z");
	tbPosZ->allowEmpty = false;
	tbPosZ->allowLetters = false;
	tbPosZ->allowSymbols = false;
	tbPosZ->setInput((float)posZ);
	generalMenu->addElement(tbPosX);
	generalMenu->addElement(tbPosY);
	generalMenu->addElement(tbPosZ);
	generalMenu->addElement(new Frame(0.0f, tabbedMenu->height() / 2.0f - 100.0f, tabbedMenu->width() - 20.0f, 60.0f, "Position"));


	auto setXpos = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == posX)
			return;

		handler->addUndoStep();

		if (!this->setPos(newInput, posY, posZ))
			tbPosX->setInputWithoutEvent((float)this->posX);
		};
	tbPosX->event_floatInput->addListener(setXpos);

	auto setYpos = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == posY)
			return;

		handler->addUndoStep();

		if (!this->setPos(posX, newInput, posZ))
			tbPosY->setInputWithoutEvent((float)this->posY);
		};
	tbPosY->event_floatInput->addListener(setYpos);

	auto setZpos = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == posZ)
			return;

		handler->addUndoStep();

		if (!this->setPos(posX, posY, newInput))
			tbPosZ->setInputWithoutEvent((float)this->posZ);
		};
	tbPosZ->event_floatInput->addListener(setZpos);

	//Rotation input
	tbRotX = new TextBox(-tabbedMenu->width() * 0.3f, tabbedMenu->height() / 2.0f - 185.0f, tabbedMenu->width() / 3.0f - 25.0f, "X");
	tbRotX->allowEmpty = false;
	tbRotX->allowLetters = false;
	tbRotX->allowSymbols = false;
	tbRotX->setInput(glm::degrees((float)rotX));
	tbRotY = new TextBox(0.0f, tabbedMenu->height() / 2.0f - 185.0f, tabbedMenu->width() / 3.0f - 25.0f, "Y");
	tbRotY->allowEmpty = false;
	tbRotY->allowLetters = false;
	tbRotY->allowSymbols = false;
	tbRotY->setInput(glm::degrees((float)rotY));
	tbRotZ = new TextBox(tabbedMenu->width() * 0.3f, tabbedMenu->height() / 2.0f - 185.0f, tabbedMenu->width() / 3.0f - 25.0f, "Z");
	tbRotZ->allowEmpty = false;
	tbRotZ->allowLetters = false;
	tbRotZ->allowSymbols = false;
	tbRotZ->setInput(glm::degrees((float)rotZ));
	generalMenu->addElement(tbRotX);
	generalMenu->addElement(tbRotY);
	generalMenu->addElement(tbRotZ);
	generalMenu->addElement(new Frame(0.0f, tabbedMenu->height() / 2.0f - 180.0f, tabbedMenu->width() - 20.0f, 60.0f, "Rotation"));

	auto setXRot = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == glm::degrees(rotX))
			return;

		handler->addUndoStep();

		if (!this->setRot(glm::radians(newInput), rotY, rotZ))
			tbRotX->setInputWithoutEvent(glm::degrees((float)rotX));
		};
	tbRotX->event_floatInput->addListener(setXRot);

	auto setYrot = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == glm::degrees((float)rotY))
			return;

		handler->addUndoStep();

		if (!this->setRot(rotX, glm::radians(newInput), rotZ))
			tbRotY->setInputWithoutEvent(glm::degrees((float)rotY));
		};
	tbRotY->event_floatInput->addListener(setYrot);

	auto setZrot = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == glm::degrees(rotZ))
			return;

		handler->addUndoStep();

		if (!this->setRot(rotX, rotY, glm::radians(newInput)))
			tbRotZ->setInputWithoutEvent(glm::degrees((float)rotZ));
		};
	tbRotZ->event_floatInput->addListener(setZrot);

	//Scale input
	tbSizeX = new TextBox(-tabbedMenu->width() * 0.3f, tabbedMenu->height() / 2.0f - 265.0f, tabbedMenu->width() / 3.0f - 25.0f, "X");
	tbSizeX->allowEmpty = false;
	tbSizeX->allowLetters = false;
	tbSizeX->allowSymbols = false;
	tbSizeX->setInput((float)sizeX);
	tbSizeY = new TextBox(0.0f, tabbedMenu->height() / 2.0f - 265.0f, tabbedMenu->width() / 3.0f - 25.0f, "Y");
	tbSizeY->allowEmpty = false;
	tbSizeY->allowLetters = false;
	tbSizeY->allowSymbols = false;
	tbSizeY->setInput((float)sizeY);
	tbSizeZ = new TextBox(tabbedMenu->width() * 0.3f, tabbedMenu->height() / 2.0f - 265.0f, tabbedMenu->width() / 3.0f - 25.0f, "Z");
	tbSizeZ->allowEmpty = false;
	tbSizeZ->allowLetters = false;
	tbSizeZ->allowSymbols = false;
	tbSizeZ->setInput((float)sizeZ);
	generalMenu->addElement(tbSizeX);
	generalMenu->addElement(tbSizeY);
	generalMenu->addElement(tbSizeZ);
	generalMenu->addElement(new Frame(0.0f, tabbedMenu->height() / 2.0f - 260.0f, tabbedMenu->width() - 20.0f, 60.0f, "Scale"));


	auto setXSize = [this](JMeventDispatcher::Event* E) {

		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == sizeX)
			return;

		handler->addUndoStep();

		if (!this->setSize(newInput, sizeY, sizeZ))
			tbSizeX->setInputWithoutEvent((float)sizeX);
		};
	tbSizeX->event_floatInput->addListener(setXSize);

	auto setYSize = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == sizeY)
			return;

		handler->addUndoStep();

		if (!this->setSize(sizeX, newInput, sizeZ))
			tbSizeY->setInputWithoutEvent((float)rotY);
		};
	tbSizeY->event_floatInput->addListener(setYSize);

	auto setZSize = [this](JMeventDispatcher::Event* E) {
		float newInput = static_cast<TextBox::FloatInputEvent*>(E)->input;
		if (newInput == sizeZ)
			return;

		handler->addUndoStep();

		if (!this->setSize(sizeX, sizeY, newInput))
			tbSizeZ->setInputWithoutEvent((float)sizeZ);
		};
	tbSizeZ->event_floatInput->addListener(setZSize);

	//Moves the view to the object when clicked.
	JMGraphics* gr = tabbedMenu->window->window;
	JMGraphics::Buffer* focusGraphics = new JMGraphics::Buffer(30.0f, 30.0f, gr);
	focusGraphics->beginDraw();
	gr->background(0.0f, 0.0f, 0.0f, 0.0f);
	gr->translate(focusGraphics->width() / 2.0f, focusGraphics->height() / 2.0f);
	gr->noFill();
	gr->stroke(tabbedMenu->window->color_lines1);
	gr->strokeWeight(2.0f);
	gr->ellipse(0.0f, 0.0f, 8.5f);
	gr->line(-13.0f, 0.0f, 0.0f, -7.0f);
	gr->line(-13.0f, 0.0f, 0.0f, 7.0f);
	gr->line(13.0f, 0.0f, 0.0f, -7.0f);
	gr->line(13.0f, 0.0f, 0.0f, 7.0f);
	focusGraphics->endDraw();
	CustomButton* focusButton = new CustomButton(-tabbedMenu->width() / 2.0f + 30.0f, tabbedMenu->height() / 2.0f - 315.0f, focusGraphics);
	generalMenu->addElement(focusButton);

	auto focusOnObject = [this](JMeventDispatcher::Event* E) {this->handler->focusViewerOnObject(this); };
	focusButton->event_clicked->addListener(focusOnObject);

	focusButton->helpMessage = "Move the camera to this object";

}
void SimObject::setupMenu1(TabbedMenu* tabbedMenu) {
	return;
	menu1 = new GuiElementHandler(tabbedMenu->window);
	menu1->addElement(new Message("Nothing here", 0.0f, 0.0f));
}
void SimObject::setupMenu2(TabbedMenu* tabbedMenu) {
	return;
	menu2 = new GuiElementHandler(tabbedMenu->window);
	menu2->addElement(new Message("Nothing here either", 0.0f, 0.0f));
}

void SimObject::setupDataWindowInternal(JMwindow* window, float dataX, float dataY) {
	if ((dataX == dataWinX && dataY == dataWinY) || window->window->mouseMoved()) { return; }
	setupDataWindow(window, dataX, dataY);
	dataWinX = dataX;
	dataWinY = dataY;
}

void SimObject::setupDataWindow(JMwindow* window, float dataX, float dataY) {

}

void SimObject::displayDataWindow(JMwindow* window, float localMouseX, float localMouseY, float dataX, float dataY) {
	JMGraphics* gr = window->window;
	gr->fill(window->color_lines1);
	gr->setFont(window->font_standard);
	gr->textSize(1.0f);

	gr->text(name + "\nCategory: " + objectCategory(), -dataX / 2.0f + 8.0f, dataY / 2.0f - 35.0f);

	if (!window->coordInRect(localMouseX, localMouseY, 0.0f, -50.0f, dataX, dataY - 100.0f)) {
		return;
	}

	gr->noFill();
	gr->stroke(window->color_fill1);
	gr->strokeWeight(1.0f);
	gr->ellipse(localMouseX, localMouseY, 10.0f);
	gr->dashedLine(-dataX / 2.0f, localMouseY, dataX / 2.0f, localMouseY, 8.0f, 5.0f);
	gr->dashedLine(localMouseX, -dataY / 2.0f, localMouseX, dataY / 2.0f - 50.0f, 8.0f, 5.0f);
}

void SimObject::save(std::ostream& stream) {
	stream << ObjectType << "\n" << ObjectCategory << "\n" << name << "\n";
	stream << posX << ' ' << posY << ' ' << posZ << ' ' << rotation.w << ' ' << rotation.x << ' ' << rotation.y << ' ' << rotation.z << ' ' << sizeX << ' ' << sizeY << ' ' << sizeZ << ' ';
	stream << lockPos << ' ' << lockRot << ' ' << lockSize << ' ' << linkedNames.size();
	stream << "\n";
	for (int i = 0; i < linkedNames.size(); i++) {
		stream << linkedNames[i] << "\n";
	}

	if (!writeData(stream)) {
		std::cerr << "SimObject::Save() Error: Failed to write derived class data to stream.\n"
			<< "Object type: " << ObjectType << " Object name: " << name << std::endl;
		return;
	}
	stream << "\n";
	return;
}
void SimObject::load(std::istream& stream) {
	std::getline(stream, ObjectCategory);
	std::getline(stream, name);

	lockPos = false;
	lockRot = false;
	lockSize = false;
	glm::dquat tmpRot;
	stream >> posX >> posY >> posZ >> tmpRot.w >> tmpRot.x >> tmpRot.y >> tmpRot.z >> sizeX >> sizeY >> sizeZ;
	setPos(posX, posY, posZ);
	setRot(tmpRot);
	setSize(sizeX, sizeY, sizeZ);
	stream >> lockPos >> lockRot >> lockSize;
	int linkedNum = 0;
	stream >> linkedNum;
	stream.ignore();
	for (int i = 0; i < linkedNum; i++) {
		std::string linkedName;
		std::getline(stream, linkedName);
		linkedNames.push_back(linkedName);
	}

	if (!readData(stream)) {
		std::cerr << "SimObject::Load() Error: Failed to read derived class data from stream.\n"
			<< "Object type: " << ObjectType << " Object name: " << name << std::endl;
		return;
	}
}

void SimObject::linkObject(SimObject* obj) {
	if (obj == nullptr) { return; }

	auto addLink = [](SimObject* A, SimObject* B) {
		auto i = std::find(A->linkedObjects.begin(), A->linkedObjects.end(), B);
		auto j = std::find(A->linkedNames.begin(), A->linkedNames.end(), B->name);

		if (i == A->linkedObjects.end()) { A->linkedObjects.push_back(B); }
		if (j == A->linkedNames.end()) { A->linkedNames.push_back(B->name); }
		};

	addLink(this, obj);
	addLink(obj, this);
}
void SimObject::unlinkObject(SimObject* obj) {
	if (obj == nullptr) { return; }

	auto removeLink = [](SimObject* A, SimObject* B) {
		auto i = std::find(A->linkedObjects.begin(), A->linkedObjects.end(), B);
		if (i != A->linkedObjects.end()) { A->linkedObjects.erase(i); }

		auto j = std::find(A->linkedNames.begin(), A->linkedNames.end(), B->name);
		if (j != A->linkedNames.end()) { A->linkedNames.erase(j); }
		};

	removeLink(this, obj);
	removeLink(obj, this);
}

std::vector<SimObject*> SimObject::allLinkedOjects() {
	return linkedObjects;
}

void SimObject::reestablishLinks() {
	for (int i = 0; i < linkedNames.size(); i++) {
		SimObject* foundObj = handler->getManager()->findObject(linkedNames[i]);
		if (foundObj != nullptr) {
			linkObject(foundObj);
		}
		else {
			linkedNames.erase(linkedNames.begin() + i);
		}
	}
}

void SimObject::unlinkAll() {
	for (SimObject* i : linkedObjects) {
		unlinkObject(i);
	}
	linkedObjects.clear();
	linkedNames.clear();
}

bool SimObject::readData(std::istream& stream) {
	return true;
}
bool SimObject::writeData(std::ostream& stream) {
	return true;
}

SimObject* SimObject::cloneInternal(){
	return new SimObject();
}
