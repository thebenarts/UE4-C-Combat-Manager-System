// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatManager.h"
#include "EnemyBase.h"
#include "EnemySpawner.h"
#include "WaveManager.h"

#include "Kismet/GameplayStatics.h"
#include "../Player/CPP_CharacterBase.h"

#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"

#include "EQST_TraceTest.h"

#include "TimerManager.h"
#include "../CPP_GameInstance.h"

#include "EnvironmentQuery/EnvQuery.h"

// Sets default values
ACombatManager::ACombatManager() :maxTokens(5), 
currTokens(maxTokens), 
TokenRegenDelay(2.f), 
bHasTrigger(true),
destructionTimer(45.f),
GridHalfSize(2000.f), 
SpaceBetweenPoints(200.f),
LastPlayerPos(FVector::ZeroVector), 
bSafeToTest(false), 
bLOSCalced(false),
currentWaveID(0)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

		TriggerOverlap = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerOverlap"));
		TriggerOverlap->SetupAttachment(RootComponent);
		TriggerOverlap->SetCollisionProfileName(FName("OverlapAll"));
		TriggerOverlap->InitBoxExtent(FVector(GridHalfSize, GridHalfSize, 500.f));

}

// Called when the game starts or when spawned
void ACombatManager::BeginPlay()
{
	Super::BeginPlay();

	// We are using this game instance ref to change bPlayerInCombat on Overlap
	// This means we will call this at the start of battle, (first Manager in the arena)
	// We will also call it at the last point where the curent Manager doesn't have
	// a nextManager, signaling the end of combat
	GameInstanceRef = Cast<UCPP_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	
	currTokens = maxTokens;
	SetManagedActors();

	TriggerOverlap->InitBoxExtent(FVector(GridHalfSize, GridHalfSize, 500.f));
	
	if(bHasTrigger)
	TriggerOverlap->OnComponentBeginOverlap.AddDynamic(this, &ACombatManager::OnComponentBeginOverlap);
	else
	{
		TriggerOverlap->SetCollisionProfileName(FName("NoCollision"));
		TriggerOverlap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (EnvQuery)
	{
		EnvQueryRequest = FEnvQueryRequest(EnvQuery, this);
		EnvQueryRequest.SetFloatParam(FName("GridHalfSize"), GridHalfSize);
		EnvQueryRequest.SetFloatParam(FName("GridSpaceBetween"), SpaceBetweenPoints);
		EnvQueryRequest.Execute(EEnvQueryRunMode::AllMatching, this, &ACombatManager::HandleQueryResult);
	}
	
	if (!bHasTrigger && EnvQuery)
	{
		SpawnFodderWave();
		SpawnWave();
	}

	CompletedWaves.Init(false,ManagedWaves.Num());
}

void ACombatManager::HandleQueryResult(TSharedPtr<FEnvQueryResult> result)
{
	if (result->IsSuccsessful())
	{
		GeneratedPointResult = result;				// Assign the result sharedpointer to our shared pointer increasing the reference count by one meaning the data won't be deleted
													// Don't forget to free the pointer when we are done ( prob when we don't have any more actors in our array		you can free with the reset method
		
		
		RatedItems.Reserve(GeneratedPointResult->Items.Num());		// Reserve Enough space for all the items

		for (int i = 0; i < result->Items.Num(); i++)		// Add all the items to our custom container
		{
			RatedItems.Emplace(FcustomItem(i,result->GetItemAsLocation(i)));
		}
		
		bSafeToTest = true;
	}
}

void ACombatManager::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACPP_CharacterBase* Player = Cast<ACPP_CharacterBase>(OtherActor))
	{
		if(GameInstanceRef)
			GameInstanceRef->bPlayerInCombat = true;

		// Spawn first wave
		SpawnWave();
		SpawnFodderWave();
		
		TriggerOverlap->SetCollisionProfileName(FName("NoCollision"));
		TriggerOverlap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TriggerOverlap->OnComponentBeginOverlap.Clear();
	}

}

void ACombatManager::ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem)
{
	if (BloodDecal)
	{
		for (const FBasicParticleData& current : Data)
		{
			UGameplayStatics::SpawnDecalAtLocation(GetWorld(), BloodDecal, FVector(60.f, 60.f, 60.f), current.Position, -1 * current.Velocity.Rotation(), 10.f);
		}
	}
}

void ACombatManager::PrintTest()
{
	for (int i = 0; i != RatedItems.Num(); ++i)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d : %s \t %f"), i, *RatedItems[i].item.ToString(), RatedItems[i].rating);
	}
}

void ACombatManager::FreeLocationIndex(int32 LocationIndex)
{
	OccupiedPoints.Remove(LocationIndex);
}

