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

	BaseTurnRate(45.f),
	BaseLookUpRate(45.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a camera boom (spring arm component) - will pull in towards the character if there is a collision
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f; // camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Whenever the controller moves, the spring arm is going to use the controller's rotation
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 50.f); // The spring arm (with camera attached!) will now be 50cm to the right and 50cm up from the character
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

void AShooterCharacter::FireWeapon()
{
	if (FireSound) 
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket) 
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		if (MuzzleFlash) 
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}


		// We were performing a line trace from the tip of our barrel (barrelsocket) - Instead we are now going to perform a line trace from the screen position of our crosshairs
		// straight out into the world


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

		// Was deprojection successful?
		if (bScreenToWorld) {
			FHitResult ScreenTraceHit;
			const FVector Start{ CrosshairWorldPosition };  // The world position of the crosshairs is the start point of the trace
			const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000 };  // We'll add the direction vector of the world position of the crosshairs * a constant - to project into the world

			FVector BeamEndPoint{ End };  // If our line trace is successfull, BeamEndPoint will be the impact point - right now, its the end point as defined above


			GetWorld()->LineTraceSingleByChannel(ScreenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);  // perform the trace

			if (ScreenTraceHit.bBlockingHit) {  // did we get a blocking hit - i.e. intercepted by a mesh with a visibility collision channel
				BeamEndPoint = ScreenTraceHit.Location; // store the beam end point as the impact point

			}


			// Perform a second trace - this time from the gun barrel
			FHitResult WeaponTraceHit;

			const FVector WeaponTraceStart{ SocketTransform.GetLocation() }; 
			const FVector WeaponTraceEnd{ BeamEndPoint };
			GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);

			if (WeaponTraceHit.bBlockingHit) { // object between barrel and beam end point, if so , we need to set the beam end point to the weapson trace hit location
				BeamEndPoint = WeaponTraceHit.Location;
			}


			// Make sure to spawn impact particles after the second blocking hit - i.e.  after beam end point (i.e. if an object gets in the way of another object)

			if (ImpactParticles) {  // is the particle system pointer valid?
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEndPoint);  // spawn the particles at the impact point (BeamEndPoint)
			}

			if (BeamParticles) {  // is the beam particles pointer valid?
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);   // spawn the beam at the socket transform (barrel of the gun)

				if (Beam) {
					Beam->SetVectorParameter(FName("Target"), BeamEndPoint);   // Check Target on the particle system
				}				
			}
		}
		

		/**
		FHitResult FireHit;
		const FVector Start{ SocketTransform.GetLocation() };
		const FQuat Rotation{ SocketTransform.GetRotation() };   // Remeber that calling GetRotation on a SocketTransform returns a Quaterion
		const FVector RotationAxis{ Rotation.GetAxisX() };  // Get the X axis of the rotation
		const FVector End{ Start + RotationAxis * 50'000.f };  // An ' can be used to make larger numbers more readible - The compiler doesn't care

		FVector BeamEndPoint{ End };

		GetWorld()->LineTraceSingleByChannel(FireHit, Start, End, ECollisionChannel::ECC_Visibility);

		if (FireHit.bBlockingHit) {

			BeamEndPoint = FireHit.Location;

			if (ImpactParticles) {
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.Location);
			}

			if (BeamParticles) {
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);

				if (Beam) {
					Beam->SetVectorParameter(FName("Target"), BeamEndPoint);   // Check Target on the particle system
				}
			}
			
		}

		*/


		// We want to play the weapon fire montage - not we need to get the anim instance from the mesh and then from the anim instance, we can call Montage_Play()
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && HipFireMontage) {
			AnimInstance->Montage_Play(HipFireMontage);
			AnimInstance->Montage_JumpToSection(FName("StartFire"));
		}

	}
	
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);  // Let's use the parent pawn class for this :)
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &ACharacter::Jump);   // Let's use the parent base character class for this :)
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::FireWeapon);

}

