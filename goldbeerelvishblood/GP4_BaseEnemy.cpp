#include "GP4_BaseEnemy.h"
#include "GP4_AIDirector.h"
#include "GP4_BasicSpawner.h"
#include "GoldBeerElvishBlood/GP4WorldSubsystemResorce.h"

AGP4_BaseEnemy::AGP4_BaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	HasTarget = false;
	AttackDistance = 100.0f;
	InterruptDistance = 200.0f;
	savedHasBeenSet = false;
	damageTimer = 0.0f;
	Damage = 10.0f;
	DamageCooldown = 4.0f;
	Health = 50.0f;
	UnitSpeed = 150.0f;
	BloodGain = 1.0f;

	Collider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collider"));
	RootComponent = Collider;
	Collider->SetCanEverAffectNavigation(false);
	Collider->SetCollisionProfileName(TEXT("Enemy"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collider);
	Mesh->SetCanEverAffectNavigation(false);

	RangeBubble = CreateDefaultSubobject<USphereComponent>(TEXT("RangeBubble"));
	RangeBubble->SetupAttachment(Collider);
	RangeBubble->SetSphereRadius(AttackDistance);
	RangeBubble->SetCanEverAffectNavigation(false);
	RangeBubble->SetCollisionProfileName(TEXT("OverlapBuilding"));
}

void AGP4_BaseEnemy::BeginPlay()
{
	Super::BeginPlay();

	//SpawnDefaultController();

	//GetCharacterMovement()->MaxWalkSpeed = UnitSpeed;

	SetMovementSpeed(UnitSpeed);

	//AGP4_AIDirector::RegisterUnit(this);
}

void AGP4_BaseEnemy::UnitSlowdownEffect(float SlowdownAmount, float Duration)
{
	//GetCharacterMovement()->MaxWalkSpeed = UnitSpeed - SlowdownAmount;
	SetMovementSpeed(UnitSpeed - SlowdownAmount);
	slowdownEffect = true;
	slowdownTimer = Duration;
}

void AGP4_BaseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGP4WorldSubsystemResorce* ResourceSubsystem = GetWorld()->GetSubsystem<UGP4WorldSubsystemResorce>();
	if (ResourceSubsystem != nullptr)
	{
		ResourceSubsystem->IncreaseBlood(BloodGain);
	}

	if (directorReference.IsValid()) directorReference->UnRegisterUnit(this);
	if (spawnerReference.IsValid()) spawnerReference->UnRegisterUnit(this);

	Super::EndPlay(EndPlayReason);
}

void AGP4_BaseEnemy::InitializeUnit(AGP4_AIDirector* director)
{
	directorReference = director;
	director->RegisterUnit(this);
}

void AGP4_BaseEnemy::InitializeUnit(AGP4_BasicSpawner* director)
{
	spawnerReference = director;
	director->RegisterUnit(this);
}

void AGP4_BaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//DrawDebugSphere(GetWorld(), targetActorLocation, 100, 6, FColor(255, 0, 0));

	damageTimer += DeltaTime;

	if (!targetActorReference.IsValid())
	{
		if (HasTarget) HasTarget = false;
	}

	if (slowdownEffect)
	{
		slowdownTimer -= DeltaTime;
		if (slowdownTimer < 0.0f)
		{
			slowdownEffect = false;
			SetMovementSpeed(UnitSpeed);
		}
	}

	if (damageTimer > DamageCooldown)
	{
		if (!targetActorReference.IsValid())
		{
			HasTarget = false;
		}
		else
		{
			AGP4BaseBuilding* bref = Cast<AGP4BaseBuilding>(targetActorReference);
			if (bref)
			{
				AttackBuilding(bref);
			}
		}
		damageTimer = 0.0f;
	}

	if (!HasTarget)
	{
		if (savedHasBeenSet)
		{
			SetUnitTargetUnconditional(savedActorReference.Get(), savedActorLocation);
			savedHasBeenSet = false;
		}
		else if (directorReference.IsValid())
		{
			directorReference->RequestTargetForUnit(this);
		}
		else if (spawnerReference.IsValid())
		{
			spawnerReference->RequestTargetForUnit(this);
		}
	}

	AGP4BaseBuilding* closeActorRef = NULL;
	if (AGP4_AIDirector::GetClosestTargetBuilding(&closeActorRef,
	                                              GetWorld()->GetSubsystem<UGP4BuildingsWorldSubsystem>()->Buildings,
	                                              this->GetActorLocation(), InterruptDistance) && (FVector::Distance(
		targetActorLocation, this->GetActorLocation()) > InterruptDistance))
	{
		if (!savedHasBeenSet)
		{
			savedActorLocation = targetActorLocation;
			savedActorReference = targetActorReference;
		}
		SetUnitTargetUnconditional(closeActorRef, closeActorRef->GetActorLocation());
		return;
	}
}

void AGP4_BaseEnemy::UnitTakeDamage(float amount)
{
	Health -= amount;
	OnUnitTakeDamage();
	if (Health <= 0.0f)
	{
		OnUnitDeath();
		this->Destroy();
	}
}

void AGP4_BaseEnemy::AttackBuilding(AGP4BaseBuilding* targetBuilding)
{
	/*if (FVector::Distance(targetBuilding->GetActorLocation(), this->GetActorLocation()) < AttackDistance)
	{
		targetBuilding->TakeDamage(Damage);
	}*/

	TArray<AActor*> temp;
	RangeBubble->GetOverlappingActors(temp, AGP4BaseBuilding::StaticClass());

	if (temp.Contains(targetBuilding))
	{
		targetBuilding->BuildingTakeDamage(Damage);
	}
}


void AGP4_BaseEnemy::SetUnitTargetUnconditional(AActor* targetActor, FVector targetLocation)
{
	targetActorReference = targetActor;
	targetActorLocation = targetLocation;
	SetAIControllerMoveTo(targetActorLocation, targetActor);
	/*UGP4_NodeAIComponent* aicomp = this->GetComponentByClass<UGP4_NodeAIComponent>();
	if (aicomp != nullptr)
	{
		aicomp->SelectTarget(targetActor);
	}*/
	HasTarget = true;
}

void AGP4_BaseEnemy::SetUnitHasCompletedPath()
{
	HasTarget = false;
}
