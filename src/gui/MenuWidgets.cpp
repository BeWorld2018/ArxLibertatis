/*
 * Copyright 2011-2013 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Based on:
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

#include "gui/MenuWidgets.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iosfwd>
#include <limits>
#include <algorithm>
#include <sstream>

#include <boost/foreach.hpp>

#include "core/Application.h"
#include "core/ArxGame.h"
#include "core/Config.h"
#include "core/Core.h"
#include "core/GameTime.h"
#include "core/Localisation.h"
#include "core/SaveGame.h"
#include "core/Version.h"

#include "gui/LoadLevelScreen.h"
#include "gui/MainMenu.h"
#include "gui/Menu.h"
#include "gui/MenuPublic.h"
#include "gui/Text.h"
#include "gui/Interface.h"
#include "gui/Credits.h"
#include "gui/TextManager.h"

#include "graphics/Draw.h"
#include "graphics/DrawLine.h"
#include "graphics/Math.h"
#include "graphics/Renderer.h"
#include "graphics/data/TextureContainer.h"
#include "graphics/font/Font.h"
#include "graphics/texture/TextureStage.h"

#include "input/Input.h"

#include "scene/ChangeLevel.h"
#include "scene/GameSound.h"
#include "scene/LoadLevel.h"

#include "window/RenderWindow.h"

using std::wistringstream;
using std::min;
using std::max;
using std::string;
using std::vector;

static int newWidth;
static int newHeight;
static bool newFullscreen;



// Imported global variables and functions
extern ARX_MENU_DATA ARXmenu;
extern TextureContainer * scursor[];

extern Rect g_size;

extern long REFUSE_GAME_RETURN;

float INTERFACE_RATIO(float a);
bool bNoMenu=false;

void ARXMenu_Private_Options_Video_SetResolution(bool fullscreen, int _iWidth, int _iHeight);

static MenuCursor * pMenuCursor = NULL;

extern CWindowMenu * pWindowMenu;
CMenuState *mainMenu;

static CMenuElement * pMenuElementResume = NULL;
extern CMenuElement * pMenuElementApply;
extern CMenuElementText * pLoadConfirm;
extern CMenuElementText * pDeleteConfirm;
extern CMenuElementText * pDeleteButton;
extern CMenuCheckButton * fullscreenCheckbox;
extern CMenuSliderText * pMenuSliderResol;

float ARXTimeMenu;
float ARXOldTimeMenu;
float ARXDiffTimeMenu;

bool bFade=false;
bool bFadeInOut=false;
int iFadeAction=-1;
float fFadeInOut=0.f;

void ARX_MENU_Clicked_NEWQUEST();

TextureContainer *pTextureLoad=NULL;
static TextureContainer *pTextureLoadRender=NULL;

void ARX_QuickSave() {
	
	if(REFUSE_GAME_RETURN) {
		return;
	}
	
	ARX_SOUND_MixerPause(ARX_SOUND_MixerGame);
	
	savegames.quicksave(savegame_thumbnail);
	
	ARX_SOUND_MixerResume(ARX_SOUND_MixerGame);
}

bool ARX_LoadGame(const SaveGame & save) {
	
	LoadLevelScreen();
	progressBarSetTotal(238);
	progressBarReset();
	progressBarAdvance();
	LoadLevelScreen(save.level);
	
	DanaeClearLevel();
	
	long ret = ARX_CHANGELEVEL_Load(save.savefile);
	
	REFUSE_GAME_RETURN = 0;

	return ret != -1;
}

bool ARX_QuickLoad() {
	bool loaded;

	SaveGameList::iterator save = savegames.quickload();
	if(save == savegames.end()) {
		// No saves found!
		return false;
	}

	ARX_SOUND_MixerPause(ARX_SOUND_MixerGame);
	loaded = ARX_LoadGame(*save);
	ARX_SOUND_MixerResume(ARX_SOUND_MixerGame);
	
	return loaded;
}

bool ARX_SlotLoad(int slotIndex) {
	if(slotIndex >= (int)savegames.size()) {
		// Invalid slot!
		return false;
	}

	ARX_SOUND_MixerPause(ARX_SOUND_MixerMenu);
	return ARX_LoadGame(savegames[slotIndex]);
}

bool MENU_NoActiveWindow() {
	if(!pWindowMenu
	   || (pWindowMenu && pWindowMenu->eCurrentMenuState == MAIN)
	) {
		return true;
	}

	return false;
}

void FontRenderText(Font* _pFont, Vec3f pos, const std::string& _pText, Color _c) {
	if(pTextManage) {
		long x = checked_range_cast<long>(pos.x);
		long y = checked_range_cast<long>(pos.y);
		pTextManage->AddText(_pFont, _pText, x, y, _c);
	}
}

void Check_Apply() {
	if(pMenuElementApply) {
		if(config.video.resolution.x != newWidth
		   || config.video.resolution.y != newHeight
		   || config.video.fullscreen != newFullscreen
		) {
			pMenuElementApply->SetCheckOn();
			((CMenuElementText*)pMenuElementApply)->lColor=((CMenuElementText*)pMenuElementApply)->lOldColor;
		} else {
			if(((CMenuElementText*)pMenuElementApply)->lColor != Color(127,127,127)) {
				pMenuElementApply->SetCheckOff();
				((CMenuElementText*)pMenuElementApply)->lOldColor=((CMenuElementText*)pMenuElementApply)->lColor;
				((CMenuElementText*)pMenuElementApply)->lColor=Color(127,127,127);
			}
		}
	}
}

static void FadeInOut(float _fVal) {

	TexturedVertex d3dvertex[4];

	u32 iColor = Color::gray(_fVal).toBGR();
	d3dvertex[0].p.x=0;
	d3dvertex[0].p.y=0;
	d3dvertex[0].p.z=0.f;
	d3dvertex[0].rhw=0.999999f;
	d3dvertex[0].color=iColor;

	d3dvertex[1].p.x=static_cast<float>(g_size.width());
	d3dvertex[1].p.y=0;
	d3dvertex[1].p.z=0.f;
	d3dvertex[1].rhw=0.999999f;
	d3dvertex[1].color=iColor;

	d3dvertex[2].p.x=0;
	d3dvertex[2].p.y=static_cast<float>(g_size.height());
	d3dvertex[2].p.z=0.f;
	d3dvertex[2].rhw=0.999999f;
	d3dvertex[2].color=iColor;

	d3dvertex[3].p.x=static_cast<float>(g_size.width());
	d3dvertex[3].p.y=static_cast<float>(g_size.height());
	d3dvertex[3].p.z=0.f;
	d3dvertex[3].rhw=0.999999f;
	d3dvertex[3].color=iColor;

	GRenderer->ResetTexture(0);
	GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);

	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetRenderState(Renderer::DepthWrite, false);
	GRenderer->SetRenderState(Renderer::DepthTest, false);
	GRenderer->SetCulling(Renderer::CullNone);

	EERIEDRAWPRIM(Renderer::TriangleStrip, d3dvertex, 4, true);

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	GRenderer->SetRenderState(Renderer::DepthWrite, true);
	GRenderer->SetRenderState(Renderer::DepthTest, true);
	GRenderer->SetCulling(Renderer::CullCCW);
}

bool ProcessFadeInOut(bool _bFadeIn, float _fspeed) {

	FadeInOut(fFadeInOut);

	if(!bFade)
		return true;

	if(_bFadeIn) {
		fFadeInOut += _fspeed * ARXDiffTimeMenu * (1.f/100);

		if(fFadeInOut > 1.f) {
			fFadeInOut = 1.f;
			bFade = false;
		}
	} else {
		fFadeInOut -= _fspeed * ARXDiffTimeMenu * (1.f/100);

		if(fFadeInOut < 0.f) {
			fFadeInOut = 0.f;
			bFade = false;
		}
	}

	return false;
}

bool Menu2_Render() {
	
	ARXOldTimeMenu = ARXTimeMenu;
	ARXTimeMenu = arxtime.get_updated(false);
	ARXDiffTimeMenu = ARXTimeMenu - ARXOldTimeMenu;
	
	// this means ArxTimeMenu is reset
	if(ARXDiffTimeMenu < 0) {
		ARXDiffTimeMenu = 0;
	}
	
	if(pMenuCursor == NULL) {
		pMenuCursor = new MenuCursor();
	}
	pMenuCursor->update(ARXDiffTimeMenu);
	
	GRenderer->GetTextureStage(0)->setMinFilter(TextureStage::FilterLinear);
	GRenderer->GetTextureStage(0)->setMagFilter(TextureStage::FilterLinear);
	
	if(AMCM_NEWQUEST == ARXmenu.currentmode || AMCM_CREDITS == ARXmenu.currentmode) {
		
		delete pWindowMenu, pWindowMenu = NULL;
		delete mainMenu, mainMenu = NULL;
		
		if(ARXmenu.currentmode == AMCM_CREDITS){
			Credits::render();
			return true;
		}
		
		return false;
	}
	
	if(pTextManage) {
		pTextManage->Clear();
	}

	GRenderer->GetTextureStage(0)->setWrapMode(TextureStage::WrapClamp);

	GRenderer->SetRenderState(Renderer::Fog, false);
	GRenderer->SetRenderState(Renderer::DepthWrite, false);
	GRenderer->SetRenderState(Renderer::DepthTest, false);
	GRenderer->SetCulling(Renderer::CullNone);

	MENUSTATE eOldMenuState=NOP;
	MENUSTATE eM;

	if(!mainMenu) {
		eM = NOP;
	} else {
		eM = mainMenu->eOldMenuWindowState;
	}
	
	if(!mainMenu || (mainMenu && mainMenu->bReInitAll)) {
		
		if(mainMenu && mainMenu->bReInitAll) {
			eOldMenuState = mainMenu->eOldMenuState;
			delete pWindowMenu, pWindowMenu = NULL;
			delete mainMenu, mainMenu = NULL;
		}
		
		mainMenu = new CMenuState();
		mainMenu->eOldMenuWindowState=eM;
		
		mainMenu->createChildElements();
	}

	bool bScroll=true;
	{
	if(pMenuElementResume) {
		
		if(ARXMenu_CanResumeGame()) {
			pMenuElementResume->SetCheckOn();
			((CMenuElementText*)pMenuElementResume)->lColor = Color(232, 204, 142);
		} else {
			pMenuElementResume->SetCheckOff();
			((CMenuElementText*)pMenuElementResume)->lColor=Color(127,127,127);
		}
	}
	
	MENUSTATE eMenuState = mainMenu->Update();
	
	if(eOldMenuState != NOP) {
		eMenuState=eOldMenuState;
		bScroll=false;
	}
	
	if(eMenuState == RESUME_GAME) {
		pTextManage->Clear();
		ARXmenu.currentmode = AMCM_OFF;
		mainMenu->pZoneClick = NULL;
		
		delete pWindowMenu, pWindowMenu = NULL;
		delete mainMenu, mainMenu = NULL;
		
		GRenderer->SetRenderState(Renderer::AlphaBlending, false);
		GRenderer->GetTextureStage(0)->setWrapMode(TextureStage::WrapRepeat);
		GRenderer->SetRenderState(Renderer::DepthWrite, true);
		GRenderer->SetRenderState(Renderer::DepthTest, true);
		
		return true;
	} else if(eMenuState != NOP) {
		MainMenuLeftCreate(eMenuState);
	
	}
	}
	mainMenu->Render();

	if(pWindowMenu) {
		if(!bScroll) {
			pWindowMenu->fAngle=90.f;
			pWindowMenu->eCurrentMenuState=mainMenu->eOldMenuWindowState;
		}

		pWindowMenu->Update(ARXDiffTimeMenu);

		if(pWindowMenu) {
			MENUSTATE eMS=pWindowMenu->Render();

			if(eMS != NOP) {
				mainMenu->eOldMenuWindowState=eMS;
			}
		}

		Check_Apply();
	}

	bNoMenu=false;

	// If the menu needs to be reinitialized, then the text in the TextManager is probably using bad fonts that were deleted already
	// Skip one update in this case
	if(pTextManage && !mainMenu->bReInitAll) {
		pTextManage->Update(ARXDiffTimeMenu);
		pTextManage->Render();
	}

	GRenderer->GetTextureStage(0)->setWrapMode(TextureStage::WrapClamp);

	GRenderer->SetRenderState(Renderer::Fog, false);
	GRenderer->SetRenderState(Renderer::DepthWrite, false);
	GRenderer->SetRenderState(Renderer::DepthTest, false);
	GRenderer->SetCulling(Renderer::CullNone);
	pMenuCursor->DrawCursor();

	if(pTextureLoadRender) {
		GRenderer->SetRenderState(Renderer::DepthTest, false);
		
		Vec2f size = Vec2f(pTextureLoad->size());
		
		Vec2f offset = Vec2f(0, 0);
		
		if(DANAEMouse.y + size.y > g_size.height()) {
			offset.y -= size.y;
		}
		
		Vec2f pos = Vec2f(DANAEMouse) + offset;
		
		Rectf rect = Rectf(pos, size.x, size.y);
		
		EERIEDrawBitmap(rect, 0.001f, pTextureLoad, Color::white);
		drawLineRectangle(rect, 0.01f, Color::white);

		pTextureLoadRender=NULL;
	}

	if(ProcessFadeInOut(bFadeInOut,0.1f)) {
		switch(iFadeAction) {
		case AMCM_CREDITS:
			ARX_MENU_Clicked_CREDITS();
			iFadeAction=-1;
			bFadeInOut=false;
			bFade=true;
			break;
		case AMCM_NEWQUEST:
			ARX_MENU_Clicked_NEWQUEST();
			iFadeAction=-1;
			bFadeInOut=false;
			bFade=true;
			cinematicBorder.reset2();
			break;
		case AMCM_OFF:
			ARX_MENU_Clicked_QUIT_GAME();
			iFadeAction=-1;
			bFadeInOut=false;
			bFade=true;
			break;
		}
	}

	GRenderer->GetTextureStage(0)->setMinFilter(TextureStage::FilterLinear);
	GRenderer->GetTextureStage(0)->setMagFilter(TextureStage::FilterLinear);
	GRenderer->GetTextureStage(0)->setWrapMode(TextureStage::WrapRepeat);

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	GRenderer->SetRenderState(Renderer::DepthWrite, true);
	GRenderer->SetRenderState(Renderer::DepthTest, true);
	GRenderer->SetCulling(Renderer::CullCCW);

	return true;
}


CMenuElement::CMenuElement(MENUSTATE _ms)
	: CMenuZone()
	, enabled(true)
{
	ePlace=NOCENTER;
	eState=TNOP;
	eMenuState=_ms;
	iShortCut=-1;
}

CMenuElement::~CMenuElement() {

	if(this == pMenuElementApply) {
		pMenuElementApply = NULL;
	}

	if(this == pMenuElementResume) {
		pMenuElementResume = NULL;
	}

	if(this == pLoadConfirm) {
		pLoadConfirm = NULL;
	}

	if(this == pDeleteConfirm) {
		pDeleteConfirm = NULL;
	}

	if(this == pDeleteButton) {
		pDeleteButton = NULL;
	}

	if(this == pMenuSliderResol) {
		pMenuSliderResol = NULL;
	}
	
	if(this == fullscreenCheckbox) {
		fullscreenCheckbox = NULL;
	}
}

CMenuElement* CMenuElement::OnShortCut() {

	if(iShortCut == -1)
		return NULL;

	if(GInput->isKeyPressedNowUnPressed(iShortCut)) {
		return this;
	}

	return NULL;
}

CMenuElementText::CMenuElementText(int _iID, Font* _pFont, const std::string& _pText, Vec2i pos, MENUSTATE _eMs) : CMenuElement(_eMs)
{
	iID = _iID;

	pFont = _pFont;

	if(!_pText.compare("---")) {
		bTestYDouble=true;
	}
	
	rZone.left = checked_range_cast<Rect::Num>(pos.x);
	rZone.top = checked_range_cast<Rect::Num>(pos.y);
	
	SetText(_pText);
	
	lColor = Color(232, 204, 142);
	lColorHighlight=lOldColor=Color(255, 255, 255);

	pRef=this;

	bSelected = false;
}

CMenuElementText::~CMenuElementText()
{
}

void CMenuElementText::SetText(const std::string & _pText)
{
	lpszText = _pText;

	Vec2i textSize = pFont->getTextSize(_pText);

	rZone.right  = textSize.x + rZone.left;
	rZone.bottom = textSize.y + rZone.top;
}

void CMenuElementText::Update(int _iDTime) {
	(void)_iDTime;
}

bool CMenuElementText::OnMouseDoubleClick() {

	switch(iID) {
	case BUTTON_MENUEDITQUEST_LOAD:
		OnMouseClick();

		if(pWindowMenu) {
			for(size_t i = 0; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
				CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];

				if(p->eMenuState == EDIT_QUEST_LOAD) {
					for(size_t j = 0; j < p->MenuAllZone.vMenuZone.size(); j++) {
						CMenuElement *pMenuElement = (CMenuElement*) ( (CMenuElement*)p->MenuAllZone.vMenuZone[j] )->GetZoneWithID(BUTTON_MENUEDITQUEST_LOAD_CONFIRM);

						if(pMenuElement) {
							pMenuElement->OnMouseClick();
						}
					}
				}
			}
		}

		return true;
	}

	return false;
}

// true: block les zones de checks
bool CMenuElementText::OnMouseClick() {
	
	if(!enabled) {
		return false;
	}
	
	switch(eState) {
		case EDIT:
			eState=EDIT_TIME;
			return true;
		case GETTOUCH:
			eState=GETTOUCH_TIME;
			lOldColor=lColorHighlight;
			return true;
		default: break;
	}

	if(iID != BUTTON_MENUMAIN_RESUMEGAME) {
		ARX_SOUND_PlayMenu(SND_MENU_CLICK);
	}

	switch(iID) {
		case -1: {
				return false;
			}
			break;
		// MENUMAIN
		case BUTTON_MENUMAIN_RESUMEGAME: {
				pTextManage->Clear();
				ARXMenu_ResumeGame();
				ARX_SOUND_PlayMenu(SND_MENU_CLICK);
			}
			break;
		case BUTTON_MENUMAIN_NEWQUEST: {
				
				if(!ARXMenu_CanResumeGame()) {
					ARXMenu_NewQuest();
				}
			}
			break;
		case BUTTON_MENUMAIN_LOADQUEST: {
			}break;
		case BUTTON_MENUMAIN_SAVEQUEST: {
			}break;
		case BUTTON_MENUMAIN_MULTIPLAYER: {
			}break;
		case BUTTON_MENUMAIN_OPTIONS: {
			}break;
		case BUTTON_MENUMAIN_CREDITS: {
				ARXMenu_Credits();
			}
			break;
		case BUTTON_MENUMAIN_QUIT: {
				ARXMenu_Quit();
			}
			break;
		case BUTTON_MENUNEWQUEST_CONFIRM: {
				ARXMenu_NewQuest();
			}
			break;
			// MENULOADQUEST
			case BUTTON_MENUOPTIONSVIDEO_INIT: {
				newWidth = config.video.resolution.x;
				newHeight = config.video.resolution.y;
				newFullscreen = config.video.fullscreen;
				break;
			}
		case BUTTON_MENUEDITQUEST_LOAD_INIT: {
			if(pWindowMenu)
			for(size_t i = 0; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
				CWindowMenuConsole * p = pWindowMenu->vWindowConsoleElement[i];
				
				if(p->eMenuState == EDIT_QUEST_LOAD) {						
					p->lData = lData;
					
					for(size_t j = 0; j < p->MenuAllZone.vMenuZone.size(); j++) {
						CMenuZone *cz = p->MenuAllZone.vMenuZone[j];
						
						if(cz->iID == BUTTON_MENUEDITQUEST_LOAD) {
							((CMenuElementText *)cz)->bSelected = false;
						}
					}
				}
			}
		}
			break;
		case BUTTON_MENUEDITQUEST_LOAD: {
			if(pWindowMenu) {
				pLoadConfirm->SetCheckOn();
				pLoadConfirm->lColor = pLoadConfirm->lOldColor;
				pDeleteConfirm->SetCheckOn();
				pDeleteConfirm->lColor = pDeleteConfirm->lOldColor;
				
				for(size_t i = 0; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
					CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];
					
					if(p->eMenuState == EDIT_QUEST_LOAD) {
						p->lData = lData;
						
						for(size_t j = 0; j < p->MenuAllZone.vMenuZone.size(); j++) {
							CMenuZone *cz = p->MenuAllZone.vMenuZone[j];
							
							if(cz->iID == BUTTON_MENUEDITQUEST_LOAD) {
								((CMenuElementText *)cz)->bSelected = false;
							}
						}
						bSelected = true;
					}
				}
			}
		}
		break;
		case BUTTON_MENUEDITQUEST_LOAD_CONFIRM: {
			if(pWindowMenu) {
				for(size_t i = 0; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
					CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];
		
					if(p->eMenuState == EDIT_QUEST_LOAD) {
						
						lData = p->lData;
						if(lData != -1) {
							eMenuState = MAIN;
							GRenderer->Clear(Renderer::DepthBuffer);
							ARXMenu_LoadQuest(lData);
							bNoMenu=true;
							if(pTextManage) {
								pTextManage->Clear();
							}
							break;
						}
					}
				}
				
				pLoadConfirm->SetCheckOff();
				pLoadConfirm->lColor = Color::grayb(127);
				pDeleteConfirm->SetCheckOff();
				pDeleteConfirm->lColor = Color::grayb(127);
			}
		}
		break;
		case BUTTON_MENUEDITQUEST_LOAD_CONFIRM_BACK:
			pLoadConfirm->SetCheckOff();
			pLoadConfirm->lColor = Color::grayb(127);
			pDeleteConfirm->SetCheckOff();
			pDeleteConfirm->lColor = Color::grayb(127);
			break;
		// MENUSAVEQUEST
		case BUTTON_MENUEDITQUEST_SAVE: {
			if(pWindowMenu)
			for(size_t i = 0; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
				CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];
				
				if(p->eMenuState == EDIT_QUEST_SAVE_CONFIRM) {
					p->lData = lData;
					CMenuElementText * me = (CMenuElementText *) p->MenuAllZone.vMenuZone[1];
					
					if(me) {
						eMenuState = MAIN;
						ARXMenu_SaveQuest(me->lpszText, me->lData);
						break;
					}
				}
			}
		}
		break;
		
		// Delete save from the load menu
		case BUTTON_MENUEDITQUEST_DELETE_CONFIRM: {
			if(pWindowMenu) {
				for(size_t i = 0 ; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
					CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];
					if(p->eMenuState == EDIT_QUEST_LOAD) {
						lData = p->lData;
						if(lData != -1) {
							eMenuState = EDIT_QUEST_LOAD;
							mainMenu->bReInitAll = true;
							savegames.remove(lData);
							break;
						}
					}
				}
			}
			pLoadConfirm->SetCheckOff();
			pLoadConfirm->lColor = Color::grayb(127);
			pDeleteConfirm->SetCheckOff();
			pDeleteConfirm->lColor = Color::grayb(127);
			break;
		}
			
		// Delete save from the save menu
		case BUTTON_MENUEDITQUEST_DELETE: {
			if(pWindowMenu) {
				for(size_t i = 0 ; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
					CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];
					if(p->eMenuState == EDIT_QUEST_SAVE_CONFIRM) {
						p->lData = lData;
						CMenuElementText * me = (CMenuElementText *) p->MenuAllZone.vMenuZone[1];
						if(me) {
							eMenuState = EDIT_QUEST_SAVE;
							mainMenu->bReInitAll = true;
							savegames.remove(me->lData);
							break;
						}
					}
				}
			}
			break;
		}
			
		case BUTTON_MENUOPTIONSVIDEO_APPLY: {
			if(newWidth != config.video.resolution.x
			   || newHeight!=config.video.resolution.y
			   || newFullscreen != config.video.fullscreen
			) {
				ARXMenu_Private_Options_Video_SetResolution(newFullscreen, newWidth, newHeight);
				pMenuSliderResol->iOldPos = -1;
				fullscreenCheckbox->iOldState = -1;
			}
			mainMenu->bReInitAll=true;
		}
		break;
		// MENUOPTIONS_CONTROLS
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_USE1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA1:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA2:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DEFAULT:
			{
			}break;
		case BUTTON_MENUOPTIONS_CONTROLS_BACK:
			{
				config.save();
			}
			break;
	}

	if(eMenuState == EDIT_QUEST_SAVE_CONFIRM) {
		for(size_t i = 0; i < pWindowMenu->vWindowConsoleElement.size(); i++) {
			CWindowMenuConsole *p = pWindowMenu->vWindowConsoleElement[i];

			if(p->eMenuState == eMenuState) {
				p->lData = lData;
				CMenuElementText * me = (CMenuElementText *) p->MenuAllZone.vMenuZone[1];

				if(me) {
					me->lData = lData;
					
					if(lData != -1) {
						me->SetText(savegames[lData].name);
						pDeleteButton->lColor = pDeleteButton->lOldColor;
						pDeleteButton->SetCheckOn();
					} else {
						pDeleteButton->lColor = Color::grayb(127);
						pDeleteButton->SetCheckOff();
						me->SetText(getLocalised("system_menu_editquest_newsavegame"));
					}
					
					p->AlignElementCenter(me);
				}
			}
		}
	}

	return false;
}

// true: block les zones de checks
CMenuElement* CMenuElementText::OnShortCut() {

	if(iShortCut==-1)
		return NULL;

	if(GInput->isKeyPressedNowUnPressed(iShortCut)) {
		return this;
	}

	return NULL;
}

void CMenuElementText::Render() {

	if(bNoMenu)
		return;

	Vec3f ePos;
	ePos.x = (float) rZone.left;
	ePos.y = (float) rZone.top;
	ePos.z = 1;

	if(bSelected) {
		FontRenderText(pFont, ePos, lpszText, lColorHighlight);
	} else if(enabled) {
		FontRenderText(pFont, ePos, lpszText, lColor);
	} else {
		FontRenderText(pFont, ePos, lpszText, Color::grayb(127));
	}

}

void CMenuElementText::RenderMouseOver() {

	if(bNoMenu)
		return;

	pMenuCursor->SetMouseOver();

	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	
	Vec3f ePos = Vec3f(Vec2f(rZone.topLeft()), 1.f);
	
	FontRenderText(pFont, ePos, lpszText, lColorHighlight);

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);

	switch(iID) {
		case BUTTON_MENUEDITQUEST_LOAD:
		case BUTTON_MENUEDITQUEST_SAVEINFO: {
			
			if(lData == -1) {
				pTextureLoadRender = NULL;
				break;
			}
			
			const res::path & image = savegames[lData].thumbnail;
			if(!image.empty()) {
				TextureContainer * t = TextureContainer::LoadUI(image, TextureContainer::NoColorKey);
				if(t != pTextureLoad) {
					delete pTextureLoad;
					pTextureLoad = t;
				}
				pTextureLoadRender = pTextureLoad;
			}
			
			break;
		}
		
		default: {
			pTextureLoadRender = NULL;
			break;
		}
	}
}

Vec2i CMenuElementText::GetTextSize() const {
	return pFont->getTextSize(lpszText);
}

CMenuState::CMenuState()
	: bReInitAll(false)
	, eOldMenuState(NOP)
	, eOldMenuWindowState(NOP)
	, pTexBackGround(NULL)
	, pMenuAllZone(new CMenuAllZone())
	, pZoneClick(NULL)
{}

CMenuState::~CMenuState() {
	delete pMenuAllZone;
	delete pTexBackGround;
}

void MACRO_MENU_PRINCIPALE(Vec2s & pos,
						   int MACRO_button,
						   MENUSTATE MACRO_menu,
						   const char * MACRO_locate,
						   int MACRO_check)
{
	std::string szMenuText = getLocalised( MACRO_locate );
	CMenuElementText *me = new CMenuElementText(MACRO_button, hFontMainMenu, szMenuText, Vec2i(RATIO_X(pos.x), RATIO_Y(pos.y)), MACRO_menu);
	if(MACRO_check) {
		pMenuElementResume=me;
		if(ARXMenu_CanResumeGame()) {
			me->SetCheckOn();
		} else {
			me->SetCheckOff();
			me->lColor=Color(127,127,127);
		}
	}
	mainMenu->AddMenuElement(me);
}

void CMenuState::createChildElements()
{
	mainMenu->pTexBackGround = TextureContainer::LoadUI("graph/interface/menus/menu_main_background", TextureContainer::NoColorKey);

	Vec2s pos = Vec2s(370, 100);
	int yOffset = 50;
	
	MACRO_MENU_PRINCIPALE(pos, BUTTON_MENUMAIN_RESUMEGAME,RESUME_GAME,"system_menus_main_resumegame",1);
	pos.y += yOffset;
	MACRO_MENU_PRINCIPALE(pos, BUTTON_MENUMAIN_NEWQUEST,NEW_QUEST,"system_menus_main_newquest",0);
	pos.y += yOffset;
	MACRO_MENU_PRINCIPALE(pos, -1,EDIT_QUEST,"system_menus_main_editquest",0);
	pos.y += yOffset;
	MACRO_MENU_PRINCIPALE(pos, BUTTON_MENUMAIN_OPTIONS,OPTIONS,"system_menus_main_options",0);
	pos.y += yOffset;
	MACRO_MENU_PRINCIPALE(pos, BUTTON_MENUMAIN_CREDITS,CREDITS,"system_menus_main_credits",0);
	pos.y += yOffset;
	MACRO_MENU_PRINCIPALE(pos, -1,QUIT,"system_menus_main_quit",0);
	pos.y += yOffset;

	std::string version = arx_version;
	if(!arx_release_codename.empty()) {
		version += " \"";
		version += arx_release_codename;
		version += "\"";
	}

	float verPosX = RATIO_X(620) - hFontControls->getTextSize(version).x;
	CMenuElementText * me = new CMenuElementText( -1, hFontControls, version, Vec2i(verPosX, RATIO_Y(80)), NOP );
	
	me->SetCheckOff();
	me->lColor=Color(127,127,127);
	mainMenu->AddMenuElement(me);
}

void CMenuState::AddMenuElement(CMenuElement * _me) {
	pMenuAllZone->AddZone((CMenuZone *)_me);
}

MENUSTATE CMenuState::Update() {
	
	pZoneClick=NULL;

	CMenuZone * iR=pMenuAllZone->CheckZone(GInput->getMousePosAbs());

	if(GInput->getMouseButton(Mouse::Button_0)) {
		if(iR) {
			pZoneClick = (CMenuElement*)iR;
			pZoneClick->OnMouseClick();
			return pZoneClick->eMenuState;
		}
	} else {
		if(iR) {
			pZoneClick = (CMenuElement*)iR;
		}
	}

	return NOP;
}

void CMenuState::Render() {

	if(bNoMenu)
		return;

	if(pTexBackGround)
		EERIEDrawBitmap2(Rectf(Vec2f(0, 0), g_size.width(), g_size.height()), 0.999f, pTexBackGround, Color::white);
	
	int iARXDiffTimeMenu = checked_range_cast<int>(ARXDiffTimeMenu);

	for(size_t i = 0; i < pMenuAllZone->GetNbZone(); ++i) {
		CMenuElement *pMe=(CMenuElement*)pMenuAllZone->GetZoneNum(i);
		pMe->Update(iARXDiffTimeMenu);
		pMe->Render();
	}

	//HIGHLIGHT
	if(pZoneClick) {
		pZoneClick->RenderMouseOver();
	}

	//DEBUG ZONE
	GRenderer->ResetTexture(0);
	pMenuAllZone->DrawZone();
}

CMenuZone::CMenuZone()
	: bActif(true)
	, bCheck(true)
	, bTestYDouble(false)
	, pRef(NULL)
	, rZone(0, 0, 0, 0)
	, iID(-1)
	, lData(0)
{}

CMenuZone::~CMenuZone() {

}

void CMenuZone::Move(const Vec2i & offset) {
	rZone.move(offset.x, offset.y);
}

void CMenuZone::SetPos(float _fX, float _fY) {

	int iWidth  = rZone.right - rZone.left;
	int iHeight = rZone.bottom - rZone.top;

	int iX = checked_range_cast<int>(_fX);
	int iY = checked_range_cast<int>(_fY);

	rZone.left   = iX;
	rZone.top    = iY;
	rZone.right  = iX + abs(iWidth);
	rZone.bottom = iY + abs(iHeight);
}

CMenuZone * CMenuZone::IsMouseOver(const Vec2s& mousePos) const {

	int iYDouble=0;

	if(bTestYDouble) {
		iYDouble=(rZone.bottom-rZone.top)>>1;
	}

	if(bActif
	   && mousePos.x >= rZone.left
	   && mousePos.y >= rZone.top - iYDouble
	   && mousePos.x <= rZone.right
	   && mousePos.y <= rZone.bottom + iYDouble
	) {
		return pRef;
	}

	return NULL;
}

CMenuAllZone::CMenuAllZone() {

	vMenuZone.clear();

	vector<CMenuZone*>::iterator i;

	for(i = vMenuZone.begin(); i != vMenuZone.end(); ++i) {
		CMenuZone *zone = *i;
		delete zone;
	}
}

CMenuAllZone::~CMenuAllZone() {

	for(std::vector<CMenuZone*>::iterator it = vMenuZone.begin(), it_end = vMenuZone.end(); it != it_end; ++it)
		delete *it;
}

void CMenuAllZone::AddZone(CMenuZone *_pMenuZone) {

	vMenuZone.push_back(_pMenuZone);
}

CMenuZone * CMenuAllZone::CheckZone(const Vec2s& mousePos) const {

	std::vector<CMenuZone*>::const_iterator i;

	for(i = vMenuZone.begin(); i != vMenuZone.end(); ++i) {
		CMenuZone *zone = *i;
		
		if(!zone->bCheck || !zone->bActif)
			continue;
		
		CMenuZone * pRef = zone->IsMouseOver(mousePos);
		
		if(pRef)
			return pRef;
	}

	return NULL;
}

CMenuZone * CMenuAllZone::GetZoneNum(size_t index) {
	return vMenuZone[index];
}

CMenuZone * CMenuAllZone::GetZoneWithID(int _iID) {

	for(std::vector<CMenuZone*>::iterator i = vMenuZone.begin(), i_end = vMenuZone.end(); i != i_end; ++i) {
		if(CMenuZone *zone = ((CMenuElement*)(*i))->GetZoneWithID(_iID))
			return zone;
	}

	return NULL;
}

void CMenuAllZone::Move(const Vec2i & offset) {

	for(std::vector<CMenuZone*>::iterator i = vMenuZone.begin(), i_end = vMenuZone.end(); i != i_end; ++i) {
		(*i)->Move(offset);
	}
}

size_t CMenuAllZone::GetNbZone() {
	return vMenuZone.size();
}

void CMenuAllZone::DrawZone()
{
	if(g_debugInfo != InfoPanelGuiDebug)
		return;
	
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->ResetTexture(0);

	BOOST_FOREACH(CMenuZone * zone, vMenuZone) {
		drawLineRectangle(Rectf(zone->rZone), 0.f, Color::red);
	}

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

CMenuCheckButton::CMenuCheckButton(int _iID, Vec2i pos, int _iTaille, TextureContainer *_pTex1, TextureContainer *_pTex2, CMenuElementText *_pText)
	:CMenuElement(NOP)
{
	iID = _iID;
	iState    = 0;
	iOldState = -1;

	m_pos.x = checked_range_cast<int>(pos.x);
	m_pos.y = checked_range_cast<int>(pos.y);

	iTaille = _iTaille;

	pText    = _pText;

	if(_pTex1) {
		float fRatioX = RATIO_X(_pTex1->m_dwWidth) ;
		float fRatioY = RATIO_Y(_pTex1->m_dwHeight);
		vTex.push_back(_pTex1);
		_iTaille = std::max(_iTaille, checked_range_cast<int>(fRatioX));
		_iTaille = std::max(_iTaille, checked_range_cast<int>(fRatioY));
	}

	if(_pTex2) {
		float fRatioX = RATIO_X(_pTex2->m_dwWidth) ;
		float fRatioY = RATIO_Y(_pTex2->m_dwHeight);
		vTex.push_back(_pTex2);
		_iTaille = std::max(_iTaille, checked_range_cast<int>(fRatioX));
		_iTaille = std::max(_iTaille, checked_range_cast<int>(fRatioY));
	}

	Vec2i textSize(0,0);

	if(pText) {
		textSize = pText->pFont->getTextSize(pText->lpszText); 

		_iTaille = std::max<int>(_iTaille, textSize.y);
		textSize.x += pText->rZone.left;
		pText->Move(m_pos + Vec2i(0, (_iTaille - textSize.y) / 2));
	}

	rZone.left = checked_range_cast<Rect::Num>(pos.x);
	rZone.top = checked_range_cast<Rect::Num>(pos.y);
	rZone.right = checked_range_cast<Rect::Num>(pos.x + _iTaille + textSize.x);
	rZone.bottom = checked_range_cast<Rect::Num>(pos.y + std::max<int>(_iTaille, textSize.y));
	pRef=this;
	
	if(_pTex1 && _pTex2) {
		float rZoneR = ( RATIO_X(200.f) + RATIO_X(_pTex1->m_dwWidth) + (RATIO_X(12*9) - RATIO_X(_pTex1->m_dwWidth))*0.5f );
		rZone.right = checked_range_cast<Rect::Num>(rZoneR);
	}
	
	Move(m_pos);
}

CMenuCheckButton::~CMenuCheckButton() {
	delete pText;
}

void CMenuCheckButton::Move(const Vec2i & offset) {
	
	CMenuElement::Move(offset);
	
	if(pText) {
		pText->Move(offset);
	}
}

bool CMenuCheckButton::OnMouseClick() {
	
	if(iOldState<0)
		iOldState=iState;

	iState ++;

	//NB : It seems that iState cannot be negative (used as tabular index / used as bool) but need further approval
	arx_assert(iState >= 0);

	if((size_t)iState >= vTex.size()) {
		iState = 0;
	}

	ARX_SOUND_PlayMenu(SND_MENU_CLICK);

	switch (iID) {
	case BUTTON_MENUOPTIONSVIDEO_FULLSCREEN: {
			newFullscreen = ((iState)?true:false);
			
			if(pMenuSliderResol) {
				pMenuSliderResol->setEnabled(newFullscreen);
			}
			
		}
		break;
	case BUTTON_MENUOPTIONSVIDEO_CROSSHAIR: {
			config.video.showCrosshair=(iState)?true:false;
		}
		break;
	case BUTTON_MENUOPTIONSVIDEO_ANTIALIASING: {
		config.video.antialiasing = iState ? true : false;
		ARX_SetAntiAliasing();
		break;
	}
	case BUTTON_MENUOPTIONSVIDEO_VSYNC: {
		config.video.vsync = iState ? true : false;
		break;
	}
	case BUTTON_MENUOPTIONSAUDIO_EAX: {
			ARXMenu_Options_Audio_SetEAX((iState)?true:false);
		}
		break;
	case BUTTON_MENUOPTIONS_CONTROLS_INVERTMOUSE: {
			ARXMenu_Options_Control_SetInvertMouse((iState)?true:false);
		}
		break;
	case BUTTON_MENUOPTIONS_CONTROLS_AUTOREADYWEAPON: {
		config.input.autoReadyWeapon = (iState) ? true : false;
		break;
	}
	case BUTTON_MENUOPTIONS_CONTROLS_MOUSELOOK: {
		config.input.mouseLookToggle = (iState) ? true : false;
		break;
	}
	case BUTTON_MENUOPTIONS_CONTROLS_AUTODESCRIPTION: {
		config.input.autoDescription = (iState) ? true : false;
		break;
	}
	case BUTTON_MENUOPTIONSVIDEO_BACK: {
		if(pMenuSliderResol && pMenuSliderResol->iOldPos >= 0) {
			pMenuSliderResol->iPos=pMenuSliderResol->iOldPos;
			pMenuSliderResol->iOldPos=-1;
			newWidth=config.video.resolution.x;
			newHeight=config.video.resolution.y;
		}
		
		if(fullscreenCheckbox && fullscreenCheckbox->iOldState >= 0) {
			fullscreenCheckbox->iState = fullscreenCheckbox->iOldState;
			fullscreenCheckbox->iOldState = -1;
			newFullscreen = config.video.fullscreen;
		}
		break;
	}
	}

	return false;
}

void CMenuCheckButton::Update(int /*_iDTime*/)
{
}

