#pragma once

#include "ObjectViewer.h"

class SimEnvironment;

/**
* A base class for sim objects. These can be added to a SimEnvironment.
* If you want the object to be rendered as anything other than a cube, override display(JMGraphics* gr).
* If you want settings menus besides the general settings, override setupMenu1(TabbedMenu* tabbedMenu) or setupMenu2(TabbedMenu* tabbedMenu).
* In these functions, make sure to initialize the correct GuiElementHandler objects (menu1 or menu2). A pointer to the TabbedMenu
* object that they will appear in is passed as a parameter by the SimEnvironment
* so that your gui elements can be scaled/position appropriately.
* 
* The cloneInternal function must be overridden if you want undo-redo, duplicating and save-load to work correctly.
* It needs to return a pointer to a copy of itself, only set the copies data that is specific to the subclass, base class data is handled by a wrapper function.
* 
* For saving and loading from a file to work, the objectType string must be set in the constructor to something unique to the derived class,
* and it must be added using the simEnvironments registerDerivedSimObject function.
* readData() and writeData() functions must be overriden to read and write data unique to the derived class in order to save and load from a file.
* Saving and loading base data is handled by the save and load wrapper functions.
*/
class SimObject {
public:
	SimObject();
	virtual ~SimObject();

	SimObject(SimObject&) = delete;
	SimObject& operator=(const SimObject&) = delete;
	SimObject(SimObject&&) = default;
	SimObject& operator=(SimObject&&) = default;

	///Render the object in the parameter graphics object.
	///Does not apply tranforms for this object, transform the graphics objects matrix
	///using this objects positionMatrix function first, or it will be rendered at 0, 0, 0 with no rotation.
	virtual void display(JMGraphics* gr);

	///Returns object/bounding box size on the X axis
	double sizex() const;
	///Returns object/bounding box size on the Y axis
	double sizey() const;
	///Returns object/bounding box size on the Z axis
	double sizez() const;
	///Returns object/bounding box size as a vector
	glm::dvec3 size() const;

	///Returns the objects position on the X axis
	double posx() const;
	///Returns the objects position on the Y axis
	double posy() const;
	///Returns the objects position on the Z axis
	double posz() const;
	///Returns the objects position as a vector. Position is at the center of the bounding box.
	glm::dvec3 pos() const;

	///Returns the objects Euler angle on the X axis
	double rotx() const;
	///Returns the objects Euler angle on the Y axis
	double roty() const;
	///Returns the objects Euler angle on the Z axis
	double rotz() const;
	///Returns the objects Euler angle as a vector. Euler angles are always applied in XYZ order.
	glm::dvec3 rot() const;
	///Returns the objects angle as a quaternion
	glm::dquat rotq() const;

	///Returns a matrix containing the objects position and rotation transformations
	glm::dmat4 positionMatrix() const;

	///Set the object/bounding box size. Returns true if allowed.
	bool setSize(double x, double y, double z);
	///Set the object/bounding box size. Returns true if allowed.
	bool setSize(glm::dvec3 size);
	///Set the objects position. Position is the center of the bounding box. Returns true if allowed.
	bool setPos(double x, double y, double z);
	///Set the objects position. Position is the center of the bounding box. Returns true if allowed.
	bool setPos(glm::dvec3 pos);
	///Set the objects rotation from Euler angles. Angles are applied in XYZ order. Returns true if allowed.
	bool setRot(double x, double y, double z);
	///Set the objects rotation from Euler angles. Angles are applied in XYZ order. Returns true if allowed.
	bool setRot(glm::dvec3 rot);
	///Set the objects angle from a quaternion. Returns true if allowed.
	bool setRot(glm::dquat rot);

	///Returns a copy of this object.
	SimObject* clone();

	///Override this to set up the first properties menu.
	virtual void setupMenu1(TabbedMenu* tabbedMenu);
	///Override this to set up the second properties menu.
	virtual void setupMenu2(TabbedMenu* tabbedMenu);
	///Sets up the general settings menu.
	void setupGeneralMenu(TabbedMenu* tabbedMenu);

	///Is called by the object handler. Calls setupDataWindow if needed.
	void setupDataWindowInternal(JMwindow* window, float dataX, float dataY);

	///Sets up the data window that the object can render to. Inputs are the windows size. coordinates 0, 0 will be the center of the window.
	///Note: this may be called multiple times if the user re-sizes the data window.
	virtual void setupDataWindow(JMwindow* window, float dataX, float dataY);
	///Is called by the object handler to display the data window
	virtual void displayDataWindow(JMwindow* window, float localMouseX, float localMouseY, float dataX, float dataY);

