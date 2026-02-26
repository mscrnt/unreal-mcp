#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// World Utilities - PIE-aware

UWorld* FUnrealMCPCommonUtils::GetTargetWorld(bool bPreferPIE)
{
    if (bPreferPIE && GEditor)
    {
        // Look for an active PIE world first, but skip worlds being torn down
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE && Context.World())
            {
                UWorld* PIEWorld = Context.World();
                // Reject worlds that are being destroyed (prevents crash during PIE teardown)
                if (PIEWorld->HasAnyFlags(RF_BeginDestroyed) || !PIEWorld->bIsWorldInitialized)
                {
                    continue;
                }
                return PIEWorld;
            }
        }
    }

    // Fall back to editor world
    if (GEditor)
    {
        return GEditor->GetEditorWorldContext().World();
    }

    return GWorld;
}

AActor* FUnrealMCPCommonUtils::FindActorByName(const FString& ActorName, bool bPreferPIE)
{
    UWorld* World = GetTargetWorld(bPreferPIE);
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && (Actor->GetName() == ActorName || Actor->GetActorLabel() == ActorName))
        {
            return Actor;
        }
    }

    // Partial match fallback
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && (Actor->GetName().Contains(ActorName) || Actor->GetActorLabel().Contains(ActorName)))
        {
            return Actor;
        }
    }

    return nullptr;
}

// JSON Utilities
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (Data.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("data"), Data);
    }
    
    return ResponseObject;
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    // 1. If it looks like a full path, try it directly
    if (BlueprintName.StartsWith(TEXT("/")) || BlueprintName.Contains(TEXT(".")))
    {
        UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintName);
        if (BP) return BP;

        // Try Package.AssetName format
        FString BaseName = FPaths::GetBaseFilename(BlueprintName);
        FString FullPath = BlueprintName + TEXT(".") + BaseName;
        BP = LoadObject<UBlueprint>(nullptr, *FullPath);
        if (BP) return BP;
    }

    // 2. Try /Game/Blueprints/ (legacy default path)
    {
        FString AssetPath = TEXT("/Game/Blueprints/") + BlueprintName;
        UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
        if (BP) return BP;
    }

    // 3. Search asset registry for any Blueprint with this name anywhere under /Game/
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString() == BlueprintName)
        {
            UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset());
            if (BP)
            {
                UE_LOG(LogTemp, Display, TEXT("FindBlueprintByName: Found '%s' at '%s' via asset registry"),
                    *BlueprintName, *AssetData.GetObjectPathString());
                return BP;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindBlueprintByName: Blueprint '%s' not found in any location"), *BlueprintName);
    return nullptr;
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"), 
            *EventName, *EventNode->NodeGuid.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableGetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();
        
        return VariableGetNode;
    }
    
    return nullptr;
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableSetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();
        
        return VariableSetNode;
    }
    
    return nullptr;
}

UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    InputActionNode->InputActionName = FName(*ActionName);
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    Graph->AddNode(InputActionNode, true);
    InputActionNode->CreateNewGuid();
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    return InputActionNode;
}

UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }
    
    return false;
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"), 
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }
    
    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) && 
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

// Actor utilities
TSharedPtr<FJsonValue> FUnrealMCPCommonUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return MakeShared<FJsonValueObject>(ActorObject);
}

