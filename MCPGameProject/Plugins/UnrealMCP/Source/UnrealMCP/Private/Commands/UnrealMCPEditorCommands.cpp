#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerStart.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "RenderingThread.h"
#include "UnrealClient.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorAssetLibrary.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Phase 5: Editor Enhancements
    else if (CommandType == TEXT("select_actors"))
    {
        return HandleSelectActors(Params);
    }
    else if (CommandType == TEXT("get_selected_actors"))
    {
        return HandleGetSelectedActors(Params);
    }
    else if (CommandType == TEXT("duplicate_actor"))
    {
        return HandleDuplicateActor(Params);
    }
    else if (CommandType == TEXT("set_viewport_camera"))
    {
        return HandleSetViewportCamera(Params);
    }
    else if (CommandType == TEXT("get_viewport_camera"))
    {
        return HandleGetViewportCamera(Params);
    }
    else if (CommandType == TEXT("set_actor_mobility"))
    {
        return HandleSetActorMobility(Params);
    }
    else if (CommandType == TEXT("set_actor_material"))
    {
        return HandleSetActorMaterial(Params);
    }
    else if (CommandType == TEXT("set_actor_tags"))
    {
        return HandleSetActorTags(Params);
    }
    else if (CommandType == TEXT("get_actor_tags"))
    {
        return HandleGetActorTags(Params);
    }
    else if (CommandType == TEXT("add_movement_input"))
    {
        return HandleAddMovementInput(Params);
    }
    else if (CommandType == TEXT("pawn_action"))
    {
        return HandlePawnAction(Params);
    }

    // Editor Utility Subsystem
    else if (CommandType == TEXT("run_editor_utility"))
    {
        return HandleRunEditorUtility(Params);
    }
    else if (CommandType == TEXT("spawn_editor_utility_tab"))
    {
        return HandleSpawnEditorUtilityTab(Params);
    }
    else if (CommandType == TEXT("close_editor_utility_tab"))
    {
        return HandleCloseEditorUtilityTab(Params);
    }
    else if (CommandType == TEXT("does_editor_utility_tab_exist"))
    {
        return HandleDoesEditorUtilityTabExist(Params);
    }
    else if (CommandType == TEXT("find_editor_utility_widget"))
    {
        return HandleFindEditorUtilityWidget(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = FUnrealMCPCommonUtils::GetTargetWorld();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No world available"));
    }

    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    ResultObj->SetStringField(TEXT("world"), World->GetMapName());
    ResultObj->SetBoolField(TEXT("is_pie"), World->WorldType == EWorldType::PIE);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }

    UWorld* World = FUnrealMCPCommonUtils::GetTargetWorld();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No world available"));
    }

    // Count total actors for diagnostics
    int32 TotalActors = 0;
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor)
        {
            TotalActors++;
            if (Actor->GetName().Contains(Pattern) || Actor->GetActorLabel().Contains(Pattern))
            {
                MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
            }
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    ResultObj->SetNumberField(TEXT("count"), MatchingActors.Num());
    ResultObj->SetNumberField(TEXT("total_actors_in_world"), TotalActors);
    ResultObj->SetStringField(TEXT("world"), World->GetMapName());
    ResultObj->SetNumberField(TEXT("world_type_id"), (int32)World->WorldType);
    ResultObj->SetBoolField(TEXT("is_pie"), World->WorldType == EWorldType::PIE);
    ResultObj->SetStringField(TEXT("pattern"), Pattern);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PlayerStart"))
    {
        NewActor = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        // Generic fallback: try to find the class by name
        UClass* ActorClass = nullptr;

        // Try exact name
        ActorClass = FindFirstObject<UClass>(*ActorType);

        // Try with A prefix (UE actor naming convention)
        if (!ActorClass && !ActorType.StartsWith(TEXT("A")))
        {
            ActorClass = FindFirstObject<UClass>(*(TEXT("A") + ActorType));
        }

        // Verify it's an Actor subclass
        if (ActorClass && ActorClass->IsChildOf(AActor::StaticClass()))
        {
            NewActor = World->SpawnActor(ActorClass, &Location, &Rotation, SpawnParams);
        }
        else
        {
            FString DiagInfo = FString::Printf(TEXT("Unknown actor type: %s. Built-in types: StaticMeshActor, PointLight, SpotLight, DirectionalLight, CameraActor, PlayerStart. "
                "Also accepts any AActor subclass name (e.g. 'TriggerBox', 'TargetPoint')."), *ActorType);
            if (ActorClass)
            {
                DiagInfo += FString::Printf(TEXT(" Found class '%s' but it is not an Actor subclass."), *ActorClass->GetName());
            }
            return FUnrealMCPCommonUtils::CreateErrorResponse(DiagInfo);
        }
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
    if (!Actor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
    Actor->Destroy();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* TargetActor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* TargetActor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* TargetActor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));
    
    // Set the property using our utility function
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully on the actor
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        // Property not found on actor — fall back to searching actor's components.
        // This handles SkeletalMeshActor.SkeletalMesh, StaticMeshActor.StaticMesh, etc.
        for (UActorComponent* Component : TargetActor->GetComponents())
        {
            if (!Component) continue;
            FString CompError;
            if (FUnrealMCPCommonUtils::SetObjectProperty(Component, PropertyName, PropertyValue, CompError))
            {
                // Notify the component about the change
                FProperty* ChangedProp = FindFProperty<FProperty>(Component->GetClass(), *PropertyName);
                if (ChangedProp)
                {
                    FPropertyChangedEvent PropEvent(ChangedProp);
                    Component->PostEditChangeProperty(PropEvent);
                }

                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("actor"), ActorName);
                ResultObj->SetStringField(TEXT("property"), PropertyName);
                ResultObj->SetStringField(TEXT("set_on_component"), Component->GetName());
                ResultObj->SetBoolField(TEXT("success"), true);
                ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
                return ResultObj;
            }
        }
        // Neither actor nor components had the property
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint - try multiple resolution strategies
    if (BlueprintName.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    UBlueprint* Blueprint = nullptr;
    FString ResolvedPath;

    // Strategy 1: Treat as full content path (e.g. "/Game/ParkourRace/Blueprints/BP_RLAgent")
    if (BlueprintName.StartsWith(TEXT("/")))
    {
        Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintName);
        if (!Blueprint)
        {
            // Try Package.ObjectName format
            FString BaseName = FPaths::GetBaseFilename(BlueprintName);
            Blueprint = LoadObject<UBlueprint>(nullptr, *(BlueprintName + TEXT(".") + BaseName));
        }
        if (Blueprint) ResolvedPath = BlueprintName;
    }

    // Strategy 2: Try under /Game/Blueprints/ (legacy default)
    if (!Blueprint)
    {
        FString LegacyPath = TEXT("/Game/Blueprints/") + BlueprintName;
        Blueprint = LoadObject<UBlueprint>(nullptr, *LegacyPath);
        if (!Blueprint)
        {
            FString BaseName = FPaths::GetBaseFilename(LegacyPath);
            Blueprint = LoadObject<UBlueprint>(nullptr, *(LegacyPath + TEXT(".") + BaseName));
        }
        if (Blueprint) ResolvedPath = LegacyPath;
    }

    // Strategy 3: Search asset registry by name
    if (!Blueprint)
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        FString SearchName = FPaths::GetBaseFilename(BlueprintName);
        TArray<FAssetData> FoundAssets;
        AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), FoundAssets);

        for (const FAssetData& Asset : FoundAssets)
        {
            if (Asset.AssetName.ToString() == SearchName)
            {
                Blueprint = Cast<UBlueprint>(Asset.GetAsset());
                if (Blueprint)
                {
                    ResolvedPath = Asset.GetObjectPathString();
                    break;
                }
            }
        }
    }

    if (!Blueprint || !Blueprint->GeneratedClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("Blueprint not found: '%s'. Provide a full path like '/Game/MyFolder/BP_Name' or just the asset name."), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = FUnrealMCPCommonUtils::GetTargetWorld(false); // Use editor world for spawning
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists (check both internal name and label)
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName || It->GetActorLabel() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                TEXT("Actor with name '%s' already exists. Use a different name or delete the existing actor first."), *ActorName));
        }
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    // Use Requested mode so UE generates a unique name on conflict instead of fatal crash
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
        ResultObj->SetStringField(TEXT("blueprint_path"), ResolvedPath);
        return ResultObj;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        AActor* TargetActor = FUnrealMCPCommonUtils::FindActorByName(TargetActorName, /*bPreferPIE=*/ false);
        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Get file path parameter (optional - defaults to temp directory)
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath) || FilePath.IsEmpty())
    {
        FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("MCP_Screenshot.png"));
    }

    // Ensure the file path has a proper extension
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    // Ensure directory exists
    FString Directory = FPaths::GetPath(FilePath);
    if (!Directory.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
    }

    bool bIsPIE = GEditor && GEditor->IsPlayingSessionInEditor();
    FString ViewportSource = TEXT("unknown");
    FViewport* Viewport = nullptr;

    // During PIE, capture the game viewport
    if (bIsPIE)
    {
        if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
        {
            Viewport = GEngine->GameViewport->Viewport;
            ViewportSource = TEXT("PIE_GameViewport");
        }
    }

    // For editor mode, get the active level editor viewport (the 3D perspective view)
    if (!Viewport)
    {
        FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveLevelViewport();
        if (ActiveLevelViewport.IsValid())
        {
            FLevelEditorViewportClient& ViewportClient = ActiveLevelViewport->GetLevelViewportClient();
            Viewport = ViewportClient.Viewport;
            ViewportSource = TEXT("LevelEditor_ActiveViewport");
        }
    }

    // Last resort fallback
    if (!Viewport && GEditor && GEditor->GetActiveViewport())
    {
        Viewport = GEditor->GetActiveViewport();
        ViewportSource = TEXT("Editor_ActiveViewport");
    }

    if (!Viewport)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No viewport available for screenshot"));
    }

    int32 Width = Viewport->GetSizeXY().X;
    int32 Height = Viewport->GetSizeXY().Y;

    if (Width <= 0 || Height <= 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("Viewport has invalid size: %dx%d (source: %s)"), Width, Height, *ViewportSource));
    }

    FlushRenderingCommands();

    TArray<FColor> Bitmap;
    FIntRect ViewportRect(0, 0, Width, Height);
    FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
    ReadFlags.SetLinearToGamma(false);

    if (!Viewport->ReadPixels(Bitmap, ReadFlags, ViewportRect))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("ReadPixels failed for viewport %dx%d (source: %s)"), Width, Height, *ViewportSource));
    }

    // Force alpha to 255 - viewport ReadPixels returns A=0 which makes
    // the PNG fully transparent (appears white). UE's own screenshot code
    // (UGameViewportClient::ProcessScreenShots) does the same fix.
    for (FColor& Pixel : Bitmap)
    {
        Pixel.A = 255;
    }

    TArray64<uint8> CompressedBitmap;
    FImageUtils::PNGCompressImageArray(Width, Height, Bitmap, CompressedBitmap);

    if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath))
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("filepath"), FilePath);
        ResultObj->SetNumberField(TEXT("width"), Width);
        ResultObj->SetNumberField(TEXT("height"), Height);
        ResultObj->SetBoolField(TEXT("is_pie"), bIsPIE);
        ResultObj->SetStringField(TEXT("viewport_source"), ViewportSource);
        return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to save screenshot to: %s"), *FilePath));
    }
}