void CMenuCheckButton::Render() {

	if(bNoMenu)
		return;

	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);

	if(!vTex.empty()) {
		TextureContainer *pTex = vTex[iState];
		
		Color color = (bCheck) ? Color::white : Color::fromBGRA(0xFF3F3F3F);
		
		float iY = 0;
		{
			iY = static_cast<float>(rZone.bottom - rZone.top);
			iY -= iTaille;
			iY = rZone.top + iY*0.5f;
		}
		
		//carre
		EERIEDrawBitmap2(Rectf(Vec2f(rZone.right - iTaille, iY), RATIO_X(iTaille), RATIO_Y(iTaille)), 0.f, pTex, color);
	}

	if(pText)
		pText->Render();

	//DEBUG
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

void CMenuCheckButton::RenderMouseOver() {

	if(bNoMenu)
		return;

	pMenuCursor->SetMouseOver();

	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);

	TextureContainer *pTex = vTex[iState];

	if(pTex) GRenderer->SetTexture(0, pTex);
	else GRenderer->ResetTexture(0);
	
	float iY = 0;
	iY = static_cast<float>(rZone.bottom - rZone.top);
	iY -= iTaille;
	iY = rZone.top + iY*0.5f;

	//carre

	EERIEDrawBitmap2(Rectf(Vec2f(rZone.right - iTaille, iY), RATIO_X(iTaille), RATIO_Y(iTaille)), 0.f, pTex, Color::white); 

	//tick
	if (pText)
		pText->RenderMouseOver();

	//DEBUG
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

