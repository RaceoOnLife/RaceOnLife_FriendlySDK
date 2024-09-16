// Copyright (c) 2022-2023 Justin Nordin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

// FFastPropertyInstanceInfo provides debugging information for an FProperty. It is similar to the
// FPropertyInstanceInfo class in KismetDebugUtilities.h but it explicitly stops property recursion
// if it finds an object property. FPropertyInstanceInfo, on the other hand, will recurse through 
// child object properties. This leads to very large property trees and 200ms+ delays for 
// determining what Blueprint properties to show in the Actor details.
//
// Even so, FFastPropertyInstanceInfo uses the FPropertyInstanceInfo class and helper functions 
// wherever possible to avoid duplicating work.
class FFastPropertyInstanceInfo
{
public:
	FFastPropertyInstanceInfo(void* Container, const FProperty* Property);

#if ENGINE_MAJOR_VERSION == 4
	struct value_pointer_marker {};
	FFastPropertyInstanceInfo(void* ValuePointer, const FProperty* Property, value_pointer_marker);
#else
	FFastPropertyInstanceInfo(void* ValuePointer, const TSharedPtr<struct FPropertyInstanceInfo>& PropertyInstanceInfo);
#endif

	const TFieldPath<const FProperty>& GetProperty() const;
	FText GetDisplayName() const;
	FText GetType() const;
	FText GetValue() const;
	void* GetValuePointer() const;
	uint32 GetValueHash() const;
	TArray<FFastPropertyInstanceInfo>& GetChildren();
	bool IsValid() const;
	const TWeakObjectPtr<UObject>& GetObject() const;

	void Refresh();

	static bool ShouldExpandProperty(FFastPropertyInstanceInfo& PropertyInstanceInfo);

#if ENGINE_MAJOR_VERSION == 4
	static FText GetPropertyValueText_UE4(const FProperty* Property, const void* PropertyValue);
#endif

private:
	void PopulateObject();
	FText GetValueTextOfAllChildren();
	void PopulateText();
	void PopulateChildren();
	
	void* ValuePointer = nullptr;
	TFieldPath<const FProperty> Property;
	FText DisplayNameText;
	FText ValueText;
	FText TypeText;
	TWeakObjectPtr<UObject> Object = nullptr;
	TArray<FFastPropertyInstanceInfo> Children;
};
