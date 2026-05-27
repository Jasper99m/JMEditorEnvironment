#include "ScreenStateManager.h"
#include "../../JMgui/JMgui.h"
#include "iostream"

ScreenState::ScreenState() {

}

ScreenState::~ScreenState() {

}

void ScreenState::setup() {

}

bool ScreenState::internalSetup(JMwindow* Window) {
	if (Window->window == nullptr) {
		std::cerr << "ScreenState Error: Must initialize the JMwindow object before seting up the ScreenState." << std::endl;
		return false;
	}
	window = Window;
	gr = window->window;

	window->animatedStartup = true;

	setup();
	glFinish();
	IsSetup = true;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	return true;
}

bool ScreenState::isSetup() const {
	return IsSetup;
}

void ScreenState::display() {
	gr->fill(window->color_lines1);
	gr->setFont(window->font_standard);
	gr->textSize(1.0f);
	gr->text(screenTitle, 10.0f, 10.0f);
}

float ScreenState::midPoint(float a, float b) {
	return (a + b) / 2.0f;
}