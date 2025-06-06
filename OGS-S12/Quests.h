#pragma once
#include "framework.h"

namespace Quests {
	void GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* Def, UFortQuestItemDefinition* QuestDef = nullptr)
	{
		if (!PC || !Def) return;

		FAthenaAccolades Accolade{};
		Accolade.AccoladeDef = Def;
		Accolade.Count = 1;
		std::string DefName = Def->GetName();
		Accolade.TemplateId = std::wstring(DefName.begin(), DefName.end()).c_str();

		auto ID = UKismetSystemLibrary::GetDefaultObj()->GetPrimaryAssetIdFromObject(Def);

		FXPEventInfo EventInfo{};
		EventInfo.Accolade = ID;
		EventInfo.EventName = Def->Name;
		EventInfo.EventXpValue = Def->GetAccoladeXpValue();
		EventInfo.Priority = Def->Priority;
		if (QuestDef) {
			EventInfo.QuestDef = QuestDef;
		}
		EventInfo.SimulatedText = Def->GetShortDescription();
		EventInfo.RestedValuePortion = EventInfo.EventXpValue;
		EventInfo.RestedXPRemaining = EventInfo.EventXpValue;
		EventInfo.SeasonBoostValuePortion = 20;
		EventInfo.TotalXpEarnedInMatch = EventInfo.EventXpValue + PC->XPComponent->TotalXpEarned;

		PC->XPComponent->MedalBonusXP += 1250;
		PC->XPComponent->MatchXp += EventInfo.EventXpValue;
		PC->XPComponent->TotalXpEarned += EventInfo.EventXpValue + 1250;

		PC->XPComponent->PlayerAccolades.Add(Accolade);
		PC->XPComponent->MedalsEarned.Add(Def);

		PC->XPComponent->ClientMedalsRecived(PC->XPComponent->PlayerAccolades);
		PC->XPComponent->OnXPEvent(EventInfo);
	}

	void ProgressQuest(AFortPlayerControllerAthena* PC, UFortQuestItemDefinition* QuestDef, FName BackendName)
	{
		PC->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, nullptr, true, false);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		for (size_t i = 0; i < PlayerState->PlayerTeam->TeamMembers.Num(); i++)
		{
			auto pc = (AFortPlayerControllerAthena*)PlayerState->PlayerTeam->TeamMembers[i];
			if (pc && pc != PC)
			{
				pc->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, PlayerState, true, false);
			}
		}
		auto QuestItem = PC->GetQuestManager(ESubGame::Athena)->GetQuestWithDefinition(QuestDef);

		int32 XPCount = 20;

		if (auto RewardsTable = QuestDef->RewardsTable)
		{
			static auto Name = FName(L"Default");
			auto DefaultRow = RewardsTable->Search([](FName& RName, uint8* Row) { return RName == Name; });
			XPCount = (*(FFortQuestRewardTableRow**)DefaultRow)->Quantity;
		}

		FXPEventEntry Entry{};
		Entry.EventXpValue = XPCount;
		Entry.QuestDef = QuestDef;
		Entry.Time = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
		PC->XPComponent->ChallengeXp += Entry.EventXpValue;
		PC->XPComponent->TotalXpEarned += Entry.EventXpValue;
		Entry.TotalXpEarnedInMatch = PC->XPComponent->TotalXpEarned;
		Entry.SimulatedXpEvent = QuestDef->GetSingleLineDescription();
		PC->XPComponent->RestXP += Entry.EventXpValue;
		PC->XPComponent->InMatchProfileVer++;
		PC->XPComponent->OnInMatchProfileUpdate(PC->XPComponent->InMatchProfileVer);
		PC->XPComponent->OnProfileUpdated();
		PC->XPComponent->OnXpUpdated(PC->XPComponent->CombatXp, PC->XPComponent->SurvivalXp, PC->XPComponent->MedalBonusXP, PC->XPComponent->ChallengeXp, PC->XPComponent->MatchXp, PC->XPComponent->TotalXpEarned);
		PC->XPComponent->WaitingQuestXp.Add(Entry);

		//std::cout << PC->XPComponent->WaitingQuestXp.Num() << endl;
		PC->XPComponent->HighPrioXPEvent(Entry);
	}

	bool ContainsTag(FGameplayTagContainer Container, FName Tag)
	{
		for (auto tag : Container.GameplayTags)
		{
			if (tag.TagName.ComparisonIndex == Tag.ComparisonIndex)
				return true;
		}
		return false;
	}

	void SendStatEvent(UFortQuestManager* ManagerComp, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count, EFortQuestObjectiveStatEvent StatEvent)
	{
		if (!ManagerComp || !TargetObject) {
			Log("ManagerComp or TargetObject is nullptr!");
		}

		auto PC = (AFortPlayerControllerAthena*)ManagerComp->GetPlayerControllerBP();
		if (!PC) {
			Log("PC doesent exist!");
			return;
		}
		else {
			Log("PC does exist!");
		}

		FGameplayTagContainer Source;
		FGameplayTagContainer Context;
		ManagerComp->GetSourceAndContextTags(&Source, &Context);
		ManagerComp->AppendTemporaryRelevancyTags(Source, Context, TargetTags);

		for (auto tag : Source.GameplayTags)
		{
			bool contains = false;
			for (auto Tag : AdditionalSourceTags.GameplayTags)
			{
				if (Tag.TagName.ComparisonIndex == tag.TagName.ComparisonIndex)
				{
					contains = true;
					break;
				}
			}
			if (!contains) {
				AdditionalSourceTags.GameplayTags.Add(tag);
			}
		}

		for (auto tag : Source.ParentTags)
		{
			bool contains = false;
			for (auto Tag : AdditionalSourceTags.ParentTags)
			{
				if (Tag.TagName.ComparisonIndex == tag.TagName.ComparisonIndex)
				{
					contains = true;
					break;
				}
			}
			if (!contains)
				AdditionalSourceTags.ParentTags.Add(tag);
		}

		/*if (StatEvent == EFortQuestObjectiveStatEvent::Kill)
		{
			int Kills = ((AFortPlayerStateAthena*)ManagerComp->GetPlayerControllerBP()->PlayerState)->KillScore + 1;

			GiveAccolade((AFortPlayerControllerAthena*)PC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination"));
			GiveAccolade((AFortPlayerControllerAthena*)PC, GetDefFromEvent(EAccoladeEvent::Kill, Kills));
		}*/

		for (auto Quest : ManagerComp->CurrentQuests)
		{
			auto QuestDef = Quest->GetQuestDefinitionBP();
			if (!QuestDef) {
				Log("no QuestDef");
				continue;
			}

			if (ManagerComp->HasCompletedQuest(QuestDef))
				continue;

			auto PrereqQuest = QuestDef->GetPrerequisiteQuest();

			if (PrereqQuest && !ManagerComp->HasCompletedQuest(PrereqQuest))
				continue;

			if (QuestDef->PrerequisiteObjective.DataTable && QuestDef->PrerequisiteObjective.RowName.ComparisonIndex && !ManagerComp->HasCompletedObjective(QuestDef, QuestDef->PrerequisiteObjective))
				return;

			for (FFortMcpQuestObjectiveInfo& Objective : QuestDef->Objectives) {
				/*if (Objective.BackendName) {
					Log("BackendName dont exist!");
					continue;
				}*/

				if (/*Objective.InlineObjectiveStats.Num() <= 0 || */ManagerComp->HasCompletedObjectiveWithName(QuestDef, Objective.BackendName))
					continue;

				bool bFoundCorrectQuest = false;

				FDataTableRowHandle ObjectiveStatHandle = Objective.ObjectiveStatHandle;
				UDataTable* DataTable = ObjectiveStatHandle.DataTable;
				if (!DataTable)
					continue;

				auto& RowMap = DataTable->RowMap;
				FFortQuestObjectiveStatTableRow* Row = nullptr;

				if (RowMap.Num() <= 0)
					continue;

				for (int i = 0; i < RowMap.Num(); ++i)
				{
					auto& Pair = RowMap[i];
					if (Pair.Key().ToString() == ObjectiveStatHandle.RowName.ToString())
					{
						Log("Wow!");
						bFoundCorrectQuest = true;
						break;
					}
				}

				if (bFoundCorrectQuest) {
					return ProgressQuest(PC, QuestDef, Objective.BackendName);
				}
			}
		}
	}

	static inline void (*SendComplexCustomStatEventOG)(UFortQuestManager* ManagerComp, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count);
	void SendComplexCustomStatEvent(UFortQuestManager* ManagerComp, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count)
	{
		Log("SendComplexCustomStateEvent Called!");
		if (!ManagerComp || !Count)
			return;

		SendStatEvent(ManagerComp, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count, EFortQuestObjectiveStatEvent::ComplexCustom);
	}

	/*static inline void (*SendComplexCustomStatEventOG)(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count);
	static void SendComplexCustomStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count) {
		Log("SendComplexCustomStateEvent Called!");

		if (!QuestManager || !Count)
			return;

		if (__int64(_ReturnAddress()) == ImageBase + 0x2A009AC) {
			SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count, EFortQuestObjectiveStatEvent::ComplexCustom);
		}
		else
		{
			SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, nullptr, nullptr, 1, EFortQuestObjectiveStatEvent::ComplexCustom);
		}

		return SendComplexCustomStatEventOG(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count);
	}*/

	static bool GetQuestContext(UFortQuestManager* QuestManager, EFortQuestObjectiveStatEvent SE) {
		Log("test!");
		Log(QuestManager->GetPlayerControllerBP()->InteractionComponent->InteractActor->GetName());
		FGameplayTagContainer SourceTags;
		FGameplayTagContainer ContextTags;
		FGameplayTagContainer PlaylistContextTags;

		QuestManager->GetSourceAndContextTags(&SourceTags, &ContextTags);
		for (auto& Tag : SourceTags.GameplayTags)
			Log(Tag.TagName.ToString());
		return true;
	}

	void Hook() {
		//MH_CreateHook((LPVOID)(ImageBase + 0x23B1420), SendComplexCustomStatEvent, (LPVOID*)&SendComplexCustomStatEventOG);
		//MH_CreateHook((LPVOID)(ImageBase + 0x23A5180), GetQuestContext, nullptr);

		Log("Quests Hooked!");
	}
}