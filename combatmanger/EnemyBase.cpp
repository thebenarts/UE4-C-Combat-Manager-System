// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"

//#include <NvParameterized.h>
//#include <ThirdParty/CryptoPP/5.6.5/include/rsa.h>

#include "Kismet/GamePlayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"

#include "Animation/AnimMontage.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "../Player/CPP_CharacterBase.h"
#include "../Weapons/CPP_WeaponBase.h"
#include "../Weapons/CPP_ProjectileBase.h"

#include "CombatManager.h"
#include "WaveManager.h"

#include "GameFramework/ProjectileMovementComponent.h"


#include "DrawDebugHelpers.h"
#include "EnemyProjectileBase.h"
#include "EnemySpawner.h"

// Sets default values
AEnemyBase::AEnemyBase() : DoOnce(true),
StaggerState(0),
LocIndex(-1),
bStaggered(false),
staggerTime(1.0f),
deathCleanUpTime(30.f),
spawnTime(2.5f),
dissolveTime(2.5f),
bAlive(true),
CombatManager(NULL),
bToken(false),
bInAttackRange(false),
AttackR(TEXT("MeleeAttack"))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionProfileName(FName("Enemy"));
	GetCapsuleComponent()->SetCollisionProfileName(FName("Enemy"));

	MeleeAttackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("MeleeAttackCollision"));
	MeleeAttackCollision->SetupAttachment(GetMesh(), FName("MeleeSocket"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	SetEnemyData();
	
	PlayerRef = GetPlayer();
	ActivateController();

	// Bind the overlap for weaponCollisionBox
	MeleeAttackCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBase::OnMeleeAttackOverlap);

	// Set collision presets for weapon boxes
	MeleeAttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeAttackCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	MeleeAttackCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	MeleeAttackCollision->SetCollisionResponseToChannel(ECC_Player, ECollisionResponse::ECR_Overlap);


	
	// Transform Patrol Point from Local to wordspace and assign that value to WorldPatrolPoint
	const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
	const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);

	if (CombatManager)
		UE_LOG(LogTemp, Warning, TEXT("Has Combat Manger"));

}

void AEnemyBase::InitiateSpawn()
{
	//We are now using the dissolve material, thus it will be hidden from the start
	SetActorEnableCollision(false);
	
	if(EnemyController)
	{
		EnemyController->UnPossess();
	}
	else
	{
		EnemyController = Cast<AEnemyController>(GetController());
		if(EnemyController)
		{
			EnemyController->UnPossess();
		}
	}

	PlaySpawningFX();
	
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer,this,&AEnemyBase::Spawn,spawnTime,false);
}

void AEnemyBase::Spawn()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	SetActorEnableCollision(true);
	
	if(EnemyController && bAlive)
	{
		EnemyController->Possess(this);
		ActivateController();
	}
	GetMesh()->bComponentUseFixedSkelBounds=false;

	SetActorHiddenInGame(false);
	
}

void AEnemyBase::PlaySpawningFX()
{
	UNiagaraComponent* NC_Appear = nullptr;
	
	if(EnemyRow.NS_Spawning)
	{
		GetMesh()->bComponentUseFixedSkelBounds=true;
		NC_Appear = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),EnemyRow.NS_Spawning,GetActorLocation());
		UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(NC_Appear,FString("SkeletalMesh"),GetMesh());
		NC_Appear->SetNiagaraVariableLinearColor("Color", P_Color * 10.f);
	}
	
	if(EnemyRow.SFX_Spawning)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),EnemyRow.SFX_Spawning,GetActorLocation());

	// this calls a Blueprint function. Implement it per enemy type
	AppearFX(NC_Appear);
}

