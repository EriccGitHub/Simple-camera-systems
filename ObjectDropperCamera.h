#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"

#include "ObjectDropperCameraMode.h"

#include "ObjectDropperCamera.generated.h"


UCLASS()
class SESSIONGAME_API AObjectDropperCamera : public ACameraActor
{
	GENERATED_BODY()
	
public:

	DECLARE_MULTICAST_DELEGATE(FObjectDropperCameraEvents);

	AObjectDropperCamera();

	/** Activate the camera */
	void Activate(FObjectDropperCameraEvents* onPostActivated = nullptr);
	/** Deactivate the camera */
	void Deactivate(FObjectDropperCameraEvents onPostDeactivated = FObjectDropperCameraEvents());

	/** Tick when the object dropper is active */
	void ObjectDropper_Tick(float DeltaTime);

	/** Copies the camera settings from the given camera component */
	void CopySettings(UCameraComponent* cameraComponent);

	/** Enable or disable the camera inputs */
	void EnableCameraInputs(bool enable);
	/** Update given input state */
	void UpdateInput(const FKey& inputKey, EInputEvent inputEvent);
	void UpdateInput(const FKey& inputAxis, float axisValue);

	/** Changes the mode to None, the camera won't do anything at all */
	void SetMode_None();
	/** Changes the mode to GoTo, the camera will go to the given location / rotation */
	void SetMode_GoTo(FVector location, FRotator rotation, bool enableCollision, UObjectDropperCamera_GoTo::FOnDestinationReached* onDestinationReached = nullptr);
	/** Changes the mode to Free, the camera is not restricted to anything, it is free to move anywhere */
	void SetMode_Free();
	/** Changes the mode to Orbit, the camera is orbiting around a target actor */
	void SetMode_Orbit(AActor* orbitTarget, FVector orbitTargetOffset = FVector::ZeroVector);


private:

	/** The offset from the player's location where we should place the camera when the camera is activated */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings|Activation")
	FVector _activationOffset;
	/** The desired pitch of the camera when it is activated */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings|Activation")
	float _activationPitch;
	/** The time it takes for the camera to activate */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings|Activation")
	float _activationTime;
	/** The radius of the collision checks */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings")
	float _collisionRadius;
	/** The linear speed at which the camera will move when in free mode */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings|Free mode")
	float _freeLinearSpeed;
	/** The angular speed at which the camera will rotate when in free mode */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings|Free mode")
	float _freeAngularSpeed;
	/** The angular speed at which the camera will rotate when in orbit mode */
	UPROPERTY(EditAnywhere, Category = "Object dropper settings|Orbit mode")
	float _orbitAngularSpeed;

	
	void Deactivate_Internal();


	UObjectDropperCameraMode* _currentMode;

	AActor* _originalViewTarget;
	FVector _originalCameraLocation;
	FRotator _originalCameraRotation;
};
