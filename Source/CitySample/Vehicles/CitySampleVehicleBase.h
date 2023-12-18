// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "WheeledVehiclePawn.h"

#include "MassActorPoolableInterface.h"

#include "MassTrafficVehicleControlInterface.h"

#include "Game/ICitySampleTraversalInterface.h"
#include "UI/CitySampleControlsOverlayInterface.h"
#include "UI/CitySampleUIComponentInterface.h"
#include "Util/CitySampleInterpolators.h"
#include "Util/CitySampleRumbleInfo.h"
#include "Util/ICitySampleInputInterface.h"
#include "Vehicles/CitySampleDrivingState.h"

#include "CitySampleVehicleBase.generated.h"

class ACitySampleCharacter;

class UCitySampleMenu;
class UCitySampleUIComponent;

/**
 * Enum to help identify seat types in drivable vehicles
 */
UENUM(BlueprintType)
enum class ECitySampleVehicleSeat : uint8
{
	Driver,
	FrontPassenger,
	RearDriver,
	RearPassenger, 
	MAX
};

/**
 * Used by destructible vehicles to store a history of transform information
 */
USTRUCT(BlueprintType)
struct FTimestampedTransform
{
	GENERATED_BODY()

	FTimestampedTransform() 
	{

	}

	FTimestampedTransform(float _TimeStamp, FTransform _Transform) :
		TimeStamp(_TimeStamp),
		Transform(_Transform)
	{

	}

	UPROPERTY(Transient, BlueprintReadWrite)
	float TimeStamp = 0.0f;

	UPROPERTY(Transient, BlueprintReadWrite)
	FTransform Transform;
};

/**
 * Enum to help identify seat types in drive-able vehicles
 */
struct FCitySampleWheelSuspensionState
{
public:
	float PreviousNormalizedSuspensionLength = 0.0f;
	float TimeSpentAtFullLength = 0.0f;
};

/** Multicast delegates for various vehicle events for different features like sound and animation to interact with */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerEnterVehicle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerExitVehicle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAccelerationStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAccelerationStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAirborneStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAirborneStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDriftStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDriftStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBrakeStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBrakeStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandbrakeStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandbrakeStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorClose);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIgnitionStart);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVehicleBackfire);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSleepStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSleepStop);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWheelBump, int, WheelIndex, float, BumpStrength);

/**
 * Base class for driveable playable vehicles
 */
