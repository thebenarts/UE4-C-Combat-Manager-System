// Fill out your copyright notice in the Description page of Project Settings.


#include "WaveManager.h"

#include "CombatManager.h"
#include "EnemyBase.h"
#include "EnemySpawner.h"


// Sets default values
AWaveManager::AWaveManager() : bFodderWave(false),
combatManagerRef(NULL),
nEnemiesToNext(0),
bNextWaveCalled(false)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AWaveManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWaveManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

AWaveManager* AWaveManager::SpawnWave(ACombatManager* CombatManager)
{
	if(CombatManager)
		combatManagerRef = CombatManager;
	
	for(AEnemySpawner* currentSpawner : managedSpawners)
	{
		currentSpawner->SpawnEnemies(this);
	}

	return this;
}

// this will be called from the EnemySpawner, once it spawned the enemy in question
void AWaveManager::addSpawnedEnemy(AEnemyBase* EnemyToAdd)
{
	if(EnemyToAdd)
	{
		EnemyToAdd->SetWaveManager(this);
		spawnedEnemies.Add(EnemyToAdd);

		if(combatManagerRef)
			EnemyToAdd->SetCombatManager(combatManagerRef);
	}
}

// this gets called from EnemySpawner
void AWaveManager::RemoveEnemy(AEnemyBase* EnemyToRemove)
{
	if(EnemyToRemove && spawnedEnemies.Num())
	{
		spawnedEnemies.Remove(EnemyToRemove);
		if(combatManagerRef)
		{
			combatManagerRef->ClearEnemyData(EnemyToRemove);
		}
	}
	WaveCompleted();
	InitiateNextWave();
}

void AWaveManager::InitiateNextWave()
{
	if(spawnedEnemies.Num() <= nEnemiesToNext && !bFodderWave && combatManagerRef && !bNextWaveCalled)
	{
		combatManagerRef->SpawnNextWave(waveID);
		bNextWaveCalled = true;
	}
}

void AWaveManager::KillSpawnedEnemies()
{
	for(AEnemyBase* currentEnemy : spawnedEnemies)
	{
		currentEnemy->DieFromSpawn();
	}
	KillSpawners();
}


void AWaveManager::KillSpawners()
{
	for(AEnemySpawner* currentSpawner : managedSpawners)
	{
		currentSpawner->Destroy();
	}
	
	managedSpawners.Empty();
}

void AWaveManager::WaveCompleted()
{
	if(!bFodderWave && 0 == spawnedEnemies.Num() && combatManagerRef)
		combatManagerRef->SetWaveCompleted(waveID);
}




