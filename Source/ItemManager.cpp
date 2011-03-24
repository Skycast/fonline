#include "StdAfx.h"
#include "ItemManager.h"
#include "Log.h"
#include "Text.h"
#include "ConstantsManager.h"

#ifdef FONLINE_SERVER
#include "Critter.h"
#include "Map.h"
#include "CritterManager.h"
#include "MapManager.h"
#include "Script.h"
#endif

#ifdef FONLINE_MAPPER
#include "Script.h"
#endif

ItemManager ItemMngr;

bool ItemManager::Init()
{
	WriteLog(NULL,"Item manager initialization...\n");

	if(IsInit())
	{
		WriteLog(NULL,"already init.\n");
		return true;
	}

	Clear();
	ClearProtos();

	isActive=true;
	WriteLog(NULL,"Item manager initialization complete.\n");
	return true;
}

void ItemManager::Finish()
{
	WriteLog(NULL,"Item manager finish...\n");

	if(!isActive)
	{
		WriteLog(NULL,"already finish or not init.\n");
		return;
	}

#ifdef FONLINE_SERVER
	ItemGarbager();

	ItemPtrMap items=gameItems;
	for(ItemPtrMapIt it=items.begin(),end=items.end();it!=end;++it)
	{
		Item* item=(*it).second;
		item->EventFinish(false);
		item->Release();
	}
	gameItems.clear();
#endif

	Clear();
	ClearProtos();

	isActive=false;
	WriteLog(NULL,"Item manager finish complete.\n");
}

void ItemManager::Clear()
{
#ifdef FONLINE_SERVER
	for(ItemPtrMapIt it=gameItems.begin(),end=gameItems.end();it!=end;++it)
		SAFEREL((*it).second);
	gameItems.clear();
	radioItems.clear();
	itemToDelete.clear();
	itemToDeleteCount.clear();
	lastItemId=0;
#endif

	for(int i=0;i<MAX_ITEM_PROTOTYPES;i++) itemCount[i]=0;
}

#if defined(FONLINE_SERVER) || defined(FONLINE_OBJECT_EDITOR) || defined(FONLINE_MAPPER)

template<class T>
T ResolveProtoValue(const char* str)
{
	if(Str::IsNumber(str)) return (T)_atoi64(str);
	else if(strstr(str,"\\") || strstr(str,"/")) return (T)Str::GetHash(str);
	return (T)ConstantsManager::GetDefineValue(str);
}

bool ItemManager::LoadProtos()
{
	WriteLog(NULL,"Loading items prototypes...\n");

	FileManager fm;
	if(!fm.LoadFile("items.lst",PT_SERVER_PRO_ITEMS))
	{
		WriteLog(NULL,"Can't open \"items.lst\".\n");
		return false;
	}

	char buf[256];
	StrVec fnames;
	int count=0;
	while(fm.GetLine(buf,sizeof(buf)))
	{
		fnames.push_back(string(buf));
		count++;
	}
	fm.UnloadFile();

	uint loaded=0;
	ProtoItemVec item_protos;
	ClearProtos();
	for(int i=0;i<count;i++)
	{
		char fname[MAX_FOPATH];
		StringCopy(fname,FileManager::GetFullPath(fnames[i].c_str(),PT_SERVER_PRO_ITEMS));

		char collection_name[MAX_FOPATH];
		StringFormat(collection_name,"%03d - %s",i+1,fnames[i].c_str());
		FileManager::EraseExtension(collection_name);

		if(LoadProtos(item_protos,fname))
		{
			ParseProtos(item_protos,collection_name);
			loaded+=item_protos.size();
		}
	}

	WriteLog(NULL,"Items prototypes successfully loaded, count<%u>.\n",loaded);
	return true;
}

