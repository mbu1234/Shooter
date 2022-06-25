// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"

// Testing SC

// Sets default values
AShooterCharacter::AShooterCharacter() :

	// Base rates for turning/lookup
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	// Turn rates when aiming/not aiming
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),
	// Mouse look sensitivity scale vectors
	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingTurnRate(0.2f),
	MouseAimingLookUpRate(0.2f),
	// Turn when aiming the weapon
	bAiming(false),
	// Camera field of view values for zooming
	CameraDefaultFOV(0.f),   // set in beginplay 
	CameraZoomedFOV(35.f),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(20.f)


{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a camera boom (spring arm component) - will pull in towards the character if there is a collision
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.f; // camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Whenever the controller moves, the spring arm is going to use the controller's rotation
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f); // The spring arm (with camera attached!) will now be 50cm to the right and 50cm up from the character
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 5.f;
	CameraBoom->CameraRotationLagSpeed = 5.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Make sure the Camera is attached to the spring arm component using the socket called SocketName (i.e. the 'Camera Boom')
	FollowCamera->bUsePawnControlRotation = false; // We don't want the camera to rotate - We only want it to follow the spring arm's rotation
	

	// We are going to allow the character to rotate along with the controller (yaw only)
	bUseControllerRotationPitch = false;   
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// We will no longer rotate character to the direction of input at the rotation rate specified (yaw only) - bOrientRotationToMovement was true, now false
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
   
	if (FollowCamera) {  // should be set up in the constructor by this time
	
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
		
	}  

}

void AShooterCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && Value != 0.0f) {

		// Find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };  // get the controller's rotation and pop it into a rotator called Rotation
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 }; // only return the yaw i.e. z-axis rotation value and store it in YawRotation

		// Get the X-Axis direction from the rotation matrix - i.e. Direction is pointing in the same direction as our controllor
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };  // get the x vector from the rotation matrix and store it in Direction

		AddMovementInput(Direction, Value); // Tell the movement component that we have to move (passing in value)
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && Value != 0.0f) {

		// Find out which way is right
		const FRotator Rotation{ Controller->GetControlRotation() }; // get the controller's rotation and pop it into a rotator called Rotation
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 }; // only return the yaw i.e. z-axis rotation value and store it in YawRotation


		// Get the X-Axis direction from the rotation matrix - i.e. Direction is pointing in the same direction as our controllor
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) }; // get the y vector from the rotation matrix and store it in Direction

		AddMovementInput(Direction, Value); // Tell the movement component that we have to move
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// Calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());  // deg/sec * sec/frame = deg/frame
}

void AShooterCharacter::LookUpRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());  //  deg/sec * sec/frame = deg/frame
}



void AShooterCharacter::Turn(float value)   // needed specifically when using the mouse rather than keyboard or controller
{
	float TurnScaleFactor;
	if (bAiming) {
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else {
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(value * TurnScaleFactor);
}



void AShooterCharacter::LookUp(float value)
{
	float LookUpScaleFactor;
	if (bAiming) {
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else {
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(value * LookUpScaleFactor);
}



void AShooterCharacter::FireWeapon()
{
	if (FireSound) 
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}


	// We want to perform a 'beam line trace' from the barrel socket transform so let's start by getting the Barrel socket.
	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket) // If we got the barrel socket, get it's transform
	{	
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		if (MuzzleFlash)  // If the particle system pointer is valid?
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);  // We can spawn it at the barrel socket
		}

		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);  // GetBeamEndLocation will perform two line traces (to take care of aim just past an object getting in the way of a target object)

		if (bBeamEnd) {  // if we have the impact point (stored in BeamEnd - In the function, we are using OutBeamLocation as as BeamEnd - BeamEnd is passed in by reference so will be updated!)
			if (ImpactParticles) {  // is the particle system pointer valid?
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd);  // spawn the particles at the impact point (BeamEnd)
			}
		}


		if (BeamParticles) {  // is the beam particles pointer valid?
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);   // spawn the beam at the socket transform (barrel of the gun)

			if (Beam) {
				Beam->SetVectorParameter(FName("Target"), BeamEnd);   // Check Target on the particle system
			}
		}
	}

		// We want to play the weapon fire montage - not we need to get the anim instance from the mesh and then from the anim instance, we can call Montage_Play()
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && HipFireMontage) {
			AnimInstance->Montage_Play(HipFireMontage);
			AnimInstance->Montage_JumpToSection(FName("StartFire"));
		}	
	
}




bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	// Get current size of the viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);   // ViewportSize is not const and will be filled in automatically with the size of the viewport (x and y values)
	}

	// Get screen space location of the crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	CrosshairLocation.Y -= 50.f;   // Refer to the HUD blueprint to double check the crosshair location


	// Get world position and direction of the crosshairs
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// Deproject the screen space location of the crosshairs to worldspace and get the world direction of the crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);

	// We were performing a line trace from the tip of our barrel (barre lsocket) - Instead we are now going to perform a line trace from the screen position of our crosshairs
	// straight out into the world


	// Was deprojection successful?
	if (bScreenToWorld) {
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };  // The world position of the crosshairs is the start point of the trace
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000 };  // We'll add the direction vector of the world position of the crosshairs * a constant - to project into the world


		OutBeamLocation = End;  // If our line trace is successfull, BeamEndPoint will be the impact point - right now, its the end point as defined above


		GetWorld()->LineTraceSingleByChannel(ScreenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);  // perform a line trace from the crosshairs, projecting out into the world
		
		if (ScreenTraceHit.bBlockingHit) {  // did we get a blocking hit? - i.e. intercepted by a mesh with a visibility collision channel
			OutBeamLocation = ScreenTraceHit.Location; // store the beam end point as the impact point

		}


		// Perform a second trace - this time from the gun barrel - we could be aiming just past an object getting in the way of what we are trying to shoot at
		FHitResult WeaponTraceHit;

		const FVector WeaponTraceStart{ MuzzleSocketLocation };   // Start is at the barrel socket
		const FVector WeaponTraceEnd{ OutBeamLocation };  // End point is the impact point
		GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);

		if (WeaponTraceHit.bBlockingHit) { // object between barrel and beam end point, if so , we need to set the beam end point to the weapson trace hit location
			OutBeamLocation = WeaponTraceHit.Location;
		}
		return true;


	}  

	return false;  // if deproject was not successful, return false - should never happen!

}

void AShooterCharacter::AimingButtonPressed()
{
	bAiming = true;

}

void AShooterCharacter::AimingButtonReleased()
{
	bAiming = false;

}



// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraInterpZoom(DeltaTime);

	// Change look sensitivity based on whether we are aiming or not
	SetLookRates();

	// Calculate the crosshair spread multiplier
	CalculateCrosshairSpread(DeltaTime);

}


void AShooterCharacter::CameraInterpZoom(float DeltaTime) {

	if (bAiming) {    // aiming button pressed? - Set camera field of view
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);  // interopolate to zoomed FOV
	}
	else {
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);  // interopolate to default FOV

	}

	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
	
}

void AShooterCharacter::SetLookRates()
{
	if (bAiming) {
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else {
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.0f, 600.f };  // Creating a 2D vector representing our character walk speed range
	FVector2D VelocityMultiplierRange{ 0.0f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor;
}


// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);  // macro - performs an assert to check the player input component is valid - will halt execution if not valid

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);

	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpRate);

	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);  // This function is just for mouse input only
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp); // This function is also just for mouse input only

	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &ACharacter::Jump);   // Let's use the parent base character class for this :)
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::FireWeapon);

	PlayerInputComponent->BindAction("AimingButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", EInputEvent::IE_Released, this, &AShooterCharacter::AimingButtonReleased);

}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

