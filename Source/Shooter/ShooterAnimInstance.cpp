// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}


void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime) 
{
	if (ShooterCharacter == nullptr) {
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
		
	}


	if (ShooterCharacter) {

		// Get the speed of the character (i.e. magnitude/size of the velocity vector)
		FVector Velocity{ ShooterCharacter->GetVelocity() };
		Velocity.Z = 0;  // zeroring out any vertical movement - we only want lateral velocity
		Speed = Velocity.Size(); // now get it's magnitude


		// Is the character in the air?
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		// Is the character accelerating?
		if (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 )    // Remember, GetCharacterMovement()->GetCurrentAcceleration() returns a velocity vector
		{
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}
	}


}