// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveManager.generated.h"

class AEnemySpawner;
class AEnemyBase;
class UCPP_GameInstance;
class ACombatManager;

UCLASS()
class CPPSINNER_API AWaveManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWaveManager();

	UFUNCTION()
	AWaveManager* SpawnWave(ACombatManager* CombatManager);

	UFUNCTION()
	void addSpawnedEnemy(AEnemyBase* EnemyToAdd);

	UFUNCTION()
	void RemoveEnemy(AEnemyBase* EnemyToRemove);

	UFUNCTION()
	void KillSpawners();

	UFUNCTION()
	void KillSpawnedEnemies();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void InitiateNextWave();

	UFUNCTION()
	void WaveCompleted();

	
private:

public:
	// set in Combat Manager BeginPlay	(SpawnFodderWave)
	UPROPERTY()
	bool bFodderWave;

	UPROPERTY()
	int32 waveID;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawners")
	TArray<AEnemySpawner*> managedSpawners;

	UPROPERTY()
	TArray<AEnemyBase*> spawnedEnemies;

	UPROPERTY()
	UCPP_GameInstance* gameInstanceRef;

	UPROPERTY()
	ACombatManager* combatManagerRef;

	// number of enemies left from this wave to enter the next wave if possible, default 0(aka only proceed to the next wave if there are no more enemies from this wave)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 nEnemiesToNext;

	UPROPERTY()
	bool bNextWaveCalled;
private:

	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
