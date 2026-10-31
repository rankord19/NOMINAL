#include "Mission/MissionControlGameMode.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
const TCHAR* PhaseName(EMissionPhase Phase)
{
	switch (Phase)
	{
	case EMissionPhase::Planning: return TEXT("Mission Planning");
	case EMissionPhase::SpacecraftDesign: return TEXT("Spacecraft Design");
	case EMissionPhase::LaunchPreparation: return TEXT("Launch Preparation");
	case EMissionPhase::Launch: return TEXT("Launch");
	case EMissionPhase::FlightOperations: return TEXT("Flight Operations");
	case EMissionPhase::ScienceOperations: return TEXT("Science Operations");
	case EMissionPhase::Completed: return TEXT("Completed — operations ended");
	case EMissionPhase::Failed: return TEXT("Failed");
	default: return TEXT("Invalid phase");
	}
}
}

AMissionControlGameMode::AMissionControlGameMode()
{
	DefaultPawnClass = nullptr;
}

bool AMissionControlGameMode::RequestPhase(EMissionPhase Next)
{
	const EMissionPhase Previous = Lifecycle.GetPhase();
	const bool Accepted = Lifecycle.TryTransitionTo(Next);
	Entries.Add(FString::Printf(TEXT("%s | %s -> %s"),
		Accepted ? TEXT("ACCEPTED") : TEXT("REJECTED"), PhaseName(Previous), PhaseName(Next)));
	if (Entries.Num() > 100) Entries.RemoveAt(0);
	return Accepted;
}

void AMissionControlGameMode::AdvancePhase()
{
	// Presentation route only: every request is validated by the existing lifecycle.
	EMissionPhase Next = Lifecycle.GetPhase();
	switch (Next)
	{
	case EMissionPhase::Planning: Next = EMissionPhase::SpacecraftDesign; break;
	case EMissionPhase::SpacecraftDesign: Next = EMissionPhase::LaunchPreparation; break;
	case EMissionPhase::LaunchPreparation: Next = EMissionPhase::Launch; break;
	case EMissionPhase::Launch: Next = EMissionPhase::FlightOperations; break;
	case EMissionPhase::FlightOperations: Next = EMissionPhase::ScienceOperations; break;
	case EMissionPhase::ScienceOperations: Next = EMissionPhase::Completed; break;
	default: break;
	}
	RequestPhase(Next);
}

FText AMissionControlGameMode::PhaseText() const
{
	return FText::FromString(PhaseName(Lifecycle.GetPhase()));
}

FText AMissionControlGameMode::LogText() const
{
	return FText::FromString(FString::Join(Entries, TEXT("\n")));
}

void AMissionControlGameMode::BeginPlay()
{
	Super::BeginPlay();
	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport || IsRunningDedicatedServer()) return;
	Entries.Add(TEXT("SESSION START | Mission Planning"));
	ControlWidget = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.04f, 1.f))
		.Padding(48)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("NOMINAL"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 36)).ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.9f)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 32)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("MISSION CONTROL  /  Lifecycle prototype"))) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 24)
			[ SNew(STextBlock).Text_UObject(this, &AMissionControlGameMode::PhaseText).Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
				[ SNew(SButton).Text(FText::FromString(TEXT("Advance phase"))).OnClicked_Lambda([this]() { AdvancePhase(); return FReply::Handled(); }) ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SButton).Text(FText::FromString(TEXT("Request failure"))).OnClicked_Lambda([this]() { RequestPhase(EMissionPhase::Failed); return FReply::Handled(); }) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 32)
			[ SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("Manual lifecycle controls. Completion does not grade scientific success. Terminal requests are rejected and logged. Restart Play for a new session."))) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("TRANSITION LOG  /  latest 100 requests"))).ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.9f)) ]
			+ SVerticalBox::Slot().FillHeight(1)
			[ SNew(SScrollBox) + SScrollBox::Slot() [ SNew(STextBlock).Text_UObject(this, &AMissionControlGameMode::LogText).AutoWrapText(true) ] ]
		];
	Viewport->AddViewportWidgetContent(ControlWidget.ToSharedRef(), 10);
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		Controller->bShowMouseCursor = true;
		FInputModeUIOnly Input;
		Input.SetWidgetToFocus(ControlWidget);
		Controller->SetInputMode(Input);
	}
}

void AMissionControlGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (ControlWidget.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
			Viewport->RemoveViewportWidgetContent(ControlWidget.ToSharedRef());
		ControlWidget.Reset();
	}
	Super::EndPlay(Reason);
}