void AEnemyBase::ActivateController()
{
	
	EnemyController = Cast<AEnemyController>(GetController());
	if (EnemyController && bAlive)
	{
		EnemyController->RunBehaviorTree(BehaviorTree);

		// Set the FireRate var in BlackBoard
		EnemyController->GetBlackboardComponent()->SetValueAsFloat(FName("FireRate"), EnemyRow.FiraRate);


		if (PlayerRef)
		{
			EnemyController->SetFocus(PlayerRef);
			EnemyController->GetBlackboardComponent()->SetValueAsObject(FName("Target"), PlayerRef);
		}
	}
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyBase::InTestTube()
{
	//TODO Disable controller
	//Play corresponding animation
	//Add a falling state to the animation state machine.

	SetActorEnableCollision(false);
	
	if(EnemyController)
	{
		EnemyController->UnPossess();
	}
	else
	{
		EnemyController = Cast<AEnemyController>(GetController());
		if(EnemyController)
		{
			EnemyController->UnPossess();
		}
	}
	
}

void AEnemyBase::Wake()
{
	Spawn();
}

void AEnemyBase::SetEnemyData()
{
	if (EnemyDataTableObject) // Check if we have a datatable assigned to this entity in the editor.
	{
		switch (EnemyType)
		{
		case EEnemyType::small:
			EnemyRow = *EnemyDataTableObject->FindRow<FEnemyData>(FName("small"), TEXT(""));
			break;

		case EEnemyType::medium:
			EnemyRow = *EnemyDataTableObject->FindRow<FEnemyData>(FName("medium"), TEXT(""));
			break;

		case EEnemyType::large:
			EnemyRow = *EnemyDataTableObject->FindRow<FEnemyData>(FName("large"), TEXT(""));
			break;
		}
		Health = static_cast<float>(EnemyRow.EnemyHealth);
		maxHealth = static_cast<float>(EnemyRow.EnemyHealth);
	}
}

void AEnemyBase::BulletHit_Implementation(FHitResult HitResult, FWeaponHitData WeaponHitData)
{
	// If there is a sound effect available , play it.
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	float Damage = static_cast<float>(WeaponHitData.hitDamage);
	
	// Apply the weapon multiplier to the damage based on the type of ammo the damage was instigated by.
	ApplyAmmoTypeMultiplier(Damage, WeaponHitData.hitAmmoType);
	// apply WeakSpotMultiplier after ammo type multiplier
	// ammo type multiplier is multiplicative while weakspot multiplier is additive
	CheckWeakSpotHit(Damage, HitResult, WeaponHitData);
	
	// Log the damage into the console.

	// Apply Damage.
	Health -= Damage;
	if (EnemyRow.EnemyFaction == EEnemyFaction::Infested)
	{
		if (WeaponHitData.hitWeaponType == EWeaponType::Shotgun)
		{
				PlayerRef->IncreaseInfestedCharge(Damage * 0.2f);
		}
		else
			PlayerRef->IncreaseInfestedCharge(Damage * 0.1f);
		
		SpawnBloodParticles(HitResult, Damage);
	}

	// If the damage applied takes the enemies health below 1 we kill the enemy.
	if (Health < 1.0f)
	{
		Die();
	}
	
	if(WeaponHitData.hitStagger >= EnemyRow.StaggerValue)
	PlayStaggerAnimation(HitResult.BoneName);

	if(EnemyRow.EnemyFaction != EEnemyFaction::Robot && !PlayerRef->CurrentWeapon->GetIsBloody())
	RangeTestForBlood(GetActorLocation(), 500.0f);
}

void AEnemyBase::ApplyAmmoTypeMultiplier(float& Damage, const EAmmoType& AmmoType)
{
	// Apply the weapon multiplier to the damage based on the type of ammo the damage was instigated by.
	switch (AmmoType)
	{
	case EAmmoType::Bullet: Damage *= EnemyRow.BulletMultiplier;
		break;
	case EAmmoType::Shell: Damage *= EnemyRow.ShellMultiplier;
		break;
	case EAmmoType::Cell: Damage *= EnemyRow.CellMultiplier;
		break;
	case EAmmoType::Rocket: Damage *= EnemyRow.RocketMultiplier;
		break;
	case EAmmoType::None: Damage *= EnemyRow.PistolMultiplier;
		break;
	case EAmmoType::Melee: Damage *= EnemyRow.MeleeMultiplier;
		break;
	default:
		break;
	}
}

void AEnemyBase::CheckWeakSpotHit(float& Damage, const FHitResult& HitResult, const FWeaponHitData& WeaponHitData)
{

	// We can handle hiding body parts by using the HideBoneByName function
	// When working with weakspots we should place the bone we want to hide as the first bone in the array and use that to hide parts of the body.
	// We probably want to create sockets to spawn gibs (name the sockets as the weakspot they are referring to)
	
	// We check every weakspot the enemy has
	for(FWeakSpot& currentWeakSpot : EnemyRow.WeakSpots)
	{
		// check every bone corresponding to the current weakspot
		for(const FName& BoneName : currentWeakSpot.weakSpotBoneNames)
		{
			if(HitResult.BoneName == BoneName)
			{
				if(WeaponHitData.hitWeakSpotStrenght >= currentWeakSpot.weakSpotReq && (!currentWeakSpot.bHasBeenActivated || currentWeakSpot.bCanBeRepeated))
				{
					Damage += currentWeakSpot.weakSpotDamage;
					HandleWeakSpotHit(HitResult, currentWeakSpot, Damage);
					currentWeakSpot.bHasBeenActivated = true;
				}
				return;
			}
		}
		
	}
}

// this function mainly performs visuals
void AEnemyBase::HandleWeakSpotHit(const FHitResult& HitResult, const FWeakSpot& WeakSpot, const float& Damage)
{
	// by this point we have already checked whether the shot has the required weakspot value && if the weakspot has already been activated before

	// The skeletal mesh should have sockets for each weakpoint...
	// Keep the naming convention consistent...
	// The socket name should be the first boneName in the NameArray + Socket
	// i.e if the weakspotBoneNames[0] = head , the socket name should be headSocket
	FString weakSpotString;
	WeakSpot.weakSpotBoneNames[0].ToString(weakSpotString);
	weakSpotString.Append("Socket");
	FName socketName = FName(*weakSpotString);
	
	//float healthValue = std::max(Health - Damage, 0.0f);
	//UE_LOG(LogTemp,Warning,TEXT("EnemyHealthValue: %f"),healthValue);
	// We have to check whether the dismemberment/weakspot requires the enemy to die or not
	if(WeakSpot.bHasToDieToDismember)
	{
		//UE_LOG(LogTemp,Warning,TEXT("Entered bHasToDieToDismember"));
		if(Health - Damage < 1.0f)
		{
			//UE_LOG(LogTemp,Warning,TEXT("Entered under 0 health condition"));
			// Spawn Particles
			if(WeakSpot.NS_WeakSpotParticle)
			{
				
				UNiagaraFunctionLibrary::SpawnSystemAttached(WeakSpot.NS_WeakSpotParticle,GetMesh(),
					socketName,
					GetMesh()->GetSocketLocation(socketName),GetMesh()->GetSocketRotation(socketName),EAttachLocation::KeepRelativeOffset,false);
			}

			// Spawn weakpoint hit sound
			if(WeakSpot.SFX_WeakSpotSound)
			{
				UGameplayStatics::SpawnSoundAttached(WeakSpot.SFX_WeakSpotSound,GetMesh(),socketName);
			}
			
			// Hide corresponding body parts
			GetMesh()->HideBoneByName(WeakSpot.weakSpotBoneNames[0],EPhysBodyOp::PBO_Term);
		}
	}
	else													// if the weakspot mechanic doesn't require death
	{
		// Spawn Particles
		if(WeakSpot.NS_WeakSpotParticle)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(WeakSpot.NS_WeakSpotParticle,GetMesh(),
					socketName,
					GetMesh()->GetSocketLocation(socketName),GetMesh()->GetSocketRotation(socketName),EAttachLocation::KeepRelativeOffset,false);
		}

		// Spawn weakpoint hit sound
		if(WeakSpot.SFX_WeakSpotSound)
		{
			UGameplayStatics::SpawnSoundAttached(WeakSpot.SFX_WeakSpotSound,GetMesh(),socketName);
		}
			
		// Hide corresponding body parts
		GetMesh()->HideBoneByName(WeakSpot.weakSpotBoneNames[0],EPhysBodyOp::PBO_Term);
	}
}