void ACombatManager::PerformVisibilityTest()
{
	ACPP_CharacterBase* PlayerRef = GetPlayer();
	if (PlayerRef)
	{
		FVector itemZOffset(0.f, 0.f, 50.f);
		FVector PlayerLoc = PlayerRef->TargetHere->GetComponentLocation();			//Create variables before the loop so we don't waste computation
		FCollisionQueryParams CollisionParam;
		CollisionParam.AddIgnoredActor(PlayerRef);

		//PrintTest();
		//..////////////////LINE TRACE VERSION 
		for (FcustomItem& current : RatedItems)
		{
			FHitResult HitRes;
			if (!GetWorld()->LineTraceSingleByChannel(HitRes, current.item + itemZOffset, PlayerLoc, ECollisionChannel::ECC_Visibility, CollisionParam))		// if the trace doesn't hit
				current.rating = 1.f;		// it means we have line of sight so we set the rating of that point to 1 else we set it to 0
			else
				current.rating = 0.f;		// we set it to 0 because we are not sure if this is the first time we perform tests on the data base 
		}

		// Once we are done with rating we sort it
		RatedItems.Sort([](const FcustomItem& a, const FcustomItem& b) {return a.rating > b.rating; });

		LastPlayerPos = PlayerLoc;
		bLOSCalced = true;
	}
}

void ACombatManager::PerformDistanceTest()
{

	ACPP_CharacterBase* PlayerRef = GetPlayer();
	if (PlayerRef)
	{

		FVector PlayerLoc = PlayerRef->TargetHere->GetComponentLocation();

		// Find the first index of the element that is no longer a valid point due to LOS requirements not being met
		int32 OnePastLastValid = RatedItems.IndexOfByPredicate([](const FcustomItem& item) {return item.rating < 1 ? true : false; });	
		if (OnePastLastValid != INDEX_NONE)		// Check if we even have a point that satisfies our condition
		{

			float preferredDistance = 1200.f;
			float normalizeFurtherMax = ((PlayerLoc - GetActorLocation()).Size() + GridHalfSize) * 2 - preferredDistance;				// Farthest point that is equal to a 0 rating

			float normalizeMin = 0;				
			float normalizeCloserMax = preferredDistance * 1.5f;		// We multiply by two so even if a point is on the player it will atleast have a value of 0.5
			// We can change the multiplier in order to determine how closer points should be graded.
			// If the multiplier is smaller the closer the point is to the player the smaller grade it will get, same is true the other way around
			for (int i = 0; i != OnePastLastValid; ++i)			// PICKUP FROM HERE: FIGURE OUT A CALC TO RATE POINTS BASED ON POINTS
			{
				float currItemDistance = (RatedItems[i].item - PlayerLoc).Size();
				
				// If the location is further than the preferred distance				AKA far away from the player
				//if (currItemDistance - preferredDistance >= 0)
				//	RatedItems[i].rating = 1 - (currItemDistance / normalizeFurtherMax);			// Normalize the rating and than invert it because we want the good ratings to be closer to one
				//else
				//	RatedItems[i].rating = 1 - (currItemDistance / normalizeCloserMax);	

				if (currItemDistance - preferredDistance >= 0)
					RatedItems[i].rating = 1 - ((currItemDistance-preferredDistance) / normalizeFurtherMax);			// Normalize the rating and than invert it because we want it to the good ratings to be closer to one
				else
					RatedItems[i].rating = 1 - (-1 * (currItemDistance-preferredDistance) / normalizeCloserMax);
			}
			RatedItems.Sort([](const FcustomItem& a, const FcustomItem& b) {return a.rating > b.rating; });
		}
		LastPlayerPos = PlayerLoc;
	}
	
}

FVector ACombatManager::ProvideFreeLocation(const FVector& currentPos, int32& currentIndex)
{

	//TSharedPtr<FEnvQueryResult> GeneratedPointResult;
	if (GeneratedPointResult)
	{
		int32 pointIndex = FMath::RandRange(0, GeneratedPointResult->Items.Num()-1);
		if (OccupiedPoints.Contains(pointIndex))
		{
			if (OccupiedPoints.Num() == GeneratedPointResult->Items.Num())
				return currentPos;

			for (int i = 0; i != GeneratedPointResult->Items.Num(); ++i)
			{
				if (!OccupiedPoints.Contains(i))
				{
					OccupiedPoints.Remove(currentIndex);
					currentIndex = i;
					OccupiedPoints.Add(i);
					return GeneratedPointResult->GetItemAsLocation(i);
				}
					
			}
		}
		OccupiedPoints.Remove(currentIndex);
		currentIndex = pointIndex;
		OccupiedPoints.Add(pointIndex);
		return GeneratedPointResult->GetItemAsLocation(pointIndex);
	}
	return currentPos;
}

