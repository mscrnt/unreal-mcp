#include "Commands/UnrealMCPAnimBlueprintCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimStateEntryNode.h"
#include "AnimationGraphSchema.h"
#include "AnimationTransitionGraph.h"
#include "AnimGraphNode_TransitionResult.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"
#include "K2Node_CallFunction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Kismet/KismetMathLibrary.h"

FUnrealMCPAnimBlueprintCommands::FUnrealMCPAnimBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_anim_blueprint"))
    {
        return HandleCreateAnimBlueprint(Params);
    }
    else if (CommandType == TEXT("add_anim_state_machine"))
    {
        return HandleAddAnimStateMachine(Params);
    }
    else if (CommandType == TEXT("add_anim_state"))
    {
        return HandleAddAnimState(Params);
    }
    else if (CommandType == TEXT("set_anim_state_animation"))
    {
        return HandleSetAnimStateAnimation(Params);
    }
    else if (CommandType == TEXT("add_anim_transition"))
    {
        return HandleAddAnimTransition(Params);
    }
    else if (CommandType == TEXT("set_anim_transition_rule"))
    {
        return HandleSetAnimTransitionRule(Params);
    }
    else if (CommandType == TEXT("get_anim_blueprint_info"))
    {
        return HandleGetAnimBlueprintInfo(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown anim blueprint command: %s"), *CommandType));
}

// Helper: find AnimBlueprint by name using asset registry (same strategy as FindBlueprintByName)
static UAnimBlueprint* FindAnimBlueprint(const FString& Name)
{
    // Try full path first
    if (Name.StartsWith(TEXT("/")))
    {
        UAnimBlueprint* ABP = LoadObject<UAnimBlueprint>(nullptr, *Name);
        if (ABP) return ABP;

        FString BaseName = FPaths::GetBaseFilename(Name);
        ABP = LoadObject<UAnimBlueprint>(nullptr, *(Name + TEXT(".") + BaseName));
        if (ABP) return ABP;
    }

    // Search asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(UAnimBlueprint::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString() == Name)
        {
            return Cast<UAnimBlueprint>(AssetData.GetAsset());
        }
    }

    return nullptr;
}

// Helper: find the main animation graph in an AnimBlueprint
static UEdGraph* FindAnimGraph(UAnimBlueprint* AnimBP)
{
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName().Contains(TEXT("AnimGraph")))
        {
            return Graph;
        }
    }
    return nullptr;
}

// Helper: find a state machine node by name in the anim graph
static UAnimGraphNode_StateMachine* FindStateMachineNode(UAnimBlueprint* AnimBP, const FString& Name)
{
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (!AnimGraph) return nullptr;

    for (UEdGraphNode* Node : AnimGraph->Nodes)
    {
        UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
        if (SMNode && SMNode->GetStateMachineName() == Name)
        {
            return SMNode;
        }
    }
    return nullptr;
}