void AEnemyBase::ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem)
{
	if (BloodDecal)
	{
		for (const FBasicParticleData& current : Data)
		{
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *current.Position.ToString());
			UGameplayStatics::SpawnDecalAtLocation(GetWorld(), BloodDecal, FVector(20.f, 202.f, 202.f), current.Position, -1 * current.Velocity.Rotation(),10.f);
		}
	}
}

UNiagaraComponent* AEnemyBase::SpawnBloodParticles(const FHitResult& HitResult, const float& Damage)
{
	if (NSBlood)
	{
		int32 particlesNum = static_cast<int32>(Damage);
		particlesNum *= 0.2f;
		//UE_LOG(LogTemp, Warning, TEXT("BloodParticles Triggered"));
		UNiagaraComponent* Blood = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NSBlood, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation());
		Blood->SetColorParameter(FName("Color"), FLinearColor::Red);
		Blood->SetIntParameter(FName("HitDamage"), particlesNum);
		if (CombatManager)
			Blood->SetVariableActor(FName("CallHandler"), CombatManager);
		else
			Blood->SetVariableObject(FName("CallHandler"), this);

		return Blood;
	}

	return nullptr;
}

void AEnemyBase::PlayStaggerAnimation(const FName& HitBone)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *HitBone.ToString());
	if (HitBone == "upperarm_l" || HitBone == "lowerarm_l" || HitBone == "hand_l")
		StaggerState = 1;
	else if (HitBone == "upperarm_r" || HitBone == "lowerarm_r" || HitBone == "hand_r")
		StaggerState = 2;
	else if (HitBone == "thigh_l" || HitBone == "calf_l" || HitBone == "foot_l")
		StaggerState = 3;

}

