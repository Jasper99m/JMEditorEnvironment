#include "ScreenStateManager.h"
#include "../../JMgui/JMgui.h"

ScreenStateManager::ScreenStateManager(JMwindow* Window) {
	if (Window == nullptr || Window->window == nullptr) {
		std::cerr << "ScreenStateManager Error: Must be initialized with a valid, already initialized JMwindow object." << std::endl;
		return;
	}
	window = Window;
	gr = window->window;
}

ScreenStateManager::~ScreenStateManager() {
	for (ScreenState* i : allStates) {
		delete(i);
	}
}

void ScreenStateManager::display() {
	if (allStates.empty()) { return; }

	if (activeScreenState == nullptr) {
		activeScreenState = allStates[0];
	}

	if (activeScreenState->isSetup()) {
		activeScreenState->display();
	}
	else {
		if (!activeScreenState->internalSetup(window)) {
			std::cerr << "ScreenStateManager Error: Failed to set up screen state." << std::endl;
		}
	}

	if (allStates.size() > 1) { displayStateSelector(); }
}

bool ScreenStateManager::addScreenState(ScreenState* state) {
	if (state == nullptr) { return false; }

	allStates.push_back(state);

	state->internalSetup(window);

	return true;
}

void ScreenStateManager::displayStateSelector() {

	//Open/close and background graphics
	if (gr->mouseY() > gr->height() - window->titleBarWidth - selectorHeight * 2.0f && gr->mouseY() < gr->height() - window->titleBarWidth) {
		anim_selectorHint = gr->smoothIncrease(anim_selectorHint, 12.0f);
	}
	else if (!selectorOpen) {
		anim_selectorHint = gr->linearDecrease(anim_selectorHint, 8.0f);
	}

	if (anim_selectorHint == 0.0f) { return; }


	if (anim_selectorOpen == 0.0f) {
		gr->fill(window->color_background1);
		gr->noStroke();
		gr->rect(gr->mouseX(), gr->height() - window->titleBarWidth - 5.0f * anim_selectorHint, 30.0f + selectorOpenTimer * 100.0f, 10.0f * anim_selectorHint, 10.0f);

		if (anim_selectorHint > 0.8f)
		{
			gr->stroke(window->color_lines1);
			gr->strokeWeight(3.0f);

			gr->line(gr->mouseX() - 4.0f, gr->height() - window->titleBarWidth - 3.0f, gr->mouseX(), gr->height() - window->titleBarWidth - 7.0f);
			gr->line(gr->mouseX() + 4.0f, gr->height() - window->titleBarWidth - 3.0f, gr->mouseX(), gr->height() - window->titleBarWidth - 7.0f);
		}

		if (gr->mouseY() > gr->height() - window->titleBarWidth - selectorHeight && gr->mouseY() < gr->height() - window->titleBarWidth) {
			selectorOpenTimer = gr->linearIncrease(selectorOpenTimer, 2.0f);
		}
		else {
			selectorOpenTimer = gr->linearDecrease(selectorOpenTimer, 6.0f);
		}

		if (selectorOpenTimer == 1.0f) { selectorOpen = true; }
	}
	else {
		glm::vec4 color1 = gr->lerpColor(window->color_clear, window->color_background1, anim_selectorOpen * 2.0f);
		glm::vec4 color2 = gr->lerpColor(window->color_clear, window->color_background1, anim_selectorOpen * 2.0f - 1.0f);

		gr->pushMatrix();
		gr->translate(gr->width() / 2.0f, gr->height() - window->titleBarWidth - selectorHeight / 2.0f);
		gr->rotate(-PI / 2.0f);
		gr->gradient(0.0f, 0.0f, selectorHeight, gr->width(), color1, color2);

		gr->popMatrix();
		if (gr->mouseY() < gr->height() - window->titleBarWidth - selectorHeight * 1.5f) {
			selectorOpen = false;
			selectorOpenTimer = 0.0f;
		}
	}

	if (selectorOpen) {
		anim_selectorOpen = gr->smoothIncrease(anim_selectorOpen, 9.0f);
	}
	else {
		anim_selectorOpen = gr->linearDecrease(anim_selectorOpen, 5.0f);
	}

	//State buttons
	if (selectorOpen) {
		float localMouseX = gr->mouseX();
		gr->pushMatrix();
		gr->translate(0.0f, gr->height() - (window->titleBarWidth + selectorHeight / 2.0f));
		gr->setFont(window->font_standard);
		gr->textSize(1.0f);
		gr->strokeWeight(1.0f);
		gr->stroke(window->color_lines1);
		gr->noStroke();
		for(ScreenState* i : allStates) {
			glm::vec2 titleSize;
			gr->fill(window->color_lines1);
			gr->text(i->screenTitle, 20.0f, -4.0f, titleSize);
			gr->line(titleSize.x + 40.0f, -selectorHeight / 2.0f + 4.0f, titleSize.x + 40.0f, selectorHeight / 2.0f - 4.0f);

			if (i == activeScreenState)
				gr->line(10.0f, -selectorHeight / 2.0f - 3.0f, titleSize.x + 30.0f, -selectorHeight / 2.0f - 3.0f);

			if (localMouseX > 0.0f && localMouseX < titleSize.x + 40.0f) {
				if (gr->mousePressed()) { gr->fill(window->color_lines1.x, window->color_lines1.y, window->color_lines1.z, 0.3f); }
				else { gr->fill(window->color_lines1.x, window->color_lines1.y, window->color_lines1.z, 0.1f); }

				gr->rect(titleSize.x / 2.0f + 20.0f, 0.0f, titleSize.x + 40.0f, selectorHeight);
				if (gr->mouseClicked()) { activeScreenState = i; }
			}

			gr->translate(titleSize.x + 40.0f, 0.0f);
			localMouseX -= titleSize.x + 40.0f;
		}
		gr->popMatrix();
	}
}