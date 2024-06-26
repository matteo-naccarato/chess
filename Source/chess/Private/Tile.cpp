// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile.h"

// Sets default values
ATile::ATile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	
	SetRootComponent(Scene);
	StaticMeshComponent->SetupAttachment(Scene);

	LetterId = TEXT("");
	NumberId = ChessEnums::NOT_ASSIGNED;

	Status = { 
		nullptr, 
		1, 
		{ 0, 0 }, 
		TArray<ABasePiece*>(),
		EPieceColor::NONE,
		EPieceType::NONE, 
		ChessEnums::NOT_ASSIGNED
	};

	PlayerOwner = ChessEnums::NOT_ASSIGNED;
	TileGridPosition = FVector2D(ChessEnums::NOT_ASSIGNED, ChessEnums::NOT_ASSIGNED);
}

UStaticMeshComponent* ATile::GetStaticMeshComponent() const { return StaticMeshComponent; }

ABasePiece* ATile::GetPiece() const { return Status.Piece; }
void ATile::SetPiece(ABasePiece* Piece) { Status.Piece = Piece; }

FString ATile::GetId() const { return LetterId + FString::Printf(TEXT("%d"), NumberId); }

void ATile::SetLetterId(const FString TileLetter) { LetterId = TileLetter; }
FString ATile::GetLetterId() const { return LetterId; }

void ATile::SetNumberId(const int8 TileNumber) { NumberId = TileNumber; }
int8 ATile::GetNumberId() const { return NumberId; }

void ATile::SetTileStatus(const FTileStatus TileStatus) { Status = TileStatus; }
FTileStatus ATile::GetTileStatus() const { return Status; }

void ATile::SetPlayerOwner(const int32 P_Owner) { PlayerOwner = P_Owner; }
int32 ATile::GetPlayerOwner() const { return PlayerOwner; }

void ATile::SetGridPosition(const double InX, const double InY) { TileGridPosition.Set(InX, InY); }
FVector2D ATile::GetGridPosition() const { return TileGridPosition; }

void ATile::ClearInfo()
{
	SetPlayerOwner(ChessEnums::NOT_ASSIGNED);
	SetTileStatus({ 
		nullptr,
		1, 
		{ 0, 0 }, 
		GetTileStatus().WhoCanGo, 
		EPieceColor::NONE,
		EPieceType::NONE, 
		ChessEnums::NOT_ASSIGNED
	});
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
/* void ATile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

} */

