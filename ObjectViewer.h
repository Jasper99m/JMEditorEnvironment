#pragma once
#include <JMgui.h>

//Forward declarations. Implementation must include ObjectHandler.h.
class SimObject;
class SimEnvironment;

/**
* A 3D evnironment rendered on a seperate thread that displays the SimObjects contained in a SimEnvironment instance.
* Allows the user to navigate the 3D environment as well as select and transform the SimObjects.
*/
class ObjectViewer {
public:
	ObjectViewer(SimEnvironment* Handler, JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY);
	~ObjectViewer();

	ObjectViewer(ObjectViewer&) = delete;
	ObjectViewer& operator=(const ObjectViewer&) = delete;
	ObjectViewer(ObjectViewer&&) = default;
	ObjectViewer& operator=(ObjectViewer&&) = default;

	void display();

	//Set the position in the window in pixel coords.
	void setPos(float x, float y);
	//Set the size in the window in pixel coords.
	void setSize(float x, float y);

	///Moves the camera to point at a specific object.
	void focusOnObject(SimObject* obj);

	///Moves the camera to point at a specific location.
	void focusOnCoord(float x, float y, float z);

	///Returns the distance from the camera to the object if the mouse is over it, else returns -1.0f. Is kind of expensive, not recomended to call every frame.
	float checkMouseOverObject(SimObject* obj);
	///returns true if the mouse is over the viewer.
	bool mouseOver() const;

	///Returns true if an object is being translated, rotated or scaled in the viewport
	bool transformingInViewport() const;

	/// size of the x, y and z axis display in the lower left corner.
	float axisDispSize = 80.0f;

	///the mouse sensitivity when rotating the view
	float rotSesitivity = 1.0f;
	///the scoll wheel sensitivity when zooming
	float zoomSensitivity = 1.0f;
	///the spacing of the grid lines that are displayed in the 3D viewport. Set this before calling display() for the first time.
	float gridSpacing = 100.0f;
	///the color of the grid lines that are displayed in the 3D viewport. Set this before calling display() for the first time.
	glm::vec4 gridColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	///if set to true, the view will slowly rotate until any mouse input is recieved.
	bool spin = false;

	JMwindow* window = nullptr;
	JMGraphics* gr = nullptr;
	RenderEnvironment* rEnvironment = nullptr;

	///Starts translating the selected object if pressed.
	unsigned int translateKey = GLFW_KEY_T;
	///Starts rotating the selected object if pressed.
	unsigned int rotateKey = GLFW_KEY_R;
	///Starts scaling the selected object if pressed.
	unsigned int scaleKey = GLFW_KEY_S;

	///If this is not nullptr, the size will be automatically changed when this changes.
	float* xSizeRef = nullptr;
	///If this is not nullptr, the size will be automatically changed when this changes.
	float* ySizeRef = nullptr;
	///If this is not nullptr, the position will be automatically changed when this changes.
	float* xPosRef = nullptr;
	///If this is not nullptr, the position will be automatically changed when this changes.
	float* yPosRef = nullptr;

private:
	void setupRenderEnvironment();
	void setupGuiElements();
	void renderAxisDisp();
	void handleTransformInput();

	///Handles moving the view towards moveToCoord if movingView is true;
	void moveView();

	void renderGrid(JMGraphics* g);

	void renderThread(JMGraphics* gr);

	void duplicateSelected();

	///Starts object transformation using the mouse.
	void startTranslatingSelected();
	void startRotatingSelected();
	void startScalingSelected();

	///Handles tranforming an object using the mouse.
	void translateObj();
	void rotateObj();
	void scaleObj();

	void stopObjTransform();

	///Renders the graphics around the selected object if it is being transformed using the mouse.
	void renderObjectTransformGraphics(JMGraphics* g);
	///Handles locking different axis with keyboard input while transforming an object.
	void setTransformAxisLock();

	void handleTransformTyped();

	///returns the axis to transform along/around from the mouse coordinates.
	glm::vec3 getTransformAxis();
	
	SimEnvironment* handler = nullptr;

	JMGraphics::Buffer3D* axisDispBuffer = nullptr;
	JMGraphics::Buffer3D* viewBuffer = nullptr;
	JMGraphics::Buffer* gridBuffer = nullptr;
	GuiElementHandler* backgroundElements = nullptr;
	GuiElementHandler* guiElements = nullptr;

	///The object that translation, rotation or scale is applied to when moving using the mouse.
	SimObject* transformingObject = nullptr;
	///used to store the old properties when translating or scaling an object, in case the action is canceled.
	glm::dvec3 oldObjTransform = glm::dvec3(1.0, 1.0, 1.0);
	///used to store the old rotation in case the action is canceled
	glm::dquat oldObjRot = glm::dquat(glm::dvec3(0.0, 0.0, 0.0));
	///stores the total amount the object has been transformed by in this action
	glm::dvec3 totalObjTransform = glm::dvec3(0.0, 0.0, 0.0);
	///stores any numbers typed while transforming an object.
	std::string transformTyped = "";
	float transformTypedFloat = 0.0f;

	bool translatingObj = false;
	bool rotatingObject = false;
	bool scalingObject = false;

	///lock specific axis while transforming an object with the mouse
	bool transLockX = false;
	bool transLockY = false;
	bool transLockZ = false;

	bool transSnapping = false;
	float transSnappingDist = 10.0f;

	bool renderLoopStarted = false;
	bool MouseOver = false;

	float posX = 0.0f;
	float posY = 0.0f;
	float sizeX = 300.0f;
	float sizeY = 300.0f;

	float sizeXLast = 0.0f;
	float sizeYLast = 0.0f;

	float localMouseX = 0.0f;
	float localMouseY = 0.0f;

	float localMouseXLast = 0.0f;
	float localMouseYLast = 0.0f;

	float viewTransX = 0.0f;
	float viewTransY = 0.0f;
	float viewTransZ = 0.0f;

	float viewRotX = 0.2f;
	float viewRotY = QUARTER_PI;
	float viewRotZ = 0.0f;

	float viewScale = 1.0f;

	///if true, the view is moved towards the moveToCoord.
	bool movingView = false;

	///The coordinates to move to if the view is being automaticaly moved.
	glm::vec3 moveToCoord = glm::vec3(0.0f, 0.0f, 0.0f);
	float moveToZoom = 0.0f;

	///For debugging object clicking
	glm::vec3 mouseRayPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 mouseRayDir = glm::vec3(0.0f, 0.0f, 0.0f);
};