bool ItemManager::LoadProtos(ProtoItemVec& protos, const char* fname)
{
	protos.clear();

	IniParser fopro;
	if(!fopro.LoadFile(fname,-1))
	{
		WriteLog(_FUNC_," - File<%s> not found.\n",fname);
		return false;
	}

	ProtoItem proto_item;
	char svalue[MAX_FOTEXT];
	char name_[MAX_FOTEXT]; // For deprecated conversion
	while(true)
	{
		if(!fopro.GotoNextApp("Proto")) break;

		proto_item.Clear();
		bool deprecated=fopro.GetStr("Proto","Pid","",svalue); // Detect deprecated by 'Pid', instead new 'ProtoId'
		asIScriptEngine* engine=Script::GetEngine();
		asIObjectType* ot=engine->GetObjectTypeById(engine->GetTypeIdByDecl("ProtoItem"));

		for(int i=0,j=ot->GetPropertyCount();i<j;i++)
		{
			const char* name;
			int type_id;
			int offset;
			ot->GetProperty(i,&name,&type_id,NULL,&offset);

			if(!fopro.GetStr("Proto",name,"",svalue))
			{
				if(deprecated)
				{
					// Convert '_' to '.' and try again
					StringCopy(name_,name);
					bool swapped=false;
					char* str=name_;
					while(*str)
					{
						if(*str=='_')
						{
							*str='.';
							swapped=true;
							break;
						}
						++str;
					}

					if(!swapped || !fopro.GetStr("Proto",name_,"",svalue))
					{
						if(!strcmp(name,"ProtoId")) StringCopy(name_,"Pid");
						else if(!strcmp(name,"PicInv") || !strcmp(name_,"PicMap")) StringAppend(name_,"Name");
						else if(!strcmp(name,"Armor_CrTypeMale")) StringCopy(name_,"Armor.Anim0Male");
						else if(!strcmp(name,"Armor_CrTypeFemale")) StringCopy(name_,"Armor.Anim0Female");
						else if(!strcmp(name,"Weapon_CriticalFailture")) StringCopy(name_,"Weapon.CrFailture");
						else if(!strcmp(name,"Weapon_MinStrength")) StringCopy(name_,"Weapon.MinSt");
						else if(!strcmp(name,"Stackable")) StringCopy(name_,"Weapon.NoWear");
						else if(!strcmp(name,"Weapon_ActiveUses")) StringCopy(name_,"Weapon.CountAttack");
						else if(!strcmp(name,"Weapon_PicUse_0")) StringCopy(name_,"Weapon.PicName_0");
						else if(!strcmp(name,"Weapon_PicUse_1")) StringCopy(name_,"Weapon.PicName_1");
						else if(!strcmp(name,"Weapon_PicUse_2")) StringCopy(name_,"Weapon.PicName_2");
						else if(!strcmp(name,"Weapon_ApCost_0")) StringCopy(name_,"Weapon.Time_0");
						else if(!strcmp(name,"Weapon_ApCost_1")) StringCopy(name_,"Weapon.Time_1");
						else if(!strcmp(name,"Weapon_ApCost_2")) StringCopy(name_,"Weapon.Time_2");
						else if(!strcmp(name,"Car_Speed")) StringCopy(name_,"MiscEx.Car.Speed");
						else if(!strcmp(name,"Car_Passability")) StringCopy(name_,"MiscEx.Car.Negotiability");
						else if(!strcmp(name,"Car_DeteriorationRate")) StringCopy(name_,"MiscEx.Car.WearConsumption");
						else if(!strcmp(name,"Car_CrittersCapacity")) StringCopy(name_,"MiscEx.Car.CritCapacity");
						else if(!strcmp(name,"Car_TankVolume")) StringCopy(name_,"MiscEx.Car.FuelTank");
						else if(!strcmp(name,"Car_MaxDeterioration")) StringCopy(name_,"MiscEx.Car.RunToBreak");
						else if(!strcmp(name,"Car_FuelConsumption")) StringCopy(name_,"MiscEx.Car.FuelConsumption");
						else if(!strcmp(name,"Car_Entrance")) StringCopy(name_,"MiscEx.Car.Entire");
						else if(!strcmp(name,"Car_MovementType")) StringCopy(name_,"MiscEx.Car.WalkType");
						else if(!strcmp(name,"ChildLines_0")) StringCopy(name_,"MiscEx.Car.Bag_0");
						else if(!strcmp(name,"ChildLines_1")) StringCopy(name_,"MiscEx.Car.Bag_1");
						else if(!strcmp(name,"BlockLines")) StringCopy(name_,"MiscEx.Car.Blocks");
						else if(!strcmp(name,"StartValue_1")) StringCopy(name_,"MiscEx.StartVal1");
						else if(!strcmp(name,"StartValue_2")) StringCopy(name_,"MiscEx.StartVal2");
						else if(!strcmp(name,"StartValue_3")) StringCopy(name_,"MiscEx.StartVal3");
						else if(!strcmp(name,"StartCount")) StringCopy(name_,"Ammo.StartCount");
						else if(!strcmp(name,"Weapon_MaxAmmoCount")) StringCopy(name_,"Weapon.VolHolder");
						else if(!strcmp(name,"Weapon_DefaultAmmoPid")) StringCopy(name_,"Weapon.DefAmmo");
						else if(!strcmp(name,"Container_Volume")) StringCopy(name_,"Container.ContVolume");
						else if(!strcmp(name,"Ammo_AcMod")) StringCopy(name_,"Ammo.ACMod");
						else if(!strcmp(name,"Ammo_DrMod")) StringCopy(name_,"Ammo.DRMod");
						else continue;

						fopro.GetStr("Proto",name_,"",svalue);
					}

					if(!svalue[0]) continue;
				}
				else
				{
					continue;
				}
			}

			// Blocks, childs
			if(!strcmp(name,"BlockLines") || !strncmp(name,"ChildLines_",11))
			{
				// Get lines destination
				uchar* lines;
				uint max_count;
				if(!strcmp(name,"BlockLines"))
				{
					lines=proto_item.BlockLines;
					max_count=sizeof(proto_item.BlockLines);
				}
				else
				{
					int child_index=name[11]-'0';
					if(child_index<0 || child_index>=ITEM_MAX_CHILDS) continue;
					lines=proto_item.ChildLines[child_index];
					max_count=sizeof(proto_item.ChildLines[child_index]);

					// Convert dir-dir-dir to dir-1-dir-1-dir-1
					if(deprecated)
					{
						char step1[]="1";
						for(int k=0,l=strlen(svalue);k<l;k++) Str::Insert(&svalue[k*2+1],step1);
						for(char* s=svalue;s[0] && s[2];) if(s[0]==s[2]){s[1]++; Str::EraseInterval(s+2,2);}else{s+=2;}
					}
				}

				// Parse lines
				uint svalue_len=strlen(svalue);
				for(uint k=0;k/2<max_count && k+1<svalue_len;k+=2)
				{
					uchar dir=(svalue[k]-'0')&0xF;
					uchar steps=(svalue[k+1]-'0')&0xF;
					if(dir>=DIRS_COUNT || !steps) break;
					lines[k/2]=(dir<<4)|steps;
				}
			}
			// Resolve value
			else
			{
				int size=engine->GetSizeOfPrimitiveType(type_id);
				switch(size)
				{
				case sizeof(uchar): *(uchar*)(((uchar*)&proto_item)+offset)=ResolveProtoValue<uchar>(svalue); break;
				case sizeof(ushort): *(ushort*)(((uchar*)&proto_item)+offset)=ResolveProtoValue<ushort>(svalue); break;
				case sizeof(uint): *(uint*)(((uchar*)&proto_item)+offset)=ResolveProtoValue<uint>(svalue); break;
				case sizeof(int64): *(int64*)(((uchar*)&proto_item)+offset)=ResolveProtoValue<int64>(svalue); break;
				default: break;
				}
			}
		}

		if(deprecated)
		{
			// Car
			if(fopro.GetStr("Proto","MiscEx.IsCar","",svalue))
			{
				proto_item.Type=ITEM_TYPE_CAR;
				proto_item.ChildPid[0]=proto_item.StartValue[1];
				proto_item.ChildPid[1]=proto_item.StartValue[2];
				proto_item.ChildPid[2]=proto_item.StartValue[3];
				proto_item.StartValue[1]=proto_item.StartValue[2]=proto_item.StartValue[3];
			}

			// Stacking, Deteriorating
			if(proto_item.Type==ITEM_TYPE_AMMO || proto_item.Type==ITEM_TYPE_DRUG ||
				proto_item.Type==ITEM_TYPE_MISC) proto_item.Stackable=true;
			if(proto_item.Type==ITEM_TYPE_WEAPON)
			{
				if(fopro.GetStr("Proto","Weapon.NoWear","",svalue))
					proto_item.Stackable=true;
				else
					proto_item.Deteriorable=true;

			}
			if(proto_item.Type==ITEM_TYPE_ARMOR) proto_item.Deteriorable=true;

			// ITEM_TYPE_MISC_EX (6)
			if(proto_item.Type==6) proto_item.Type=ITEM_TYPE_MISC;

			// IsGroundLevel
			proto_item.GroundLevel=true;
			if(proto_item.Type==ITEM_TYPE_CONTAINER) proto_item.GroundLevel=(proto_item.Container_MagicHandsGrnd?true:false);
			else if(proto_item.IsDoor() || proto_item.IsCar() || proto_item.IsScen() ||
				proto_item.IsGrid() || !proto_item.IsCanPickUp()) proto_item.GroundLevel=false;

			// Unarmed
			if(proto_item.Weapon_IsUnarmed)
			{
				proto_item.Stackable=false;
				proto_item.Deteriorable=false;
			}

			// No open container
			if(fopro.GetStr("Proto","Container.IsNoOpen","",svalue)) SETFLAG(proto_item.Locker_Condition,LOCKER_NOOPEN);
		}

		if(proto_item.ProtoId)
		{
			// Check script
			SAFEDELA(protoScript[proto_item.ProtoId]);
			char script_module[MAX_SCRIPT_NAME+1];
			char script_func[MAX_SCRIPT_NAME+1];
			fopro.GetStr("Proto","ScriptModule","",script_module);
			fopro.GetStr("Proto","ScriptFunc","",script_func);
			if(script_module[0] && script_func[0]) protoScript[proto_item.ProtoId]=StringDuplicate(Str::Format("%s@%s",script_module,script_func));

			// Add to collection
			protos.push_back(proto_item);

		//	if(FLAG(proto_item.Flags,ITEM_MULTI_HEX)) WriteLog(NULL,"pid<%u> ITEM_MULTI_HEX\n",proto_item.ProtoId);
		//	if(FLAG(proto_item.Flags,ITEM_WALL_TRANS_END)) WriteLog(NULL,"pid<%u> ITEM_WALL_TRANS_END\n",proto_item.ProtoId);
		//	if(FLAG(proto_item.Flags,ITEM_CACHED)) WriteLog(NULL,"pid<%u> ITEM_CACHED\n",proto_item.ProtoId);
		}
	}

	if(protos.empty())
	{
		WriteLog(_FUNC_," - Proto items not found<%s>.\n",fname);
		return false;
	}
	return true;
}
#endif

