#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class UNREALMCP_API FUnrealMCPGameplayCommands
{
public:
	FUnrealMCPGameplayCommands();

	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	TSharedPtr<FJsonObject> HandleSetDefaultGameMode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetDefaultMap(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCreateEnhancedInputAction(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetProjectSetting(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetProjectSetting(const TSharedPtr<FJsonObject>& Params);
};
