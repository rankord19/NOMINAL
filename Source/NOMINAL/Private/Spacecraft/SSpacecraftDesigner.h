#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Spacecraft/SpacecraftConfiguration.h"

class SVerticalBox;
/** Catalog-driven editor panel. Installed state is owned by the session, not the widget. */
class SSpacecraftDesigner : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSpacecraftDesigner) {}
		SLATE_ARGUMENT(const FSpacecraftConfiguration*, Configuration)
		SLATE_ARGUMENT(TFunction<FString(FName, FName)>, Install)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
private:
	void RefreshOptions();
	FText Summary() const;
	const FSpacecraftConfiguration* Configuration = nullptr;
	TFunction<FString(FName, FName)> Install;
	FName SelectedSlot;
	FString Status = TEXT("Select a category, then install a module.");
	TSharedPtr<SVerticalBox> Options;
};