bool CompProtoByPid(ProtoItem o1, ProtoItem o2){return o1.ProtoId<o2.ProtoId;}
void ItemManager::ParseProtos(ProtoItemVec& protos, const char* collection_name /* = NULL*/)
{
	if(protos.empty())
	{
		WriteLog(_FUNC_," - List is empty, parsing ended.\n");
		return;
	}

	for(uint i=0,j=protos.size();i<j;i++)
	{
		ProtoItem& proto_item=protos[i];
		ushort pid=proto_item.ProtoId;

		if(!pid || pid>=MAX_ITEM_PROTOTYPES)
		{
			WriteLog(_FUNC_," - Invalid pid<%u> of item prototype.\n",pid);
			continue;
		}

		if(IsInitProto(pid))
		{
			ClearProto(pid);
			WriteLog(_FUNC_," - Item prototype is already parsed, pid<%u>. Rewrite.\n",pid);
		}

		int type=protos[i].Type;
		if(type<0 || type>=ITEM_MAX_TYPES) type=ITEM_TYPE_OTHER;

		typeProto[type].push_back(proto_item);
		allProto[pid]=proto_item;

#ifdef FONLINE_MAPPER
		ProtosCollectionName[pid]=collection_name;
#endif
	}

	for(int i=0;i<ITEM_MAX_TYPES;i++)
	{
		protoHash[i]=0;
		if(typeProto[i].size())
		{
			std::sort(typeProto[i].begin(),typeProto[i].end(),CompProtoByPid);
			protoHash[i]=Crypt.Crc32((uchar*)&typeProto[i][0],sizeof(ProtoItem)*typeProto[i].size());
		}
	}
}

ProtoItem* ItemManager::GetProtoItem(ushort pid)
{
	if(!IsInitProto(pid)) return NULL;
	return &allProto[pid];
}

ProtoItem* ItemManager::GetAllProtos()
{
	return allProto;
}

void ItemManager::GetCopyAllProtos(ProtoItemVec& protos)
{
	for(int i=0;i<ITEM_MAX_TYPES;i++)
	{
		for(uint j=0;j<typeProto[i].size();j++)
		{
			protos.push_back(typeProto[i][j]);
		}
	}
}

bool ItemManager::IsInitProto(ushort pid)
{
	if(!pid || pid>=MAX_ITEM_PROTOTYPES) return false;
	return allProto[pid].ProtoId!=0;
}

const char* ItemManager::GetProtoScript(ushort pid)
{
	if(!IsInitProto(pid)) return NULL;
	return protoScript[pid];
}

void ItemManager::ClearProtos(int type /* = 0xFF */)
{
	if(type==0xFF)
	{
		for(int i=0;i<ITEM_MAX_TYPES;i++)
		{
			protoHash[i]=0;
			typeProto[i].clear();
		}
		ZeroMemory(allProto,sizeof(allProto));
		for(int i=0;i<MAX_ITEM_PROTOTYPES;i++) SAFEDELA(protoScript[i]);
	}
	else if(type<ITEM_MAX_TYPES)
	{
		for(uint i=0;i<typeProto[type].size();++i)
		{
			ushort pid=(typeProto[type])[i].ProtoId;
			if(IsInitProto(pid))
			{
				allProto[pid].Clear();
				SAFEDELA(protoScript[pid]);
			}
		}
		typeProto[type].clear();
		protoHash[type]=0;
	}
	else
	{
		WriteLog(_FUNC_," - Wrong proto type<%u>.\n",type);
	}
}

