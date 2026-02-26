#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Editor-related MCP commands
 * Handles viewport control, actor manipulation, and level management
 */
class UNREALMCP_API FUnrealMCPEditorCommands
{
public:
    FUnrealMCPEditorCommands();

    // Handle editor commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Actor manipulation commands
    TSharedPtr<FJsonObject> HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params);

    // Blueprint actor spawning
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);

    // Editor viewport commands
    TSharedPtr<FJsonObject> HandleFocusViewport(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params);

    // Phase 5: Editor Enhancements
    TSharedPtr<FJsonObject> HandleSelectActors(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetSelectedActors(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDuplicateActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetViewportCamera(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetViewportCamera(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetActorMobility(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetActorMaterial(const TSharedPtr<FJsonObject>& Params);

    // Actor tags
    TSharedPtr<FJsonObject> HandleSetActorTags(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetActorTags(const TSharedPtr<FJsonObject>& Params);

    // PIE / RL tools
    TSharedPtr<FJsonObject> HandleAddMovementInput(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandlePawnAction(const TSharedPtr<FJsonObject>& Params);
}; 