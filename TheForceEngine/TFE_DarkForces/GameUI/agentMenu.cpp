#include "agentMenu.h"
#include "editBox.h"
#include "delt.h"
#include "uiDraw.h"
#include <TFE_DarkForces/agent.h>
#include <TFE_DarkForces/util.h>
#include <TFE_Archive/archive.h>
#include <TFE_Settings/settings.h>
#include <TFE_Input/input.h>
#include <TFE_RenderBackend/renderBackend.h>
#include <TFE_Jedi/Math/core_math.h>
#include <TFE_Jedi/Level/rtexture.h>
#include <TFE_System/system.h>

using namespace TFE_Jedi;

namespace TFE_DarkForces
{
	///////////////////////////////////////////
	// Constants
	///////////////////////////////////////////
	enum AgentMenuButtons
	{
		AGENT_NEW,
		AGENT_REMOVE,
		AGENT_EXIT,
		AGENT_BEGIN,
		AGENT_COUNT
	};
	static const Vec2i c_agentButtons[AGENT_COUNT] =
	{
		{21, 165},	// AGENT_NEW
		{90, 165},	// AGENT_REMOVE
		{164, 165},	// AGENT_EXIT
		{239, 165},	// AGENT_BEGIN
	};
	static const Vec2i c_agentButtonDim = { 60, 25 };

	enum NewAgentDlg
	{
		NEW_AGENT_NO,
		NEW_AGENT_YES,
		NEW_AGENT_COUNT
	};
	static const Vec2i c_newAgentButtons[NEW_AGENT_COUNT] =
	{
		{167, 107},	// NEW_AGENT_NO
		{206, 107},	// NEW_AGENT_YES
	};
	static const Vec2i c_newAgentButtonDim = { 28, 15 };

	///////////////////////////////////////////
	// Internal State
	///////////////////////////////////////////
	static JBool s_loaded = JFALSE;
	static JBool s_displayInit = JFALSE;
	static u8 s_framebuffer[320 * 200];
	static Vec2i s_cursorPosAccum;
	static Vec2i s_cursorPos;

	static s32 s_selectedMission = 0;
	static s32 s_editCursorFlicker = 0;
	static s32 s_agentCount = 0;
	static JBool s_lastSelectedAgent = JTRUE;
	static JBool s_newAgentDlg = JFALSE;
	static JBool s_removeAgentDlg = JFALSE;
	static JBool s_quitConfirmDlg = JFALSE;
	static JBool s_missionBegin = JFALSE;
	static EditBox s_editBox = {};

	static s32 s_buttonPressed = -1;
	static JBool s_buttonHover = JFALSE;
	
	static u8 s_menuPalette[768];
	static DeltFrame* s_agentMenuFrames;
	static DeltFrame* s_agentDlgFrames;
	static DeltFrame  s_cursor;
	static u32 s_agentMenuCount;
	static u32 s_agentDlgCount;
	static Font* s_sysFoint;

	static char s_newAgentName[32];

	///////////////////////////////////////////
	// Forward Declarations
	///////////////////////////////////////////
	void agentMenu_startup();
	void agentMenu_startupDisplay();
	void agentMenu_blit();
	void agentMenu_handleMousePosition();
	void agentMenu_update();
	void agentMenu_draw();

	void drawYesNoDlg(s32 baseFrame);
	void drawNewAgentDlg();

	void updateNewAgentDlg();
	void updateRemoveAgentDlg();
	JBool updateQuitConfirmDlg();

	///////////////////////////////////////////
	// API Implementation
	///////////////////////////////////////////
	JBool agentMenu_update(s32* levelIndex)
	{
		if (!s_loaded)
		{
			agentMenu_startup();
			s_loaded = JTRUE;
		}
		if (!s_displayInit)
		{
			agentMenu_startupDisplay();
			s_missionBegin = JFALSE;
			s_displayInit = JTRUE;
		}

		// Update the mouse position.
		agentMenu_handleMousePosition();
		agentMenu_update();
		agentMenu_draw();
		
		agentMenu_blit();

		// Reset for next time.
		if (s_missionBegin)
		{
			s_displayInit = JFALSE;
		}

		*levelIndex = s_selectedMission + 1;
		return ~s_missionBegin;
	}
	