void ItemManager::ClearProto(ushort pid)
{
	ProtoItem* proto_item=GetProtoItem(pid);
	if(!proto_item) return;

	int type=proto_item->Type;
	if(type<0 || type>=ITEM_MAX_TYPES) type=ITEM_TYPE_OTHER;

	ProtoItemVec& protos=typeProto[type];
	uint& hash=protoHash[type];

	allProto[pid].Clear();

	ProtoItemVecIt it=std::find(protos.begin(),protos.end(),pid);
	if(it!=protos.end()) protos.erase(it);

	hash=Crypt.Crc32((uchar*)&protos[0],sizeof(ProtoItem)*protos.size());
}

#ifdef FONLINE_SERVER
void ItemManager::SaveAllItemsFile(void(*save_func)(void*,size_t))
{
	uint count=gameItems.size();
	save_func(&count,sizeof(count));
	for(ItemPtrMapIt it=gameItems.begin(),end=gameItems.end();it!=end;++it)
	{
		Item* item=(*it).second;
		save_func(&item->Id,sizeof(item->Id));
		save_func(&item->Proto->ProtoId,sizeof(item->Proto->ProtoId));
		save_func(&item->Accessory,sizeof(item->Accessory));
		save_func(&item->ACC_BUFFER.Buffer[0],sizeof(item->ACC_BUFFER.Buffer[0]));
		save_func(&item->ACC_BUFFER.Buffer[1],sizeof(item->ACC_BUFFER.Buffer[1]));
		save_func(&item->Data,sizeof(item->Data));
		if(item->PLexems)
		{
			uchar lex_len=strlen(item->PLexems);
			save_func(&lex_len,sizeof(lex_len));
			save_func(item->PLexems,lex_len);
		}
		else
		{
			uchar zero=0;
			save_func(&zero,sizeof(zero));
		}
	}
}

bool ItemManager::LoadAllItemsFile(FILE* f, int version)
{
	WriteLog(NULL,"Load items...");

	lastItemId=0;

	uint count;
	fread(&count,sizeof(count),1,f);
	if(!count)
	{
		WriteLog(NULL,"items not found.\n");
		return true;
	}

	int errors=0;
	for(uint i=0;i<count;++i)
	{
		uint id;
		fread(&id,sizeof(id),1,f);
		ushort pid;
		fread(&pid,sizeof(pid),1,f);
		uchar acc;
		fread(&acc,sizeof(acc),1,f);
		uint acc0;
		fread(&acc0,sizeof(acc0),1,f);
		uint acc1;
		fread(&acc1,sizeof(acc1),1,f);
		Item::ItemData data;
		fread(&data,sizeof(data),1,f);
		uchar lex_len;
		char lexems[1024]={0};
		fread(&lex_len,sizeof(lex_len),1,f);
		if(lex_len)
		{
			fread(lexems,lex_len,1,f);
			lexems[lex_len]=0;
		}

		Item* item=CreateItem(pid,0,id);
		if(!item)
		{
			WriteLog(NULL,"Create item error id<%u>, pid<%u>.\n",id,pid);
			errors++;
			continue;
		}
		if(id>lastItemId) lastItemId=id;

		item->Accessory=acc;
		item->ACC_BUFFER.Buffer[0]=acc0;
		item->ACC_BUFFER.Buffer[1]=acc1;
		item->Data=data;
		if(lexems[0]) item->SetLexems(lexems);

		AddItemStatistics(pid,item->GetCount());

		// Radio collection
		if(item->IsRadio()) RadioRegister(item,true);

		// Patches
		if(version<WORLD_SAVE_V11)
		{
			if(item->GetProtoId()==100/*PID_RADIO*/ && FLAG(item->Proto->Flags,ITEM_RADIO)) SETFLAG(item->Data.Flags,ITEM_RADIO);
			else if(item->GetProtoId()==58/*PID_HOLODISK*/ && FLAG(item->Proto->Flags,ITEM_HOLODISK)) SETFLAG(item->Data.Flags,ITEM_HOLODISK);
		}
	}
	if(errors) return false;

	WriteLog(NULL,"complete, count<%u>.\n",count);
	return true;
}

#pragma MESSAGE("Add item proto functions checker.")
bool ItemManager::CheckProtoFunctions()
{
	for(int i=0;i<MAX_ITEM_PROTOTYPES;i++)
	{
		const char* script=protoScript[i];
		// Check for items
		// Check for scenery
	}
	return true;
}

void ItemManager::RunInitScriptItems()
{
	ItemPtrMap items=gameItems;
	for(ItemPtrMapIt it=items.begin(),end=items.end();it!=end;++it)
	{
		Item* item=(*it).second;
		if(item->Data.ScriptId) item->ParseScript(NULL,false);
	}
}

void ItemManager::GetGameItems(ItemPtrVec& items)
{
	SCOPE_LOCK(itemLocker);
	items.reserve(gameItems.size());
	for(ItemPtrMapIt it=gameItems.begin(),end=gameItems.end();it!=end;++it)
	{
		Item* item=(*it).second;
		items.push_back(item);
	}
}

uint ItemManager::GetItemsCount()
{
	SCOPE_LOCK(itemLocker);
	uint count=gameItems.size();
	return count;
}

void ItemManager::SetCritterItems(Critter* cr)
{
	ItemPtrVec items;
	uint crid=cr->GetId();

	itemLocker.Lock();
	for(ItemPtrMapIt it=gameItems.begin(),end=gameItems.end();it!=end;++it)
	{
		Item* item=(*it).second;
		if(item->Accessory==ITEM_ACCESSORY_CRITTER && item->ACC_CRITTER.Id==crid)
			items.push_back(item);
	}
	itemLocker.Unlock();

	for(ItemPtrVecIt it=items.begin(),end=items.end();it!=end;++it)
	{
		Item* item=*it;
		SYNC_LOCK(item);

		if(item->Accessory==ITEM_ACCESSORY_CRITTER && item->ACC_CRITTER.Id==crid)
		{
			cr->SetItem(item);
			if(item->IsRadio()) RadioRegister(item,true);
		}
	}
}

void ItemManager::GetItemIds(UIntSet& item_ids)
{
	SCOPE_LOCK(itemLocker);

	for(ItemPtrMapIt it=gameItems.begin(),end=gameItems.end();it!=end;++it)
		item_ids.insert((*it).second->GetId());
}

