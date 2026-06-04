#include "ObjectViewer.h"
#include "../SimEnvironment.h"

ObjectViewer::ObjectViewer(SimEnvironment* Handler, JMwindow* Window, float PosX, float PosY, float SizeX, float SizeY) {
	if (Window == nullptr || Window->window == nullptr) {
		std::cerr << "ObjectViewer Error: Must initialize with an initialized JMwindow object." << std::endl;
		return;
	}
	posX = PosX;
	posY = PosY;
	sizeX = SizeX;
	sizeY = SizeY;
	window = Window;
	gr = window->window;
	handler = Handler;

	setupRenderEnvironment();
	setupGuiElements();
}

ObjectViewer::~ObjectViewer() {
	delete(rEnvironment);
	delete(axisDispBuffer);
	delete(gridBuffer);
	delete(backgroundElements);
	delete(guiElements);
}

void ObjectViewer::renderThread(JMGraphics* g) {

	if (spin) { viewRotY += 0.00015f * g->frameTime(); }

	if (gridBuffer == nullptr) {
		gridBuffer = new JMGraphics::Buffer(2048, 2048, rEnvironment->gr);
		renderGrid(g);
	}

	if (viewBuffer->width() != sizeX || viewBuffer->height() != sizeY) { 
		viewBuffer->setSize(sizeX, sizeY);
		glFinish();
	}

	g->image(viewBuffer, g->width() / 2.0f, g->height() / 2.0f, g->width(), g->height());

	viewBuffer->beginDraw();
	g->background(0.0f, 0.0f, 0.0f);
	g->resetMatrix();

	g->translate(g->width() / 2.0f, g->height() / 2.0f, -500.0f);
	g->scale(viewScale, viewScale, viewScale);
	g->rotate(viewRotX, viewRotY, viewRotZ);
	g->translate(viewTransX, viewTransY, viewTransZ);

	//Draws the last mouse ray for debugging mouse clicking in 3d space.
	/*g->stroke(0.8f, 0.7f, 0.0f);
	g->strokeWeight(3.0f);
	g->line(mouseRayPos.x + mouseRayDir.x * 10.0f, mouseRayPos.y + mouseRayDir.y * 10.0f, mouseRayPos.z + mouseRayDir.z * 10.0f,
		mouseRayPos.x + mouseRayDir.x * 10000.0f, mouseRayPos.y + mouseRayDir.y * 10000.0f, mouseRayPos.z + mouseRayDir.z * 10000.0f);
	*/

	renderObjectTransformGraphics(g);

	handler->renderObjects(g);
	g->rotateX(HALF_PI);
	g->image(gridBuffer, 0.0f, 0.0f);
	viewBuffer->endDraw();
}

void ObjectViewer::display() {
	if (rEnvironment->gr == nullptr) { return; }

	//user input
	localMouseXLast = localMouseX;
	localMouseYLast = localMouseY;

	localMouseX = gr->mouseX() - (posX - sizeX / 2.0f);
	localMouseY = gr->mouseY() - (posY - sizeY / 2.0f);

	MouseOver = (localMouseX > 0.0f && localMouseX < sizeX && localMouseY > 0.0f && localMouseY < sizeY);

	handleTransformInput();
	moveView();
		
	backgroundElements->display();

	//display the rendered environment
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	gr->noStroke();
	gr->fill(0.0f, 0.0f, 0.0f);
	gr->rect(posX, posY, sizeX, sizeY, 6.0f);

	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	rEnvironment->display(posX, posY);

	glDisable(GL_STENCIL_TEST);
	gr->noFill();

	if (mouseOver()) { gr->stroke(window->color_lines1.x, window->color_lines1.y, window->color_lines1.z, 0.25f); }
	else { gr->stroke(window->color_lines1.x, window->color_lines1.y, window->color_lines1.z, 0.2f); }

	gr->strokeWeight(2.0f);
	gr->rect(posX, posY, sizeX, sizeY, 6.0f);

	//display forground gui
	renderAxisDisp();
	gr->image(axisDispBuffer, posX - sizeX / 2.0f + axisDispSize / 2.0f, posY - sizeY / 2.0f + axisDispSize / 2.0f, axisDispSize, axisDispSize);
	guiElements->display();

	if (xSizeRef != nullptr && *xSizeRef != sizeX) { sizeX = *xSizeRef; }
	if (ySizeRef != nullptr && *ySizeRef != sizeY) { sizeY = *ySizeRef; }
	if (xPosRef != nullptr && *xPosRef != posX) { posX = *xPosRef; }
	if (yPosRef != nullptr && *yPosRef != posY) { posY = *yPosRef; }

	if ((sizeX != sizeXLast || sizeY != sizeYLast) && !gr->mousePressed()) { setSize(sizeX, sizeY); }
}