// Spawner kills this actor
void AEnemyBase::DieFromSpawn()
{
	if(this)
	{
		bAlive = false;
		
		if(GetController())
			GetController()->UnPossess();

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionProfileName(FName("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);

		if(EnemyRow.SFX_Death)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(),EnemyRow.SFX_Death,GetActorLocation());

		OnDestroyed.Broadcast(this);	// this initiates the exploding component if the enemy has it

		GetWorld()->GetTimerManager().SetTimer(DissolveTimer,this, &AEnemyBase::InitiateDissolveEffect, dissolveTime, false);
		GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &AEnemyBase::SignalDeath, deathCleanUpTime, false);
	}
}


void AEnemyBase::Die()
{
	if (this)
	{
		bAlive = false;
		
		if(GetController())
		GetController()->UnPossess();

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionProfileName(FName("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
		
		if(EnemyRow.SFX_Death)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),EnemyRow.SFX_Death,GetActorLocation());

		OnDestroyed.Broadcast(this);	// this initiates the exploding component if the enemy has it

		if(SpawnerManager)
		{
			SpawnerManager->RemoveEnemy(this);
		}
		else if(CombatManager)
		{
			CombatManager->ClearEnemyData(this);
		}

		GetWorld()->GetTimerManager().SetTimer(DissolveTimer,this, &AEnemyBase::InitiateDissolveEffect, dissolveTime, false);
		GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &AEnemyBase::SignalDeath, deathCleanUpTime, false);		// waits xx seconds before destroying actor in case anything references it
	}
}

void AEnemyBase::InitiateDissolveEffect()
{
	GetWorld()->GetTimerManager().ClearTimer(DissolveTimer);
	// BlueprintImplementableEvent called 
	Dissolve();
	UE_LOG(LogTemp,Warning, TEXT("DissolveInitiated"));
}



void AEnemyBase::SignalDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("SIGNAL DEATH"));
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);

	// TODO Destroy the controller aswell
	
	MarkPendingKill();
	Destroy();
}


void AEnemyBase::SetCombatManager(ACombatManager* OwningManager)
{
	CombatManager = OwningManager;
}

void AEnemyBase::SetSpawnerManager(AEnemySpawner* OwningSpawner)
{
	SpawnerManager = OwningSpawner;
}

void AEnemyBase::SetWaveManager(AWaveManager* OwningWave)
{
	waveManagerRef = OwningWave;
}

void AEnemyBase::RangeTestForBlood(FVector Location, float Radius, float DebugDuration, FColor DebugColor)
{
	if (DebugDuration > 0)
	{
		DrawDebugSphere(GetWorld(), Location, Radius, 16, DebugColor, false, DebugDuration);
	}

	//Create an ObjectType Array and fill it
	TArray<TEnumAsByte<EObjectTypeQuery> > TraceObjectTypes;
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Player));

	//Create an array of actors to ignore and fill it
	TArray<AActor*> ObjectsToIgnore;
	ObjectsToIgnore.Add(this);

	// Create an Array of actors that will be filled by the function SphereOverlapActors
	TArray<AActor*> OutActors;

	//Creates an invisible sphere and fills up the OutActors array with the results that match the given conditions 
	//It returns a bool that we can check if any of the overlapped actors even matched our criteria
	if (UKismetSystemLibrary::SphereOverlapActors(this, Location, Radius, TraceObjectTypes, NULL, ObjectsToIgnore, OutActors))
	{
		for (AActor* HitActor : OutActors)
		{
			if (ACPP_CharacterBase* Player = Cast<ACPP_CharacterBase>(HitActor))
			{
					Player->CurrentWeapon->SetIsBloody(true);
					break;
			}
		}
	}
}

