#include "imuse.h"
#include "imSoundFader.h"
#include <TFE_System/system.h>
#include <TFE_Audio/midi.h>

namespace TFE_Jedi
{
	////////////////////////////////////////////////////
	// Structures
	////////////////////////////////////////////////////
	struct ImTrigger
	{
		s32 soundId;
		s32 marker;
		s32 opcode;
		s32 args[10];
	};

	struct ImDeferCmd
	{
		s32 time;
		s32 opcode;
		s32 args[10];
	};

	/////////////////////////////////////////////////////
	// Internal State
	/////////////////////////////////////////////////////
	static ImTrigger s_triggers[16];
	static ImDeferCmd s_deferCmd[8];
	static s32 s_imDeferredCmds = 0;

	void ImHandleDeferredCommand(ImDeferCmd* cmd);

	/////////////////////////////////////////////////////////// 
	// API
	/////////////////////////////////////////////////////////// 
	s32 ImSetTrigger(s32 soundId, s32 marker, s32 opcode)
	{
		if (!soundId) { return imArgErr; }

		ImTrigger* trigger = s_triggers;
		for (s32 i = 0; i < 16; i++, trigger++)
		{
			if (!trigger->soundId)
			{
				trigger->soundId = soundId;
				trigger->marker = marker;
				trigger->opcode = opcode;

				// The code beyond this point handles variable parameters - but this isn't used in Dark Forces.
				// It blindly copies 10 arguments from the stack and stores them in trigger->args[]
				return imSuccess;
			}
		}
		TFE_System::logWrite(LOG_ERROR, "IMuse", "tr unable to alloc trigger.");
		return imAllocErr;
	}

	// Returns the number of matching triggers.
	// '-1' acts as a wild card.
	s32 ImCheckTrigger(s32 soundId, s32 marker, s32 opcode)
	{
		ImTrigger* trigger = s_triggers;
		s32 count = 0;
		for (s32 i = 0; i < 16; i++, trigger++)
		{
			if (trigger->soundId)
			{
				if ((soundId == -1 || soundId == trigger->soundId) &&
					(marker == -1 || marker == trigger->marker) &&
					(opcode == -1 || opcode == trigger->opcode))
				{
					count++;
				}
			}
		}
		return count;
	}

	s32 ImClearTrigger(s32 soundId, s32 marker, s32 opcode)
	{
		ImTrigger* trigger = s_triggers;
		for (s32 i = 0; i < 16; i++, trigger++)
		{
			// Only clear set triggers and match Sound ID
			if (trigger->soundId && (soundId == -1 || soundId == trigger->soundId))
			{
				// Match marker and opcode.
				if ((marker == -1 || marker == trigger->marker) && (opcode == -1 || opcode == trigger->opcode))
				{
					trigger->soundId = 0;
				}
			}
		}
		return imSuccess;
	}

	s32 ImClearTriggersAndCmds()
	{
		for (s32 i = 0; i < 16; i++)
		{
			s_triggers[i].soundId = 0;
		}
		for (s32 i = 0; i < 8; i++)
		{
			s_deferCmd[i].time = 0;
		}
		s_imDeferredCmds = 0;
		return imSuccess;
	}

	// The original function can take a variable number of arguments, but it is used exactly once in Dark Forces,
	// so I simplified it.
	s32 ImDeferCommand(s32 time, s32 opcode, s32 arg1)
	{
		ImDeferCmd* cmd = s_deferCmd;
		if (time == 0)
		{
			return imArgErr;
		}

		for (s32 i = 0; i < 8; i++, cmd++)
		{
			if (cmd->time == 0)
			{
				cmd->opcode = opcode;
				cmd->time = time;

				// This copies variable arguments into cmd->args[], for Dark Forces, only a single argument is used.
				cmd->args[0] = arg1;
				s_imDeferredCmds = 1;
				return imSuccess;
			}
		}

		TFE_System::logWrite(LOG_ERROR, "IMuse", "tr unable to alloc deferred cmd.");
		return imAllocErr;
	}
		
	void ImHandleDeferredCommands()
	{
		ImDeferCmd* cmd = s_deferCmd;
		if (s_imDeferredCmds)
		{
			s_imDeferredCmds = 0;
			for (s32 i = 0; i < 8; i++, cmd++)
			{
				if (cmd->time)
				{
					s_imDeferredCmds = 1;
					cmd->time--;
					if (cmd->time == 0)
					{
						ImHandleDeferredCommand(cmd);
					}
				}
			}
		}
	}

	////////////////////////////////////
	// Internal
	////////////////////////////////////
	void ImHandleDeferredCommand(ImDeferCmd* cmd)
	{
		// In Dark Forces, only stop sound is used, basically to deal stopping a sound after an iMuse transition.
		// In the original code, this calls the dispatch function and pushes all 10 arguments.
		// A non-dispatch version can be created here with a simple case statement.
		if (cmd->opcode == imStopSound)
		{
			ImStopSound(cmd->args[0]);
		}
	}

}  // namespace TFE_Jedi
