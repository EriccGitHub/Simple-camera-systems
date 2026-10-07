#include "ObjectDropperCameraMode.h"

#include "Camera/CameraActor.h"


#pragma region ObjectDropperCameraMode - Base class
UObjectDropperCameraMode::UObjectDropperCameraMode()
	: _modeValue(EObjectDropperCameraModes::ODCM_Inactive)
	, _inputsEnabled(true)
	, _camera(nullptr)
{
}

UObjectDropperCameraMode::UObjectDropperCameraMode(EObjectDropperCameraModes modeValue, ACameraActor* camera)
	: _modeValue(modeValue)
	, _inputsEnabled(true)
	, _camera(camera)
{
}

void UObjectDropperCameraMode::Tick(float deltaTime)
{
	//No implementation in the base class
}

void UObjectDropperCameraMode::EnableInputs(bool enable)
{
	_inputsEnabled = enable;

	if (!_inputsEnabled)
	{
		_pressedInputKeys.Empty();
		_axisInputValues.Empty();
	}
}

void UObjectDropperCameraMode::UpdateInput(const FKey& inputKey, EInputEvent inputEvent)
{
	if (!_inputsEnabled) return;

	if (inputEvent == EInputEvent::IE_Pressed)
	{
		_pressedInputKeys.AddUnique(inputKey);
	}
	else if (inputEvent == EInputEvent::IE_Released)
	{
		_pressedInputKeys.Remove(inputKey);
	}
}

void UObjectDropperCameraMode::UpdateInput(const FKey& axisKey, float axisValue)
{
	if (!_inputsEnabled) return;

	float& oldAxisValue = _axisInputValues.FindOrAdd(axisKey, axisValue);
	oldAxisValue = axisValue;
}

bool UObjectDropperCameraMode::IsInputPressed(const FKey& inputKey)
{
	return _pressedInputKeys.Contains(inputKey);
}

float UObjectDropperCameraMode::GetAxisValue(const FKey& axisKey)
{
	const float* axisValue = _axisInputValues.Find(axisKey);

	return axisValue ? *axisValue : 0.f;
}
#pragma endregion


#pragma region ObjectDropperCamera_GoTo
UObjectDropperCamera_GoTo::UObjectDropperCamera_GoTo()
	: UObjectDropperCameraMode()
	, _collisionRadius(0.f)
	, _originLocation(FVector::ZeroVector)
	, _originRotation(FQuat::Identity)
	, _destinationLocation(FVector::ZeroVector)
	, _destinationRotation(FQuat::Identity)
	, _totalTimeToDestination(0.f)
	, _currentTimeToDestination(0.f)
{
}

UObjectDropperCamera_GoTo::UObjectDropperCamera_GoTo(ACameraActor* camera, float cameraCollisionRadius, FVector goToLocation, FQuat goToRotation, float timeToDestination)
	: UObjectDropperCameraMode(EObjectDropperCameraModes::ODCM_GoTo, camera)
	, _collisionRadius(cameraCollisionRadius)
	, _originLocation(FVector::ZeroVector)
	, _originRotation(FQuat::Identity)
	, _destinationLocation(goToLocation)
	, _destinationRotation(goToRotation)
	, _totalTimeToDestination(timeToDestination)
	, _currentTimeToDestination(0.f)
{
	if (!_camera) return;

	_originLocation = _camera->GetActorLocation();
	_originRotation = _camera->GetActorRotation().Quaternion();

	if (_collisionRadius > 0)
	{
		//Check for collision between origin and destination
		CollisionCheck();
	}
}

void UObjectDropperCamera_GoTo::Tick(float deltaTime)
{
	//Did we reach destination? If not, lerp to it
	if (_currentTimeToDestination < _totalTimeToDestination)
	{
		_currentTimeToDestination += deltaTime;
		const float alpha = _currentTimeToDestination / _totalTimeToDestination;
		const FVector newCameraLocation = FMath::Lerp(_originLocation, _destinationLocation, alpha);
		const FQuat newCameraQuat = FQuat::Slerp(_originRotation, _destinationRotation, alpha);
		_camera->SetActorLocationAndRotation(newCameraLocation, newCameraQuat);
	}
	else
	{
		OnDestinationReached.Broadcast();
	}
}

void UObjectDropperCamera_GoTo::CollisionCheck()
{
	//Make sure the desired location isn't going through something
	FHitResult outHitResult;
	FVector sweepStartOffset = _destinationLocation - _originLocation;
	sweepStartOffset = sweepStartOffset.GetSafeNormal() * _collisionRadius;

	if (_camera->GetWorld()->SweepSingleByChannel(outHitResult, _originLocation + sweepStartOffset, _destinationLocation, _destinationRotation, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeSphere(_collisionRadius)))
	{
		FVector desiredDirection = _originLocation - _destinationLocation;
		_destinationLocation = outHitResult.ImpactPoint + (desiredDirection.GetSafeNormal() * _collisionRadius);
	}
}
#pragma endregion


#pragma region ObjectDropperCamera_Free
UObjectDropperCamera_Free::UObjectDropperCamera_Free()
	: UObjectDropperCameraMode()
	, _linearSpeed(0.f)
	, _angularSpeed(0.f)
{
}