// ============================================================
// Phase 5: Editor Enhancements
// ============================================================

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSelectActors(const TSharedPtr<FJsonObject>& Params)
{
	const TArray<TSharedPtr<FJsonValue>>* NamesArray;
	if (!Params->TryGetArrayField(TEXT("names"), NamesArray))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'names' parameter"));
	}

	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	if (!EditorActorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorActorSubsystem"));
	}

	// Clear current selection
	GEditor->SelectNone(true, true);

	int32 SelectedCount = 0;
	for (const TSharedPtr<FJsonValue>& NameVal : *NamesArray)
	{
		FString ActorName = NameVal->AsString();
		AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName, /*bPreferPIE=*/ false);
		if (Actor)
		{
			GEditor->SelectActor(Actor, true, true);
			SelectedCount++;
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetNumberField(TEXT("selected_count"), SelectedCount);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetSelectedActors(const TSharedPtr<FJsonObject>& Params)
{
	USelection* Selection = GEditor->GetSelectedActors();
	if (!Selection)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor selection"));
	}

	TArray<TSharedPtr<FJsonValue>> ActorArray;
	for (int32 i = 0; i < Selection->Num(); i++)
	{
		AActor* Actor = Cast<AActor>(Selection->GetSelectedObject(i));
		if (Actor)
		{
			ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetArrayField(TEXT("actors"), ActorArray);
	ResultObj->SetNumberField(TEXT("count"), ActorArray.Num());
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDuplicateActor(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString NewName;
	Params->TryGetStringField(TEXT("new_name"), NewName);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world"));

	// Find source actor (editor world only - can't duplicate PIE actors)
	AActor* SourceActor = FUnrealMCPCommonUtils::FindActorByName(ActorName, /*bPreferPIE=*/ false);
	if (!SourceActor) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));

	// Use EditorActorSubsystem to duplicate
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	if (!EditorActorSubsystem) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorActorSubsystem"));

	AActor* DuplicatedActor = EditorActorSubsystem->DuplicateActor(SourceActor, World);
	if (!DuplicatedActor) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to duplicate actor"));

	// Set location if provided
	if (Params->HasField(TEXT("location")))
	{
		FVector Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
		DuplicatedActor->SetActorLocation(Location);
	}

	// Rename if requested
	if (!NewName.IsEmpty())
	{
		DuplicatedActor->Rename(*NewName);
		DuplicatedActor->SetActorLabel(NewName);
	}

	return FUnrealMCPCommonUtils::ActorToJsonObject(DuplicatedActor, true);
}

// Helper: safely get the first level editor viewport client
static FLevelEditorViewportClient* GetLevelEditorViewportClient()
{
	// Try active viewport first
	FViewport* ActiveViewport = GEditor->GetActiveViewport();
	if (ActiveViewport)
	{
		FLevelEditorViewportClient* Client = static_cast<FLevelEditorViewportClient*>(ActiveViewport->GetClient());
		if (Client) return Client;
	}

	// Fallback: iterate all level viewports and use the first perspective one
	for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
	{
		if (LevelVC)
		{
			return LevelVC;
		}
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetViewportCamera(const TSharedPtr<FJsonObject>& Params)
{
	FLevelEditorViewportClient* ViewportClient = GetLevelEditorViewportClient();
	if (!ViewportClient)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No level editor viewport available"));
	}

	if (Params->HasField(TEXT("location")))
	{
		FVector Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
		ViewportClient->SetViewLocation(Location);
	}

	if (Params->HasField(TEXT("rotation")))
	{
		FRotator Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
		ViewportClient->SetViewRotation(Rotation);
	}

	ViewportClient->Invalidate();

	// Force an immediate redraw so the next screenshot reflects the new camera position
	GEditor->RedrawAllViewports(true);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	FVector Loc = ViewportClient->GetViewLocation();
	FRotator Rot = ViewportClient->GetViewRotation();
	TArray<TSharedPtr<FJsonValue>> LocArr;
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.X));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Y));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Z));
	TArray<TSharedPtr<FJsonValue>> RotArr;
	RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
	RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
	RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
	ResultObj->SetArrayField(TEXT("location"), LocArr);
	ResultObj->SetArrayField(TEXT("rotation"), RotArr);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetViewportCamera(const TSharedPtr<FJsonObject>& Params)
{
	FLevelEditorViewportClient* ViewportClient = GetLevelEditorViewportClient();
	if (!ViewportClient)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No level editor viewport available"));
	}

	FVector Loc = ViewportClient->GetViewLocation();
	FRotator Rot = ViewportClient->GetViewRotation();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> LocArr;
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.X));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Y));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Z));
	TArray<TSharedPtr<FJsonValue>> RotArr;
	RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
	RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
	RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
	ResultObj->SetArrayField(TEXT("location"), LocArr);
	ResultObj->SetArrayField(TEXT("rotation"), RotArr);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorMobility(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString MobilityStr;
	if (!Params->TryGetStringField(TEXT("mobility"), MobilityStr))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'mobility' parameter"));
	}

	EComponentMobility::Type Mobility;
	if (MobilityStr == TEXT("Static")) Mobility = EComponentMobility::Static;
	else if (MobilityStr == TEXT("Stationary")) Mobility = EComponentMobility::Stationary;
	else if (MobilityStr == TEXT("Movable")) Mobility = EComponentMobility::Movable;
	else return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid mobility: %s (use Static/Stationary/Movable)"), *MobilityStr));

	AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	USceneComponent* RootComp = Actor->GetRootComponent();
	if (!RootComp)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no root component"));
	}

	RootComp->SetMobility(Mobility);
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("mobility"), MobilityStr);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorMaterial(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
	}

	int32 SlotIndex = 0;
	Params->TryGetNumberField(TEXT("slot"), SlotIndex);

	// Load material
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
	if (!Material)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Material = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
	}
	if (!Material) return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));

	AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	// Try StaticMeshComponent first, then SkeletalMeshComponent
	UMeshComponent* MeshComp = Cast<UMeshComponent>(Actor->FindComponentByClass<UStaticMeshComponent>());
	FString CompType = TEXT("StaticMeshComponent");
	if (!MeshComp)
	{
		MeshComp = Cast<UMeshComponent>(Actor->FindComponentByClass<USkeletalMeshComponent>());
		CompType = TEXT("SkeletalMeshComponent");
	}
	if (!MeshComp)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no StaticMeshComponent or SkeletalMeshComponent"));
	}

	MeshComp->SetMaterial(SlotIndex, Material);
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("material"), MaterialPath);
	ResultObj->SetNumberField(TEXT("slot"), SlotIndex);
	ResultObj->SetStringField(TEXT("component_type"), CompType);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

