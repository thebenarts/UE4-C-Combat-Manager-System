// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"
#include "EnemyBase.h"
#include "CombatManager.h"
#include "DrawDebugHelpers.h"

#include "WaveManager.h"

#include "../CPP_GameInstance.h"
#include "Chaos/Collision/CollisionApplyType.h"


#include "Kismet/KismetMathLibrary.h"

// Sets default values
AEnemySpawner::AEnemySpawner() : bKeepSpawning(false), secondarySpawnTime(2.0f), bSpawnOnGround(true)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	nEnemies = PositionArray.Num();
	
	GameInstanceRef = Cast<UCPP_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemySpawner::SetWaveManager(AWaveManager* WaveManager)
{
	waveManagerRef = WaveManager;
}

AEnemyBase* AEnemySpawner::SpawnUnit(const FVector& position)
{
	FVector spawnPosition = position;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	AEnemyBase* Enemy;
	// if bSpawnOnGround , we want to find the ground
	if(bSpawnOnGround)
	{
		spawnPosition += GetActorLocation();
		FHitResult HitResult;

		FVector End = spawnPosition + FVector(0.0f,0.0f,-1.0f) * 2500.f;
		
		GetWorld()->LineTraceSingleByChannel(HitResult,spawnPosition,End,ECC_Visibility);
		if(HitResult.bBlockingHit)
		{
			End = HitResult.ImpactPoint;
		}
		else
		{
			End = spawnPosition;
			End.Z += 15.f;
		}

		spawnPosition = UKismetMathLibrary::InverseTransformLocation(GetActorTransform(),End);
		
		Enemy = GetWorld()->SpawnActor<AEnemyBase>(EnemyType, UKismetMathLibrary::TransformLocation(GetActorTransform(),spawnPosition), FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		Enemy = GetWorld()->SpawnActor<AEnemyBase>(EnemyType, UKismetMathLibrary::TransformLocation(GetActorTransform(),spawnPosition), FRotator::ZeroRotator, SpawnParams);
	}

	if(Enemy)
	{
		if(bKeepSpawning)
			SpawnedActors.Add(Enemy);		// Only fill the array if this is a fodder spawner
			
		Enemy->SetSpawnerManager(this);		// Enemy will need to let the spawner know when it dies
		Enemy->InitiateSpawn();
		// if the spawner is a fodder spawner it should handle spawning a new one in place of the dead one

		if(waveManagerRef)
		{
			waveManagerRef->addSpawnedEnemy(Enemy);
		}
	}

	return Enemy;
}

void AEnemySpawner::SpawnEnemies(AWaveManager* WaveManager)
{
	SetWaveManager(WaveManager);
	
	for (const FVector& currentPos : PositionArray)
		SpawnUnit(currentPos);
	
}

void AEnemySpawner::ClearChainEnemy(AEnemyBase* EnemyToRemove)
{
	if(EnemyToRemove && waveManagerRef)
	{
		waveManagerRef->RemoveEnemy(EnemyToRemove);
	}
}


void AEnemySpawner::RemoveEnemy(AEnemyBase* EnemyToRemove)
{
	ClearChainEnemy(EnemyToRemove);
	
	if(SpawnedActors.Num() && bKeepSpawning)
	{
		SpawnedActors.Remove(EnemyToRemove);
		
		if(GameInstanceRef && GameInstanceRef->bPlayerInCombat)
		GetWorld()->GetTimerManager().SetTimer(secondarySpawnTimer,this,&AEnemySpawner::spawnFodder,secondarySpawnTime,false);
	}
}

void AEnemySpawner::spawnFodder()
{
	GetWorld()->GetTimerManager().ClearTimer(secondarySpawnTimer);

	int32 spawnPosID = UKismetMathLibrary::RandomIntegerInRange(0,nEnemies-1);

	SpawnUnit(PositionArray[spawnPosID]);
	
	// perform a check if we are still waiting for another fodder to spawn, since we just cleared the timer when entering this function
	if(SpawnedActors.Num() < nEnemies && GameInstanceRef && GameInstanceRef->bPlayerInCombat)
		GetWorld()->GetTimerManager().SetTimer(secondarySpawnTimer, this, &AEnemySpawner::spawnFodder,secondarySpawnTime/2.f,false);
}

