#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/InputSettings.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "EdGraphSchema_K2.h"

// Declare the log category
DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCP, Log, All);

FUnrealMCPBlueprintNodeCommands::FUnrealMCPBlueprintNodeCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("connect_blueprint_nodes"))
    {
        return HandleConnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("add_blueprint_get_self_component_reference"))
    {
        return HandleAddBlueprintGetSelfComponentReference(Params);
    }
    else if (CommandType == TEXT("add_blueprint_event_node"))
    {
        return HandleAddBlueprintEvent(Params);
    }
    else if (CommandType == TEXT("add_blueprint_function_node"))
    {
        return HandleAddBlueprintFunctionCall(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable"))
    {
        return HandleAddBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("add_blueprint_input_action_node"))
    {
        return HandleAddBlueprintInputActionNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_self_reference"))
    {
        return HandleAddBlueprintSelfReference(Params);
    }
    else if (CommandType == TEXT("find_blueprint_nodes"))
    {
        return HandleFindBlueprintNodes(Params);
    }
    // Phase 4: Advanced Blueprint Nodes
    else if (CommandType == TEXT("add_blueprint_branch_node"))
    {
        return HandleAddBlueprintBranchNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_for_loop_node"))
    {
        return HandleAddBlueprintForLoopNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_delay_node"))
    {
        return HandleAddBlueprintDelayNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_print_string_node"))
    {
        return HandleAddBlueprintPrintStringNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_set_timer_node"))
    {
        return HandleAddBlueprintSetTimerNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_custom_event_node"))
    {
        return HandleAddBlueprintCustomEventNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable_get_node"))
    {
        return HandleAddBlueprintVariableGetNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable_set_node"))
    {
        return HandleAddBlueprintVariableSetNode(Params);
    }
    else if (CommandType == TEXT("set_node_pin_default_value"))
    {
        return HandleSetNodePinDefaultValue(Params);
    }
    else if (CommandType == TEXT("add_blueprint_math_node"))
    {
        return HandleAddBlueprintMathNode(Params);
    }
    else if (CommandType == TEXT("remove_blueprint_variable"))
    {
        return HandleRemoveBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("change_blueprint_variable_type"))
    {
        return HandleChangeBlueprintVariableType(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint node command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString TargetNodeId;
    if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin"), SourcePinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the nodes
    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId)
        {
            SourceNode = Node;
        }
        else if (Node->NodeGuid.ToString() == TargetNodeId)
        {
            TargetNode = Node;
        }
    }

    if (!SourceNode || !TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    // Connect the nodes
    if (FUnrealMCPCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        // Mark the blueprint as modified
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect nodes"));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }
    
    // We'll skip component verification since the GetAllNodes API may have changed in UE5.5
    
    // Create the variable get node directly
    UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetComponentNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create get component node"));
    }
    
    // Set up the variable reference properly for UE5.5
    FMemberReference& VarRef = GetComponentNode->VariableReference;
    VarRef.SetSelfMember(FName(*ComponentName));
    
    // Set node position
    GetComponentNode->NodePosX = NodePosition.X;
    GetComponentNode->NodePosY = NodePosition.Y;
    
    // Add to graph
    EventGraph->AddNode(GetComponentNode);
    GetComponentNode->CreateNewGuid();
    GetComponentNode->PostPlacedNewNode();
    GetComponentNode->AllocateDefaultPins();
    
    // Explicitly reconstruct node for UE5.5
    GetComponentNode->ReconstructNode();
    
    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the event node
    UK2Node_Event* EventNode = FUnrealMCPCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    if (!EventNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Check for target parameter (optional)
    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the function
    UFunction* Function = nullptr;
    UK2Node_CallFunction* FunctionNode = nullptr;
    
    // Add extensive logging for debugging
    UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in target '%s'"), 
           *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target);
    
    // Check if we have a target class specified
    if (!Target.IsEmpty())
    {
        // Try to find the target class
        UClass* TargetClass = nullptr;
        
        // First try without a prefix
        TargetClass = FindFirstObject<UClass>(*Target);
        UE_LOG(LogTemp, Display, TEXT("Tried to find class '%s': %s"), 
               *Target, TargetClass ? TEXT("Found") : TEXT("Not found"));
        
        // If not found, try with U prefix (common convention for UE classes)
        if (!TargetClass && !Target.StartsWith(TEXT("U")))
        {
            FString TargetWithPrefix = FString(TEXT("U")) + Target;
            TargetClass = FindFirstObject<UClass>(*TargetWithPrefix);
            UE_LOG(LogTemp, Display, TEXT("Tried to find class '%s': %s"), 
                   *TargetWithPrefix, TargetClass ? TEXT("Found") : TEXT("Not found"));
        }
        
        // If still not found, try with common component names
        if (!TargetClass)
        {
            // Try some common component class names
            TArray<FString> PossibleClassNames;
            PossibleClassNames.Add(FString(TEXT("U")) + Target + TEXT("Component"));
            PossibleClassNames.Add(Target + TEXT("Component"));
            
            for (const FString& ClassName : PossibleClassNames)
            {
                TargetClass = FindFirstObject<UClass>(*ClassName);
                if (TargetClass)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found class using alternative name '%s'"), *ClassName);
                    break;
                }
            }
        }
        
        // Special case handling for common classes like UGameplayStatics
        if (!TargetClass && Target == TEXT("UGameplayStatics"))
        {
            // For UGameplayStatics, use a direct reference to known class
            TargetClass = FindFirstObject<UClass>(TEXT("UGameplayStatics"));
            if (!TargetClass)
            {
                // Try loading it from its known package
                TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.GameplayStatics"));
                UE_LOG(LogTemp, Display, TEXT("Explicitly loading GameplayStatics: %s"), 
                       TargetClass ? TEXT("Success") : TEXT("Failed"));
            }
        }
        
        // If we found a target class, look for the function there
        if (TargetClass)
        {
            UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in class '%s'"), 
                   *FunctionName, *TargetClass->GetName());
                   
            // First try exact name
            Function = TargetClass->FindFunctionByName(*FunctionName);
            
            // If not found, try class hierarchy
            UClass* CurrentClass = TargetClass;
            while (!Function && CurrentClass)
            {
                UE_LOG(LogTemp, Display, TEXT("Searching in class: %s"), *CurrentClass->GetName());
                
                // Try exact match
                Function = CurrentClass->FindFunctionByName(*FunctionName);
                
                // Try case-insensitive match
                if (!Function)
                {
                    for (TFieldIterator<UFunction> FuncIt(CurrentClass); FuncIt; ++FuncIt)
                    {
                        UFunction* AvailableFunc = *FuncIt;
                        UE_LOG(LogTemp, Display, TEXT("  - Available function: %s"), *AvailableFunc->GetName());
                        
                        if (AvailableFunc->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive match: %s"), *AvailableFunc->GetName());
                            Function = AvailableFunc;
                            break;
                        }
                    }
                }
                
                // Move to parent class
                CurrentClass = CurrentClass->GetSuperClass();
            }
            
            // Special handling for known functions
            if (!Function)
            {
                if (TargetClass->GetName() == TEXT("GameplayStatics") && 
                    (FunctionName == TEXT("GetActorOfClass") || FunctionName.Equals(TEXT("GetActorOfClass"), ESearchCase::IgnoreCase)))
                {
                    UE_LOG(LogTemp, Display, TEXT("Using special case handling for GameplayStatics::GetActorOfClass"));
                    
                    // Create the function node directly
                    FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
                    if (FunctionNode)
                    {
                        // Direct setup for known function
                        FunctionNode->FunctionReference.SetExternalMember(
                            FName(TEXT("GetActorOfClass")), 
                            TargetClass
                        );
                        
                        FunctionNode->NodePosX = NodePosition.X;
                        FunctionNode->NodePosY = NodePosition.Y;
                        EventGraph->AddNode(FunctionNode);
                        FunctionNode->CreateNewGuid();
                        FunctionNode->PostPlacedNewNode();
                        FunctionNode->AllocateDefaultPins();
                        
                        UE_LOG(LogTemp, Display, TEXT("Created GetActorOfClass node directly"));
                        
                        // List all pins
                        for (UEdGraphPin* Pin : FunctionNode->Pins)
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Pin: %s, Direction: %d, Category: %s"), 
                                   *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
                        }
                    }
                }
            }
        }
    }
    
    // If we still haven't found the function, try in the blueprint's class
    if (!Function && !FunctionNode)
    {
        UE_LOG(LogTemp, Display, TEXT("Trying to find function in blueprint class"));
        Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
    }
    
    // Create the function call node if we found the function
    if (Function && !FunctionNode)
    {
        FunctionNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
    }
    
    if (!FunctionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target));
    }

    // Set parameters if provided
    if (Params->HasField(TEXT("params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj;
        if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
        {
            // Process parameters
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
            {
                const FString& ParamName = Param.Key;
                const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                
                // Find the parameter pin
                UEdGraphPin* ParamPin = FUnrealMCPCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
                if (ParamPin)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found parameter pin '%s' of category '%s'"), 
                           *ParamName, *ParamPin->PinType.PinCategory.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  Current default value: '%s'"), *ParamPin->DefaultValue);
                    if (ParamPin->PinType.PinSubCategoryObject.IsValid())
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Pin subcategory: '%s'"), 
                               *ParamPin->PinType.PinSubCategoryObject->GetName());
                    }
                    
                    // Set parameter based on type
                    if (ParamValue->Type == EJson::String)
                    {
                        FString StringVal = ParamValue->AsString();
                        UE_LOG(LogTemp, Display, TEXT("  Setting string parameter '%s' to: '%s'"), 
                               *ParamName, *StringVal);
                        
                        // Handle class reference parameters (e.g., ActorClass in GetActorOfClass)
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                        {
                            // For class references, we require the exact class name with proper prefix
                            // - Actor classes must start with 'A' (e.g., ACameraActor)
                            // - Non-actor classes must start with 'U' (e.g., UObject)
                            const FString& ClassName = StringVal;
                            
                            // TODO: This likely won't work in UE5.5+, so don't rely on it.
                            UClass* Class = FindFirstObject<UClass>(*ClassName);

                            if (!Class)
                            {
                                Class = LoadObject<UClass>(nullptr, *ClassName);
                                UE_LOG(LogUnrealMCP, Display, TEXT("FindObject<UClass> failed. Assuming soft path  path: %s"), *ClassName);
                            }
                            
                            // If not found, try with Engine module path
                            if (!Class)
                            {
                                FString EngineClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
                                Class = LoadObject<UClass>(nullptr, *EngineClassName);
                                UE_LOG(LogUnrealMCP, Display, TEXT("Trying Engine module path: %s"), *EngineClassName);
                            }
                            
                            if (!Class)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to find class '%s'. Make sure to use the exact class name with proper prefix (A for actors, U for non-actors)"), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to find class '%s'"), *ClassName));
                            }

                            const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                            if (!K2Schema)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to get K2Schema"));
                                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get K2Schema"));
                            }

                            K2Schema->TrySetDefaultObject(*ParamPin, Class);
                            if (ParamPin->DefaultObject != Class)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to set class reference for pin '%s'"), *ParamPin->PinName.ToString()));
                            }

                            UE_LOG(LogUnrealMCP, Log, TEXT("Successfully set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                            continue;
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            bool BoolValue = ParamValue->AsBool();
                            ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                            UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                                   *ParamName, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
                        {
                            // Handle array parameters - like Vector parameters
                            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                            if (ParamValue->TryGetArray(ArrayValue))
                            {
                                // Check if this could be a vector (array of 3 numbers)
                                if (ArrayValue->Num() == 3)
                                {
                                    // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                    float X = (*ArrayValue)[0]->AsNumber();
                                    float Y = (*ArrayValue)[1]->AsNumber();
                                    float Z = (*ArrayValue)[2]->AsNumber();
                                    
                                    FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                    ParamPin->DefaultValue = VectorString;
                                    
                                    UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                           *ParamName, *VectorString);
                                    UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                           *ParamPin->DefaultValue);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                                }
                            }
                        }
                    }
                    else if (ParamValue->Type == EJson::Number)
                    {
                        // Handle integer vs float parameters correctly
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                    }
                    else if (ParamValue->Type == EJson::Boolean)
                    {
                        bool BoolValue = ParamValue->AsBool();
                        ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                        UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                               *ParamName, *ParamPin->DefaultValue);
                    }
                    else if (ParamValue->Type == EJson::Array)
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Processing array parameter '%s'"), *ParamName);
                        // Handle array parameters - like Vector parameters
                        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                        if (ParamValue->TryGetArray(ArrayValue))
                        {
                            // Check if this could be a vector (array of 3 numbers)
                            if (ArrayValue->Num() == 3 && 
                                (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct) &&
                                (ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()))
                            {
                                // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                float X = (*ArrayValue)[0]->AsNumber();
                                float Y = (*ArrayValue)[1]->AsNumber();
                                float Z = (*ArrayValue)[2]->AsNumber();
                                
                                FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                ParamPin->DefaultValue = VectorString;
                                
                                UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                       *ParamName, *VectorString);
                                UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                       *ParamPin->DefaultValue);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                            }
                        }
                    }
                    // Add handling for other types as needed
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Parameter pin '%s' not found"), *ParamName);
                }
            }
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    // Get optional parameters
    bool IsExposed = false;
    if (Params->HasField(TEXT("is_exposed")))
    {
        IsExposed = Params->GetBoolField(TEXT("is_exposed"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create variable based on type
    FEdGraphPinType PinType;
    
    // Set up pin type based on variable_type string
    if (VariableType == TEXT("Boolean"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (VariableType == TEXT("Float") || VariableType == TEXT("Double") || VariableType == TEXT("Real"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        PinType.PinSubCategory = TEXT("double");
    }
    else if (VariableType == TEXT("String"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (VariableType == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    // Create the variable
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Set variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // Set exposure in editor
        if (IsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit;
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the input action node
    UK2Node_InputAction* InputActionNode = FUnrealMCPCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the self node
    UK2Node_Self* SelfNode = FUnrealMCPCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // node_type is now optional — if omitted, list all nodes
    FString NodeType;
    Params->TryGetStringField(TEXT("node_type"), NodeType);

    // Optional filter by event_name (for Event nodes) or name (general text search)
    FString EventName;
    Params->TryGetStringField(TEXT("event_name"), EventName);
    FString NameFilter;
    Params->TryGetStringField(TEXT("name"), NameFilter);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Collect nodes from all graphs (event graph + function graphs)
    TArray<UEdGraph*> AllGraphs;
    AllGraphs.Append(Blueprint->UbergraphPages);
    AllGraphs.Append(Blueprint->FunctionGraphs);

    TArray<TSharedPtr<FJsonValue>> NodesArray;

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;

            FString NodeClassName = Node->GetClass()->GetName();
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();

            // Apply node_type filter if specified
            if (!NodeType.IsEmpty())
            {
                bool bMatch = false;

                if (NodeType == TEXT("Event"))
                {
                    UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
                    if (EventNode)
                    {
                        // If event_name specified, filter by it
                        if (!EventName.IsEmpty())
                        {
                            bMatch = (EventNode->EventReference.GetMemberName() == FName(*EventName));
                        }
                        else
                        {
                            bMatch = true;
                        }
                    }
                }
                else if (NodeType == TEXT("Function") || NodeType == TEXT("CallFunction"))
                {
                    bMatch = (Cast<UK2Node_CallFunction>(Node) != nullptr);
                }
                else if (NodeType == TEXT("Variable") || NodeType == TEXT("VariableGet"))
                {
                    bMatch = (Cast<UK2Node_VariableGet>(Node) != nullptr);
                }
                else if (NodeType == TEXT("VariableSet"))
                {
                    bMatch = (Cast<UK2Node_VariableSet>(Node) != nullptr);
                }
                else if (NodeType == TEXT("CustomEvent"))
                {
                    bMatch = (Cast<UK2Node_CustomEvent>(Node) != nullptr);
                }
                else
                {
                    // Generic match: check class name contains the type string
                    bMatch = NodeClassName.Contains(NodeType);
                }

                if (!bMatch) continue;
            }

            // Apply name filter if specified
            if (!NameFilter.IsEmpty())
            {
                if (!NodeTitle.Contains(NameFilter) && !NodeClassName.Contains(NameFilter))
                {
                    continue;
                }
            }

            // Build node info
            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
            NodeObj->SetStringField(TEXT("class"), NodeClassName);
            NodeObj->SetStringField(TEXT("title"), NodeTitle);
            NodeObj->SetStringField(TEXT("graph"), Graph->GetName());
            NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
            NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);

            // Add pin info
            TArray<TSharedPtr<FJsonValue>> PinsArray;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin) continue;
                TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
                PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                if (!Pin->DefaultValue.IsEmpty())
                {
                    PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
                }
                PinObj->SetBoolField(TEXT("connected"), Pin->LinkedTo.Num() > 0);
                PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
            }
            NodeObj->SetArrayField(TEXT("pins"), PinsArray);

            NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
    ResultObj->SetNumberField(TEXT("count"), NodesArray.Num());

    // Also return node_guids for backward compatibility
    TArray<TSharedPtr<FJsonValue>> NodeGuidArray;
    for (const TSharedPtr<FJsonValue>& NodeVal : NodesArray)
    {
        if (NodeVal->AsObject()->HasField(TEXT("node_id")))
        {
            NodeGuidArray.Add(MakeShared<FJsonValueString>(NodeVal->AsObject()->GetStringField(TEXT("node_id"))));
        }
    }
    ResultObj->SetArrayField(TEXT("node_guids"), NodeGuidArray);

    return ResultObj;
}

// ============================================================
// Phase 4: Advanced Blueprint Nodes
// ============================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintBranchNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
	if (!BranchNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Branch node"));

	BranchNode->NodePosX = NodePosition.X;
	BranchNode->NodePosY = NodePosition.Y;
	EventGraph->AddNode(BranchNode);
	BranchNode->CreateNewGuid();
	BranchNode->PostPlacedNewNode();
	BranchNode->AllocateDefaultPins();

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), BranchNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintForLoopNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	// Find the ForLoop macro graph
	UEdGraph* ForLoopMacro = nullptr;
	UBlueprint* MacroLibrary = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
	if (MacroLibrary)
	{
		for (UEdGraph* MacroGraph : MacroLibrary->MacroGraphs)
		{
			if (MacroGraph && MacroGraph->GetFName() == FName(TEXT("ForLoop")))
			{
				ForLoopMacro = MacroGraph;
				break;
			}
		}
	}

	if (!ForLoopMacro)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find ForLoop macro"));
	}

	UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(EventGraph);
	MacroNode->SetMacroGraph(ForLoopMacro);
	MacroNode->NodePosX = NodePosition.X;
	MacroNode->NodePosY = NodePosition.Y;
	EventGraph->AddNode(MacroNode);
	MacroNode->CreateNewGuid();
	MacroNode->PostPlacedNewNode();
	MacroNode->AllocateDefaultPins();

	// Set default first/last index if provided
	int32 FirstIndex = 0, LastIndex = 0;
	if (Params->TryGetNumberField(TEXT("first_index"), FirstIndex))
	{
		UEdGraphPin* FirstPin = FUnrealMCPCommonUtils::FindPin(MacroNode, TEXT("FirstIndex"), EGPD_Input);
		if (FirstPin) FirstPin->DefaultValue = FString::FromInt(FirstIndex);
	}
	if (Params->TryGetNumberField(TEXT("last_index"), LastIndex))
	{
		UEdGraphPin* LastPin = FUnrealMCPCommonUtils::FindPin(MacroNode, TEXT("LastIndex"), EGPD_Input);
		if (LastPin) LastPin->DefaultValue = FString::FromInt(LastIndex);
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), MacroNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintDelayNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	double Duration = 1.0;
	Params->TryGetNumberField(TEXT("duration"), Duration);

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	// Create Delay function call node
	UFunction* DelayFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName(TEXT("Delay")));
	if (!DelayFunc) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find Delay function"));

	UK2Node_CallFunction* FuncNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, DelayFunc, NodePosition);
	if (!FuncNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Delay node"));

	// Set duration default
	UEdGraphPin* DurationPin = FUnrealMCPCommonUtils::FindPin(FuncNode, TEXT("Duration"), EGPD_Input);
	if (DurationPin) DurationPin->DefaultValue = FString::SanitizeFloat(Duration);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), FuncNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintPrintStringNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	FString Text = TEXT("Hello");
	Params->TryGetStringField(TEXT("text"), Text);

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	UFunction* PrintFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName(TEXT("PrintString")));
	if (!PrintFunc) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find PrintString function"));

	UK2Node_CallFunction* FuncNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, PrintFunc, NodePosition);
	if (!FuncNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create PrintString node"));

	// Set InString default
	UEdGraphPin* StringPin = FUnrealMCPCommonUtils::FindPin(FuncNode, TEXT("InString"), EGPD_Input);
	if (StringPin) StringPin->DefaultValue = Text;

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), FuncNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintSetTimerNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	double Time = 1.0;
	Params->TryGetNumberField(TEXT("time"), Time);

	bool bLooping = false;
	Params->TryGetBoolField(TEXT("looping"), bLooping);

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	UFunction* TimerFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName(TEXT("K2_SetTimer")));
	if (!TimerFunc)
	{
		// Try alternate name
		TimerFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName(TEXT("SetTimer")));
	}
	if (!TimerFunc) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find SetTimer function"));

	UK2Node_CallFunction* FuncNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, TimerFunc, NodePosition);
	if (!FuncNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create SetTimer node"));

	// Set defaults
	UEdGraphPin* FuncNamePin = FUnrealMCPCommonUtils::FindPin(FuncNode, TEXT("FunctionName"), EGPD_Input);
	if (FuncNamePin) FuncNamePin->DefaultValue = FunctionName;

	UEdGraphPin* TimePin = FUnrealMCPCommonUtils::FindPin(FuncNode, TEXT("Time"), EGPD_Input);
	if (TimePin) TimePin->DefaultValue = FString::SanitizeFloat(Time);

	UEdGraphPin* LoopingPin = FUnrealMCPCommonUtils::FindPin(FuncNode, TEXT("bLooping"), EGPD_Input);
	if (LoopingPin) LoopingPin->DefaultValue = bLooping ? TEXT("true") : TEXT("false");

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), FuncNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintCustomEventNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	UK2Node_CustomEvent* CustomEventNode = NewObject<UK2Node_CustomEvent>(EventGraph);
	if (!CustomEventNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Custom Event node"));

	CustomEventNode->CustomFunctionName = FName(*EventName);
	CustomEventNode->NodePosX = NodePosition.X;
	CustomEventNode->NodePosY = NodePosition.Y;
	EventGraph->AddNode(CustomEventNode);
	CustomEventNode->CreateNewGuid();
	CustomEventNode->PostPlacedNewNode();
	CustomEventNode->AllocateDefaultPins();

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), CustomEventNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariableGetNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(EventGraph);
	if (!GetNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create VariableGet node"));

	GetNode->VariableReference.SetSelfMember(FName(*VariableName));
	GetNode->NodePosX = NodePosition.X;
	GetNode->NodePosY = NodePosition.Y;
	EventGraph->AddNode(GetNode);
	GetNode->CreateNewGuid();
	GetNode->PostPlacedNewNode();
	GetNode->AllocateDefaultPins();
	GetNode->ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), GetNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariableSetNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(EventGraph);
	if (!SetNode) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create VariableSet node"));

	SetNode->VariableReference.SetSelfMember(FName(*VariableName));
	SetNode->NodePosX = NodePosition.X;
	SetNode->NodePosY = NodePosition.Y;
	EventGraph->AddNode(SetNode);
	SetNode->CreateNewGuid();
	SetNode->PostPlacedNewNode();
	SetNode->AllocateDefaultPins();
	SetNode->ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), SetNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSetNodePinDefaultValue(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}

	FString Value;
	if (!Params->TryGetStringField(TEXT("value"), Value))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	// Find the node
	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		if (Node->NodeGuid.ToString() == NodeId)
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// Find the pin
	UEdGraphPin* Pin = FUnrealMCPCommonUtils::FindPin(TargetNode, PinName, EGPD_Input);
	if (!Pin)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s on node %s"), *PinName, *NodeId));
	}

	Pin->DefaultValue = Value;
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("pin_name"), PinName);
	ResultObj->SetStringField(TEXT("value"), Value);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintMathNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString Operation;
	if (!Params->TryGetStringField(TEXT("operation"), Operation))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'operation' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

	UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

	// Map operation string to KismetMathLibrary function name
	FName FuncName;
	if (Operation == TEXT("+") || Operation == TEXT("Add")) FuncName = FName(TEXT("Add_IntInt"));
	else if (Operation == TEXT("-") || Operation == TEXT("Subtract")) FuncName = FName(TEXT("Subtract_IntInt"));
	else if (Operation == TEXT("*") || Operation == TEXT("Multiply")) FuncName = FName(TEXT("Multiply_IntInt"));
	else if (Operation == TEXT("/") || Operation == TEXT("Divide")) FuncName = FName(TEXT("Divide_IntInt"));
	else if (Operation == TEXT(">") || Operation == TEXT("Greater")) FuncName = FName(TEXT("Greater_IntInt"));
	else if (Operation == TEXT("<") || Operation == TEXT("Less")) FuncName = FName(TEXT("Less_IntInt"));
	else if (Operation == TEXT("==") || Operation == TEXT("Equal")) FuncName = FName(TEXT("EqualEqual_IntInt"));
	else if (Operation == TEXT("!=") || Operation == TEXT("NotEqual")) FuncName = FName(TEXT("NotEqual_IntInt"));
	else if (Operation == TEXT("AddFloat") || Operation == TEXT("+f")) FuncName = FName(TEXT("Add_FloatFloat"));
	else if (Operation == TEXT("SubtractFloat") || Operation == TEXT("-f")) FuncName = FName(TEXT("Subtract_FloatFloat"));
	else if (Operation == TEXT("MultiplyFloat") || Operation == TEXT("*f")) FuncName = FName(TEXT("Multiply_FloatFloat"));
	else if (Operation == TEXT("DivideFloat") || Operation == TEXT("/f")) FuncName = FName(TEXT("Divide_FloatFloat"));
	else if (Operation == TEXT("GreaterFloat") || Operation == TEXT(">f")) FuncName = FName(TEXT("Greater_FloatFloat"));
	else if (Operation == TEXT("LessFloat") || Operation == TEXT("<f")) FuncName = FName(TEXT("Less_FloatFloat"));
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown math operation: %s"), *Operation));
	}

	UFunction* MathFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(FuncName);
	if (!MathFunc)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Math function not found: %s"), *FuncName.ToString()));
	}

	UK2Node_CallFunction* FuncNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, MathFunc, NodePosition);
	if (!FuncNode)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create math node"));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), FuncNode->NodeGuid.ToString());
	ResultObj->SetStringField(TEXT("operation"), Operation);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleRemoveBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Verify the variable exists before attempting removal
	FName VarFName(*VariableName);
	int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarFName);
	if (VarIndex == INDEX_NONE)
	{
		// List available variables for debugging
		TArray<TSharedPtr<FJsonValue>> VarList;
		for (const FBPVariableDescription& Var : Blueprint->NewVariables)
		{
			VarList.Add(MakeShared<FJsonValueString>(Var.VarName.ToString()));
		}
		TSharedPtr<FJsonObject> ErrorObj = FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Variable '%s' not found on Blueprint '%s'"), *VariableName, *BlueprintName));
		ErrorObj->SetArrayField(TEXT("available_variables"), VarList);
		return ErrorObj;
	}

	// Get the variable type before removing (for the response)
	FString VarType = Blueprint->NewVariables[VarIndex].VarType.PinCategory.ToString();

	// Remove the variable - this also removes all graph nodes referencing it
	FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, VarFName);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("removed_variable"), VariableName);
	ResultObj->SetStringField(TEXT("variable_type"), VarType);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleChangeBlueprintVariableType(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString NewType;
	if (!Params->TryGetStringField(TEXT("new_type"), NewType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_type' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Verify the variable exists
	FName VarFName(*VariableName);
	int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarFName);
	if (VarIndex == INDEX_NONE)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Variable '%s' not found on Blueprint '%s'"), *VariableName, *BlueprintName));
	}

	// Get old type for the response
	FString OldType = Blueprint->NewVariables[VarIndex].VarType.PinCategory.ToString();

	// Build the new pin type
	FEdGraphPinType NewPinType;
	if (NewType == TEXT("Boolean"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (NewType == TEXT("Integer") || NewType == TEXT("Int"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (NewType == TEXT("Float") || NewType == TEXT("Double") || NewType == TEXT("Real"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		NewPinType.PinSubCategory = TEXT("double");
	}
	else if (NewType == TEXT("String"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (NewType == TEXT("Vector"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		NewPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (NewType == TEXT("Rotator"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		NewPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (NewType == TEXT("Name"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (NewType == TEXT("Text"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (NewType == TEXT("Byte"))
	{
		NewPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported variable type: %s. Supported: Boolean, Integer, Float, Double, String, Vector, Rotator, Name, Text, Byte"), *NewType));
	}

	// Change the variable type - this handles node updates and marks modified
	FBlueprintEditorUtils::ChangeMemberVariableType(Blueprint, VarFName, NewPinType);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("variable"), VariableName);
	ResultObj->SetStringField(TEXT("old_type"), OldType);
	ResultObj->SetStringField(TEXT("new_type"), NewType);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}