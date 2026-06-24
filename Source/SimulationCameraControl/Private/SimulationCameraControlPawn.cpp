#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"
#include "CameraInputBehavior.h"
#include "CameraMovementBehavior.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

DEFINE_LOG_CATEGORY(LogSimulationCameraControl);

ASimulationCameraControl::ASimulationCameraControl()
{
	// Enable Tick for smooth interpolation
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->TargetArmLength = 1200.0f;
	SpringArm->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw  = false;
	bUseControllerRotationRoll = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Initialize target values for smoothing
	TargetArmLength = SpringArm->TargetArmLength;
	TargetRelativeRotation = SpringArm->GetRelativeRotation();

	// Default input behavior: provides the 5 standard camera-movement actions
	// (Zoom, Orbit, Orbit_Modifier, Pan, Pan_Modifier) so the pawn works out of
	// the box. Designers can remove or augment this in the Details panel; add
	// additional behaviors (boost, focus, free-look, etc.) the same way.
	UCameraMovementBehavior* DefaultMovement = CreateDefaultSubobject<UCameraMovementBehavior>(TEXT("DefaultCameraMovement"));
	Behaviors.Add(DefaultMovement);
}

void ASimulationCameraControl::BeginPlay()
{
	Super::BeginPlay();

	if (MinPitch > MaxPitch)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("BeginPlay: MinPitch %.2f > MaxPitch %.2f. Swapping values to preserve clamp."),
			MinPitch, MaxPitch);
		Swap(MinPitch, MaxPitch);
	}

	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength, MinArmLength, MaxArmLength);
		TargetArmLength = SpringArm->TargetArmLength;
		TargetRelativeRotation = SpringArm->GetRelativeRotation();
		TargetActorLocation = GetActorLocation();
		bTargetsInitialized = true;
	}

	InitializeInputMapping();
}

void ASimulationCameraControl::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime <= 0.0f || !SpringArm)
	{
		return;
	}

	// Smooth zoom interpolation (arm length)
	if (bSmoothZoom)
	{
		const float CurrentArmLength = SpringArm->TargetArmLength;
		const float NewArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
		SpringArm->TargetArmLength = FMath::Clamp(NewArmLength, MinArmLength, MaxArmLength);
	}
	else
	{
		SpringArm->TargetArmLength = FMath::Clamp(TargetArmLength, MinArmLength, MaxArmLength);
	}

	// Smooth orbit interpolation (rotation)
	if (bSmoothOrbit)
	{
		const FRotator CurrentRotation = SpringArm->GetRelativeRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRelativeRotation, DeltaTime, OrbitInterpSpeed);
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, MinPitch, MaxPitch);
		NewRotation.Roll = 0.0f;
		SpringArm->SetRelativeRotation(NewRotation);
	}
	else
	{
		FRotator ClampedRotation = TargetRelativeRotation;
		ClampedRotation.Pitch = FMath::Clamp(ClampedRotation.Pitch, MinPitch, MaxPitch);
		ClampedRotation.Roll = 0.0f;
		SpringArm->SetRelativeRotation(ClampedRotation);
	}

	// Smooth pan interpolation (location)
	if (bSmoothPan)
	{
		const FVector CurrentLocation = GetActorLocation();
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetActorLocation, DeltaTime, PanInterpSpeed);
		NewLocation.Z = CurrentLocation.Z; // Maintain Z height
		SetActorLocation(NewLocation);

		if (bHasCachedFocus)
		{
			LastValidHitLocation += (NewLocation - CurrentLocation);
		}
	}
	else
	{
		FVector NewLocation = TargetActorLocation;
		NewLocation.Z = GetActorLocation().Z; // Maintain Z height
		SetActorLocation(NewLocation);

		if (bHasCachedFocus)
		{
			LastValidHitLocation += (NewLocation - GetActorLocation());
		}
	}
}

void ASimulationCameraControl::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeInputMapping();
}

void ASimulationCameraControl::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeInputMapping();
}

void ASimulationCameraControl::SetInputEnabled(bool bInEnabled)
{
	const bool bOldState = bInputEnabled;
	bInputEnabled = bInEnabled;
	UE_LOG(LogSimulationCameraControl, Verbose, TEXT("SetInputEnabled: %s -> %s"),
		bOldState ? TEXT("true") : TEXT("false"),
		bInputEnabled ? TEXT("true") : TEXT("false"));
}

void ASimulationCameraControl::SetInputMappingPriority(int32 InPriority)
{
	InputMappingPriority = FMath::Max(0, InPriority);
	InitializeInputMapping();
}

void ASimulationCameraControl::SetInputBindingsOverride(UCameraInputBindings* InBindings)
{
	if (InputBindingsOverride == InBindings)
	{
		return;
	}

	InputBindingsOverride = InBindings;

	// Force a rebuild on next setup. The existing ActiveInputMapping is left
	// in place until the next PossessedBy/PawnClientRestart cycle so the
	// current session's input bindings aren't yanked mid-frame.
	ActiveInputMapping = nullptr;
}
