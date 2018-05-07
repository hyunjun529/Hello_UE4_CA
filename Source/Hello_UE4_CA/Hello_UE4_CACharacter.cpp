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
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////
// AHello_UE4_CACharacter

AHello_UE4_CACharacter::AHello_UE4_CACharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(10.f, 96.0f);
	

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

	/** GOD FORGIVE ME **/
	footPrintR.X = 0.f;
	footPrintR.Y = 0.f;
	footPrintR.Z = 0.f;

	footPrintL.X = 0.f;
	footPrintL.Y = 0.f;
	footPrintL.Z = 0.f;

	pelvis = 0;
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


	float lineLength = 40.f;


	/** get World Location **/
	// get location from Actor
	float characterLocationX = this->GetActorLocation().X;
	float characterLocationY = this->GetActorLocation().Y;

	// get location from Capsule
	float characterLocationZ = GetCapsuleComponent()->GetComponentLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// make locVec
	FVector centerLoc(characterLocationX, characterLocationY, characterLocationZ);


	/** get Socket Local Location **/
	// get location from Mesh.Socket foot
	FVector d3FootRLoc = GetMesh()->GetSocketLocation("foot_r");
	FVector d3FootLLoc = GetMesh()->GetSocketLocation("foot_l");

	// make 2D, Real(3D)
	FVector d2FootRLoc = d3FootRLoc;
	FVector d2FootLLoc = d3FootLLoc;

	d2FootRLoc.Z = centerLoc.Z;
	d2FootLLoc.Z = centerLoc.Z;

	
	/** LineTrace, BreakHit : Right **/
	FCollisionQueryParams RV_TraceParams_R = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams_R.bTraceComplex = true;
	RV_TraceParams_R.bTraceAsyncScene = true;
	RV_TraceParams_R.bReturnPhysicalMaterial = false;

	//Re-initialize hit info
	FHitResult RV_Hit_R(ForceInit);

	//call GetWorld() from within an actor extending class
	GetWorld()->LineTraceSingleByChannel(
		RV_Hit_R,        //result
		FVector(d2FootRLoc.X, d2FootRLoc.Y, d2FootRLoc.Z + lineLength),    //start
		FVector(d2FootRLoc.X, d2FootRLoc.Y, d2FootRLoc.Z - lineLength), //end
		ECC_Pawn, //collision channel
		RV_TraceParams_R
	);


	/** LineTrace, BreakHit : Left **/
	FCollisionQueryParams RV_TraceParams_L = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams_L.bTraceComplex = true;
	RV_TraceParams_L.bTraceAsyncScene = true;
	RV_TraceParams_L.bReturnPhysicalMaterial = false;

	FHitResult RV_Hit_L(ForceInit);

	GetWorld()->LineTraceSingleByChannel(
		RV_Hit_L,        //result
		FVector(d2FootLLoc.X, d2FootLLoc.Y, d2FootLLoc.Z + lineLength),    //start
		FVector(d2FootLLoc.X, d2FootLLoc.Y, d2FootLLoc.Z - lineLength), //end
		ECC_Pawn, //collision channel
		RV_TraceParams_L
	);


	// update foot print
	FVector toLocalHitPoint_R = FVector(0, 0, 0);
	FVector toLocalHitPoint_L = FVector(0, 0, 0);

	if (onFootPrint)
	{
		float localFootZ_R = RV_Hit_R.ImpactPoint.Z - d2FootRLoc.Z;
		float localFootZ_L = RV_Hit_L.ImpactPoint.Z - d2FootLLoc.Z;

		if (RV_Hit_R.bBlockingHit) { toLocalHitPoint_R = FVector(0, 0, localFootZ_R); }
		if (RV_Hit_L.bBlockingHit) { toLocalHitPoint_L = FVector(0, 0, localFootZ_L); }

		if (RV_Hit_R.bBlockingHit && RV_Hit_L.bBlockingHit)
		{
			// 오른발이 위
			if (localFootZ_R > localFootZ_L)
			{
				pelvis = localFootZ_L;
				localFootZ_L = 0;
			}
			// 왼발이 위
			else
			{
				pelvis = localFootZ_R;
				localFootZ_R = 0;
			}
		}
	}

	footPrintR.X = toLocalHitPoint_R.X;
	footPrintR.Y = toLocalHitPoint_R.Y;
	footPrintR.Z = toLocalHitPoint_R.Z;

	footPrintL.X = toLocalHitPoint_L.X;
	footPrintL.Y = toLocalHitPoint_L.Y;
	footPrintL.Z = toLocalHitPoint_L.Z;


	if (onDebugText)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Purple,
		//	FString::Printf(TEXT("is stick to? [ (L : %s), (R : %s) ]"),
		//	(RV_Hit_L.bBlockingHit) ? ("Y") : ("N"), (RV_Hit_R.bBlockingHit) ? ("Y") : ("N"))
		//	);

		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red,
			FString::Printf(TEXT("ImpactPoint R ? x : %f, y : %f, z : %f"),
				footPrintR.X, footPrintR.Y, footPrintR.Z)
		);

		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue,
			FString::Printf(TEXT("ImpactPoint L ? x : %f, y : %f, z : %f"),
				footPrintL.X, footPrintL.Y, footPrintL.Z)
		);

		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Purple,
			FString::Printf(TEXT("pelvis? %f"),
				pelvis)
		);
	}

	if (onDebugPoint)
	{
		DrawDebugPoint(GetWorld(), d3FootRLoc, 10, FColor(255, 0, 0), false, 0.25f);
		DrawDebugPoint(GetWorld(), d3FootLLoc, 10, FColor(0, 0, 255), false, 0.25f);
		DrawDebugPoint(GetWorld(), this->GetActorLocation(), 10, FColor(255, 0, 255), false, 0.25f);
	}

	if (onDebugLine)
	{
		float dbgLineLength = lineLength;

		DrawDebugLine(
			GetWorld(),
			FVector(d2FootRLoc.X, d2FootRLoc.Y, d2FootRLoc.Z + dbgLineLength),
			FVector(d2FootRLoc.X, d2FootRLoc.Y, d2FootRLoc.Z - dbgLineLength),
			FColor(255, 0, 0),
			false, -1, 0,
			2.f
		);
		DrawDebugLine(
			GetWorld(),
			FVector(d2FootLLoc.X, d2FootLLoc.Y, d2FootLLoc.Z + dbgLineLength),
			FVector(d2FootLLoc.X, d2FootLLoc.Y, d2FootLLoc.Z - dbgLineLength),
			FColor(0, 0, 255),
			false, -1, 0,
			2.f
		);
	}
}


FVector AHello_UE4_CACharacter::getFootPrintR()
{
	return footPrintR;
}

FVector AHello_UE4_CACharacter::getFootPrintL()
{
	return footPrintL;
}

float AHello_UE4_CACharacter::getPelvis()
{
	return pelvis;
}