Item* ItemManager::CreateItem(ushort pid, uint count, uint item_id /* = 0 */)
{
	if(!IsInitProto(pid)) return NULL;

	Item* item=new Item();
	if(!item)
	{
		WriteLog(_FUNC_," - Allocation fail.\n");
		return NULL;
	}

	item->Init(&allProto[pid]);
	if(item_id)
	{
		SCOPE_LOCK(itemLocker);

		if(gameItems.count(item_id))
		{
			WriteLog(_FUNC_," - Item already created, id<%u>.\n",item_id);
			delete item;
			return NULL;
		}

		item->Id=item_id;
	}
	else
	{
		SCOPE_LOCK(itemLocker);

		item->Id=lastItemId+1;
		lastItemId++;
	}

	if(item->IsStackable()) item->Count_Set(count);
	else AddItemStatistics(pid,1);

	SYNC_LOCK(item);

	// Main collection
	itemLocker.Lock();
	gameItems.insert(ItemPtrMapVal(item->Id,item));
	itemLocker.Unlock();

	// Radio collection
	if(item->IsRadio()) RadioRegister(item,true);

	// Prototype script
	if(!item_id && count && protoScript[pid]) // Only for new items
	{
		item->ParseScript(protoScript[pid],true);
		if(item->IsNotValid) return NULL;
	}
	return item;
}

Item* ItemManager::SplitItem(Item* item, uint count)
{
	if(!item->IsStackable())
	{
		WriteLog(_FUNC_," - Splitted item is not stackable, id<%u>, pid<%u>.\n",item->GetId(),item->GetProtoId());
		return NULL;
	}

	uint item_count=item->GetCount();
	if(!count || count>=item_count)
	{
		WriteLog(_FUNC_," - Invalid count, id<%u>, pid<%u>, count<%u>, split count<%u>.\n",item->GetId(),item->GetProtoId(),item_count,count);
		return NULL;
	}

	Item* new_item=CreateItem(item->GetProtoId(),0); // Ignore init script
	if(!new_item)
	{
		WriteLog(_FUNC_," - Create item fail, pid<%u>, count<%u>.\n",item->GetProtoId(),count);
		return NULL;
	}

	item->Count_Sub(count);
	new_item->Data=item->Data;
	new_item->Data.Count=0;
	new_item->Count_Add(count);
	if(item->PLexems) new_item->SetLexems(item->PLexems);

	// Radio collection
	if(new_item->IsRadio()) RadioRegister(new_item,true);

	return new_item;
}

Item* ItemManager::GetItem(uint item_id, bool sync_lock)
{
	Item* item=NULL;

	itemLocker.Lock();
	ItemPtrMapIt it=gameItems.find(item_id);
	if(it!=gameItems.end()) item=(*it).second;
	itemLocker.Unlock();

	if(item && sync_lock) SYNC_LOCK(item);
	return item;
}

void ItemManager::ItemToGarbage(Item* item)
{
	SCOPE_LOCK(itemLocker);
	itemToDelete.push_back(item->GetId());
	itemToDeleteCount.push_back(item->GetCount());
}

void ItemManager::ItemGarbager()
{
	if(!itemToDelete.empty())
	{
		itemLocker.Lock();
		UIntVec to_del=itemToDelete;
		UIntVec to_del_count=itemToDeleteCount;
		itemToDelete.clear();
		itemToDeleteCount.clear();
		itemLocker.Unlock();

		for(uint i=0,j=to_del.size();i<j;i++)
		{
			uint id=to_del[i];
			uint count=to_del_count[i];

			// Erase from main collection
			itemLocker.Lock();
			ItemPtrMapIt it=gameItems.find(id);
			if(it==gameItems.end())
			{
				itemLocker.Unlock();
				continue;
			}
			Item* item=(*it).second;
			gameItems.erase(it);
			itemLocker.Unlock();

			// Synchronize
			SYNC_LOCK(item);

			// Maybe some items added
			if(item->IsStackable() && item->GetCount()>count)
			{
				itemLocker.Lock();
				gameItems.insert(ItemPtrMapVal(item->Id,item));
				itemLocker.Unlock();

				item->Count_Sub(count);
				NotifyChangeItem(item);
				continue;
			}

			// Call finish event
			if(item->IsValidAccessory()) EraseItemHolder(item);
			if(!item->IsNotValid && item->FuncId[ITEM_EVENT_FINISH]>0) item->EventFinish(true);

			item->IsNotValid=true;
			if(item->IsValidAccessory()) EraseItemHolder(item);

			// Erase from statistics
			if(item->IsStackable()) item->Count_Set(0);
			else SubItemStatistics(item->GetProtoId(),1);

			// Erase from radio collection
			if(item->IsRadio()) RadioRegister(item,false);

			// Clear, release
			item->FullClear();
			Job::DeferredRelease(item);
		}
	}
}

void ItemManager::NotifyChangeItem(Item* item)
{
	switch(item->Accessory)
	{
	case ITEM_ACCESSORY_CRITTER:
		{
			Critter* cr=CrMngr.GetCritter(item->ACC_CRITTER.Id,false);
			if(cr) cr->SendAA_ItemData(item);
		}
		break;
	case ITEM_ACCESSORY_HEX:
		{
			Map* map=MapMngr.GetMap(item->ACC_HEX.MapId,false);
			if(map) map->ChangeDataItem(item);
		}
		break;
	default:
		break;
	}
}

void ItemManager::EraseItemHolder(Item* item)
{
	switch(item->Accessory)
	{
	case ITEM_ACCESSORY_CRITTER:
		{
			Critter* cr=CrMngr.GetCritter(item->ACC_CRITTER.Id,true);
			if(cr) cr->EraseItem(item,true);
			else if(item->IsRadio()) ItemMngr.RadioRegister(item,true);
		}
		break;
	case ITEM_ACCESSORY_HEX:
		{
			Map* map=MapMngr.GetMap(item->ACC_HEX.MapId,true);
			if(map) map->EraseItem(item->GetId());
		}
		break;
	case ITEM_ACCESSORY_CONTAINER:
		{
			Item* cont=ItemMngr.GetItem(item->ACC_CONTAINER.ContainerId,true);
			if(cont) cont->ContEraseItem(item);
		}
		break;
	default:
		break;
	}
	item->Accessory=45;
}