void AEnemyBase::DoDamageToPlayer(AActor* ActorToDamage)
{
	if (ActorToDamage == nullptr) return;
	else if (ActorToDamage == PlayerRef)
	{
		UGameplayStatics::ApplyDamage(ActorToDamage, 10.f, EnemyController, this, UDamageType::StaticClass());
	}
}

void AEnemyBase::PlayAttackMontage(FName Section, float PlayRate)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AttackMontage)
	{
		//UE_LOG(LogTemp, Warning, TEXT("AnimInstance and attackmontage passed"))
		AnimInstance->Montage_Play(AttackMontage);
		AnimInstance->Montage_JumpToSection(Section, AttackMontage);
	}
}

// This will need to be worked on. For now it is hardcoded to the one value we have.
FName AEnemyBase::GetAttackSectionName()
{
	FName SectionName(TEXT("MeleeAttack"));
	return SectionName;
}

void AEnemyBase::OnMeleeAttackOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(DoOnce.Execute())
	DoDamageToPlayer(OtherActor);
}

void AEnemyBase::ActivateMeleeAttackCollision()
{
	MeleeAttackCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemyBase::DisableMeleeAttackCollision()
{
	MeleeAttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoOnce.Reset();
}


FTransform AEnemyBase::CalculateInterception()
{
	FVector MuzzleLoc = GetMesh()->GetSocketLocation(FName("ProjectileSocket"));
	FVector TargetLoc;
	FVector TargetVelocity;
	if(PlayerRef)
	{
	TargetLoc = PlayerRef->TargetHere->GetComponentLocation();
	TargetVelocity = PlayerRef->GetVelocity();
	}
	else
	{
	TargetLoc = GetPlayer()->TargetHere->GetComponentLocation();
	TargetVelocity = GetPlayer()->GetVelocity();
	}

	FVector dX = UKismetMathLibrary::Subtract_VectorVector(TargetLoc, MuzzleLoc);
	FVector dV = TargetVelocity - EnemyRow.ProjecileSpeed;
	float time = UKismetMathLibrary::Divide_VectorVector(dX, dV).Size();

	FVector Result = TargetLoc + TargetVelocity * time;

	if (time > 0)
	{
		FRotator Rotator = UKismetMathLibrary::FindLookAtRotation(MuzzleLoc, Result);
		FTransform SpawnTransform(Rotator, MuzzleLoc);
		return SpawnTransform;
	}
	else
	{
		FRotator Rotator = UKismetMathLibrary::FindLookAtRotation(MuzzleLoc, TargetLoc);
		FTransform SpawnTransform(Rotator, MuzzleLoc);
		return SpawnTransform;
	}

}

AEnemyProjectileBase* AEnemyBase::SpawnProjectile(FTransform& SpawnTransform)
{
	if (ProjectileClass)
	{
		if (PlayerRef->GetIsCharacterMoving() && !bToken)			// if the character is moving we add inaccuracy based on his movement rating
		{
			float x = SpawnTransform.GetRotation().Rotator().Euler().X;
			float y = SpawnTransform.GetRotation().Rotator().Euler().Y;		// leave this value alone
			float z = SpawnTransform.GetRotation().Rotator().Euler().Z;

			float MovementRatingMultiplier = PlayerRef->GetMovementRating();		// Get the Rating and invert it for our use
			if (MovementRatingMultiplier != 0)				//Check to see if it's zero, if so we want to inver it by setting it to one
			{
				MovementRatingMultiplier -= 1;											// We invert it so the more similar the movement is the more accurate the AI will be
				MovementRatingMultiplier *= -1;											// We invert it so the more similar the movement is the more accurate the AI will be
			}																			
			else
				MovementRatingMultiplier = 1.f;


			if (MovementRatingMultiplier <= 0.2f)				// We can also cap this so if the movement is up to a certain% similar the AI has perfect accuracy
				MovementRatingMultiplier = 0.f;

			if (MovementRatingMultiplier != 0.f)
			{
				if (UKismetMathLibrary::RandomBool())
					x += EnemyRow.AccuracyOffset * MovementRatingMultiplier;
				else
					x -= EnemyRow.AccuracyOffset * MovementRatingMultiplier;

				if (UKismetMathLibrary::RandomBool())
					z += EnemyRow.AccuracyOffset * MovementRatingMultiplier;
				else
					z -= EnemyRow.AccuracyOffset * MovementRatingMultiplier;
			}
			
			
			//UE_LOG(LogTemp, Warning, TEXT("%f \t %f"), MovementRatingMultiplier, PlayerRef->GetMovementRating());
			FRotator newRotator(y, z, x);
			SpawnTransform.SetRotation(newRotator.Quaternion());
		}
		

		Projectile = GetWorld()->SpawnActor<AEnemyProjectileBase>(ProjectileClass, SpawnTransform);
		Projectile->SetProjectileData(FEnemyHitData(EnemyRow.PrimaryDamage,EnemyRow.EnemyFaction,EnemyRow.bSlowPlayer),EnemyRow.ProjecileSpeed);
		return Projectile;
	}

	return NULL;
}

void AEnemyBase::FireProjectile()
{

	RequestToken();

	FTransform SpawnTransform = CalculateInterception();
	SpawnProjectile(SpawnTransform);

	ReleaseToken();
}

void AEnemyBase::FireTripleProjectile()
{
	RequestToken();

	FTransform SpawnTransform = CalculateInterception();
	SpawnProjectile(SpawnTransform);
	
	FRotator leftRotator = SpawnTransform.Rotator();
	leftRotator.Yaw += -2.5f;
	
	SpawnTransform.SetRotation(leftRotator.Quaternion());
	SpawnProjectile(SpawnTransform);

	leftRotator.Yaw += 5.f;
	SpawnTransform.SetRotation(leftRotator.Quaternion());
	SpawnProjectile(SpawnTransform);

	ReleaseToken();
}

void AEnemyBase::FireDelayedProjectile(float delayTime, int32 numberOfProjectiles)
{
	for(int32 i = 0; i != numberOfProjectiles; ++i)
	{
		GetWorld()->GetTimerManager().SetTimer(FireTimer,this,&AEnemyBase::FireProjectile,i*delayTime,false);
	}
}



void AEnemyBase::InitiateFire(float PlayRate)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Play(AttackMontage);
		AnimInstance->Montage_JumpToSection(FName("InitiateFire"), AttackMontage);
	}
}