float ObjectViewer::checkMouseOverObject(SimObject* obj) {
	if (obj == nullptr || !mouseOver())
		return -1.0f;

	//get the mouse ray in object local space.--------------------------------
	
	
	glm::vec3 rayOrigin = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::vec3 rayDir = glm::vec3(0.0f, 0.0f, -1.0f);
	if (viewBuffer->perspectiveProjection) {
		//Get mouse position in NDC
		rayOrigin = glm::vec3((localMouseX / rEnvironment->gr->width()) * 2.0f - 1.0f, 1.0f - (localMouseY / rEnvironment->gr->height()) * 2.0f, 0.0f);
		//Get ray direction using projection matrix
		glm::vec4 rayClip = glm::vec4(rayOrigin.x, rayOrigin.y, 1.0f, -1.0f);
		glm::vec4 rayEye = glm::inverse(viewBuffer->projectionMatrix()) * rayClip;
		rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
		rayDir = glm::normalize(glm::vec3(rayEye));
	}
	else {
		rayOrigin = glm::vec3(localMouseX - rEnvironment->gr->width() / 2.0f, -(localMouseY - rEnvironment->gr->height() / 2.0f), 5000.0f);
		rayOrigin *= 2.0f;
	}

	//Generate view point matrix
	glm::mat4 viewMat = glm::mat4(1.0f);
	viewMat = glm::translate(viewMat, glm::vec3(0.0f, 0.0f, -500.0f));
	viewMat = glm::scale(viewMat, glm::vec3(viewScale, viewScale, viewScale));
	viewMat = glm::rotate(viewMat, viewRotX, glm::vec3(1.0f, 0.0f, 0.0f));
	viewMat = glm::rotate(viewMat, viewRotY, glm::vec3(0.0f, 1.0f, 0.0f));
	viewMat = glm::rotate(viewMat, viewRotZ, glm::vec3(0.0f, 0.0f, 1.0f));
	viewMat = glm::translate(viewMat, glm::vec3(viewTransX, viewTransY, viewTransZ));

	//Apply view point matrix to the direction vector.
	rayDir.x *= viewBuffer->width() / 2.0f;
	rayDir.y *= -viewBuffer->height() / 2.0f;
	rayDir = glm::inverse(viewMat) * glm::vec4(rayDir, 0.0f);
	rayDir = glm::normalize(rayDir);
	
	//Apply the view point matrix to the origin.
	rayOrigin.y *= -0.5f;
	rayOrigin.x *= 0.5f;
	rayOrigin = glm::inverse(viewMat) * glm::vec4(rayOrigin, 1.0f);

	//For debugging
	mouseRayPos = rayOrigin;
	mouseRayDir = rayDir;
	
	//Transform the rays to object local space
	glm::mat4 objMat = glm::mat4(1.0f);
	objMat = glm::translate(objMat, glm::vec3((float)obj->posx(), (float)obj->posy(), (float)obj->posz()));
	objMat = glm::rotate(objMat, (float)obj->rotx(), glm::vec3(1.0f, 0.0f, 0.0f));
	objMat = glm::rotate(objMat, (float)obj->roty(), glm::vec3(0.0f, 1.0f, 0.0f));
	objMat = glm::rotate(objMat, (float)obj->rotz(), glm::vec3(0.0f, 0.0f, 1.0f));
	
	rayOrigin = glm::inverse(objMat) * glm::vec4(rayOrigin, 1.0f);
	rayDir = glm::inverse(objMat) * glm::vec4(rayDir, 0.0f);
	rayDir = glm::normalize(rayDir);


	//Detect ray-AABB intersection
	glm::vec3 boxMin( -obj->sizex() * 0.5f, -obj->sizey() * 0.5f, -obj->sizez() * 0.5f);
	glm::vec3 boxMax(obj->sizex() * 0.5f, obj->sizey() * 0.5f, obj->sizez() * 0.5f);

	float tMin = (boxMin.x - rayOrigin.x) / rayDir.x;
	float tMax = (boxMax.x - rayOrigin.x) / rayDir.x;

	if (tMin > tMax) { std::swap(tMin, tMax); }

	float tyMin = (boxMin.y - rayOrigin.y) / rayDir.y;
	float tyMax = (boxMax.y - rayOrigin.y) / rayDir.y;

	if (tyMin > tyMax) { std::swap(tyMin, tyMax); }

	if ((tMin > tyMax) || (tyMin > tMax)) { return -1.0f; }

	if (tyMin > tMin) { tMin = tyMin; }

	if (tyMax < tMax) { tMax = tyMax; }

	float tzMin = (boxMin.z - rayOrigin.z) / rayDir.z;
	float tzMax = (boxMax.z - rayOrigin.z) / rayDir.z;

	if (tzMin > tzMax) { std::swap(tzMin, tzMax); }

	if ((tMin > tzMax) || (tzMin > tMax)) { return -1.0f; }

	return tMin >= 0.0f ? tMin : -1.0f;
}

bool ObjectViewer::mouseOver()  const {
	return MouseOver;
}

bool ObjectViewer::transformingInViewport()  const {
	return translatingObj || rotatingObject || scalingObject;
}
void ObjectViewer::setPos(float x, float y) {
	posX = x;
	posY = y;

	backgroundElements->xOffset = x;
	backgroundElements->yOffset = y;
	guiElements->xOffset = posX + sizeX / 2.0f;
	guiElements->yOffset = posY - sizeY / 2.0f;
}
void ObjectViewer::setSize(float x, float y) {
	sizeX = x;
	sizeY = y;
	rEnvironment->setSize(x, y);
	backgroundElements->xSize = x;
	backgroundElements->ySize = y;

	guiElements->xOffset = posX + sizeX / 2.0f;
	guiElements->yOffset = posY - sizeY / 2.0f;

	sizeXLast = sizeX;
	sizeYLast = sizeY;
}

void ObjectViewer::focusOnObject(SimObject* obj) {
	if (obj == nullptr) { return; }

	focusOnCoord(-(float)obj->posx(), -(float)obj->posy(), -(float)obj->posz());
	float objSize = sqrt((float)obj->sizex() * (float)obj->sizex() + (float)obj->sizey() * (float)obj->sizey() + (float)obj->sizez() * (float)obj->sizez());
	moveToZoom = 1.0f / (objSize * 0.003f);
	handler->setInfoBarMessage("Focusing on object: " + obj->name);
}

