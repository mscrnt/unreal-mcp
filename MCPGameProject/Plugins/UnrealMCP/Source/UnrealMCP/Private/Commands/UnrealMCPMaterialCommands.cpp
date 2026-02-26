#include "Commands/UnrealMCPMaterialCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "EngineUtils.h"

FUnrealMCPMaterialCommands::FUnrealMCPMaterialCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_material"))
	{
		return HandleCreateMaterial(Params);
	}
	else if (CommandType == TEXT("create_material_instance"))
	{
		return HandleCreateMaterialInstance(Params);
	}
	else if (CommandType == TEXT("set_material_scalar_param"))
	{
		return HandleSetMaterialScalarParam(Params);
	}
	else if (CommandType == TEXT("set_material_vector_param"))
	{
		return HandleSetMaterialVectorParam(Params);
	}
	else if (CommandType == TEXT("set_material_texture_param"))
	{
		return HandleSetMaterialTextureParam(Params);
	}
	else if (CommandType == TEXT("add_material_expression"))
	{
		return HandleAddMaterialExpression(Params);
	}
	else if (CommandType == TEXT("connect_material_expressions"))
	{
		return HandleConnectMaterialExpressions(Params);
	}
	else if (CommandType == TEXT("connect_material_property"))
	{
		return HandleConnectMaterialProperty(Params);
	}
	else if (CommandType == TEXT("apply_material_to_actor"))
	{
		return HandleApplyMaterialToActor(Params);
	}
	else if (CommandType == TEXT("recompile_material"))
	{
		return HandleRecompileMaterial(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString Path = TEXT("/Game/Materials");
	Params->TryGetStringField(TEXT("path"), Path);

	FString PackagePath = Path / MaterialName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	UMaterial* NewMaterial = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone);
	if (!NewMaterial)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
	}

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(NewMaterial);
	NewMaterial->MarkPackageDirty();

	// Post-edit to update
	NewMaterial->PostEditChange();

	// Save the package
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewMaterial, *PackageFilename, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), MaterialName);
	ResultObj->SetStringField(TEXT("path"), PackagePath);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params)
{
	FString InstanceName;
	if (!Params->TryGetStringField(TEXT("name"), InstanceName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString ParentPath;
	if (!Params->TryGetStringField(TEXT("parent_material"), ParentPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_material' parameter"));
	}

	FString Path = TEXT("/Game/Materials");
	Params->TryGetStringField(TEXT("path"), Path);

	// Load parent material
	UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, *ParentPath);
	if (!ParentMaterial)
	{
		// Try with asset path format
		FString AssetPath = ParentPath + TEXT(".") + FPaths::GetBaseFilename(ParentPath);
		ParentMaterial = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
	}
	if (!ParentMaterial)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent material not found: %s"), *ParentPath));
	}

	FString PackagePath = Path / InstanceName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	UMaterialInstanceConstant* NewInstance = NewObject<UMaterialInstanceConstant>(Package, *InstanceName, RF_Public | RF_Standalone);
	if (!NewInstance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material instance"));
	}

	UMaterialEditingLibrary::SetMaterialInstanceParent(NewInstance, ParentMaterial);
	FAssetRegistryModule::AssetCreated(NewInstance);
	NewInstance->MarkPackageDirty();

	// Save
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewInstance, *PackageFilename, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), InstanceName);
	ResultObj->SetStringField(TEXT("path"), PackagePath);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialScalarParam(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	FString ParamName;
	if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
	}

	double Value = 0.0;
	if (!Params->TryGetNumberField(TEXT("value"), Value))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
	if (!Instance)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *AssetPath);
	}
	if (!Instance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *MaterialPath));
	}

	if (!UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(Instance, FName(*ParamName), (float)Value))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to set scalar parameter"));
	}

	UMaterialEditingLibrary::UpdateMaterialInstance(Instance);

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialVectorParam(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	FString ParamName;
	if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
	}

	// Get value as array [R, G, B, A]
	const TArray<TSharedPtr<FJsonValue>>* ValueArray;
	if (!Params->TryGetArrayField(TEXT("value"), ValueArray) || ValueArray->Num() < 3)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'value' parameter (expected [R,G,B] or [R,G,B,A])"));
	}

	FLinearColor Color;
	Color.R = (*ValueArray)[0]->AsNumber();
	Color.G = (*ValueArray)[1]->AsNumber();
	Color.B = (*ValueArray)[2]->AsNumber();
	Color.A = ValueArray->Num() > 3 ? (*ValueArray)[3]->AsNumber() : 1.0f;

	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
	if (!Instance)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *AssetPath);
	}
	if (!Instance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *MaterialPath));
	}

	if (!UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(Instance, FName(*ParamName), Color))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to set vector parameter"));
	}

	UMaterialEditingLibrary::UpdateMaterialInstance(Instance);

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialTextureParam(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	FString ParamName;
	if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
	}

	FString TexturePath;
	if (!Params->TryGetStringField(TEXT("texture_path"), TexturePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'texture_path' parameter"));
	}

	UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
	if (!Texture)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Texture not found: %s"), *TexturePath));
	}

	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
	if (!Instance)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *AssetPath);
	}
	if (!Instance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *MaterialPath));
	}

	if (!UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(Instance, FName(*ParamName), Texture))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to set texture parameter"));
	}

	UMaterialEditingLibrary::UpdateMaterialInstance(Instance);

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	FString ExpressionType;
	if (!Params->TryGetStringField(TEXT("expression_type"), ExpressionType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_type' parameter"));
	}

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	if (!Material)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Material = LoadObject<UMaterial>(nullptr, *AssetPath);
	}
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
	}

	// Map expression type string to class
	UClass* ExpressionClass = nullptr;

	// Common expression types
	static TMap<FString, UClass*> ExpressionMap;
	if (ExpressionMap.Num() == 0)
	{
		ExpressionMap.Add(TEXT("Constant"), UMaterialExpressionConstant::StaticClass());
		ExpressionMap.Add(TEXT("Constant3Vector"), UMaterialExpressionConstant3Vector::StaticClass());
		ExpressionMap.Add(TEXT("Constant4Vector"), UMaterialExpressionConstant4Vector::StaticClass());
		ExpressionMap.Add(TEXT("TextureSample"), UMaterialExpressionTextureSample::StaticClass());
		ExpressionMap.Add(TEXT("ScalarParameter"), UMaterialExpressionScalarParameter::StaticClass());
		ExpressionMap.Add(TEXT("VectorParameter"), UMaterialExpressionVectorParameter::StaticClass());
		ExpressionMap.Add(TEXT("Multiply"), UMaterialExpressionMultiply::StaticClass());
		ExpressionMap.Add(TEXT("Add"), UMaterialExpressionAdd::StaticClass());
		ExpressionMap.Add(TEXT("Lerp"), UMaterialExpressionLinearInterpolate::StaticClass());
	}

	UClass** FoundClass = ExpressionMap.Find(ExpressionType);
	if (FoundClass)
	{
		ExpressionClass = *FoundClass;
	}
	else
	{
		// Try to find expression class by name
		FString ClassName = TEXT("MaterialExpression") + ExpressionType;
		ExpressionClass = FindFirstObject<UClass>(*ClassName);
		if (!ExpressionClass)
		{
			ExpressionClass = FindFirstObject<UClass>(*(TEXT("U") + ClassName));
		}
	}

	if (!ExpressionClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown expression type: %s"), *ExpressionType));
	}

	int32 PosX = 0, PosY = 0;
	if (Params->HasField(TEXT("position")))
	{
		FVector2D Pos = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
		PosX = (int32)Pos.X;
		PosY = (int32)Pos.Y;
	}

	UMaterialExpression* NewExpression = UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, PosX, PosY);
	if (!NewExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material expression"));
	}

	// Set parameter name if applicable
	FString ParamName;
	if (Params->TryGetStringField(TEXT("param_name"), ParamName))
	{
		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->ParameterName = FName(*ParamName);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->ParameterName = FName(*ParamName);
		}
	}

	// Set default value for constants
	double DefaultValue;
	if (Params->TryGetNumberField(TEXT("value"), DefaultValue))
	{
		if (UMaterialExpressionConstant* ConstExpr = Cast<UMaterialExpressionConstant>(NewExpression))
		{
			ConstExpr->R = (float)DefaultValue;
		}
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->DefaultValue = (float)DefaultValue;
		}
	}

	// Set color for vector constants
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		FLinearColor Color((*ColorArray)[0]->AsNumber(), (*ColorArray)[1]->AsNumber(), (*ColorArray)[2]->AsNumber(),
			ColorArray->Num() > 3 ? (*ColorArray)[3]->AsNumber() : 1.0f);
		if (UMaterialExpressionConstant3Vector* Vec3Expr = Cast<UMaterialExpressionConstant3Vector>(NewExpression))
		{
			Vec3Expr->Constant = Color;
		}
		else if (UMaterialExpressionConstant4Vector* Vec4Expr = Cast<UMaterialExpressionConstant4Vector>(NewExpression))
		{
			Vec4Expr->Constant = Color;
		}
		else if (UMaterialExpressionVectorParameter* VecParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VecParam->DefaultValue = Color;
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("expression_name"), NewExpression->GetName());
	ResultObj->SetNumberField(TEXT("expression_index"), Material->GetExpressions().Num() - 1);
	return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	int32 FromIndex, ToIndex;
	if (!Params->TryGetNumberField(TEXT("from_expression_index"), FromIndex))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_expression_index' parameter"));
	}
	if (!Params->TryGetNumberField(TEXT("to_expression_index"), ToIndex))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_expression_index' parameter"));
	}

	FString FromOutput, ToInput;
	Params->TryGetStringField(TEXT("from_output"), FromOutput);
	Params->TryGetStringField(TEXT("to_input"), ToInput);

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	if (!Material)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Material = LoadObject<UMaterial>(nullptr, *AssetPath);
	}
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
	}

	auto Expressions = Material->GetExpressions();
	if (FromIndex < 0 || FromIndex >= Expressions.Num() || ToIndex < 0 || ToIndex >= Expressions.Num())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Expression index out of range"));
	}

	if (!UMaterialEditingLibrary::ConnectMaterialExpressions(Expressions[FromIndex], FromOutput, Expressions[ToIndex], ToInput))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect expressions"));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleConnectMaterialProperty(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	int32 FromIndex;
	if (!Params->TryGetNumberField(TEXT("from_expression_index"), FromIndex))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_expression_index' parameter"));
	}

	FString PropertyStr;
	if (!Params->TryGetStringField(TEXT("property"), PropertyStr))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property' parameter"));
	}

	FString FromOutput;
	Params->TryGetStringField(TEXT("from_output"), FromOutput);

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	if (!Material)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Material = LoadObject<UMaterial>(nullptr, *AssetPath);
	}
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
	}

	auto Expressions = Material->GetExpressions();
	if (FromIndex < 0 || FromIndex >= Expressions.Num())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Expression index out of range"));
	}

	// Map property string to enum
	EMaterialProperty MaterialProperty = MP_BaseColor;
	if (PropertyStr == TEXT("BaseColor")) MaterialProperty = MP_BaseColor;
	else if (PropertyStr == TEXT("Metallic")) MaterialProperty = MP_Metallic;
	else if (PropertyStr == TEXT("Specular")) MaterialProperty = MP_Specular;
	else if (PropertyStr == TEXT("Roughness")) MaterialProperty = MP_Roughness;
	else if (PropertyStr == TEXT("EmissiveColor")) MaterialProperty = MP_EmissiveColor;
	else if (PropertyStr == TEXT("Opacity")) MaterialProperty = MP_Opacity;
	else if (PropertyStr == TEXT("OpacityMask")) MaterialProperty = MP_OpacityMask;
	else if (PropertyStr == TEXT("Normal")) MaterialProperty = MP_Normal;
	else if (PropertyStr == TEXT("AmbientOcclusion")) MaterialProperty = MP_AmbientOcclusion;
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material property: %s"), *PropertyStr));
	}

	if (!UMaterialEditingLibrary::ConnectMaterialProperty(Expressions[FromIndex], FromOutput, MaterialProperty))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect to material property"));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
	}

	int32 SlotIndex = 0;
	Params->TryGetNumberField(TEXT("slot_index"), SlotIndex);

	// Find the actor
	UWorld* World = GEditor->GetEditorWorldContext().World();
	AActor* Actor = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetActorLabel() == ActorName || It->GetName() == ActorName)
		{
			Actor = *It;
			break;
		}
	}
	if (!Actor)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	// Load material
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
	if (!Material)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Material = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
	}
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
	}

	// Apply to static mesh component
	UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>();
	if (MeshComp)
	{
		MeshComp->SetMaterial(SlotIndex, Material);
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("actor"), ActorName);
		ResultObj->SetStringField(TEXT("material"), MaterialPath);
		ResultObj->SetNumberField(TEXT("slot"), SlotIndex);
		return FUnrealMCPCommonUtils::CreateSuccessResponse(ResultObj);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no StaticMeshComponent"));
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleRecompileMaterial(const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	if (!Material)
	{
		FString AssetPath = MaterialPath + TEXT(".") + FPaths::GetBaseFilename(MaterialPath);
		Material = LoadObject<UMaterial>(nullptr, *AssetPath);
	}
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
	}

	UMaterialEditingLibrary::RecompileMaterial(Material);

	return FUnrealMCPCommonUtils::CreateSuccessResponse();
}
