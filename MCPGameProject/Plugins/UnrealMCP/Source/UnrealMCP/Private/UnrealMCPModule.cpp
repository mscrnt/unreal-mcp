#include "UnrealMCPModule.h"
#include "UnrealMCPBridge.h"
#include "MCPSettings.h"
#include "Modules/ModuleManager.h"
#include "EditorSubsystem.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"
#include "ISettingsModule.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

void FUnrealMCPModule::StartupModule()
{
	UE_LOG(LogTemp, Display, TEXT("UnrealMCP: Module starting up"));

	// Register settings in Editor Preferences
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "Plugins", "MCP Settings",
			LOCTEXT("MCPSettingsName", "MCP Settings"),
			LOCTEXT("MCPSettingsDescription", "Configure the MCP bridge server (port, bind address, Docker support)"),
			GetMutableDefault<UMCPSettings>()
		);
	}

	// Register toolbar button after engine is ready
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FUnrealMCPModule::ExtendLevelEditorToolbar);
}

void FUnrealMCPModule::ShutdownModule()
{
	// Unregister settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "MCP Settings");
	}

	CloseControlPanel();
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	UE_LOG(LogTemp, Display, TEXT("UnrealMCP: Module shut down"));
}

// --- Toolbar ---

void FUnrealMCPModule::ExtendLevelEditorToolbar()
{
	static bool bAlreadyExtended = false;
	if (bAlreadyExtended) return;

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	if (ToolbarMenu)
	{
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("MCP");

		Section.AddEntry(FToolMenuEntry::InitWidget(
			"MCPServerControl",
			SNew(SButton)
			.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
			.OnClicked(FOnClicked::CreateRaw(this, &FUnrealMCPModule::OnOpenControlPanelClicked))
			.ToolTipText(LOCTEXT("MCPButtonTooltip", "MCP Server Control Panel"))
			.ContentPadding(FMargin(2))
			.Content()
			[
				SNew(SHorizontalBox)

				// Status dot
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(SImage)
					.DesiredSizeOverride(FVector2D(8, 8))
					.ColorAndOpacity_Lambda([this]() -> FSlateColor {
						return IsServerRunning()
							? FSlateColor(FLinearColor(0.0f, 0.85f, 0.0f))
							: FSlateColor(FLinearColor(0.85f, 0.0f, 0.0f));
					})
					.Image(FAppStyle::GetBrush("WhiteBrush"))
				]

				// Label
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MCPToolbarLabel", "MCP"))
					.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
				]
			],
			FText::GetEmpty(),
			true, false, false
		));

		UE_LOG(LogTemp, Display, TEXT("UnrealMCP: Toolbar button added"));
	}

	// Also add to Window menu
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (WindowMenu)
	{
		FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");
		Section.AddMenuEntry(
			"MCPControlPanel",
			LOCTEXT("MCPWindowMenuLabel", "MCP Server Control Panel"),
			LOCTEXT("MCPWindowMenuTooltip", "Open MCP Server Control Panel"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnrealMCPModule::OpenControlPanel),
				FCanExecuteAction()
			)
		);
	}

	bAlreadyExtended = true;
}

// --- Control Panel ---

FReply FUnrealMCPModule::OnOpenControlPanelClicked()
{
	OpenControlPanel();
	return FReply::Handled();
}

