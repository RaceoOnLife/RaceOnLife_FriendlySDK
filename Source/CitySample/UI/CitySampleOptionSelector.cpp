// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/CitySampleOptionSelector.h"

UCitySampleOptionSelector::UCitySampleOptionSelector()
{
	ValueType = ECitySampleOptionSelectorType::Boolean;

	bAutoInit = true;


	bInitialValue = true;
	
	NumericType = ECitySampleOptionSelectorNumericType::Float;
	InitialValue = 0.0f;
	StartValue = 0.0f;
	EndValue = 1.0f;
	ValueStepSize = 1.0f;

	InitialIndex = 0;
	MaxIndex = 0;

	BooleanLabelFalse = NSLOCTEXT("CitySampleOptionSelector", "BooleanFalseText", "Off");
	BooleanLabelTrue = NSLOCTEXT("CitySampleOptionSelector", "BooleanTrueText", "On");

	bWrapAround = true;
}

void UCitySampleOptionSelector::NativeOnInitialized()
{
	if (bAutoInit)
	{
		InitOptionSelector_Internal();
	}

	// Calls BP event OnInitialized after native initialization
	Super::NativeOnInitialized();
}

void UCitySampleOptionSelector::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (IsDesignTime())
	{
		InitOptionSelector_Internal();
	}
}

void UCitySampleOptionSelector::InitOptionSelectorBool(const bool bInitialValueOverride)
{
	if (ValueType != ECitySampleOptionSelectorType::Boolean)
	{
		const FString ValueTypeStr = UEnum::GetValueAsString(TEXT("/Script/CitySample.ECitySampleOptionSelectorType"), ValueType);
		UE_LOG(LogCitySampleUI, Error, TEXT("%s failed due to type mismatch. Option selector is of type '%s'"), ANSI_TO_TCHAR(__func__), *ValueTypeStr);
		return;
	}

	ValueCount = 2;
	ValueIndex = static_cast<int32>(bInitialValueOverride);

	// Ensure the initial value property matches the set value
	bInitialValue = bInitialValueOverride;

	// Let BP make updates according to the initial value
	const FText& ValueLabel = GetValueLabel();
	ReceiveValueChanged(ValueIndex, ValueLabel);
	ReceiveValueChangedBool(bInitialValue, ValueLabel);
}

void UCitySampleOptionSelector::InitOptionSelectorNumeric(const float InitialValueOverride)
{
	if (ValueType != ECitySampleOptionSelectorType::Numeric)
	{
		const FString ValueTypeStr = UEnum::GetValueAsString(TEXT("/Script/CitySample.ECitySampleOptionSelectorType"), ValueType);
		UE_LOG(LogCitySampleUI, Error, TEXT("%s failed due to type mismatch. Option selector is of type '%s'"), ANSI_TO_TCHAR(__func__), *ValueTypeStr);
		return;
	}

	ensure(ValueStepSize > SMALL_NUMBER);

	if (NumericType == ECitySampleOptionSelectorNumericType::Integer)
	{
		// Round all initialization properties to integer values
		StartValue = FMath::RoundToInt(StartValue);
		EndValue = FMath::RoundToInt(EndValue);
		InitialValue = FMath::RoundToInt(InitialValueOverride);
		ValueStepSize = FMath::Max(FMath::RoundToInt(ValueStepSize), 1);
	}

	// Get the min and max numeric values and clamp the initial value
	const float MinValue = FMath::Min(StartValue, EndValue);
	const float MaxValue = FMath::Max(StartValue, EndValue);
	InitialValue = FMath::Clamp(InitialValueOverride, MinValue, MaxValue);

	// Calculate the value range and divide by the step size to get the number of possible values
	const float Range = MaxValue - MinValue;
	ValueStepSize = FMath::Clamp(ValueStepSize, SMALL_NUMBER, Range);
	ValueCount = FMath::RoundToInt(Range / ValueStepSize) + 1;

	// Re-evaluate the value step size to best fit the final value count
	ValueStepSize = (ValueCount > 1) ? (Range / (ValueCount - 1)) : 1.0f;

	// Convert the numeric value label mappings to value index label mappings
	NumericIndexLabelMap.Reset();
	NumericIndexLabelMap.Reserve(NumericValueLabels.Num());
	for (const TPair<float, FText>& ValueLabelPair : NumericValueLabels)
	{
		NumericIndexLabelMap.Add(IndexOfNumeric(ValueLabelPair.Key), ValueLabelPair.Value);
	}

	// Set to the value nearest the given initial value
	ValueIndex = IndexOfNumeric(InitialValue);

	// Ensure the initial value property matches the set initial value
	InitialValue = GetValueNumeric();

	// Let BP make updates according to the initial value
	const FText& ValueLabel = GetValueLabel();
	ReceiveValueChanged(ValueIndex, ValueLabel);
	ReceiveValueChangedNumeric(InitialValue, ValueLabel);
}

