#include "Spacecraft/SpacecraftPresentation.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

bool FSpacecraftPresentation::Initialize(UWorld* World)
{
	Parts.Reset();
	const TArray<FName> Roles = {TEXT("Designer.Panel"), TEXT("Designer.Dish"), TEXT("Designer.Feed"), TEXT("Designer.Nozzle"), TEXT("Designer.Mount")};
	TSet<FName> Found;
	for (TActorIterator<AActor> It(World); It; ++It)
		for (FName Role : Roles)
			if (It->ActorHasTag(Role))
			{
				Parts.Add({*It, Role, It->GetActorTransform()});
				Found.Add(Role);
			}
	return Found.Num() == Roles.Num();
}

void FSpacecraftPresentation::Apply(const FSpacecraftConfiguration& Configuration)
{
	// These profiles only rearrange temporary geometry; they do not derive hardware from specs.
	const bool CompactPower = Configuration.Installed(TEXT("power"))->Id == TEXT("power.wing2");
	const bool CompactDish = Configuration.Installed(TEXT("communications"))->Id == TEXT("comms.dish06");
	const bool CompactEngine = Configuration.Installed(TEXT("propulsion"))->Id == TEXT("propulsion.compact");
	for (const FPart& Part : Parts)
	{
		AActor* Actor = Part.Actor.Get();
		if (!Actor) continue;
		FTransform Transform = Part.Original;
		bool Hidden = false;
		if (Part.Role == TEXT("Designer.Panel"))
			Hidden = CompactPower && FMath::Abs(Transform.GetLocation().X) > 300;
		else if (CompactDish && Part.Role == TEXT("Designer.Dish"))
			Transform.SetScale3D(Transform.GetScale3D() * 0.5);
		else if (CompactDish && Part.Role == TEXT("Designer.Feed"))
		{
			Transform.SetScale3D(Transform.GetScale3D() * 0.5);
			Transform.SetLocation(FVector(0, 0, 225));
		}
		else if (CompactEngine && Part.Role == TEXT("Designer.Nozzle"))
		{
			// Retain the mounting depth so the narrow nozzle remains inspectable below the bus.
			Transform.SetScale3D(Transform.GetScale3D() * FVector(0.5, 0.5, 1));
		}
		else if (CompactEngine && Part.Role == TEXT("Designer.Mount"))
			Transform.SetScale3D(Transform.GetScale3D() * FVector(0.5, 0.5, 1));
		Actor->SetActorTransform(Transform);
		Actor->SetActorHiddenInGame(Hidden);
	}
}
