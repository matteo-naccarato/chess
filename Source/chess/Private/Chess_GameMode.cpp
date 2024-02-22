// Fill out your copyright notice in the Description page of Project Settings.

#include "Chess_GameMode.h"
#include "GameField.h"
#include "Players/Chess_PlayerController.h"
#include "Players/Chess_HumanPlayer.h"
#include "Players/Chess_RandomPlayer.h"
#include "Players/Chess_MiniMaxPlayer.h"
#include "EngineUtils.h"

AChess_GameMode::AChess_GameMode()
{
	PlayerControllerClass = AChess_PlayerController::StaticClass();
	DefaultPawnClass = AChess_HumanPlayer::StaticClass();
	FieldSize = 8;
}

void AChess_GameMode::BeginPlay()
{
	Super::BeginPlay();

	IsGameOver = false;
	MoveCounter = 0;

	AChess_HumanPlayer* HumanPlayer = Cast<AChess_HumanPlayer>(*TActorIterator<AChess_HumanPlayer>(GetWorld()));

	if (GameFieldClass != nullptr)
	{
		GField = GetWorld()->SpawnActor<AGameField>(GameFieldClass);
		GField->Size = FieldSize;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameFieldClass is null"));
	}

	float CameraPosX = ((GField->TileSize * (FieldSize + ((FieldSize - 1) * GField->NormalizedCellPadding) - (FieldSize - 1))) / 2) - (GField->TileSize / 2);
	FVector CameraPos(CameraPosX, CameraPosX, 1000.0f); // TODO: 1000 da mettere come attributo
	HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());
	
	// Human player = 0
	Players.Add(HumanPlayer);
	// Random player
	auto* AI = GetWorld()->SpawnActor<AChess_RandomPlayer>(FVector(), FRotator());
	// Minimax player
	// auto* AI = GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());

	Players.Add(AI);


	this->ChoosePlayerAndStartGame();
}

void AChess_GameMode::ChoosePlayerAndStartGame()
{
	CurrentPlayer = 0;
	for (int32 i = 0; i < Players.Num(); i++)
	{
		Players[i]->PlayerNumber = i;
		Players[i]->Color = i == CurrentPlayer ? EColor::W : EColor::B;
	}

	MoveCounter += 1;

	Players[CurrentPlayer]->OnTurn();
}

void AChess_GameMode::SetCellPawn(const int32 PlayerNumber, const FVector& SpawnPosition)
{
	

	// -1 to notify draw
	if (PlayerNumber == -1)
	{
		IsGameOver = true;
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("DRAW"));

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
			{
				GField->ResetField();
			}, 3, false);
	}
	else
	{
		if (IsGameOver || PlayerNumber != CurrentPlayer)
			return;

		TurnNextPlayer();
	}

	/*
	
	TODO
	
	*/

	// FVector Location = GField->GetActorLocation() + SpawnPosition + FVector(0, 0, 10);
	// GetWorld()->SpawnActor(B_KingClass, &Location);
	// ABasePawn* BasePawnObj = GetWorld()->SpawnActor<ABasePawn>(B_KingClass, Location, FRotator(0,90,0));
	/*if (BasePawnObj != nullptr)
	{
		// FVector2D xy = GField->GetXYPositionByRelativeLocation(Location);
		// BasePawnObj->SetGridPosition(xy[0], xy[1]);
		const float TileScale = GField->TileSize / 100; // TileScale as attribute
		BasePawnObj->SetActorScale3D(FVector(TileScale * 0.8, TileScale * 0.8, 0.05));
	}*/


	// GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("Set"));


	// UClass* ColorActor = Players[CurrentPlayer]->Color == E

	
}

int32 AChess_GameMode::GetNextPlayer(int32 Player)
{
	Player++;
	if (!Players.IsValidIndex(Player))
	{
		Player = 0;
	}
	return Player;
}

void AChess_GameMode::TurnNextPlayer()
{
	MoveCounter += 1;
	CurrentPlayer = GetNextPlayer(CurrentPlayer);
	// GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("MOVE COUNTER %d"), MoveCounter));
	Players[CurrentPlayer]->OnTurn();
}

std::vector<std::pair<int8, int8>> AChess_GameMode::ShowPossibleMoves(ABasePawn* Pawn, const int8 NewX, const int8 NewY)
{
	std::vector<std::pair<int8, int8>> PossibleMoves;
	std::vector<ECardinalDirection> PawnDirections = Pawn->GetCardinalDirections();
	int8 MaxSteps = Pawn->GetMaxNumberSteps();
	int8 FlagDirection = 0;
	int8 XOffset = 0, YOffset = 0;
	for (const auto& PawnDirection : PawnDirections)
	{
		for (int8 i = 1; i <= MaxSteps; i++)
		{
			XOffset = 0, YOffset = 0;
			switch (PawnDirection)
			{
			case ECardinalDirection::NORTH:
				FlagDirection = 1;
			case ECardinalDirection::SOUTH:
				GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("FlagDirection %d"), FlagDirection));
				FlagDirection = FlagDirection ? FlagDirection : -1;
				XOffset = i * FlagDirection;
				YOffset = 0;
				break; 

			case ECardinalDirection::NORTHEAST:
				FlagDirection = 1;
			case ECardinalDirection::SOUTHWEST:
				FlagDirection = FlagDirection ? FlagDirection : -1;
				XOffset = i * FlagDirection;
				YOffset = i * FlagDirection;
				break;

			case ECardinalDirection::EAST:
				FlagDirection = 1;
			case ECardinalDirection::WEST:
				FlagDirection = FlagDirection ? FlagDirection : -1;
				XOffset = 0;
				YOffset = i * FlagDirection;
				break;

			case ECardinalDirection::NORTHWEST:
				FlagDirection = 1;
			case ECardinalDirection::SOUTHEAST: 
				FlagDirection = FlagDirection ? FlagDirection : -1;
				XOffset = i * FlagDirection;
				YOffset = i * (- FlagDirection);
				break;

			case ECardinalDirection::KNIGHT_TL:
				YOffset = -1;
			case ECardinalDirection::KNIGHT_TR:
				XOffset = 2;
				YOffset = YOffset ? YOffset : 1;
				break;

			case ECardinalDirection::KNIGHT_RT:
				XOffset = 1;
			case ECardinalDirection::KNIGHT_RB:
				XOffset = XOffset ? XOffset : -1;
				YOffset = 2;
				break;

			case ECardinalDirection::KNIGHT_BR:
				YOffset = 1;
			case ECardinalDirection::KNIGHT_BL:
				XOffset = -2;
				YOffset = YOffset ? YOffset : -1;
				break;

			case ECardinalDirection::KNIGHT_LT:
				XOffset = 1;
			case ECardinalDirection::KNIGHT_LB:
				XOffset = XOffset ? XOffset : -1;
				YOffset = -2;
				break;

			} 

			
			//ATile* Tile = GField->GetTileArray()[(NewX + XOffset) * GField->Size + NewY + YOffset];
			// bool EatFlag = Tile->GetTileStatus().PawnColor == EPawnColor::BLACK;
			if (IsValidMove(Pawn, NewX + XOffset, NewY + YOffset, /*EatFlag, */true))
			{
				PossibleMoves.push_back(std::make_pair(NewX + XOffset, NewY + YOffset));
				GField->GetTileArray()[(NewX + XOffset) * GField->Size + NewY + YOffset]->GetStaticMeshComponent()->SetMaterial(0, GField->MaterialGreen);
			}
			FlagDirection = 0;
			
		}
	}


	/* switch (Pawn->GetType())
	{
	case EPawnType::PAWN:
		int8 MaxSteps = Pawn->GetMaxNumberSteps();
		for (int8 i = 1; i <= MaxSteps; i++)
		{
			// TODO, manage eatflag
			if (IsValidMove(Pawn, NewX + i, NewY, false, true))
			{
				PossibleMoves.push_back(std::make_pair(NewX + i, NewY));
				GField->GetTileArray()[(NewX+i) * GField->Size + NewY]->GetStaticMeshComponent()->SetMaterial(0, GField->MaterialGreen);
			}
		}
		break;
	} */
	return PossibleMoves;
}

