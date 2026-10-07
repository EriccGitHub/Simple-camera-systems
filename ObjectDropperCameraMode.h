#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ObjectDropperCameraMode.generated.h"


UENUM(BlueprintType)
enum class EObjectDropperCameraModes : uint8
{
	ODCM_Inactive,
	ODCM_GoTo,
	ODCM_Free,
	ODCM_Orbit,
};


class SESSIONGAME_API UObjectDropperCameraMode
{

private: 

	/** The mode enum value */
	EObjectDropperCameraModes _modeValue;

	/** Whether or not the inputs are currently enabled. */
	bool _inputsEnabled;
	/** Array containing all the keys currently pressed */
	TArray<FKey> _pressedInputKeys;
	/** Map containing all the axis keys in used, along with their axis values */
	TMap<FKey, float> _axisInputValues;

protected:

	/** The camera used by this mode */
	ACameraActor* _camera;

	bool IsInputPressed(const FKey& inputKey);
	float GetAxisValue(const FKey& axisKey);

public:

	UObjectDropperCameraMode();
	UObjectDropperCameraMode(EObjectDropperCameraModes modeValue, ACameraActor* camera);
	virtual ~UObjectDropperCameraMode() {}

	/** Returns the mode enum value */
	EObjectDropperCameraModes GetModeValue() const { return _modeValue; }
	
	virtual void Tick(float deltaTime);

	/** Enable or disable all camera inputs */
	void EnableInputs(bool enable);
	/** Update given input state */
	void UpdateInput(const FKey& inputKey, EInputEvent inputEvent);
	void UpdateInput(const FKey& axisKey, float axisValue);
};

class SESSIONGAME_API UObjectDropperCamera_GoTo : public UObjectDropperCameraMode
{

public:

	DECLARE_MULTICAST_DELEGATE(FOnDestinationReached)
	/** Called when the given destination has been reached */
	FOnDestinationReached OnDestinationReached;

	UObjectDropperCamera_GoTo();
	UObjectDropperCamera_GoTo(class ACameraActor* camera, float cameraCollisionRadius, FVector goToLocation, FQuat goToRotation, float timeToDestination);
	virtual ~UObjectDropperCamera_GoTo() {}

	virtual void Tick(float deltaTime) override;

	void CollisionCheck();

private:

	float _collisionRadius;

	FVector _originLocation;
	FQuat _originRotation;
	FVector _destinationLocation;
	FQuat _destinationRotation;

	float _totalTimeToDestination;
	float _currentTimeToDestination;
};

class SESSIONGAME_API UObjectDropperCamera_Free : public UObjectDropperCameraMode
{

public:

	UObjectDropperCamera_Free();
	UObjectDropperCamera_Free(class ACameraActor* camera, float cameraLinearSpeed, float cameraAngularSpeed);
	virtual ~UObjectDropperCamera_Free() {}

	virtual void Tick(float delatTime) override;

private:

	float _linearSpeed;
	float _angularSpeed;
};

class SESSIONGAME_API UObjectDropperCamera_Orbit : public UObjectDropperCameraMode
{

public:

	UObjectDropperCamera_Orbit();
	UObjectDropperCamera_Orbit(class ACameraActor* camera, AActor* orbitTarget, float cameraSpeed, FVector targetOffset);
	virtual ~UObjectDropperCamera_Orbit() {}

	virtual void Tick(float delatTime) override;

private:

	float _speed;
	AActor* _target;

	FVector _targetOffset;
	FVector _lastLocation;
	float _orbitDistance;
	float _orbitAzimuth;
	float _orbitAltitude;
};