void UCitySampleOptionSelector::InitOptionSelectorCustom(const int32 InitialIndexOverride)
{
	if (ValueType != ECitySampleOptionSelectorType::Custom)
	{
		const FString ValueTypeStr = UEnum::GetValueAsString(TEXT("/Script/CitySample.ECitySampleOptionSelectorType"), ValueType);
		UE_LOG(LogCitySampleUI, Error, TEXT("%s failed due to type mismatch. Option selector is of type '%s'"), ANSI_TO_TCHAR(__func__), *ValueTypeStr);
		return;
	}

	ValueCount = MaxIndex + 1;
	CustomLabels.SetNum(ValueCount, true);
	InitialIndex = FMath::Clamp(InitialIndexOverride, 0, MaxIndex);
	ValueIndex = InitialIndex;

	// Let BP make updates according to the initial value
	const FText& ValueLabel = GetValueLabel();
	ReceiveValueChanged(ValueIndex, ValueLabel);
	ReceiveValueChangedCustom(InitialIndex, ValueLabel);
}

void UCitySampleOptionSelector::InitOptionSelector_Internal()
{
	switch (ValueType)
	{
	case ECitySampleOptionSelectorType::Boolean:
		InitOptionSelectorBool(bInitialValue);
		break;
	case ECitySampleOptionSelectorType::Numeric: 
		InitOptionSelectorNumeric(InitialValue);
		break;
	case ECitySampleOptionSelectorType::Custom:
		InitOptionSelectorCustom(InitialIndex);
		break;

	default:
		checkNoEntry();
		break;
	}
}

void UCitySampleOptionSelector::SetValue_Internal(const int32 Index)
{
	ensureMsgf(Index >= 0 && Index < ValueCount, TEXT("%s failed: %d is an invalid index."), ANSI_TO_TCHAR(__func__), Index);

	// Set the new value index
	ValueIndex = Index;

	// Get the current value label
	const FText& ValueLabel = GetValueLabel();

	// Fire the appropriate events and delegates and let BP handle value change events
	ReceiveValueChanged(ValueIndex, ValueLabel);
	OnValueChanged.Broadcast(this, ValueIndex, ValueLabel);

	switch (ValueType)
	{
	case ECitySampleOptionSelectorType::Boolean:
	{
		const bool bCurrentValue = GetValueBool();
		ReceiveValueChangedBool(bCurrentValue, ValueLabel);
		OnValueChangedBool.Broadcast(this, bCurrentValue, ValueLabel);
		break;
	}

	case ECitySampleOptionSelectorType::Numeric:
	{
		const float CurrentValue = GetValueNumeric();
		ReceiveValueChangedNumeric(CurrentValue, ValueLabel);
		OnValueChangedNumeric.Broadcast(this, CurrentValue, ValueLabel);
		break;
	}

	case ECitySampleOptionSelectorType::Custom:
	{
		ReceiveValueChangedCustom(ValueIndex, ValueLabel);
		OnValueChangedCustom.Broadcast(this, ValueIndex, ValueLabel);
		break;
	}

	default:
		checkNoEntry();
		break;
	}
}

int32 UCitySampleOptionSelector::SetValue(const int32 Index)
{
	if (Index >= 0 && Index < ValueCount)
	{
		return SetValueChecked(Index);
	}
	else
	{
		return ValueIndex;
	}
}

int32 UCitySampleOptionSelector::SetValueChecked(const int32 Index)
{
	if (ValueIndex != Index)
	{
		SetValue_Internal(Index);
	}

	return ValueIndex;
}

int32 UCitySampleOptionSelector::SetValueClamped(const int32 Index)
{
	return SetValue(FMath::Clamp(Index, 0, ValueCount - 1));
}

int32 UCitySampleOptionSelector::SetValueNumeric(const float Value)
{
	return SetValueChecked(IndexOfNumeric(Value));
}

int32 UCitySampleOptionSelector::IncrementValue(const int32 StepSize/*=1*/)
{
	int32 NewValueIndex = ValueIndex + StepSize;

	if (bWrapAround)
	{
		if (NewValueIndex >= ValueCount)
		{
			// Wrap value index to first valid index
			NewValueIndex = 0;
		}
		else if (NewValueIndex < 0)
		{
			// Wrap value index to the last valid index
			NewValueIndex = ValueCount - 1;
		}
	}

	return SetValueClamped(NewValueIndex);
}