UObjectDropperCamera_Free::UObjectDropperCamera_Free(class ACameraActor* camera, float cameraLinearSpeed, float cameraAngularSpeed)
	: UObjectDropperCameraMode(EObjectDropperCameraModes::ODCM_Free, camera)
	, _linearSpeed(cameraLinearSpeed)
	, _angularSpeed(cameraAngularSpeed)
{
}

void UObjectDropperCamera_Free::Tick(float deltaTime)
{
	//Get all the input values needed
	const float forwardMovementInput = GetAxisValue(EKeys::Gamepad_LeftY);
	const float rightMovementInput = GetAxisValue(EKeys::Gamepad_LeftX);
	const float pitchRotationInput = GetAxisValue(EKeys::Gamepad_RightY);
	const float yawRotationInput = GetAxisValue(EKeys::Gamepad_RightX);
	const float pitchLimit = 89.f; //Pitch limit to avoid gimble lock or camera freaking out
	float verticalMovementInput = 0.f;
	if (IsInputPressed(EKeys::Gamepad_DPad_Down))
	{
		verticalMovementInput = -1.f;
	}
	else if (IsInputPressed(EKeys::Gamepad_DPad_Up))
	{
		verticalMovementInput = 1.f;
	}

	//Calculate new location
	const FVector movementInputRotator = FVector(forwardMovementInput, rightMovementInput, 0.F);
	const FVector worldVelocity = FVector::UpVector * verticalMovementInput + _camera->GetActorQuat().RotateVector(movementInputRotator);
	const FVector deltaLocation = worldVelocity * _linearSpeed * deltaTime;
	const FVector actorLocation = _camera->GetActorLocation();

	//Set location
	_camera->SetActorLocation(actorLocation + deltaLocation);

	//Calculate new rotation
	FRotator worldRotation = _camera->GetActorRotation();
	worldRotation.Pitch = FMath::ClampAngle(worldRotation.Pitch + pitchRotationInput * _angularSpeed * deltaTime, -pitchLimit, pitchLimit);
	worldRotation.Yaw = FRotator::ClampAxis(worldRotation.Yaw + yawRotationInput * _angularSpeed * deltaTime);

	//Set rotation
	_camera->SetActorRotation(worldRotation);
}
#pragma endregion


#pragma region ObjectDropperCamera_Orbit
UObjectDropperCamera_Orbit::UObjectDropperCamera_Orbit()
	: UObjectDropperCameraMode()
	, _speed(0.f)
	, _target(nullptr)
	, _targetOffset(FVector::ZeroVector)
	, _lastLocation(FVector::ZeroVector)
	, _orbitDistance(0.f)
	, _orbitAzimuth(0.f)
	, _orbitAltitude(0.f)
{
}

UObjectDropperCamera_Orbit::UObjectDropperCamera_Orbit(ACameraActor* camera, AActor* orbitTarget, float cameraSpeed, FVector targetOffset)
	: UObjectDropperCameraMode(EObjectDropperCameraModes::ODCM_Orbit, camera)
	, _speed(cameraSpeed)
	, _target(orbitTarget)
	, _targetOffset(targetOffset)
	, _lastLocation(FVector::ZeroVector)
	, _orbitDistance(0.f)
	, _orbitAzimuth(0.f)
	, _orbitAltitude(0.f)
{
	if (!_target || !_camera) return;

	//Calculate some starting orbit stuff
	const FVector relativeTargetLocation = _target->GetActorLocation() - _camera->GetActorLocation() + targetOffset;
	const FRotator cameraOrientation = (-relativeTargetLocation).Rotation();

	_orbitDistance = (relativeTargetLocation).Size();
	_orbitAzimuth = cameraOrientation.Yaw;
	_orbitAltitude = cameraOrientation.Pitch;
}

void UObjectDropperCamera_Orbit::Tick(float deltaTime)
{
	if (!_target) return;

	//Get all the input values needed
	const float pitchRotationInput = GetAxisValue(EKeys::Gamepad_RightY);
	const float yawRotationInput = GetAxisValue(EKeys::Gamepad_RightX);
	const float pitchLimit = 89.f; //Pitch limit to avoid gimble lock

	//Calculate orbit delta
	const float deltaFactor = _speed * deltaTime;
	_orbitAzimuth = FRotator::ClampAxis(_orbitAzimuth + yawRotationInput * deltaFactor);
	_orbitAltitude = FMath::ClampAngle(_orbitAltitude - pitchRotationInput * deltaFactor, -pitchLimit, pitchLimit);

	//Calculate new location
	const FRotator cameraOrientation{ _orbitAltitude, _orbitAzimuth, 0.F };
	const FVector cameraPositionFromTarget = cameraOrientation.Vector() * _orbitDistance;
	const FVector targetLocation = _target->GetActorLocation() + _targetOffset;

	//Set location / rotation
	FVector newCameraLocation = targetLocation + cameraPositionFromTarget;
	FRotator newCameraRotation = (targetLocation - newCameraLocation).Rotation();

	_camera->SetActorLocationAndRotation(newCameraLocation, newCameraRotation);
}
#pragma endregion