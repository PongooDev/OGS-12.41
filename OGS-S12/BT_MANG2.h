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
				bFinishedWait = true;
				return EBTNodeResult::Succeeded;
			}
			// If it is still waiting then return inprogress
			return EBTNodeResult::InProgress;
		}
	};

	class BTTask_SteerMovement : public BTNode {
	public:
		virtual EBTNodeResult ChildTask(BTContext Context) override {
			auto* BB = Context.Controller->Blackboard;
			FVector SteerDirection = BB->GetValueAsVector(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_CharacterLaunched_SteerDirection"));
			if (!SteerDirection.IsZero()) {
				EPathFollowingRequestResult Result = Context.Controller->MoveToLocation(SteerDirection, 10.f, true, false, false, true, nullptr, true);
				if (Result == EPathFollowingRequestResult::Failed) {
					return EBTNodeResult::Failed;
				}
				else if (Result == EPathFollowingRequestResult::RequestSuccessful) {
					return EBTNodeResult::InProgress;
				}
				else if (Result == EPathFollowingRequestResult::AlreadyAtGoal) {
					return EBTNodeResult::Succeeded;
				}
			}
			return EBTNodeResult::Failed;
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

	class BTDecorator_Blackboard_21 : public BTDecorator {
	public:
		BTDecorator_Blackboard_21() {
			Name = "BTDecorator_Blackboard_21";
			CachedDescription = "AIEvaluator_CharacterLaunched_ExecutionStatus Is Greater Than Or Equal To Execution Pending";
			NodeName = "Is Character being launched?";
		}

		virtual bool Evaluate(BTContext Context) override {
			auto* BB = Context.Controller->Blackboard;
			uint8 ExecStatus = BB->GetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_CharacterLaunched_ExecutionStatus"));
			if (ExecStatus >= 3) {
				return true;
			}
			return false;
		}
	};
}

namespace BT_MANG_Services {
	class BTService_AIEvaluator_4 : public BTService {
	public:
		BTService_AIEvaluator_4() {
			Name = "FortAthenaBTService_AIEvaluator_4";
			NodeName = "Focus Scan Around Only";

			Interval = 0.03f;
			NextTickTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) + Interval;
		}

		virtual void TickService(BTContext Context) override {
			float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

			if (CurrentTime >= NextTickTime) {
				//Log("BTService_AIEvaluator_4");
				FVector CurrentLoc = Context.Pawn->K2_GetActorLocation();
				FVector LookLoc = FVector((CurrentLoc.X + (UKismetMathLibrary::RandomFloatInRange(-3.f, 3.f))),
					(CurrentLoc.Y + (UKismetMathLibrary::RandomFloatInRange(-3.f, 3.f))), CurrentLoc.Z);

				Context.Controller->K2_SetFocalPoint(LookLoc);

				NextTickTime = CurrentTime + Interval;
			}
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

		{
			auto* Task = new BT_MANG2_Tasks::BTTask_Wait(2.f);
			Task->AddDecorator(new BT_MANG_Decorators::BTDecorator_Blackboard_5());
			Task->AddDecorator(new BT_MANG_Decorators::BTDecorator_Loop());
			RootSelector->AddChild(Task);
		}

		{
			auto* Task = new BT_MANG2_Tasks::BTTask_SteerMovement();
			Task->AddDecorator(new BT_MANG_Decorators::BTDecorator_Blackboard_21());
			Task->AddService(new BT_MANG_Services::BTService_AIEvaluator_4());
			RootSelector->AddChild(Task);
		}

		Tree->RootNode = RootSelector;
		Tree->AllNodes.push_back(RootSelector);

		BT_MANG2 = Tree;
		return Tree;
	}
}