	///////////////////////////////////////////
	// Internal Implementation
	///////////////////////////////////////////
	void agentMenu_update()
	{
		if (s_newAgentDlg)
		{
			updateNewAgentDlg();
			return;
		}
		else if (s_removeAgentDlg)
		{
			updateRemoveAgentDlg();
			return;
		}
		else if (s_quitConfirmDlg)
		{
			if (updateQuitConfirmDlg())
			{
				TFE_System::postQuitMessage();
			}
			return;
		}

		const s32 x = s_cursorPos.x;
		const s32 z = s_cursorPos.z;
		if (TFE_Input::mousePressed(MBUTTON_LEFT))
		{
			s_buttonPressed = -1;
			for (u32 i = 0; i < AGENT_COUNT; i++)
			{
				if (x >= c_agentButtons[i].x && x < c_agentButtons[i].x + c_agentButtonDim.x &&
					z >= c_agentButtons[i].z && z < c_agentButtons[i].z + c_agentButtonDim.z)
				{
					s_buttonPressed = s32(i);
					s_buttonHover = JTRUE;
					break;
				}
			}

			if (s_buttonPressed < 0)
			{
				if (x >= 25 && x < 122 && z >= 35 && z < 147 && s_agentCount > 0)
				{
					s_agentId = max(0, min((z - 35) / 8, (s32)s_agentCount - 1));
					s_selectedMission = s_agentData[s_agentId].selectedMission - 1;
					s_lastSelectedAgent = true;
				}
				else if (x >= 171 && x < 290 && z >= 35 && z < 147 && s_agentCount > 0)
				{
					s_selectedMission = max(0, min((z - 35) / 8, (s32)s_agentData[s_agentId].nextMission - 1));
					s_agentData[s_agentId].selectedMission = s_selectedMission + 1;
					s_lastSelectedAgent = false;
				}
			}
		}
		else if (TFE_Input::mouseDown(MBUTTON_LEFT) && s_buttonPressed >= 0)
		{
			// Verify that the mouse is still over the button.
			if (x >= c_agentButtons[s_buttonPressed].x && x < c_agentButtons[s_buttonPressed].x + c_agentButtonDim.x &&
				z >= c_agentButtons[s_buttonPressed].z && z < c_agentButtons[s_buttonPressed].z + c_agentButtonDim.z)
			{
				s_buttonHover = true;
			}
			else
			{
				s_buttonHover = false;
			}
		}
		else
		{
			if (TFE_Input::keyPressed(KEY_N))
			{
				s_buttonPressed = AGENT_NEW;
				s_buttonHover = JTRUE;
			}
			else if (TFE_Input::keyPressed(KEY_R))
			{
				s_buttonPressed = AGENT_REMOVE;
				s_buttonHover = JTRUE;
			}
			else if (TFE_Input::keyPressed(KEY_D))
			{
				s_buttonPressed = AGENT_EXIT;
				s_buttonHover = JTRUE;
			}
			else if (TFE_Input::keyPressed(KEY_B) || TFE_Input::keyPressed(KEY_RETURN) || TFE_Input::keyPressed(KEY_KP_ENTER))
			{
				s_buttonPressed = AGENT_BEGIN;
				s_buttonHover = JTRUE;
			}

			// Arrow keys to change the selected agent or mission.
			if (TFE_Input::bufferedKeyDown(KEY_DOWN))
			{
				if (s_lastSelectedAgent && s_agentCount > 0)
				{
					s_agentId++;
					if (s_agentId >= (s32)s_agentCount) { s_agentId = 0; }
					if (s_agentId >= 0) { s_selectedMission = s_agentData[s_agentId].selectedMission - 1; }
				}
				else if (!s_lastSelectedAgent && s_agentId >= 0 && s_agentData[s_agentId].nextMission > 1)
				{
					s_selectedMission++;
					if (s_selectedMission > s_agentData[s_agentId].nextMission - 1) { s_selectedMission = 0; }
				}
			}
			else if (TFE_Input::bufferedKeyDown(KEY_UP))
			{
				if (s_lastSelectedAgent && s_agentCount > 0)
				{
					s_agentId--;
					if (s_agentId < 0) { s_agentId = s_agentCount - 1; }
					if (s_agentId >= 0) { s_selectedMission = s_agentData[s_agentId].selectedMission - 1; }
				}
				else if (!s_lastSelectedAgent && s_agentId >= 0 && s_agentData[s_agentId].nextMission > 1)
				{
					s_selectedMission--;
					if (s_selectedMission < 0) { s_selectedMission = s_agentData[s_agentId].nextMission - 1; }
				}
			}
			else if (TFE_Input::bufferedKeyDown(KEY_LEFT) || TFE_Input::bufferedKeyDown(KEY_RIGHT))
			{
				s_lastSelectedAgent = !s_lastSelectedAgent;
			}

			if (s_buttonPressed >= AGENT_NEW && s_buttonHover)
			{
				switch (s_buttonPressed)
				{
					case AGENT_NEW:
					{
						s_newAgentDlg = JTRUE;
						memset(s_newAgentName, 0, 32);
						s_editBox.cursor = 0;
						s_editBox.inputField = s_newAgentName;
						s_editBox.maxLen = 16;
					} break;
					case AGENT_REMOVE:
						s_removeAgentDlg = JTRUE;
						break;
					case AGENT_EXIT:
						s_quitConfirmDlg = JTRUE;
						break;
					case AGENT_BEGIN:
						if (s_agentCount > 0 && s_selectedMission >= 0)
						{
							s_missionBegin = JTRUE;
						}
						break;
				};
			}

			// Reset.
			s_buttonPressed = -1;
			s_buttonHover = JFALSE;
		}
	}

