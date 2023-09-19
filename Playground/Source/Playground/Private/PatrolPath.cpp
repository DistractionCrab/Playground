// Fill out your copyright notice in the Description page of Project Settings.


#include "PatrolPath.h"

// Sets default values
APatrolPath::APatrolPath()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	this->Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(this->Root);
}

// Called when the game starts or when spawned
void APatrolPath::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APatrolPath::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

AActor* APatrolPath::FindClosestPoint(AActor* Patroller) {
	AActor* Goal = Patroller;
	float Dist = this->LostDistance * this->LostDistance;

	for (AActor* p : this->PatrolPoints) {
		float DSquared = FVector::DistSquared(p->GetActorLocation(), Patroller->GetActorLocation());
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, 
				FString::Printf(TEXT("DSquared: %f with %s name. Dist = %f"), DSquared, *p->GetName(), Dist));
		if (DSquared < Dist) {
			Goal = p;
			Dist = DSquared;
		}
	}
	return Goal;
}