/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//////////////////////////////////////////////////////////////////////////////////////
// ARX_ViewImage.CPP - Image Slideshow and Viewer System
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Image slideshow presentation system for Arx Fatalis
//		Displays sequential images with fade transitions
//		Used for game ending credits, cutscenes, and image galleries
//
// Purpose:
//		- Display sequences of images as slideshows
//		- Fade in/out transitions between images
//		- Timed automatic advancement
//		- Skip on user input
//		- Support BMP and JPG formats
//
// Key Responsibilities:
//		- Load images from directory sequentially
//		- Manage fade transitions (fade in/out)
//		- Handle user input for skipping
//		- Center images on screen
//		- Time-based automatic progression
//		- Cleanup and resource management
//
// ViewImage Class:
//		Constructor(dir, ext):
//			- Scans directory for numbered images
//			- Loads quit0.bmp, quit1.bmp, etc.
//			- Also checks for .jpg versions
//			- Builds image list
//
//		DrawAllImage():
//			- Main slideshow loop
//			- Handles fade in/out
//			- Displays each image
//			- Processes input
//			- Auto-advances after timer
//
//		Destructor:
//			- Frees image filename list
//			- Cleans up resources
//
// Slideshow Flow:
//		State Machine (iAction):
//			0 - Prepare fade in
//			1 - Load next image texture
//			2 - Fade in image
//			3 - Display image (60 second timeout)
//			4 - Fade out image
//			Loop back to 0
//
// Fade System:
//		- Linear fade using color multiplier
//		- fColor ranges from 0.0 (black) to 1.0 (full)
//		- Fade speed: 0.1% per frame time
//		- bSens controls direction (true=in, false=out)
//		- bActiveFade indicates transition in progress
//
// Image Loading:
//		- Searches for "quit<N>.bmp" and "quit<N>.jpg"
//		- Sequential numbering (quit0, quit1, quit2...)
//		- Stops when file not found
//		- Converts filenames to uppercase
//		- Stores in vListImage vector
//
// Rendering:
//		- Centers images on screen
//		- Scales to fit if larger than screen
//		- Uses EERIEDrawBitmap for rendering
//		- Applies color fade via D3DRGB
//		- Full-screen presentation
//
// User Interaction:
//		- Any key press skips to next image
//		- Polls all 256 keyboard keys
//		- Uses DirectInput for input
//		- Triggers fade out on input
//
// Timing:
//		- 60 second display per image
//		- Frame-time based fade speed
//		- Uses ARX_TIME_Get() for timing
//		- Automatic progression after timeout
//
// Use Cases:
//		- Game ending credits/gallery
//		- Cutscene image sequences
//		- Instruction/tutorial slides
//		- Concept art viewer
//		- Development debug display
//
// StartImageDemo():
//		- Convenience function
//		- Loads images from "graph/interface/misc/"
//		- Creates viewer, runs slideshow
//		- Cleans up when done
//
// File Format Support:
//		- BMP (Windows Bitmap) - priority format
//		- JPG (JPEG) - fallback if no BMP
//		- Loaded via MakeTCFromFile()
//		- Converted to TextureContainer
//
// Screen Positioning:
//		- Centers horizontally: (DANAESIZX - width) / 2
//		- Centers vertically: (DANAESIZY - height) / 2
//		- Clips to screen bounds
//		- No scaling (displays at native size)
//
// Performance:
//		- One image loaded at a time
//		- Previous texture deleted before loading next
//		- Minimal memory footprint
//		- No image preloading/caching
//
// Technical Notes:
//		- State machine architecture for flow control
//		- Fade calculations use floating-point color
//		- Input polling on every frame
//		- Blocking presentation (takes control until done)
//		- No background music/audio playback
//
// Limitations:
//		- Fixed 60-second display time
//		- No manual image navigation (forward/back)
//		- Cannot pause slideshow
//		- One fade speed for all images
//		- No configurable transitions
//		- Blocking operation (freezes game)
//
// Dependencies:
//		- Danae.h (application framework)
//		- EERIETexture.h (texture loading)
//		- EERIEDraw.h (2D rendering)
//		- ARX_Time.h (timing functions)
//		- ARX_Menu2.h (menu integration)
//		- HermesMain.h (PAK file system)
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////
#include <d3d.h>
#include "Danae.h"
#include "ARX_ViewImage.h"
#include "ARX_Menu2.h"
#include "arx_time.h"
#include "EERIETexture.h"
#include "EERIEDraw.h"
#include "Hermesmain.h"


//-----------------------------------------------------------------------------
extern LPDIRECT3DDEVICE7 GDevice;
extern long DANAESIZX;
extern long DANAESIZY;
extern CDirectInput * pGetInfoDirectInput;
extern PakManager * pPakManager;

//-----------------------------------------------------------------------------
 

