#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MCPSettings.generated.h"

/**
 * Settings for the UnrealMCP plugin.
 * Accessible via Editor > Preferences > Plugins > MCP Settings
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "MCP Settings"))
class UNREALMCP_API UMCPSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** TCP port the MCP bridge listens on */
	UPROPERTY(config, EditAnywhere, Category = "MCP|Network",
		meta = (ClampMin = "1024", ClampMax = "65535",
			ToolTip = "TCP port for the MCP bridge server. Restart required after changing."))
	int32 Port = 55557;

	/** Bind address for the TCP server */
	UPROPERTY(config, EditAnywhere, Category = "MCP|Network",
		meta = (ToolTip = "Bind address. Use 0.0.0.0 for all interfaces, 127.0.0.1 for local only."))
	FString BindAddress = TEXT("0.0.0.0");

	/** Whether to auto-start the MCP server when the editor opens */
	UPROPERTY(config, EditAnywhere, Category = "MCP|General",
		meta = (ToolTip = "Automatically start the MCP server when the editor opens."))
	bool bAutoStart = true;

	// UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("MCP Settings"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("MCPSettings", "MCPSettingsSection", "MCP Settings");
	}

	virtual FText GetSectionDescription() const override
	{
		return NSLOCTEXT("MCPSettings", "MCPSettingsDesc",
			"Configure the Model Context Protocol (MCP) bridge server.");
	}
#endif
};
