// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "NiagaraDataInterfaceExport.h"

#include "EnemyData.h"
#include "EnemyProjectileBase.h"

#include "../BulletHitInteface.h"
#include "../DoOnce.h"

#include "EnemyBase.generated.h"


class AEnemyController;
class ACPP_CharacterBase;
class USphereComponent;
class UAnimMontage;
class ACombatManager;
class AEnemySpawner;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundCue;
class AWaveManager;

UCLASS()
class CPPSINNER_API AEnemyBase : public ACharacter, public IBulletHitInteface, public INiagaraParticleCallbackHandler
{
	GENERATED_BODY()


public:
	// Sets default values for this character's properties
	AEnemyBase();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BulletHit_Implementation(FHitResult HitResult, FWeaponHitData WeaponHitData) override;

	virtual void ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem);

	void HealEnemy(int32 healAmount);

	UFUNCTION()
	void DieFromSpawn();

	UFUNCTION()
	void Spawn();

	UFUNCTION()
	void InitiateSpawn();

	// when the enemy is in a test tube.
	UFUNCTION()
	void InTestTube();

	// when the test tube gets destroyed the enemy should wake up
	UFUNCTION()
	void Wake();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void DoDamageToPlayer(AActor* ActorToDamage);

	UFUNCTION(BlueprintCallable)
	void PlayAttackMontage(FName Section, float PlayRate = 1.0f);

	UFUNCTION(BlueprintPure)
	FName GetAttackSectionName();

	UFUNCTION()
	void OnMeleeAttackOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable)
	void ActivateMeleeAttackCollision();

	UFUNCTION(BlueprintCallable)
	void DisableMeleeAttackCollision();

	UFUNCTION()
	FTransform CalculateInterception();

	UFUNCTION()
	void Die();
	
	UFUNCTION()
	void SignalDeath();

	UFUNCTION()
	void InitiateDissolveEffect();

	UFUNCTION(BlueprintImplementableEvent)
	void Dissolve();

	UFUNCTION()
	void ApplyAmmoTypeMultiplier(float& Damage, const EAmmoType& AmmoType);

	UFUNCTION()
	virtual void CheckWeakSpotHit(float& Damage, const FHitResult& HitResult, const FWeaponHitData& WeaponHitData);

	UFUNCTION()
	virtual void HandleWeakSpotHit(const FHitResult& HitResult, const FWeakSpot& WeakSpot, const float& Damage);

	UFUNCTION()
	AEnemyProjectileBase* SpawnProjectile(FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable)
	void FireProjectile();

	UFUNCTION(BlueprintCallable)
	void FireTripleProjectile();

	UFUNCTION(BlueprintCallable)
	void FireDelayedProjectile(float delayTime = 0.5f, int32 numberOfProjectiles = 3);

	UFUNCTION()
	void PlayStaggerAnimation(const FName& HitBone);

	UFUNCTION(BlueprintCallable)
	FVector RequestNewPosition();

	UFUNCTION(BlueprintCallable)
	bool GetLOS() const;

	UFUNCTION(BlueprintCallable)
	void UpdateLOS() const;

	float GetDistance()const;

	UFUNCTION(BlueprintCallable)
	bool GetIsValidPosition(float Radius = 500.f, float DebugDuration = 0.0f, FColor DebugColor = FColor::Red)const;

	UFUNCTION(BlueprintCallable)
	void UpdateIsValidPosition() const;

	UFUNCTION(BlueprintCallable)
	void InitiateFire(float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable)
	UNiagaraComponent* SpawnFireParticles();

	UNiagaraComponent* SpawnBloodParticles(const FHitResult&, const float&);

	UFUNCTION()
	void PlaySpawningFX();

	UFUNCTION(BlueprintImplementableEvent)
	void AppearFX(UNiagaraComponent* NiagaraComp);

private:

	void SetEnemyData();

	void RangeTestForBlood(FVector Location, float Radius = 500.0f, float DebugDuration = 0.0f, FColor DebugColor= FColor::Red);

	ACPP_CharacterBase* GetPlayer()const;

	float CalculateIsInView();



public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyType EnemyType;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 StaggerState;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 LocIndex;

protected:

	UPROPERTY()
	FDoOnce DoOnce;
	
	UPROPERTY()
	FTimerHandle FireTimer;

	UPROPERTY()
	FTimerHandle SpawnTimer;

	// timer for freeing this object
	UPROPERTY()
	FTimerHandle DeathTimer;

	// timer for initiating the dissolve effect
	UPROPERTY()
	FTimerHandle DissolveTimer;

	UPROPERTY()
	FTimerHandle StaggerTimer;

	UPROPERTY()
	bool bStaggered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat")
	float staggerTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Visual")
	float deathCleanUpTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Visual")
	float spawnTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Visual")
	float dissolveTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Visual")
	FLinearColor P_Color;
	
	// Sound to play when hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundCue* ImpactSound;

	UPROPERTY()
	float Health;

	UPROPERTY()
	float maxHealth;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AEnemyProjectileBase>ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Particle")
	UNiagaraSystem* NS_FireParticle;

	UPROPERTY()
	AEnemyProjectileBase* Projectile;

	UPROPERTY()
	bool bAlive;
	

	UPROPERTY(BlueprintReadOnly)
	ACombatManager* CombatManager;

	UPROPERTY(BlueprintReadOnly)
	AEnemySpawner* SpawnerManager;

	UPROPERTY(BlueprintReadOnly)
	AWaveManager* waveManagerRef;

	UPROPERTY(EditDefaultsOnly, Category = "Blood")
	UNiagaraSystem* NSBlood;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Blood")
	UMaterialInterface* BloodDecal;

	UPROPERTY(BlueprintReadOnly, Category = "EnemyData")
	FEnemyData EnemyRow;

	// Montage containing different attacks			Should include more but currently we only have one animation for testing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StaggerMontage;

private:

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bToken;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	UDataTable* EnemyDataTableObject;

	// Behavior Tree Variables

	// Behavior tree for the Character to be set in BP.
	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true"))
	class UBehaviorTree* BehaviorTree;

	// Point for the enemy to move to 
	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint;

	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint2;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* MeleeAttackCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bInAttackRange;

	UPROPERTY()
	class AEnemyController* EnemyController;

	UPROPERTY()
	ACPP_CharacterBase* PlayerRef;

	//Animation

	FName AttackR;

public:
	//Getters
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	void SetCombatManager(ACombatManager* OwningManager);

	void SetSpawnerManager(AEnemySpawner* OwningSpawner);

	void SetWaveManager(AWaveManager* OwningWave);

	void ActivateController();

	UFUNCTION(BlueprintCallable)
	void GetEnemyInfo()const;		// Set's the blackboard data for the enemy

	void RequestToken();

	void ReleaseToken();

	FORCEINLINE bool GetHasToken() const { return bToken;}

	FORCEINLINE AEnemyController* GetEnemyController() const {return EnemyController;}

	FORCEINLINE ACPP_CharacterBase* GetPlayerRef() const {return PlayerRef;}
};
