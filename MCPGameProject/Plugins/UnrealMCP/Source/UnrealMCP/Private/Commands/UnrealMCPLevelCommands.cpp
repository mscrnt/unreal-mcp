#include "Commands/UnrealMCPLevelCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "LevelEditorSubsystem.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "AssetRegistry/AssetRegistryModule.h"

FUnrealMCPLevelCommands::FUnrealMCPLevelCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("new_level"))
	{
		return HandleNewLevel(Params);
	}
	else if (CommandType == TEXT("load_level"))
	{
		return HandleLoadLevel(Params);
	}
	else if (CommandType == TEXT("save_level"))
	{
		return HandleSaveLevel(Params);
	}
	else if (CommandType == TEXT("save_all_levels"))
	{
		return HandleSaveAllLevels(Params);
	}
	else if (CommandType == TEXT("get_current_level"))
	{
		return HandleGetCurrentLevel(Params);
	}
	else if (CommandType == TEXT("play_in_editor"))
	{
		return HandlePlayInEditor(Params);
	}
	else if (CommandType == TEXT("stop_play_in_editor"))
	{
		return HandleStopPlayInEditor(Params);
	}
	else if (CommandType == TEXT("is_playing"))
	{
		return HandleIsPlaying(Params);
	}
	else if (CommandType == TEXT("execute_console_command"))
	{
		return HandleExecuteConsoleCommand(Params);
	}
	else if (CommandType == TEXT("build_lighting"))
	{
		return HandleBuildLighting(Params);
	}
	else if (CommandType == TEXT("set_world_settings"))
	{
		return HandleSetWorldSettings(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown level command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleNewLevel(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	FString AssetPath;
	Params->TryGetStringField(TEXT("asset_path"), AssetPath);

	FString TemplatePath;
	bool bHasTemplate = Params->TryGetStringField(TEXT("template"), TemplatePath);

	bool bSuccess = false;
	if (bHasTemplate && !TemplatePath.IsEmpty())
	{
		bSuccess = LevelEditorSubsystem->NewLevelFromTemplate(AssetPath, TemplatePath);
	}
	else
	{
		bSuccess = LevelEditorSubsystem->NewLevel(AssetPath);
	}

	if (!bSuccess)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create new level"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("level_path"), AssetPath);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleLoadLevel(const TSharedPtr<FJsonObject>& Params)
{
	FString LevelPath;
	if (!Params->TryGetStringField(TEXT("level_path"), LevelPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'level_path' parameter"));
	}

	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	// Record the old world name so we can verify the level actually changed
	FString OldLevelName;
	if (UWorld* OldWorld = GEditor->GetEditorWorldContext().World())
	{
		OldLevelName = OldWorld->GetMapName();
	}

	// Try loading - also try common path variations if the initial path fails
	bool bLoaded = LevelEditorSubsystem->LoadLevel(LevelPath);

	if (!bLoaded)
	{
		// Try with /Game/ prefix if not already there
		if (!LevelPath.StartsWith(TEXT("/Game/")))
		{
			FString GamePath = TEXT("/Game/") + LevelPath;
			bLoaded = LevelEditorSubsystem->LoadLevel(GamePath);
			if (bLoaded) LevelPath = GamePath;
		}
	}

	if (!bLoaded)
	{
		// Try searching the asset registry for a map with this name
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> MapAssets;
		AssetRegistry.GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("World")), MapAssets);

		FString SearchName = FPaths::GetBaseFilename(LevelPath);
		for (const FAssetData& MapAsset : MapAssets)
		{
			if (MapAsset.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
			{
				FString FoundPath = MapAsset.GetObjectPathString();
				// Remove the .MapName suffix for LoadLevel
				FString PackagePath = MapAsset.PackageName.ToString();
				bLoaded = LevelEditorSubsystem->LoadLevel(PackagePath);
				if (bLoaded)
				{
					LevelPath = PackagePath;
					break;
				}
			}
		}
	}

	if (!bLoaded)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Failed to load level: %s (tried exact path, /Game/ prefix, and asset registry search)"), *LevelPath));
	}

	// Verify the level actually changed by checking the new world
	UWorld* NewWorld = GEditor->GetEditorWorldContext().World();
	FString NewLevelName = NewWorld ? NewWorld->GetMapName() : TEXT("unknown");
	FString NewLevelPath = NewWorld ? NewWorld->GetPathName() : TEXT("unknown");

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("requested_path"), LevelPath);
	ResultObj->SetStringField(TEXT("loaded_level_name"), NewLevelName);
	ResultObj->SetStringField(TEXT("loaded_level_path"), NewLevelPath);
	ResultObj->SetStringField(TEXT("previous_level"), OldLevelName);
	ResultObj->SetBoolField(TEXT("level_changed"), NewLevelName != OldLevelName);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleSaveLevel(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	if (!LevelEditorSubsystem->SaveCurrentLevel())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to save current level"));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleSaveAllLevels(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	if (!LevelEditorSubsystem->SaveAllDirtyLevels())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to save all dirty levels"));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleGetCurrentLevel(const TSharedPtr<FJsonObject>& Params)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("level_name"), World->GetMapName());
	ResultObj->SetStringField(TEXT("level_path"), World->GetPathName());
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandlePlayInEditor(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	if (LevelEditorSubsystem->IsInPlayInEditor())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Already playing in editor"));
	}

	// Check for optional "simulate" parameter — default is Play mode (not Simulate)
	bool bSimulate = false;
	Params->TryGetBoolField(TEXT("simulate"), bSimulate);

	if (bSimulate)
	{
		LevelEditorSubsystem->EditorPlaySimulate();
	}
	else
	{
		// Use RequestPlaySession for proper Play-In-Editor with player possession
		FRequestPlaySessionParams SessionParams;
		SessionParams.WorldType = EPlaySessionWorldType::PlayInEditor;
		GEditor->RequestPlaySession(SessionParams);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("playing"), true);
	ResultObj->SetStringField(TEXT("mode"), bSimulate ? TEXT("simulate") : TEXT("play"));
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleStopPlayInEditor(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	if (!LevelEditorSubsystem->IsInPlayInEditor())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Not currently playing in editor"));
	}

	// Request PIE to end. This is deferred — PIE tears down on subsequent tick(s).
	// The GetTargetWorld() guard (RF_BeginDestroyed / bIsWorldInitialized check)
	// prevents subsequent commands from grabbing the dying PIE world.
	// NOTE: We cannot wait/spin here because we're on the game thread (inside an
	// AsyncTask lambda) — pumping the task graph would cause re-entrant execution.
	LevelEditorSubsystem->EditorRequestEndPlay();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("playing"), false);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleIsPlaying(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("is_playing"), LevelEditorSubsystem->IsInPlayInEditor());
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