void ObjectViewer::focusOnCoord(float x, float y, float z) {
	movingView = true;
	spin = false;
	moveToCoord = glm::vec3(x, y, z);
}

void ObjectViewer::moveView() {
	if (!movingView) { return; }

	if (gr->mousePressedRight() || gr->ScrolledLast() != 0.0f) {
		moveToZoom = 0.0f;
		movingView = false;
		return;
	}

	glm::vec3 currentTrans = glm::vec3(viewTransX, viewTransY, viewTransZ);
	glm::vec3 currentRot = glm::vec3(viewRotX, viewRotY, viewRotZ);

	if (glm::distance(currentTrans, moveToCoord) < 2.0f && glm::distance(currentRot, glm::vec3(0.0f, 0.0f, 0.0f)) < 0.02f&& (moveToZoom == 0.0f || abs(moveToZoom - viewScale) < 0.02f)) {
		moveToZoom = 0.0f;
		movingView = false;
		return;
	}

	viewRotX += (0.17f - viewRotX) * gr->frameTime() / 150.0f;
	viewRotY += (QUARTER_PI - viewRotY) * gr->frameTime() / 150.0f;
	viewRotZ += (-viewRotZ) * gr->frameTime() / 150.0f;

	glm::vec3 moveDir = (moveToCoord - currentTrans) * gr->frameTime() / 100.0f;

	viewTransX += moveDir.x;
	viewTransY += moveDir.y;
	viewTransZ += moveDir.z;

	if (moveToZoom == 0.0f) { return; }

	viewScale += (moveToZoom - viewScale) * gr->frameTime() / 100.0f;
}

void ObjectViewer::setupRenderEnvironment() {
	rEnvironment = new RenderEnvironment(sizeX, sizeY, window);
	rEnvironment->waitForDisplay = true;
	rEnvironment->gr->limitFrameRate(60);
	rEnvironment->gr->textureFxaa(true);
	rEnvironment->gr->bufferNearestFilter = true;
	rEnvironment->gr->roundedStroke = false;

	viewBuffer = new JMGraphics::Buffer3D(sizeX, sizeY, rEnvironment->gr);
	viewBuffer->fov = 0.002f;

	rEnvironment->setRenderFunc([this](JMGraphics* g) { renderThread(g); });
	rEnvironment->startLoop();
}

void ObjectViewer::setupGuiElements() {
	axisDispBuffer = new JMGraphics::Buffer3D(axisDispSize, axisDispSize, gr);
	axisDispBuffer->fov = 0.03f;

	backgroundElements = new GuiElementHandler(window);
	backgroundElements->xOffset = posX;
	backgroundElements->yOffset = posY;
	backgroundElements->xSize = sizeX;
	backgroundElements->ySize = sizeY;
	//backgroundElements->addElement(new BackgroundBox(0.0f, 0.0f, sizeX + 10.0f, sizeY + 10.0f));

	guiElements = new GuiElementHandler(window);
	guiElements->xOffset = posX + sizeX / 2.0f;
	guiElements->yOffset = posY - sizeY / 2.0f;

	//Help button
	JMGraphics::Buffer* helpButtImage = new JMGraphics::Buffer(20.0f, 20.0f, window->window);
	helpButtImage->beginDraw();
	gr->fill(0.8f);
	gr->setFont(window->font_large);
	gr->textSize(1.0f);
	gr->text("?", 2.0f, 2.0f);

	helpButtImage->endDraw();

	CustomButton* helpButton = new CustomButton(-25.0f, 15.0f, helpButtImage);
	helpButton->helpMessage = "Rotate view:\n    Right mouse button\n    or V + arrows\n    or V + 1, 2 or 3\nTranslate view:  shift + right mouse button\nZoom:  Mouse wheel or ctrl + '+'/'-'"
		"\nOrthographic / perspective view:  ctrl + O\nSpin view:  ctrl + shift + R\n\nSelect object : Left mouse button\nSecond select object : ctrl or shift + left mouse button\nDeselect all : shift + A"
		"\nRotate selected : R\nTranslate selected : T\nScale selected : S\nDuplicate selected : shift + D\nDelete selected : backspace\nFocus on selected : shift + F\nHide / unhide selected : ctrl + H\nUnhide all : alt + H"
		"\nWhen transfroming selected:\n    Lock to axis:  X, Y or Z\n    Lock axis:    shift + X, Y or Z\n    Snapping:  shift\n    Transform by specific value:  Lock to axis then type value"
		"\n\nUndo: ctrl + Z\nRedo : ctrl + Y\nSave : ctrl + S\nSave as : ctrl + shift + S\nLoad file : ctrl + L";

	guiElements->addElement(helpButton);

	//Projection mode button
	JMGraphics::Buffer* projButtImage = new JMGraphics::Buffer(20.0f, 20.0f, window->window);
	projButtImage->beginDraw();

	gr->noFill();
	gr->strokeWeight(1.f);
	gr->stroke(0.4f);
	gr->rect(projButtImage->width() / 2.0f - 3.0f, projButtImage->height() / 2.0f + 3.0f, projButtImage->width() - 12.0f, projButtImage->height() - 12.0f);
	gr->stroke(0.8f);
	gr->rect(projButtImage->width() / 2.0f + 3.0f, projButtImage->height() / 2.0f - 3.0f, projButtImage->width() - 8.0f, projButtImage->height() - 8.0f);

	projButtImage->endDraw();

	CustomButton* projModeButton = new CustomButton(-55.0f, 15.0f, projButtImage);
	projModeButton->event_clicked->addListener([this](JMeventDispatcher::Event* E) {this->viewBuffer->perspectiveProjection = !this->viewBuffer->perspectiveProjection; });

	guiElements->addElement(projModeButton);

	//Rotate button
	JMGraphics::Buffer* rotButtImage = new JMGraphics::Buffer(20.0f, 20.0f, window->window);
	rotButtImage->beginDraw();

	gr->strokeWeight(2.0f);
	gr->stroke(0.8f);
	gr->noFill();

	gr->arc(rotButtImage->width() / 2.0f, rotButtImage->height() / 2.0f, rotButtImage->width() -10.0f, rotButtImage->height() -10.0f, 0.0f, PI * 1.5f);
	gr->translate(rotButtImage->width() - 5.0f, rotButtImage->height() / 2.0f - 5.0f);
	gr->line(0.0f, 0.0f, 5.0f, 5.0f);
	gr->line(0.0f, 0.0f, -5.0f, 5.0f);

	rotButtImage->endDraw();

	CustomButton* rotButton = new CustomButton(-85.0f, 15.0f, rotButtImage);
	rotButton->event_clicked->addListener([this](JMeventDispatcher::Event* E) { this->spin = !this->spin; this->movingView = false; });
	guiElements->addElement(rotButton);
}