// ============================================================
// Actor Tags
// ============================================================

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTags(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	if (!Params->HasField(TEXT("tags")))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'tags' parameter (array of strings)"));
	}

	// Parse tags from JSON array
	const TArray<TSharedPtr<FJsonValue>>& TagsArray = Params->GetArrayField(TEXT("tags"));

	// Check mode: "add", "remove", or "set" (default: "set" replaces all tags)
	FString Mode = TEXT("set");
	Params->TryGetStringField(TEXT("mode"), Mode);

	Actor->Modify();

	if (Mode == TEXT("add"))
	{
		for (const TSharedPtr<FJsonValue>& TagValue : TagsArray)
		{
			FName Tag(*TagValue->AsString());
			Actor->Tags.AddUnique(Tag);
		}
	}
	else if (Mode == TEXT("remove"))
	{
		for (const TSharedPtr<FJsonValue>& TagValue : TagsArray)
		{
			FName Tag(*TagValue->AsString());
			Actor->Tags.Remove(Tag);
		}
	}
	else // "set" — replace all tags
	{
		Actor->Tags.Empty();
		for (const TSharedPtr<FJsonValue>& TagValue : TagsArray)
		{
			Actor->Tags.Add(FName(*TagValue->AsString()));
		}
	}

	// Build response with current tags
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("mode"), Mode);

	TArray<TSharedPtr<FJsonValue>> CurrentTags;
	for (const FName& Tag : Actor->Tags)
	{
		CurrentTags.Add(MakeShared<FJsonValueString>(Tag.ToString()));
	}
	ResultObj->SetArrayField(TEXT("tags"), CurrentTags);
	ResultObj->SetNumberField(TEXT("tag_count"), Actor->Tags.Num());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorTags(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);

	TArray<TSharedPtr<FJsonValue>> TagsJson;
	for (const FName& Tag : Actor->Tags)
	{
		TagsJson.Add(MakeShared<FJsonValueString>(Tag.ToString()));
	}
	ResultObj->SetArrayField(TEXT("tags"), TagsJson);
	ResultObj->SetNumberField(TEXT("tag_count"), Actor->Tags.Num());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

