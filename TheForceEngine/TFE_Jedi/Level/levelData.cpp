#include <climits>
#include <cstring>

#include "levelData.h"
#include "rsector.h"
#include "rwall.h"
#include "robjData.h"
#include <TFE_Game/igame.h>
#include <TFE_Jedi/Serialization/serialization.h>

// TODO: coupling between Dark Forces and Jedi.
using namespace TFE_DarkForces;
#include <TFE_DarkForces/projectile.h>

namespace TFE_Jedi
{
	enum LevelStateVersion : u32
	{
		LevelState_InitVersion = 1,
		LevelState_CurVersion = LevelState_InitVersion,
	};

	LevelState s_levelState = {};
	LevelInternalState s_levelIntState = {};

	void level_serializeSector(Stream* stream, RSector* sector);
	void level_serializeSafe(Stream* stream, Safe* safe);
	void level_serializeAmbientSound(Stream* stream, AmbientSound* sound);
	void level_serializeWall(Stream* stream, RWall* wall, RSector* sector);
	void level_serializeTextureList(Stream* stream);

	/////////////////////////////////////////////
	// Implementation
	/////////////////////////////////////////////
	void level_clearData()
	{
		s_levelState = { 0 };
		s_levelIntState = { 0 };

		s_levelState.controlSector = (RSector*)level_alloc(sizeof(RSector));
		sector_clear(s_levelState.controlSector);

		objData_clear();
	}

	void level_serializeFixupMirrors()
	{
		RSector* sector = s_levelState.sectors;
		for (s32 s = 0; s < s_levelState.sectorCount; s++, sector++)
		{
			s32 wallCount = sector->wallCount;
			RWall* wall = sector->walls;
			for (s32 w = 0; w < wallCount; w++, wall++)
			{
				if (wall->nextSector && wall->mirror >= 0)
				{
					wall->mirrorWall = &wall->nextSector->walls[wall->mirror];
				}
			}
		}
	}
		
	void level_serialize(Stream* stream)
	{
		if (serialization_getMode() == SMODE_READ)
		{
			level_clearData();
		}
		SERIALIZE_VERSION(LevelState_CurVersion);
		
		SERIALIZE(LevelState_InitVersion, s_levelState.minLayer, 0);
		SERIALIZE(LevelState_InitVersion, s_levelState.maxLayer, 0);
		SERIALIZE(LevelState_InitVersion, s_levelState.secretCount, 0);
		SERIALIZE(LevelState_InitVersion, s_levelState.sectorCount, 0);
		SERIALIZE(LevelState_InitVersion, s_levelState.parallax0, 0);
		SERIALIZE(LevelState_InitVersion, s_levelState.parallax1, 0);

		// This is needed because level textures are pointers to the list itself, rather than the texture.
		level_serializeTextureList(stream);

		if (serialization_getMode() == SMODE_READ)
		{
			s_levelState.sectors = (RSector*)level_alloc(sizeof(RSector) * s_levelState.sectorCount);
		}
		RSector* sector = s_levelState.sectors;
		for (u32 s = 0; s < s_levelState.sectorCount; s++, sector++)
		{
			level_serializeSector(stream, sector);
		}

		serialization_serializeSectorPtr(stream, LevelState_InitVersion, s_levelState.bossSector);
		serialization_serializeSectorPtr(stream, LevelState_InitVersion, s_levelState.mohcSector);
		serialization_serializeSectorPtr(stream, LevelState_InitVersion, s_levelState.completeSector);

		// The control sector is just a dummy sector, no need to save anything.
		// Sound emitter isn't actually used.

		if (serialization_getMode() == SMODE_WRITE)
		{
			allocator_saveIter(s_levelState.safeLoc);
				s32 safeCount = allocator_getCount(s_levelState.safeLoc);
				SERIALIZE(LevelState_InitVersion, safeCount, 0);
				Safe* safe = (Safe*)allocator_getHead(s_levelState.safeLoc);
				while (safe)
				{
					level_serializeSafe(stream, safe);
					safe = (Safe*)allocator_getNext(s_levelState.safeLoc);
				}
			allocator_restoreIter(s_levelState.safeLoc);

			allocator_saveIter(s_levelState.ambientSounds);
				s32 ambientSoundCount = allocator_getCount(s_levelState.ambientSounds);
				SERIALIZE(LevelState_InitVersion, ambientSoundCount, 0);
				AmbientSound* sound = (AmbientSound*)allocator_getHead(s_levelState.ambientSounds);
				while (sound)
				{
					level_serializeAmbientSound(stream, sound);
					sound = (AmbientSound*)allocator_getNext(s_levelState.ambientSounds);
				}
			allocator_restoreIter(s_levelState.ambientSounds);
		}
		else
		{
			s32 safeCount;
			SERIALIZE(LevelState_InitVersion, safeCount, 0);
			s_levelState.safeLoc = nullptr;
			if (safeCount)
			{
				s_levelState.safeLoc = allocator_create(sizeof(Safe));
				for (s32 s = 0; s < safeCount; s++)
				{
					Safe* safe = (Safe*)allocator_newItem(s_levelState.safeLoc);
					level_serializeSafe(stream, safe);
				}
			}

			s32 ambientSoundCount;
			SERIALIZE(LevelState_InitVersion, ambientSoundCount, 0);
			s_levelState.ambientSounds = nullptr;
			if (ambientSoundCount)
			{
				s_levelState.ambientSounds = allocator_create(sizeof(AmbientSound));
				for (s32 s = 0; s < ambientSoundCount; s++)
				{
					AmbientSound* sound = (AmbientSound*)allocator_newItem(s_levelState.ambientSounds);
					level_serializeAmbientSound(stream, sound);
				}
			}
		}
		level_serializeFixupMirrors();

		// Serialize objects.
		objData_serialize(stream);
	}
		