void ObjectViewer::renderAxisDisp() {
	axisDispBuffer->beginDraw();
	gr->background(0.0f, 0.0f, 0.0f, 0.0f);
	gr->pushMatrix();
	gr->resetMatrix();
	gr->translate(axisDispBuffer->width() / 2.0f, axisDispBuffer->height() / 2.0f, -100.0f);
	gr->rotate(viewRotX, viewRotY, viewRotZ);

	gr->strokeWeight(2.0f);

	gr->stroke(1.0f, 0.0f, 0.0f);
	gr->line(0.0f, 0.0f, axisDispBuffer->width() / 2.0f, 0.0f);
	gr->stroke(1.0f, 1.0f, 0.0f);
	gr->line(0.0f, 0.0f, 0.0f, axisDispBuffer->width() / 2.0f);
	gr->stroke(0.0f, 0.0f, 1.0f);
	gr->rotateX(HALF_PI);
	gr->line(0.0f, 0.0f, 0.0f, axisDispBuffer->width() / 2.0f);

	gr->popMatrix();
	axisDispBuffer->endDraw();
}

void ObjectViewer::handleTransformInput() {

	//ctrl + shift keyboard shortcuts
	if (gr->controlKeyDown() && gr->shiftKeyDown()) {
		if (gr->keyPressed(GLFW_KEY_R) && mouseOver()) {
			spin = true;
			movingView = false;
		}
		if (gr->keyPressed(GLFW_KEY_Z)) { handler->redo(); }
		if (gr->keyPressed(GLFW_KEY_S)) {
			stopObjTransform();
			handler->saveEnvironmentAs(window);
		}
	}

	//ctrl keyboard shortcuts
	if (gr->controlKeyDown()) {
		if (gr->keyPressed(GLFW_KEY_O) && mouseOver()) { viewBuffer->perspectiveProjection = !viewBuffer->perspectiveProjection; }
		if (gr->keyPressed(GLFW_KEY_Z)) { handler->undo(); }
		if (gr->keyPressed(GLFW_KEY_Y)) { handler->redo(); }
		if (gr->keyPressed(GLFW_KEY_MINUS)) { viewScale *= 0.98f; }
		if (gr->keyPressed(GLFW_KEY_EQUAL)) { viewScale *= 1.02f; }
		if (gr->keyPressed(GLFW_KEY_S)) {
			stopObjTransform();
			handler->saveEnvironment(window);
		}
		if (gr->keyPressed(GLFW_KEY_L)) { handler->loadEnvironment(window); }
		if (gr->keyPressed(GLFW_KEY_H) && handler->selectedObject() != nullptr) {
			handler->selectedObject()->hide = !handler->selectedObject()->hide;
			if (handler->selectedObject()->hide) { handler->deselectAll(); }
		}
	}

	//alt keyboard shortcuts
	if (gr->altKeyDown()) {
		if (gr->keyPressed(GLFW_KEY_H)) {
			for (SimObject* i : handler->allObjects) { i->hide = false; }
		}
	}

	if (translatingObj) { translateObj(); }
	if (rotatingObject) { rotateObj(); }
	if (scalingObject) { scaleObj(); }

	if (!mouseOver()) { return; }

	//shift keyboard shortcuts
	if (gr->shiftKeyDown()) {
		if (gr->keyPressed(GLFW_KEY_A)) { handler->deselectAll(); }
		if (gr->keyPressed(GLFW_KEY_F) && handler->selectedObject() != nullptr) { focusOnObject(handler->selectedObject()); }
		if (gr->keyPressed(GLFW_KEY_D) && mouseOver()) { duplicateSelected(); }
	}

	if (gr->keyPressed(GLFW_KEY_BACKSPACE) && handler->selectedObject() != nullptr) {
		handler->addUndoStep();
		handler->deleteObject(handler->selectedObject());
	}

	//Mouse transform object
	if (!gr->controlKeyDown()) {
		if (gr->keyPressed(translateKey)) { startTranslatingSelected(); }
		if (gr->keyPressed(rotateKey)) { startRotatingSelected(); }
		if (gr->keyPressed(scaleKey)) { startScalingSelected(); }
	}

	//Move view with keyboard
	if (gr->keyDown(GLFW_KEY_V)) {
		movingView = false;
		spin = false;
		if (gr->keyPressed(GLFW_KEY_1)) { viewRotX = 0.0f; viewRotY = 0.0f; viewRotZ = 0.0f; }
		if (gr->keyPressed(GLFW_KEY_2)) { viewRotX = 0.0f; viewRotY = HALF_PI; viewRotZ = 0.0f; }
		if (gr->keyPressed(GLFW_KEY_3)) { viewRotX = HALF_PI; viewRotY = 0.0f; viewRotZ = 0.0f; }

		if (gr->keyPressed(GLFW_KEY_RIGHT)) { viewRotY -= PI / 8.0f; }
		if (gr->keyPressed(GLFW_KEY_LEFT)) { viewRotY += PI / 8.0f; }
		if (gr->keyPressed(GLFW_KEY_UP)) { viewRotX += PI / 8.0f; }
		if (gr->keyPressed(GLFW_KEY_DOWN)) { viewRotX -= PI / 8.0f; }
	}

	//keep rotation within bounds
	while (viewRotX > PI) { viewRotX -= TWO_PI; }
	while (viewRotX < -PI) { viewRotX += TWO_PI; }

	while (viewRotY > PI) { viewRotY -= TWO_PI; }
	while (viewRotY < -PI) { viewRotY += TWO_PI; }

	while (viewRotZ > PI) { viewRotZ -= TWO_PI; }
	while (viewRotZ < -PI) { viewRotZ += TWO_PI; }

	if (gr->mousePressedRight()) {
		spin = false;

		glm::vec3 mouseVec = glm::vec3(localMouseX - localMouseXLast, localMouseY - localMouseYLast, 0.0f);
		
		
		if (gr->shiftKeyDown()) {
			//translate
			glm::mat4 mouseRot = glm::mat4(1.0f);
			
			
			mouseRot = glm::rotate(mouseRot, -viewRotZ, glm::vec3(0.0f, 0.0f, 1.0f));
			mouseRot = glm::rotate(mouseRot, -viewRotY, glm::vec3(0.0f, 1.0f, 0.0f));
			mouseRot = glm::rotate(mouseRot, -viewRotX, glm::vec3(1.0f, 0.0f, 0.0f));

			mouseVec = mouseRot * glm::vec4(mouseVec, 1.0f);

			viewTransX += (mouseVec.x * 0.5f) / viewScale;
			viewTransY += (mouseVec.y * 0.5f) / viewScale;
			viewTransZ += (mouseVec.z * 0.5f) / viewScale;
		}
		else {
			//rotate
			viewRotX += -mouseVec.y * 0.005f * rotSesitivity;

			if (abs(viewRotX) < HALF_PI) { viewRotY += mouseVec.x * 0.005f * rotSesitivity; }
			else { viewRotY -= mouseVec.x * 0.005f * rotSesitivity; }
		}
	}

	viewScale *= 1.0f + (gr->scrollY() * 0.04f * zoomSensitivity);

	if (viewScale < 0.005f) {
		viewScale = 0.01f;
	}

	if (viewScale > 40.0f) {
		viewScale = 20.0f;
	}
}