int32 UCitySampleOptionSelector::DecrementValue(const int32 StepSize/*=1*/)
{
	return IncrementValue(-StepSize);
}

void UCitySampleOptionSelector::ResetOptionSelector()
{
	switch (ValueType)
	{
	case ECitySampleOptionSelectorType::Boolean:
		SetValueChecked(static_cast<int32>(bInitialValue));
		break;

	case ECitySampleOptionSelectorType::Numeric:
	{
		SetValueChecked(IndexOfNumeric(InitialValue));
		break;
	}

	case ECitySampleOptionSelectorType::Custom:
		SetValueChecked(InitialIndex);
		break;

	default:
		checkNoEntry();
		break;
	}
}

FText UCitySampleOptionSelector::GetValueLabel() const
{
	FText ValueLabel;

	switch (ValueType)
	{
	case ECitySampleOptionSelectorType::Boolean:
		ValueLabel = GetValueBool() ? BooleanLabelTrue : BooleanLabelFalse;
		break;

	case ECitySampleOptionSelectorType::Numeric:
	{
		const FText* CustomLabel = NumericIndexLabelMap.Find(ValueIndex);
		ValueLabel = CustomLabel ? *CustomLabel : GetValueDefaultLabelNumeric();
		break;
	}

	case ECitySampleOptionSelectorType::Custom:
		ValueLabel = CustomLabels.IsValidIndex(ValueIndex) ? CustomLabels[ValueIndex] : FText();
		break;

	default:
		checkNoEntry();
		break;
	}

	const FFormatOrderedArguments OutValueLabelArgs = { ValueLabelPrefix, ValueLabel, ValueLabelSuffix };
	return FText::Format(NSLOCTEXT("CitySampleOptionSelector", "OutValueLabel", "{0}{1}{2}"), OutValueLabelArgs);
}

FText UCitySampleOptionSelector::GetValueDefaultLabelNumeric() const
{
	const float NumericValue = GetValueNumeric();

	const auto FindCustomization = [NumericValue](const FCitySampleOptionsNumericRangeCustomization& Customization)
	{
		return Customization.ValueRange.Contains(NumericValue);
	};

	const FCitySampleOptionsNumericRangeCustomization* Customization = NumericRangeCustomization.FindByPredicate(FindCustomization);
	
	switch (NumericType)
	{
	case ECitySampleOptionSelectorNumericType::Float:
		if (Customization)
		{
			const FText ValueString = FText::AsNumber(NumericValue * Customization->Scaling);
			const FFormatOrderedArguments OutValueDefaultLabelNumericArgs = { Customization->Prefix, ValueString, Customization->Suffix };
			return FText::Format(NSLOCTEXT("CitySampleOptionSelector", "OutValueDefaultLabelNumeric", "{0}{1}{2}"), OutValueDefaultLabelNumericArgs);
		}
		else
		{
			return FText::AsNumber(NumericValue);
		}
	
	case ECitySampleOptionSelectorNumericType::Percent:
		if (Customization)
		{
			const FText ValueString = FText::AsPercent(NumericValue * Customization->Scaling);
			const FFormatOrderedArguments OutValueDefaultLabelNumericArgs = { Customization->Prefix, ValueString, Customization->Suffix };
			return FText::Format(NSLOCTEXT("CitySampleOptionSelector", "OutValueDefaultLabelNumeric", "{0}{1}{2}"), OutValueDefaultLabelNumericArgs);
		}
		else
		{
			return FText::AsPercent(NumericValue);
		}

	case ECitySampleOptionSelectorNumericType::Integer:
		if (Customization)
		{
			const FText ValueString = FText::AsNumber(FMath::RoundToInt(NumericValue * Customization->Scaling));
			const FFormatOrderedArguments OutValueDefaultLabelNumericArgs = { Customization->Prefix, ValueString, Customization->Suffix };
			return FText::Format(NSLOCTEXT("CitySampleOptionSelector", "OutValueDefaultLabelNumeric", "{0}{1}{2}"), OutValueDefaultLabelNumericArgs);
		}
		else
		{
			return FText::AsNumber(FMath::RoundToInt(NumericValue));
		}

	default:
		checkNoEntry();
		return FText();
	}
}

#if WITH_EDITOR
void UCitySampleOptionSelector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//if (PropertyName == GET_MEMBER_NAME_CHECKED(UCitySampleOptionSelector, Propety))
	//{
	//}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR
