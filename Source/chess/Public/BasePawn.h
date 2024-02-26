// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "vector"
#include "Chess_GameMode.h"
// #include "Tile.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasePawn.generated.h"


UENUM()
enum class EPawnType : uint8
{
	NONE, // 0
	ROOK, // 1
	KNIGHT, // 2
	BISHOP, // 3
	QUEEN, // 4
	KING, // 5
	PAWN, // 6
};

// Color (Black = -1, Whitee = 1) per cambiare direzione se vincolata, tipo pedoni
UENUM()
enum class EPawnColor : int8
{
	NONE = 0,
	WHITE = 1,
	BLACK = -1
};

/*UENUM()
enum class EPawnMovement : uint8
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	DIAGONAL
};*/


UENUM()
enum class ECardinalDirection : uint8
{
	NORTH,
	NORTHEAST,
	EAST,
	SOUTHEAST,
	SOUTH,
	SOUTHWEST,
	WEST,
	NORTHWEST,
	KNIGHT_TR, // top-right
	KNIGHT_TL, // top-left
	KNIGHT_RT, // TODO gli altri
	KNIGHT_RB,
	KNIGHT_BR,
	KNIGHT_BL,
	KNIGHT_LB,
	KNIGHT_LT
	 // KNIGHT
};

UENUM()
enum class EPawnStatus: int8
{
	ALIVE, // 0
	DEAD // 1
};

UCLASS()
class CHESS_API ABasePawn : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABasePawn();

	/* void SetTileId(const FString TileId);
	FString GetTileId() const; */

	// EPawnMovement GetMovement() const;

	TArray<ECardinalDirection> GetCardinalDirections() const;

	void SetMaxNumberSteps(int32 NumberSteps);
	int32 GetMaxNumberSteps() const;

	void SetColor(EPawnColor PawnColor);
	EPawnColor GetColor() const;

	
	void SetType(EPawnType PawnType);
	EPawnType GetType() const;

	void SetStatus(EPawnStatus PawnStatus);
	EPawnStatus GetStatus() const;

	void SetGridPosition(const double InX, const double InY);
	FVector2D GetGridPosition() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/* UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString TileId; */

	// TODO => FString ID per log




	// Color (Black = -1, Whitee = 1) per cambiare direzione se vincolata, tipo pedoni
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPawnColor Color;

	/* UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPawnMovement Movement; */

	// TODO int8 ?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MaxNumberSteps;

	// Type (e.g. Rook)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPawnType Type;

	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<ECardinalDirection> CardinalDirections;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPawnStatus Status;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D TileGridPosition;

public:	
	// Called every frame
	// virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void SelfDestroy();
};
