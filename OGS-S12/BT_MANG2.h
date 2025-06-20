#pragma once
#include "framework.h"

#include "BehaviourTree_System.h"

// btw if you didnt know BT_MANG2 the 2 means 2.0 (atleast i think so)

namespace BT_MANG2_Tasks {
	class BTTask_Wait : public BTNode {
	public:
		float WaitTime = 0.f;
		float WorldWaitTime = 0.f;
		bool bFinishedWait = false;
	public:
		BTTask_Wait(float InWaitTime) {
			WaitTime = InWaitTime;
			WorldWaitTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) + InWaitTime;
		}

		virtual EBTNodeResult ChildTask(BTContext Context) override {
			if (WaitTime == 0.f || WorldWaitTime == 0.f) {
				return EBTNodeResult::Failed;
			}
			if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) >= WorldWaitTime) {
				// Reset the wait if the wait is completed
				if (bFinishedWait) {
					WorldWaitTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) + WaitTime;
					bFinishedWait = false;
					return EBTNodeResult::InProgress;
				}
				Log("WaitTime over!");
				bFinishedWait = true;
				return EBTNodeResult::Succeeded;
			}
			// If it is still waiting then return inprogress
			return EBTNodeResult::InProgress;
		}
	};
}

namespace BT_MANG_Decorators {
	class BTDecorator_Loop : public BTDecorator {
	public:
		virtual bool Evaluate(BTContext Context) override {
			// All of the loop decorators that i saw are infinite loops so...
			return true;
		}
	};

	class BTDecorator_Blackboard_5 : public BTDecorator {
	public:
		BTDecorator_Blackboard_5() {
			Name = "BTDecorator_Blackboard_5";
			CachedDescription = "AIEvaluator_Global_IsMovementBlocked is Is Set";
			NodeName = "Is Movement Blocked?";
		}

		virtual bool Evaluate(BTContext Context) override {
			auto* BB = Context.Controller->Blackboard;
			return BB->GetValueAsBool(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_Global_IsMovementBlocked"));
		}
	};
}

namespace BT_MANG2 {
	// Cached Constructed BT_MANG2 BT
	static BehaviorTree* BT_MANG2 = nullptr;

	// Construct the behaviortree and cache it
	BehaviorTree* ConstructBehaviorTree() {
		auto* Tree = new BehaviorTree();

		auto* RootSelector = new BTComposite_Selector();
		RootSelector->Name = "BTComposite_Selector_2";

		auto* WaitTask = new BT_MANG2_Tasks::BTTask_Wait(2.f);
		WaitTask->AddDecorator(new BT_MANG_Decorators::BTDecorator_Blackboard_5());
		WaitTask->AddDecorator(new BT_MANG_Decorators::BTDecorator_Loop());

		RootSelector->AddChild(WaitTask);

		Tree->RootNode = RootSelector;
		Tree->AllNodes.push_back(RootSelector);

		BT_MANG2 = Tree;
		return Tree;
	}
}