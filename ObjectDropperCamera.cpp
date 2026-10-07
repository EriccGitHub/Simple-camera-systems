#include "ObjectDropperCamera.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"


AObjectDropperCamera::AObjectDropperCamera()
	: _activationOffset(FVector(-400, 0, 400))
	, _activationPitch(-30.f)
	, _activationTime(0.5)
	, _collisionRadius(30.f)
	, _freeLinearSpeed(1500.f)
	, _freeAngularSpeed(150.f)
	, _orbitAngularSpeed(150.f)
	, _originalViewTarget(nullptr)
	, _originalCameraLocation(FVector::ZeroVector)
	, _originalCameraRotation(FRotator::ZeroRotator)
{
	//Set current mode to an empty mode
	_currentMode = new UObjectDropperCameraMode();
}

void AObjectDropperCamera::ObjectDropper_Tick(float DeltaTime)
{
	//Tick the current mode
	if (_currentMode) _currentMode->Tick(DeltaTime);
}

void AObjectDropperCamera::Activate(FObjectDropperCameraEvents* onPostActivated/* = nullptr*/)
{
	APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!cameraManager) return;

	APawn* pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!pawn) return;

	//Original camera variables
	_originalCameraLocation = cameraManager->GetCameraLocation();
	_originalCameraRotation = cameraManager->GetCameraRotation();
	FVector originalCameraForward = FRotationMatrix(_originalCameraRotation).GetScaledAxis(EAxis::X);
	FVector originalCameraRight = FRotationMatrix(_originalCameraRotation).GetScaledAxis(EAxis::Y);

	//Offset to apply to the camera location
	FVector offsetForward = originalCameraForward;
	offsetForward.Z = 0;
	offsetForward.Normalize();

	FVector locationOffset = originalCameraForward * _activationOffset.X;
	locationOffset += originalCameraRight * _activationOffset.Y;
	locationOffset += FVector::UpVector * _activationOffset.Z;

	//Desired new location of the camera
	FVector desiredLocation = pawn->GetActorLocation() + locationOffset;

	//Desired new rotation of the camera
	FRotator desiredRotation = _originalCameraRotation;
	desiredRotation.Pitch = _activationPitch;
	desiredRotation.Roll = 0.f;
		
	//Set the typical stuff
	if (APlayerController* playerController = Cast<APlayerController>(pawn->GetController()))
	{
		_originalViewTarget = playerController->GetViewTarget();
		playerController->SetViewTarget(this);
	}

	SetActorLocationAndRotation(_originalCameraLocation, _originalCameraRotation);

	SetMode_GoTo(desiredLocation, desiredRotation, true, onPostActivated);
}

void AObjectDropperCamera::Deactivate(FObjectDropperCameraEvents onPostDeactivated/* = FObjectDropperCameraEvents()*/)
{
	auto OnDeactivateInternal = [this, onPostDeactivated] ()
		{
			Deactivate_Internal();

			// We are now fully done with the camera transition, notify caller if necessary
			if (onPostDeactivated.IsBound())
			{
				onPostDeactivated.Broadcast();
			}
		};

	// We'll let the camera do its thing (zoom back in on the player) then we'll fully deactivate everything
	FObjectDropperCameraEvents internalDeactivation;
	internalDeactivation.AddLambda(OnDeactivateInternal);

	SetMode_GoTo(_originalCameraLocation, _originalCameraRotation, false, &internalDeactivation);
}

void AObjectDropperCamera::CopySettings(UCameraComponent* cameraComponent)
{
	if (!cameraComponent) return;

	GetCameraComponent()->SetConstraintAspectRatio(cameraComponent->bConstrainAspectRatio);
	GetCameraComponent()->SetAspectRatio(cameraComponent->AspectRatio);
	GetCameraComponent()->SetFieldOfView(cameraComponent->FieldOfView);
}

void AObjectDropperCamera::EnableCameraInputs(bool enable)
{
	if (_currentMode)
	{
		_currentMode->EnableInputs(enable);
	}
}

void AObjectDropperCamera::UpdateInput(const FKey& inputKey, EInputEvent inputEvent)
{
	if (_currentMode)
	{
		_currentMode->UpdateInput(inputKey, inputEvent);
	}
}

void AObjectDropperCamera::UpdateInput(const FKey& inputAxis, float axisValue)
{
	if (_currentMode)
	{
		_currentMode->UpdateInput(inputAxis, axisValue);
	}
}

void AObjectDropperCamera::SetMode_None()
{
	_currentMode = new UObjectDropperCameraMode();
}

void AObjectDropperCamera::SetMode_GoTo(FVector location, FRotator rotation, bool enableCollision, UObjectDropperCamera_GoTo::FOnDestinationReached* onDestinationReached/* = nullptr*/)
{
	const float collisionRadius = enableCollision ? _collisionRadius : 0.f;
	UObjectDropperCamera_GoTo* goToMode = new UObjectDropperCamera_GoTo(this, collisionRadius, location, rotation.Quaternion(), _activationTime);
	if (onDestinationReached) goToMode->OnDestinationReached = *onDestinationReached;

	_currentMode = goToMode;
}

void AObjectDropperCamera::SetMode_Free()
{
	_currentMode = new UObjectDropperCamera_Free(this, _freeLinearSpeed, _freeAngularSpeed);
}

void AObjectDropperCamera::SetMode_Orbit(AActor* orbitTarget, FVector orbitTargetOffset/* = FVector::ZeroVector*/)
{
	_currentMode = new UObjectDropperCamera_Orbit(this, orbitTarget, _orbitAngularSpeed, orbitTargetOffset);
}

void AObjectDropperCamera::Deactivate_Internal()
{
	APawn* pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!pawn) return;

	//Return to the original view target
	if (APlayerController* playerController = Cast<APlayerController>(pawn->GetController()))
	{
		playerController->SetViewTarget(_originalViewTarget);
	}
}