CWindowMenu::CWindowMenu(Vec2i pos, Vec2i size)
{
	m_pos = RATIO_2(pos);
	m_size = RATIO_2(size);

	vWindowConsoleElement.clear();

	fPosXCalc=((float)-m_size.x);
	fDist=((float)(m_size.x+m_pos.x));
	fAngle=0.f;

	eCurrentMenuState=NOP;


	float fCalc	= fPosXCalc + (fDist * sin(radians(fAngle)));

	m_pos.x = checked_range_cast<int>(fCalc);
}

CWindowMenu::~CWindowMenu() {

	for(std::vector<CWindowMenuConsole*>::iterator it = vWindowConsoleElement.begin(), it_end = vWindowConsoleElement.end(); it < it_end; ++it)
		delete *it;
}

void CWindowMenu::AddConsole(CWindowMenuConsole *_pMenuConsoleElement) {

	vWindowConsoleElement.push_back(_pMenuConsoleElement);
	_pMenuConsoleElement->m_oldPos.x = 0;
	_pMenuConsoleElement->m_oldPos.y = 0;
	_pMenuConsoleElement->m_pos = m_pos;
}

void CWindowMenu::Update(float _fDTime) {

	float fCalc	= fPosXCalc + (fDist * sin(radians(fAngle)));

	m_pos.x = checked_range_cast<int>(fCalc);
	fAngle += _fDTime * 0.08f;

	if(fAngle > 90.f)
		fAngle = 90.f;
}