void ObjectViewer::renderObjectTransformGraphics(JMGraphics* g) {
	if (transformingObject == nullptr) { return; }

	g->pushMatrix();
	g->translate((float)transformingObject->posx(), (float)transformingObject->posy(), (float)transformingObject->posz());
	if (scalingObject) { g->multiplyMatrix(glm::mat4_cast(transformingObject->rotq())); }

	g->strokeWeight(2.0f);
	if (rotatingObject) {
		float rad = ((float)transformingObject->sizex() * (float)transformingObject->sizex());
		rad += ((float)transformingObject->sizey() * (float)transformingObject->sizey());
		rad += ((float)transformingObject->sizez() * (float)transformingObject->sizez());
		rad = sqrt(rad) + 10.0f;

		g->strokeWeight(2.0f);
		g->noFill();
		if (!transLockX) {
			g->pushMatrix();
			g->rotate(0.0f, HALF_PI, 0.0f);
			g->stroke(1.0f, 0.0f, 0.0f);
			g->ellipse(0.0f, 0.0f, rad);
			if (transSnapping) {
				for (int i = 0; i < 16; i++) {
					g->line(rad / 2.0f, 0.0f, 3.0f, rad / 2.0f, 0.0f, -3.0f);
					g->line(rad / 2.0f + 3.0f, 0.0f, 0.0f, rad / 2.0f - 3.0f, 0.0f, 0.0f);
					g->rotate(QUARTER_PI / 2.0f);
				}
			}
			g->popMatrix();
		}
		if (!transLockY) {
			g->pushMatrix();
			g->rotate(HALF_PI, 0.0f, 0.0f);
			g->stroke(1.0f, 1.0f, 0.0f);
			g->ellipse(0.0f, 0.0f, rad);
			if (transSnapping) {
				for (int i = 0; i < 16; i++) {
					g->line(rad / 2.0f, 0.0f, 3.0f, rad / 2.0f, 0.0f, -3.0f);
					g->line(rad / 2.0f + 3.0f, 0.0f, 0.0f, rad / 2.0f - 3.0f, 0.0f, 0.0f);
					g->rotate(QUARTER_PI / 2.0f);
				}
			}
			g->popMatrix();
		}
		if (!transLockZ) {
			g->pushMatrix();
			g->stroke(0.0f, 0.0f, 1.0f);
			g->ellipse(0.0f, 0.0f, rad);
			if (transSnapping) {
				for (int i = 0; i < 16; i++) {
					g->line(rad / 2.0f, 0.0f, 3.0f, rad / 2.0f, 0.0f, -3.0f);
					g->line(rad / 2.0f + 3.0f, 0.0f, 0.0f, rad / 2.0f - 3.0f, 0.0f, 0.0f);
					g->rotate(QUARTER_PI / 2.0f);
				}
			}
			g->popMatrix();
		}
	}
	else {
		if (!transLockX) {
			g->stroke(1.0f, 0.0f, 0.0f);
			g->line(-5000.0f, 0.0f, 0.0f, 5000.0f, 0.0f, 0.0f);
			if (transSnapping) {
				g->pushMatrix();
				g->translate(-500.0f, 0.0f, 0.0f);
				int ticNum = (int)(1000.0f / transSnappingDist);
				for (int i = 0; i < ticNum; i++) {
					g->line(0.0f, -2.0f, 0.0f, 0.0f, 2.0f, 0.0f);
					g->line(0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 2.0f);
					g->translate(transSnappingDist, 0.0f);
				}
				g->popMatrix();
			}
		}
		if (!transLockY) {
			g->stroke(1.0f, 1.0f, 0.0f);
			g->line(0.0f, -5000.0f, 0.0f, 0.0f, 5000.0f, 0.0f);
			if (transSnapping) {
				g->pushMatrix();
				g->translate(0.0f, -500.0f, 0.0f);
				int ticNum = (int)(1000.0f / transSnappingDist);
				for (int i = 0; i < ticNum; i++) {
					g->line(-2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f);
					g->line(0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 2.0f);
					g->translate(0.0f, transSnappingDist);
				}
				g->popMatrix();
			}
		}
		if (!transLockZ) {
			g->stroke(0.0f, 0.0f, 1.0f);
			g->line(0.0f, 0.0f, -5000.0f, 0.0f, 0.0f, 5000.0f);
			if (transSnapping) {
				g->pushMatrix();
				g->translate(0.0f, 0.0f, -500.0f);
				int ticNum = (int)(1000.0f / transSnappingDist);
				for (int i = 0; i < ticNum; i++) {
					g->line(0.0f, -2.0f, 0.0f, 0.0f, 2.0f, 0.0f);
					g->line(-2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f);
					g->translate(0.0f, 0.0f, transSnappingDist);
				}
				g->popMatrix();
			}
		}
	}
	g->popMatrix();
}