	/////////////////////////////////////////////
	// Internal - Serialize
	/////////////////////////////////////////////
	void level_serializeTextureList(Stream* stream)
	{
		SERIALIZE(LevelState_InitVersion, s_levelState.textureCount, 0);
		if (serialization_getMode() == SMODE_READ)
		{
			s_levelState.textures = (TextureData**)level_alloc(s_levelState.textureCount * sizeof(TextureData**));
		}
		for (s32 i = 0; i < s_levelState.textureCount; i++)
		{
			serialization_serializeTexturePtr(stream, LevelState_InitVersion, s_levelState.textures[i]);
		}
	}

	void level_serializeTexturePointer(Stream* stream, TextureData**& texData)
	{
		s32 index = -1;
		if (serialization_getMode() == SMODE_WRITE && texData)
		{
			ptrdiff_t offset = (ptrdiff_t(texData) - ptrdiff_t(s_levelState.textures)) / (ptrdiff_t)sizeof(TextureData**);
			if (offset < 0 || offset >= s_levelState.textureCount)
			{
				// This is probably a sign texture - so let the INF System handle it.
				offset = -1;
			}
			index = s32(offset);
			assert(index < 0 || s_levelState.textures[index] == *texData);
		}
		SERIALIZE(LevelState_InitVersion, index, -1);

		if (serialization_getMode() == SMODE_READ)
		{
			assert(index >= -1 && index < s_levelState.textureCount);
			texData = (index < 0) ? nullptr : &s_levelState.textures[index];
		}
	}