// ============================================================
// PIE / RL Tools
// ============================================================

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleAddMovementInput(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' is not a Pawn (class: %s)"), *ActorName, *Actor->GetClass()->GetName()));
	}

	// Get direction vector
	FVector Direction(0.f, 0.f, 0.f);
	if (Params->HasField(TEXT("direction")))
	{
		Direction = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("direction"));
	}
	else
	{
		// Accept individual axis floats for convenience
		double X = 0, Y = 0, Z = 0;
		Params->TryGetNumberField(TEXT("x"), X);
		Params->TryGetNumberField(TEXT("y"), Y);
		Params->TryGetNumberField(TEXT("z"), Z);
		Direction = FVector(X, Y, Z);
	}

	double Scale = 1.0;
	Params->TryGetNumberField(TEXT("scale"), Scale);

	Pawn->AddMovementInput(Direction, Scale);

	// Return current state after input
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);

	FVector Loc = Pawn->GetActorLocation();
	TArray<TSharedPtr<FJsonValue>> LocArr;
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.X));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Y));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Z));
	ResultObj->SetArrayField(TEXT("location"), LocArr);

	FVector Vel(0, 0, 0);
	ACharacter* Character = Cast<ACharacter>(Pawn);
	if (Character && Character->GetCharacterMovement())
	{
		Vel = Character->GetCharacterMovement()->Velocity;
	}
	TArray<TSharedPtr<FJsonValue>> VelArr;
	VelArr.Add(MakeShared<FJsonValueNumber>(Vel.X));
	VelArr.Add(MakeShared<FJsonValueNumber>(Vel.Y));
	VelArr.Add(MakeShared<FJsonValueNumber>(Vel.Z));
	ResultObj->SetArrayField(TEXT("velocity"), VelArr);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandlePawnAction(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString Action;
	if (!Params->TryGetStringField(TEXT("action"), Action))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action' parameter"));
	}

	AActor* Actor = FUnrealMCPCommonUtils::FindActorByName(ActorName);
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' is not a Pawn"), *ActorName));
	}

	ACharacter* Character = Cast<ACharacter>(Pawn);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("action"), Action);

	if (Action == TEXT("jump") || Action == TEXT("Jump"))
	{
		if (!Character)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor is not a Character — cannot jump"));
		}
		Character->Jump();
		ResultObj->SetBoolField(TEXT("is_jumping"), true);
	}
	else if (Action == TEXT("stop_jumping") || Action == TEXT("StopJumping"))
	{
		if (!Character)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor is not a Character — cannot stop jumping"));
		}
		Character->StopJumping();
	}
	else if (Action == TEXT("crouch") || Action == TEXT("Crouch"))
	{
		if (!Character)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor is not a Character — cannot crouch"));
		}
		Character->Crouch();
		ResultObj->SetBoolField(TEXT("is_crouched"), Character->bIsCrouched);
	}
	else if (Action == TEXT("uncrouch") || Action == TEXT("UnCrouch"))
	{
		if (!Character)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor is not a Character — cannot uncrouch"));
		}
		Character->UnCrouch();
	}
	else if (Action == TEXT("launch"))
	{
		// Launch with a velocity vector — useful for applying impulses
		FVector LaunchVelocity(0, 0, 0);
		if (Params->HasField(TEXT("velocity")))
		{
			LaunchVelocity = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("velocity"));
		}
		bool bXYOverride = false, bZOverride = false;
		Params->TryGetBoolField(TEXT("xy_override"), bXYOverride);
		Params->TryGetBoolField(TEXT("z_override"), bZOverride);

		if (Character)
		{
			Character->LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
		}
		else
		{
			// For non-Character pawns, just set velocity on the movement component
			UPawnMovementComponent* MoveComp = Pawn->GetMovementComponent();
			if (MoveComp)
			{
				MoveComp->Velocity = LaunchVelocity;
			}
		}
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Unknown action: %s. Supported: jump, stop_jumping, crouch, uncrouch, launch"), *Action));
	}

	// Return current state
	FVector Loc = Pawn->GetActorLocation();
	TArray<TSharedPtr<FJsonValue>> LocArr;
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.X));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Y));
	LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Z));
	ResultObj->SetArrayField(TEXT("location"), LocArr);

	if (Character && Character->GetCharacterMovement())
	{
		FVector Vel = Character->GetCharacterMovement()->Velocity;
		TArray<TSharedPtr<FJsonValue>> VelArr;
		VelArr.Add(MakeShared<FJsonValueNumber>(Vel.X));
		VelArr.Add(MakeShared<FJsonValueNumber>(Vel.Y));
		VelArr.Add(MakeShared<FJsonValueNumber>(Vel.Z));
		ResultObj->SetArrayField(TEXT("velocity"), VelArr);
		ResultObj->SetBoolField(TEXT("is_falling"), Character->GetCharacterMovement()->IsFalling());
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

// ============================================================================
// Editor Utility Subsystem Tools
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleRunEditorUtility(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
	}

	UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!Asset)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	UEditorUtilitySubsystem* EUS = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (!EUS)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorUtilitySubsystem"));
	}

	if (!EUS->CanRun(Asset))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset cannot be run as an Editor Utility: %s. Must be an Editor Utility Blueprint or Editor Utility Widget Blueprint."), *AssetPath));
	}

	bool bSuccess = EUS->TryRun(Asset);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObj->SetStringField(TEXT("asset_class"), Asset->GetClass()->GetName());
	ResultObj->SetBoolField(TEXT("success"), bSuccess);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnEditorUtilityTab(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
	}

	UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!Asset)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	UEditorUtilityWidgetBlueprint* WidgetBP = Cast<UEditorUtilityWidgetBlueprint>(Asset);
	if (!WidgetBP)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not an Editor Utility Widget Blueprint: %s (class: %s)"), *AssetPath, *Asset->GetClass()->GetName()));
	}

	UEditorUtilitySubsystem* EUS = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (!EUS)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorUtilitySubsystem"));
	}

	FName TabID;
	FString RequestedTabId;
	UEditorUtilityWidget* Widget = nullptr;

	if (Params->TryGetStringField(TEXT("tab_id"), RequestedTabId) && !RequestedTabId.IsEmpty())
	{
		Widget = EUS->SpawnAndRegisterTabWithId(WidgetBP, FName(*RequestedTabId));
		TabID = FName(*RequestedTabId);
	}
	else
	{
		Widget = EUS->SpawnAndRegisterTabAndGetID(WidgetBP, TabID);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObj->SetStringField(TEXT("tab_id"), TabID.ToString());
	ResultObj->SetBoolField(TEXT("widget_created"), Widget != nullptr);
	if (Widget)
	{
		ResultObj->SetStringField(TEXT("widget_class"), Widget->GetClass()->GetName());
	}
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCloseEditorUtilityTab(const TSharedPtr<FJsonObject>& Params)
{
	FString TabId;
	if (!Params->TryGetStringField(TEXT("tab_id"), TabId))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'tab_id' parameter"));
	}

	UEditorUtilitySubsystem* EUS = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (!EUS)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorUtilitySubsystem"));
	}

	bool bClosed = EUS->CloseTabByID(FName(*TabId));

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("tab_id"), TabId);
	ResultObj->SetBoolField(TEXT("closed"), bClosed);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDoesEditorUtilityTabExist(const TSharedPtr<FJsonObject>& Params)
{
	FString TabId;
	if (!Params->TryGetStringField(TEXT("tab_id"), TabId))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'tab_id' parameter"));
	}

	UEditorUtilitySubsystem* EUS = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (!EUS)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorUtilitySubsystem"));
	}

	bool bExists = EUS->DoesTabExist(FName(*TabId));

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("tab_id"), TabId);
	ResultObj->SetBoolField(TEXT("exists"), bExists);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindEditorUtilityWidget(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
	}

	UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!Asset)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	UEditorUtilityWidgetBlueprint* WidgetBP = Cast<UEditorUtilityWidgetBlueprint>(Asset);
	if (!WidgetBP)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not an Editor Utility Widget Blueprint: %s"), *AssetPath));
	}

	UEditorUtilitySubsystem* EUS = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (!EUS)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get EditorUtilitySubsystem"));
	}

	UEditorUtilityWidget* Widget = EUS->FindUtilityWidgetFromBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObj->SetBoolField(TEXT("widget_found"), Widget != nullptr);
	if (Widget)
	{
		ResultObj->SetStringField(TEXT("widget_class"), Widget->GetClass()->GetName());
		ResultObj->SetStringField(TEXT("widget_name"), Widget->GetName());
	}
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}