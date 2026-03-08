#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SWindow;

class FUnrealMCPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FUnrealMCPModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FUnrealMCPModule>("UnrealMCP");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UnrealMCP");
	}

private:
	// Toolbar
	void ExtendLevelEditorToolbar();

	// Control panel
	void OpenControlPanel();
	FReply OnOpenControlPanelClicked();
	void CloseControlPanel();
	void OnControlPanelClosed(const TSharedRef<SWindow>& Window);
	TSharedRef<class SWidget> CreateControlPanelContent();

	// Server helpers
	bool IsServerRunning() const;
	FReply OnStartServerClicked();
	FReply OnStopServerClicked();

	TSharedPtr<SWindow> ControlPanelWindow;
};