UCLASS(BlueprintType)
class CITYSAMPLE_API ACitySampleVehicleBase :	public AWheeledVehiclePawn,
										public ICitySampleTraversalInterface,
										public ICitySampleUIComponentInterface,
										public ICitySampleControlsOverlayInterface,
										public ICitySampleInputInterface,
										public IMassActorPoolableInterface,
										public IMassTrafficVehicleControlInterface
{
	GENERATED_BODY()

public:
	ACitySampleVehicleBase();

	//~ Begin ICitySampleTraversalInterface Interface
	virtual EPlayerTraversalState GetTraversalState_Implementation() const override
	{
		return EPlayerTraversalState::InVehicle;
	}
	//~ End ICitySampleTraversalInterface Interface

	//~ Begin IMassActorPoolableInterface
	void PrepareForPooling_Implementation() override;
	void PrepareForGame_Implementation() override;
	//~ End IMassActorPoolableInterface

	//~ Begin ICitySampleInputInterface Interface
	virtual void AddInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface) override;
	virtual void RemoveInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface) override;
	//~ End ICitySampleInputInterface Interface

	//~ Begin ICitySampleControlsOverlayInterface Interface
	virtual TMap<UInputAction*, FText> GetInputActionDescriptions_Implementation() const override
	{
		return InputActionDescriptions;
	};

	virtual TMap<FKey, FText> GetInputKeyDescriptionOverrides_Implementation() const override
	{
		return InputKeyDescriptionOverrides;
	};
	//~ End ICitySampleControlsOverlayInterface Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin APawn Interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	//~ End APawn Interface

	//~ Begin IMassTrafficVehicleControlInterface
	virtual void SetVehicleInputs_Implementation(const float Throttle, const float Brake, const bool bHandBrake, const float Steering, const bool bSetSteering) override;
	virtual void OnParkedVehicleSpawned_Implementation() override;
	virtual void OnTrafficVehicleSpawned_Implementation() override;
	//~ End IMassTrafficVehicleControlInterface 

	UFUNCTION(BlueprintPure, Category = "Driving")
	const FCitySampleDrivingState& GetDrivingState() const 
	{ 
		return DrivingState; 
	}

	UFUNCTION(BlueprintCallable, Category = "Driving")
	FVector GetMeasuredVelocity() const
	{
		return MeasuredVelocity;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	float GetThrottleInput() const
	{
		return DrivingState.Throttle;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	float GetBrakeInput() const
	{
		return DrivingState.Brake;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	bool IsHandbrakeOn() const
	{
		return DrivingState.bHandbrakeOn;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	float GetForwardSpeed() const
	{
		return DrivingState.ForwardSpeed;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	float GetRPM() const
	{
		return DrivingState.RPM;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	int32 GetGear() const
	{
		return DrivingState.Gear;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	bool IsUsingAutoGears() const
	{
		return DrivingState.bAutomatic;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	bool HasAllWheelsOnGround() const
	{
		return DrivingState.bAllWheelsOnGround;
	}

	UFUNCTION(BlueprintPure, Category = "Driving")
	bool ShouldHideDrivingUI() const
	{
		return bHideDrivingUI;
	}

	UFUNCTION(BlueprintCallable, Category = "Driving")
	void RequestHideDrivingUI(const bool bShouldHideDrivingUI)
	{
		bHideDrivingUI = bShouldHideDrivingUI;
	}

	UFUNCTION(BlueprintPure, Category = Pawn)
	virtual bool IsPossessableByPlayer() const 
	{ 
		return bIsPossessableByPlayer; 
	}

	UFUNCTION(BlueprintPure)
	bool IsPossessedByPlayer() const 
	{ 
		return bIsPossessedByPlayer; 
	}

	UFUNCTION(BlueprintPure)
	float GetTimeSpentSharpSteering() const 
	{ 
		return TimeSpentSharpSteering; 
	}

	UFUNCTION(BlueprintPure, Category = "Camera Overriddes")
	bool ShouldOverrideCameraToPivotOffset() const 
	{
		return bOverrideCameraToPivotOffset;
	}

	UFUNCTION(BlueprintPure, Category = "Camera Overriddes")
	bool ShouldOverrideLookAtPointOffset() const 
	{ 
		return bOverrideLookAtPointOffset; 
	}

	UFUNCTION(BlueprintPure, Category = "Camera Overriddes")
	const FVector& GetCameraToPivotOffsetOverride() const 
	{ 
		return CameraToPivotOffsetOverride; 
	}

	UFUNCTION(BlueprintPure, Category = "Camera Overriddes")
	const FVector& GetLookAtPointOffsetOverride() const 
	{ 
		return LookAtPointOffsetOverride; 
	}

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool ShouldDrawRumbleDebugInfo() const;

	/** Whether or not the vehicle center of mass debug widget should be drawn */
	bool ShouldDrawCenterOfMassDebug() const;
	
	/** Ensures any leftover pawns inside the vehicle are properly destroyed */
	void CleanUpUnpossessedDriver();

	/** Calculates and applies proper parameter values for wheel motion blur */
	UFUNCTION(BlueprintCallable)
	void ApplyWheelMotionBlurNative(const TArray<UMaterialInstanceDynamic*>& MotionBlurMIDs);

	/** Compares current actor rotation against threshold to determine if the vehicle is flipped */
	UFUNCTION(BlueprintCallable)
	bool IsVehicleFlippedOver(const float RollThreshold) const;

	/** Re-orients a vehicle with the local Z-axis pointing up. */
	UFUNCTION(BlueprintCallable)
	void FlipVehicle();

	/** Returns seat socket name that corresponds to the vehicle seat type passed in */
	UFUNCTION(BlueprintCallable)
	FName GetSeatSocket(ECitySampleVehicleSeat Seat) const;

	/** Returns reference to character in vehicle seat if one exists */
	UFUNCTION(BlueprintCallable)
	ACitySampleCharacter* GetOccupantInSeat(ECitySampleVehicleSeat Seat) const;

	/** Formally associates a character with a seat in the vehicle */
	void SetSeatOccupant(ECitySampleVehicleSeat Seat, ACitySampleCharacter* NewOccupant);

	/** Returns the COM position in terms of the vehicle's bounding box. A COM in the middle of the vehicle will give you 0.5, 0.5 */
	UFUNCTION(BlueprintPure, Category = "Debug")
	FVector2D GetCOMPositionRatio() const;

	/** Returns a flag as to whether or not the vehicle is drifting.Determined by rear wheel slip angles and a user - defined threshold */
	UFUNCTION(BlueprintPure, Category = "Drifting")
	bool IsVehicleDrifting() const;

	/** Returns requested wheel slip angle in degrees */
	UFUNCTION(BlueprintPure, Category = "Drifting")
	float GetWheelSlipAngleInDegrees(int WheelIndex) const;

	/** Returns the requested wheel's suspension length in a normalized format (0.f - 1.f) */
	UFUNCTION(BlueprintPure, Category = "Suspension")
	float GetNormalizedSuspensionLength(int WheelIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SetThrottleDisabled(bool IsDisabled)
	{
		bThrottleDisabled = IsDisabled;
	}

	/** Sets steering input modifier. Used to force a vehicle in a certain direction, like when damaged */
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void SetSteeringModifier(float NewModifier);

	/** BP Callable function for executing gameplay - related vehicle exit logic that isn't necessarily tied to pawn possession state */
	UFUNCTION(BlueprintCallable)
	void OnExitVehicle();

	/** Updates the TransformsOverTime member with relevant information */
	UFUNCTION(BlueprintCallable)
	void UpdateTransformsOverTime(class USkeletalMeshComponent* SKM_Proxy);

	/** List of vehicle transforms stored with accompanying timestamps */
	UPROPERTY(BlueprintReadWrite)
	TArray<FTimestampedTransform> TransformsOverTime;

	/** Checks vehicle speed against custom threshold to determine whether CCD should be used */
	UFUNCTION(BlueprintCallable)
	void UpdateContinuousCollisionDetectionAtSpeed();

	/** Resets motion vector values for all components composing this actor */
	UFUNCTION(BlueprintCallable)
	void ResetMotionBlurForAllComponentsThisFrame();

	/** Vehicle event multicast delegates. Useful when other systems want to be keyed into changes in the vehicle */
	UPROPERTY(BlueprintAssignable)
	FOnPlayerEnterVehicle OnPlayerEnterVehicle;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerExitVehicle OnPlayerExitVehicle;

	UPROPERTY(BlueprintAssignable)
	FOnAccelerationStart OnAccelerationStart;

	UPROPERTY(BlueprintAssignable)
	FOnAccelerationStop OnAccelerationStop;

	UPROPERTY(BlueprintAssignable)
	FOnReverseStart OnReverseStart;

	UPROPERTY(BlueprintAssignable)
	FOnReverseStop OnReverseStop;

	UPROPERTY(BlueprintAssignable)
	FOnAirborneStart OnAirborneStart;

	UPROPERTY(BlueprintAssignable)
	FOnAirborneStop OnAirborneStop;

	UPROPERTY(BlueprintAssignable)
	FOnDriftStart OnDriftStart;

	UPROPERTY(BlueprintAssignable)
	FOnDriftStop OnDriftStop;

	UPROPERTY(BlueprintAssignable)
	FOnBrakeStart OnBrakeStart;

	UPROPERTY(BlueprintAssignable)
	FOnBrakeStop OnBrakeStop;

	UPROPERTY(BlueprintAssignable)
	FOnHandbrakeStart OnHandbrakeStart;

	UPROPERTY(BlueprintAssignable)
	FOnHandbrakeStop OnHandbrakeStop;

	UPROPERTY(BlueprintAssignable)
	FOnWheelBump OnWheelBump;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnDoorOpen OnDoorOpen;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnDoorClose OnDoorClose;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnIgnitionStart OnIgnitionStart;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnVehicleBackfire OnVehicleBackfire;

	UPROPERTY(BlueprintAssignable)
	FOnSleepStart OnSleepStart;

	UPROPERTY(BlueprintAssignable)
	FOnSleepStop OnSleepStop;

protected:

	/** Sets corresponding Vehicle Movement Component parked flag */
	UFUNCTION(BlueprintCallable, Category = "Driving")
	void SetParked(bool bIsParked);

	/** Callback that resets vehicle input values when opening the ingame pause menu */
	UFUNCTION()
	void OnOptionsMenuOpened(UCitySampleUIComponent* const CitySampleUI, UCitySampleMenu* const OptionsMenu);

	/** Sets vehicle throttle value, applies it, and broadcasts any relevant events as a result */
	UFUNCTION(BlueprintCallable, Category = "Driving")
	void SetThrottleInput(float Throttle);

	/** Sets vehicle brake value and applies it */
	UFUNCTION(BlueprintCallable, Category = "Driving")
	void SetBrakeInput(float Brake);

	/** Sets vehicle steering value and applies it */
	UFUNCTION(BlueprintCallable, Category = "Driving")
	void SetSteeringInput(float Steering);

	/** Sets vehicle handbrake value, applies it, and broadcasts any relevant events as a result */
	UFUNCTION(BlueprintCallable, Category = "Driving")
	void SetHandbrakeInput(bool bHandbrakeActive);

	/** Resets all vehicle input values, useful when you want to clear vehicle inputs in the event of a sudden loss of input context */
	void ResetInputs();

	/** Called via input to look at a given rate */
	void Look(const FVector2D& Value);

	/** Called via input to turn at a given rate */
	void TurnAtRate(float Rate);

	/** Called via input to turn look up/down at a given rate. */
	void LookUpAtRate(float Rate);

	/** Flag to keep track of whether or not inputs are active for this vehicle instance - useful for making sure queued inputs are blocked post - input context removal */
	bool bInputsActive = false;

	/** Base camera turn rate, in deg/sec. Other scaling may affect final turn rate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base camera look up/down rate, in deg/sec. Other scaling may affect final rate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	/** Raw steering magnitude that needs to be passed for the vehicle to be considered to be "Sharp Steering". This is a value that helps the driving camera with turning behaviors */
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	float SharpSteeringThreshold = .75f;

	/** Mapping of pawn relevant InputActions to their string description for the controls overlay UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls Overlay")
	TMap<UInputAction*, FText> InputActionDescriptions;

	/** Mapping of pawn relevant input keys to their string description for the controls overlay UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls Overlay")
	TMap<FKey, FText> InputKeyDescriptionOverrides;

	/** Delay before player can actually drive the vehicle upon receiving the "OnIgnitionStart" event*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driving")
	float PlayerDrivingDelay = 1.3f;

	/** Specifies that the driving UI, if it exists, should be hidden */
	UPROPERTY(EditAnywhere, Category = "Driving")
	bool bHideDrivingUI;

	/** Offset amount uses to increment the search radius after each failed attempt to find a teleport spot to flip */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving")
	float TeleportOffsetStep;

	/** Max search distance for a valid teleport spot when attempting to flip */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving")
	float FlipVehicleMaxDistance;

	/** Speed threshold used when determining whether to use Continuous Collision Detection */
	UPROPERTY(EditDefaultsOnly)
	float CCDSpeedThreshold;
	 
	/** Flag for keeping track of whether Continous Collision Detection is currently in use */
	bool bUseCCDAtSpeed = false;

	/** List of seat socket names, should be ordered by CitySampleVehicleSeat enum */
	UPROPERTY(EditAnywhere)
	FName SeatSockets[(uint8)ECitySampleVehicleSeat::MAX];

	/** List of vehicle occupants, should be ordered by CitySampleVehicleSeat enum */
	UPROPERTY(transient)
	ACitySampleCharacter* Occupants[(uint8)ECitySampleVehicleSeat::MAX];

	/** Velocity measured by actor displacement since last tick */
	FVector MeasuredVelocity;

	/** Stores Actor location from previous tick for measuring velocity */
	FVector LocationLastTick;

	/** Whether the vehicle is possess-able by a player */
	bool bIsPossessableByPlayer = true;

	/** Whether the vehicle is currently possessed by a player */
	bool bIsPossessedByPlayer = false;

	/** Max wheel angular velocity need to hit the max blur wheel angle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel Motion Blur")
	float BlurAngleVelocityMax = 3000.f;

	/** Max wheel angle to pass into the Motion Blur MID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel Motion Blur")
	float BlurAngleMax = 0.035f;

	/** List of cached motion blur wheel angle used when applying wheel motion blur updates */
	TArray<float> CachedMotionBlurWheelAngle;

	/** List of cached MIDs used when applying wheel motion blur updates */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> CachedMotionBlurWheelMIDs;

	/** BIE for updating rumble info tied to time the suspension spent at full length in the driver side wheels */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rumble|Suspension")
	void UpdateLeftSuspensionRumbleInfo(float MaxSuspensionTime);

	/** BIE for updating rumble info tied to time the suspension spent at full length in the passenger side wheels */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rumble|Suspension")
	void UpdateRightSuspensionRumbleInfo(float MaxSuspensionTime);

	/** Plays Dynamic Force Feedback based on current LeftSuspensionRumbleInfo */
	UFUNCTION(BlueprintCallable, Category = "Rumble|Suspension")
	void TriggerLeftSuspensionRumble();

	/** Plays Dynamic Force Feedback based on current RightSuspensionRumbleInfo */
	UFUNCTION(BlueprintCallable, Category = "Rumble|Suspension")
	void TriggerRightSuspensionRumble();

	/** BIE for updating rumble info tied to sudden changes in wheel suspension in the driver side wheels */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rumble|Bumps")
	void UpdateLeftBumpRumbleInfo(float SuspensionDiff);

	/** BIE for updating rumble info tied to sudden changes in wheel suspension in the passenger side wheels */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rumble|Bumps")
	void UpdateRightBumpRumbleInfo(float SuspensionDiff);

	/** Plays Dynamic Force Feedback based on current LeftBumpRumbleInfo */
	UFUNCTION(BlueprintCallable, Category = "Rumble|Bumps")
	void TriggerLeftBumpRumble();

	/** Plays Dynamic Force Feedback based on current RightBumpRumbleInfo */
	UFUNCTION(BlueprintCallable, Category = "Rumble|Bumps")
	void TriggerRightBumpRumble();

	/** Plays Dynamic Force Feedback based on current CollisionRumbleInfo which is updated on collision events */
	UFUNCTION(BlueprintCallable, Category = "Rumble|Collision")
	void TriggerCollisionRumble();

	/** Returns physical surface type currently detected underneath the requested wheel */
	UFUNCTION(BlueprintPure, Category = "Rumble|Surfaces")
	EPhysicalSurface GetPhysicalSurfaceUnderWheel(int32 WheelIndex) const;

	/** List of active wheel suspension states, useful for knowing when to trigger things like suspension based rumble */
	TArray<FCitySampleWheelSuspensionState> WheelSuspensionStates;

	/** Difference in Normalized Suspension in one frame required to trigger a rumble */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rumble|Bumps")
	float NormalizedSuspensionDiffThreshold = 0.1f;

	/** Rumble Info members used to store and set ForceFeedback parameters for the variety of contexts in which the vehicle can trigger ForceFeedbacks */
	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Speed")
	FCitySampleRumbleInfo SpeedRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Cornering")
	FCitySampleRumbleInfo CorneringRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Surfaces")
	FCitySampleRumbleInfo LeftSurfaceRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Surfaces")
	FCitySampleRumbleInfo RightSurfaceRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Suspension")
	FCitySampleRumbleInfo LeftSuspensionRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Suspension")
	FCitySampleRumbleInfo RightSuspensionRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Bumps")
	FCitySampleRumbleInfo LeftBumpRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Bumps")
	FCitySampleRumbleInfo RightBumpRumbleInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Rumble|Collisions")
	FCitySampleRumbleInfo CollisionRumbleInfo;

	/** Handles for all Dynamic Force Feedback instances the vehicle supports */
	uint64 SpeedRumbleHandle = 0;
	uint64 CorneringRumbleHandle = 0;
	uint64 LeftSurfaceRumbleHandle = 0;
	uint64 RightSurfaceRumbleHandle = 0;
	uint64 LeftSuspensionRumbleHandle = 0;
	uint64 RightSuspensionRumbleHandle = 0;
	uint64 LeftBumpRumbleHandle = 0;
	uint64 RightBumpRumbleHandle = 0;
	uint64 CollisionRumbleHandle = 0;

	////////////////////////////////////////////////////////////
	///// Below are all members used to tweak driving feel /////
	////////////////////////////////////////////////////////////

	/** Ties Angular Damping Strength to current steering value. The less steering applied, the more angular damping is applied - which helps keep the car straight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angular Damping")
	bool bAdjustAngularDampingToSteering = false;

	/** Curve that applies angular damping based on your current steering magnitude. X = Steering, Y = Damping to apply. Damping scales with the "Angular Damping Scale" property */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angular Damping", meta = (EditCondition = bAdjustAngularDampingToSteering))
	UCurveFloat* SteeringToAngularDamping;

	/** Final scale value applied to the result when calculating angular damping in relation to steering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angular Damping", meta = (EditCondition = bAdjustAngularDampingToSteering))
	float AngularDampingScale = 2.0f;

	/** Enable to use Center of Mass adjustments at run time. Center of mass settings can be adjusted to alter driving feel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments")
	bool bUseCenterOfMassAdjustments = false;

	/** What Forward offset to apply to the center of mass when using the handbrake.  This is additive with any center of mass adjustments calculated from the "SteeringToForwardCOMOffset" curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	UCurveFloat* HandbrakeForwardCOMOffset;

	/** Scale that multiplies the offset set in the "HandbrakeForwardCOMOffset curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	float HandbrakeForwardCOMOffsetScale = 1.0f;

	/** Member to store forward Handbrake Interpolation results between updates */
	float ForwardHandbrakeCurveInput = 0.0f;

	/** Overrides Interpolation speeds for the "HandbrakeForwardCOMOffsetInterpolator". X = Rise Speed (Handbrake Held), Y = Fall Speed. (Handbrake Released) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	FVector2D HandbrakeForwardCOMInterpRiseAndFallSpeeds = FVector2D(1.0f, 1.0f);

	/** Curve that applies a forward offset to the COM of the car based on how hard the car is currently steering (absolute value of whichever direction you are steering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	UCurveFloat* SteeringToForwardCOMOffset;

	/** Scale that multiplies the offset set in the "SteeringToFowardCOMOffset curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	float ForwardCOMOffsetScale = 1.0f;

	/** Handles interpolation to target COM forward offset. The speed here will be overwritten by the "ForwardCOMInterpRiseAndFallSpeeds" member below */
	FIIRInterpolatorFloat ForwardCOMOffsetInterpolator = FIIRInterpolatorFloat(0.f);

	/** Overrides Interpolation speeds for the "ForwardCOMOffsetInterpolator". X = Rise Speed (change caused by stick input), Y = Fall Speed. (change caused by lack of stick input) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	FVector2D ForwardCOMInterpRiseAndFallSpeeds = FVector2D(8.0f, 8.0f);

	/** Any vehicle speeds less than this velocity will disable the Forward COM offsets. Used to disable ForwardCOM offsets when reversing, as it can cause control issues */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Forward Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	float MinVelocityToDisableForwardCOMOffset = -100.0f;

	/** Curve that applies a horizontal offset to the COM of the car based on how hard the car is currently steering. The output will automatically flip depending on which direction you are steering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Horizontal Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	UCurveFloat* SteeringToHorizontalCOMOffset;

	/** Scale that multiplies the offset set in the "SteeringToHorizontalCOMOffset" curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Horizontal Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	float HorizontalCOMOffsetScale = 1.0f;

	/** Handles interpolation to target COM horizontal offset. The speed here will be overwritten by the "HorizontalCOMInterpRiseAndFallSpeeds" member below */
	FIIRInterpolatorFloat HorizontalCOMOffsetInterpolator = FIIRInterpolatorFloat(0.f);

	/** Overrides Interpolation speeds for the "HorizontalCOMOffsetInterpolator". X = Rise Speed (change caused by stick input), Y = Fall Speed. (change caused by lack of stick input)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Horizontal Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	FVector2D HorizontalCOMInterpRiseAndFallSpeeds = FVector2D(8.0f, 8.0f);

	/** What Vertical offset to apply to the center of mass when using the handbrake.  This is additive with any center of mass adjustments calculated from the "SteeringToVerticalCOMOffset" curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Vertical Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	UCurveFloat* HandbrakeVerticalCOMOffset;

	/** Member to store Vertical Handbrake Interpolation results between updates */
	float VerticalHandbrakeCurveInput = 0.0f;

	/** Scale that multiplies the offset set in the "HandbrakeVerticalCOMOffset curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Vertical Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	float HandbrakeVerticalCOMOffsetScale = 1.0f;

	/** Overrides Interpolation speeds for the "HandbrakeVerticalCOMOffsetInterpolator". X = Rise Speed (Handbrake Held), Y = Fall Speed. (Handbrake Released)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Vertical Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	FVector2D HandbrakeVerticalCOMInterpRiseAndFallSpeeds = FVector2D(1.0f, 1.0f);

	/** Curve that applies a vertical offset to the COM of the car based on how hard the car is currently steering (absolute value of whichever direction you are steering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Vertical Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	UCurveFloat* SteeringToVerticalCOMOffset;

	/** Scale that multiplies the offset set in the "SteeringToVerticalCOMOffset curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Vertical Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	float VerticalCOMOffsetScale = 1.0f;

	/** Handles interpolation to target COM vertical offset. The speed here will be overwritten by the "VerticalCOMInterpRiseAndFallSpeeds" member below*/
	FIIRInterpolatorFloat VerticalCOMOffsetInterpolator = FIIRInterpolatorFloat(0.f);

	/** Overrides Interpolation speeds for the "ForwardCOMOffsetInterpolator". X = Rise Speed (change caused by stick input), Y = Fall Speed. (change caused by lack of stick input)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Center of Mass Offset Adjustments|Vertical Offsets", meta = (EditCondition = bUseCenterOfMassAdjustments))
	FVector2D VerticalCOMInterpRiseAndFallSpeeds = FVector2D(8.0f, 8.0f);

	/** Enable the use of Wheel Friction adjustments at run time*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel Friction Adjustments")
	bool bUseWheelFrictionMultiplierAdjustments = false;

	/** Curve that adjusts the current front wheel friction multiplier depending on the magnitude of the vehicle's current steering value. This is additive to whatever the wheel's default friction multiplier is.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel Friction Adjustments", meta = (EditCondition = bUseWheelFrictionMultiplierAdjustments))
	UCurveFloat* SteeringToFrontWheelFrictionMultiplierAdjustment;

	/** Scale that multiplies the friction value set in the "SteeringToFrontWheelFriction" curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel Friction Adjustments", meta = (EditCondition = bUseWheelFrictionMultiplierAdjustments))
	float FrontWheelFrictionMultiplierAdjustmentScale = 1.0f;

	/** Enable to use Burnout adjustments at run time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burnout Adjustments")
	bool bUseBurnoutAdjustments = false;

	/** Wheel-spin/burnout behavior - reduce friction during burnout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burnout Adjustments", meta = (EditCondition = bUseBurnoutAdjustments))
	float BurnoutFrictionMultiplier = 0.8f;

	/** Wheel-spin/burnout behavior - reduce lateral forces during burnout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burnout Adjustments", meta = (EditCondition = bUseBurnoutAdjustments))
	float BurnoutLateralSlipMultiplier = 0.5f;

	/** Wheel-spin/burnout behavior - Min MPH for burnout to enable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burnout Adjustments", meta = (EditCondition = bUseBurnoutAdjustments))
	float BurnoutMinSpeedThreshold = 5.0f;

	/** Wheel-spin/burnout behavior - Max MPH for burnout effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burnout Adjustments", meta = (EditCondition = bUseBurnoutAdjustments))
	float BurnoutMaxSpeedThreshold = 25.0f;

	/////////////// End of driving feel members ///////////////

	/** Minimum Slip Angle (in degrees) that must be surpassed by the rear wheels before the vehicle is considered to be drifting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Drifting")
	float RearWheelSlipAngleDriftingMinThreshold = 10.0f;

	/** Minimum velocity that must be surpassed by the vehicle before it can be considered to be drifting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Drifting")
	float DriftingSpeedMinThreshold = 100.0f;

	/** Determines whether the CameraToPivot offset should be overridden when this vehicle is set as a camera's view target */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Mode Overrides")
	bool bOverrideCameraToPivotOffset = false;

	/** Camera to Pivot Offset used in the camera mode when bOverrideCameraToPivotOffset is true, value is additive to what is set in the camera mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera Mode Overrides", meta = (EditCondition = bOverrideCameraToPivotOffset))
	FVector CameraToPivotOffsetOverride;

	/** Determines whether the Camera LookAtPoint offset should be overridden when this vehicle is set as a camera's view target */
	UPROPERTY(EditDefaultsOnly, Category = "Camera Mode Overrides")
	bool bOverrideLookAtPointOffset = false;

	/** Camera LookAtPoint Offset used in the camera mode when bOverrideLookAtPointOffset is true, , value is additive to what is set in the camera mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera Mode Overrides", meta = (EditCondition = bOverrideLookAtPointOffset))
	FVector LookAtPointOffsetOverride;

private:

	/** Keep driving state members up to date while executing relevant driving BIEs and delegates */
	void UpdateDrivingState();

	/** Initializes the default angular damping member for later use*/
	void InitializeAngularDampingDefault();

	/** Updates dynamic angular damping adjustments when enabling bAdjustAngularDampingToSteering */
	void UpdateAngularDamping();
	
	/** Applies a downward force to combat vehicle popping when switching LODs*/
	void UpdateLODBlend(float DeltaTime);

	/** Handle drawing debug primitives related to vehicle physics sleep state */
	void UpdateDebugDraw();

	/** Update continuous rumble feedback, like speed and cornering */
	void UpdateControllerRumble();

	/** Update suspension states of all wheels and update any suspension-related rumble triggers */
	void UpdateWheelSuspensionStates(float DeltaTime);

	/** When using COM adjustments, this function initialized COM interpolators and sets the initial COM offset */
	void InitializeCenterOfMassOffsets();

	/** Updates Forward, Vertical, and Horizontal center of mass offset values */
	void UpdateCenterOfMassOffsets(float DeltaTime);

	/** Updates wheel friction multiplier based on current steering values */
	void UpdateWheelFrictionMultiplierAdjustments();

	/** When enabled, adjusts Wheel friction, traction, and slip multipliers to facilitate the burnout behavior */
	void UpdateBurnout(float DeltaTime);

	/** Checks current wheel slip angles against a custom threshold to determine if the vehicle has entered/exited drifting */
	void UpdateVehicleDriftingState();

	/** Updates the time the vehicle spends steering sharply, an arbitrary value that the driving camera uses for turning behaviors */
	void UpdateTimeSpentSharpSteering(float DeltaTime);

	/** Returns true if all wheels are currently in contact with a surface */
	bool CheckAllWheelsOnGround() const;

	/** Returns true if all wheels are currently not in contact with a surface */
	bool CheckNoWheelsOnGround() const;

	/** These functions are called when then vehicle's brakes functionally stop/start */
	void OnLiteralBrakeStop();
	void OnLiteralBrakeStart();

	/** These functions are called when then vehicle functionally starts/stops reversing */
	void OnLiteralReverseStart();
	void OnLiteralReverseStop();

	/** Binded to the enter photo mode delegate, this parks the vehicle */
	UFUNCTION()
	void OnEnterPhotomode();

	/** Binded to the enter photo mode delegate, this un-parks the vehicle */
	UFUNCTION()
	void OnExitPhotomode();

	/** When entering the vehicle, this function starts the timer the blocks the player from driving until done */
	UFUNCTION()
	void StartParkingReleaseTimer();

	/** When entering a vehicle, we need to give time for the ignition sfx to play, so we block the player from driving with these timer members */
	FTimerHandle ParkingReleaseTimerHandle;
	FTimerDelegate ParkingReleaseTimerDelegate;

	/** Stores any relevant driving information about the current vehicle */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Driving")
	FCitySampleDrivingState DrivingState;

	/** Stores the drifting state of the vehicle */
	bool bIsDrifting = false;

	/** Whether burnout behavior is currently active or not */
	bool bBurnoutActivated;

	/** Flag for gatekeeping airborne events.Used to block airborne events until the vehicle first lands(stops airborne events from firing when the vehicle spawns in) */
	bool bIsOkayToFireAirborneEvents = false;

	/** Stores the default COM offset to use as a starting point for offset adjustments */
	FVector DefaultCenterOfMassOffset;

	/** Stores the default angular damping for restoring this value when vehicle is in the air */
	float DefaultAngularDamping;

	/* Keep track of whether the vehicle is currently reversing */
	bool bIsReversing = false;

	/** Keeps track of whether the vehicle is currently breaking. The current "braking" input can become a reverse throttle, and the accelerator input acts as a brake while reversing */
	bool bIsLiterallyBreaking = false;

	/** Whether vehicle throttle is disabled, triggered via vehicle damage */
	bool bThrottleDisabled = false;

	/** Current amount of artificial steering to apply to the vehicle, triggered via vehicle damage */
	float SteeringModifier = 0.0f;

	/** Time spent past the sharp steering threshold */
	float TimeSpentSharpSteering = 0.0f;

	/** Previous Vehicle sleep state, used in UpdateLODBlend */
	bool PreviouslyAwake = true;

	/** Current LOD blend timer value */
	float BlendTimer = 0;

	/** Input Mapping and Input Actions that driveable vehicles depend on */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ThrottleAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* BrakeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* SteeringAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* HandbrakeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* LookControlsInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookDeltaAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 InputMappingPriority = 1;

	/** Methods binded to vehicle input actions to facilitate player input */
	void ThrottleActionBinding(const struct FInputActionValue& ActionValue);
	void BrakeActionBinding(const struct FInputActionValue& ActionValue);
	void SteeringActionBinding(const struct FInputActionValue& ActionValue);
	void HandbrakeActionBinding(const struct FInputActionValue& ActionValue);
	void LookActionBinding(const struct FInputActionValue& ActionValue);
	void LookDeltaActionBinding(const struct FInputActionValue& ActionValue);

};
