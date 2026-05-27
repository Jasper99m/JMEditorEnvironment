#pragma once
#include <vector>


//Forward declarations. Implementation must include JMgui.h.
class JMwindow;
class JMGraphics;

/**
* The base class for a selectable screen state. When using a ScreenStateManager containing the derived ScreenStates,
* the user can navigate between the ScreenStates using tabs at the top of the screen.
* Create classes derived from this in order to use them with
* a ScreenStateManager.
* Put anything that needs to run once before this screen state is rendered
* in setup(), and put everthing that needs to run once every frame in display().
*/
class ScreenState {
public:
	ScreenState();
	virtual ~ScreenState();

	ScreenState(ScreenState&) = delete;
	ScreenState& operator=(const ScreenState&) = delete;
	ScreenState(ScreenState&&) = default;
	ScreenState& operator=(ScreenState&&) = default;

	///Is called once every frame in the draw loop.
	virtual void display();
	///Is called one time before being displayed.
	virtual void setup();
	///Is called by the screen state handler to set up the window.
	bool internalSetup(JMwindow* Window);
	///Returns true if this screen states setup function has been called.
	bool isSetup() const;

	///returns the median of two numbers.
	float midPoint(float a, float b);

	///The title of the screen state. This is displayed on the tab if multiple screen states exist.
	const char* screenTitle = " ";

protected:
	///The window that the screen state has been set up on.
	JMwindow* window = nullptr;
	JMGraphics* gr = nullptr;

private:

	///is set to true if setup() has been called at least once.
	bool IsSetup = false;
};

/**
* Manages setting up and displaying screen states.
* First, create a screen state object derived from ScreenState,
* then create and init an instance of it, set the screenTitle string
* and use addScreenState() to add it to the manager.
* 
* If there is only one screen state, the manager will set it up and
* display it. If multiple are added, tabs will apear at the top of the 
* screen to select a state by the states screenTitle.
* Each state will only be set up the first time it is accessed.
*/
class ScreenStateManager {
public:
	ScreenStateManager(JMwindow* Window);
	~ScreenStateManager();

	ScreenStateManager(ScreenStateManager&) = delete;
	ScreenStateManager& operator=(const ScreenStateManager&) = delete;
	ScreenStateManager(ScreenStateManager&&) = default;
	ScreenStateManager& operator=(ScreenStateManager&&) = default;

	///call this once inside the draw loop to display the current screen state, and the screen state selector gui.
	void display();

	bool addScreenState(ScreenState* state);
private:

	void displayStateSelector();

	ScreenState* activeScreenState = nullptr;
	JMwindow* window = nullptr;
	JMGraphics* gr = nullptr;

	bool selectorOpen = false;
	float selectorHeight = 20.0f;

	float anim_selectorHint = 0.0f;
	float anim_selectorOpen = 0.0f;
	float selectorOpenTimer = 0.0f;

	std::vector<ScreenState*> allStates;
};