void ObjectViewer::renderGrid(JMGraphics* g) {
	gridBuffer->beginDraw();
	g->resetMatrix();
	g->background(0.0f, 0.0f, 0.0f, 0.0f);
	g->stroke(gridColor);
	g->strokeWeight(1.0f);

	for (int i = 0; i < (int)(gridBuffer->width() / gridSpacing) / 2; i++) {
		g->line(gridBuffer->width() / 2.0f + i * gridSpacing, 0.0f, gridBuffer->width() / 2.0f + i * gridSpacing, gridBuffer->height());
		g->line(0.0f, gridBuffer->width() / 2.0f + i * gridSpacing, gridBuffer->width(), gridBuffer->width() / 2.0f + i * gridSpacing);

		g->line(gridBuffer->width() / 2.0f - i * gridSpacing, 0.0f, gridBuffer->width() / 2.0f - i * gridSpacing, gridBuffer->height());
		g->line(0.0f, gridBuffer->width() / 2.0f - i * gridSpacing, gridBuffer->width(), gridBuffer->width() / 2.0f - i * gridSpacing);
	}

	g->noFill();
	g->rect(gridBuffer->width() / 2.0f, gridBuffer->height() / 2.0f, 20.0f, 20.0f);
	g->rect(gridBuffer->width() / 2.0f, gridBuffer->height() / 2.0f, 30.0f, 30.0f);

	gridBuffer->endDraw();
}

void ObjectViewer::duplicateSelected() {
	if (handler->selectedObject() == nullptr || handler->objectsFrozen()) { return; }

	handler->addUndoStep();
	SimObject* newObj = handler->selectedObject()->clone();
	newObj->isSelected = false;
	handler->deselectObject(handler->selectedObject());
	handler->addObject(newObj);
	handler->selectObject(newObj);

	startTranslatingSelected();
}

void ObjectViewer::startTranslatingSelected() {
	if (handler->selectedObject() == nullptr || transformingObject != nullptr) { return; }

	handler->addUndoStep();
	transformingObject = handler->selectedObject();
	translatingObj = true;
	oldObjTransform.x = transformingObject->posx();
	oldObjTransform.y = transformingObject->posy();
	oldObjTransform.z = transformingObject->posz();
}
void ObjectViewer::startRotatingSelected() {
	if (handler->selectedObject() == nullptr || transformingObject != nullptr) { return; }

	handler->addUndoStep();
	transformingObject = handler->selectedObject();
	rotatingObject = true;
	oldObjRot = transformingObject->rotq();
}
void ObjectViewer::startScalingSelected() {
	if (handler->selectedObject() == nullptr || transformingObject != nullptr) { return; }

	handler->addUndoStep();
	transformingObject = handler->selectedObject();
	scalingObject = true;
	oldObjTransform.x = transformingObject->sizex();
	oldObjTransform.y = transformingObject->sizey();
	oldObjTransform.z = transformingObject->sizez();
}

