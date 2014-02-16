/** \file
 * \brief Implementation of ContainerHead's graphics.
 */
#include "../ContainerHead.h"
#include "../vastspace.h"
#include "draw/material.h"
#include "glstack.h"
#include "draw/WarDraw.h"
#include "draw/OpenGLState.h"
#include "draw/mqoadapt.h"




const double ContainerHead::sufscale = .0002;
Model *ContainerHead::model = NULL;
Model *ContainerHead::containerModels[Num_ContainerType] = {NULL};

void ContainerHead::draw(wardraw_t *wd){
	ContainerHead *const p = this;
	if(!w)
		return;

	/* cull object */
	if(cull(wd))
		return;
//	wd->lightdraws++;

	draw_healthbar(this, wd, health / getMaxHealth(), .1, 0, capacitor / frigate_mn.capacity);

	// The pointed type is not meaningful; it just indicates status of initialization by its presense.
	static OpenGLState::weak_ptr<bool> initialized = false;
	if(!initialized){

		// Register alpha test texture
		TexParam stp;
		stp.flags = STP_ALPHA | STP_ALPHA_TEST | STP_TRANSPARENTCOLOR;
		stp.transparentColor = 0;
		AddMaterial(modPath() + "models/containerrail.bmp", modPath() + "models/containerrail.bmp", &stp, NULL, NULL);

		model = LoadMQOModel(modPath() + "models/containerhead.mqo");

		static const char *modelNames[Num_ContainerType] = {"models/gascontainer.mqo", "models/hexcontainer.mqo"};
		for(int i = 0; i < Num_ContainerType; i++){
			containerModels[i] = LoadMQOModel(modPath() + modelNames[i]);
		}

		initialized.create(*openGLState);
	}
	static int drawcount = 0;
	drawcount++;
	{
		double scale = sufscale;
		Mat4d mat;

		glPushMatrix();
		transform(mat);
		glMultMatrixd(mat);

		GLattrib gla(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

		glPushMatrix();
		glScaled(-scale, scale, -scale);

		glTranslated(0, 0, 150 * ncontainers);

		// A silly technique to hide parts of a Model.
		// We have head and tail meshes in a Model and want to draw them in different places.
		// We abuse MotionPose mechanism to hide head when drawing tail, and vise versa.
		// Probably a better way is to separate head and tail models into distinct .mqo files,
		// but doing so makes the model less easy to maintain.
		MotionPose pose;
		MotionNode *headNode = pose.addNode("containerhead");
		MotionNode *tailNode = pose.addNode("containertail");
		if(model){
			if(tailNode)
				tailNode->visible = 0;
			DrawMQOPose(model, &pose);
		}

		glTranslated(0, 0, -150);

		// Draw each container types
		for(int i = 0; i < ncontainers; i++){
			if(containerModels[containers[i]])
				DrawMQOPose(containerModels[containers[i]], NULL);
			glTranslated(0, 0, -300);
		}

		glTranslated(0, 0, 150);

		if(model){
			if(headNode)
				headNode->visible = 0;
			if(tailNode)
				tailNode->visible = 1;
			DrawMQOPose(model, &pose);
		}

		glPopMatrix();

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void ContainerHead::drawtra(wardraw_t *wd){
	st::drawtra(wd);
	ContainerHead *p = this;
	Mat4d mat;

/*	if(p->dock && p->undocktime == 0)
		return;*/

	transform(mat);

//	drawCapitalBlast(wd, Vec3d(0,-0.003,.06), .01);

//	drawShield(wd);
}

void ContainerHead::drawOverlay(wardraw_t *){
	glScaled(10, 10, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2d(-.10,  .00);
	glVertex2d(-.05, -.05 * sqrt(3.));
	glVertex2d( .05, -.05 * sqrt(3.));
	glVertex2d( .10,  .00);
	glVertex2d( .05,  .05 * sqrt(3.));
	glVertex2d(-.05,  .05 * sqrt(3.));
	glEnd();
}


Entity::Props ContainerHead::props()const{
	Props ret = st::props();
	ret.push_back(gltestp::dstring("I am ContainerHead!"));
	return ret;
}