	void level_serializeSector(Stream* stream, RSector* sector)
	{
		if (serialization_getMode() == SMODE_READ)
		{
			sector->self = sector;
		}
		SERIALIZE(LevelState_InitVersion, sector->id, 0);
		SERIALIZE(LevelState_InitVersion, sector->index, 0);
		SERIALIZE(LevelState_InitVersion, sector->prevDrawFrame, 0);
		SERIALIZE(LevelState_InitVersion, sector->prevDrawFrame2, 0);

		SERIALIZE(LevelState_InitVersion, sector->vertexCount, 0);
		const size_t vtxSize = sector->vertexCount * sizeof(vec2_fixed);
		if (serialization_getMode() == SMODE_READ)
		{
			sector->verticesWS = (vec2_fixed*)level_alloc(vtxSize);
			sector->verticesVS = (vec2_fixed*)level_alloc(vtxSize);
		}
		SERIALIZE_BUF(LevelState_InitVersion, sector->verticesWS, u32(vtxSize));
		// view space vertices don't need to be serialized.

		SERIALIZE(LevelState_InitVersion, sector->wallCount, 0);
		if (serialization_getMode() == SMODE_READ)
		{
			sector->walls = (RWall*)level_alloc(sector->wallCount * sizeof(RWall));
		}
		RWall* wall = sector->walls;
		for (s32 w = 0; w < sector->wallCount; w++, wall++)
		{
			level_serializeWall(stream, wall, sector);
		}

		SERIALIZE(LevelState_InitVersion, sector->floorHeight, 0);
		SERIALIZE(LevelState_InitVersion, sector->ceilingHeight, 0);
		SERIALIZE(LevelState_InitVersion, sector->secHeight, 0);
		SERIALIZE(LevelState_InitVersion, sector->ambient, 0);

		SERIALIZE(LevelState_InitVersion, sector->colFloorHeight, 0);
		SERIALIZE(LevelState_InitVersion, sector->colCeilHeight, 0);
		SERIALIZE(LevelState_InitVersion, sector->colSecHeight, 0);
		SERIALIZE(LevelState_InitVersion, sector->colMinHeight, 0);

		// infLink will be set when the INF system is serialized.
		if (serialization_getMode() == SMODE_READ)
		{
			sector->infLink = nullptr;
		}

		level_serializeTexturePointer(stream, sector->floorTex);
		level_serializeTexturePointer(stream, sector->ceilTex);

		const vec2_fixed def = { 0,0 };
		SERIALIZE(LevelState_InitVersion, sector->floorOffset, def);
		SERIALIZE(LevelState_InitVersion, sector->ceilOffset, def);

		// Objects are handled seperately and added to the sector later, so just initialize the object data.
		if (serialization_getMode() == SMODE_READ)
		{
			sector->objectCount = 0;
			sector->objectCapacity = 0;
			sector->objectList = nullptr;
		}

		SERIALIZE(LevelState_InitVersion, sector->collisionFrame, 0);
		SERIALIZE(LevelState_InitVersion, sector->startWall, 0);
		SERIALIZE(LevelState_InitVersion, sector->drawWallCnt, 0);
		SERIALIZE(LevelState_InitVersion, sector->flags1, 0);
		SERIALIZE(LevelState_InitVersion, sector->flags2, 0);
		SERIALIZE(LevelState_InitVersion, sector->flags3, 0);
		SERIALIZE(LevelState_InitVersion, sector->layer, 0);
		SERIALIZE(LevelState_InitVersion, sector->boundsMin, def);
		SERIALIZE(LevelState_InitVersion, sector->boundsMax, def);

		// dirty flags will be set on deserialization.
		if (serialization_getMode() == SMODE_READ)
		{
			sector->dirtyFlags = SDF_ALL;
		}
	}
		
	void level_serializeSafe(Stream* stream, Safe* safe)
	{
		serialization_serializeSectorPtr(stream, LevelState_InitVersion, safe->sector);
		SERIALIZE(LevelState_InitVersion, safe->x, 0);
		SERIALIZE(LevelState_InitVersion, safe->z, 0);
		SERIALIZE(LevelState_InitVersion, safe->yaw, 0);
		SERIALIZE(LevelState_InitVersion, safe->pad, 0);	// for padding.
	}