void ObjectViewer::translateObj() {
	if (transformingObject == nullptr)
		return;

	//Cancel
	if (rotatingObject || scalingObject || gr->mousePressedRight() || gr->keyDown(GLFW_KEY_ESCAPE) || gr->keyDown(GLFW_KEY_BACKSPACE) || handler->selectedObject() != transformingObject) {
		transformingObject->setPos(oldObjTransform.x, oldObjTransform.y, oldObjTransform.z);
		stopObjTransform();
		return;
	}

	//Confirm
	if (gr->keyDown(GLFW_KEY_ENTER) || gr->mousePressed()) {
		stopObjTransform();
		return;
	}

	//Handle axis locking
	setTransformAxisLock();
	if (transLockX) { transformingObject->setPos(oldObjTransform.x, transformingObject->posy(), transformingObject->posz()); }
	if (transLockY) { transformingObject->setPos(transformingObject->posx(), oldObjTransform.y, transformingObject->posz()); }
	if (transLockZ) { transformingObject->setPos(transformingObject->posx(), transformingObject->posy(), oldObjTransform.z); }

	//Handle typed input if any
	handleTransformTyped();
	if (!transformTyped.empty()) {
		if (transLockY && transLockZ) { transformingObject->setPos(oldObjTransform.x + transformTypedFloat, oldObjTransform.y, oldObjTransform.z); }
		if (transLockX && transLockZ) { transformingObject->setPos(oldObjTransform.x, oldObjTransform.y + transformTypedFloat, oldObjTransform.z); }
		if (transLockX && transLockY) { transformingObject->setPos(oldObjTransform.x, oldObjTransform.y, oldObjTransform.z + transformTypedFloat); }
		return;
	}



	glm::vec3 v = getTransformAxis() * (0.5f / viewScale);
	totalObjTransform += v;
	transformingObject->setPos(transformingObject->posx() + v.x, transformingObject->posy() + v.y, transformingObject->posz() + v.z);

	//Handle snapping
	if (transSnapping) {
		glm::vec3 snapped = glm::vec3(transformingObject->posx(), transformingObject->posy(), transformingObject->posz());
		if (!transLockX) { snapped.x = std::round(std::round(((float)totalObjTransform.x + (float)oldObjTransform.x) / transSnappingDist) * transSnappingDist); }
		if (!transLockY) { snapped.y = std::round(std::round(((float)totalObjTransform.y + (float)oldObjTransform.y) / transSnappingDist) * transSnappingDist); }
		if (!transLockZ) { snapped.z = std::round(std::round(((float)totalObjTransform.z + (float)oldObjTransform.z) / transSnappingDist) * transSnappingDist); }
		transformingObject->setPos((double)snapped.x, (double)snapped.y, (double)snapped.z);
	}
}
void ObjectViewer::rotateObj() {
	if (transformingObject == nullptr)
		return;

	//Cancel
	if (translatingObj || scalingObject || gr->mousePressedRight() || gr->keyDown(GLFW_KEY_ESCAPE) || gr->keyDown(GLFW_KEY_BACKSPACE) || handler->selectedObject() != transformingObject) {
		transformingObject->setRot(oldObjRot);
		stopObjTransform();
		return;
	}

	//Confirm
	if (gr->keyDown(GLFW_KEY_ENTER) || gr->mousePressed()) {
		stopObjTransform();
		return;
	}


	glm::vec3 v = glm::vec3(-localMouseY + localMouseYLast, localMouseX - localMouseXLast, 0.0f) * 0.01f;

	glm::mat4 mouseRot = glm::mat4(1.0f);
	mouseRot = glm::rotate(mouseRot, -viewRotZ, glm::vec3(0.0f, 0.0f, 1.0f));
	mouseRot = glm::rotate(mouseRot, -viewRotY, glm::vec3(0.0f, 1.0f, 0.0f));
	mouseRot = glm::rotate(mouseRot, -viewRotX, glm::vec3(1.0f, 0.0f, 0.0f));

	v = mouseRot * glm::vec4(v, 1.0f);
	totalObjTransform += v;

	//Handle axis locking
	setTransformAxisLock();
	if (transLockX) { totalObjTransform.x = 0.0; }
	if (transLockY) { totalObjTransform.y = 0.0; }
	if (transLockZ) { totalObjTransform.z = 0.0; }

	//Handle snapping
	if (transSnapping) {
		if (!transLockX) { totalObjTransform.x = std::round(totalObjTransform.x / 0.392699) * 0.392699; }
		if (!transLockY) { totalObjTransform.y = std::round(totalObjTransform.y / 0.392699) * 0.392699; }
		if (!transLockZ) { totalObjTransform.z = std::round(totalObjTransform.z / 0.392699) * 0.392699; }
	}

	//Handle typed input if any
	handleTransformTyped();
	if (!transformTyped.empty()) {
		if (transLockY && transLockZ) { totalObjTransform.x = glm::radians(transformTypedFloat); }
		if (transLockX && transLockZ) { totalObjTransform.y = glm::radians(transformTypedFloat); }
		if (transLockX && transLockY) { totalObjTransform.z = glm::radians(transformTypedFloat); }
	}

	glm::dquat globalRot =  glm::angleAxis(totalObjTransform.z, glm::dvec3(0.0, 0.0, 1.0)) *
							glm::angleAxis(totalObjTransform.y, glm::dvec3(0.0, 1.0, 0.0)) *
							glm::angleAxis(totalObjTransform.x, glm::dvec3(1.0, 0.0, 0.0));

	transformingObject->setRot(globalRot * oldObjRot);
}
void ObjectViewer::scaleObj() {
	if (transformingObject == nullptr)
		return;

	//Cancel
	if (rotatingObject || translatingObj || gr->mousePressedRight() || gr->keyDown(GLFW_KEY_ESCAPE) || gr->keyDown(GLFW_KEY_BACKSPACE) || handler->selectedObject() != transformingObject) {
		transformingObject->setSize(oldObjTransform.x, oldObjTransform.y, oldObjTransform.z);
		stopObjTransform();
		return;
	}

	//Confirm
	if (gr->keyDown(GLFW_KEY_ENTER) || gr->mousePressed()) {
		stopObjTransform();
		return;
	}

	//Handle axis locking
	setTransformAxisLock();
	if (transLockX) { transformingObject->setSize(oldObjTransform.x, transformingObject->sizey(), transformingObject->sizez()); }
	if (transLockY) { transformingObject->setSize(transformingObject->sizex(), oldObjTransform.y, transformingObject->sizez()); }
	if (transLockZ) { transformingObject->setSize(transformingObject->sizex(), transformingObject->sizey(), oldObjTransform.z); }

	//Handle typed input if any
	handleTransformTyped();
	if (!transformTyped.empty()) {
		if (transLockY && transLockZ) { transformingObject->setSize(oldObjTransform.x * transformTypedFloat, oldObjTransform.y, oldObjTransform.z); }
		else if (transLockX && transLockZ) { transformingObject->setSize(oldObjTransform.x, oldObjTransform.y * transformTypedFloat, oldObjTransform.z); }
		else if (transLockX && transLockY) { transformingObject->setSize(oldObjTransform.x, oldObjTransform.y, oldObjTransform.z * transformTypedFloat); }
		else { transformingObject->setSize(oldObjTransform.x * transformTypedFloat, oldObjTransform.y * transformTypedFloat, oldObjTransform.z * transformTypedFloat); }

		return;
	}

	glm::vec3 v;
	if (!transLockX && !transLockY && !transLockZ) {
		v = glm::vec3(localMouseX - localMouseXLast) * 0.5f / viewScale;
		transSnapping = false;
	}
	else {
		v = getTransformAxis() * 0.5f / viewScale;
	}
	totalObjTransform += v;

	transformingObject->setSize(transformingObject->sizex() + v.x, transformingObject->sizey() + v.y, transformingObject->sizez() + v.z);

	//Handle snapping
	if (transSnapping) {
		glm::vec3 snapped = glm::vec3(transformingObject->sizex(), transformingObject->sizey(), transformingObject->sizez());
		if (!transLockX) { snapped.x = (float)std::round(std::round((totalObjTransform.x + oldObjTransform.x) / transSnappingDist) * transSnappingDist); }
		if (!transLockY) { snapped.y = (float)std::round(std::round((totalObjTransform.y + oldObjTransform.y) / transSnappingDist) * transSnappingDist); }
		if (!transLockZ) { snapped.z = (float)std::round(std::round((totalObjTransform.z + oldObjTransform.z) / transSnappingDist) * transSnappingDist); }
		transformingObject->setSize(snapped.x, snapped.y, snapped.z);
	}
}