	///Returns the object category string.
	std::string objectCategory() const;

	///A stored copy of the render engines transformation matrix when the object was last rendered.
	///This includes all object transformations, and view/camera transformations.
	///All transormations are done in pixel coordinates, not NDC.
	glm::mat4 renderMatrix = glm::mat4(1.0f);

	///is called by the object handler if the properties menus are needed.
	void setupMenus(TabbedMenu* tabbedMenu);

	///Writes this objects data to a stream. Used to save to a file.
	void save(std::ostream& stream);
	///Reads object data from a stream. Used to load from a file.
	void load(std::istream& stream);

	///Establishes a two way link with a specific sim object.
	void linkObject(SimObject* obj);
	///Removes the two way link with a specific sim object.
	void unlinkObject(SimObject* obj);

	///Returns all sim objects that are linked with this one.
	std::vector<SimObject*> allLinkedOjects();

	///Will try to re link all previously linked sim objects.
	///If sim objects are cloned or loaded from a file, link pointers are cleared so this must be called.
	void reestablishLinks();

	///Removes all sim object links.
	void unlinkAll();

	///The name that's displayed on the screen and can be set by the user.
	std::string name = "Object - 001";

	///This name is used to label the tabs on the propertiesMenu.
	std::string menu1Name = "Properties";
	///This name is used to label the tabs on the propertiesMenu.
	std::string menu2Name = "Properties 2";

	///Is set to true by the SimEnvironment if this is the primary selection.  Avoid seting this yourself.
	bool isSelected = false;
	///Is set to true by the SimEnvironment if this is a secondary selection.  Avoid seting this yourself.
	bool isSecondSelected = false;

	///If set to true, the object can't be selected in the viewer and is not rendered.
	bool hide = false;

	///Set to false to disable selection in the viewport.
	bool viewportSelectable = true;

	///This is displayed in tbe SimEnvironments properties menu if it is set up and displayed. It is created and set up by the SimObject base class.
	GuiElementHandler* generalMenu = nullptr;
	//This is displayed in tbe SimEnvironments properties menu if it is set up and displayed. It must be created in setupMenu1() in a derived class.
	GuiElementHandler* menu1 = nullptr;
	//This is displayed in tbe SimEnvironments properties menu if it is set up and displayed. It must be created in setupMenu2() in a derived class.
	GuiElementHandler* menu2 = nullptr;

	///A pointer to the SimEnvironment instance containing this SimObject.
	SimEnvironment* handler = nullptr;

protected:

	///Reads data for loading from a file. Override this and read only data unique to the derived class.
	///Base class data is read by wrapper function.
	virtual bool readData(std::istream& stream);
	///Writes data for saving to a file. Override this and write only data unique to the derived class.
	///Base class data is written by wrapper function.
	virtual bool writeData(std::ostream& stream);

	///Override this to clone all derived class data. Must return a pointer to the new object.
	///Does not need to write base class data to the clone.
	virtual SimObject* cloneInternal();

	bool lockSize = false;
	bool lockPos = false;
	bool lockRot = false;

	///This can be used to filter for objects of a specific catagory.
	std::string ObjectCategory = "General";

	///The object type name used when saving and loading from a file. Must be unique to the derived class.
	std::string ObjectType = "BASE";

	float dataWinX = 0.0;
	float dataWinY = 0.0;

	///the graphics object used to render the object.
	JMGraphics* graphicsObject = nullptr;

private:
	
	double sizeX = 50.0f;
	double sizeY = 50.0f;
	double sizeZ = 50.0f;

	double posX = 0.0f;
	double posY = 0.0f;
	double posZ = 0.0f;

	double rotX = 0.0f;
	double rotY = 0.0f;
	double rotZ = 0.0f;

	glm::dmat4 positionMat = glm::mat4(1.0f);
	glm::dquat rotation = glm::dquat(glm::dvec3(0.0, 0.0, 0.0));

	bool mouseOverDataWin = false;

	//The general menu gui elements
	TextBox* tbPosX = nullptr;
	TextBox* tbPosY = nullptr;
	TextBox* tbPosZ = nullptr;
	TextBox* tbRotX = nullptr;
	TextBox* tbRotY = nullptr;
	TextBox* tbRotZ = nullptr;
	TextBox* tbSizeX = nullptr;
	TextBox* tbSizeY = nullptr;
	TextBox* tbSizeZ = nullptr;

	std::vector<SimObject*> linkedObjects;
	std::vector<std::string> linkedNames;
};

class SimResourceManager;