//-----------------------------------------------------------------------------
ViewImage::ViewImage(char * _pcDir, char * _pExt)
{
	vListImage.clear();

	char tTxt[256];
	int iNum = 0;

	while (1)
	{
		sprintf(tTxt, "%squit%d.bmp", _pcDir + strlen(Project.workingdir), iNum);

		if (pPakManager->ExistFile(tTxt))
		{
			char * pCopy = strdup(tTxt);
			pCopy = strupr(pCopy);
			vListImage.push_back(pCopy);
			iNum++;
		}
		else
		{
			sprintf(tTxt, "%squit%d.jpg", _pcDir + strlen(Project.workingdir), iNum);

			if (pPakManager->ExistFile(tTxt))
			{
				char * pCopy = strdup(tTxt);
				pCopy = strupr(pCopy);
				vListImage.push_back(pCopy);
				iNum++;
			}
			else
				break;
		}
	}

	pTexCurr = NULL;
}

//-----------------------------------------------------------------------------
ViewImage::~ViewImage()
{
	int iI = vListImage.size();

	while (iI--)
	{
		free((void *)vListImage[iI]);
	}

	vListImage.clear();
}

//-----------------------------------------------------------------------------
void ViewImage::DrawAllImage()
{
	int iI = vListImage.size();

	if (iI)
	{
		TextureContainer	* pTex	= NULL;
		int					iTime	= 0;
		int					iJ		= 0;
		bool				bEnd	= true;
		float				fColor	= 0.f;
		bool				bSens	= false;
		bool				bActiveFade	= true;
		int					iAction	= 0;

		while (bEnd)
		{
			float iCurrentTime = ARX_TIME_Get();

			switch (iAction)
			{
				case 0:
					bSens		= true;
					bActiveFade	= true;
					iAction++;
					break;
				case 1:
					ARX_WARN("bSens set to false by default.");
					iAction++;
					break;
				case 2:
					ARX_WARN("bSens set to false by default.");

					if (!bActiveFade)
					{

						float fCTime	= iCurrentTime + 60000 ;
						ARX_CHECK_INT(fCTime);

						iTime	= ARX_CLEAN_WARN_CAST_INT(fCTime);


						iAction++;
					}

					break;
				case 3:
				{
					bool bEnd = false;

					if (pGetInfoDirectInput)
					{
						pGetInfoDirectInput->GetInput();

						for (int i = 0 ; i < 256 ; i++)
						{
							if (pGetInfoDirectInput->iOneTouch[i] > 0)
							{
								bEnd = true;
							}
						}
					}

					if (((iTime - iCurrentTime) < 0) ||
					        bEnd)
					{
						bSens		= false;
						bActiveFade	= true;
						iAction++;
					}
					else
					{
						ARX_WARN("bSens set to false by default.");
					}
				}
				break;
				case 4:
					ARX_WARN("bSens set to false by default.");

					if (!bActiveFade)
					{
						iAction		= 0;
					}

					break;
			}

			if (iAction == 1)
			{
				if (pTex)
				{
					delete pTex;
				}

				if (iJ >= iI) break;

				char * pName = vListImage[iJ];
				pTex = MakeTCFromFile(pName, 0);
				iJ++;
			}

			if (!danaeApp.DANAEStartRender()) continue;


			float fDepX = ARX_CLEAN_WARN_CAST_FLOAT(__max(0, ((DANAESIZX - pTex->m_dwWidth) >> 1)));
			float fDepY = ARX_CLEAN_WARN_CAST_FLOAT(__max(0, ((DANAESIZY - pTex->m_dwHeight) >> 1)));




			ARX_CHECK_NOT_NEG(DANAESIZX);
			ARX_CHECK_NOT_NEG(DANAESIZY);
			EERIEDrawBitmap(GDevice,
			                fDepX,
			                fDepY,
			                ARX_CLEAN_WARN_CAST_FLOAT(__min(pTex->m_dwWidth, ARX_CAST_ULONG(DANAESIZX))),
			                ARX_CLEAN_WARN_CAST_FLOAT(__min(pTex->m_dwHeight, ARX_CAST_ULONG(DANAESIZY))),
			                0.f,
			                pTex,
			                D3DRGB(fColor, fColor, fColor));



			danaeApp.DANAEEndRender();
			danaeApp.m_pFramework->ShowFrame();

			if (bActiveFade)
			{

				float fTGet = ARX_TIME_Get() - iCurrentTime ;
				ARX_CHECK_INT(fTGet);

				int iFrameTime	= ARX_CLEAN_WARN_CAST_INT(fTGet);


				float fIncFade = ((float)iFrameTime) * (.1f / 100.f);

				if (bSens)
				{
					fColor += fIncFade;

					if (fColor > 1.f)
					{
						fColor		= 1.f;
						bActiveFade	= false;
					}
				}
				else
				{
					fColor -= fIncFade;

					if (fColor < 0.f)
					{
						fColor		= 0.f;
						bActiveFade	= false;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void StartImageDemo()
{
	char tTxt[256];
	sprintf(tTxt, Project.workingdir);
	strcat(tTxt, "graph\\interface\\misc\\");
	ViewImage * pViewImage = new ViewImage(tTxt, "*.bmp");

	if (!pViewImage) return;

	danaeApp.DANAEEndRender();
	pViewImage->DrawAllImage();

	delete pViewImage;
}