// TODO make it const
bool AChess_GameMode::IsValidMove(ABasePawn* Pawn, const int8 NewX, const int8 NewY, /*const bool EatFlag,*/ const bool TestFlag)
{
	bool IsValid = false;

	if (NewX >= 0 && NewX < GField->Size
		&& NewY >= 0 && NewY < GField->Size
		&& !(NewX == Pawn->GetGridPosition()[0] && NewY == Pawn->GetGridPosition()[1]))
	{
		TArray<ATile*> TileArray = GField->GetTileArray();
		ATile* NewTile = TileArray[NewX * GField->Size + NewY];
		// it means that colors are -1 and 1 or viceversa
		bool EatFlag = static_cast<int>(NewTile->GetTileStatus().PawnColor) == -static_cast<int>(Pawn->GetColor());
	

		// CurrTile => remove it

		FVector2D NewGridPosition = NewTile->GetGridPosition();
		FVector2D CurrGridPosition = Pawn->GetGridPosition();
	


		// NewGridPosition , CurrPawnPosition
		EPawnColor DirectionFlag = Pawn->GetColor();
		int8 DeltaX = (NewGridPosition[0] - CurrGridPosition[0]) * static_cast<double>(DirectionFlag);
		int8 DeltaY = NewGridPosition[1] - CurrGridPosition[1];



		if ((NewTile->GetTileStatus().EmptyFlag && !EatFlag) || (EatFlag && !NewTile->GetTileStatus().EmptyFlag && (NewTile->GetTileStatus().PawnColor != Pawn->GetColor())))
		{
			switch (Pawn->GetType())
			{
			case EPawnType::PAWN:
				if (EatFlag)
					IsValid = this->CheckDirection(EDirection::DIAGONAL, Pawn, NewGridPosition, CurrGridPosition, TestFlag);
				else
					IsValid = this->CheckDirection(EDirection::FORWARD, Pawn, NewGridPosition, CurrGridPosition, TestFlag);
				break;

			case EPawnType::ROOK:
				IsValid = this->CheckDirection(EDirection::FORWARD, Pawn, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || this->CheckDirection(EDirection::BACKWARD, Pawn, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || this->CheckDirection(EDirection::HORIZONTAL, Pawn, NewGridPosition, CurrGridPosition);
				break;

			case EPawnType::KNIGHT:
				IsValid = this->CheckDirection(EDirection::KNIGHT, Pawn, NewGridPosition, CurrGridPosition);
				break;

			case EPawnType::BISHOP:
				IsValid = this->CheckDirection(EDirection::DIAGONAL, Pawn, NewGridPosition, CurrGridPosition);
				break;

			case EPawnType::QUEEN: // so it works like an OR for Queen and King
			case EPawnType::KING:
				IsValid = this->CheckDirection(EDirection::FORWARD, Pawn, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || this->CheckDirection(EDirection::BACKWARD, Pawn, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || this->CheckDirection(EDirection::HORIZONTAL, Pawn, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || this->CheckDirection(EDirection::DIAGONAL, Pawn, NewGridPosition, CurrGridPosition);
				break;
			}
		}
	}

	return IsValid;
}

bool AChess_GameMode::CheckDirection(const EDirection Direction, ABasePawn* Pawn, const FVector2D NewGridPosition, const FVector2D CurrGridPosition, const bool TestFlag) const
{
	EPawnColor DirectionFlag = Pawn->GetColor();
	int8 DeltaX = (NewGridPosition[0] - CurrGridPosition[0]) * static_cast<double>(DirectionFlag);
	int8 DeltaY = NewGridPosition[1] - CurrGridPosition[1];
	int8 MaxSteps = (Pawn->GetType() == EPawnType::PAWN) ? 1 : Pawn->GetMaxNumberSteps();

	switch (Direction)
	{
	case EDirection::FORWARD:
		if (DeltaY == 0 && DeltaX >= 0 && DeltaX <= Pawn->GetMaxNumberSteps())
		{
			if (!this->IsLineClear(ELine::VERTICAL, CurrGridPosition, DeltaX, DeltaY))
				return false;

			if (Pawn->GetType() == EPawnType::PAWN && !TestFlag)
				Pawn->SetMaxNumberSteps(1);
			
			return true;
		}
		break;

	case EDirection::BACKWARD:
		return DeltaY == 0 && -DeltaX >= 0 && -DeltaX <= Pawn->GetMaxNumberSteps() && IsLineClear(ELine::VERTICAL, CurrGridPosition, DeltaX, DeltaY);
		
	case EDirection::HORIZONTAL:
		return DeltaX == 0 && FMath::Abs(DeltaY) >= 0 && FMath::Abs(DeltaY) <= Pawn->GetMaxNumberSteps() && IsLineClear(ELine::HORIZONTAL, CurrGridPosition, DeltaX, DeltaY);

	case EDirection::DIAGONAL:
		if (FMath::Abs(DeltaX) == FMath::Abs(DeltaY) && FMath::Abs(DeltaX) <= MaxSteps)
		{
			if (!this->IsLineClear(ELine::DIAGONAL, CurrGridPosition, DeltaX, DeltaY))
				return false;

			if (Pawn->GetType() == EPawnType::PAWN)
				Pawn->SetMaxNumberSteps(1);
			
			return true;
		}
		break;

	case EDirection::KNIGHT:
		return (FMath::Abs(DeltaX) == 1 && FMath::Abs(DeltaY) == 2) || (FMath::Abs(DeltaX) == 2 && FMath::Abs(DeltaY) == 1);
	}
	
	return false;
}

bool AChess_GameMode::IsValidTile(const int8 X, const int8 Y) const
{
	return X >= 0 && X < GField->Size
		&& Y >= 0 && X < GField->Size;
}

/*
* return false if there is a pawn along the movement
*/
bool AChess_GameMode::IsLineClear(ELine Line, const FVector2D CurrGridPosition, const int8 DeltaX, const int8 DeltaY) const
{
	switch (Line)
	{
	case ELine::HORIZONTAL:	
		for (int8 YOffset = 1; YOffset < FMath::Abs(DeltaY); YOffset++)
		{
			if (IsValidTile(CurrGridPosition[0], CurrGridPosition[1] + YOffset) 
				&& !GField->TileArray[CurrGridPosition[0] * GField->Size + CurrGridPosition[1] + YOffset * FMath::Sign(DeltaY)]->GetTileStatus().EmptyFlag)
				return false;
		}
		break;

	case ELine::VERTICAL:
		for (int8 XOffset = 1; XOffset < FMath::Abs(DeltaX); XOffset++)
		{
			if (IsValidTile(CurrGridPosition[0] + XOffset * FMath::Sign(DeltaX), CurrGridPosition[1])
				&& !GField->TileArray[(CurrGridPosition[0] + XOffset * FMath::Sign(DeltaX)) * GField->Size + CurrGridPosition[1]]->GetTileStatus().EmptyFlag)
				return false;
		}
		break;

	case ELine::DIAGONAL:
		for (int8 Offset = 1; Offset < FMath::Abs(DeltaX); Offset++)
		{
			if (IsValidTile(CurrGridPosition[0] + Offset * FMath::Sign(DeltaX), CurrGridPosition[1] + Offset * FMath::Sign(DeltaY))
				&& !GField->TileArray[(CurrGridPosition[0] + Offset * FMath::Sign(DeltaX)) * GField->Size + CurrGridPosition[1] + Offset * FMath::Sign(DeltaY)]->GetTileStatus().EmptyFlag)
				return false;
		}
		break; 
	}

	return true;
}