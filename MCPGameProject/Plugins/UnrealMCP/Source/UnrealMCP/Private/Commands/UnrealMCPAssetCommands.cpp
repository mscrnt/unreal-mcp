#include "Commands/UnrealMCPAssetCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetImportTask.h"
#include "Editor.h"

FUnrealMCPAssetCommands::FUnrealMCPAssetCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("list_assets"))
	{
		return HandleListAssets(Params);
	}
	else if (CommandType == TEXT("find_asset"))
	{
		return HandleFindAsset(Params);
	}
	else if (CommandType == TEXT("does_asset_exist"))
	{
		return HandleDoesAssetExist(Params);
	}
	else if (CommandType == TEXT("duplicate_asset"))
	{
		return HandleDuplicateAsset(Params);
	}
	else if (CommandType == TEXT("delete_asset"))
	{
		return HandleDeleteAsset(Params);
	}
	else if (CommandType == TEXT("rename_asset"))
	{
		return HandleRenameAsset(Params);
	}
	else if (CommandType == TEXT("create_folder"))
	{
		return HandleCreateFolder(Params);
	}
	else if (CommandType == TEXT("import_asset"))
	{
		return HandleImportAsset(Params);
	}
	else if (CommandType == TEXT("save_asset"))
	{
		return HandleSaveAsset(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown asset command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleListAssets(const TSharedPtr<FJsonObject>& Params)
{
	FString Path = TEXT("/Game");
	Params->TryGetStringField(TEXT("path"), Path);

	bool bRecursive = true;
	Params->TryGetBoolField(TEXT("recursive"), bRecursive);

	// Ensure path is scanned in the asset registry (catches newly-added content packs)
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.ScanPathsSynchronous({Path}, true);

	TArray<FString> AssetPaths;
	if (UEditorAssetLibrary::DoesDirectoryExist(Path))
	{
		AssetPaths = UEditorAssetLibrary::ListAssets(Path, bRecursive);
	}

	// Optionally filter by class
	FString ClassFilter;
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);

	TArray<TSharedPtr<FJsonValue>> AssetArray;

	for (const FString& AssetPath : AssetPaths)
	{
		if (!ClassFilter.IsEmpty())
		{
			FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
			if (AssetData.IsValid() && !AssetData.AssetClassPath.GetAssetName().ToString().Contains(ClassFilter))
			{
				continue;
			}
		}

		TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
		AssetObj->SetStringField(TEXT("path"), AssetPath);
		AssetObj->SetStringField(TEXT("name"), FPaths::GetBaseFilename(AssetPath));
		AssetArray.Add(MakeShared<FJsonValueObject>(AssetObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetArrayField(TEXT("assets"), AssetArray);
	ResultObj->SetNumberField(TEXT("count"), AssetArray.Num());
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleFindAsset(const TSharedPtr<FJsonObject>& Params)
{
	FString Name;
	if (!Params->TryGetStringField(TEXT("name"), Name))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// Scan /Game to ensure newly-added content packs are discoverable
	AssetRegistry.ScanPathsSynchronous({TEXT("/Game")}, true);

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TArray<TSharedPtr<FJsonValue>> MatchArray;
	for (const FAssetData& AssetData : Assets)
	{
		if (AssetData.AssetName.ToString().Contains(Name))
		{
			TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
			AssetObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
			AssetObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
			AssetObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
			MatchArray.Add(MakeShared<FJsonValueObject>(AssetObj));
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetArrayField(TEXT("assets"), MatchArray);
	ResultObj->SetNumberField(TEXT("count"), MatchArray.Num());
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleDoesAssetExist(const TSharedPtr<FJsonObject>& Params)
{
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
	}

	// Scan the parent directory to catch newly-added content
	FString ParentPath = FPaths::GetPath(Path);
	if (!ParentPath.IsEmpty())
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.ScanPathsSynchronous({ParentPath}, true);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("exists"), UEditorAssetLibrary::DoesAssetExist(Path));
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleDuplicateAsset(const TSharedPtr<FJsonObject>& Params)
{
	FString SourcePath, DestPath;
	if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("dest_path"), DestPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dest_path' parameter"));
	}

	UObject* DuplicatedAsset = UEditorAssetLibrary::DuplicateAsset(SourcePath, DestPath);
	if (!DuplicatedAsset)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to duplicate asset"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("path"), DestPath);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params)
{
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
	}

	if (!UEditorAssetLibrary::DeleteAsset(Path))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to delete asset: %s"), *Path));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleRenameAsset(const TSharedPtr<FJsonObject>& Params)
{
	FString SourcePath, DestPath;
	if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("dest_path"), DestPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dest_path' parameter"));
	}

	// Check if source exists
	if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source asset not found: %s"), *SourcePath));
	}

	// Check if destination already exists
	if (UEditorAssetLibrary::DoesAssetExist(DestPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Destination already exists: %s"), *DestPath));
	}

	if (!UEditorAssetLibrary::RenameAsset(SourcePath, DestPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to rename '%s' to '%s'. The asset may be loaded/referenced. Try saving all assets first, or close any editors using it."),
				*SourcePath, *DestPath));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("new_path"), DestPath);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleCreateFolder(const TSharedPtr<FJsonObject>& Params)
{
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
	}

	if (!UEditorAssetLibrary::MakeDirectory(Path))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create folder: %s"), *Path));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("path"), Path);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleImportAsset(const TSharedPtr<FJsonObject>& Params)
{
	FString SourceFile;
	if (!Params->TryGetStringField(TEXT("source_file"), SourceFile))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_file' parameter"));
	}

	FString DestPath = TEXT("/Game");
	Params->TryGetStringField(TEXT("dest_path"), DestPath);

	FString DestName;
	Params->TryGetStringField(TEXT("dest_name"), DestName);

	// Create import task
	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	ImportTask->Filename = SourceFile;
	ImportTask->DestinationPath = DestPath;
	if (!DestName.IsEmpty())
	{
		ImportTask->DestinationName = DestName;
	}
	ImportTask->bReplaceExisting = true;
	ImportTask->bAutomated = true;
	ImportTask->bSave = true;

	// Execute import
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.ImportAssetTasks({ImportTask});

	const TArray<UObject*>& ImportedObjects = ImportTask->GetObjects();
	if (ImportedObjects.Num() == 0)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Import completed but no objects were created"));
	}

	TArray<TSharedPtr<FJsonValue>> ImportedArray;
	for (UObject* Obj : ImportedObjects)
	{
		TSharedPtr<FJsonObject> ObjInfo = MakeShared<FJsonObject>();
		ObjInfo->SetStringField(TEXT("name"), Obj->GetName());
		ObjInfo->SetStringField(TEXT("path"), Obj->GetPathName());
		ObjInfo->SetStringField(TEXT("class"), Obj->GetClass()->GetName());
		ImportedArray.Add(MakeShared<FJsonValueObject>(ObjInfo));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetArrayField(TEXT("imported"), ImportedArray);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleSaveAsset(const TSharedPtr<FJsonObject>& Params)
{
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
	}

	// First, try to load the asset to ensure the package is in memory and properly marked
	UObject* Asset = UEditorAssetLibrary::LoadAsset(Path);
	if (Asset)
	{
		// Ensure the package is marked dirty before saving
		UPackage* Package = Asset->GetOutermost();
		if (Package)
		{
			Package->MarkPackageDirty();
		}

		// For Blueprints, also ensure the generated class CDO is included in the save
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
		if (Blueprint)
		{
			// Mark the Blueprint package itself as needing save
			Blueprint->MarkPackageDirty();

			UE_LOG(LogTemp, Log, TEXT("SaveAsset - Saving Blueprint: %s (GeneratedClass: %s)"),
				*Path, Blueprint->GeneratedClass ? *Blueprint->GeneratedClass->GetName() : TEXT("NULL"));
		}
	}

	if (!UEditorAssetLibrary::SaveAsset(Path, /*bOnlyIfIsDirty=*/ false))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to save asset: %s"), *Path));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("path"), Path);
	ResultObj->SetBoolField(TEXT("saved"), true);
	if (Asset)
	{
		ResultObj->SetStringField(TEXT("asset_class"), Asset->GetClass()->GetName());
	}
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}