FVector ACombatManager::ReturnRandomFromPerfectScores(const FVector& currentPos, int32& currentIndex)
{
	if (RatedItems.Num()>0)			// Safety check incase we haven't yet filled the array with data
	{
		int32 OnePastLastValid = RatedItems.IndexOfByPredicate([](const FcustomItem& item) {return item.rating < 0.95f ? true : false; });	// Get the range of possible items
		if (OnePastLastValid == INDEX_NONE)
			return currentPos;

		int32 randPointIndex = FMath::RandRange(0, OnePastLastValid - 1);	// Try random item in range

		if (!OccupiedPoints.Contains(RatedItems[randPointIndex].index))
		{
			// if we are already standing on the point we just return the current position
			if (currentIndex == RatedItems[randPointIndex].index)
				return currentPos;

			// Remove the current index from the set so that other actors may access this point later on
			OccupiedPoints.Remove(currentIndex);
			// set the index in the character to the new index
			currentIndex = RatedItems[randPointIndex].index;
			// Add the new index to the set so others know this point is occupied
			OccupiedPoints.Add(currentIndex);

			// return the new position
			return RatedItems[randPointIndex].item;
		}

		// Else We just go through the possible items range to find one that isn't already in use
		for (int i = 0; i != OnePastLastValid; ++i)
		{
			if (!OccupiedPoints.Contains(RatedItems[randPointIndex].index))
			{
				// Remove the current index from the set so that other actors may access this point later on
				OccupiedPoints.Remove(currentIndex);
				// set the index in the character to the new index
				currentIndex = RatedItems[i].index;
				// Add the new index to the set so others know this point is occupied
				OccupiedPoints.Add(currentIndex);

				// return the new position
				return RatedItems[i].item;
			}
		}
		return currentPos;
	}
	return currentPos;
}

FVector ACombatManager::ProvideFreeLocationWithLOS(const FVector& currentPos, int32& currentIndex)
{
	if (bSafeToTest)
	{
		ACPP_CharacterBase* PlayerRef = GetPlayer();
		if (PlayerRef)
		{
			// if the player hasn't moved too much since the last test was executed we can just use the data from the previous test
			//if (bLOSCalced && (PlayerRef->TargetHere->GetComponentLocation() - LastPlayerPos).Size() < 500.f)
			//{
			//	//return ReturnRandomFromPerfectScores(currentPos, currentIndex);
			//	return ReturnClosest(currentPos, currentIndex);
			//}

				// Else we have to perform a test on the points to have fresh data
			PerformVisibilityTest();
			PerformDistanceTest();
			return ReturnClosest(currentPos, currentIndex);
			//return ReturnRandomFromPerfectScores(currentPos, currentIndex);
		}
	}
	return currentPos;
}

FVector ACombatManager::ReturnClosest(const FVector& currentPos, int32& currentIndex)
{
	//Get Range of items
	float ImportanceRatio = .85f;			// for weighted normalization between the (distance from player) and (distance from prev point)

	float Percentage = .8f;

	while (Percentage > -0.1)
	{
		int32 OnePastLastValid = RatedItems.IndexOfByPredicate([&](const FcustomItem& item) {return item.rating < Percentage ? true : false; });	// Get the range of possible items
		if (OnePastLastValid == INDEX_NONE)	// If we have points
			Percentage -= 0.2f;
		else
		{
			
			float SmallestDistance = 500000;		// values needed to normalize the data
			float LargestDistance = 0;

			for (int32 i = 0; i != OnePastLastValid; ++i)			// loop to find the smallest and largest value, NEEDED in order to normalize the rating
			{

				if (!OccupiedPoints.Contains(RatedItems[i].index))
				{
					float currDistance = (RatedItems[i].item - currentPos).Size();

					if (currDistance < SmallestDistance)
						SmallestDistance = currDistance;

					if (currDistance > LargestDistance)
						LargestDistance = currDistance;
				}
			}
			
			float NormalizeRange = LargestDistance - SmallestDistance;
			

			// Calculate the first items weighted rating
			FcustomItem ReturnThisItem;

			for (int32 i = 0; i != OnePastLastValid; ++i)
			{
				if (!OccupiedPoints.Contains(RatedItems[i].index))
				{
					// Normalize the value and flip it because we want the values closer to the prev point to be rated higher
					float currNormalized = 1 - ((RatedItems[i].item - currentPos).Size() - SmallestDistance) / NormalizeRange;
					float FinalRating = RatedItems[i].rating * ImportanceRatio + currNormalized * (1 - ImportanceRatio);		// Do weighted normalization

					if (FinalRating > ReturnThisItem.rating)		// if our rating is better than the previous best than update our item
					{
						ReturnThisItem.rating = FinalRating;
						ReturnThisItem.index = RatedItems[i].index;
						ReturnThisItem.item = RatedItems[i].item;
					}
				}
			}

			OccupiedPoints.Remove(currentIndex);
			currentIndex = ReturnThisItem.index;
			OccupiedPoints.Add(currentIndex);
			return ReturnThisItem.item;

		}
	}
	
	// if for some reason we can't find it we just return a copy with the current variables
	return currentPos;

}