// Helper to convert a UProperty value to a JSON value for serialization
static TSharedPtr<FJsonValue> PropertyToJsonValue(FProperty* Property, const void* ValuePtr)
{
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(ValuePtr));
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        return MakeShared<FJsonValueNumber>(IntProp->GetPropertyValue(ValuePtr));
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        return MakeShared<FJsonValueNumber>(FloatProp->GetPropertyValue(ValuePtr));
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        return MakeShared<FJsonValueNumber>(DoubleProp->GetPropertyValue(ValuePtr));
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(ValuePtr));
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(ValuePtr).ToString());
    }
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        return MakeShared<FJsonValueString>(TextProp->GetPropertyValue(ValuePtr).ToString());
    }
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProp->GetEnum();
        FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
        int64 EnumValue = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
        FString EnumName = Enum ? Enum->GetNameStringByValue(EnumValue) : FString::FromInt(EnumValue);
        return MakeShared<FJsonValueString>(EnumName);
    }
    else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (ByteProp->IsEnum())
        {
            uint8 ByteValue = ByteProp->GetPropertyValue(ValuePtr);
            FString EnumName = ByteProp->Enum->GetNameStringByValue(ByteValue);
            return MakeShared<FJsonValueString>(EnumName);
        }
        return MakeShared<FJsonValueNumber>(ByteProp->GetPropertyValue(ValuePtr));
    }
    else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
    {
        UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
        return MakeShared<FJsonValueString>(Obj ? Obj->GetPathName() : TEXT("None"));
    }
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        UClass* ClassValue = Cast<UClass>(ClassProp->GetObjectPropertyValue(ValuePtr));
        return MakeShared<FJsonValueString>(ClassValue ? ClassValue->GetPathName() : TEXT("None"));
    }
    else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
    {
        FSoftObjectPtr SoftPtr = SoftObjProp->GetPropertyValue(ValuePtr);
        return MakeShared<FJsonValueString>(SoftPtr.ToString());
    }
    else if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
    {
        FSoftObjectPtr SoftPtr = SoftClassProp->GetPropertyValue(ValuePtr);
        return MakeShared<FJsonValueString>(SoftPtr.ToString());
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (StructProp->Struct == TBaseStructure<FVector>::Get())
        {
            const FVector& Vec = *static_cast<const FVector*>(ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Vec.X));
            Arr.Add(MakeShared<FJsonValueNumber>(Vec.Y));
            Arr.Add(MakeShared<FJsonValueNumber>(Vec.Z));
            return MakeShared<FJsonValueArray>(Arr);
        }
        else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
        {
            const FRotator& Rot = *static_cast<const FRotator*>(ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
            Arr.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
            Arr.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
            return MakeShared<FJsonValueArray>(Arr);
        }
        else if (StructProp->Struct == TBaseStructure<FColor>::Get())
        {
            const FColor& Color = *static_cast<const FColor*>(ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Color.R));
            Arr.Add(MakeShared<FJsonValueNumber>(Color.G));
            Arr.Add(MakeShared<FJsonValueNumber>(Color.B));
            Arr.Add(MakeShared<FJsonValueNumber>(Color.A));
            return MakeShared<FJsonValueArray>(Arr);
        }
        else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
        {
            const FLinearColor& Color = *static_cast<const FLinearColor*>(ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Arr;
            Arr.Add(MakeShared<FJsonValueNumber>(Color.R));
            Arr.Add(MakeShared<FJsonValueNumber>(Color.G));
            Arr.Add(MakeShared<FJsonValueNumber>(Color.B));
            Arr.Add(MakeShared<FJsonValueNumber>(Color.A));
            return MakeShared<FJsonValueArray>(Arr);
        }
        // Fallback: export as string
        FString ExportedText;
        StructProp->ExportTextItem_Direct(ExportedText, ValuePtr, nullptr, nullptr, PPF_None);
        return MakeShared<FJsonValueString>(ExportedText);
    }
    // Fallback for unknown types
    FString ExportedText;
    Property->ExportTextItem_Direct(ExportedText, ValuePtr, nullptr, nullptr, PPF_None);
    return MakeShared<FJsonValueString>(ExportedText);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);

    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);

    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);

    // When detailed, iterate all EditAnywhere/VisibleAnywhere UPROPERTYs
    if (bDetailed)
    {
        TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();

        for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;

            // Only include properties that are visible or editable in the editor
            if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst | CPF_BlueprintVisible))
            {
                continue;
            }

            // Skip deprecated and transient
            if (Property->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient))
            {
                continue;
            }

            const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
            FString PropName = Property->GetName();

            TSharedPtr<FJsonValue> JsonVal = PropertyToJsonValue(Property, ValuePtr);
            if (JsonVal.IsValid())
            {
                PropertiesObj->SetField(PropName, JsonVal);
            }
        }

        ActorObject->SetObjectField(TEXT("properties"), PropertiesObj);
    }

    return ActorObject;
}

UK2Node_Event* FUnrealMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

