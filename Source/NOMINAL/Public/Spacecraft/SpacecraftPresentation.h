#pragma once
#include "CoreMinimal.h"
#include "Spacecraft/SpacecraftConfiguration.h"
class AActor;
class UWorld;

/** Visual-only adapter for the tagged, existing lunar spacecraft parts. */
class NOMINAL_API FSpacecraftPresentation
{
public:
	bool Initialize(UWorld* World);
	void Apply(const FSpacecraftConfiguration& Configuration);
private:
	struct FPart { TWeakObjectPtr<AActor> Actor; FName Role; FTransform Original; };
	TArray<FPart> Parts;
};
