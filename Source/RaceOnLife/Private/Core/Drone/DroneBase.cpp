#include "Core/Drone/DroneBase.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

ADroneBase::ADroneBase()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementSpeed = 350.0f;
	LiftSpeed = 300.0f;
	TiltAngle = 10.0f;

	DroneCharge = 100.0f;

	UFloatingPawnMovement* FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	FloatingPawnMovement->MaxSpeed = MovementSpeed;
}

void ADroneBase::BeginPlay()
{
	Super::BeginPlay();
}

void ADroneBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CurrentVelocity.IsZero())
	{
		FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
		SetActorLocation(NewLocation);

		FRotator NewRotation = GetActorRotation();
		NewRotation.Pitch = FMath::Clamp(CurrentTilt.Pitch, -TiltAngle, TiltAngle);
		NewRotation.Roll = FMath::Clamp(CurrentTilt.Roll, -TiltAngle, TiltAngle);
		SetActorRotation(NewRotation);
	}

	ReduceDroneCharge();
}

void ADroneBase::Possessed(AController NewController*)
{
	Super::Possessed(NewController);

	if (WidgetClass && NewController->IsA(APlayerController::StaticClass()))
	{
		CreatedWidget = CreateWidget<UUserWidget>(Cast<APlayerController>(NewController), WidgetClass);
		if (CreatedWidget)
		{
			CreatedWidget->AddToViewport;
		}
	}
}

void ADroneBase::UnPossessed()
{
	Super::UnPossessed();

	if (CreatedWidget)
	{
		CreatedWidget->RemoveFromParent();
		CreatedWidget = nullptr;
	}
}

void ADroneBase::ReduceDroneCharge()
{
	if (DroneCharge >= 0.f)
	{
		DroneCharge = FMath::Clamp(DroneCharge - 0.00025f, 0, 100);
	}
	else {
		if (Destroy())
		{
			OnDestroyed.Broadcast(this);
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("Failed to destroy drone."));
		}
	}
}

void ADroneBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ADroneBase::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADroneBase::MoveRight);
	PlayerInputComponent->BindAxis("MoveUpward", this, &ADroneBase::MoveUpward);
}

void ADroneBase::MoveForward(float Value)
{
	CurrentVelocity.X = Value * MovementSpeed;

	CurrentTilt.Pitch = Value * TiltAngle;
}

void ADroneBase::MoveRight(float Value)
{
	CurrentVelocity.Y = Value * MovementSpeed;

	CurrentTilt.Roll = -Value * TiltAngle;
}

void ADroneBase::MoveUpward(float Value)
{
	CurrentVelocity.Z = Value * LiftSpeed;
}

float ADroneBase::GetDroneCharge()
{
	return DroneCharge;
}
