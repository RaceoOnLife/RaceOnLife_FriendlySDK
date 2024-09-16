

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EditorPawn.generated.h"

UCLASS()
class INGAMELEVELEDITOR_API AEditorPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AEditorPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