void ItemManager::MoveItem(Item* item, uint count, Critter* to_cr)
{
	if(item->Accessory==ITEM_ACCESSORY_CRITTER && item->ACC_CRITTER.Id==to_cr->GetId()) return;

	if(count>=item->GetCount() || !item->IsStackable())
	{
		EraseItemHolder(item);
		to_cr->AddItem(item,true);
	}
	else
	{
		Item* item_=ItemMngr.SplitItem(item,count);
		if(item_)
		{
			NotifyChangeItem(item);
			to_cr->AddItem(item_,true);
		}
	}
}

void ItemManager::MoveItem(Item* item, uint count, Map* to_map, ushort to_hx, ushort to_hy)
{
	if(item->Accessory==ITEM_ACCESSORY_HEX && item->ACC_HEX.MapId==to_map->GetId() && item->ACC_HEX.HexX==to_hx && item->ACC_HEX.HexY==to_hy) return;

	if(count>=item->GetCount() || !item->IsStackable())
	{
		EraseItemHolder(item);
		to_map->AddItem(item,to_hx,to_hy);
	}
	else
	{
		Item* item_=ItemMngr.SplitItem(item,count);
		if(item_)
		{
			NotifyChangeItem(item);
			to_map->AddItem(item_,to_hx,to_hy);
		}
	}
}

void ItemManager::MoveItem(Item* item, uint count, Item* to_cont, uint stack_id)
{
	if(item->Accessory==ITEM_ACCESSORY_CONTAINER && item->ACC_CONTAINER.ContainerId==to_cont->GetId() && item->ACC_CONTAINER.SpecialId==stack_id) return;

	if(count>=item->GetCount() || !item->IsStackable())
	{
		EraseItemHolder(item);
		to_cont->ContAddItem(item,stack_id);
	}
	else
	{
		Item* item_=ItemMngr.SplitItem(item,count);
		if(item_)
		{
			NotifyChangeItem(item);
			to_cont->ContAddItem(item_,stack_id);
		}
	}
}

Item* ItemManager::AddItemContainer(Item* cont, ushort pid, uint count, uint stack_id)
{
	if(!cont || !cont->IsContainer()) return NULL;

	Item* item=cont->ContGetItemByPid(pid,stack_id);
	Item* result=NULL;

	if(item)
	{
		if(item->IsStackable())
		{
			item->Count_Add(count);
			result=item;
		}
		else
		{
			if(count>MAX_ADDED_NOGROUP_ITEMS) count=MAX_ADDED_NOGROUP_ITEMS;
			for(uint i=0;i<count;++i)
			{
				item=ItemMngr.CreateItem(pid,1);
				if(!item) continue;
				cont->ContAddItem(item,stack_id);
				result=item;
			}
		}
	}
	else
	{
		ProtoItem* proto_item=ItemMngr.GetProtoItem(pid);
		if(!proto_item) return result;

		if(proto_item->Stackable)
		{
			item=ItemMngr.CreateItem(pid,count);
			if(!item) return result;
			cont->ContAddItem(item,stack_id);
			result=item;
		}
		else
		{
			if(count>MAX_ADDED_NOGROUP_ITEMS) count=MAX_ADDED_NOGROUP_ITEMS;
			for(uint i=0;i<count;++i)
			{
				item=ItemMngr.CreateItem(pid,1);
				if(!item) continue;
				cont->ContAddItem(item,stack_id);
				result=item;
			}
		}
	}

	return result;
}

Item* ItemManager::AddItemCritter(Critter* cr, ushort pid, uint count)
{
	if(!count) return NULL;

	Item* item=cr->GetItemByPid(pid);
	Item* result=NULL;

	if(item)
	{
		if(item->IsStackable())
		{
			item->Count_Add(count);
			cr->SendAA_ItemData(item);
			result=item;
		}
		else
		{
			if(count>MAX_ADDED_NOGROUP_ITEMS) count=MAX_ADDED_NOGROUP_ITEMS;
			for(uint i=0;i<count;++i)
			{
				item=ItemMngr.CreateItem(pid,1);
				if(!item) break;
				cr->AddItem(item,true);
				result=item;
			}
		}
	}
	else
	{
		ProtoItem* proto_item=ItemMngr.GetProtoItem(pid);
		if(!proto_item) return result;

		if(proto_item->Stackable)
		{
			item=ItemMngr.CreateItem(pid,count);
			if(!item) return result;
			cr->AddItem(item,true);
			result=item;
		}
		else
		{
			if(count>MAX_ADDED_NOGROUP_ITEMS) count=MAX_ADDED_NOGROUP_ITEMS;
			for(uint i=0;i<count;++i)
			{
				item=ItemMngr.CreateItem(pid,1);
				if(!item) break;
				cr->AddItem(item,true);
				result=item;
			}
		}
	}

	return result;
}

bool ItemManager::SubItemCritter(Critter* cr, ushort pid, uint count, ItemPtrVec* erased_items)
{
	if(!count) return true;

	Item* item=cr->GetItemByPidInvPriority(pid);
	if(!item) return true;

	if(item->IsStackable())
	{
		if(count>=item->GetCount())
		{
			cr->EraseItem(item,true);
			if(!erased_items) ItemMngr.ItemToGarbage(item);
			else erased_items->push_back(item);
		}
		else
		{
			if(erased_items)
			{
				Item* item_=ItemMngr.SplitItem(item,count);
				if(item_) erased_items->push_back(item_);
			}
			else
			{
				item->Count_Sub(count);
			}
			cr->SendAA_ItemData(item);
		}
	}
	else
	{
		for(uint i=0;i<count;++i)
		{
			cr->EraseItem(item,true);
			if(!erased_items) ItemMngr.ItemToGarbage(item);
			else erased_items->push_back(item);
			item=cr->GetItemByPidInvPriority(pid);
			if(!item) return true;
		}
	}

	return true;
}

bool ItemManager::SetItemCritter(Critter* cr, ushort pid, uint count)
{
	uint cur_count=cr->CountItemPid(pid);
	if(cur_count>count) return SubItemCritter(cr,pid,cur_count-count);
	else if(cur_count<count) return AddItemCritter(cr,pid,count-cur_count)!=NULL;
	return true;
}