	void agentMenu_draw()
	{
		memset(s_framebuffer, 0, 320 * 200);
		blitDeltaFrame(&s_agentMenuFrames[0], 0, 0, s_framebuffer);

		// Draw the buttons
		for (s32 i = 0; i < 4; i++)
		{
			s32 index;
			if (s_newAgentDlg)
			{
				if (i == 0) { index = i * 2 + 1; }
				else { index = i * 2 + 2; }
			}
			else if (s_removeAgentDlg)
			{
				if (i == 1) { index = i * 2 + 1; }
				else { index = i * 2 + 2; }
			}
			else if (s_quitConfirmDlg)
			{
				if (i == 2) { index = i * 2 + 1; }
				else { index = i * 2 + 2; }
			}
			else
			{
				if (s_buttonPressed == i && s_buttonHover) { index = i * 2 + 1; }
				else { index = i * 2 + 2; }
			}
			blitDeltaFrame(&s_agentMenuFrames[index], 0, 0, s_framebuffer);
		}

		// 11 = easy, 12 = medium, 13 = hard
		s32 yOffset = -20;
		s32 textX = 174;
		s32 textY = 36;
		u8 color = 47;
		if (s_agentId >= 0 && s_agentCount > 0)
		{
			for (u32 i = 0; i < s_agentData[s_agentId].nextMission - 1; i++)
			{
				color = 47;
				if (s_selectedMission == i)
				{
					color = 32;
					drawColoredQuad(textX - 3, textY - 1, 118, 9, s_lastSelectedAgent ? 13 : 12, s_framebuffer);
				}

				print(s_levelDisplayNames[i], textX, textY, color, s_framebuffer);
				s32 diff = s_agentData[s_agentId].completed[i] + 11;
				blitDeltaFrame(&s_agentMenuFrames[diff], 0, yOffset, s_framebuffer);
				yOffset += 8;
				textY += 8;
			}
			color = 47;
			if (s_selectedMission == s_agentData[s_agentId].nextMission - 1)
			{
				color = 32;
				drawColoredQuad(textX - 3, textY - 1, 118, 9, s_lastSelectedAgent ? 13 : 12, s_framebuffer);
			}
			print(s_levelDisplayNames[s_agentData[s_agentId].nextMission - 1], textX, textY, color, s_framebuffer);
		}

		// Draw agents.
		textX = 28;
		textY = 36;
		for (u32 i = 0; i < s_agentCount; i++)
		{
			color = 47;
			if (s_agentId == i)
			{
				color = 32;
				drawColoredQuad(textX - 3, textY - 1, 118, 9, s_lastSelectedAgent ? 12 : 13, s_framebuffer);
			}
			print(s_agentData[i].name, textX, textY, color, s_framebuffer);
			textY += 8;
		}

		if (s_newAgentDlg)
		{
			drawNewAgentDlg();
		}
		else if (s_removeAgentDlg)
		{
			drawYesNoDlg(0);
		}
		else if (s_quitConfirmDlg)
		{
			drawYesNoDlg(1);
		}
	}