// Helper: find a state node by name in a state machine graph
static UAnimStateNode* FindStateNode(UAnimationStateMachineGraph* SMGraph, const FString& StateName)
{
    for (UEdGraphNode* Node : SMGraph->Nodes)
    {
        UAnimStateNode* StateNode = Cast<UAnimStateNode>(Node);
        if (StateNode && StateNode->GetStateName() == StateName)
        {
            return StateNode;
        }
    }
    return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString SkeletonPath;
    if (!Params->TryGetStringField(TEXT("skeleton"), SkeletonPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'skeleton' parameter (path to USkeleton asset)"));
    }

    FString Path = TEXT("/Game/");
    Params->TryGetStringField(TEXT("path"), Path);
    if (!Path.EndsWith(TEXT("/"))) Path += TEXT("/");

    // Load the skeleton
    USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath);
    if (!Skeleton)
    {
        // Try with .SkeletonName format
        FString BaseName = FPaths::GetBaseFilename(SkeletonPath);
        Skeleton = LoadObject<USkeleton>(nullptr, *(SkeletonPath + TEXT(".") + BaseName));
    }
    if (!Skeleton)
    {
        // Search asset registry for skeleton by name
        FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> Assets;
        ARM.Get().GetAssetsByClass(USkeleton::StaticClass()->GetClassPathName(), Assets);
        for (const FAssetData& AD : Assets)
        {
            if (AD.AssetName.ToString() == FPaths::GetBaseFilename(SkeletonPath) ||
                AD.GetObjectPathString().Contains(SkeletonPath))
            {
                Skeleton = Cast<USkeleton>(AD.GetAsset());
                if (Skeleton) break;
            }
        }
    }
    if (!Skeleton)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Skeleton not found: '%s'. Use full path like '/Game/Characters/Mannequin/Mesh/SK_Mannequin'"), *SkeletonPath));
    }

    // Create the AnimBlueprint using factory
    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->TargetSkeleton = Skeleton;
    Factory->ParentClass = UAnimInstance::StaticClass();

    FString PackagePath = Path + Name;
    UPackage* Package = CreatePackage(*PackagePath);

    UObject* NewAsset = Factory->FactoryCreateNew(
        UAnimBlueprint::StaticClass(),
        Package,
        *Name,
        RF_Standalone | RF_Public,
        nullptr,
        GWarn
    );

    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(NewAsset);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Animation Blueprint"));
    }

    FAssetRegistryModule::AssetCreated(AnimBP);
    Package->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), Name);
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    ResultObj->SetStringField(TEXT("skeleton"), Skeleton->GetPathName());
    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleAddAnimStateMachine(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_blueprint"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint' parameter"));
    }

    FString MachineName = TEXT("Locomotion");
    Params->TryGetStringField(TEXT("machine_name"), MachineName);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AnimBPName));
    }

    // Find the anim graph
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (!AnimGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimBlueprint has no AnimGraph"));
    }

    // Create state machine node
    UAnimGraphNode_StateMachine* SMNode = NewObject<UAnimGraphNode_StateMachine>(AnimGraph);
    if (!SMNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create StateMachine node"));
    }

    SMNode->NodePosX = 200;
    SMNode->NodePosY = 0;
    AnimGraph->AddNode(SMNode, true, false);
    SMNode->CreateNewGuid();
    SMNode->PostPlacedNewNode();
    SMNode->AllocateDefaultPins();

    // Rename the state machine
    FBlueprintEditorUtils::RenameGraph(SMNode->EditorStateMachineGraph, MachineName);

    // Try to connect to the root node's pose input
    UAnimGraphNode_Root* RootNode = nullptr;
    for (UEdGraphNode* Node : AnimGraph->Nodes)
    {
        RootNode = Cast<UAnimGraphNode_Root>(Node);
        if (RootNode) break;
    }

    if (RootNode)
    {
        // Find the output pose pin on the state machine and input pose pin on root
        UEdGraphPin* SMOutputPin = nullptr;
        for (UEdGraphPin* Pin : SMNode->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                SMOutputPin = Pin;
                break;
            }
        }

        UEdGraphPin* RootInputPin = nullptr;
        for (UEdGraphPin* Pin : RootNode->Pins)
        {
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                RootInputPin = Pin;
                break;
            }
        }

        if (SMOutputPin && RootInputPin)
        {
            SMOutputPin->MakeLinkTo(RootInputPin);
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("machine_name"), MachineName);
    ResultObj->SetStringField(TEXT("node_id"), SMNode->NodeGuid.ToString());
    ResultObj->SetBoolField(TEXT("connected_to_root"), RootNode != nullptr);
    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleAddAnimState(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_blueprint"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint' parameter"));
    }

    FString MachineName = TEXT("Locomotion");
    Params->TryGetStringField(TEXT("machine_name"), MachineName);

    FString StateName;
    if (!Params->TryGetStringField(TEXT("state_name"), StateName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'state_name' parameter"));
    }

    bool bIsDefault = false;
    Params->TryGetBoolField(TEXT("is_default"), bIsDefault);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AnimBPName));
    }

    // Find the state machine
    UAnimGraphNode_StateMachine* SMNode = FindStateMachineNode(AnimBP, MachineName);
    if (!SMNode || !SMNode->EditorStateMachineGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State machine '%s' not found"), *MachineName));
    }

    UAnimationStateMachineGraph* SMGraph = SMNode->EditorStateMachineGraph;

    // Get position from params or auto-layout
    float PosX = 300.0f;
    float PosY = 0.0f;
    if (Params->HasField(TEXT("position")))
    {
        const TArray<TSharedPtr<FJsonValue>>& PosArr = Params->GetArrayField(TEXT("position"));
        if (PosArr.Num() >= 2)
        {
            PosX = PosArr[0]->AsNumber();
            PosY = PosArr[1]->AsNumber();
        }
    }
    else
    {
        // Auto-position based on existing state count
        int32 StateCount = 0;
        for (UEdGraphNode* N : SMGraph->Nodes)
        {
            if (Cast<UAnimStateNode>(N)) StateCount++;
        }
        PosX = 300.0f + (StateCount % 3) * 300.0f;
        PosY = (StateCount / 3) * 200.0f;
    }

    // Create state node
    UAnimStateNode* StateNode = NewObject<UAnimStateNode>(SMGraph);
    StateNode->NodePosX = (int32)PosX;
    StateNode->NodePosY = (int32)PosY;
    SMGraph->AddNode(StateNode, true, false);
    StateNode->CreateNewGuid();
    StateNode->PostPlacedNewNode();
    StateNode->AllocateDefaultPins();

    // Rename the state
    StateNode->Rename(*StateName);
    if (StateNode->BoundGraph)
    {
        FBlueprintEditorUtils::RenameGraph(StateNode->BoundGraph, StateName);
    }

    // If this is the default/entry state, connect from entry node
    if (bIsDefault && SMGraph->EntryNode)
    {
        UEdGraphPin* EntryOutput = nullptr;
        for (UEdGraphPin* Pin : SMGraph->EntryNode->Pins)
        {
            if (Pin->Direction == EGPD_Output)
            {
                EntryOutput = Pin;
                break;
            }
        }

        UEdGraphPin* StateInput = nullptr;
        for (UEdGraphPin* Pin : StateNode->Pins)
        {
            if (Pin->Direction == EGPD_Input)
            {
                StateInput = Pin;
                break;
            }
        }

        if (EntryOutput && StateInput)
        {
            EntryOutput->MakeLinkTo(StateInput);
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("state_name"), StateName);
    ResultObj->SetStringField(TEXT("node_id"), StateNode->NodeGuid.ToString());
    ResultObj->SetBoolField(TEXT("is_default"), bIsDefault);
    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleSetAnimStateAnimation(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_blueprint"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint' parameter"));
    }

    FString MachineName = TEXT("Locomotion");
    Params->TryGetStringField(TEXT("machine_name"), MachineName);

    FString StateName;
    if (!Params->TryGetStringField(TEXT("state_name"), StateName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'state_name' parameter"));
    }

    FString AnimationPath;
    if (!Params->TryGetStringField(TEXT("animation"), AnimationPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'animation' parameter (path to AnimSequence)"));
    }

    bool bLooping = true;
    Params->TryGetBoolField(TEXT("looping"), bLooping);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AnimBPName));
    }

    // Find the state machine and state
    UAnimGraphNode_StateMachine* SMNode = FindStateMachineNode(AnimBP, MachineName);
    if (!SMNode || !SMNode->EditorStateMachineGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State machine '%s' not found"), *MachineName));
    }

    UAnimStateNode* StateNode = FindStateNode(SMNode->EditorStateMachineGraph, StateName);
    if (!StateNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State '%s' not found in machine '%s'"), *StateName, *MachineName));
    }

    // Load the animation asset
    UAnimSequence* AnimSequence = LoadObject<UAnimSequence>(nullptr, *AnimationPath);
    if (!AnimSequence)
    {
        FString BaseName = FPaths::GetBaseFilename(AnimationPath);
        AnimSequence = LoadObject<UAnimSequence>(nullptr, *(AnimationPath + TEXT(".") + BaseName));
    }
    if (!AnimSequence)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Animation not found: '%s'. Use full path like '/Game/Characters/Animations/Idle'"), *AnimationPath));
    }

    // Get the state's bound graph (the animation graph inside the state)
    UEdGraph* StateGraph = StateNode->BoundGraph;
    if (!StateGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State '%s' has no bound graph"), *StateName));
    }

    // Find or create the sequence player node in the state graph
    UAnimGraphNode_SequencePlayer* SeqPlayer = nullptr;
    for (UEdGraphNode* Node : StateGraph->Nodes)
    {
        SeqPlayer = Cast<UAnimGraphNode_SequencePlayer>(Node);
        if (SeqPlayer) break;
    }

    if (!SeqPlayer)
    {
        // Create new sequence player
        SeqPlayer = NewObject<UAnimGraphNode_SequencePlayer>(StateGraph);
        SeqPlayer->NodePosX = 0;
        SeqPlayer->NodePosY = 0;
        StateGraph->AddNode(SeqPlayer, true, false);
        SeqPlayer->CreateNewGuid();
        SeqPlayer->PostPlacedNewNode();
        SeqPlayer->AllocateDefaultPins();
    }

    // Set the animation
    SeqPlayer->SetAnimationAsset(AnimSequence);

    // Try to connect to the state result node
    UAnimGraphNode_StateResult* ResultNode = nullptr;
    for (UEdGraphNode* Node : StateGraph->Nodes)
    {
        ResultNode = Cast<UAnimGraphNode_StateResult>(Node);
        if (ResultNode) break;
    }

    if (ResultNode)
    {
        // Connect sequence player output to result input
        UEdGraphPin* PlayerOutputPin = nullptr;
        for (UEdGraphPin* Pin : SeqPlayer->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                PlayerOutputPin = Pin;
                break;
            }
        }

        UEdGraphPin* ResultInputPin = nullptr;
        for (UEdGraphPin* Pin : ResultNode->Pins)
        {
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                ResultInputPin = Pin;
                break;
            }
        }

        if (PlayerOutputPin && ResultInputPin)
        {
            // Break existing links first
            ResultInputPin->BreakAllPinLinks();
            PlayerOutputPin->MakeLinkTo(ResultInputPin);
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("state_name"), StateName);
    ResultObj->SetStringField(TEXT("animation"), AnimSequence->GetPathName());
    ResultObj->SetBoolField(TEXT("connected_to_result"), ResultNode != nullptr);
    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleAddAnimTransition(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_blueprint"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint' parameter"));
    }

    FString MachineName = TEXT("Locomotion");
    Params->TryGetStringField(TEXT("machine_name"), MachineName);

    FString FromState;
    if (!Params->TryGetStringField(TEXT("from_state"), FromState))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_state' parameter"));
    }

    FString ToState;
    if (!Params->TryGetStringField(TEXT("to_state"), ToState))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_state' parameter"));
    }

    float Duration = 0.2f;
    if (Params->HasField(TEXT("duration")))
    {
        Duration = (float)Params->GetNumberField(TEXT("duration"));
    }

    bool bAutomatic = false;
    Params->TryGetBoolField(TEXT("automatic"), bAutomatic);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AnimBPName));
    }

    UAnimGraphNode_StateMachine* SMNode = FindStateMachineNode(AnimBP, MachineName);
    if (!SMNode || !SMNode->EditorStateMachineGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State machine '%s' not found"), *MachineName));
    }

    UAnimationStateMachineGraph* SMGraph = SMNode->EditorStateMachineGraph;

    UAnimStateNode* FromNode = FindStateNode(SMGraph, FromState);
    if (!FromNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("From state '%s' not found"), *FromState));
    }

    UAnimStateNode* ToNode = FindStateNode(SMGraph, ToState);
    if (!ToNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("To state '%s' not found"), *ToState));
    }

    // Create transition node
    UAnimStateTransitionNode* TransNode = NewObject<UAnimStateTransitionNode>(SMGraph);
    TransNode->NodePosX = (FromNode->NodePosX + ToNode->NodePosX) / 2;
    TransNode->NodePosY = (FromNode->NodePosY + ToNode->NodePosY) / 2;
    SMGraph->AddNode(TransNode, true, false);
    TransNode->CreateNewGuid();
    TransNode->PostPlacedNewNode();
    TransNode->AllocateDefaultPins();

    // Connect the transition between states
    TransNode->CreateConnections(FromNode, ToNode);

    // Set transition properties
    TransNode->CrossfadeDuration = Duration;

    if (bAutomatic)
    {
        TransNode->bAutomaticRuleBasedOnSequencePlayerInState = true;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("from_state"), FromState);
    ResultObj->SetStringField(TEXT("to_state"), ToState);
    ResultObj->SetStringField(TEXT("node_id"), TransNode->NodeGuid.ToString());
    ResultObj->SetNumberField(TEXT("crossfade_duration"), Duration);
    ResultObj->SetBoolField(TEXT("automatic"), bAutomatic);
    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

