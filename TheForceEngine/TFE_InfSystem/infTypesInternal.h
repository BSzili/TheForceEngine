#pragma once
#include <TFE_System/types.h>
#include <TFE_Level/rsector.h>
#include <TFE_Level/rwall.h>
#include <TFE_Level/robject.h>
#include "infPublicTypes.h"

struct TextureData;
struct Allocator;

using namespace TFE_Level;

namespace TFE_InfSystem
{
	struct InfLink;
	
	// How an elevator moves if triggered.
	enum ElevTrigMove
	{
		TRIGMOVE_HOLD = -1,	// The elevator is holding, so the trigger message has no effect.
		TRIGMOVE_CONT = 0,	// Continue to the next stop if NOT already moving.
		TRIGMOVE_LAST = 1,	// Set next (target) stop to the last stop whether moving or not.
		TRIGMOVE_NEXT = 2,	// Set next (target) stop to the next stop if NOT waiting for a timed delay.
		TRIGMOVE_PREV = 3,	// Set next (target) stop to the prev stop if waiting for a timed delay or goes back 2 stops if NOT waiting for a timed delay.
	};

	enum ElevUpdateFlags
	{
		ELEV_MOVING         = FLAG_BIT(0),	// elevator should be moving.
		ELEV_MASTER_ON      = FLAG_BIT(1),	// master on
		ELEV_MOVING_REVERSE = FLAG_BIT(2),	// the elevator is moving in reverse.
	};

	enum InfDelay
	{
		// IDELAY_SECONDS < IDELAY_COMPLETE
		IDELAY_HOLD      = 0xffffffff,
		IDELAY_TERMINATE = 0xfffffffe,
		IDELAY_COMPLETE  = 0xfffffffd,
	};
		
	enum TriggerType
	{
		ITRIGGER_WALL = 0,
		ITRIGGER_SECTOR,
		ITRIGGER_SWITCH1,
		ITRIGGER_TOGGLE,
		ITRIGGER_SINGLE
	};
		
	enum InfEntityMask
	{
		INF_ENTITY_ENEMY     = FLAG_BIT(0),
		INF_ENTITY_WEAPON    = FLAG_BIT(3),
		INF_ENTITY_SMART_OBJ = FLAG_BIT(11),
		INF_ENTITY_PLAYER    = FLAG_BIT(31),
		INF_ENTITY_ANY = 0xffffffffu
	};

	enum InfElevatorFlags
	{
		INF_EFLAG_MOVE_FLOOR = FLAG_BIT(0),	// Move on floor.
		INF_EFLAG_MOVE_SECHT = FLAG_BIT(1),	// Move on second height.
		INF_EFLAG_MOVE_CEIL  = FLAG_BIT(2),	// Move on ceiling (head has to be close enough?)
		INF_EFLAG_DOOR       = FLAG_BIT(3),	// This is a door auto-created from the sector flag.
	};

	enum KeyItem
	{
		KEY_NONE   = 0,
		KEY_RED    = 23,
		KEY_YELLOW = 24,
		KEY_BLUE   = 25,
	};

	enum TeleportType
	{
		TELEPORT_BASIC = 0,
		TELEPORT_CHUTE = 1
	};

	struct Teleport
	{
		RSector* sector;
		RSector* target;
		TeleportType type;

		vec3_fixed dstPosition;
		angle14_16  dstAngle[3];
	};

	struct TriggerTarget
	{
		RSector* sector;
		RWall*   wall;
		u32      eventMask;
	};

	struct InfTrigger
	{
		TriggerType type;
		InfLink* link;
		AnimatedTexture* animTex;
		Allocator* targets;
		InfMessageType cmd;
		u32 event;
		u32 arg0;
		u32 arg1;

		s32 u20;
		s32 u24;
		s32 u28;
		s32 u2c;

		u32 u30;
		s32 u34;
		s32 master;
		TextureData* tex;
		SoundSourceID soundId;
		s32 state;
		s32* u48;
		u32 textId;
	};

	struct InfMessage
	{
		RSector* sector;
		RWall* wall;
		InfMessageType msgType;
		u32 event;
		u32 arg1;
		u32 arg2;
	};

	struct Slave
	{
		RSector* sector;
		s32 value;
	};

	struct AdjoinCmd
	{
		RWall* wall0;
		RWall* wall1;
		RSector* sector0;
		RSector* sector1;
	};

	struct Stop
	{
		fixed16_16 value;
		Tick delay;				// delay in 'ticks' (see TICKS_PER_SECOND in TFE_DarkForces/time.h)
		Allocator* messages;
		Allocator* adjoinCmds;
		SoundSourceID pageId;
		TextureData* floorTex;
		TextureData* ceilTex;
	};

	struct InfElevator
	{
		InfElevator* self;
		InfElevatorType type;
		ElevTrigMove trigMove;
		RSector* sector;
		KeyItem key;
		s32 fixedStep;
		Tick nextTick;
		s32 u1c;
		Allocator* stops;
		Allocator* slaves;
		Stop* nextStop;
		fixed16_16 speed;
		fixed16_16* value;
		fixed16_16 iValue;
		vec2_fixed dirOrCenter;
		u32 flags;
		SoundSourceID sound0;
		SoundSourceID sound1;
		SoundSourceID sound2;
		SoundEffectID loopingSoundID;
		s32 u54;
		s32 updateFlags;
	};
}