	void agentMenu_handleMousePosition()
	{
		DisplayInfo displayInfo;
		TFE_RenderBackend::getDisplayInfo(&displayInfo);

		u32 width, height;
		width = TFE_RenderBackend::getVirtualDisplayWidth2D();
		height = TFE_RenderBackend::getVirtualDisplayHeight();

		s32 dx, dy;
		TFE_Input::getMouseMove(&dx, &dy);

		s_cursorPosAccum.x += dx;
		s_cursorPosAccum.z += dy;
		s_cursorPos.x = max(0, min(s_cursorPosAccum.x * (s32)height / (s32)displayInfo.height, (s32)width - 3));
		s_cursorPos.z = max(0, min(s_cursorPosAccum.z * (s32)height / (s32)displayInfo.height, (s32)height - 3));
	}

	void setPalette()
	{
		// Update the palette.
		u32 palette[256];
		u32* outColor = palette;
		u8* srcColor = s_menuPalette;
		for (s32 i = 0; i < 256; i++, outColor++, srcColor += 3)
		{
			*outColor = u32(srcColor[0]) | (u32(srcColor[1]) << 8u) | (u32(srcColor[2]) << 16u) | (0xffu << 24u);
		}
		TFE_RenderBackend::setPalette(palette);
	}

	// TFE Specific.
	void agentMenu_startupDisplay()
	{
		// Setup or update the virtual display.
		TFE_Settings_Graphics* graphics = TFE_Settings::getGraphicsSettings();
		u32 vdispFlags = 0;
		if (graphics->asyncFramebuffer)
		{
			vdispFlags |= VDISP_ASYNC_FRAMEBUFFER;
		}
		if (graphics->gpuColorConvert)
		{
			vdispFlags |= VDISP_GPU_COLOR_CONVERT;
		}

		// TFE Specific: always use 320x200 when rendering in-game UI (for now).
		VirtualDisplayInfo vdisp =
		{
			DMODE_ASPECT_CORRECT,	// Output display mode.
			vdispFlags,				// See VirtualDisplayFlags.

			320,	// full width
			200,	// full height
			320,	// width for 2D game UI and cutscenes.
			320,	// width for 3D drawing.
		};
		TFE_RenderBackend::createVirtualDisplay(vdisp);

		// Reset the cursor.
		s_cursorPosAccum = { 0, 0 };
		s_cursorPos = { 0, 0 };

		setPalette();
	}
		
	void agentMenu_startup()
	{
		FilePath filePath;
		if (!TFE_Paths::getFilePath("AGENTMNU.LFD", &filePath)) { return; }
		Archive* archive = Archive::getArchive(ARCHIVE_LFD, "AGENTMNU", filePath.path);
		TFE_Paths::addLocalArchive(archive);

		loadPaletteFromPltt("agentmnu.pltt", s_menuPalette);
		s_agentMenuCount = getFramesFromAnim("agentmnu.anim", &s_agentMenuFrames);
		s_agentDlgCount = getFramesFromAnim("agentdlg.anim", &s_agentDlgFrames);
		getFrameFromDelt("cursor.delt", &s_cursor);
		TFE_Paths::removeLastArchive();

		createFont();

		for (s32 i = 0; i < MAX_AGENT_COUNT; i++)
		{
			if (!s_agentData[i].name[0])
			{
				break;
			}
			s_agentCount++;
		}

		if (s_agentCount > 0)
		{
			s_agentId = 0;
			s_selectedMission = s_agentData[s_agentId].selectedMission - 1;
		}
		else
		{
			s_agentId = -1;
			s_selectedMission = 0;
		}

		s_missionBegin = JFALSE;
	}
		
	void agentMenu_blit()
	{
		blitDeltaFrame(&s_cursor, s_cursorPos.x, s_cursorPos.z, s_framebuffer);

		setPalette();
		TFE_RenderBackend::updateVirtualDisplay(s_framebuffer, 320 * 200);
	}

	void agentMenu_createNewAgent()
	{
		if (s_agentCount < MAX_AGENT_COUNT)
		{
			memset(&s_agentData[s_agentCount], 0, sizeof(AgentData));
			strCopyAndZero(s_agentData[s_agentCount].name, s_newAgentName, 32);
			s_agentData[s_agentCount].difficulty = 1;
			s_agentData[s_agentCount].nextMission = 1;
			s_agentData[s_agentCount].selectedMission = 1;
			s_agentId = s_agentCount;
			s_agentCount++;
		}
	}