// Helper: find a transition between two named states
static UAnimStateTransitionNode* FindTransitionNode(UAnimationStateMachineGraph* SMGraph, const FString& FromState, const FString& ToState)
{
    UAnimStateNode* FromNode = FindStateNode(SMGraph, FromState);
    if (!FromNode) return nullptr;

    TArray<UAnimStateTransitionNode*> Transitions;
    FromNode->GetTransitionList(Transitions);

    for (UAnimStateTransitionNode* Trans : Transitions)
    {
        UAnimStateNodeBase* NextState = Trans->GetNextState();
        if (NextState && NextState->GetStateName() == ToState)
        {
            return Trans;
        }
    }
    return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleSetAnimTransitionRule(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_blueprint"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint' parameter"));
    }

    FString MachineName = TEXT("Locomotion");
    Params->TryGetStringField(TEXT("machine_name"), MachineName);

    FString FromState;
    if (!Params->TryGetStringField(TEXT("from_state"), FromState))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_state' parameter"));
    }

    FString ToState;
    if (!Params->TryGetStringField(TEXT("to_state"), ToState))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_state' parameter"));
    }

    // Rule definition: variable_name, operator (>, <, >=, <=, ==, !=), value
    // For bool: just variable_name (optionally with negate)
    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString Operator;
    Params->TryGetStringField(TEXT("operator"), Operator); // empty for bool check

    bool bNegate = false;
    Params->TryGetBoolField(TEXT("negate"), bNegate);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AnimBPName));
    }

    UAnimGraphNode_StateMachine* SMNode = FindStateMachineNode(AnimBP, MachineName);
    if (!SMNode || !SMNode->EditorStateMachineGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State machine '%s' not found"), *MachineName));
    }

    UAnimStateTransitionNode* TransNode = FindTransitionNode(SMNode->EditorStateMachineGraph, FromState, ToState);
    if (!TransNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("No transition from '%s' to '%s' found"), *FromState, *ToState));
    }

    // Get the transition's bound graph (the rule graph)
    UAnimationTransitionGraph* TransGraph = Cast<UAnimationTransitionGraph>(TransNode->BoundGraph);
    if (!TransGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Transition has no rule graph"));
    }

    // Find the result node
    UAnimGraphNode_TransitionResult* ResultNode = TransGraph->GetResultNode();
    if (!ResultNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Transition graph has no result node"));
    }

    // Find the "bCanEnterTransition" input pin on the result node
    UEdGraphPin* CanEnterPin = nullptr;
    for (UEdGraphPin* Pin : ResultNode->Pins)
    {
        if (Pin->Direction == EGPD_Input && Pin->PinName.ToString().Contains(TEXT("bCanEnterTransition")))
        {
            CanEnterPin = Pin;
            break;
        }
    }
    // Fallback: find any bool input pin
    if (!CanEnterPin)
    {
        for (UEdGraphPin* Pin : ResultNode->Pins)
        {
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
            {
                CanEnterPin = Pin;
                break;
            }
        }
    }
    if (!CanEnterPin)
    {
        // List available pins for debugging
        FString PinList;
        for (UEdGraphPin* Pin : ResultNode->Pins)
        {
            PinList += FString::Printf(TEXT("[%s dir=%d cat=%s] "),
                *Pin->PinName.ToString(),
                (int)Pin->Direction,
                *Pin->PinType.PinCategory.ToString());
        }
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Cannot find 'bCanEnterTransition' pin on result node. Available pins: %s"), *PinList));
    }

    // Break existing connections
    CanEnterPin->BreakAllPinLinks();

    FString RuleDescription;

    if (Operator.IsEmpty())
    {
        // Bool variable check: connect variable directly to result
        // Create VariableGet node for the bool variable
        UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(TransGraph);
        VarGetNode->VariableReference.SetSelfMember(FName(*VariableName));
        VarGetNode->NodePosX = ResultNode->NodePosX - 300;
        VarGetNode->NodePosY = ResultNode->NodePosY;
        TransGraph->AddNode(VarGetNode, true, false);
        VarGetNode->CreateNewGuid();
        VarGetNode->PostPlacedNewNode();
        VarGetNode->AllocateDefaultPins();

        UEdGraphPin* VarOutputPin = VarGetNode->GetValuePin();

        if (bNegate)
        {
            // Add NOT node
            UK2Node_CallFunction* NotNode = NewObject<UK2Node_CallFunction>(TransGraph);
            NotNode->FunctionReference.SetExternalMember(
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool),
                UKismetMathLibrary::StaticClass());
            NotNode->NodePosX = ResultNode->NodePosX - 150;
            NotNode->NodePosY = ResultNode->NodePosY;
            TransGraph->AddNode(NotNode, true, false);
            NotNode->CreateNewGuid();
            NotNode->PostPlacedNewNode();
            NotNode->AllocateDefaultPins();

            // Connect var -> NOT input
            UEdGraphPin* NotInputPin = nullptr;
            UEdGraphPin* NotOutputPin = nullptr;
            for (UEdGraphPin* Pin : NotNode->Pins)
            {
                if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                    NotInputPin = Pin;
                if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                    NotOutputPin = Pin;
            }

            if (VarOutputPin && NotInputPin) VarOutputPin->MakeLinkTo(NotInputPin);
            if (NotOutputPin) NotOutputPin->MakeLinkTo(CanEnterPin);

            RuleDescription = FString::Printf(TEXT("NOT %s"), *VariableName);
        }
        else
        {
            if (VarOutputPin) VarOutputPin->MakeLinkTo(CanEnterPin);
            RuleDescription = VariableName;
        }
    }
    else
    {
        // Comparison: variable <op> value
        // Parse the compare value
        double CompareValue = 0.0;
        if (Params->HasField(TEXT("value")))
        {
            CompareValue = Params->GetNumberField(TEXT("value"));
        }

        // Determine which comparison function to use
        FName CompFunctionName;
        if (Operator == TEXT(">")) CompFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble);
        else if (Operator == TEXT("<")) CompFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble);
        else if (Operator == TEXT(">=")) CompFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble);
        else if (Operator == TEXT("<=")) CompFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble);
        else if (Operator == TEXT("==")) CompFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_DoubleDouble);
        else if (Operator == TEXT("!=")) CompFunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_DoubleDouble);
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Unknown operator '%s'. Supported: >, <, >=, <=, ==, !="), *Operator));
        }

        // Create VariableGet node
        UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(TransGraph);
        VarGetNode->VariableReference.SetSelfMember(FName(*VariableName));
        VarGetNode->NodePosX = ResultNode->NodePosX - 500;
        VarGetNode->NodePosY = ResultNode->NodePosY;
        TransGraph->AddNode(VarGetNode, true, false);
        VarGetNode->CreateNewGuid();
        VarGetNode->PostPlacedNewNode();
        VarGetNode->AllocateDefaultPins();

        // Create comparison function node
        UK2Node_CallFunction* CompNode = NewObject<UK2Node_CallFunction>(TransGraph);
        CompNode->FunctionReference.SetExternalMember(CompFunctionName, UKismetMathLibrary::StaticClass());
        CompNode->NodePosX = ResultNode->NodePosX - 250;
        CompNode->NodePosY = ResultNode->NodePosY;
        TransGraph->AddNode(CompNode, true, false);
        CompNode->CreateNewGuid();
        CompNode->PostPlacedNewNode();
        CompNode->AllocateDefaultPins();

        // Connect variable to first input (A) of comparison
        UEdGraphPin* VarOutputPin = VarGetNode->GetValuePin();
        UEdGraphPin* CompInputA = nullptr;
        UEdGraphPin* CompInputB = nullptr;
        UEdGraphPin* CompOutputPin = nullptr;

        for (UEdGraphPin* Pin : CompNode->Pins)
        {
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
            {
                if (!CompInputA) CompInputA = Pin;
                else if (!CompInputB) CompInputB = Pin;
            }
            // Also check for PC_Float (older UE versions)
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
            {
                if (!CompInputA) CompInputA = Pin;
                else if (!CompInputB) CompInputB = Pin;
            }
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
            {
                CompOutputPin = Pin;
            }
        }

        if (VarOutputPin && CompInputA) VarOutputPin->MakeLinkTo(CompInputA);
        if (CompInputB) CompInputB->DefaultValue = FString::SanitizeFloat(CompareValue);
        if (CompOutputPin) CompOutputPin->MakeLinkTo(CanEnterPin);

        RuleDescription = FString::Printf(TEXT("%s %s %g"), *VariableName, *Operator, CompareValue);
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("from_state"), FromState);
    ResultObj->SetStringField(TEXT("to_state"), ToState);
    ResultObj->SetStringField(TEXT("rule"), RuleDescription);
    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAnimBlueprintCommands::HandleGetAnimBlueprintInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_blueprint"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint' parameter"));
    }

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AnimBPName));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), AnimBP->GetName());
    ResultObj->SetStringField(TEXT("path"), AnimBP->GetPathName());
    ResultObj->SetStringField(TEXT("skeleton"), AnimBP->TargetSkeleton ? AnimBP->TargetSkeleton->GetPathName() : TEXT("None"));

    // List state machines
    TArray<TSharedPtr<FJsonValue>> MachinesArr;
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (AnimGraph)
    {
        for (UEdGraphNode* Node : AnimGraph->Nodes)
        {
            UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
            if (!SMNode) continue;

            TSharedPtr<FJsonObject> SMObj = MakeShared<FJsonObject>();
            SMObj->SetStringField(TEXT("name"), SMNode->GetStateMachineName());
            SMObj->SetStringField(TEXT("node_id"), SMNode->NodeGuid.ToString());

            // List states
            if (SMNode->EditorStateMachineGraph)
            {
                TArray<TSharedPtr<FJsonValue>> StatesArr;
                for (UEdGraphNode* SMChild : SMNode->EditorStateMachineGraph->Nodes)
                {
                    UAnimStateNode* StateNode = Cast<UAnimStateNode>(SMChild);
                    if (!StateNode) continue;

                    TSharedPtr<FJsonObject> StateObj = MakeShared<FJsonObject>();
                    StateObj->SetStringField(TEXT("name"), StateNode->GetStateName());
                    StateObj->SetStringField(TEXT("node_id"), StateNode->NodeGuid.ToString());

                    // Check if it's the entry state
                    bool bIsEntry = false;
                    if (SMNode->EditorStateMachineGraph->EntryNode)
                    {
                        for (UEdGraphPin* Pin : SMNode->EditorStateMachineGraph->EntryNode->Pins)
                        {
                            for (UEdGraphPin* Linked : Pin->LinkedTo)
                            {
                                if (Linked->GetOwningNode() == StateNode)
                                {
                                    bIsEntry = true;
                                    break;
                                }
                            }
                        }
                    }
                    StateObj->SetBoolField(TEXT("is_entry"), bIsEntry);

                    // List transitions from this state
                    TArray<UAnimStateTransitionNode*> Transitions;
                    StateNode->GetTransitionList(Transitions);
                    TArray<TSharedPtr<FJsonValue>> TransArr;
                    for (UAnimStateTransitionNode* Trans : Transitions)
                    {
                        TSharedPtr<FJsonObject> TransObj = MakeShared<FJsonObject>();
                        UAnimStateNodeBase* NextState = Trans->GetNextState();
                        TransObj->SetStringField(TEXT("to"), NextState ? NextState->GetStateName() : TEXT("Unknown"));
                        TransObj->SetNumberField(TEXT("duration"), Trans->CrossfadeDuration);
                        TransArr.Add(MakeShared<FJsonValueObject>(TransObj));
                    }
                    StateObj->SetArrayField(TEXT("transitions"), TransArr);

                    StatesArr.Add(MakeShared<FJsonValueObject>(StateObj));
                }
                SMObj->SetArrayField(TEXT("states"), StatesArr);
            }

            MachinesArr.Add(MakeShared<FJsonValueObject>(SMObj));
        }
    }
    ResultObj->SetArrayField(TEXT("state_machines"), MachinesArr);

    // List all graphs
    TArray<TSharedPtr<FJsonValue>> GraphsArr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph)
        {
            TSharedPtr<FJsonObject> GObj = MakeShared<FJsonObject>();
            GObj->SetStringField(TEXT("name"), Graph->GetName());
            GObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
            GraphsArr.Add(MakeShared<FJsonValueObject>(GObj));
        }
    }
    ResultObj->SetArrayField(TEXT("graphs"), GraphsArr);

    return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}