MENUSTATE CWindowMenu::Render() {
	
	if(bNoMenu)
		return NOP;
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	
	MENUSTATE eMS=NOP;
	
	BOOST_FOREACH(CWindowMenuConsole * c, vWindowConsoleElement) {
		if(eCurrentMenuState == c->eMenuState) {
			eMS = c->Update(m_pos);
			
			if(eMS != NOP)
				break;
		}
	}
	
	BOOST_FOREACH(CWindowMenuConsole * c, vWindowConsoleElement) {
		if(eCurrentMenuState == c->eMenuState) {
			c->Render();
			break;
		}
	}
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	
	if(eMS != NOP) {
		eCurrentMenuState=eMS;
	}
	
	return eMS;
}

CWindowMenuConsole::CWindowMenuConsole(Vec2i pos, Vec2i size, MENUSTATE _eMenuState)
	: iSavePosY(0)
	, iInterligne(10)
	, pZoneClick(NULL)
	, bEdit(false)
	, lData(0)
	, bMouseAttack(false)
{
	m_offset = RATIO_2(pos);
	m_size = RATIO_2(size);
	
	eMenuState=_eMenuState;

	pTexBackground = TextureContainer::LoadUI("graph/interface/menus/menu_console_background");
	pTexBackgroundBorder = TextureContainer::LoadUI("graph/interface/menus/menu_console_background_border");

	bFrameOdd=false;
}

void CWindowMenuConsole::AddMenu(CMenuElement *_pMenuElement)
{
	_pMenuElement->ePlace=NOCENTER;

	_pMenuElement->Move(Vec2i(m_offset.x, m_offset.y));
	MenuAllZone.AddZone((CMenuZone*)_pMenuElement);
}

// TODO copy-paste
void CWindowMenuConsole::AddMenuCenterY(CMenuElement * element) {

	element->ePlace = CENTERY;

	int iDy = element->rZone.bottom - element->rZone.top;

	for(size_t iJ = 0; iJ < MenuAllZone.GetNbZone(); iJ++) {
		CMenuZone * pZone = MenuAllZone.GetZoneNum(iJ);

		iDy += iInterligne;
		iDy += pZone->rZone.bottom - pZone->rZone.top;
	}

	int iDepY = m_offset.y;

	if(iDy < m_size.y) {
		iDepY += ((m_size.y - iDy) >> 1);
	}

	int dy = 0;

	if(MenuAllZone.GetNbZone()) {
		dy = iDepY - MenuAllZone.GetZoneNum(0)->rZone.top;
	}
	
	for(size_t iJ = 0; iJ < MenuAllZone.GetNbZone(); iJ++) {
		CMenuZone *pZone = MenuAllZone.GetZoneNum(iJ);
		iDepY += (pZone->rZone.bottom - pZone->rZone.top) + iInterligne;
		
		pZone->Move(Vec2i(0, dy));
	}

	element->Move(Vec2i(0, iDepY));

	MenuAllZone.AddZone(element);
}

