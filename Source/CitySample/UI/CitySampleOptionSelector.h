// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CitySampleButtonPrompt.h"
#include "CitySampleOptionSelector.generated.h"


UENUM(BlueprintType)
enum class ECitySampleOptionSelectorType : uint8
{
	Boolean,
	Numeric,
	Custom
};

UENUM(BlueprintType)
enum class ECitySampleOptionSelectorNumericType : uint8
{
	Float,
	Percent,
	Integer
};

USTRUCT()
struct FCitySampleOptionsNumericRangeCustomization
{
	GENERATED_BODY();

public:
	/** Value range in which the scaling should be applied. */
	UPROPERTY(EditAnywhere)
	FFloatRange ValueRange;

	/** Additional scaling applied to the range when generating the default value label. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.00001"))
	float Scaling = 1.0f;

	/** String that will be appended to the front of the value label. */
	UPROPERTY(EditAnywhere)
	FText Prefix;

	/** String that will be appended to the back of the value label. */
	UPROPERTY(EditAnywhere)
	FText Suffix;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCitySampleOptionSelectorOnValueChanged, UCitySampleOptionSelector* const, OptionSelector, const int32, Index, const FText&, ValueLabel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCitySampleOptionSelectorOnValueChangedBool, UCitySampleOptionSelector* const, OptionSelector, const bool, bValue, const FText&, ValueLabel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCitySampleOptionSelectorOnValueChangedNumeric, UCitySampleOptionSelector* const, OptionSelector, const float, Value, const FText&, ValueLabel);


/**
 * Represents a menu option with a list of labeled values from which one value can be selected.
 * These values can be boolean, numeric, or custom (represented by a labeled index).
 */
UCLASS()
class CITYSAMPLE_API UCitySampleOptionSelector : public UCitySampleButtonPrompt
{
	GENERATED_BODY()
	

public:
	UCitySampleOptionSelector();

	virtual void NativeOnInitialized() override;
	virtual void SynchronizeProperties() override;

public:
	/**
	 * Initializes the option selector to the given initial value.
	 * @note	If called, bAutoInit should most likely be disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	void InitOptionSelectorBool(const bool bInitialValueOverride);

	/**
	 * Initializes the option selector to the given initial value.
	 * @note	If called, bAutoInit should most likely be disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	void InitOptionSelectorNumeric(const float InitialValueOverride);

	/**
	 * Initializes the option selector to the given initial value.
	 * @note	If called, bAutoInit should most likely be disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	void InitOptionSelectorCustom(const int32 InitialIndexOverride);

private:
	/** Initializes the widget using editor defined initial values when bAutoInit is enabled. */
	void InitOptionSelector_Internal();

	/** Private helper for setting the current value to a given index. */
	void SetValue_Internal(const int32 Index);
	
public:
	/** Sets the option selector to the value at the given index, if valid. Otherwise fails quietly. */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	int32 SetValue(const int32 Index);

	/** Sets the option selector to the value at the given index, if valid. Otherwise, ensures and logs failure. */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	int32 SetValueChecked(const int32 Index);
	
	/** Sets the option selector to the value at the given index clamped to the valid index range. */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	int32 SetValueClamped(const int32 Index);
	
	/** Sets the option selector to the index of the closest numeric value to the passed in value. */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	int32 SetValueNumeric(const float Value);

	/**
	 * Increments the current value index by the step size and calls SetValueClamped.
	 *
	 * @note	If bWrapAround is set, this wraps around to the value at index 0 when incrementing past the last valid index.
	 */	
	 UFUNCTION(BlueprintCallable, Category = "Option Selector")
	int32 IncrementValue(const int32 StepSize=1);

	/**
	 * Decrements the current value index by the step size and calls SetValueClamped.
	 *
	 * @note	If bWrapAround is set, this wraps around to the value at the highest valid index when decrementing past index 0.
	 */	
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	int32 DecrementValue(const int32 StepSize=1);

	/** Sets the current value to the initial value. Trusts that the Option Selector has been properly initialized via InitOptionsSelector(). */
	UFUNCTION(BlueprintCallable, Category = "Option Selector")
	void ResetOptionSelector();

	/** Returns the current value as a bool. False at index 0. Otherwise true. */
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	bool GetValueBool() const
	{
		return ValueIndex != 0;
	}

	/** 
	 * Returns the current value as a float, even though the value may represent an integer or percentage.
	 * 
	 * @note	The returned value is only valid when the option selector value type is set as numeric.
	 */
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	float GetValueNumeric() const
	{
		return (ValueIndex * ((StartValue <= EndValue) ? ValueStepSize : -ValueStepSize)) + StartValue;
	}
	
	/** 
	 * Returns the current value index. 
	 * 
	 *	Index Ranges
	 *		- Boolean:	0 [false] - 1 [true]
	 *		- Numeric:	0 - (Range / Step Size)
	 *		- Custom:	0 - MaxIndex
	 */
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	int32 GetValueIndex() const
	{
		return ValueIndex;
	}
	
	/** 
	 * Returns the closest index for a given numeric value.
	 * 
	 * @note	Values outside range between Start and End values will have an appropriately out of range index.
	 * @note	This is mainly used for mapping numeric values to a custom label.
	 */
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	int32 IndexOfNumeric(float Value) const
	{
		return FMath::RoundToInt(FMath::Abs((Value - StartValue) / ValueStepSize));
	}
	
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	int32 GetValueCount() const
	{
		return ValueCount;
	}

	/** Returns the string that represents the current value. */
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	FText GetValueLabel() const;