bool ItemManager::MoveItemCritters(Critter* from_cr, Critter* to_cr, uint item_id, uint count)
{
	Item* item=from_cr->GetItem(item_id,false);
	if(!item) return false;

	if(!count || count>item->GetCount()) count=item->GetCount();

	if(item->IsStackable() && item->GetCount()>count)
	{
		Item* item_=to_cr->GetItemByPid(item->GetProtoId());
		if(!item_)
		{
			item_=ItemMngr.CreateItem(item->GetProtoId(),count);
			if(!item_)
			{
				WriteLog(_FUNC_," - Create item fail, pid<%u>.\n",item->GetProtoId());
				return false;
			}

			to_cr->AddItem(item_,true);
		}
		else
		{
			item_->Count_Add(count);
			to_cr->SendAA_ItemData(item_);
		}

		item->Count_Sub(count);
		from_cr->SendAA_ItemData(item);
	}
	else
	{
		from_cr->EraseItem(item,true);
		to_cr->AddItem(item,true);
	}

	return true;
}

bool ItemManager::MoveItemCritterToCont(Critter* from_cr, Item* to_cont, uint item_id, uint count, uint stack_id)
{
	if(!to_cont->IsContainer()) return false;

	Item* item=from_cr->GetItem(item_id,false);
	if(!item) return false;

	if(!count || count>item->GetCount()) count=item->GetCount();

	if(item->IsStackable() && item->GetCount()>count)
	{
		Item* item_=to_cont->ContGetItemByPid(item->GetProtoId(),stack_id);
		if(!item_)
		{
			item_=ItemMngr.CreateItem(item->GetProtoId(),count);
			if(!item_)
			{
				WriteLog(_FUNC_," - Create item fail, pid<%u>.\n",item->GetProtoId());
				return false;
			}

			item_->ACC_CONTAINER.SpecialId=stack_id;
			to_cont->ContSetItem(item_);
		}
		else
		{
			item_->Count_Add(count);
		}

		item->Count_Sub(count);
		from_cr->SendAA_ItemData(item);
	}
	else
	{
		from_cr->EraseItem(item,true);
		to_cont->ContAddItem(item,stack_id);
	}

	return true;
}

bool ItemManager::MoveItemCritterFromCont(Item* from_cont, Critter* to_cr, uint item_id, uint count)
{
	if(!from_cont->IsContainer()) return false;

	Item* item=from_cont->ContGetItem(item_id,false);
	if(!item) return false;

	if(!count || count>item->GetCount()) count=item->GetCount();

	if(item->IsStackable() && item->GetCount()>count)
	{
		Item* item_=to_cr->GetItemByPid(item->GetProtoId());
		if(!item_)
		{
			item_=ItemMngr.CreateItem(item->GetProtoId(),count);
			if(!item_)
			{
				WriteLog(_FUNC_," - Create item fail, pid<%u>.\n",item->GetProtoId());
				return false;
			}

			to_cr->AddItem(item_,true);
		}
		else
		{
			item_->Count_Add(count);
			to_cr->SendAA_ItemData(item_);
		}

		item->Count_Sub(count);
	}
	else
	{
		from_cont->ContEraseItem(item);
		to_cr->AddItem(item,true);
	}

	return true;
}

bool ItemManager::MoveItemsContainers(Item* from_cont, Item* to_cont, uint from_stack_id, uint to_stack_id)
{
	if(!from_cont->IsContainer() || !to_cont->IsContainer()) return false;
	if(!from_cont->ContIsItems()) return true;

	ItemPtrVec items;
	from_cont->ContGetItems(items,from_stack_id,true);
	for(ItemPtrVecIt it=items.begin(),end=items.end();it!=end;++it)
	{
		Item* item=*it;
		from_cont->ContEraseItem(item);
		to_cont->ContAddItem(item,to_stack_id);
	}

	return true;
}

bool ItemManager::MoveItemsContToCritter(Item* from_cont, Critter* to_cr, uint stack_id)
{
	if(!from_cont->IsContainer()) return false;
	if(!from_cont->ContIsItems()) return true;

	ItemPtrVec items;
	from_cont->ContGetItems(items,stack_id,true);
	for(ItemPtrVecIt it=items.begin(),end=items.end();it!=end;++it)
	{
		Item* item=*it;
		from_cont->ContEraseItem(item);
		to_cr->AddItem(item,true);
	}

	return true;
}

void ItemManager::RadioRegister(Item* radio, bool add)
{
	SCOPE_LOCK(radioItemsLocker);

	ItemPtrVecIt it=std::find(radioItems.begin(),radioItems.end(),radio);

	if(add)
	{
		if(it==radioItems.end()) radioItems.push_back(radio);
	}
	else
	{
		if(it!=radioItems.end()) radioItems.erase(it);
	}
}

void ItemManager::RadioSendText(Critter* cr, const char* text, ushort text_len, bool unsafe_text, ushort text_msg, uint num_str, UShortVec& channels)
{
	ItemPtrVec radios;
	ItemPtrVec items=cr->GetItemsNoLock();
	for(ItemPtrVecIt it=items.begin(),end=items.end();it!=end;++it)
	{
		Item* item=*it;
		if(item->IsRadio() && item->RadioIsSendActive() &&
			std::find(channels.begin(),channels.end(),item->Data.Radio.Channel)==channels.end())
		{
			channels.push_back(item->Data.Radio.Channel);
			radios.push_back(item);

			if(radios.size()>100) break;
		}
	}

	for(uint i=0,j=radios.size();i<j;i++)
	{
		RadioSendTextEx(channels[i],
			radios[i]->Data.Radio.BroadcastSend,cr->GetMap(),cr->Data.WorldX,cr->Data.WorldY,
			text,text_len,cr->IntellectCacheValue,unsafe_text,text_msg,num_str);
	}
}

void ItemManager::RadioSendTextEx(ushort channel, int broadcast_type, uint from_map_id, ushort from_wx, ushort from_wy,
								  const char* text, ushort text_len, ushort intellect, bool unsafe_text,
								  ushort text_msg, uint num_str)
{
	// Broadcast
	if(broadcast_type!=RADIO_BROADCAST_FORCE_ALL && broadcast_type!=RADIO_BROADCAST_WORLD &&
		broadcast_type!=RADIO_BROADCAST_MAP && broadcast_type!=RADIO_BROADCAST_LOCATION &&
		!(broadcast_type>=101 && broadcast_type<=200)/*RADIO_BROADCAST_ZONE*/) return;
	if((broadcast_type==RADIO_BROADCAST_MAP || broadcast_type==RADIO_BROADCAST_LOCATION) && !from_map_id) return;

	int broadcast=0;
	uint broadcast_map_id=0;
	uint broadcast_loc_id=0;

	// Get copy of all radios
	radioItemsLocker.Lock();
	ItemPtrVec radio_items=radioItems;
	radioItemsLocker.Unlock();

	// Multiple sending controlling
	// Not thread safe, but this not so important in this case
	static uint msg_count=0;
	msg_count++;

	// Send
	for(ItemPtrVecIt it=radio_items.begin(),end=radio_items.end();it!=end;++it)
	{
		Item* radio=*it;

		if(radio->Data.Radio.Channel==channel && radio->RadioIsRecvActive())
		{
			if(broadcast_type!=RADIO_BROADCAST_FORCE_ALL && radio->Data.Radio.BroadcastRecv!=RADIO_BROADCAST_FORCE_ALL)
			{
				if(broadcast_type==RADIO_BROADCAST_WORLD) broadcast=radio->Data.Radio.BroadcastRecv;
				else if(radio->Data.Radio.BroadcastRecv==RADIO_BROADCAST_WORLD) broadcast=broadcast_type;
				else broadcast=MIN(broadcast_type,radio->Data.Radio.BroadcastRecv);

				if(broadcast==RADIO_BROADCAST_WORLD) broadcast=RADIO_BROADCAST_FORCE_ALL;
				else if(broadcast==RADIO_BROADCAST_MAP || broadcast==RADIO_BROADCAST_LOCATION)
				{
					if(!broadcast_map_id)
					{
						Map* map=MapMngr.GetMap(from_map_id,false);
						if(!map) continue;
						broadcast_map_id=map->GetId();
						broadcast_loc_id=map->GetLocation(false)->GetId();
					}
				}
				else if(!(broadcast>=101 && broadcast<=200)/*RADIO_BROADCAST_ZONE*/) continue;
			}
			else
			{
				broadcast=RADIO_BROADCAST_FORCE_ALL;
			}

			if(radio->Accessory==ITEM_ACCESSORY_CRITTER)
			{
				Client* cl=CrMngr.GetPlayer(radio->ACC_CRITTER.Id,false);
				if(cl && cl->RadioMessageSended!=msg_count)
				{
					if(broadcast!=RADIO_BROADCAST_FORCE_ALL)
					{
						if(broadcast==RADIO_BROADCAST_MAP)
						{
							if(broadcast_map_id!=cl->GetMap()) continue;
						}
						else if(broadcast==RADIO_BROADCAST_LOCATION)
						{
							Map* map=MapMngr.GetMap(cl->GetMap(),false);
							if(!map || broadcast_loc_id!=map->GetLocation(false)->GetId()) continue;
						}
						else if(broadcast>=101 && broadcast<=200) // RADIO_BROADCAST_ZONE
						{
							if(!MapMngr.IsIntersectZone(from_wx,from_wy,0,cl->Data.WorldX,cl->Data.WorldY,0,broadcast-101)) continue;
						}
						else continue;
					}

					if(text) cl->Send_TextEx(radio->GetId(),text,text_len,SAY_RADIO,intellect,unsafe_text);
					else cl->Send_TextMsg(radio->GetId(),num_str,SAY_RADIO,text_msg);

					cl->RadioMessageSended=msg_count;
				}
			}
			else if(radio->Accessory==ITEM_ACCESSORY_HEX)
			{
				if(broadcast==RADIO_BROADCAST_MAP && broadcast_map_id!=radio->ACC_HEX.MapId) continue;

				Map* map=MapMngr.GetMap(radio->ACC_HEX.MapId,false);
				if(map)
				{
					if(broadcast!=RADIO_BROADCAST_FORCE_ALL && broadcast!=RADIO_BROADCAST_MAP)
					{
						if(broadcast==RADIO_BROADCAST_LOCATION)
						{
							Location* loc=map->GetLocation(false);
							if(broadcast_loc_id!=loc->GetId()) continue;
						}
						else if(broadcast>=101 && broadcast<=200) // RADIO_BROADCAST_ZONE
						{
							Location* loc=map->GetLocation(false);
							if(!MapMngr.IsIntersectZone(from_wx,from_wy,0,loc->Data.WX,loc->Data.WY,loc->GetRadius(),broadcast-101)) continue;
						}
						else continue;
					}

					if(text) map->SetText(radio->ACC_HEX.HexX,radio->ACC_HEX.HexY,0xFFFFFFFE,text,text_len,intellect,unsafe_text);
					else map->SetTextMsg(radio->ACC_HEX.HexX,radio->ACC_HEX.HexY,0xFFFFFFFE,text_msg,num_str);
				}
			}
		}
	}
}
#endif // FONLINE_SERVER

void ItemManager::AddItemStatistics(ushort pid, uint val)
{
	if(!IsInitProto(pid)) return;

	SCOPE_SPINLOCK(itemCountLocker);

	itemCount[pid]+=(int64)val;
}

void ItemManager::SubItemStatistics(ushort pid, uint val)
{
	if(!IsInitProto(pid)) return;

	SCOPE_SPINLOCK(itemCountLocker);

	itemCount[pid]-=(int64)val;
}

int64 ItemManager::GetItemStatistics(ushort pid)
{
	SCOPE_SPINLOCK(itemCountLocker);

	int64 count=itemCount[pid];
	return count;
}

string ItemManager::GetItemsStatistics()
{
	SCOPE_SPINLOCK(itemCountLocker);

	string result="Pid    Name                                     Count\n";
	if(IsInit())
	{
		char str[512];
		char str_[128];
		char str__[128];
		for(int i=0;i<MAX_ITEM_PROTOTYPES;i++)
		{
			ProtoItem* proto_item=GetProtoItem(i);
			if(proto_item && proto_item->IsItem())
			{
				char* s=(char*)ConstantsManager::GetItemName(i);
				sprintf(str,"%-6u %-40s %-20s\n",i,s?s:_itoa(i,str_,10),_i64toa(itemCount[i],str__,10));
				result+=str;
			}
		}
	}
	return result;
}

//==============================================================================================================================
//******************************************************************************************************************************
//==============================================================================================================================