UNiagaraComponent* AEnemyBase::SpawnFireParticles()
{
	if (NS_FireParticle)
		return UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NS_FireParticle, GetMesh()->GetSocketLocation("ProjectileSocket"),GetMesh()->GetSocketRotation("ProjectileSocket"));

	return NULL;
}

ACPP_CharacterBase* AEnemyBase::GetPlayer() const 
{ 
	return Cast<ACPP_CharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)); 
}

FVector AEnemyBase::RequestNewPosition()
{
	if (CombatManager)
	{
		FVector newPos = CombatManager->ProvideFreeLocationWithLOS(GetActorLocation(), LocIndex);
		EnemyController->GetBlackboardComponent()->SetValueAsVector(FName("PatrolPoint"), newPos);
		EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("bSphereCheck"), false);
		return newPos;
	}
	return GetActorLocation();
}

bool AEnemyBase::GetLOS()const
{
	if (ACPP_CharacterBase* Player = GetPlayer())
	{
		FHitResult HitRes;


		TArray<AActor*>IgnoreActors;
		IgnoreActors.Add(PlayerRef);
		IgnoreActors.Add(GetOwner());
		//reverse the result, if the trace doesn't hit anything it means we have LOS thus return true, otherwise false
		return (!UKismetSystemLibrary::SphereTraceSingle(this, GetMesh()->GetSocketLocation(FName("ProjectileSocket")), Player->TargetHere->GetComponentLocation(),
			25.f, UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false, IgnoreActors, EDrawDebugTrace::None, HitRes, true));
	}
	
	// Incase of some error return false
	return false;
}

