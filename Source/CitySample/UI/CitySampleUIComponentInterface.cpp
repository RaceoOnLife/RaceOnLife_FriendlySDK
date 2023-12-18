// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CitySampleUIComponentInterface.h"


UCitySamplePanel* ICitySampleUIComponentInterface::AddOverlay_Implementation(UCitySampleUIComponent* CitySampleUI, const bool bSkipAnimation/*=false*/)
{
	return nullptr;
}

void ICitySampleUIComponentInterface::RemoveOverlay_Implementation(UCitySampleUIComponent* CitySampleUI, const bool bSkipAnimation/*=false*/)
{
}