// TODO copy-paste
void CWindowMenuConsole::AddMenuCenter(CMenuElement * element) {

	element->ePlace = CENTER;

	int iDx = element->rZone.right - element->rZone.left;
	int dx  = ((m_size.x - iDx) >> 1) - element->rZone.left;

	if(dx < 0) {
		dx = 0;
	}

	int iDy = element->rZone.bottom - element->rZone.top;

	for(size_t iJ = 0; iJ < MenuAllZone.GetNbZone(); iJ++) {
		CMenuZone * pZone = MenuAllZone.GetZoneNum(iJ);

		iDy += iInterligne;
		iDy += pZone->rZone.bottom - pZone->rZone.top;
	}

	int iDepY = m_offset.y;

	if(iDy < m_size.y) {
		iDepY += ((m_size.y - iDy) >> 1);
	}

	int dy = 0;

	if(MenuAllZone.GetNbZone()) {
		dy = iDepY - MenuAllZone.GetZoneNum(0)->rZone.top;
	}
	
	for(size_t iJ = 0; iJ < MenuAllZone.GetNbZone(); iJ++) {
		CMenuZone *pZone = MenuAllZone.GetZoneNum(iJ);
		iDepY += (pZone->rZone.bottom - pZone->rZone.top) + iInterligne;
		
		pZone->Move(Vec2i(0, dy));
	}

	element->Move(Vec2i(dx, iDepY));

	MenuAllZone.AddZone(element);
}

void CWindowMenuConsole::AlignElementCenter(CMenuElement *_pMenuElement) {
	
	_pMenuElement->Move(Vec2i(-_pMenuElement->rZone.left, 0));
	_pMenuElement->ePlace = CENTER;
	
	int iDx = _pMenuElement->rZone.right - _pMenuElement->rZone.left;
	int dx = (m_size.x - iDx) / 2 - _pMenuElement->rZone.left;
	
	_pMenuElement->Move(Vec2i(std::max(dx, 0), 0));
}

void CWindowMenuConsole::UpdateText() {

	if(GInput->isAnyKeyPressed()) {

		if(GInput->isKeyPressed(Keyboard::Key_Enter)
		   || GInput->isKeyPressed(Keyboard::Key_NumPadEnter)
		   || GInput->isKeyPressed(Keyboard::Key_Escape)
		) {
			ARX_SOUND_PlayMenu(SND_MENU_CLICK);
			((CMenuElementText*)pZoneClick)->eState=EDIT;

			if(((CMenuElementText*)pZoneClick)->lpszText.empty()) {
				std::string szMenuText;
				szMenuText = getLocalised("system_menu_editquest_newsavegame");

				((CMenuElementText*)pZoneClick)->SetText(szMenuText);

				int iDx=pZoneClick->rZone.right-pZoneClick->rZone.left;

				if(pZoneClick->ePlace) {
					pZoneClick->rZone.left=m_pos.x+((m_size.x-iDx)>>1);

					if(pZoneClick->rZone.left < 0) {
						pZoneClick->rZone.left=0;
					}
				}

				pZoneClick->rZone.right=pZoneClick->rZone.left+iDx;
			}

			pZoneClick=NULL;
			bEdit=false;
			return;
		}

		bool bKey=false;
		std::string tText;
		
		CMenuElementText *pZoneText=(CMenuElementText*)pZoneClick;

		if(GInput->isKeyPressedNowPressed(Keyboard::Key_Backspace)) {
			tText = pZoneText->lpszText;

			if(!tText.empty()) {
				tText.resize(tText.size() - 1);
				bKey=true;
			}
		} else {
			int iKey = GInput->getKeyPressed();
			iKey&=0xFFFF;

			if(GInput->isKeyPressedNowPressed(iKey)) {
				tText = pZoneText->lpszText;

				char tCat;
				
				bKey = GInput->getKeyAsText(iKey, tCat);

				if(bKey) {
					int iChar = tCat & 0x000000FF; // To prevent ascii chars between [128, 255] from causing an assertion in the functions below...
					if((isalnum(iChar) || isspace(iChar) || ispunct(iChar)) && (tCat != '\t') && (tCat != '*'))
						tText += tCat;
				}
			}
		}

		if(bKey) {
			pZoneText->SetText(tText);

			if(pZoneText->rZone.right - pZoneText->rZone.left > m_size.x - RATIO_X(64)) {
				if(!tText.empty()) {
					tText.resize(tText.size() - 1);
					pZoneText->SetText(tText);
				}
			}

			int iDx=pZoneClick->rZone.right-pZoneClick->rZone.left;

			if(pZoneClick->ePlace) {
				pZoneClick->rZone.left=m_pos.x+((m_size.x-iDx)>>1);

				if(pZoneClick->rZone.left < 0) {
					pZoneClick->rZone.left=0;
				}
			}

			pZoneClick->rZone.right=pZoneClick->rZone.left+iDx;
		}
	}

	if(pZoneClick->rZone.top == pZoneClick->rZone.bottom) {
		Vec2i textSize = ((CMenuElementText*)pZoneClick)->pFont->getTextSize("|");
		pZoneClick->rZone.bottom += textSize.y;
	}

	//DRAW CURSOR
	TexturedVertex v[4];
	GRenderer->ResetTexture(0);
	float col=.5f+rnd()*.5f;
	v[0].color = v[1].color = v[2].color = v[3].color = Color::gray(col).toBGR();
	v[0].p.z=v[1].p.z=v[2].p.z=v[3].p.z=0.f;    
	v[0].rhw=v[1].rhw=v[2].rhw=v[3].rhw=0.999999f;

	v[0].p.x = (float)pZoneClick->rZone.right;
	v[0].p.y = (float)pZoneClick->rZone.top;
	v[1].p.x = v[0].p.x+2.f;
	v[1].p.y = v[0].p.y;
	v[2].p.x = v[0].p.x;
	v[2].p.y = (float)pZoneClick->rZone.bottom;
	v[3].p.x = v[1].p.x;
	v[3].p.y = v[2].p.y;

	EERIEDRAWPRIM(Renderer::TriangleStrip, v, 4);
}

CMenuElement * CWindowMenuConsole::GetTouch(bool keyTouched, int keyId, InputKeyId* pInputKeyId, bool _bValidateTest)
{
	int iMouseButton = keyTouched ? 0 : GInput->getMouseButtonClicked();
	
	if(pInputKeyId)
		*pInputKeyId = keyId;

	if(keyTouched || (iMouseButton & (Mouse::ButtonBase | Mouse::WheelBase))) {
		if(!keyTouched && !bMouseAttack) {
			bMouseAttack=!bMouseAttack;
			return NULL;
		}

		CMenuElementText *pZoneText=(CMenuElementText*)pZoneClick;

		if(_bValidateTest) {
			if(pZoneClick->iID == BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE1 ||
			   pZoneClick->iID == BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE2)
			{
				bool bOk=true;

				if((iMouseButton & Mouse::ButtonBase) && !(iMouseButton & Mouse::WheelBase)) {
					bOk=false;
				} else {
					for(int buttonId = Mouse::ButtonBase; buttonId < Mouse::ButtonMax; buttonId++) {
						if(keyId == buttonId) {
							bOk=false;
							break;
						}
					}
				}

				if(bOk)
					return NULL;
			}
		}

		std::string pText;
		if(iMouseButton & (Mouse::ButtonBase | Mouse::WheelBase))
			pText = GInput->getKeyName(iMouseButton, true); 
		else
			pText = GInput->getKeyName(keyId, true);

		if(!pText.empty()) {
			pZoneText->lColorHighlight=pZoneText->lOldColor;

			pZoneText->eState=GETTOUCH;
			pZoneText->SetText(pText);
			
			int iDx=pZoneClick->rZone.right-pZoneClick->rZone.left;

			if(pZoneClick->ePlace) {
				pZoneClick->rZone.left=(m_size.x-iDx)>>1;

				if(pZoneClick->rZone.left < 0) {
					pZoneClick->rZone.left=0;
				}
			}

			pZoneClick->rZone.right=pZoneClick->rZone.left+iDx;

			pZoneClick=NULL;
			bEdit=false;

			if(iMouseButton & (Mouse::ButtonBase | Mouse::WheelBase)) {
				if(pInputKeyId)
					*pInputKeyId = iMouseButton;
			}

			bMouseAttack=false;

			return (CMenuElement*)pZoneText;
		}
	}

	return NULL;
}

MENUSTATE CWindowMenuConsole::Update(Vec2i pos) {

	bFrameOdd=!bFrameOdd;

	iSavePosY=pos.y;
	
	MenuAllZone.Move(m_pos - m_oldPos);
	
	for(size_t iJ = 0; iJ < MenuAllZone.GetNbZone(); ++iJ) {
		CMenuZone *pZone = MenuAllZone.GetZoneNum(iJ);

		if(pZone->rZone.top < iSavePosY || pZone->rZone.bottom + iInterligne > iSavePosY + m_size.y) {
			pZone->bActif=false;
		} else {
			pZone->bActif=true;
		}

		pZone->bActif=true;
	}

	m_oldPos.x=m_pos.x;
	m_oldPos.y=m_pos.y;
	m_pos = pos;

	// Check if mouse over
		if(!bEdit) {
			pZoneClick=NULL;
			CMenuZone * iR = MenuAllZone.CheckZone(GInput->getMousePosAbs());

			if(iR) {
				pZoneClick=(CMenuElement*)iR;

				if(GInput->getMouseButtonDoubleClick(Mouse::Button_0, 300)) {
					MENUSTATE e = pZoneClick->eMenuState;
					bEdit = pZoneClick->OnMouseDoubleClick();

					if(pZoneClick->iID == BUTTON_MENUEDITQUEST_LOAD)
						return MAIN;

					if(bEdit)
						return pZoneClick->eMenuState;

					return e;
				}

				if(GInput->getMouseButton(Mouse::Button_0)) {
					MENUSTATE e = pZoneClick->eMenuState;
					bEdit = pZoneClick->OnMouseClick();
					return e;
				} else {
					pZoneClick->EmptyFunction();
				}
			}
		} else {
			if(!pZoneClick) {
				CMenuZone * iR = MenuAllZone.CheckZone(GInput->getMousePosAbs());

				if(iR) {
					pZoneClick=(CMenuElement*)iR;

					if(GInput->getMouseButtonDoubleClick(Mouse::Button_0, 300)) {
						bEdit = pZoneClick->OnMouseDoubleClick();

						if(bEdit)
							return pZoneClick->eMenuState;
					}
				}
			}
		}

	//check les shortcuts
	if(!bEdit) {
		for(size_t iJ = 0; iJ < MenuAllZone.GetNbZone(); ++iJ) {
			CMenuElement *pMenuElement=(CMenuElement*)MenuAllZone.GetZoneNum(iJ);
			CMenuElement *CMenuElementShortCut = pMenuElement->OnShortCut();

			if(CMenuElementShortCut) {
				pZoneClick=CMenuElementShortCut;
				MENUSTATE e = pZoneClick->eMenuState;
				bEdit = pZoneClick->OnMouseClick();
				pZoneClick=CMenuElementShortCut;
				return e;
			}
		}
	}

	return NOP;
}

