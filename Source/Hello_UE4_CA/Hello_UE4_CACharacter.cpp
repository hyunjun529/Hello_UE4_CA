// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Hello_UE4_CACharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

// h529
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////
// AHello_UE4_CACharacter

AHello_UE4_CACharacter::AHello_UE4_CACharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(20.f, 96.0f);
	

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHello_UE4_CACharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AHello_UE4_CACharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHello_UE4_CACharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHello_UE4_CACharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHello_UE4_CACharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AHello_UE4_CACharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AHello_UE4_CACharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHello_UE4_CACharacter::OnResetVR);
}


void AHello_UE4_CACharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHello_UE4_CACharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AHello_UE4_CACharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AHello_UE4_CACharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHello_UE4_CACharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AHello_UE4_CACharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AHello_UE4_CACharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

/** h529 **/
void AHello_UE4_CACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// get location from Actor
	float characterLocationX = this->GetActorLocation().X;
	float characterLocationY = this->GetActorLocation().Y;

	// get location from Capsule
	float characterLocationZ = GetCapsuleComponent()->GetComponentLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// make locVec
	FVector centerLoc(characterLocationX, characterLocationY, characterLocationZ);

	// get location from Mesh.Socket foot
	FVector offsetFootRLoc = GetMesh()->GetSocketLocation("foot_r");
	FVector offsetFootLLoc = GetMesh()->GetSocketLocation("foot_l");

	// make offset, location
	//offsetFootRLoc = offsetFootRLoc - centerLoc;
	offsetFootRLoc.Z = centerLoc.Z;
	//offsetFootLLoc = offsetFootLLoc - centerLoc;
	offsetFootLLoc.Z = centerLoc.Z;


	if (isDebugMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3600.0f, FColor::Blue,
			FString::Printf(TEXT("Actor : x: %f, y: %f, z: %f "),
				characterLocationX, characterLocationY, characterLocationZ)
		);

		GEngine->AddOnScreenDebugMessage(-1, 3600.0f, FColor::Green,
			FString::Printf(TEXT("FootR : x: %f, y: %f"),
				offsetFootRLoc.X, offsetFootRLoc.Y)
		);

		GEngine->AddOnScreenDebugMessage(-1, 3600.0f, FColor::Green,
			FString::Printf(TEXT("FootL : x: %f, y: %f"),
				offsetFootLLoc.X, offsetFootLLoc.Y)
		);

		DrawDebugPoint(GetWorld(), offsetFootRLoc, 10, FColor(255, 0, 0), false, 0.25f);
		DrawDebugPoint(GetWorld(), offsetFootLLoc, 10, FColor(0, 0, 255), false, 0.25f);

		float dbgLineLength = 20.f;
		DrawDebugLine(
			GetWorld(),
			FVector(offsetFootRLoc.X, offsetFootRLoc.Y, offsetFootRLoc.Z + dbgLineLength),
			FVector(offsetFootRLoc.X, offsetFootRLoc.Y, offsetFootRLoc.Z - dbgLineLength),
			FColor(255, 0, 0),
			false, -1, 0,
			2.f
		);
		DrawDebugLine(
			GetWorld(),
			FVector(offsetFootLLoc.X, offsetFootLLoc.Y, offsetFootLLoc.Z + dbgLineLength),
			FVector(offsetFootLLoc.X, offsetFootLLoc.Y, offsetFootLLoc.Z - dbgLineLength),
			FColor(0, 0, 255),
			false, -1, 0,
			2.f
		);
	}
}