/**
* Contains and manages SimObject instances and the gui elements used to interact with them.\n
* Each gui section needs to be set up if it will be used. 
* They can either be set up at fixed pixel coordinates, or alligned to GuiGuides (recomended), allowing the user to re size them.
* If guides are used, they must be added first using addGuideX and addGuidY functions, then selected in the _toGuides versions of the setup functions by index.
* \n\n
* The gui sections are:\nViewer. A 3D environment that is rendered in a seperate thread
* where SimObjects can be viewed and interacted with.
* \n\n
* PropertiesMenu. A menu where the user can adjust the selected SimObjects properties. By default, a general settings menu for each SimObject exists
* but SimObject derived classes can more custom properties menus.
* \n\n
* Selector. A ListMenu that displays all of the SimObjects in the environment. SimObjects can be selected here as well as the viewer.
* \n\n
* ObjectDataWindow. SimObject has setupDataWindow and displayDataWindow virtual functions that can be overriden in derived classes to display custom data here.
*/
class SimEnvironment {
public:
	SimEnvironment();
	~SimEnvironment();

	SimEnvironment(SimEnvironment&) = delete;
	SimEnvironment& operator=(const SimEnvironment&) = delete;
	SimEnvironment(SimEnvironment&&) = default;
	SimEnvironment& operator=(SimEnvironment&&) = default;

	///Sets up a 3d environment that renders the objects and allows the user to manipulate them.
	void setupViewer(JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY);
	///Sets up a 3d environment that renders the objects and allows the user to manipulate them. Allignes to pre-existing gui guides.
	void setupViewer_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
	void displayViewer();

	///Sets up a tabbed menu that displays object properties. Set the "setupNoObjSelectedMenu" function for a custom menu.
	void setupPropertiesMenu(JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY);
	///Sets up a tabbed menu that displays object properties. Allignes to pre-existing gui guides. Set the "setupNoObjSelectedMenu" function for a custom menu.
	void setupPropertiesMenu_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
	void displayPropertiesMenu();

	///Sets up a list menu that displays all existing objects and allows the user to select them.
	void setupSelector(JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY);
	///Sets up a list menu that displays all existing objects and allows the user to select them. Allignes to pre-existing gui guides.
	void setupSelector_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
	void displaySelector();