// Custom output device to capture console command output
class FMCPOutputDevice : public FOutputDevice
{
public:
	FString CapturedOutput;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!CapturedOutput.IsEmpty())
		{
			CapturedOutput += TEXT("\n");
		}
		CapturedOutput += V;
	}
};

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleExecuteConsoleCommand(const TSharedPtr<FJsonObject>& Params)
{
	FString Command;
	if (!Params->TryGetStringField(TEXT("command"), Command))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'command' parameter"));
	}

	FString Output;
	if (GEditor)
	{
		FMCPOutputDevice OutputDevice;
		GEditor->Exec(GEditor->GetEditorWorldContext().World(), *Command, OutputDevice);
		Output = OutputDevice.CapturedOutput;
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("command"), Command);
	ResultObj->SetBoolField(TEXT("executed"), true);
	if (!Output.IsEmpty())
	{
		ResultObj->SetStringField(TEXT("output"), Output);
	}
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleBuildLighting(const TSharedPtr<FJsonObject>& Params)
{
	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	if (!LevelEditorSubsystem)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get LevelEditorSubsystem"));
	}

	FString QualityStr;
	Params->TryGetStringField(TEXT("quality"), QualityStr);

	// Default to production quality
	ELightingBuildQuality Quality = ELightingBuildQuality::Quality_Production;
	if (QualityStr == TEXT("Preview"))
	{
		Quality = ELightingBuildQuality::Quality_Preview;
	}
	else if (QualityStr == TEXT("Medium"))
	{
		Quality = ELightingBuildQuality::Quality_Medium;
	}
	else if (QualityStr == TEXT("High"))
	{
		Quality = ELightingBuildQuality::Quality_High;
	}

	bool bWithReflectionCaptures = false;
	Params->TryGetBoolField(TEXT("with_reflection_captures"), bWithReflectionCaptures);

	if (!LevelEditorSubsystem->BuildLightMaps(Quality, bWithReflectionCaptures))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to build lighting"));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleSetWorldSettings(const TSharedPtr<FJsonObject>& Params)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();
	if (!WorldSettings)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get world settings"));
	}

	// Set game mode override
	FString GameModeClass;
	if (Params->TryGetStringField(TEXT("game_mode"), GameModeClass))
	{
		UClass* GMClass = FindFirstObject<UClass>(*GameModeClass);
		if (!GMClass)
		{
			GMClass = LoadObject<UClass>(nullptr, *GameModeClass);
		}
		if (GMClass)
		{
			WorldSettings->DefaultGameMode = GMClass;
		}
		else
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Game mode class not found: %s"), *GameModeClass));
		}
	}

	// Set KillZ
	double KillZ;
	if (Params->TryGetNumberField(TEXT("kill_z"), KillZ))
	{
		WorldSettings->KillZ = KillZ;
	}

	// Mark world settings as modified
	WorldSettings->Modify();

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}
