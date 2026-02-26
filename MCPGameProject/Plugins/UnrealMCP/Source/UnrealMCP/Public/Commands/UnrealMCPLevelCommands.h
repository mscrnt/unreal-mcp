#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class UNREALMCP_API FUnrealMCPLevelCommands
{
public:
	FUnrealMCPLevelCommands();

	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	TSharedPtr<FJsonObject> HandleNewLevel(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleLoadLevel(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSaveLevel(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSaveAllLevels(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetCurrentLevel(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandlePlayInEditor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleStopPlayInEditor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleIsPlaying(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleExecuteConsoleCommand(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleBuildLighting(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetWorldSettings(const TSharedPtr<FJsonObject>& Params);
};