void FUnrealMCPModule::OpenControlPanel()
{
	if (ControlPanelWindow.IsValid())
	{
		ControlPanelWindow->BringToFront();
		return;
	}

	ControlPanelWindow = SNew(SWindow)
		.Title(LOCTEXT("MCPControlPanelTitle", "MCP Server Control Panel"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.HasCloseButton(true)
		.CreateTitleBar(true)
		.IsTopmostWindow(true)
		.MinWidth(320)
		.MinHeight(180);

	ControlPanelWindow->SetContent(CreateControlPanelContent());
	ControlPanelWindow->GetOnWindowClosedEvent().AddRaw(this, &FUnrealMCPModule::OnControlPanelClosed);

	FSlateApplication::Get().AddWindow(ControlPanelWindow.ToSharedRef());
}

void FUnrealMCPModule::OnControlPanelClosed(const TSharedRef<SWindow>& Window)
{
	ControlPanelWindow.Reset();
}

void FUnrealMCPModule::CloseControlPanel()
{
	if (ControlPanelWindow.IsValid())
	{
		ControlPanelWindow->RequestDestroyWindow();
		ControlPanelWindow.Reset();
	}
}

TSharedRef<SWidget> FUnrealMCPModule::CreateControlPanelContent()
{
	const UMCPSettings* Settings = GetDefault<UMCPSettings>();

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)

			// --- Status ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 6)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 8, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("StatusLabel", "Status:"))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() -> FText {
						return IsServerRunning()
							? LOCTEXT("StatusRunning", "Running")
							: LOCTEXT("StatusStopped", "Stopped");
					})
					.ColorAndOpacity_Lambda([this]() -> FSlateColor {
						return IsServerRunning()
							? FSlateColor(FLinearColor(0.0f, 0.85f, 0.0f))
							: FSlateColor(FLinearColor(0.85f, 0.0f, 0.0f));
					})
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
			]

			// --- Port ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 8, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PortLabel", "Port:"))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::FromInt(Settings->Port)))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
			]

			// --- Bind Address ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 8, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BindLabel", "Bind:"))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Settings->BindAddress))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
			]

			// --- Docker hint ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 12)
			[
				SNew(STextBlock)
				.Text_Lambda([Settings]() -> FText {
					if (Settings->BindAddress == TEXT("0.0.0.0"))
					{
						return LOCTEXT("DockerReady", "Docker/remote: enabled (bound to all interfaces)");
					}
					return LOCTEXT("DockerOff", "Docker/remote: disabled (local only)");
				})
				.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
				.Font(FAppStyle::GetFontStyle("SmallText"))
			]

			// --- Start / Stop buttons ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			.HAlign(HAlign_Center)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(5.0f))
				.MinDesiredSlotWidth(110.0f)

				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("StartBtn", "Start Server"))
					.IsEnabled_Lambda([this]() -> bool { return !IsServerRunning(); })
					.OnClicked(FOnClicked::CreateRaw(this, &FUnrealMCPModule::OnStartServerClicked))
				]

				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("StopBtn", "Stop Server"))
					.IsEnabled_Lambda([this]() -> bool { return IsServerRunning(); })
					.OnClicked(FOnClicked::CreateRaw(this, &FUnrealMCPModule::OnStopServerClicked))
				]
			]

			// --- Open Settings ---
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("OpenSettingsBtn", "Open Settings"))
				.OnClicked_Lambda([]() -> FReply {
					if (ISettingsModule* SM = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
					{
						SM->ShowViewer("Editor", "Plugins", "MCP Settings");
					}
					return FReply::Handled();
				})
			]
		];
}

// --- Server Helpers ---

bool FUnrealMCPModule::IsServerRunning() const
{
	if (GEditor)
	{
		UUnrealMCPBridge* Bridge = GEditor->GetEditorSubsystem<UUnrealMCPBridge>();
		if (Bridge)
		{
			return Bridge->IsRunning();
		}
	}
	return false;
}

FReply FUnrealMCPModule::OnStartServerClicked()
{
	if (GEditor)
	{
		UUnrealMCPBridge* Bridge = GEditor->GetEditorSubsystem<UUnrealMCPBridge>();
		if (Bridge)
		{
			Bridge->StartServer();
		}
	}

	// Refresh toolbar status dot
	if (UToolMenus* TM = UToolMenus::Get())
	{
		TM->RefreshAllWidgets();
	}

	return FReply::Handled();
}

FReply FUnrealMCPModule::OnStopServerClicked()
{
	if (GEditor)
	{
		UUnrealMCPBridge* Bridge = GEditor->GetEditorSubsystem<UUnrealMCPBridge>();
		if (Bridge)
		{
			Bridge->StopServer();
		}
	}

	if (UToolMenus* TM = UToolMenus::Get())
	{
		TM->RefreshAllWidgets();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
