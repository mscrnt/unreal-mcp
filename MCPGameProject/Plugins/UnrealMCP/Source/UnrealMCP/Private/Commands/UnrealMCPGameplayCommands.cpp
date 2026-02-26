#include "Commands/UnrealMCPGameplayCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameModeBase.h"
#include "GameMapsSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/ConfigCacheIni.h"

FUnrealMCPGameplayCommands::FUnrealMCPGameplayCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("set_default_game_mode"))
	{
		return HandleSetDefaultGameMode(Params);
	}
	else if (CommandType == TEXT("set_default_map"))
	{
		return HandleSetDefaultMap(Params);
	}
	else if (CommandType == TEXT("create_enhanced_input_action"))
	{
		return HandleCreateEnhancedInputAction(Params);
	}
	else if (CommandType == TEXT("create_input_mapping_context"))
	{
		return HandleCreateInputMappingContext(Params);
	}
	else if (CommandType == TEXT("set_project_setting"))
	{
		return HandleSetProjectSetting(Params);
	}
	else if (CommandType == TEXT("get_project_setting"))
	{
		return HandleGetProjectSetting(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown gameplay command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleSetDefaultGameMode(const TSharedPtr<FJsonObject>& Params)
{
	FString GameModeClass;
	if (!Params->TryGetStringField(TEXT("game_mode_class"), GameModeClass))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'game_mode_class' parameter"));
	}

	// Resolve the class through multiple strategies
	UClass* GMClass = nullptr;
	FString DiagInfo;

	// 1. Try exact path as UClass
	GMClass = LoadObject<UClass>(nullptr, *GameModeClass);
	if (GMClass) { DiagInfo += TEXT("Found via LoadObject<UClass> exact path. "); }

	// 2. Try with _C suffix (Blueprint generated class)
	if (!GMClass && !GameModeClass.EndsWith(TEXT("_C")))
	{
		FString WithC = GameModeClass + TEXT("_C");
		GMClass = LoadObject<UClass>(nullptr, *WithC);
		if (GMClass) { DiagInfo += TEXT("Found via _C suffix. "); }

		if (!GMClass)
		{
			FString BaseName = FPaths::GetBaseFilename(GameModeClass);
			FString DotC = GameModeClass + TEXT(".") + BaseName + TEXT("_C");
			GMClass = LoadObject<UClass>(nullptr, *DotC);
			if (GMClass) { DiagInfo += TEXT("Found via Path.Name_C format. "); }
		}
	}

	// 3. Try loading as Blueprint and getting GeneratedClass
	if (!GMClass)
	{
		FString BPPath = GameModeClass;
		BPPath.RemoveFromEnd(TEXT("_C"));
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BPPath);
		if (!BP)
		{
			FString BaseName = FPaths::GetBaseFilename(BPPath);
			FString FullBPPath = BPPath + TEXT(".") + BaseName;
			BP = LoadObject<UBlueprint>(nullptr, *FullBPPath);
		}
		if (BP && BP->GeneratedClass)
		{
			GMClass = BP->GeneratedClass;
			DiagInfo += TEXT("Found via Blueprint->GeneratedClass. ");
		}
	}

	// 4. FindFirstObject as last resort
	if (!GMClass)
	{
		GMClass = FindFirstObject<UClass>(*GameModeClass);
		if (GMClass) { DiagInfo += TEXT("Found via FindFirstObject. "); }
	}

	if (!GMClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Game mode class not found: '%s'. Tried: LoadObject<UClass>, _C suffix, Blueprint->GeneratedClass, FindFirstObject. "
				"Use a full path like '/Game/Path/BP_MyGameMode' or '/Script/Engine.GameModeBase'"), *GameModeClass));
	}

	// Verify it's actually a GameModeBase subclass
	if (!GMClass->IsChildOf(AGameModeBase::StaticClass()))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Class '%s' (%s) is not a GameModeBase subclass"), *GameModeClass, *GMClass->GetPathName()));
	}

	// Get the proper class path for config storage
	FString ResolvedClassPath = GMClass->GetPathName();
	DiagInfo += FString::Printf(TEXT("Resolved path: %s. "), *ResolvedClassPath);

	// Set on world settings for the current level
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();
		if (WorldSettings)
		{
			WorldSettings->DefaultGameMode = GMClass;
			WorldSettings->Modify();
			// Mark the level package dirty so PIE picks up the change
			World->MarkPackageDirty();
			DiagInfo += TEXT("Set on WorldSettings + marked package dirty. ");
		}
	}

	// Also set in project settings via config using the resolved class path
	GConfig->SetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GlobalDefaultGameMode"),
		*ResolvedClassPath,
		GEngineIni);
	GConfig->Flush(false, GEngineIni);
	DiagInfo += TEXT("Written to DefaultEngine.ini. ");

	UE_LOG(LogTemp, Display, TEXT("SetDefaultGameMode: %s"), *DiagInfo);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("game_mode_class"), GameModeClass);
	ResultObj->SetStringField(TEXT("resolved_class"), ResolvedClassPath);
	ResultObj->SetStringField(TEXT("debug"), DiagInfo);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleSetDefaultMap(const TSharedPtr<FJsonObject>& Params)
{
	FString MapPath;
	if (!Params->TryGetStringField(TEXT("map_path"), MapPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'map_path' parameter"));
	}

	FString MapType = TEXT("game");
	Params->TryGetStringField(TEXT("map_type"), MapType);

	if (MapType == TEXT("editor"))
	{
		// EditorStartupMap setter doesn't exist in UE 5.6, use config directly
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("EditorStartupMap"),
			*MapPath,
			GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}
	else
	{
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameDefaultMap"),
			*MapPath,
			GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("map_path"), MapPath);
	ResultObj->SetStringField(TEXT("map_type"), MapType);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleCreateEnhancedInputAction(const TSharedPtr<FJsonObject>& Params)
{
	FString ActionName;
	if (!Params->TryGetStringField(TEXT("name"), ActionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString ValueTypeStr = TEXT("Boolean");
	Params->TryGetStringField(TEXT("value_type"), ValueTypeStr);

	FString Path = TEXT("/Game/Input");
	Params->TryGetStringField(TEXT("path"), Path);

	FString PackagePath = Path / (TEXT("IA_") + ActionName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	UInputAction* InputAction = NewObject<UInputAction>(Package, *(TEXT("IA_") + ActionName), RF_Public | RF_Standalone);
	if (!InputAction)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create InputAction"));
	}

	// Set value type
	if (ValueTypeStr == TEXT("Axis1D"))
	{
		InputAction->ValueType = EInputActionValueType::Axis1D;
	}
	else if (ValueTypeStr == TEXT("Axis2D"))
	{
		InputAction->ValueType = EInputActionValueType::Axis2D;
	}
	else if (ValueTypeStr == TEXT("Axis3D"))
	{
		InputAction->ValueType = EInputActionValueType::Axis3D;
	}
	else
	{
		InputAction->ValueType = EInputActionValueType::Boolean;
	}

	FAssetRegistryModule::AssetCreated(InputAction);
	InputAction->MarkPackageDirty();

	// Save
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, InputAction, *PackageFilename, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), TEXT("IA_") + ActionName);
	ResultObj->SetStringField(TEXT("path"), PackagePath);
	ResultObj->SetStringField(TEXT("value_type"), ValueTypeStr);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
	FString ContextName;
	if (!Params->TryGetStringField(TEXT("name"), ContextName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString Path = TEXT("/Game/Input");
	Params->TryGetStringField(TEXT("path"), Path);

	FString PackagePath = Path / (TEXT("IMC_") + ContextName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	UInputMappingContext* MappingContext = NewObject<UInputMappingContext>(Package, *(TEXT("IMC_") + ContextName), RF_Public | RF_Standalone);
	if (!MappingContext)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create InputMappingContext"));
	}

	// Add mappings if provided
	const TArray<TSharedPtr<FJsonValue>>* MappingsArray;
	if (Params->TryGetArrayField(TEXT("mappings"), MappingsArray))
	{
		for (const TSharedPtr<FJsonValue>& MappingVal : *MappingsArray)
		{
			TSharedPtr<FJsonObject> MappingObj = MappingVal->AsObject();
			if (!MappingObj.IsValid()) continue;

			FString ActionPath;
			FString KeyName;
			if (MappingObj->TryGetStringField(TEXT("action"), ActionPath) &&
				MappingObj->TryGetStringField(TEXT("key"), KeyName))
			{
				UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionPath);
				if (!Action)
				{
					// Try common path patterns
					FString FullPath = Path / ActionPath;
					Action = LoadObject<UInputAction>(nullptr, *FullPath);
				}
				if (Action)
				{
					FKey MappedKey{FName(*KeyName)};
					MappingContext->MapKey(Action, MappedKey);
				}
			}
		}
	}

	FAssetRegistryModule::AssetCreated(MappingContext);
	MappingContext->MarkPackageDirty();

	// Save
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, MappingContext, *PackageFilename, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), TEXT("IMC_") + ContextName);
	ResultObj->SetStringField(TEXT("path"), PackagePath);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleSetProjectSetting(const TSharedPtr<FJsonObject>& Params)
{
	FString Section, Key, Value;
	if (!Params->TryGetStringField(TEXT("section"), Section))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'section' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("key"), Key))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("value"), Value))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	GConfig->SetString(*Section, *Key, *Value, GGameIni);
	GConfig->Flush(false, GGameIni);

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPGameplayCommands::HandleGetProjectSetting(const TSharedPtr<FJsonObject>& Params)
{
	FString Section, Key;
	if (!Params->TryGetStringField(TEXT("section"), Section))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'section' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("key"), Key))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
	}

	FString Value;
	if (!GConfig->GetString(*Section, *Key, Value, GGameIni))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Setting not found: [%s] %s"), *Section, *Key));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("section"), Section);
	ResultObj->SetStringField(TEXT("key"), Key);
	ResultObj->SetStringField(TEXT("value"), Value);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}