void AEnemyBase::UpdateLOS()const
{
	EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("bHasLOS"), GetLOS());
}

float AEnemyBase::GetDistance()const
{
	if (ACPP_CharacterBase* Player = GetPlayer())
	{
		return (GetMesh()->GetSocketLocation(FName("ProjectileSocket")) - Player->TargetHere->GetComponentLocation()).Size();
	}

	// Incase of some error return false
	return 0.f;
}

void AEnemyBase::GetEnemyInfo()const
{
	// Set HasLOS value in blackboard
	EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("bHasLOS"), GetLOS());

	float DistanceVariance = 1000.f;		// for now this is a local var but we might want to move this over to the enemy data table if we want more variation
	float Distance = GetDistance();

	// 0 is EDistane::MeleeRange
	// 1 is EDistane::PreferredDistance
	// 2 is EDistance::BadRange
	if(Distance <= EnemyRow.MeleeAttackRange)
		EnemyController->GetBlackboardComponent()->SetValueAsEnum(FName("DistanceEnum"),0);
	else if(Distance<= EnemyRow.PreferredDistance + DistanceVariance && Distance>= EnemyRow.PreferredDistance - DistanceVariance)
		EnemyController->GetBlackboardComponent()->SetValueAsEnum(FName("DistanceEnum"), 1);
	else
		EnemyController->GetBlackboardComponent()->SetValueAsEnum(FName("DistanceEnum"), 2);
}

bool AEnemyBase::GetIsValidPosition(float Radius , float DebugDuration, FColor DebugColor) const
{
	if (DebugDuration > 0)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 16, DebugColor, false, DebugDuration);
	}

	//Create an ObjectType Array and fill it
	TArray<TEnumAsByte<EObjectTypeQuery> > TraceObjectTypes;
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Enemy));

	//Create an array of actors to ignore and fill it
	TArray<AActor*> ObjectsToIgnore;
	ObjectsToIgnore.Add(GetOwner());

	// Create an Array of actors that will be filled by the function SphereOverlapActors
	TArray<AActor*> OutActors;

	//Creates an invisible sphere and fills up the OutActors array with the results that match the given conditions 
	//It returns a bool that we can check if any of the overlapped actors even matched our criteria
	if (UKismetSystemLibrary::SphereOverlapActors(this, GetActorLocation(), Radius, TraceObjectTypes, NULL, ObjectsToIgnore, OutActors))
	{
		for (AActor* HitActor : OutActors)
		{
			if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
			{
				//if there happens to be an enemy in our safe range, check if they are moving or not. if they are still moving this enemy is safe to stop.
				AEnemyController* otherEnemyController = Cast<AEnemyController>(Enemy->GetController());
				if(otherEnemyController && !otherEnemyController->GetBlackboardComponent()->GetValueAsBool("bMoving"))
					return false;
			}
		}
	}

	return true;
}


void AEnemyBase::UpdateIsValidPosition() const
{
	EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("bSphereCheck"), GetIsValidPosition());
}


void AEnemyBase::RequestToken()
{
	if (CombatManager && !bToken)
	{
		if(CalculateIsInView() >= 0.7f)
		bToken = CombatManager->ProvideToken();
	}
		
}

void AEnemyBase::ReleaseToken()
{
	if (CombatManager && bToken)
	{
		CombatManager->ReceiveToken();
		bToken = false;
	}
		
}

float AEnemyBase::CalculateIsInView()
{
	if (PlayerRef)
	{
		FVector PlayerToEnemy = (GetActorLocation() - PlayerRef->GetActorLocation());
		UKismetMathLibrary::Vector_Normalize(PlayerToEnemy);

		// returns how similar the view vector is to the point the enemy is standing at. 1 means the player is looking exactly at the enemy. If we implement variable FOV we have to perform extra calcs
		return UKismetMathLibrary::Dot_VectorVector(PlayerRef->GetFirstPersonCameraComponent()->GetForwardVector(), PlayerToEnemy);	
	}
	return 0;
}

void AEnemyBase::HealEnemy(int32 healAmount)
{
	if(Health+ healAmount > maxHealth)
		Health = maxHealth;
	else
		Health += healAmount;
	
}
