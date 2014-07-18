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

#ifndef _LOOTMGR_H
#define _LOOTMGR_H

struct ItemPrototype;
class MapMgr;
class LootRoll : public EventableObject
{
public:
	LootRoll(uint32 timer, uint32 groupcount, uint64 guid, uint32 slotid, uint32 itemid, uint32 itemunk1, uint32 itemunk2, MapMgr * mgr);
	~LootRoll();
	void PlayerRolled(Player *player, uint8 choice);
	void Finalize();

	int32 event_GetInstanceID();

private:
	std::map<uint32, uint32> m_NeedRolls;
	std::map<uint32, uint32> m_GreedRolls;
	set<uint32> m_passRolls;
	uint32 _groupcount;
	uint32 _slotid;
	uint32 _itemid;
	uint32 random_suffix_factor;
	uint32 random_suffix_id;
	uint8 unk_33_1;	//since 3.3. not really sure it is a mask :(
	uint32 _remaining;
	uint64 _guid;
	MapMgr * _mgr;
};

typedef vector<pair<RandomProps*, float> > RandomPropertyVector;
typedef vector<pair<ItemRandomSuffixEntry*, float> > RandomSuffixVector;

typedef struct
{
	ItemPrototype * itemproto;
	uint32 displayid;
}_LootItem;

typedef std::set<uint32> LooterSet;

struct __LootItem
{
	_LootItem item;
	uint32 iItemsCount;
	RandomProps * iRandomProperty;
	ItemRandomSuffixEntry * iRandomSuffix;
	LootRoll *roll;		//rolls are born to be deleted after 
	bool passed;
	LooterSet has_looted;
	uint32 ffa_loot;
};


typedef struct
{
	_LootItem item;
	float chance;
	float chance2;
	uint32 mincount;
	uint32 maxcount;
	uint32 ffa_loot;
}StoreLootItem;


typedef struct 
{
	uint32 count;			//number of items in this list
	uint32 mincount;		//list needs to drop at least N items
	uint32 maxcount;		//list must not drop more then M items
	StoreLootItem *items;	//vector of items
}StoreLootList;

typedef struct 
{
	uint32 count;			//number of lists
	StoreLootList *lists;	//vector of lists
}StoreLootLootList;

typedef struct
{
	uint32 CurrencyType;
	uint32 CurrencyCount;
	LooterSet has_looted;
}__LootCurrency;

typedef struct
{
	std::vector<__LootItem> items;
	std::vector<__LootCurrency> currencies;
	uint32 gold;
	LooterSet looters;
}Loot;

struct tempy
{
	uint32 itemid;
	float chance;
	float chance_2;
	uint32 mincount;
	uint32 maxcount;
	uint32 ffa_loot;
};

//////////////////////////////////////////////////////////////////////////////////////////


typedef HM_NAMESPACE::hash_map<uint32, StoreLootList > LootStore;  

#define PARTY_LOOT_FFA	  0
#define PARTY_LOOT_MASTER   2
#define PARTY_LOOT_RR	   1
#define PARTY_LOOT_NBG	  4
#define PARTY_LOOT_GROUP	3


#define LOOT_ROLL_TYPE_NEED				1
#define LOOT_ROLL_TYPE_GREED			2
#define LOOT_ROLL_TYPE_DISENCHANT		3
#define LOOT_ROLL_TYPE_PASS_ON_LOOT		4

struct LootStoreStruct;

class SERVER_DECL LootMgr : public Singleton < LootMgr >
{
public:
	LootMgr();
	~LootMgr();

	void AddLoot(Loot * loot, uint32 itemid, uint32 mincount, uint32 maxcount, uint32 ffa_loot);

	void FillCreatureLoot(Loot * loot,uint32 loot_id, uint32 instance_difficulty);
	void FillGOLoot(Loot * loot,uint32 loot_id, uint32 instance_difficulty);
	void FillItemLoot(Loot *loot, uint32 loot_id);
	void FillFishingLoot(Loot * loot,uint32 loot_id);
//	void FillSkinningLoot(Loot * loot,uint32 loot_id);
//	void FillPickpocketingLoot(Loot *loot, uint32 loot_id);
//	void FillDisenchantingLoot(Loot *loot, uint32 loot_id);
//	void FillProspectingLoot(Loot *loot, uint32 loot_id);
//	void FillMillingLoot( Loot *loot, uint32 loot_id );
	//loot list is passed directly by object !
	void FillObjectLoot(Object *obj,LootStoreStruct *ls );
	bool FillObjectLoot(Object *obj,LootStoreStruct *ls, LootStoreStruct *reference );
	bool FillObjectLoot(Object *obj,SimpleVect<LootStoreStruct*> *source_list, LootStoreStruct *reference );
	bool FillObjectLoot(Object *obj,SimpleVect< SimpleVect<LootStoreStruct*>* > *source_list, LootStoreStruct *reference=NULL );


	bool CanGODrop(uint32 LootId,uint32 itemid);
	bool IsPickpocketable(uint32 creatureId);
//	bool IsSkinnable(uint32 creatureId);
	bool IsFishable(uint32 zoneid);

	void LoadLoot();
	void LoadLootProp();
	
	LootStore	CreatureLoot;
	LootStore	FishingLoot;
	LootStore	SkinningLoot;
	LootStore	GOLoot;
	LootStore	ItemLoot;
//	LootStore	ProspectingLoot;
//	LootStore   MillingLoot;
//	LootStore	DisenchantingLoot;
	LootStore	PickpocketingLoot;
	std::map<uint32, std::set<uint32> > quest_loot_go;

	RandomProps * GetRandomProperties(ItemPrototype * proto);
	ItemRandomSuffixEntry * GetRandomSuffix(ItemPrototype * proto);

	bool is_loading;

private:
	void LoadLootTables(const char * szTableName,LootStore * LootTable);
	void PushLoot(StoreLootList *list,Loot * loot, uint32 instance_difficulty);
	
	map<uint32, RandomPropertyVector> _randomprops;
	map<uint32, RandomSuffixVector> _randomsuffix;
};

#define lootmgr LootMgr::getSingleton()

#endif