	///Sets up a window to display object data.
	void setupObjectDataWindow(JMwindow* Window, float posX, float posY, float sizeX, float sizeY);
	///Sets up a window to display object data. Allignes to pre-existing gui guides.
	void setupObjectDataWindow_toGuides(JMwindow* Window, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
	void displayObjectDataWindow();

	///Displays an info bar at the bottom of the screen containing basic info about the environment. Width is determined by the position of the first Y guide.
	void displayInfoBar(JMwindow* window);

	///Add a new SimObject to the SimEnvironment
	void addObject(SimObject* obj);
	///Deletes the object if its contained in the SimEnvironment
	void deleteObject(SimObject* obj);

	///Moves the object viewers camera to the input object if the viewer is setup
	void focusViewerOnObject(SimObject* obj);

	///Selects an object. If an object is already selected, the object will be second-selected.
	void selectObject(SimObject* obj);
	///Selects an object by name. If an object is already selected, the object will be second-selected.
	void selectObject(std::string name);
	///Deselects an object.
	void deselectObject(SimObject* obj);
	///Deselects an object.
	void deselectObject(std::string name);
	///Deselects all objects.
	void deselectAll();
	///Deletes all the objects.
	void deleteAll();
	///Disables selection and modification of objects. If objects are selected, they remain selected.
	void freezeObjects();
	///Re-enables selection and modification of objects.
	void unfreezeObjects();
	///Returns true if objects are frozen.
	bool objectsFrozen() const;
	///Sets the name for a specific object. Returns the new name, as it might be modified if its a duplicate.
	std::string setObjectName(SimObject* obj, std::string newName);

	///Returns a pointer to the primary selection.
	SimObject* selectedObject() const;
	///Returns an std::vector of pointers to all of the objects that are second-selected.
	std::vector<SimObject*> selectedObjects() const;

	///Returns an std::vector of pointers to all of the objects that are second-selected, and who's ObjectCategory string matches the input
	std::vector<SimObject*> selectedObjects(std::string category) const;

	///Returns an std::vector of pointers to all of the objects whos ObjectCatagory string matches the input
	std::vector<SimObject*> allInCategory(std::string category) const;

	///Returns an std::vector of pointers to copies of all of the objects. Note: These copies will not be managed by the sim environment, and must all be deleted to avoid memory leaks.
	std::vector<SimObject*> copyObjects() const;

	///Displays all the gui elements that have been set up.
	void displayGui();

	void renderObjects(JMGraphics* gr);

	void addUndoStep();
	void undo();
	void redo();

	///Adds a gui guide on the x axis. Parameters are in percentage of window width.
	void addGuideX(float startPos, float min, float max, JMwindow* window);
	///Adds a gui guide on the x axis. Parameters are in percentage of window width. HandleMin and handleMax are indices of Y guides to limit where this guide can be clicked.
	void addGuideX(float startPos, float min, float max, JMwindow* window, unsigned int handleGrabMin, unsigned int handleGrabMax);
	///Adds a gui guide on the y axis. Parameters are in percentage of window height.
	void addGuideY(float startPos, float min, float max, JMwindow* window);
	///Adds a gui guide on the x axis. Parameters are in percentage of window width. HandleMin and handleMax are indices of X guides to limit where this guide can be clicked. 
	void addGuideY(float startPos, float min, float max, JMwindow* window, unsigned int handleGrabMin, unsigned int handleGrabMax);

	///Alligns a gui element handler between two guide lines. Must add guide lines with addGuideX and addGuideY first.
	void allignToGuides(GuiElementHandler* menu, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
	///Alligns a tabbed menu between two guide lines. Must add guide lines with addGuideX and addGuideY first.
	void allignToGuides(TabbedMenu* menu, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);
	///Alligns a viewer between two guide lines. Must add guide lines with addGuideX and addGuideY first.
	void allignToGuides(ObjectViewer* viewer, unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);

	///Writes the environment and all object to the stream.
	bool saveEnvironment(std::ostream& stream) const;
	///Loads an environment and all objects from a stream.
	bool loadEnvironment(std::istream& stream);

	///Saves the environment to the projectFilePath directory if it isn't empty, or opens a file explorer window if it is.
	bool saveEnvironment(JMwindow* window);
	///Loads the environment from a user selected file. Returns false if the file is invalid/doesn't have the correct extension.
	bool loadEnvironment(JMwindow* window);

	void setInfoBarMessage(std::string message);

	///Cleares stored file path and opens a save file dialog.
	bool saveEnvironmentAs(JMwindow* window);

	///Loads the backup save file if it exists.
	void recoverBackup();
	///Saves a backup file.
	void saveBackup();

	///Adds a derived simObject classes constructor to the internal factory map. Must register all derived classes for saving and loading from a file to work.
	///The first parameter is whatever the derived class sets its ObjectType string to in its constructor.
	///The second parameter must be a function or lambda that returns the new object instace. For example: []() { return new DerivedClass(); }
	void registerDerivedSimObject(const std::string& objectType, std::function<SimObject* ()> constructor);

	///Returns a pointer to the global resource manager for all SimEnvironments.
	SimResourceManager* getManager();

	///Returns the Euler angles of rotation from a matrix
	glm::dvec3 extractAngles(glm::dmat4 mat);

	float guideMargin = 5.0f;

	///The maximum number of undo steps to store.
	int undoSteps = 64;
	///The maximum number of redo steps to store.
	int redoSteps = 16;

	///The maximum number of threads to use when simulating.
	int simThreadNum = 10;

	///If true, objects can be selected in the viewer by clicking on them.
	bool selectInViewer = true;
	///If true, clicking in empty space in the viewer will deselect all objects.
	bool deselectOnClick = true;
	///If true, a box will be rendered around selected objects in the viewer.
	bool selectedBox = true;


	///All distance calculations should be scaled by this. 1.0 = meters.
	double distanceScale = 0.01;
	///All magnetic flux density calculations should be scaled by this. 1.0 = Tesla.
	double fluxDensityScale = 1.0;

	///The name of the current project
	std::string EnvironmentName = "Simulator";

	///All of the objects currently in the sim environment.
	///DO NOT add to or remove from this directly. This may cause dangling pointers.
	///Use addObject and deleteObject functions.
	std::vector<SimObject*> allObjects;

	///The function used to setup the menu that is displayed in the properties menu when no object is selected.
	std::function<GuiElementHandler*(TabbedMenu*,SimEnvironment*)> setupNoObjSelectedMenu = nullptr;

	///The menu that is displayed in the object properties menu when no object is selected.
	GuiElementHandler* noObjSelectedMenu = nullptr;
	TabbedMenu* propertiesMenu = nullptr;
	GuiElementHandler* selector = nullptr;
	ListMenu* selectorList = nullptr;
	ObjectViewer* viewer = nullptr;
	GuiElementHandler* objectDataBackground = nullptr;
private:

	///sets the object properties menu pointers to the selected objects menus. If there is no selected object, the menus are set to noObjectSelectedMenu.
	void setObjectMenus();

	///Is called if the setupNoObjSelectedMenu function is not set before calling setupPropertiesMenu.
	GuiElementHandler* setupNoObjMenuDefault(TabbedMenu* propertiesMenu);

	///Adds clones of all the objects to a vector, then adds that vector to the input vector.
	void copyObjectsToVector(std::vector<std::vector<SimObject*>>& vec);
	///Deletes all the objects and replaces them with all the objects in the vector at the back of the input vector. Does nothing if the stack is empty.
	void copyObjectsFromVector(std::vector<std::vector<SimObject*>>& vec);
	///Deletes all the objects in the vector at the back of the input vector, then pops it.
	void popObjectVector(std::vector<std::vector<SimObject*>>& vec);
	///Deletes all the objects in the vector at the front of the input vector, then pops it.
	void popFrontObjectVector(std::vector<std::vector<SimObject*>>& vec);

	void updateGuides();
	void detectMouseOverGuides();
	void handleGuideMovement();

	///Creates two new guiGuideCenters and pushes them two the back of the guide center vectors.
	bool createGuideCenters(unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2);

	void checkWindowResized(JMGraphics* gr);

	///Guide line
	struct guiGuide {
		float pos = 0.0f;
		float min = 0.0f;
		float max = 0.0f;
		float pixelPos = 0.0f;
		float handleSize = 5.0f;
		unsigned int handleMin = 0;
		unsigned int handleMax = 0;
		JMwindow* window = nullptr;

		bool mouseOverX(SimEnvironment* env) const;
		bool mouseOverY(SimEnvironment* env) const;
	};

	///Stores the center between two guide lines
	struct guiGuideCenter {
		unsigned int guide1 = 0;
		unsigned int guide2 = 0;
		float pixelPos = 0.0f;
		float pixelSize = 0.0f;
	};

	std::vector<guiGuide> xGuides;
	std::vector<guiGuide> yGuides;

	std::vector<guiGuideCenter*> xGuideCenters;
	std::vector<guiGuideCenter*> yGuideCenters;

	///The guide that the mouse is over. Is -1 if the mouse is over none.
	int mouseOverGuideX = -1;
	///The guide that the mouse is over. Is -1 if the mouse is over none.
	int mouseOverGuideY = -1;

	SimObject* selectedObj = nullptr;
	std::vector<SimObject*> SelectedObjects{};

	float objectDataSizeX = 0.0f;
	float objectDataSizeY = 0.0f;

	std::vector<std::vector<SimObject*>> undoVector;
	std::vector<std::vector<SimObject*>> redoVector;

	using FactoryMap = std::unordered_map<std::string, std::function<SimObject* ()>>;
	///Used to de-serialize derived simObject classes in the load function.
	FactoryMap loadFactory;

	std::mutex renderMtx;

	bool ObjectsFrozen = false;

	float windowWidthLast = 0.0f;
	float windowHeightLast = 0.0f;

	SimResourceManager* manager;
};

///Singleton resource manager class for all SimEnvironment instances. Handles saving and loading, as well as sharing data between environments.
class SimResourceManager {
public:

	static SimResourceManager& getInstance() {
		static SimResourceManager instance;
		return instance;
	};

	SimResourceManager(SimResourceManager&) = delete;
	SimResourceManager& operator=(const SimResourceManager) = delete;

	///Saves all SimEnvironment objects in the vector to the stored filepath, or opens a file dialog if the file path is empty.
	void save(JMwindow* window);
	void saveAs(JMwindow* window);
	///Opens a file dialog and loads a project from the selected file.
	void load(JMwindow* window);

	void save(std::string filepath);
	void load(std::string filepath);

	///Loads the backup save file if it exists.
	void recoverBackup();
	///Saves a backup file.
	void saveBackup();

	///Generates a new name for the object if it matches any other objects in any SimEnvironment.
	void makeNameUnique(SimObject* obj);

	///Searches for an object by name in all environments and returns a pointer to it if found, else returns nullptr.
	SimObject* findObject(std::string name);

	///Returns a vector of pointers to all sim objects in all sim environments.
	std::vector<SimObject*> allSimObjects();

	std::vector<SimEnvironment*> allEnvironments;

	std::string projectName = "unsaved project";
	std::string infoBarMessage = "";

	///The file extension used when saving and loading. Will only be able to open files with this extension.
	///Only set once for all simEnvironments, as it's in the singleton resource manager. Must start with '.'
	std::string fileExtension = ".jmeditor";
private:

	SimResourceManager();
	~SimResourceManager();

	std::string projectFilePath = "";
	std::string backupFilePath = "backupSave";
};