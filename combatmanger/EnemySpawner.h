// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPPSinner/CPP_GameInstance.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"


class AEnemyBase;
class AEnemyController;
class ACombatManager;
class UCPP_GameInstance;
class AWaveManager;

UCLASS()
class CPPSINNER_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void spawnFodder();

	UFUNCTION()
	void ClearChainEnemy(AEnemyBase* EnemyToRemove);

	UFUNCTION()
	AEnemyBase* SpawnUnit(const FVector& position);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SpawnEnemies(AWaveManager* CombatManager);

	void RemoveEnemy(AEnemyBase* EnemyToRemove);

	void SetWaveManager(AWaveManager* WaveManager);
	
	UPROPERTY()
	TArray<AEnemyBase*>SpawnedActors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawner")
	bool bKeepSpawning;

protected:
	UPROPERTY()
	int32 nEnemies;

	UPROPERTY()
	FTimerHandle secondarySpawnTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawner")
	float secondarySpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (MakeEditWidget = "true"))
	TArray<FVector>PositionArray;

	UPROPERTY(EditDefaultsOnly, Category = "Spawner")
	TSubclassOf<AEnemyBase> EnemyType;

	UPROPERTY()
	ACombatManager* combatManager;

	UPROPERTY()
	AWaveManager* waveManagerRef;

	UPROPERTY()
	UCPP_GameInstance* GameInstanceRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bSpawnOnGround;
};
