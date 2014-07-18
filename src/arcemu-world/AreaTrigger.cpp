/*
 * ArcEmu MMORPG Server
 * Copyright (C) 2005-2007 Ascent Team <http://www.ascentemu.com/>
 * Copyright (C) 2008 <http://www.ArcEmu.org/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "StdAfx.h"
void WorldSession::HandleAreaTriggerOpcode(WorldPacket & recv_data)
{
	 if(!_player->IsInWorld()) return;
	CHECK_PACKET_SIZE(recv_data, 4);
	uint32 id ;
	recv_data >> id;
	_HandleAreaTriggerOpcode(id);
}

enum AreaTriggerFailures
{
	AREA_TRIGGER_FAILURE_OK				= 0,
	AREA_TRIGGER_FAILURE_UNAVAILABLE	= 1,
	AREA_TRIGGER_FAILURE_NO_BC			= 2,
	AREA_TRIGGER_FAILURE_NO_HEROIC		= 3,
	AREA_TRIGGER_FAILURE_NO_RAID		= 4,
	AREA_TRIGGER_FAILURE_NO_ATTUNE_Q	= 5,
	AREA_TRIGGER_FAILURE_NO_ATTUNE_I	= 6,
	AREA_TRIGGER_FAILURE_LEVEL			= 7,
	AREA_TRIGGER_FAILURE_NO_GROUP		= 8,
	AREA_TRIGGER_FAILURE_NO_KEY         = 9,
	AREA_TRIGGER_FAILURE_LEVEL_HEROIC	= 9,
	AREA_TRIGGER_FAILURE_NO_CHECK		= 10,
	AREA_TRIGGER_FAILURE_NEED_WOTLK		= 11,
};

const char * AreaTriggerFailureMessages[] = {
	"-",
	"This instance is unavailable",
	"You must have The Burning Crusade Expansion to access this content.",
	"Heroic mode unavailable for this instance.",
	"You must be in a raid group to pass through here.",
	"You do not have the required attunement to pass through here.", //TODO: Replace attunment with real itemname
	"You do not have the required attunement to pass through here.", //TODO: Replace attunment with real itemname
	"You must be at least level %u to pass through here.",
	"You must be in a party to pass through here.",
	"You do not have the required attunement to pass through here.", //TODO: Replace attunment with real itemname
	"You must be level 70 to enter heroic mode.",
};

uint32 CheckTriggerPrerequsites(AreaTrigger * pAreaTrigger, WorldSession * pSession, Player * pPlayer, MapInfo * pMapInfo)
{
	if(!pMapInfo || !pMapInfo->HasFlag(WMI_INSTANCE_ENABLED))
		return AREA_TRIGGER_FAILURE_UNAVAILABLE;

//	if(!pSession->HasFlag(ACCOUNT_FLAG_XPACK_01 | ACCOUNT_FLAG_XPACK_02) && pMapInfo->HasFlag(WMI_INSTANCE_XPACK_01))
//		return AREA_TRIGGER_FAILURE_NO_BC;

//	if(!pSession->HasFlag( ACCOUNT_FLAG_XPACK_02) && pMapInfo->HasFlag(WMI_INSTANCE_XPACK_02) )
//		return AREA_TRIGGER_FAILURE_NEED_WOTLK;

	// These can be overridden by cheats/GM
	if(pPlayer->TriggerpassCheat)
		return AREA_TRIGGER_FAILURE_OK;

	if(pAreaTrigger->required_level && pPlayer->getLevel() < pAreaTrigger->required_level)
		return AREA_TRIGGER_FAILURE_LEVEL;

//	if( IS_HEROIC_INSTANCE( pPlayer->GetInstanceDifficulty() ) && pMapInfo->type != INSTANCE_MULTIMODE && pMapInfo->type != INSTANCE_NULL)
//		return AREA_TRIGGER_FAILURE_NO_HEROIC;
	uint32 difficulty_mask = (1 << (pPlayer->GetInstanceDifficulty()+2)) | INSTANCE_FLAG_STATIC_MAP | INSTANCE_FLAG_PVP;
	if( (pMapInfo->type_flags & difficulty_mask)==0)
	{
		pPlayer->BroadcastMessage("This map does not have '%s' difficulty mode. Please choose another mode",GetDifficultyString( pPlayer->GetInstanceDifficulty() ) );
		return AREA_TRIGGER_FAILURE_UNAVAILABLE;
	}

	if( IS_MAP_RAID( pMapInfo ) && IS_MAP_DUNGEON( pMapInfo ) == 0 && (!pPlayer->GetGroup() || (pPlayer->GetGroup() && pPlayer->GetGroup()->GetGroupType() != GROUP_TYPE_RAID)))
		return AREA_TRIGGER_FAILURE_NO_RAID;

	uint32 player_instance_difficultie_mode = pPlayer->GetInstanceDifficulty();
	if( IS_RAID_INSTANCE_DIFFICULTIE( player_instance_difficultie_mode ) && !pPlayer->GetGroup())
		return AREA_TRIGGER_FAILURE_NO_GROUP;

	if(pMapInfo && pMapInfo->required_quest && !pPlayer->HasFinishedQuest(pMapInfo->required_quest))
		return AREA_TRIGGER_FAILURE_NO_ATTUNE_Q;

	if(pMapInfo && pMapInfo->required_item && !pPlayer->GetItemInterface()->GetItemCount(pMapInfo->required_item, true))
		return AREA_TRIGGER_FAILURE_NO_ATTUNE_I;

	if ( IS_HEROIC_INSTANCE_DIFFICULTIE( player_instance_difficultie_mode )  && 
		 IS_MAP_INSTANCE( pMapInfo) && 
		(		(pMapInfo->heroic_key_1 != 0 && !pPlayer->GetItemInterface()->GetItemCount(pMapInfo->heroic_key_1, false)) 
			||  (pMapInfo->heroic_key_2 != 0 && !pPlayer->GetItemInterface()->GetItemCount(pMapInfo->heroic_key_2, false))
			)
		)
		return AREA_TRIGGER_FAILURE_NO_KEY;

	if( pPlayer->getLevel() < pMapInfo->minlevel && IS_HEROIC_INSTANCE_DIFFICULTIE( player_instance_difficultie_mode ) && IS_MAP_INSTANCE( pMapInfo ) )
		return AREA_TRIGGER_FAILURE_LEVEL_HEROIC;

	uint32 required_item_level = 0;
	if( pMapInfo->required_heroic_avg_item_level && IS_HEROIC_INSTANCE_DIFFICULTIE( player_instance_difficultie_mode ) )
		required_item_level = pMapInfo->required_heroic_avg_item_level;
	if( pMapInfo->required_avg_item_level && IS_NORMAL_INSTANCE_DIFFICULTIE( player_instance_difficultie_mode ) )
		required_item_level = pMapInfo->required_avg_item_level;
	if( required_item_level )
	{
		//not sure what is the proper way to check for avg item score :(
		uint32 avg_score = pPlayer->GetItemAvgLevelBlizzlike();
		if( avg_score < required_item_level )
		{
			pPlayer->BroadcastMessage( "This instance requires an avg item score %d, your score is considered %d ( top X gear wearing )",required_item_level,avg_score);
			return AREA_TRIGGER_FAILURE_LEVEL;
		}
	}
	return AREA_TRIGGER_FAILURE_OK;
}

void WorldSession::_HandleAreaTriggerOpcode(uint32 id)
{		
	sLog.outDebug("AreaTrigger: %u", id);

	// Are we REALLY here?
	if( !_player->IsInWorld() )
		return;

    // Search quest log, find any exploration quests
	sQuestMgr.OnPlayerExploreArea(GetPlayer(),id);

	AreaTriggerEntry* entry = dbcAreaTrigger.LookupEntry(id);
	AreaTrigger* pAreaTrigger = AreaTriggerStorage.LookupEntry(id);

	if( entry == NULL )
	{
		sLog.outDebug("Missing AreaTrigger: %u", id);
		return;
	}

	sHookInterface.OnAreaTrigger(GetPlayer(), id);
	CALL_INSTANCE_SCRIPT_EVENT( GetPlayer()->GetMapMgr(), OnAreaTrigger )( GetPlayer(), id );

	if( _player->GetSession()->CanUseCommand('z') )
		sChatHandler.BlueSystemMessage( this, "[%sSystem%s] |rEntered areatrigger: %s%u. (%s)", MSG_COLOR_WHITE, MSG_COLOR_LIGHTBLUE, MSG_COLOR_SUBWHITE, id, pAreaTrigger ? pAreaTrigger->Name : "Unknown name" );

	// if in BG handle is triggers
	if( _player->m_bg )
	{
		_player->m_bg->HookOnAreaTrigger(_player, id);
		return;
	}

	// Hook for Scripted Areatriggers
	_player->GetMapMgr()->HookOnAreaTrigger(_player, id);

	if (pAreaTrigger == NULL) 
	{
		sLog.outDebug("Missing areatrigger info. Exiting");
		return;
	}

	switch(pAreaTrigger->Type)
	{
	case ATTYPE_INSTANCE:
		{
			if(GetPlayer()->GetPlayerStatus() != TRANSFER_PENDING) //only ports if player is out of pendings
			{
				uint32 reason = CheckTriggerPrerequsites(pAreaTrigger, this, _player, WorldMapInfoStorage.LookupEntry(pAreaTrigger->Mapid));
				if(reason != AREA_TRIGGER_FAILURE_OK)
				{
					const char * pReason = AreaTriggerFailureMessages[reason];
//					char msg[200];
//					sStackWorldPacket( data, SMSG_AREA_TRIGGER_MESSAGE, 250);
//					data << uint32(0);
                    
					switch (reason)
					{
					case AREA_TRIGGER_FAILURE_LEVEL:
//						snprintf(msg,200,pReason,pAreaTrigger->required_level);
//						data << msg;
						_player->BroadcastMessage("You need to have at least level %u ",pAreaTrigger->required_level);
						break;
					case AREA_TRIGGER_FAILURE_NO_ATTUNE_I:
						{
							MapInfo * pMi = WorldMapInfoStorage.LookupEntry(pAreaTrigger->Mapid);
							ItemPrototype * pItem = ItemPrototypeStorage.LookupEntry(pMi->required_item);
							if(pItem)
//								snprintf(msg,200,"You must have the item, `%s` to pass through here.",pItem->Name1);
								_player->BroadcastMessage("You must have the item, `%s` to pass through here.",pItem->Name1);
							else
//								snprintf(msg,200,"You must have the item, UNKNOWN to pass through here.");
								_player->BroadcastMessage("You must have the item, UNKNOWN to pass through here ");

//							data << msg;
						}break;
					case AREA_TRIGGER_FAILURE_NO_ATTUNE_Q:
						{
							MapInfo * pMi = WorldMapInfoStorage.LookupEntry(pAreaTrigger->Mapid);
							Quest * pQuest = QuestStorage.LookupEntry(pMi->required_quest);
							if(pQuest)
//								snprintf(msg,200,"You must have finished the quest, `%s` to pass through here.",pQuest->title);
								_player->BroadcastMessage("You must have finished the quest, `%s` to pass through here.",pQuest->title);
							else
//								snprintf(msg,200,"You must have finished the quest, UNKNOWN to pass through here.");
								_player->BroadcastMessage("You must have finished the quest, UNKNOWN to pass through here.");

//							data << msg;
						}break;
					case AREA_TRIGGER_FAILURE_NO_KEY:
						{
							MapInfo * pMi = WorldMapInfoStorage.LookupEntry(pAreaTrigger->Mapid);
							ItemPrototype * pItem = ItemPrototypeStorage.LookupEntry(pMi->heroic_key_1);
							if(pItem)
//								snprintf(msg,200,"You must have the item, `%s` to pass through here.",pItem->Name1);
								_player->BroadcastMessage("You must have the item, `%s` to pass through here.",pItem->Name1);
							else
//								snprintf(msg,200,"You must have the item, UNKNOWN to pass through here.");
								_player->BroadcastMessage("You must have the item, UNKNOWN to pass through here.");

//							data << msg;
						}break;
					default:
						_player->BroadcastMessage(pReason);
//						data << pReason;
						break;
					}

//					data << uint8(0);
//					SendPacket(&data);
					return;
				}

				GetPlayer()->SaveEntryPoint(pAreaTrigger->Mapid);
				GetPlayer()->SafeTeleport(pAreaTrigger->Mapid, 0, LocationVector(pAreaTrigger->x, pAreaTrigger->y, pAreaTrigger->z, pAreaTrigger->o));
			}
		}break;
	case ATTYPE_QUESTTRIGGER:
		{

		}break;
	case ATTYPE_INN:
		{
			// Inn
			if (!GetPlayer()->m_isResting) 
				GetPlayer()->ApplyPlayerRestState(true);
		}break;
	case ATTYPE_TELEPORT:
		{
			if(GetPlayer()->GetPlayerStatus() != TRANSFER_PENDING) //only ports if player is out of pendings
			{
				GetPlayer()->SaveEntryPoint(pAreaTrigger->Mapid);
				GetPlayer()->SafeTeleport(pAreaTrigger->Mapid, 0, LocationVector(pAreaTrigger->x, pAreaTrigger->y, pAreaTrigger->z, pAreaTrigger->o));
			}
		}break;
	default:break;
	}
}