	void agentMenu_removeAgent(s32 index)
	{
		if (s_agentCount < 1)
		{
			return;
		}

		for (s32 i = index; i < (s32)s_agentCount; i++)
		{
			s_agentData[i] = s_agentData[i + 1];
		}
		s_agentCount--;
		memset(&s_agentData[s_agentCount], 0, sizeof(AgentData));
		
		s_agentId = s_agentCount > 0 ? 0 : -1;
		s_selectedMission = s_agentCount > 0 ? s_agentData[0].selectedMission : -1;
	}

	// UI routines.
	void drawYesNoButtons(u32 indexNo, u32 indexYes)
	{
		if (s_buttonPressed == 0 && s_buttonHover)
		{
			blitDeltaFrame(&s_agentDlgFrames[indexNo], 0, 0, s_framebuffer);
		}
		else if (s_buttonPressed == 1 && s_buttonHover)
		{
			blitDeltaFrame(&s_agentDlgFrames[indexYes], 0, 0, s_framebuffer);
		}
	}

	void drawYesNoDlg(s32 baseFrame)
	{
		blitDeltaFrame(&s_agentDlgFrames[baseFrame], 0, 0, s_framebuffer);
		drawYesNoButtons(4, 6);
	}

	void drawNewAgentDlg()
	{
		blitDeltaFrame(&s_agentDlgFrames[2], 0, 0, s_framebuffer);
		drawEditBox(&s_editBox, 106, 87, 216, 100, s_framebuffer);
		drawYesNoButtons(8, 10);
	}

	void updateNewAgentDlg()
	{
		updateEditBox(&s_editBox);

		if (TFE_Input::keyPressed(KEY_RETURN) || TFE_Input::keyPressed(KEY_KP_ENTER))
		{
			agentMenu_createNewAgent();
			s_newAgentDlg = JFALSE;
			return;
		}
		else if (TFE_Input::keyPressed(KEY_ESCAPE))
		{
			s_newAgentDlg = JFALSE;
			return;
		}

		if (TFE_Input::mousePressed(MBUTTON_LEFT))
		{
			s_buttonPressed = -1;
			for (u32 i = 0; i < NEW_AGENT_COUNT; i++)
			{
				if (s_cursorPos.x >= c_newAgentButtons[i].x && s_cursorPos.x < c_newAgentButtons[i].x + c_newAgentButtonDim.x &&
					s_cursorPos.z >= c_newAgentButtons[i].z && s_cursorPos.z < c_newAgentButtons[i].z + c_newAgentButtonDim.z)
				{
					s_buttonPressed = s32(i);
					s_buttonHover = true;
					break;
				}
			}
		}
		else if (TFE_Input::mouseDown(MBUTTON_LEFT) && s_buttonPressed >= 0)
		{
			// Verify that the mouse is still over the button.
			if (s_cursorPos.x >= c_newAgentButtons[s_buttonPressed].x && s_cursorPos.x < c_newAgentButtons[s_buttonPressed].x + c_newAgentButtonDim.x &&
				s_cursorPos.z >= c_newAgentButtons[s_buttonPressed].z && s_cursorPos.z < c_newAgentButtons[s_buttonPressed].z + c_newAgentButtonDim.z)
			{
				s_buttonHover = true;
			}
			else
			{
				s_buttonHover = false;
			}
		}
		else
		{
			// Activate button 's_escButtonPressed'
			if (s_buttonPressed >= NEW_AGENT_NO && s_buttonHover)
			{
				if (s_buttonPressed == NEW_AGENT_YES)
				{
					agentMenu_createNewAgent();
				}
				s_newAgentDlg = JFALSE;
			}
			// Reset.
			s_buttonPressed = -1;
			s_buttonHover = false;
		}
	}