	/** Returns the array of labels for each index. Only applicable when the option selector value type is set to "Custom". */
	UFUNCTION(BlueprintPure, Category = "Option Selector")
	const TArray<FText>& GetCustomLabels() const
	{
		return CustomLabels;
	}

protected:
	/** Whether the option selector value should be treated as a boolean, numeric, or custom indexed value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector")
	ECitySampleOptionSelectorType ValueType;

	/** 
	 * Whether the option selector should automatically initialize with the compile-time initial value.
	 * @note	Disable this when you want to modify the initial value at runtime, then call InitOptionSelector manually.
	 */
	UPROPERTY(EditAnywhere, Category = "Option Selector")
	bool bAutoInit;

	/** The boolean value used to initialize the option selector when bAutoInit is enabled. */
	UPROPERTY(EditAnywhere, Category = "Option Selector|Boolean", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Boolean && bAutoInit"))
	bool bInitialValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Boolean", meta = (EditCondition = "ValueType == ECitySampleOptionSelectorType::Boolean"))
	FText BooleanLabelFalse;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Boolean", meta = (EditCondition = "ValueType == ECitySampleOptionSelectorType::Boolean"))
	FText BooleanLabelTrue;

	/** Numeric type of the option selector value, affecting the default label string representation and the valid property value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Numeric", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric"))
	ECitySampleOptionSelectorNumericType NumericType;

	/** The numeric value used to initialize the option selector when bAutoInit is enabled. Clamped between StartValue and EndValue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Numeric", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric && bAutoInit"))
	float InitialValue;

	/** The numeric value assigned to value index 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Numeric", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric"))
	float StartValue;

	/** The numeric value that is assigned to the last valid index (the index equivalent to Range / Step Size). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Numeric", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric"))
	float EndValue;

	/** The value step sized used to calculate the value change between indices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Numeric", meta=(ClampMin="0.00001", EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric"))
	float ValueStepSize;

	/** Additional scaling applied to the values when generating the default value label. */
	UPROPERTY(EditAnywhere, Category = "Option Selector|Numeric", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric"))
	TArray<FCitySampleOptionsNumericRangeCustomization> NumericRangeCustomization;

	/** Optional mapping to add custom labels for specific values, overriding the default label for the given numeric type. */
	UPROPERTY(EditAnywhere, Category = "Option Selector|Numeric", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Numeric"))
	TMap<float, FText> NumericValueLabels;

	/** Index of the value used to initialize the option selector when bAutoInit is enabled. Only applicable when the option selector type is set to "Custom". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Custom", meta=(ClampMin="0", EditCondition="ValueType == ECitySampleOptionSelectorType::Custom && bAutoInit"))
	int32 InitialIndex;

	/** Max index for the "Custom" values, effectively denoting the number of custom values (minus 1 for Index 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector|Custom", meta=(ClampMin="0", EditCondition="ValueType == ECitySampleOptionSelectorType::Custom"))
	int32 MaxIndex;
	
	/** Array of custom labels for each value index. Only applicable when the option selector type is set to "Custom". */
	UPROPERTY(EditAnywhere, Category = "Option Selector|Custom", meta=(EditCondition="ValueType == ECitySampleOptionSelectorType::Custom"))
	TArray<FText> CustomLabels;

	/** Whether IncrementValue/DecrementValue wraps around to the first/last value when exceeding valid indices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector")
	bool bWrapAround;

	/** String that will be appended to the front of the value label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector")
	FText ValueLabelPrefix;

	/** String that will be appended to the back of the value label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Option Selector")
	FText ValueLabelSuffix;

	/** BP hook for when the option selector value is updated for any type. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Option Selector")
	void ReceiveValueChanged(const int32 Index, const FText& ValueLabel);

	/** BP hook for when the option selector value is Boolean and the value is updated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Option Selector")
	void ReceiveValueChangedBool(const bool bValue, const FText& ValueLabel);

	/** BP hook for when the option selector value is Numeric and the value is updated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Option Selector")
	void ReceiveValueChangedNumeric(const float Value, const FText& ValueLabel);

	/** BP hook for when the option selector value is a Custom type and the value is updated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Option Selector")
	void ReceiveValueChangedCustom(const int32 Index, const FText& ValueLabel);

	/** Delegate fired when the option selector value is changed for any type. */
	UPROPERTY(BlueprintAssignable, Category = "Option Selector")
	FCitySampleOptionSelectorOnValueChanged OnValueChanged;

	/** Delegate fired when the option selector value is Boolean and the value is changed. */
	UPROPERTY(BlueprintAssignable, Category = "Option Selector")
	FCitySampleOptionSelectorOnValueChangedBool OnValueChangedBool;

	/** Delegate fired when the option selector value is Numeric and the value is changed. */
	UPROPERTY(BlueprintAssignable, Category = "Option Selector")
	FCitySampleOptionSelectorOnValueChangedNumeric OnValueChangedNumeric;
	
	/** Delegate fired when the option selector value is a Custom type and the value is changed. */
	UPROPERTY(BlueprintAssignable, Category = "Option Selector")
	FCitySampleOptionSelectorOnValueChanged OnValueChangedCustom;

private:
	/**
	 * The current value index.
	 *
	 *	Ranges
	 *		- Boolean:	0 [false] - 1 [true]
	 *		- Numeric:	0 - (Range / Step Size)
	 *		- Custom:	0 - MaxIndex
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Option Selector")
	int32 ValueIndex;

	/** The number of possible values. Boolean = 2, Numeric = Range / StepSize, Custom = MaxIndex + 1. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Option Selector")
	int32 ValueCount;
	
	/** A mapping of value indices to a string label. Used to override default numeric labels for specific values. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Option Selector|Numeric")
	TMap<int32, FText> NumericIndexLabelMap;

	/** Gets the default string representation for a numeric value. */
	FText GetValueDefaultLabelNumeric() const;

#if WITH_EDITOR
private:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
};