	void level_serializeAmbientSound(Stream* stream, AmbientSound* sound)
	{
		serialization_serializeDfSound(stream, LevelState_InitVersion, &sound->soundId);
		if (serialization_getMode() == SMODE_READ)
		{
			sound->instanceId = 0;
		}
		const vec3_fixed def = { 0,0,0 };
		SERIALIZE(LevelState_InitVersion, sound->pos, def);
	}
				
	void level_serializeWall(Stream* stream, RWall* wall, RSector* sector)
	{
		SERIALIZE(LevelState_InitVersion, wall->id, 0);
		SERIALIZE(LevelState_InitVersion, wall->seen, 0);
		SERIALIZE(LevelState_InitVersion, wall->visible, 0);

		serialization_serializeSectorPtr(stream, LevelState_InitVersion, wall->sector);
		serialization_serializeSectorPtr(stream, LevelState_InitVersion, wall->nextSector);

		// mirrorWall will be computed from mirror.
		SERIALIZE(LevelState_InitVersion, wall->drawFlags, 0);
		SERIALIZE(LevelState_InitVersion, wall->mirror, -1);
		if (serialization_getMode() == SMODE_READ)
		{
			wall->mirrorWall = nullptr;
		}
		
		// worldspace vertices.
		s32 i0, i1;
		if (serialization_getMode() == SMODE_WRITE)
		{
			i0 = PTR_INDEX_S32(wall->w0, sector->verticesWS, sizeof(vec2_fixed));
			i1 = PTR_INDEX_S32(wall->w1, sector->verticesWS, sizeof(vec2_fixed));
			SERIALIZE(LevelState_InitVersion, i0, 0);
			SERIALIZE(LevelState_InitVersion, i1, 0);
			// viewspace vertices need not be saved.
		}
		else
		{
			SERIALIZE(LevelState_InitVersion, i0, 0);
			SERIALIZE(LevelState_InitVersion, i1, 0);
			wall->w0 = &sector->verticesWS[i0];
			wall->w1 = &sector->verticesWS[i1];
			wall->v0 = &sector->verticesVS[i0];
			wall->v1 = &sector->verticesVS[i1];
		}

		level_serializeTexturePointer(stream, wall->topTex);
		level_serializeTexturePointer(stream, wall->midTex);
		level_serializeTexturePointer(stream, wall->botTex);
		level_serializeTexturePointer(stream, wall->signTex);

		SERIALIZE(LevelState_InitVersion, wall->texelLength, 0);
		SERIALIZE(LevelState_InitVersion, wall->topTexelHeight, 0);
		SERIALIZE(LevelState_InitVersion, wall->midTexelHeight, 0);
		SERIALIZE(LevelState_InitVersion, wall->botTexelHeight, 0);

		const vec2_fixed def = { 0, 0 };
		SERIALIZE(LevelState_InitVersion, wall->topOffset, def);
		SERIALIZE(LevelState_InitVersion, wall->midOffset, def);
		SERIALIZE(LevelState_InitVersion, wall->botOffset, def);
		SERIALIZE(LevelState_InitVersion, wall->signOffset, def);

		SERIALIZE(LevelState_InitVersion, wall->wallDir, def);
		SERIALIZE(LevelState_InitVersion, wall->length, 0);
		SERIALIZE(LevelState_InitVersion, wall->collisionFrame, 0);
		SERIALIZE(LevelState_InitVersion, wall->drawFrame, 0);

		// infLink will be filled in when serializing the INF system.
		if (serialization_getMode() == SMODE_READ)
		{
			wall->infLink = nullptr;
		}

		SERIALIZE(LevelState_InitVersion, wall->flags1, 0);
		SERIALIZE(LevelState_InitVersion, wall->flags2, 0);
		SERIALIZE(LevelState_InitVersion, wall->flags3, 0);

		SERIALIZE(LevelState_InitVersion, wall->worldPos0, def);
		SERIALIZE(LevelState_InitVersion, wall->wallLight, 0);
		SERIALIZE(LevelState_InitVersion, wall->angle, 0);
	}
}