	void updateRemoveAgentDlg()
	{
		if (TFE_Input::keyPressed(KEY_RETURN) || TFE_Input::keyPressed(KEY_KP_ENTER) || TFE_Input::keyPressed(KEY_ESCAPE))
		{
			s_removeAgentDlg = false;
			return;
		}

		if (TFE_Input::mousePressed(MBUTTON_LEFT))
		{
			s_buttonPressed = -1;
			for (u32 i = 0; i < NEW_AGENT_COUNT; i++)
			{
				if (s_cursorPos.x >= c_newAgentButtons[i].x && s_cursorPos.x < c_newAgentButtons[i].x + c_newAgentButtonDim.x &&
					s_cursorPos.z >= c_newAgentButtons[i].z && s_cursorPos.z < c_newAgentButtons[i].z + c_newAgentButtonDim.z)
				{
					s_buttonPressed = s32(i);
					s_buttonHover = true;
					break;
				}
			}
		}
		else if (TFE_Input::mouseDown(MBUTTON_LEFT) && s_buttonPressed >= 0)
		{
			// Verify that the mouse is still over the button.
			if (s_cursorPos.x >= c_newAgentButtons[s_buttonPressed].x && s_cursorPos.x < c_newAgentButtons[s_buttonPressed].x + c_newAgentButtonDim.x &&
				s_cursorPos.z >= c_newAgentButtons[s_buttonPressed].z && s_cursorPos.z < c_newAgentButtons[s_buttonPressed].z + c_newAgentButtonDim.z)
			{
				s_buttonHover = true;
			}
			else
			{
				s_buttonHover = false;
			}
		}
		else
		{
			if (TFE_Input::keyPressed(KEY_Y))
			{
				agentMenu_removeAgent(s_agentId);
				s_removeAgentDlg = false;
			}
			else if (TFE_Input::keyPressed(KEY_N))
			{
				s_removeAgentDlg = false;
			}
			else if (s_buttonPressed >= NEW_AGENT_NO && s_buttonHover)
			{
				if (s_buttonPressed == NEW_AGENT_YES)
				{
					agentMenu_removeAgent(s_agentId);
				}
				s_removeAgentDlg = false;
			}
			// Reset.
			s_buttonPressed = -1;
			s_buttonHover = false;
		}
	}

	JBool updateQuitConfirmDlg()
	{
		JBool quit = JFALSE;
		if (TFE_Input::keyPressed(KEY_ESCAPE))
		{
			s_quitConfirmDlg = JFALSE;
			return JFALSE;
		}
		else if (TFE_Input::keyPressed(KEY_RETURN) || TFE_Input::keyPressed(KEY_KP_ENTER))
		{
			s_quitConfirmDlg = JFALSE;
			return JTRUE;
		}

		if (TFE_Input::mousePressed(MBUTTON_LEFT))
		{
			s_buttonPressed = -1;
			for (u32 i = 0; i < NEW_AGENT_COUNT; i++)
			{
				if (s_cursorPos.x >= c_newAgentButtons[i].x && s_cursorPos.x < c_newAgentButtons[i].x + c_newAgentButtonDim.x &&
					s_cursorPos.z >= c_newAgentButtons[i].z && s_cursorPos.z < c_newAgentButtons[i].z + c_newAgentButtonDim.z)
				{
					s_buttonPressed = s32(i);
					s_buttonHover = JTRUE;
					break;
				}
			}
		}
		else if (TFE_Input::mouseDown(MBUTTON_LEFT) && s_buttonPressed >= 0)
		{
			// Verify that the mouse is still over the button.
			if (s_cursorPos.x >= c_newAgentButtons[s_buttonPressed].x && s_cursorPos.x < c_newAgentButtons[s_buttonPressed].x + c_newAgentButtonDim.x &&
				s_cursorPos.z >= c_newAgentButtons[s_buttonPressed].z && s_cursorPos.z < c_newAgentButtons[s_buttonPressed].z + c_newAgentButtonDim.z)
			{
				s_buttonHover = JTRUE;
			}
			else
			{
				s_buttonHover = JFALSE;
			}
		}
		else
		{
			// Activate button 's_escButtonPressed'
			if (TFE_Input::keyPressed(KEY_Y))
			{
				quit = JTRUE;
				s_quitConfirmDlg = JFALSE;
			}
			else if (TFE_Input::keyPressed(KEY_N))
			{
				s_quitConfirmDlg = JFALSE;
			}
			else if (s_buttonPressed >= NEW_AGENT_NO && s_buttonHover)
			{
				if (s_buttonPressed == NEW_AGENT_YES)
				{
					quit = JTRUE;
				}
				s_quitConfirmDlg = JFALSE;
			}
			// Reset.
			s_buttonPressed = -1;
			s_buttonHover = false;
		}

		return quit;
	}
}