// Called every frame
void ACombatManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//------------------------------------------------------------------------------------------------------------------------------
// MANAGE SPAWNING AND DEATH OF ENEMIES

// set the already existing enemies, so they know of their manager.
// we should strive to spawn all enemies, but if we want to make a set piece where enemies are already in place 
// we have the option of assigning already placed enemies to the manager
// this is important because this way, they are able to ask for tokens, and positions
void ACombatManager::AddManagedActor(AEnemyBase* EnemyToAdd)
{
	if(EnemyToAdd)
	{
		ManagedEnemies.Add(EnemyToAdd);
	}
}
void ACombatManager::SetManagedActors()
{
	for(AEnemyBase* currentEnemy : ManagedEnemies)
	{
		currentEnemy->SetCombatManager(this);
	}
}

void ACombatManager::SpawnFodderWave()
{
	if(ManagedFodderWave)
	{
		ManagedFodderWave->bFodderWave = true;
		ManagedFodderWave->SpawnWave(this);
	}
}


// spawns the current wave
void ACombatManager::SpawnWave()
{
	if(ManagedWaves.Num() && currentWaveID < ManagedWaves.Num())
	{
		AWaveManager* currentWave = ManagedWaves[currentWaveID]->SpawnWave(this);
		
		if(currentWave)
		currentWave->waveID = currentWaveID;
	}
}

// called from wavemanager. if possible spawns the next wave, furthermore initiates freeing this memory if we have cleared all the waves
void ACombatManager::SpawnNextWave(int32 waveID)
{
	// This is technically a double check, if performance really needs to be optimized we can save a few cpu cycles here
	if(waveID < currentWaveID)
		return;
	
	++currentWaveID;
	if(currentWaveID < ManagedWaves.Num())
		SpawnWave();
}

void ACombatManager::SetWaveCompleted(int32 waveID)
{
	if(waveID < CompletedWaves.Num())
	{
		CompletedWaves[waveID] = true;
	}

	bool waveCheck = true;
	
	for(const bool& wave : CompletedWaves)
	{
		if(!wave)
			waveCheck = false;
	}

	if(waveCheck)
	{
		if(GameInstanceRef)
			GameInstanceRef->bPlayerInCombat = false;

		GetWorldTimerManager().SetTimer(TL_Destruction,this, &ACombatManager::InitiateDestruction,destructionTimer,false);
	}
}


void ACombatManager::InitiateDestruction()
{
	GetWorldTimerManager().ClearTimer(TL_Destruction);

	for(AWaveManager* currentWaveManager : ManagedWaves)
	{
		if(currentWaveManager)
		{
			currentWaveManager->KillSpawnedEnemies();
			currentWaveManager->Destroy();
		}
	}
	ManagedWaves.Empty();

	if(ManagedFodderWave)
	{
		ManagedFodderWave->KillSpawnedEnemies();
		ManagedFodderWave->Destroy();
	}
	ManagedFodderWave = NULL;

	ClearCustomItemArr();
	Destroy();
}

void ACombatManager::ClearCustomItemArr()		// Delete the dynamically allocated objects held in the array
{
	RatedItems.Empty();
}

bool ACombatManager::ProvideToken()
{
	if (currTokens > 0)
	{
		currTokens -= 1;
		return true;
	}
	return false;
}

void ACombatManager::ReceiveToken()
{
	// Delay the token acquisition, this will prove as another variable to balance the game
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ACombatManager::AddToken, TokenRegenDelay, false);
}

void ACombatManager::AddToken()
{
	currTokens += 1;
}

// This gets called from WaveManager, clear up data of the dead enemy.
void ACombatManager::ClearEnemyData(AEnemyBase* EnemyToRemove)
{
	if(EnemyToRemove)
	{
		EnemyToRemove->ReleaseToken();
		FreeLocationIndex(EnemyToRemove->LocIndex);
		EnemyToRemove->SetCombatManager(NULL);
	}
}