static bool UpdateGameKey(bool bEdit, CMenuElement *pmeElement, InputKeyId inputKeyId) {
	bool bChange=false;

	if(!bEdit && pmeElement) {
		switch(pmeElement->iID) {
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP2:
			bChange=config.setActionKey(CONTROLS_CUST_JUMP,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE2:
			bChange=config.setActionKey(CONTROLS_CUST_MAGICMODE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE2:
			bChange=config.setActionKey(CONTROLS_CUST_STEALTHMODE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD2:
			bChange=config.setActionKey(CONTROLS_CUST_WALKFORWARD,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD2:
			bChange=config.setActionKey(CONTROLS_CUST_WALKBACKWARD,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT2:
			bChange=config.setActionKey(CONTROLS_CUST_STRAFELEFT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT2:
			bChange=config.setActionKey(CONTROLS_CUST_STRAFERIGHT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT2:
			bChange=config.setActionKey(CONTROLS_CUST_LEANLEFT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT2:
			bChange=config.setActionKey(CONTROLS_CUST_LEANRIGHT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH2:
			bChange=config.setActionKey(CONTROLS_CUST_CROUCH,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_USE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_USE2:
			bChange=config.setActionKey(CONTROLS_CUST_USE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_USE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE2:
			bChange=config.setActionKey(CONTROLS_CUST_ACTION,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY2:
			bChange=config.setActionKey(CONTROLS_CUST_INVENTORY,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK2:
			bChange=config.setActionKey(CONTROLS_CUST_BOOK,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKCHARSHEET1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKCHARSHEET2:
			bChange=config.setActionKey(CONTROLS_CUST_BOOKCHARSHEET,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKCHARSHEET1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKSPELL1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKSPELL2:
			bChange=config.setActionKey(CONTROLS_CUST_BOOKSPELL,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKSPELL1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKMAP1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKMAP2:
			bChange=config.setActionKey(CONTROLS_CUST_BOOKMAP,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKMAP1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKQUEST1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKQUEST2:
			bChange=config.setActionKey(CONTROLS_CUST_BOOKQUEST,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKQUEST1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE2:
			bChange=config.setActionKey(CONTROLS_CUST_DRINKPOTIONLIFE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA2:
			bChange=config.setActionKey(CONTROLS_CUST_DRINKPOTIONMANA,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TORCH1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TORCH2:
			bChange=config.setActionKey(CONTROLS_CUST_TORCH,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_TORCH1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CANCELCURSPELL1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CANCELCURSPELL2:    
			bChange=config.setActionKey(CONTROLS_CUST_CANCELCURSPELL,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_CANCELCURSPELL1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST1_2:
			bChange=config.setActionKey(CONTROLS_CUST_PRECAST1,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST2:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST2_2:
			bChange=config.setActionKey(CONTROLS_CUST_PRECAST2,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST2,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST3:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST3_2:
			bChange=config.setActionKey(CONTROLS_CUST_PRECAST3,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST3,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WEAPON1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_WEAPON2:
			bChange=config.setActionKey(CONTROLS_CUST_WEAPON,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_WEAPON1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKLOAD:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKLOAD2:
			bChange=config.setActionKey(CONTROLS_CUST_QUICKLOAD,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKLOAD,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKSAVE:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKSAVE2:
			bChange=config.setActionKey(CONTROLS_CUST_QUICKSAVE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKSAVE,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNLEFT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNLEFT2:
			bChange=config.setActionKey(CONTROLS_CUST_TURNLEFT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNLEFT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNRIGHT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNRIGHT2:
			bChange=config.setActionKey(CONTROLS_CUST_TURNRIGHT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNRIGHT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKUP1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKUP2:
			bChange=config.setActionKey(CONTROLS_CUST_LOOKUP,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKUP1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKDOWN1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKDOWN2:
			bChange=config.setActionKey(CONTROLS_CUST_LOOKDOWN,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKDOWN1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFE2:
			bChange=config.setActionKey(CONTROLS_CUST_STRAFE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CENTERVIEW1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CENTERVIEW2:
			bChange=config.setActionKey(CONTROLS_CUST_CENTERVIEW,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_CENTERVIEW1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_FREELOOK1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_FREELOOK2:
			bChange=config.setActionKey(CONTROLS_CUST_FREELOOK,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_FREELOOK1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PREVIOUS1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_PREVIOUS2:
			bChange=config.setActionKey(CONTROLS_CUST_PREVIOUS,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_PREVIOUS1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_NEXT1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_NEXT2:    
			bChange=config.setActionKey(CONTROLS_CUST_NEXT,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_NEXT1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCHTOGGLE1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCHTOGGLE2:    
			bChange=config.setActionKey(CONTROLS_CUST_CROUCHTOGGLE,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCHTOGGLE1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_UNEQUIPWEAPON1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_UNEQUIPWEAPON2:    
			bChange=config.setActionKey(CONTROLS_CUST_UNEQUIPWEAPON,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_UNEQUIPWEAPON1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_MINIMAP1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_MINIMAP2:
			bChange=config.setActionKey(CONTROLS_CUST_MINIMAP,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_MINIMAP1,inputKeyId);
			break;
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TOGGLE_FULLSCREEN1:
		case BUTTON_MENUOPTIONS_CONTROLS_CUST_TOGGLE_FULLSCREEN2:
			bChange=config.setActionKey(CONTROLS_CUST_TOGGLE_FULLSCREEN,pmeElement->iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_TOGGLE_FULLSCREEN1,inputKeyId);
			break;
		}
	}

	return bChange;
}

void CWindowMenuConsole::Render() {

	if(bNoMenu)
		return;
	
	//------------------------------------------------------------------------
	// Console display
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);
	GRenderer->SetRenderState(Renderer::DepthTest, false);

	EERIEDrawBitmap2(Rectf(Vec2f(m_pos.x, iSavePosY),
	                 RATIO_X(pTexBackground->m_dwWidth), RATIO_Y(pTexBackground->m_dwHeight)),
	                 0, pTexBackground, Color::white);

	GRenderer->SetRenderState(Renderer::DepthTest, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	EERIEDrawBitmap2(Rectf(Vec2f(m_pos.x, iSavePosY),
	                 RATIO_X(pTexBackgroundBorder->m_dwWidth), RATIO_Y(pTexBackgroundBorder->m_dwHeight)),
	                 0, pTexBackgroundBorder, Color::white);

	//------------------------------------------------------------------------

	int iARXDiffTimeMenu  = checked_range_cast<int>(ARXDiffTimeMenu);

	for(size_t i = 0; i < MenuAllZone.GetNbZone(); ++i) {
		CMenuElement *pMe=(CMenuElement*)MenuAllZone.GetZoneNum(i);

		if(pMe->bActif) {
			pMe->Update(iARXDiffTimeMenu);
			pMe->Render();
		}
	}

	//HIGHLIGHT
	if(pZoneClick && pZoneClick->bActif) {
		bool bReInit=false;

		pZoneClick->RenderMouseOver();

		switch(pZoneClick->eState) {
		case EDIT_TIME:
			UpdateText();
			break;
		case GETTOUCH_TIME: {
				if(bFrameOdd)
					((CMenuElementText*)pZoneClick)->lColorHighlight = Color(255, 0, 0);
				else
					((CMenuElementText*)pZoneClick)->lColorHighlight = Color(50, 0, 0);

				bool keyTouched = GInput->isAnyKeyPressed();
				int keyId = GInput->getKeyPressed();
				
				if( GInput->isKeyPressed(Keyboard::Key_LeftShift)||
					GInput->isKeyPressed(Keyboard::Key_RightShift)||
					GInput->isKeyPressed(Keyboard::Key_LeftCtrl)||
					GInput->isKeyPressed(Keyboard::Key_RightCtrl)||
					GInput->isKeyPressed(Keyboard::Key_LeftAlt)||
					GInput->isKeyPressed(Keyboard::Key_RightAlt)
				) {
					if(!((keyId & INPUT_COMBINATION_MASK )>>16))
						keyTouched = false;
				} else {
					if(GInput->isKeyPressedNowUnPressed(Keyboard::Key_LeftShift)) {
						keyTouched = true;
						keyId = Keyboard::Key_LeftShift;
					}

					if(GInput->isKeyPressedNowUnPressed(Keyboard::Key_RightShift)) {
						keyTouched = true;
						keyId = Keyboard::Key_RightShift;
					}

					if(GInput->isKeyPressedNowUnPressed(Keyboard::Key_LeftCtrl)) {
						keyTouched = true;
						keyId = Keyboard::Key_LeftCtrl;
					}

					if(GInput->isKeyPressedNowUnPressed(Keyboard::Key_RightCtrl)) {
						keyTouched = true;
						keyId = Keyboard::Key_RightCtrl;
					}

					if(GInput->isKeyPressedNowUnPressed(Keyboard::Key_LeftAlt)) {
						keyTouched = true;
						keyId = Keyboard::Key_LeftAlt;
					}

					if(GInput->isKeyPressedNowUnPressed(Keyboard::Key_RightAlt)) {
						keyTouched = true;
						keyId = Keyboard::Key_RightAlt;
					}
				}

				InputKeyId inputKeyId;
				CMenuElement *pmeElement = GetTouch(keyTouched, keyId, &inputKeyId, true);

				if(pmeElement) {
					if(UpdateGameKey(bEdit,pmeElement, inputKeyId)) {
						bReInit=true;
					}
				}
			}
			break;
		default:
			{
				if(GInput->getMouseButtonNowPressed(Mouse::Button_0)) {
					CMenuZone *pmzMenuZone = MenuAllZone.GetZoneWithID(BUTTON_MENUOPTIONS_CONTROLS_CUST_DEFAULT);

					if(pmzMenuZone==pZoneClick) {
						config.setDefaultActionKeys();
						bReInit=true;
					}
				}
			}
			break;
		}

		if(bReInit) {
			ReInitActionKey();
			bMouseAttack=false;
		}
	}

	//DEBUG ZONE
	MenuAllZone.DrawZone();
}

void CWindowMenuConsole::ReInitActionKey()
{
	int iID=BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP1;
	int iI=NUM_ACTION_KEY;
	
	while(iI--) {
		int iTab=(iID-BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP1)>>1;

		CMenuZone *pmzMenuZone = MenuAllZone.GetZoneWithID(iID);

		if(pmzMenuZone) {
			if(pmzMenuZone) {
				pZoneClick = (CMenuElement*)pmzMenuZone;
				GetTouch(true, config.actions[iTab].key[0]);
			}

			pmzMenuZone = MenuAllZone.GetZoneWithID(iID+1);

			if(pmzMenuZone) {
				pZoneClick = (CMenuElement*)pmzMenuZone;
				GetTouch(true, config.actions[iTab].key[1]);
			}
		}

		iID+=2;
	}
}

CMenuPanel::CMenuPanel()
	: CMenuElement(NOP)
{
	vElement.clear();
	pRef = this;
}

CMenuPanel::~CMenuPanel()
{
	BOOST_FOREACH(CMenuElement * e, vElement) {
		delete e;
	}
}

void CMenuPanel::Move(const Vec2i & offset)
{
	rZone.move(offset.x, offset.y);
	
	BOOST_FOREACH(CMenuElement * e, vElement) {
		e->Move(offset);
	}
}

// patch on ajoute à droite en ligne
void CMenuPanel::AddElement(CMenuElement* _pElem)
{
	vElement.push_back(_pElem);

	if(vElement.size() == 1) {
		rZone = _pElem->rZone;
	} else {
		rZone.left = std::min(rZone.left, _pElem->rZone.left);
		rZone.top = std::min(rZone.top, _pElem->rZone.top);
	}

	// + taille elem
	rZone.right = std::max(rZone.right, _pElem->rZone.right);
	rZone.bottom = std::max(rZone.bottom, _pElem->rZone.bottom);

	_pElem->Move(Vec2i(0, ((GetHeight() - _pElem->rZone.bottom) / 2)));
}

// patch on ajoute à droite en ligne
void CMenuPanel::AddElementNoCenterIn(CMenuElement* _pElem)
{
	vElement.push_back(_pElem);

	if(vElement.size() == 1) {
		rZone = _pElem->rZone;
	} else {
		rZone.left = std::min(rZone.left, _pElem->rZone.left);
		rZone.top = std::min(rZone.top, _pElem->rZone.top);
	}

	// + taille elem
	rZone.right = std::max(rZone.right, _pElem->rZone.right);
	rZone.bottom = std::max(rZone.bottom, _pElem->rZone.bottom);
}

CMenuElement* CMenuPanel::OnShortCut()
{
	BOOST_FOREACH(CMenuElement * e, vElement) {
		if(e->OnShortCut())
			return e;
	}

	return NULL;
}

void CMenuPanel::Update(int _iTime)
{
	rZone.right = rZone.left;
	rZone.bottom = rZone.top;

	BOOST_FOREACH(CMenuElement * e, vElement) {
		e->Update(_iTime);
		rZone.right = std::max(rZone.right, e->rZone.right);
		rZone.bottom = std::max(rZone.bottom, e->rZone.bottom);
	}
}

void CMenuPanel::Render() {

	if(bNoMenu)
		return;

	BOOST_FOREACH(CMenuElement * e, vElement) {
		e->Render();
	}
}

CMenuZone * CMenuPanel::GetZoneWithID(int _iID)
{
	BOOST_FOREACH(CMenuElement * e, vElement) {
		if(CMenuZone * pZone = e->GetZoneWithID(_iID))
			return pZone;
	}
	
	return NULL;
}

CMenuZone * CMenuPanel::IsMouseOver(const Vec2s& mousePos) const {

	if(rZone.contains(Vec2i(mousePos))) {
		BOOST_FOREACH(CMenuElement * e, vElement) {
			if(e->bCheck && e->bActif && e->rZone.contains(Vec2i(mousePos))) {
				return e->pRef;
			}
		}
	}

	return NULL;
}

CMenuButton::CMenuButton(int _iID, MENUSTATE _eMenuState, Vec2i pos, TextureContainer *_pTex)
	: CMenuElement(_eMenuState)
{
	iID = _iID;

	rZone.left=pos.x;
	rZone.top=pos.y;
	rZone.right  = rZone.left ;
	rZone.bottom = rZone.top ;
	
	pTex=_pTex;
	
	if(pTex) {
		float rZoneR = rZone.left + RATIO_X(pTex->m_dwWidth);
		float rZoneB = rZone.top + RATIO_Y(pTex->m_dwHeight);
		rZone.right  = max(rZone.right,  checked_range_cast<Rect::Num>(rZoneR));
		rZone.bottom = max(rZone.bottom, checked_range_cast<Rect::Num>(rZoneB));
	}
	
	pRef=this;
}

CMenuButton::~CMenuButton() {
}

void CMenuButton::SetPos(float _iX,float _iY)
{
	CMenuZone::SetPos(_iX, _iY);

	int iWidth = 0;
	int iHeight = 0;
	if(pTex) {
		iWidth = checked_range_cast<int>(RATIO_X(pTex->m_dwWidth));
		iHeight = checked_range_cast<int>(RATIO_Y(pTex->m_dwHeight));
	}
	
	rZone.right = static_cast<int>(_iX) + iWidth;
	rZone.bottom = static_cast<int>(_iY) + iHeight;
}

bool CMenuButton::OnMouseClick() {
	
	if(!enabled) {
		return false;
	}
	
	ARX_SOUND_PlayMenu(SND_MENU_CLICK);

	return false;
}

void CMenuButton::Update(int _iDTime) {
	(void)_iDTime;
}

void CMenuButton::Render() {

	if(bNoMenu)
		return;

	//affichage de la texture
	if(pTex) {
		EERIEDrawBitmap2(Rectf(rZone), 0, pTex, Color::white);
	}
}

void CMenuButton::RenderMouseOver() {

	if(bNoMenu)
		return;

	pMenuCursor->SetMouseOver();
}

CMenuSliderText::CMenuSliderText(int _iID, Vec2i pos)
	: CMenuElement(NOP)
{
	iID = _iID;
	TextureContainer *pTex = TextureContainer::Load("graph/interface/menus/menu_slider_button_left");
	pLeftButton = new CMenuButton(-1, NOP, pos, pTex);
	pTex = TextureContainer::Load("graph/interface/menus/menu_slider_button_right");
	pRightButton = new CMenuButton(-1, NOP, pos, pTex);

	vText.clear();

	iPos = 0;
	iOldPos = -1;

	rZone.left   = pos.x;
	rZone.top    = pos.y;
	rZone.right  = pos.x + pLeftButton->GetWidth() + pRightButton->GetWidth();
	rZone.bottom = pos.y + max(pLeftButton->GetHeight(), pRightButton->GetHeight());

	pRef = this;
}

CMenuSliderText::~CMenuSliderText() {
	delete pLeftButton;
	delete pRightButton;
	BOOST_FOREACH(CMenuElementText * e, vText) {
		delete e;
	}
}

void CMenuSliderText::AddText(CMenuElementText *_pText) {
	
	_pText->setEnabled(enabled);
	
	_pText->Move(Vec2i(rZone.left + pLeftButton->GetWidth(), rZone.top + 0));
	vText.push_back(_pText);

	Vec2i textSize = _pText->GetTextSize();

	rZone.right  = max(rZone.right, rZone.left + pLeftButton->GetWidth() + pRightButton->GetWidth() + textSize.x);
	rZone.bottom = max(rZone.bottom, rZone.top + textSize.y);

	pLeftButton->SetPos(rZone.left, rZone.top+(textSize.y>>2));
	pRightButton->SetPos(rZone.right-pRightButton->GetWidth(), rZone.top+(textSize.y>>2));

	int dx=rZone.right-rZone.left-pLeftButton->GetWidth()-pRightButton->GetWidth();
	//on recentre tout
	vector<CMenuElementText*>::iterator it;

	for(it = vText.begin(); it < vText.end(); ++it) {
		CMenuElementText *pMenuElementText=*it;
		
		textSize = pMenuElementText->GetTextSize();

		int dxx=(dx-textSize.x)>>1;
		pMenuElementText->SetPos(static_cast<float>(pLeftButton->rZone.right + dxx), static_cast<float>(rZone.top));
	}
}

void CMenuSliderText::Move(const Vec2i & offset) {

	CMenuZone::Move(offset);

	pLeftButton->Move(offset);
	pRightButton->Move(offset);

	for(std::vector<CMenuElementText*>::const_iterator i = vText.begin(), i_end = vText.end(); i != i_end; ++i)
		(*i)->Move(offset);
}

void CMenuSliderText::EmptyFunction() {

	//Touche pour la selection
	if(GInput->isKeyPressedNowPressed(Keyboard::Key_LeftArrow)) {
		iPos--;

		if(iPos <= 0)
			iPos = 0;
	} else {
		if(GInput->isKeyPressedNowPressed(Keyboard::Key_RightArrow)) {
			iPos++;

			arx_assert(iPos >= 0);

			if((size_t)iPos >= vText.size() - 1)
				iPos = vText.size() - 1;
		}
	}
}

bool CMenuSliderText::OnMouseClick() {
	
	if(!enabled) {
		return false;
	}
	
	ARX_SOUND_PlayMenu(SND_MENU_CLICK);

	if(iOldPos<0)
		iOldPos=iPos;
	
	const Vec2i cursor = Vec2i(GInput->getMousePosAbs());

	if(rZone.contains(cursor)) {
		if(pLeftButton->rZone.contains(cursor)) {
			iPos--;

			if(iPos < 0) {
				iPos = vText.size() - 1;
			}
		}
		
		if(pRightButton->rZone.contains(cursor)) {
			iPos++;

			arx_assert(iPos >= 0);

			if(size_t(iPos) >= vText.size()) {
				iPos = 0;
			}
		}
	}

	switch(iID) {
		case BUTTON_MENUOPTIONSVIDEO_RESOLUTION: {
			std::string pcText = (vText.at(iPos))->lpszText;
			
			if(pcText == AUTO_RESOLUTION_STRING) {
				newWidth = newHeight = 0;
			} else {
				std::stringstream ss( pcText );
				int iX = config.video.resolution.x;
				int iY = config.video.resolution.y;
				char tmp;
				ss >> iX >> tmp >> iY;
				newWidth = iX;
				newHeight = iY;
			}
			break;
		}
		case BUTTON_MENUOPTIONSVIDEO_RENDERER: {
			switch((vText.at(iPos))->eMenuState) {
				case OPTIONS_VIDEO_RENDERER_OPENGL:    config.window.framework = "SDL"; break;
				case OPTIONS_VIDEO_RENDERER_AUTOMATIC: config.window.framework = "auto"; break;
				default: break;
			}
			break;
		}
		case BUTTON_MENUOPTIONSAUDIO_BACKEND: {
			switch((vText.at(iPos))->eMenuState) {
				case OPTIONS_AUDIO_BACKEND_OPENAL:    config.audio.backend = "OpenAL"; break;
				case OPTIONS_AUDIO_BACKEND_AUTOMATIC: config.audio.backend = "auto"; break;
				default: break;
			}
			break;
		}
		// MENUOPTIONS_VIDEO
		case BUTTON_MENUOPTIONSVIDEO_OTHERSDETAILS: {
			ARXMenu_Options_Video_SetDetailsQuality(iPos);
			break;
		}
	}
	
	return false;
}

void CMenuSliderText::Update(int _iTime) {

	pLeftButton->Update(_iTime);
	pRightButton->Update(_iTime);
}

void CMenuSliderText::Render() {
	
	if(bNoMenu)
		return;
	
	if(enabled) {
		pLeftButton->Render();
		pRightButton->Render();
	}
	
	if(iPos >= 0 && size_t(iPos) < vText.size() && vText[iPos]) {
		GRenderer->SetRenderState(Renderer::AlphaBlending, false);
		vText[iPos]->Render();
		GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	}
}

void CMenuSliderText::setEnabled(bool enable) {
	CMenuElement::setEnabled(enable);
	pLeftButton->setEnabled(enable);
	pRightButton->setEnabled(enable);
	for(size_t i = 0; i < vText.size(); i++) {
		vText[i]->setEnabled(enable);
	}
}

void CMenuSliderText::RenderMouseOver() {

	if(bNoMenu)
		return;

	pMenuCursor->SetMouseOver();

	Vec2i cursor = Vec2i(GInput->getMousePosAbs());
	
	if(!enabled) {
		return;
	}

	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);

	if(rZone.contains(cursor)) {
		if(pLeftButton->rZone.contains(cursor)) {
			pLeftButton->Render();
		}
		
		if(pRightButton->rZone.contains(cursor)) {
			pRightButton->Render();
		}
	}
}


//-----------------------------------------------------------------------------
// CMenuSlider
//-----------------------------------------------------------------------------

CMenuSlider::CMenuSlider(int _iID, Vec2i pos)
	: CMenuElement(NOP)
{
	iID = _iID;

	TextureContainer *pTexL = TextureContainer::Load("graph/interface/menus/menu_slider_button_left");
	TextureContainer *pTexR = TextureContainer::Load("graph/interface/menus/menu_slider_button_right");
	pLeftButton = new CMenuButton(-1, NOP, pos, pTexL);
	pRightButton = new CMenuButton(-1, NOP, pos, pTexR);
	pTex1 = TextureContainer::Load("graph/interface/menus/menu_slider_on");
	pTex2 = TextureContainer::Load("graph/interface/menus/menu_slider_off");
	arx_assert(pTex1);
	arx_assert(pTex2);
	
	iPos = 0;

	rZone.left   = pos.x;
	rZone.top    = pos.y;
	rZone.right  = pos.x + pLeftButton->GetWidth() + pRightButton->GetWidth() + 10*max(pTex1->m_dwWidth, pTex2->m_dwWidth);
	rZone.bottom = pos.y + max(pLeftButton->GetHeight(), pRightButton->GetHeight());

	arx_assert(rZone.bottom >= 0);
	rZone.bottom = max( static_cast<unsigned long>( rZone.bottom ), (unsigned long)max( pTex1->m_dwHeight, pTex2->m_dwHeight ) );


	pRightButton->Move(Vec2i(pLeftButton->GetWidth() + 10*max(pTex1->m_dwWidth, pTex2->m_dwWidth), 0));

	pRef = this;
}

CMenuSlider::~CMenuSlider() {
	delete pLeftButton;
	delete pRightButton;
}

void CMenuSlider::Move(const Vec2i & offset) {
	CMenuZone::Move(offset);
	pLeftButton->Move(offset);
	pRightButton->Move(offset);
}

void CMenuSlider::EmptyFunction() {

	//Touche pour la selection
	if(GInput->isKeyPressedNowPressed(Keyboard::Key_LeftArrow)) {
		iPos--;

		if(iPos <= 0)
			iPos = 0;
	} else {
		if(GInput->isKeyPressedNowPressed(Keyboard::Key_RightArrow)) {
			iPos++;

			if(iPos >= 10)
				iPos = 10;
		}
	}
}

bool CMenuSlider::OnMouseClick() {
	
	ARX_SOUND_PlayMenu(SND_MENU_CLICK);

	const Vec2i cursor = Vec2i(GInput->getMousePosAbs());
	
	if(rZone.contains(cursor)) {
		if(pLeftButton->rZone.contains(cursor)) {
			iPos--;
			if(iPos <= 0)
				iPos = 0;
		}
		
		if(pRightButton->rZone.contains(cursor)) {
			iPos++;
			if(iPos >= 10)
				iPos = 10;
		}
	}
	
	switch (iID) {
	// MENUOPTIONS_VIDEO
	case BUTTON_MENUOPTIONSVIDEO_FOG:
		ARXMenu_Options_Video_SetFogDistance(iPos);
		break;
	// MENUOPTIONS_AUDIO
	case BUTTON_MENUOPTIONSAUDIO_MASTER:
		ARXMenu_Options_Audio_SetMasterVolume(iPos);
		break;
	case BUTTON_MENUOPTIONSAUDIO_SFX:
		ARXMenu_Options_Audio_SetSfxVolume(iPos);
		break;
	case BUTTON_MENUOPTIONSAUDIO_SPEECH:
		ARXMenu_Options_Audio_SetSpeechVolume(iPos);
		break;
	case BUTTON_MENUOPTIONSAUDIO_AMBIANCE:
		ARXMenu_Options_Audio_SetAmbianceVolume(iPos);
		break;
	// MENUOPTIONS_CONTROLS
	case BUTTON_MENUOPTIONS_CONTROLS_MOUSESENSITIVITY:
		ARXMenu_Options_Control_SetMouseSensitivity(iPos);
		break;
		case BUTTON_MENUOPTIONS_CONTROLS_QUICKSAVESLOTS: {
			iPos = std::max(iPos, 1);
			config.misc.quicksaveSlots = iPos;
			break;
		}
	}

	return false;
}

void CMenuSlider::Update(int _iTime) {
	
	pLeftButton->Update(_iTime);
	pRightButton->Update(_iTime);
	pRightButton->SetPos(rZone.left, rZone.top);


	float fWidth = pLeftButton->GetWidth() + RATIO_X(10*max(pTex1->m_dwWidth, pTex2->m_dwWidth)) ;
	pRightButton->Move(Vec2i(checked_range_cast<int>(fWidth), 0));

	rZone.right  = checked_range_cast<Rect::Num>( rZone.left + pLeftButton->GetWidth() + pRightButton->GetWidth() + RATIO_X(10*std::max(pTex1->m_dwWidth, pTex2->m_dwWidth)) );

	rZone.bottom = rZone.top + std::max(pLeftButton->GetHeight(), pRightButton->GetHeight());
}

void CMenuSlider::Render() {

	if(bNoMenu)
		return;

	pLeftButton->Render();
	pRightButton->Render();

	Vec2f pos(rZone.left + pLeftButton->GetWidth(), rZone.top);
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	
	for(int i = 0; i < 10; i++) {
		TextureContainer * pTex = (i < iPos) ? pTex1 : pTex2;
		Rectf rect = Rectf(pos, RATIO_X(pTex->m_dwWidth), RATIO_Y(pTex->m_dwHeight));
		
		EERIEDrawBitmap2(rect, 0, pTex, Color::white);
		
		pos.x += rect.width();
	}

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

void CMenuSlider::RenderMouseOver() {

	if(bNoMenu)
		return;

	pMenuCursor->SetMouseOver();

	const Vec2i cursor = Vec2i(GInput->getMousePosAbs());

	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	
	if(rZone.contains(cursor)) {
		if(pLeftButton->rZone.contains(cursor)) {
			pLeftButton->Render();
		}
		
		if(pRightButton->rZone.contains(cursor)) {
			pRightButton->Render();
		}
	}
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

MenuCursor::MenuCursor() {
	
	SetCursorOff();
	
	iNbOldCoord=0;
	iMaxOldCoord=40;
	
	exited = true;
	m_storedTime = 0;
	
	bMouseOver=false;
	
	iNumCursor=0;
	lFrameDiff=0;
}

MenuCursor::~MenuCursor()
{
}

void MenuCursor::SetCursorOff() {
	eNumTex=CURSOR_OFF;
}

void MenuCursor::SetCursorOn() {
	eNumTex=CURSOR_ON;
}

void MenuCursor::SetMouseOver() {
	bMouseOver=true;
	SetCursorOn();
}

void MenuCursor::DrawOneCursor(const Vec2s& mousePos) {
	
	if(!GInput->isMouseInWindow()) {
		return;
	}
	
	GRenderer->GetTextureStage(0)->setMinFilter(TextureStage::FilterNearest);
	GRenderer->GetTextureStage(0)->setMagFilter(TextureStage::FilterNearest);
	GRenderer->GetTextureStage(0)->setWrapMode(TextureStage::WrapClamp);

	EERIEDrawBitmap2(Rectf(Vec2f(mousePos),
	                 scursor[iNumCursor]->m_dwWidth,
	                 scursor[iNumCursor]->m_dwHeight),
	                 0.00000001f, scursor[iNumCursor], Color::white);

	GRenderer->GetTextureStage(0)->setMinFilter(TextureStage::FilterLinear);
	GRenderer->GetTextureStage(0)->setMagFilter(TextureStage::FilterLinear);
	GRenderer->GetTextureStage(0)->setWrapMode(TextureStage::WrapRepeat);
}

void MenuCursor::reset() {
	iNbOldCoord = 0;
}

void MenuCursor::update(float time) {
	
	bool inWindow = GInput->isMouseInWindow();
	if(inWindow && exited) {
		// Mouse is re-entering the window - reset the cursor trail
		reset();
	}
	exited = !inWindow;
	
	Vec2s iDiff;
	if(scursor[eNumTex]) {
		iDiff = Vec2s(scursor[eNumTex]->m_dwWidth / 2, scursor[eNumTex]->m_dwHeight / 2);
	} else {
		iDiff = Vec2s_ZERO;
	}
	
	iOldCoord[iNbOldCoord] = GInput->getMousePosAbs() + iDiff;
	
	const float targetFPS = 61.f;
	const float targetDelay = 1000.f / targetFPS;
	m_storedTime += time;
	if(m_storedTime > targetDelay) {
		m_storedTime = std::min(targetDelay, m_storedTime - targetDelay);
		
		iNbOldCoord++;
		
		if(iNbOldCoord >= iMaxOldCoord) {
			iNbOldCoord = iMaxOldCoord - 1;
			memmove(iOldCoord, iOldCoord + 1, sizeof(Vec2s) * iNbOldCoord);
		}
		
	}
	
}

bool MenuCursor::ComputePer(const Vec2s & _psPoint1, const Vec2s & _psPoint2, TexturedVertex * _psd3dv1, TexturedVertex * _psd3dv2, float _fSize) {
	
	Vec2f sTemp((float)(_psPoint2.x - _psPoint1.x), (float)(_psPoint2.y - _psPoint1.y));
	float fTemp = sTemp.x;
	sTemp.x = -sTemp.y;
	sTemp.y = fTemp;
	float fMag = glm::length(sTemp);
	if(fMag < std::numeric_limits<float>::epsilon()) {
		return false;
	}

	fMag = _fSize / fMag;

	_psd3dv1->p.x=(sTemp.x*fMag);
	_psd3dv1->p.y=(sTemp.y*fMag);
	_psd3dv2->p.x=((float)_psPoint1.x)-_psd3dv1->p.x;
	_psd3dv2->p.y=((float)_psPoint1.y)-_psd3dv1->p.y;
	_psd3dv1->p.x+=((float)_psPoint1.x);
	_psd3dv1->p.y+=((float)_psPoint1.y);

	return true;
}

void MenuCursor::DrawLine2D(float _fSize, float _fRed, float _fGreen, float _fBlue) {
	
	if(iNbOldCoord < 2) {
		return;
	}
	
	float fSize = _fSize / iNbOldCoord;
	float fTaille = fSize;
	
	float fDColorRed = _fRed / iNbOldCoord;
	float fColorRed = fDColorRed;
	float fDColorGreen = _fGreen / iNbOldCoord;
	float fColorGreen = fDColorGreen;
	float fDColorBlue = _fBlue / iNbOldCoord;
	float fColorBlue = fDColorBlue;
	
	GRenderer->SetBlendFunc(Renderer::BlendDstColor, Renderer::BlendInvDstColor);
	GRenderer->ResetTexture(0);
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	
	TexturedVertex v[4];
	v[0].p.z = v[1].p.z = v[2].p.z = v[3].p.z = 0.f;
	v[0].rhw = v[1].rhw = v[2].rhw = v[3].rhw = 0.999999f;
	
	v[0].color = v[2].color = Color3f(fColorRed, fColorGreen, fColorBlue).toBGR();
	
	if(!ComputePer(iOldCoord[0], iOldCoord[1], &v[0], &v[2], fTaille)) {
		v[0].p.x = v[2].p.x = iOldCoord[0].x;
		v[0].p.y = v[2].p.y = iOldCoord[1].y;
	}
	
	for(int i = 1; i < iNbOldCoord - 1; i++) {
		
		fTaille += fSize;
		fColorRed += fDColorRed;
		fColorGreen += fDColorGreen;
		fColorBlue += fDColorBlue;
		
		if(ComputePer(iOldCoord[i], iOldCoord[i + 1], &v[1], &v[3], fTaille)) {
			
			v[1].color = v[3].color = Color3f(fColorRed, fColorGreen, fColorBlue).toBGR();
			EERIEDRAWPRIM(Renderer::TriangleStrip, v, 4);
			
			v[0].p.x = v[1].p.x;
			v[0].p.y = v[1].p.y;
			v[0].color = v[1].color;
			v[2].p.x = v[3].p.x;
			v[2].p.y = v[3].p.y;
			v[2].color = v[3].color;
		}
		
	}
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

void MenuCursor::DrawCursor() {
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	DrawLine2D(10.f, .725f, .619f, 0.56f);

	if(scursor[iNumCursor])
		GRenderer->SetTexture(0, scursor[iNumCursor]);
	else 
		GRenderer->ResetTexture(0);

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);

	GRenderer->SetRenderState(Renderer::DepthTest, false);
	DrawOneCursor(GInput->getMousePosAbs());
	GRenderer->SetRenderState(Renderer::DepthTest, true);

	lFrameDiff += checked_range_cast<long>(ARXDiffTimeMenu);

	if(lFrameDiff > 70) {
		if(bMouseOver) {
			if(iNumCursor < 4) {
				iNumCursor++;
			} else {
				if(iNumCursor > 4) {
					iNumCursor--;
				}
			}

			SetCursorOff();
			bMouseOver=false;
		} else {
			if(iNumCursor > 0) {
				iNumCursor++;

				if(iNumCursor > 7)
					iNumCursor=0;
			}
		}

		lFrameDiff=0;
	}

	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

void Menu2_Open() {
	if(pMenuCursor) {
		pMenuCursor->reset();
	}
}

void Menu2_Close() {
	
	ARXmenu.currentmode = AMCM_OFF;
	
	delete pWindowMenu, pWindowMenu = NULL;
	delete mainMenu, mainMenu = NULL;
	delete pMenuCursor, pMenuCursor = NULL;
}

void MenuReInitAll() {
	
	if(!mainMenu)
		return;
	
	mainMenu->bReInitAll=true;
}
