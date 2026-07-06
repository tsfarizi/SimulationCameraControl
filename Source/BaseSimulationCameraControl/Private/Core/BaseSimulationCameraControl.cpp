#include "BaseSimulationCameraControl.h"
#include "BaseSimulationCameraControl_Internal.h"
#include "CameraInputComponent.h"
#include "CameraMovementComponent.h"
#include "CameraDragPanComponent.h"
#include "CameraEdgePanComponent.h"
#include "SimulationCameraController.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"

DEFINE_LOG_CATEGORY(LogSimulationCameraControl);

ABaseSimulationCameraControl::ABaseSimulationCameraControl()
{
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

	TargetArmLength = SpringArm->TargetArmLength;
	TargetRelativeRotation = SpringArm->GetRelativeRotation();
	TargetActorLocation = FVector::ZeroVector;
}

void ABaseSimulationCameraControl::BeginPlay()
{
	Super::BeginPlay();

	if (MinPitch > MaxPitch)
	{
		UE_LOG(LogSimulationCameraControl, Warning,
			TEXT("BeginPlay: MinPitch %.2f > MaxPitch %.2f. Swapping values."), MinPitch, MaxPitch);
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

	TArray<UCameraInputComponent*> InputComps;
	GetComponents(InputComps);
	for (UCameraInputComponent* Comp : InputComps)
	{
		if (UCameraMovementComponent* MoveComp = Cast<UCameraMovementComponent>(Comp))
		{
			MoveComp->OnZoomInput.AddDynamic(this, &ABaseSimulationCameraControl::Zoom);
			MoveComp->OnOrbitInput.AddDynamic(this, &ABaseSimulationCameraControl::Orbit);
			MoveComp->OnPanInput.AddDynamic(this, &ABaseSimulationCameraControl::Pan);
		}
		if (UCameraDragPanComponent* DragComp = Cast<UCameraDragPanComponent>(Comp))
		{
			DragComp->OnDragPanInput.AddDynamic(this, &ABaseSimulationCameraControl::Pan);
		}
	}

	TArray<UCameraEdgePanComponent*> EdgePanComps;
	GetComponents(EdgePanComps);
	for (UCameraEdgePanComponent* EdgeComp : EdgePanComps)
	{
		EdgeComp->OnEdgePanInput.AddDynamic(this, &ABaseSimulationCameraControl::Pan);
	}
}

void ABaseSimulationCameraControl::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime <= 0.0f || !SpringArm) return;

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

	if (bSmoothPan)
	{
		const FVector CurrentLocation = GetActorLocation();
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetActorLocation, DeltaTime, PanInterpSpeed);
		NewLocation.Z = CurrentLocation.Z;
		SetActorLocation(NewLocation);

		if (bHasCachedFocus)
		{
			LastValidHitLocation += (NewLocation - CurrentLocation);
		}
	}
	else
	{
		FVector NewLocation = TargetActorLocation;
		NewLocation.Z = GetActorLocation().Z;
		SetActorLocation(NewLocation);

		if (bHasCachedFocus)
		{
			LastValidHitLocation += (NewLocation - GetActorLocation());
		}
	}

	ApplyTerrainClamp();
}

void ABaseSimulationCameraControl::ApplyTerrainClamp()
{
	if (!bEnableGroundClamp)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TraceStart = CurrentLocation + FVector(0.0f, 0.0f, GroundTraceUpOffset);
	const FVector TraceEnd   = CurrentLocation - FVector(0.0f, 0.0f, GroundTraceDownDistance);

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CameraGroundClamp), false, this);
	QueryParams.AddIgnoredActor(this);

	if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, GroundTraceChannel, QueryParams))
	{
		return;
	}

	const float MinZ = GroundHit.ImpactPoint.Z + MinAltitudeAboveGround;
	if (CurrentLocation.Z < MinZ)
	{
		const FVector ClampedLocation(CurrentLocation.X, CurrentLocation.Y, MinZ);
		SetActorLocation(ClampedLocation);
		TargetActorLocation.Z = MinZ;
	}
}

void ABaseSimulationCameraControl::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ASimulationCameraController* PC = Cast<ASimulationCameraController>(GetController()))
		{
			PC->BindActionsToEnhancedInput(EIC);
		}
	}
}

void ABaseSimulationCameraControl::SetInputEnabled(bool bInEnabled)
{
	bInputEnabled = bInEnabled;
}