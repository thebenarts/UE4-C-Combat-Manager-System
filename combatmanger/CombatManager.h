// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EnvQueryManager.h"

#include "NiagaraDataInterfaceExport.h"

#include "Templates/SharedPointer.h"

#include "CombatManager.generated.h"

class AEnemyBase;
class AEnemySpawner;
class UEnvQuery;
class ACPP_CharacterBase;
class UCPP_GameInstance;
class AWaveManager;
class UBoxComponent;

USTRUCT()
struct CPPSINNER_API FcustomItem {

	GENERATED_BODY()

	FcustomItem() :index(0), item(FVector::ZeroVector), rating(0){}
	FcustomItem(const int32& inIndex, const FVector& inItem = FVector::ZeroVector, const float& inRating = 0) : index(inIndex), item(inItem), rating(inRating) {}


	int32 index;
	FVector item;
	float rating;
};

UCLASS()
class CPPSINNER_API ACombatManager : public AActor, public INiagaraParticleCallbackHandler
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACombatManager();
	
	UFUNCTION()
	void ClearEnemyData(AEnemyBase* EnemyToRemove);

	UFUNCTION()
	void SpawnNextWave(int32 waveID);

	virtual void ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem);

	UFUNCTION()
	void SetWaveCompleted(int32 waveID);

	UFUNCTION()
	void AddManagedActor(AEnemyBase* EnemyToAdd);

protected:
	void HandleQueryResult(TSharedPtr<FEnvQueryResult> result);

	UFUNCTION()
	void SpawnWave();
	
	UFUNCTION()
	void SpawnFodderWave();
	
	UFUNCTION()
	void InitiateDestruction();

	UFUNCTION()
	void SetManagedActors();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	//void GetAndAddAllSpawnedActors();

	void ClearCustomItemArr();

	void PerformVisibilityTest();

	void PerformDistanceTest();

	FVector ReturnClosest(const FVector&, int32&);
	

	void PrintTest();

	FVector ReturnRandomFromPerfectScores(const FVector&, int32&);

	FORCEINLINE ACPP_CharacterBase* GetPlayer() const { return Cast<ACPP_CharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)); }

	UFUNCTION()
	void AddToken();

	UFUNCTION()
		void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager")
	TArray<AWaveManager*>ManagedWaves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager")
	AWaveManager* ManagedFodderWave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager")
	TArray<AEnemyBase*>ManagedEnemies;

	UPROPERTY()
	TArray<bool>CompletedWaves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQS")
	UEnvQuery* EnvQuery;

	UPROPERTY()
	FEnvQueryRequest EnvQueryRequest;

	// SharedPtrs can't be uproperty
	TSharedPtr<FEnvQueryResult>GeneratedPointResult;

	UPROPERTY()
	TSet<int32>OccupiedPoints;

	UPROPERTY()
	TArray<FcustomItem> RatedItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager")
	uint8 maxTokens;

	UPROPERTY(VisibleAnywhere, Category = "Manager")
	uint8 currTokens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager")
	float TokenRegenDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Blood")
	UMaterialInterface* BloodDecal;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Spawner")
	UBoxComponent* TriggerOverlap;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bHasTrigger;

	UPROPERTY()
	FTimerHandle TL_Destruction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float destructionTimer;
	
private:
	UPROPERTY(EditAnywhere, Category = "EQS")
	float GridHalfSize;

	UPROPERTY(EditAnywhere, Category = "EQS")
	float SpaceBetweenPoints;

	UPROPERTY()
	FVector LastPlayerPos;

	UPROPERTY()
	bool bSafeToTest;

	UPROPERTY()
	bool bLOSCalced;

	UPROPERTY()
	UCPP_GameInstance* GameInstanceRef;

	UPROPERTY()
	int32 currentWaveID;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FVector ProvideFreeLocation(const FVector&, int32& currentIndex);

	FVector ProvideFreeLocationWithLOS(const FVector&, int32& currentIndex);

	FORCEINLINE bool GetIsSafeToTest() const { return bSafeToTest;}
	
	bool ProvideToken();

	void ReceiveToken();

	void FreeLocationIndex(int32 LocationIndex);
};