void ObjectViewer::stopObjTransform() {
	transformingObject = nullptr;
	translatingObj = false;
	rotatingObject = false;
	scalingObject = false;
	transLockX = false;
	transLockY = false;
	transLockZ = false;
	transformTyped = "";
	transformTypedFloat = 0.0f;
	totalObjTransform = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 ObjectViewer::getTransformAxis() {
	glm::vec3 mouseVec = glm::vec3(localMouseX - localMouseXLast, localMouseY - localMouseYLast, 0.0f);

	glm::mat4 mouseRot = glm::mat4(1.0f);
	mouseRot = glm::rotate(mouseRot, -viewRotZ, glm::vec3(0.0f, 0.0f, 1.0f));
	mouseRot = glm::rotate(mouseRot, -viewRotY, glm::vec3(0.0f, 1.0f, 0.0f));
	mouseRot = glm::rotate(mouseRot, -viewRotX, glm::vec3(1.0f, 0.0f, 0.0f));

	mouseVec = mouseRot * glm::vec4(mouseVec, 1.0f);

	if (transLockX) { mouseVec.x = 0.0f; }
	if (transLockY) { mouseVec.y = 0.0f; }
	if (transLockZ) { mouseVec.z = 0.0f; }

	return mouseVec;
}

void ObjectViewer::setTransformAxisLock(){
	if (gr->keyDown(GLFW_KEY_X)) {
		transLockX = false;
		transLockY = false;
		transLockZ = false;
		if (gr->keyDown(GLFW_KEY_LEFT_SHIFT)) {
			transLockX = true;
		}
		else {
			transLockY = true;
			transLockZ = true;
		}
	}
	if (gr->keyDown(GLFW_KEY_Y)) {
		transLockX = false;
		transLockY = false;
		transLockZ = false;
		if (gr->keyDown(GLFW_KEY_LEFT_SHIFT)) {
			transLockY = true;
		}
		else {
			transLockX = true;
			transLockZ = true;
		}
	}
	if (gr->keyDown(GLFW_KEY_Z)) {
		transLockX = false;
		transLockY = false;
		transLockZ = false;
		if (gr->keyDown(GLFW_KEY_LEFT_SHIFT)) {
			transLockZ = true;
		}
		else {
			transLockX = true;
			transLockY = true;
		}
	}
}

void ObjectViewer::handleTransformTyped() {

	if (gr->keyDown(GLFW_KEY_LEFT_SHIFT)) { transSnapping = true; }
	else { transSnapping = false; }

	if (!gr->typedCharacters.empty()) {
		for (char c : gr->typedCharacters) {
			if (std::isdigit(c) || c == '.' || c == '-')
				transformTyped.push_back(c);
		}
	}

	if (transformTyped.empty())
		return;

	float typedVal = 0.0f;
	try {
		typedVal = std::stof(transformTyped);
	}
	catch (const std::invalid_argument) {
		typedVal = 0.0f;
	}
	catch (const std::out_of_range) {
		typedVal = 0.0f;
	}

	transformTypedFloat = typedVal;
}