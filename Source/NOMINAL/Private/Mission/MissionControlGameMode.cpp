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
#include "Mission/LunarCameraPawn.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SCompoundWidget.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Spacecraft/SSpacecraftDesigner.h"
#include "Widgets/Layout/SBox.h"

namespace
{
/** Pointer input belongs to this canvas, never to Mission Control or toolbar buttons. */
class SOrbitalCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOrbitalCanvas) {} SLATE_ARGUMENT(ALunarCameraPawn*, CameraPawn) SLATE_END_ARGS()
	void Construct(const FArguments& Args) { Pawn = Args._CameraPawn; }
	void SetPanDrag(bool bPan) { bPanDrag = bPan; }
	virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton || Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton)
			return FReply::Handled().CaptureMouse(SharedThis(this));
		return FReply::Unhandled();
	}
	virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
	{
		if (HasMouseCapture() && (Event.GetEffectingButton() == EKeys::LeftMouseButton || Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton))
			return FReply::Handled().ReleaseMouseCapture();
		return FReply::Unhandled();
	}
	virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override
	{
		if (HasMouseCapture() && Pawn.IsValid())
		{
			const FVector2D Delta = Event.GetCursorDelta();
			if (Event.IsMouseButtonDown(EKeys::RightMouseButton)) Pawn->Orbit(Delta.X, Delta.Y);
			if (Event.IsMouseButtonDown(EKeys::MiddleMouseButton)) Pawn->Pan(Delta.X, Delta.Y);
			if (Event.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				if (bPanDrag) Pawn->Pan(Delta.X, Delta.Y);
				else Pawn->Orbit(Delta.X, Delta.Y);
			}
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
	virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override
	{
		if (Pawn.IsValid()) Pawn->Zoom(Event.GetWheelDelta());
		return FReply::Handled();
	}
private:
	TWeakObjectPtr<ALunarCameraPawn> Pawn;
	bool bPanDrag = false;
};

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
	DefaultPawnClass = ALunarCameraPawn::StaticClass();
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
	if (!Presentation.Initialize(GetWorld()))
		UE_LOG(LogTemp, Warning, TEXT("Spacecraft designer: missing tagged visual parts; use L_LunarOrbit."));
	Presentation.Apply(Spacecraft);
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
				+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0)
				[ SNew(SButton).Text(FText::FromString(TEXT("Enter lunar view"))).OnClicked_Lambda([this]() { ShowOrbitalView(true); return FReply::Handled(); }) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0)
				[ SNew(SButton).Text(FText::FromString(TEXT("Spacecraft Designer"))).OnClicked_Lambda([this]() { ShowDesigner(); return FReply::Handled(); }) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 32)
			[ SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("Manual lifecycle controls. Completion does not grade scientific success. Terminal requests are rejected and logged. Restart Play for a new session."))) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("TRANSITION LOG  /  latest 100 requests"))).ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.9f)) ]
			+ SVerticalBox::Slot().FillHeight(1)
			[ SNew(SScrollBox) + SScrollBox::Slot() [ SNew(STextBlock).Text_UObject(this, &AMissionControlGameMode::LogText).AutoWrapText(true) ] ]
		];
	Viewport->AddViewportWidgetContent(ControlWidget.ToSharedRef(), 10);
	ALunarCameraPawn* CameraPawn = Cast<ALunarCameraPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());
	TSharedRef<SOrbitalCanvas> Canvas = SNew(SOrbitalCanvas).CameraPawn(CameraPawn);
	DesignerWidget = SNew(SBox).WidthOverride(380)
		[ SNew(SSpacecraftDesigner).Configuration(&Spacecraft)
			.Install([this](FName Slot, FName Module) { return InstallModule(Slot, Module); }) ];
	DesignerWidget->SetVisibility(EVisibility::Collapsed);
	OrbitalWidget = SNew(SOverlay)
		+ SOverlay::Slot() [ Canvas ]
		+ SOverlay::Slot().HAlign(HAlign_Left).Padding(16, 112, 0, 16) [ DesignerWidget.ToSharedRef() ]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBorder).Padding(16).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.03f, 1.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,16,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Mission Control"))).OnClicked_Lambda([this]() { ShowOrbitalView(false); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,16,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Spacecraft focus"))).OnClicked_Lambda([Weak = TWeakObjectPtr<ALunarCameraPawn>(CameraPawn)]() { if (Weak.IsValid()) Weak->FocusMoon(false); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,16,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Moon focus"))).OnClicked_Lambda([Weak = TWeakObjectPtr<ALunarCameraPawn>(CameraPawn)]() { if (Weak.IsValid()) Weak->FocusMoon(true); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Orbit drag"))).OnClicked_Lambda([Canvas]() { Canvas->SetPanDrag(false); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Pan drag"))).OnClicked_Lambda([Canvas]() { Canvas->SetPanDrag(true); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,16,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Capture PNG"))).OnClicked_Lambda([]() { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/LunarView.png"), true, true); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth() [ SNew(STextBlock).Text_UObject(this, &AMissionControlGameMode::PhaseText) ]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("NOMINAL  |  LMB: selected drag mode   RMB: orbit   MMB: pan   Wheel: zoom  |  Static visual study; temporary spacecraft."))) ]
			]
		];
	OrbitalWidget->SetVisibility(EVisibility::Collapsed);
	Viewport->AddViewportWidgetContent(OrbitalWidget.ToSharedRef(), 11);
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		Controller->bShowMouseCursor = true;
		FInputModeUIOnly Input;
		Controller->SetInputMode(Input);
	}
}

void AMissionControlGameMode::ShowOrbitalView(bool bShow)
{
	if (ControlWidget) ControlWidget->SetVisibility(bShow ? EVisibility::Collapsed : EVisibility::Visible);
	if (OrbitalWidget) OrbitalWidget->SetVisibility(bShow ? EVisibility::Visible : EVisibility::Collapsed);
	if (DesignerWidget) DesignerWidget->SetVisibility(EVisibility::Collapsed);
}

void AMissionControlGameMode::ShowDesigner()
{
	ShowOrbitalView(true);
	if (DesignerWidget) DesignerWidget->SetVisibility(EVisibility::Visible);
	if (auto* Controller = GetWorld()->GetFirstPlayerController())
		if (auto* Camera = Cast<ALunarCameraPawn>(Controller->GetPawn())) Camera->FocusMoon(false);
}

FString AMissionControlGameMode::InstallModule(FName SlotId, FName ModuleId)
{
	FString Reason;
	if (Spacecraft.TryInstall(SlotId, ModuleId, Reason)) Presentation.Apply(Spacecraft);
	else Reason = TEXT("REJECTED: ") + Reason;
	return Reason;
}

void AMissionControlGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (ControlWidget.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
			Viewport->RemoveViewportWidgetContent(ControlWidget.ToSharedRef());
		ControlWidget.Reset();
	}
	if (OrbitalWidget.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
			Viewport->RemoveViewportWidgetContent(OrbitalWidget.ToSharedRef());
		OrbitalWidget.Reset();
	}
	Super::EndPlay(Reason);
	DesignerWidget.Reset();
}