bool FUnrealMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName,
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return false;
    }

    // Notify the object that it's about to be modified (enables undo/redo tracking)
    Object->Modify();

    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);

    // Handle different property types
    if (Property->IsA<FBoolProperty>())
    {
        ((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (Property->IsA<FIntProperty>())
    {
        int32 IntValue = static_cast<int32>(Value->AsNumber());
        FIntProperty* IntProperty = CastField<FIntProperty>(Property);
        if (IntProperty)
        {
            IntProperty->SetPropertyValue_InContainer(Object, IntValue);
            return true;
        }
    }
    else if (Property->IsA<FFloatProperty>())
    {
        ((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FStrProperty>())
    {
        ((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (Property->IsA<FByteProperty>())
    {
        FByteProperty* ByteProp = CastField<FByteProperty>(Property);
        UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;
        
        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"), 
                      *PropertyName, ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"), 
                          *PropertyName, *EnumValueName, ByteValue);
                    return true;
                }
                
                // Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    else if (Property->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
        UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
        FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;
        
        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"), 
                      *PropertyName, EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                
                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    
    else if (Property->IsA<FDoubleProperty>())
    {
        ((FDoubleProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FNameProperty>())
    {
        ((FNameProperty*)Property)->SetPropertyValue(PropertyAddr, FName(*Value->AsString()));
        return true;
    }
    else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        FString AssetPath = Value->AsString();
        UObject* LoadedObject = StaticLoadObject(ObjectProp->PropertyClass, nullptr, *AssetPath);

        // Try "Package.ObjectName" format if direct path fails
        if (!LoadedObject && !AssetPath.Contains(TEXT(".")))
        {
            FString BaseName = FPaths::GetBaseFilename(AssetPath);
            FString FullPath = AssetPath + TEXT(".") + BaseName;
            LoadedObject = StaticLoadObject(ObjectProp->PropertyClass, nullptr, *FullPath);
        }

        // Try loading as UObject (less restrictive class filter) and check compatibility
        if (!LoadedObject)
        {
            UObject* AnyObject = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
            if (!AnyObject && !AssetPath.Contains(TEXT(".")))
            {
                FString BaseName = FPaths::GetBaseFilename(AssetPath);
                AnyObject = StaticLoadObject(UObject::StaticClass(), nullptr, *(AssetPath + TEXT(".") + BaseName));
            }
            if (AnyObject && AnyObject->IsA(ObjectProp->PropertyClass))
            {
                LoadedObject = AnyObject;
            }
            else if (AnyObject)
            {
                // If it's a Blueprint, try getting the GeneratedClass for TSubclassOf/UClass* properties
                UBlueprint* BP = Cast<UBlueprint>(AnyObject);
                if (BP && BP->GeneratedClass && BP->GeneratedClass->IsChildOf(ObjectProp->PropertyClass))
                {
                    LoadedObject = BP->GeneratedClass;
                }
            }
        }

        if (LoadedObject)
        {
            ObjectProp->SetObjectPropertyValue(PropertyAddr, LoadedObject);
            UE_LOG(LogTemp, Display, TEXT("Set object property %s to %s (loaded: %s)"),
                *PropertyName, *AssetPath, *LoadedObject->GetPathName());
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Failed to load asset '%s' for property %s (expected type: %s)"),
                *AssetPath, *PropertyName, *ObjectProp->PropertyClass->GetName());
            return false;
        }
    }
    else if (FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(Property))
    {
        FString AssetPath = Value->AsString();
        FSoftObjectPtr SoftPtr{FSoftObjectPath{AssetPath}};
        SoftObjectProp->SetPropertyValue(PropertyAddr, SoftPtr);
        UE_LOG(LogTemp, Display, TEXT("Set soft object property %s to %s"), *PropertyName, *AssetPath);
        return true;
    }
    else if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
    {
        FString AssetPath = Value->AsString();
        FSoftObjectPtr SoftPtr{FSoftObjectPath{AssetPath}};
        SoftClassProp->SetPropertyValue(PropertyAddr, SoftPtr);
        UE_LOG(LogTemp, Display, TEXT("Set soft class property %s to %s"), *PropertyName, *AssetPath);
        return true;
    }

    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        if (StructProp->Struct == TBaseStructure<FVector>::Get())
        {
            if (Value->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
                if (Arr.Num() == 3)
                {
                    FVector Vec(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
                    StructProp->CopySingleValue(PropertyAddr, &Vec);
                    return true;
                }
            }
            OutErrorMessage = FString::Printf(TEXT("FVector property %s requires array of 3 numbers [x,y,z]"), *PropertyName);
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
        {
            if (Value->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
                if (Arr.Num() == 3)
                {
                    FRotator Rot(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
                    StructProp->CopySingleValue(PropertyAddr, &Rot);
                    return true;
                }
            }
            OutErrorMessage = FString::Printf(TEXT("FRotator property %s requires array of 3 numbers [pitch,yaw,roll]"), *PropertyName);
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FColor>::Get())
        {
            if (Value->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
                if (Arr.Num() >= 3)
                {
                    FColor Color(
                        (uint8)Arr[0]->AsNumber(),
                        (uint8)Arr[1]->AsNumber(),
                        (uint8)Arr[2]->AsNumber(),
                        Arr.Num() >= 4 ? (uint8)Arr[3]->AsNumber() : 255);
                    StructProp->CopySingleValue(PropertyAddr, &Color);
                    return true;
                }
            }
            OutErrorMessage = FString::Printf(TEXT("FColor property %s requires array of 3-4 numbers [R,G,B] or [R,G,B,A]"), *PropertyName);
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
        {
            if (Value->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
                if (Arr.Num() >= 3)
                {
                    FLinearColor Color(
                        Arr[0]->AsNumber(),
                        Arr[1]->AsNumber(),
                        Arr[2]->AsNumber(),
                        Arr.Num() >= 4 ? Arr[3]->AsNumber() : 1.0f);
                    StructProp->CopySingleValue(PropertyAddr, &Color);
                    return true;
                }
            }
            OutErrorMessage = FString::Printf(TEXT("FLinearColor property %s requires array of 3-4 numbers [R,G,B] or [R,G,B,A]"), *PropertyName);
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FTransform>::Get())
        {
            if (Value->Type == EJson::Object)
            {
                TSharedPtr<FJsonObject> Obj = Value->AsObject();
                FTransform Transform;
                if (Obj->HasField(TEXT("location")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("location"));
                    if (Arr.Num() == 3)
                        Transform.SetLocation(FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber()));
                }
                if (Obj->HasField(TEXT("rotation")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("rotation"));
                    if (Arr.Num() == 3)
                        Transform.SetRotation(FQuat(FRotator(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber())));
                }
                if (Obj->HasField(TEXT("scale")))
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("scale"));
                    if (Arr.Num() == 3)
                        Transform.SetScale3D(FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber()));
                }
                StructProp->CopySingleValue(PropertyAddr, &Transform);
                return true;
            }
            OutErrorMessage = FString::Printf(TEXT("FTransform property %s requires object with location/rotation/scale arrays"), *PropertyName);
            return false;
        }
        else if (StructProp->Struct == TBaseStructure<FVector2D>::Get())
        {
            if (Value->Type == EJson::Array)
            {
                const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
                if (Arr.Num() == 2)
                {
                    FVector2D Vec(Arr[0]->AsNumber(), Arr[1]->AsNumber());
                    StructProp->CopySingleValue(PropertyAddr, &Vec);
                    return true;
                }
            }
            OutErrorMessage = FString::Printf(TEXT("FVector2D property %s requires array of 2 numbers [x,y]"), *PropertyName);
            return false;
        }

        OutErrorMessage = FString::Printf(TEXT("Unsupported struct type: %s for property %s"), *StructProp->Struct->GetName(), *PropertyName);
        return false;
    }
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        FString ClassPath = Value->AsString();
        UClass* LoadedClass = nullptr;

        // 1. Try exact path as UClass (e.g. "/Script/Engine.Character" or "/Game/Path/BP_Name.BP_Name_C")
        LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);

        // 2. If not found and doesn't end with _C, try appending _C (Blueprint generated class convention)
        if (!LoadedClass && !ClassPath.EndsWith(TEXT("_C")))
        {
            // Try "Path.Name_C" format
            FString ClassPathWithC = ClassPath + TEXT("_C");
            LoadedClass = LoadObject<UClass>(nullptr, *ClassPathWithC);

            // Also try "Path.Path_C" format (when only package path given like "/Game/Path/BP_Name")
            if (!LoadedClass)
            {
                FString BaseName = FPaths::GetBaseFilename(ClassPath);
                FString ClassPathDotC = ClassPath + TEXT(".") + BaseName + TEXT("_C");
                LoadedClass = LoadObject<UClass>(nullptr, *ClassPathDotC);
            }
        }

        // 3. Try loading as Blueprint asset and getting its GeneratedClass
        if (!LoadedClass)
        {
            FString BlueprintPath = ClassPath;
            BlueprintPath.RemoveFromEnd(TEXT("_C"));
            UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
            if (!BP)
            {
                // Try "Path.Path" format
                FString BaseName = FPaths::GetBaseFilename(BlueprintPath);
                FString FullBPPath = BlueprintPath + TEXT(".") + BaseName;
                BP = LoadObject<UBlueprint>(nullptr, *FullBPPath);
            }
            if (BP && BP->GeneratedClass)
            {
                LoadedClass = BP->GeneratedClass;
            }
        }

        // 4. Try FindFirstObject as last resort (for already-loaded classes by short name)
        if (!LoadedClass)
        {
            LoadedClass = FindFirstObject<UClass>(*ClassPath);
        }

        if (LoadedClass)
        {
            ClassProp->SetObjectPropertyValue(PropertyAddr, LoadedClass);
            UE_LOG(LogTemp, Display, TEXT("Set class property %s to %s (resolved: %s)"),
                *PropertyName, *ClassPath, *LoadedClass->GetPathName());
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Failed to load class '%s' for property %s. Try full path like '/Game/Path/BP_Name' or '/Script/Module.ClassName'"),
                *ClassPath, *PropertyName);
            return false;
        }
    }

    // Handle array properties (TArray<FName>, TArray<FString>, TArray<UObject*>)
    else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        FProperty* InnerProp = ArrayProp->Inner;

        // Parse JSON array
        if (Value->Type != EJson::Array)
        {
            OutErrorMessage = FString::Printf(TEXT("ArrayProperty %s requires a JSON array value"), *PropertyName);
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>& JsonArray = Value->AsArray();

        FScriptArrayHelper ArrayHelper(ArrayProp, PropertyAddr);

        if (InnerProp->IsA<FNameProperty>())
        {
            // TArray<FName> — used for Tags, etc.
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                FName NameVal(*JsonArray[i]->AsString());
                CastField<FNameProperty>(InnerProp)->SetPropertyValue(ArrayHelper.GetRawPtr(i), NameVal);
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<FName> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else if (InnerProp->IsA<FStrProperty>())
        {
            // TArray<FString>
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                CastField<FStrProperty>(InnerProp)->SetPropertyValue(ArrayHelper.GetRawPtr(i), JsonArray[i]->AsString());
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<FString> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else if (InnerProp->IsA<FIntProperty>())
        {
            // TArray<int32>
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                CastField<FIntProperty>(InnerProp)->SetPropertyValue(ArrayHelper.GetRawPtr(i), static_cast<int32>(JsonArray[i]->AsNumber()));
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<int32> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else if (InnerProp->IsA<FFloatProperty>())
        {
            // TArray<float>
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                CastField<FFloatProperty>(InnerProp)->SetPropertyValue(ArrayHelper.GetRawPtr(i), static_cast<float>(JsonArray[i]->AsNumber()));
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<float> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else if (InnerProp->IsA<FDoubleProperty>())
        {
            // TArray<double>
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                CastField<FDoubleProperty>(InnerProp)->SetPropertyValue(ArrayHelper.GetRawPtr(i), JsonArray[i]->AsNumber());
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<double> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else if (InnerProp->IsA<FBoolProperty>())
        {
            // TArray<bool>
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                CastField<FBoolProperty>(InnerProp)->SetPropertyValue(ArrayHelper.GetRawPtr(i), JsonArray[i]->AsBool());
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<bool> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else if (FObjectProperty* InnerObjProp = CastField<FObjectProperty>(InnerProp))
        {
            // TArray<UObject*> — used for OverrideMaterials, etc.
            ArrayHelper.Resize(JsonArray.Num());
            for (int32 i = 0; i < JsonArray.Num(); i++)
            {
                FString AssetPath = JsonArray[i]->AsString();
                UObject* LoadedObject = nullptr;

                if (AssetPath.IsEmpty() || AssetPath == TEXT("null") || AssetPath == TEXT("None"))
                {
                    // Allow null entries in the array
                    InnerObjProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(i), nullptr);
                    continue;
                }

                LoadedObject = StaticLoadObject(InnerObjProp->PropertyClass, nullptr, *AssetPath);
                if (!LoadedObject && !AssetPath.Contains(TEXT(".")))
                {
                    FString BaseName = FPaths::GetBaseFilename(AssetPath);
                    LoadedObject = StaticLoadObject(InnerObjProp->PropertyClass, nullptr, *(AssetPath + TEXT(".") + BaseName));
                }
                if (LoadedObject)
                {
                    InnerObjProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(i), LoadedObject);
                }
                else
                {
                    OutErrorMessage = FString::Printf(TEXT("Failed to load asset '%s' at array index %d for property %s (expected type: %s)"),
                        *AssetPath, i, *PropertyName, *InnerObjProp->PropertyClass->GetName());
                    return false;
                }
            }
            UE_LOG(LogTemp, Display, TEXT("Set array<UObject*> property %s with %d elements"), *PropertyName, JsonArray.Num());
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Unsupported array inner type: %s for property %s"),
                *InnerProp->GetClass()->GetName(), *PropertyName);
            return false;
        }
    }

    OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"),
                                    *Property->GetClass()->GetName(), *PropertyName);
    return false;
} 