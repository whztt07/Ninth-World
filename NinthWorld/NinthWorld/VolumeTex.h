/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2013 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/
/**	Generate 3D julia sets and render them as volume texture
This demonstrates
- User generated textures
- Procedural volume textures (Julia makes nice dust clouds)
- Custom renderables
@author W.J. van der Laan
*/

#include "SdkSample.h"
#include "SamplePlugin.h"
#include <OgreTexture.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreTextureManager.h>
#include <OgreLogManager.h>
#include <sstream>

#include "VolumeRenderable.h"
#include "ThingRenderable.h"
#include "Julia.h"

using namespace Ogre;
using namespace OgreBites;

class Sample_VolumeTex : public SdkSample
{
protected:
	TexturePtr ptex;
	SimpleRenderable *vrend;
	SimpleRenderable *trend;
	Overlay* overlay;
	float xtime;
	SceneNode *snode, *fnode;
	AnimationState* mOgreAnimState;

public:

	Sample_VolumeTex() :vrend(0), trend(0), overlay(NULL), xtime(0.0f), mOgreAnimState(0)
	{
		mInfo["Title"] = "Volume Textures";
		mInfo["Description"] = "Demonstrates the use of volume textures.";
		mInfo["Thumbnail"] = "thumb_voltex.png";
		mInfo["Category"] = "Unsorted";
	}

	void testCapabilities(const RenderSystemCapabilities* caps)
	{
		if (!caps->hasCapability(RSC_TEXTURE_3D))
		{
			OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "Your card does not support 3D textures, so cannot "
				"run this demo. Sorry!", "Sample_VolumeTex::testCapabilities");
		}
	}

public:

	float global_real, global_imag, global_theta;

	void setupView(void)
	{
		SdkSample::setupView();

		// Create the camera
		mCamera->setPosition(Vector3(220, -2, 176));
		mCamera->lookAt(Vector3(0, 0, 0));
		mCamera->setNearClipDistance(5);
	}

	void setupContent(void)
	{
		// Create dynamic texture
		ptex = TextureManager::getSingleton().createManual(
			"DynaTex", "General", TEX_TYPE_3D, 64, 64, 64, 0, PF_A8R8G8B8);


		//mRoot->getRenderSystem()->clearFrameBuffer(FBT_COLOUR, ColourValue(255,255,255,0));


		// Create volume renderable
		snode = mSceneMgr->getRootSceneNode()->createChildSceneNode(Vector3(0, 0, 0));

		vrend = new VolumeRenderable(32, 750.0f, "DynaTex");
		snode->attachObject(vrend);

		trend = new ThingRenderable(90.0f, 32, 7.5f);
		trend->setMaterial("Examples/VTDarkStuff");
		snode->attachObject(trend);


		//mFountainNode->attachObject(pSys2);

		//Setup defaults
		global_real = 0.4f;
		global_imag = 0.6f;
		global_theta = 0.0f;

		// show GUI
		createControls();

		setDragLook(true);

		generate();
	}

	bool frameRenderingQueued(const FrameEvent& evt)
	{
		xtime += evt.timeSinceLastFrame;
		xtime = fmod(xtime, 10.0f);
		//snode->roll(Degree(evt.timeSinceLastFrame * 20.0f));
		//fnode->roll(Degree(evt.timeSinceLastFrame * 20.0f));
		static_cast<ThingRenderable*>(trend)->addTime(evt.timeSinceLastFrame * 0.05f);
		return SdkSample::frameRenderingQueued(evt);
	}

	void cleanupContent(void)
	{
		TextureManager::getSingleton().remove("DynaTex");
		delete vrend;
		delete trend;
	}

	void createControls()
	{
		mTrayMgr->createLabel(TL_TOPLEFT, "JuliaParamLabel", "Julia Parameters", 200);
		mTrayMgr->createThickSlider(TL_TOPLEFT, "RealSlider", "Real", 200, 80, -1, 1, 50)->setValue(global_real, false);
		mTrayMgr->createThickSlider(TL_TOPLEFT, "ImagSlider", "Imag", 200, 80, -1, 1, 50)->setValue(global_imag, false);
		mTrayMgr->createThickSlider(TL_TOPLEFT, "ThetaSlider", "Theta", 200, 80, -1, 1, 50)->setValue(global_theta, false);
		mTrayMgr->showCursor();
	}

	void sliderMoved(Slider* slider)
	{
		if (slider->getName() == "RealSlider")
		{
			global_real = slider->getValue();
		}
		else if (slider->getName() == "ImagSlider")
		{
			global_imag = slider->getValue();
		}
		else if (slider->getName() == "ThetaSlider")
		{
			global_theta = slider->getValue();
		}
		generate();
	}

	void generate()
	{
		/* Evaluate julia fractal for each point */
		MyQuaternion julia(global_real, global_imag, global_theta);
		const float scale = 2.5;
		const float vcut = 29.0f;
		const float vscale = 1.0f / vcut;

		HardwarePixelBufferSharedPtr buffer = ptex->getBuffer(0, 0);
		Ogre::StringStream d;
		d << "HardwarePixelBuffer " << buffer->getWidth() << " " << buffer->getHeight() << " " << buffer->getDepth();
		LogManager::getSingleton().logMessage(d.str());

		buffer->lock(HardwareBuffer::HBL_NORMAL);
		const PixelBox &pb = buffer->getCurrentLock();
		d.str("");
		d << "PixelBox " << pb.getWidth() << " " << pb.getHeight() << " " << pb.getDepth() << " " << pb.rowPitch << " " << pb.slicePitch << " " << pb.data << " " << PixelUtil::getFormatName(pb.format);
		LogManager::getSingleton().logMessage(d.str());

		Ogre::uint32 *pbptr = static_cast<Ogre::uint32*>(pb.data);
		for (size_t z = pb.front; z<pb.back; z++)
		{
			for (size_t y = pb.top; y<pb.bottom; y++)
			{
				for (size_t x = pb.left; x<pb.right; x++)
				{
					if (z == pb.front || z == (pb.back - 1) || y == pb.top || y == (pb.bottom - 1) ||
						x == pb.left || x == (pb.right - 1))
					{
						// On border, must be zero
						pbptr[x] = 0;
					}
					else
					{
						float val = julia.eval(((float)x / pb.getWidth() - 0.5f) * scale,
							((float)y / pb.getHeight() - 0.5f) * scale,
							((float)z / pb.getDepth() - 0.5f) * scale);
						if (val > vcut)
							val = vcut;

						PixelUtil::packColour((float)x / pb.getWidth(), (float)y / pb.getHeight(), (float)z / pb.getDepth(), (1.0f - (val*vscale))*0.7f, PF_A8R8G8B8, &pbptr[x]);

					}
				}
				pbptr += pb.rowPitch;
			}
			pbptr += pb.getSliceSkip();
		}
		buffer->unlock();
	}
};