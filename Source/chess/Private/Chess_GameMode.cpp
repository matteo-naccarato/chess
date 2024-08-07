// Fill out your copyright notice in the Description page of Project Settings.

#include "Chess_GameMode.h"
#include "GameField.h"
#include "Players/Chess_PlayerInterface.h"
#include "Players/Chess_PlayerController.h"
#include "Players/Chess_HumanPlayer.h"
#include "Players/Chess_RandomPlayer.h"
#include "Players/Chess_MiniMaxPlayer.h"
#include "EngineUtils.h"


AChess_GameMode::AChess_GameMode()
{
	PlayerControllerClass = AChess_PlayerController::StaticClass();
	DefaultPawnClass = AChess_HumanPlayer::StaticClass();
	PawnPromotionType = EPieceType::NONE;
	LastGridPosition = FVector2D(ChessEnums::NOT_ASSIGNED, ChessEnums::NOT_ASSIGNED);
	LastPawnMoveHappened = 0;
	LastCaptureHappened = 0;
	GameInstance = Cast<UChess_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

	/* SOUNDS */
	GameStartSound			= CreateDefaultSubobject<USoundBase>(TEXT("Game Start Sound"));
	MoveSound				= CreateDefaultSubobject<USoundBase>(TEXT("Move Sound"));
	CastlingSound			= CreateDefaultSubobject<USoundBase>(TEXT("Castling Sound"));
	CaptureSound			= CreateDefaultSubobject<USoundBase>(TEXT("Capture Sound"));
	CheckSound				= CreateDefaultSubobject<USoundBase>(TEXT("Check Sound"));
	GameOverCheckmateSound	= CreateDefaultSubobject<USoundBase>(TEXT("Game Over Checkmate Sound"));
	GameOverDrawSound		= CreateDefaultSubobject<USoundBase>(TEXT("Game Over Draw Sound"));
}

void AChess_GameMode::BeginPlay()
{
	Super::BeginPlay();

	// Init variables
	IsGameOver			= false;
	bIsHumanPlaying		= false;
	ReplayInProgress	= 0;
	MoveCounter			= 0;
	CheckFlag			= EPieceColor::NONE;
	MatchStatus			= EMatchResult::NONE;

	// Spawn GameField
	if (GameFieldClass && GameInstance)
	{
		GField = GetWorld()->SpawnActor<AGameField>(GameFieldClass);

		// Get and Set Human Player (Camera) Location
		AChess_HumanPlayer* HumanPlayer = Cast<AChess_HumanPlayer>(*TActorIterator<AChess_HumanPlayer>(GetWorld()));
		float CameraPosX = ((GField->TileSize * (GField->Size + ((GField->Size - 1) * GField->NormalizedCellPadding) - (GField->Size - 1))) / 2) - (GField->TileSize / 2);
		FVector CameraPos(CameraPosX, CameraPosX, CAMERA_POS_Z); 
		HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());
	

		/* PLAYER CREATION */
		// Human player at INDEX 0
		// It is always in the game (it is linked with the camera). 
		// If it does not play, it woks like a spectator
		IChess_PlayerInterface* AI_1 = nullptr;
		IChess_PlayerInterface* AI_2 = nullptr;
		FString TextPlayer_1 = TEXT("");
		FString TextPlayer_2 = TEXT("");

		switch (GameInstance->GetMatchMode())
		{
		case EMatchMode::HUMAN_RANDOM:
			AI_1 = GetWorld()->SpawnActor<AChess_RandomPlayer>(FVector(), FRotator());
			TextPlayer_2 = UChess_GameInstance::RANDOM;
		case EMatchMode::HUMAN_MINIMAX_BASE:
			AI_1 = AI_1 ? AI_1 : GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());
			TextPlayer_1 = UChess_GameInstance::HUMAN;
			TextPlayer_2 = (TextPlayer_2 != TEXT("")) ? TextPlayer_2 : UChess_GameInstance::MINIMAX;
			if (HumanPlayer && AI_1)
			{
				HumanPlayer->bIsActivePlayer = true;
				AI_1->bIsActivePlayer = true;

				bIsHumanPlaying = true;
				Players.Add(HumanPlayer);	// white
				Players.Add(AI_1);			// black
			}
			break;

		case EMatchMode::RANDOM_RANDOM:
			TextPlayer_2 = UChess_GameInstance::RANDOM_2;
			AI_2 = GetWorld()->SpawnActor<AChess_RandomPlayer>(FVector(), FRotator());
		case EMatchMode::RANDOM_MINIMAX_BASE:
			TextPlayer_1 = UChess_GameInstance::RANDOM_1;
			TextPlayer_2 = (TextPlayer_2 != TEXT("")) ? TextPlayer_2 : UChess_GameInstance::MINIMAX_2;
			AI_1 = GetWorld()->SpawnActor<AChess_RandomPlayer>(FVector(), FRotator());
			AI_2 = AI_2 ? AI_2 : GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());
		case EMatchMode::MINIMAX_BASE_MINIMAX_BASE:
			TextPlayer_1 = (TextPlayer_1 != TEXT("")) ? TextPlayer_1 : UChess_GameInstance::MINIMAX_1;
			TextPlayer_2 = (TextPlayer_2 != TEXT("")) ? TextPlayer_2 : UChess_GameInstance::MINIMAX_2;
			AI_1 = AI_1 ? AI_1 : GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());
			AI_2 = AI_2 ? AI_2 : GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());

			if (HumanPlayer && AI_1 && AI_2)
			{
				AI_1->bIsActivePlayer = true;
				AI_2->bIsActivePlayer = true;
				HumanPlayer->bIsActivePlayer = false;

				Players.Add(AI_1);			// white
				Players.Add(AI_2);			// black
				Players.Add(HumanPlayer);	// spectator
			}
			break;


		case EMatchMode::MINIMAX_BASE_MINIMAX_PESTO:
			TextPlayer_1 = UChess_GameInstance::MINIMAX;
			TextPlayer_2 = UChess_GameInstance::MINIMAX_PESTO;
			AI_1 = AI_1 ? AI_1 : GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());
			AI_2 = AI_2 ? AI_2 : GetWorld()->SpawnActor<AChess_MiniMaxPlayer>(FVector(), FRotator());
			Cast<AChess_MiniMaxPlayer>(AI_1)->SetEvaluationFunction(EEValuationFunction::BASE);
			Cast<AChess_MiniMaxPlayer>(AI_2)->SetEvaluationFunction(EEValuationFunction::PESTO);
			if (HumanPlayer && AI_1 && AI_2)
			{
				AI_1->bIsActivePlayer = true;
				AI_2->bIsActivePlayer = true;
				HumanPlayer->bIsActivePlayer = false;

				Players.Add(AI_1);			// white
				Players.Add(AI_2);			// black
				Players.Add(HumanPlayer);	// spectator
			}
			break;
		}
		
		GameInstance->SetPlayerText_1(TextPlayer_1);
		GameInstance->SetPlayerText_2(TextPlayer_2);
		GameInstance->SetGamesCounter(0);
		GameInstance->SetDrawsCounter(0);
		GameInstance->SetScorePlayer_1(0);
		GameInstance->SetScorePlayer_2(0);
		GameInstance->Minutes = 0;
		GameInstance->Seconds = 0;
		

		// Create replay widget
		UWorld* World = GetWorld();
		if (World && ReplayWidgetRef)
		{
			ReplayWidget = CreateWidget<UUserWidget>(World, ReplayWidgetRef, FName("Replay"));
			ReplayWidget->AddToViewport(0);
		}

		// Ready to start the game
		this->ChoosePlayerAndStartGame();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameFieldClass OR GameInstance is null"));
	}
}


/*
 * Function: ChoosePlayerAndStartGame
 * ----------------------------
 * Assign player numbers and colors, then start the game
 */
void AChess_GameMode::ChoosePlayerAndStartGame()
{
	CurrentPlayer = 0; // its value will change from 0 (white player) to 1 (black player) and viceversa during the game 
	for (int8 i = 0; i < Players.Num(); i++)
	{
		if (Players[i]->bIsActivePlayer)
		{
			Players[i]->PlayerNumber = i;
			Players[i]->Color = i == CurrentPlayer ? EPieceColor::WHITE : EPieceColor::BLACK;
		}
	}

	CheckFlag = EPieceColor::NONE;
	MatchStatus = EMatchResult::NONE;
	MoveCounter += 1;

	// Operation to init data strcture
	InitTurn();

	GameInstance->Minutes = 0;
	GameInstance->Seconds = 0;

	GetWorld()->GetTimerManager().SetTimer(StopwatchTimerHandle, 
		GameInstance, 
		&UChess_GameInstance::IncrementStopwatch, 
		1.0f, 
		true);
	
	if (Players.Num() >= MIN_NUMBER_SPAWN_PLAYERS && Players[FMath::Abs(CurrentPlayer - 1)] && Players[CurrentPlayer])
	{
		Players[FMath::Abs(CurrentPlayer - 1)]->IsMyTurn = false;
		Players[CurrentPlayer]->IsMyTurn = true;
		Players[CurrentPlayer]->OnTurn();
	}
}


/*
 * Function: GetNextPlayer
 * ----------------------------
 * Returns next player index. Active players are positioned at index 0 and 1.
 * This function switch between these two values.
 *	 e.g.
 *		Player = 0		->		return 1
 *		Player = 1		->		return 0
 *
 * @param Player	int32	Number of the current player
 *
 * @return			int32	Next player index
 */
int32 AChess_GameMode::GetNextPlayer(int32 Player)
{
	return FMath::Abs(Player - 1);
}


/*
 * Function: TurnNextPlayer
 * ----------------------------
 * Start the next player turn
 */
void AChess_GameMode::TurnNextPlayer()
{
	MoveCounter += 1;
	CurrentPlayer = GetNextPlayer(CurrentPlayer);

	if (Players.IsValidIndex(CurrentPlayer))
	{
		Players[CurrentPlayer]->IsMyTurn = true;
		Players[CurrentPlayer]->OnTurn();
	}
}


/*
 * Function: InitTurn
 * ----------------------------
 * Elaborate and execute initialisation operations (clearing info from the arrays, computing new possible moves of each piece and computing check situation)
 * to do at the begin of a new turn
 */
void AChess_GameMode::InitTurn()
{
	// Clear possible moves
	for (auto& InnerArray : TurnPossibleMoves)
		InnerArray.Empty();
	TurnPossibleMoves.Empty();

	// Clear tile status (AttackableFrom and WhoCanGo variables)
	for (auto& Tile : GField->TileArray)
	{
		FTileStatus TileStatus = Tile->GetTileStatus();
		TileStatus.AttackableFrom[0] = 0; TileStatus.AttackableFrom[1] = 0;
		TileStatus.WhoCanGo.Empty();
		Tile->SetTileStatus(TileStatus);
	}

	WhitePiecesCanMove.Empty();
	BlackPiecesCanMove.Empty();

	// Pre-compute check needed to verify if castling condition is satisfied or not
	IsCheck();

	for (const auto& Piece : GField->PieceArray)
	{
		TArray<std::pair<int8, int8>> Tmp = ShowPossibleMoves(Piece, false, true, true);
		TurnPossibleMoves.Add(Tmp);
		if (Tmp.Num() > 0)
		{
			switch (Piece->GetColor())
			{
			case EPieceColor::WHITE: WhitePiecesCanMove.Add(std::make_pair(Piece->GetPieceNum(), Tmp)); break;
			case EPieceColor::BLACK: BlackPiecesCanMove.Add(std::make_pair(Piece->GetPieceNum(), Tmp)); break;
			}
		}
	}

	CheckFlag = CheckKingsUnderAttack();

	if (GField->TileRestoreMaterialCoordinates != FVector2D(-1, -1))
	{
		int8 x = GField->TileRestoreMaterialCoordinates[0];
		int8 y = GField->TileRestoreMaterialCoordinates[1];
		UMaterialInterface* Material = ((x + y) % 2) ?
			GField->MaterialsLight[ETileMaterialType::STANDARD] :
			GField->MaterialsDark[ETileMaterialType::STANDARD];
		GField->TileArray[x * GField->Size + y]->GetStaticMeshComponent()->SetMaterial(0, Material);
		GField->TileRestoreMaterialCoordinates = FVector2D(-1, -1);
	}
}


/*
 * Function: EndTurn
 * ----------------------------
 * Handle the end of turn of the player passed as parameter
 *
 * @param PlayerNumber			int32	Number of the player whose turn ends
 * @param PiecePromotionFlag	bool	Flag to notify piece promotion, required in the computation of the move name
 */
void AChess_GameMode::EndTurn(const int32 PlayerNumber, const bool PiecePromotionFlag)
{
	// -1 to notify end game
	if (PlayerNumber == -1)
	{
		IsGameOver = true;
		// Increment needed to perform replay last two moves properly
		MoveCounter++;

		// GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("END - %d"), MatchStatus));
		
		GetWorldTimerManager().ClearTimer(StopwatchTimerHandle);

		// 0 => white player
		// 1 => black player
		int8 WinningPlayer = -1; 
		int8 KingX = -1;
		int8 KingY = -1;
		UMaterialInterface* MaterialCheckmated = nullptr;
		// End game events
		switch (MatchStatus)
		{
		case EMatchResult::WHITE:
			WinningPlayer = 1;
			KingX = GField->PieceArray[KingWhitePieceNum]->GetGridPosition()[0];
			KingY = GField->PieceArray[KingWhitePieceNum]->GetGridPosition()[1];
			MaterialCheckmated = ((KingX + KingY) % 2) ?
				GField->MaterialsLight[ETileMaterialType::CHECKMATE] :
				GField->MaterialsDark[ETileMaterialType::CHECKMATE];
		case EMatchResult::BLACK:
			WinningPlayer = WinningPlayer != -1 ? WinningPlayer : 0;
			KingX = KingX != -1 ? KingX : GField->PieceArray[KingBlackPieceNum]->GetGridPosition()[0];
			KingY = KingY != -1 ? KingY : GField->PieceArray[KingBlackPieceNum]->GetGridPosition()[1];
			MaterialCheckmated = MaterialCheckmated ? MaterialCheckmated :
				((KingX + KingY) % 2) ?
				GField->MaterialsLight[ETileMaterialType::CHECKMATE] :
				GField->MaterialsDark[ETileMaterialType::CHECKMATE];

			UGameplayStatics::PlaySound2D(GetWorld(), GameOverCheckmateSound, 1, 1, 0, NULL, false, true);
			if (Players.IsValidIndex(WinningPlayer))
			{
				Players[WinningPlayer]->OnWin();
				for (int32 i = 0; i < Players.Num(); i++)
					if (Players[i]->bIsActivePlayer && i != WinningPlayer)
						Players[i]->OnLose();
			}
			break;
		default:
			UGameplayStatics::PlaySound2D(GetWorld(), GameOverDrawSound, 1, 1, 0, NULL, false, true);
			for (int32 i = 0; i < Players.Num(); i++)
				if (Players[i]->bIsActivePlayer || Players.Num() == MIN_NUMBER_SPAWN_PLAYERS)
					Players[i]->OnDraw();
			if (GameInstance)
				GameInstance->IncrementDrawsCounter();
			break;
		}

		if (MaterialCheckmated)
		{
			GField->TileArray[KingX * GField->Size + KingY]->GetStaticMeshComponent()->SetMaterial(0, MaterialCheckmated);
			GField->TileRestoreMaterialCoordinates = FVector2D(KingX, KingY);
		}


		if (GameInstance)
		{
			GameInstance->IncrementGamesCounter();
			
			// Save statistics on file
			FString FilePath = FPaths::ProjectDir() + STATISTICS_DIRECTORY_NAME;
			switch (GameInstance->GetMatchMode())
			{
			case EMatchMode::HUMAN_RANDOM:					FilePath += FILENAME_HUMAN_RANDOM;					break;
			case EMatchMode::HUMAN_MINIMAX_BASE:			FilePath += FILENAME_HUMAN_MINIMAX_BASE;			break;
			case EMatchMode::RANDOM_RANDOM:					FilePath += FILENAME_RANDOM_RANDOM;					break;
			case EMatchMode::RANDOM_MINIMAX_BASE:			FilePath += FILENAME_RANDOM_MINIMAX_BASE;			break;
			case EMatchMode::MINIMAX_BASE_MINIMAX_BASE:		FilePath += FILENAME_MINIMAX_BASE_MINIMAX_BASE;		break;
			case EMatchMode::MINIMAX_BASE_MINIMAX_PESTO:	FilePath += FILENAME_MINIMAX_BASE_MINIMAX_PESTO;	break;
			}

			bool bSuccess = false; FString OutInfoMessage = TEXT("");
			SaveGameOnFile(FilePath, bSuccess, OutInfoMessage);
		}

		if (!bIsHumanPlaying)
		{
			// Timer to reset the field when only the two AIs are playing (Human is not playing, he is just a spectator)
			FTimerHandle ResetTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(ResetTimerHandle, [&]()
				{
					GField->ResetField(true);
				}, 3, false); 
		}
	
	}
	else
	{
		if (IsGameOver || PlayerNumber != CurrentPlayer)
			return;

		if (Players.IsValidIndex(PlayerNumber) && Players.IsValidIndex(FMath::Abs(CurrentPlayer - 1)))
		{
			Players[PlayerNumber]->IsMyTurn = false;

			// Clean opponent's attackable tiles, they will be overwritten the next turn,
			// so the previous state is useless
			for (auto& InnerTArray : Players[FMath::Abs(CurrentPlayer - 1)]->AttackableTiles)
				InnerTArray.Empty();
			Players[FMath::Abs(CurrentPlayer - 1)]->AttackableTiles.Empty();


			// Clear AttackableFrom and WhoCanGo attribute of each tile
			for (ATile* Tile : GField->TileArray)
			{
				FTileStatus TileStatus = Tile->GetTileStatus();
				TileStatus.AttackableFrom[FMath::Abs(CurrentPlayer - 1)] = 0;
				TileStatus.WhoCanGo.Empty();
				Tile->SetTileStatus(TileStatus);
			}


			// Save position (X,Y) of each piece in order to store current board situation
			//	the index in the BoardSaving TArray identifies each piece
			//	e.g. White rook at bottom-left is at index 0,
			//	     White knight next to it is at 1,
			//		 ...
			//  e.g. [{0,0}, {2,0}, {1,3}, ...] means the Bottom-left white rook is at (0,0), the knight next to it is at (2,0), ...
			TArray<FPieceSaving> BoardSaving;
			for (const auto& Piece : GField->PieceArray)
			{
				BoardSaving.Add({
					static_cast<int8>(Piece->GetGridPosition()[0]),
					static_cast<int8>(Piece->GetGridPosition()[1]),
					Piece->GetStatus()
					});
			}

			// Save current board and add it to the game history
			std::get<0>(CurrentBoard) = BoardSaving;
			GameSaving.Add(std::make_tuple(BoardSaving, CastlingInfoWhite, CastlingInfoBlack));


			// Operation to init data strcture
			InitTurn();


			if (CheckFlag != EPieceColor::NONE)
			{
				GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Yellow, FString::Printf(TEXT("King under check | %d"), CheckFlag));
				UGameplayStatics::PlaySound2D(GetWorld(), CheckSound, 1, 1, 0, NULL, false, true);

				const ABasePiece* KingUnderCheck = CheckFlag == EPieceColor::WHITE ? GField->PieceArray[KingWhitePieceNum] : GField->PieceArray[KingBlackPieceNum];
				int8 x = KingUnderCheck->GetGridPosition()[0];
				int8 y = KingUnderCheck->GetGridPosition()[1];
				UMaterialInterface* Material = ((x + y) % 2) ?
					GField->MaterialsLight[ETileMaterialType::CHECK] :
					GField->MaterialsDark[ETileMaterialType::CHECK];
				GField->TileArray[x * GField->Size + y]->GetStaticMeshComponent()->SetMaterial(0, Material);
				GField->TileRestoreMaterialCoordinates = FVector2D(x, y);
			}


			// Match situation
			MatchStatus = ComputeMatchResult(WhitePiecesCanMove, BlackPiecesCanMove);

			if (LastPiece->GetType() == EPieceType::PAWN)
				LastPawnMoveHappened = MoveCounter;
			if (LastEatFlag)
				LastCaptureHappened = MoveCounter;

			ReplayManager::AddToReplay(this, LastPiece, LastEatFlag, PiecePromotionFlag);

			// New turn
			if (MatchStatus != EMatchResult::NONE)
				EndTurn(-1);
			else
				TurnNextPlayer();
		}
	}
}


/*
 * Function: IsCheck
 * ----------------------------
 * If no parameters are passed the check situation is computed calculating the attackable tiles
 * and verifying if a king is under attack, after this it is assigned to the gamemode attribute.
 * Otherwise, the new check situation obtained by simulating the move of Piece on [NewX, NewY] is evaluated
 *
 * @param Piece	ABasePiece* = nullptr	Piece to move on new x and new y
 * @param NewX	const int8 = -1			New X position of the piece to move
 * @param NewY	const int8 = -1			New Y position of the piece to move
 *
 * @return		EPieceColor				Check situation
 */
EPieceColor AChess_GameMode::IsCheck(ABasePiece* Piece, const int8 NewX, const int8 NewY, const bool CastlingFlag)
{
	EPieceColor ColorAttacker = CurrentPlayer ? EPieceColor::BLACK : EPieceColor::WHITE;

	// If no parameters are passed, calculate check situation by computing attackable tiles and evaluating MatchStatus
	if (Piece == nullptr)
	{
		ComputeAttackableTiles();

		if (MatchStatus == EMatchResult::NONE)
			CheckFlag = CheckKingsUnderAttack();

		return CheckFlag;
	}
	else
	{
		// Evaluate the new situation by simulating the move of Piece on [NewX, NewY]
		if (GField->IsValidTile(NewX, NewY)
			&& IsValidMove(Piece, NewX, NewY, false, false, CastlingFlag))
		{
			// Backup old tile and new tile
			FVector2D Backup_OldPosition = Piece->GetGridPosition();
			ATile* OldTile = GField->TileArray[Backup_OldPosition[0] * GField->Size + Backup_OldPosition[1]];
			ATile* NewTile = GField->TileArray[NewX * GField->Size + NewY];

			ABasePiece* Backup_OldPawn = OldTile->GetPiece();
			int32 Backup_OldPlayerOwner = OldTile->GetPlayerOwner();
			FTileStatus Backup_OldTileStatus = OldTile->GetTileStatus();

			int32 Backup_NewPlayerOwner = NewTile->GetPlayerOwner();
			FTileStatus Backup_NewTileStatus = NewTile->GetTileStatus();

			ABasePiece* PieceToEat = NewTile->GetPiece();
			FVector2D Backup_PieceToEatGridPosition;
			if (PieceToEat)
			{
				Backup_PieceToEatGridPosition = PieceToEat->GetGridPosition();
				PieceToEat->SetStatus(EPieceStatus::DEAD);
				PieceToEat->SetGridPosition(ChessEnums::NOT_ASSIGNED, ChessEnums::NOT_ASSIGNED);
			}

			// Backup tile status
			TArray<std::pair<std::pair<int8, int8>, FTileStatus>> TileStatusBackup;
			for (ATile* Tile : GField->TileArray)
			{
				int8 X = Tile->GetGridPosition()[0];
				int8 Y = Tile->GetGridPosition()[1];
				FTileStatus TileStatus = Tile->GetTileStatus();
				TileStatusBackup.Add(std::make_pair(std::make_pair(X, Y), TileStatus));

				TileStatus.AttackableFrom[0] = 0; TileStatus.AttackableFrom[1] = 0;
				Tile->SetTileStatus(TileStatus);
			}

			// Simulate move
			Piece->Move(OldTile, NewTile, true);

			// Compute attackable tile with the new move
			ComputeAttackableTiles();

			EPieceColor OldCheckFlag = CheckFlag;
			EPieceColor NewCheckFlag = CheckKingsUnderAttack();

			// Undo and restore backed-up info
			for (const auto& pair : TileStatusBackup)
				GField->TileArray[pair.first.first * GField->Size + pair.first.second]->SetTileStatus(pair.second);

			OldTile->SetPlayerOwner(Backup_OldPlayerOwner);
			OldTile->SetTileStatus(Backup_OldTileStatus);

			NewTile->SetPlayerOwner(Backup_NewPlayerOwner);
			NewTile->SetTileStatus(Backup_NewTileStatus);
			Piece->SetGridPosition(Backup_OldPosition[0], Backup_OldPosition[1]);

			if (PieceToEat)
			{
				PieceToEat->SetStatus(EPieceStatus::ALIVE);
				PieceToEat->SetGridPosition(Backup_PieceToEatGridPosition[0], Backup_PieceToEatGridPosition[1]);
			}

			CheckFlag = OldCheckFlag;

			return NewCheckFlag;
		}
		return CheckFlag;
	}
}


/*
 * Function: ComputeAttackableTiles
 * ----------------------------
 * Compute all attackable tiles based on the current situation of data structure
 *		( GameMode->GField->TileArray, GameMode->GField->PieceArray ).
 * Update the AttackableTileStatus attribute of each Tile where necessary
 */
void AChess_GameMode::ComputeAttackableTiles()
{
	// Reset AttackableFrom info
	for (auto& Tile : GField->TileArray)
	{
		FTileStatus TileStatus = Tile->GetTileStatus();
		TileStatus.AttackableFrom[0] = 0; TileStatus.AttackableFrom[1] = 0;
		Tile->SetTileStatus(TileStatus);
	}

	for (const auto& Piece : GField->PieceArray)
	{
		if (Piece->GetStatus() == EPieceStatus::ALIVE)
		{
			ShowPossibleMoves(Piece, true, false);
		}
	}
}


/*
 * Function: CheckKingUnderAttack
 * ----------------------------
 * Look if each king is on a tile which is attackable from an opponent piece
 *
 * @return	 EPieceColor		Color of the king under check (NONE, WHITE, BLACK, BOTH). BOTH means a NOT valid situation
 */
EPieceColor AChess_GameMode::CheckKingsUnderAttack() const
{
	EPieceColor TmpCheckFlag = EPieceColor::NONE;
	if (GField->PieceArray.IsValidIndex(KingWhitePieceNum)
		&& GField->PieceArray.IsValidIndex(KingBlackPieceNum))
	{
		for (const auto& King : {
			GField->PieceArray[KingWhitePieceNum],
			GField->PieceArray[KingBlackPieceNum] })
		{
			FVector2D KingXY = King->GetGridPosition();
			int8 OpponentIdx = (static_cast<int>(King->GetColor()) == 1) ? 1 : 0;
			if (King->GetStatus() == EPieceStatus::ALIVE && GField->IsValidTile(KingXY[0], KingXY[1]))
			{
				if (GField->TileArray[KingXY[0] * GField->Size + KingXY[1]]->GetTileStatus().AttackableFrom[OpponentIdx])
				{
					if (TmpCheckFlag == EPieceColor::NONE)
						TmpCheckFlag = King->GetColor();
					else
						TmpCheckFlag = EPieceColor::BOTH; // Error, kings cannot be under check at the same time
				}
			}
		}
	}

	return TmpCheckFlag;
}


/*
 * Function: ComputeMatchResult
 * ----------------------------
 * Compute match result based on white and black pieces which can be moved, check situation and draw conditions
 *
 * @param WhitePieces	TArray<std::pair<int8, TArray<std::pair<int8, int8>>>>&		Reference to white pieces which can be moved.
 *																					Each element of the TArray is like <piece_number, <new_x, new_y>>
 * @param BlackPieces	TArray<std::pair<int8, TArray<std::pair<int8, int8>>>>&		Reference to black pieces which can be moved.
 *																					Each element of the TArray is like <piece_number, <new_x, new_y>
 * 
 * @return				EMatchResult												Match result computed
 */
EMatchResult AChess_GameMode::ComputeMatchResult(TArray<std::pair<int8, TArray<std::pair<int8, int8>>>>& WhitePieces, TArray<std::pair<int8, TArray<std::pair<int8, int8>>>>& BlackPieces)
{
	EMatchResult Result = EMatchResult::NONE;

	if (WhitePieces.Num() == 0)
		Result = CheckFlag == EPieceColor::WHITE ? EMatchResult::WHITE : EMatchResult::STALEMATE;
	else if (BlackPieces.Num() == 0)
		Result = CheckFlag == EPieceColor::BLACK ? EMatchResult::BLACK : EMatchResult::STALEMATE;
	else if (SameConfigurationBoard(SAME_CONFIGURATION_BOARD_RULE))
		Result = EMatchResult::FIVEFOLD_REPETITION;
	else if (SeventyFive_MoveRule())
		Result = EMatchResult::SEVENTY_FIVE_MOVE_RULE;
	else if (ImpossibilityToCheckmate())
		Result = EMatchResult::INSUFFICIENT_MATERIAL;

	return Result;
}


/*
 * Function: ShowPossibleMoves
 * ----------------------------
 * Compute the eligible moves of the chess piece passed as parameter
 *
 * @param Piece								ABasePiece*							Piece on which to calculate the eligible moves
 * @param ConsiderOnlyAttackableTiles		bool = false						Flag to determine if only the attackable tiles should be taken into account 
 *																				(possible tiles to move on and attackable tiles are different for pawns)
 * @param CheckCheckFlag					bool = false						Flag to determine if checking the new check situation should be evaluated
 * @param UpdateWhoCanGoFlag				bool = false						Flag to notify if updating WhoCanGo: FTileStatus, 
 *																				attribute of the tile taken into account should be updated with the current piece
 *
 * @return									TArray<std::pair<int8, int8>>		TArray made of new possible X,Y of the chess piece
 */
TArray<std::pair<int8, int8>> AChess_GameMode::ShowPossibleMoves(ABasePiece* Piece, const bool ConsiderOnlyAttackableTiles, const bool CheckCheckFlag, const bool UpdateWhoCanGoFlag)
{
	TArray<std::pair<int8, int8>> PossibleMoves;
	if (Piece && Piece->GetStatus() == EPieceStatus::ALIVE)
	{
		FVector2D CurrPieceGridPosition = Piece->GetGridPosition();
		const int8 X = CurrPieceGridPosition[0];
		const int8 Y = CurrPieceGridPosition[1];

		TArray<ECardinalDirection> PieceDirections = Piece->GetCardinalDirections();
		int8 MaxSteps = Piece->GetMaxNumberSteps();
		int8 XOffset = 0, YOffset = 0;

		for (const auto& PieceDirection : PieceDirections)
		{  
			for (int8 i = 1; i <= MaxSteps; i++)
			{
				std::pair<int8, int8> Offsets = Piece->GetXYOffset(i, PieceDirection);
				XOffset = Offsets.first;
				YOffset = Offsets.second;

				// Evaluate if this move is valid or not
				if (IsValidMove(Piece, X + XOffset, Y + YOffset, ConsiderOnlyAttackableTiles, CheckCheckFlag))
				{
					// Add the VALID move to result TArray
					PossibleMoves.Add(std::make_pair(X + XOffset, Y + YOffset));
					FTileStatus TileStatus = GField->TileArray[(X + XOffset) * GField->Size + Y + YOffset]->GetTileStatus();

					// Check needed to avoid pawns eat on straight line
					if (!(Piece->GetType() == EPieceType::PAWN && PieceDirection == ECardinalDirection::NORTH))
					{
						// Index 0 means attackable from whites
						// Index 1 means attackable from blacks 
						TileStatus.AttackableFrom[(static_cast<int>(Piece->GetColor()) == 1) ? 0 : 1] += 1;
					}
					if (UpdateWhoCanGoFlag)
						TileStatus.WhoCanGo.Add(Piece);

					GField->TileArray[(X + XOffset) * GField->Size + Y + YOffset]->SetTileStatus(TileStatus);
				}
				else {
					// If the piece cannot do a move with n steps in a direction (due to the presence of another piece),
					//  it will not do it with n+1 steps in that direction
					if (GField->IsValidTile(X + XOffset, Y + YOffset) && !GField->TileArray[(X + XOffset) * GField->Size + Y + YOffset]->GetTileStatus().EmptyFlag)
						break;
				}
			}
		}

		// Castling handling
		if (Piece->GetType() == EPieceType::KING)
		{
			// short castling and long castling
			for (ECardinalDirection Direction : { ECardinalDirection::EAST, ECardinalDirection::WEST })
			{
				std::pair<int8, int8> Offsets = Piece->GetXYOffset(2, Direction);
				XOffset = Offsets.first;
				YOffset = Offsets.second;
				// Evaluate if this move is valid or not
				if (IsValidMove(Piece, X + XOffset, Y + YOffset, ConsiderOnlyAttackableTiles, CheckCheckFlag, true))
				{
					// Add the VALID move to result TArray
					PossibleMoves.Add(std::make_pair(X + XOffset, Y + YOffset));
				}
			}
		}
		
	}

	return PossibleMoves;
}


/*
 * Function: IsValidMove
 * ----------------------------
 * Compute if a move (specified through parameters) is valid or not (following the rule of chess game)
 *
 * @param Piece							ABasePiece*			Piece to try to move on new x and new y 
 * @param NewX							const int8			New x position of the piece
 * @param NewY							const int8			New y position of the piece
 * @param ConsiderOnlyAttackableTiles	const bool = false	Determine if only the attackable tiles should be taken into account 
 *															(possible tiles to move on and attackable tiles are different for pawns)
 * @param CheckCheckFlag				const bool = true	Determine if checking check state after this move is required
 * @param CastlingFlag					const bool = false	Determine if checking castling situation is required
 * 
 * @return								bool				Determine if a move is valid or not
 */
bool AChess_GameMode::IsValidMove(ABasePiece* Piece, const int8 NewX, const int8 NewY, const bool ConsiderOnlyAttackableTiles, const bool CheckCheckFlag, const bool CastlingFlag)
{
	bool IsValid = false;

	if (Piece
		&& GField->IsValidTile(NewX, NewY)
		&& !(NewX == Piece->GetGridPosition()[0] && NewY == Piece->GetGridPosition()[1]))
	{
		ATile* NewTile = GField->TileArray[NewX * GField->Size + NewY];

		// If the colors of the pieces on the tile are -1 and 1 or viceversa, 
		//  it means that a piece is eating another one
		bool EatFlag = static_cast<int>(NewTile->GetTileStatus().PieceColor) == -static_cast<int>(Piece->GetColor());
	
		FVector2D NewGridPosition = NewTile->GetGridPosition();
		FVector2D CurrGridPosition = Piece->GetGridPosition();
	
		EPieceColor DirectionFlag = Piece->GetColor();
		int8 DeltaX = (NewGridPosition[0] - CurrGridPosition[0]); 
		int8 DeltaY = NewGridPosition[1] - CurrGridPosition[1];

		// Compute the eligibily of the move based on piece type and starting/ending grid position
		if ((NewTile->GetTileStatus().EmptyFlag && !EatFlag) 
			|| (EatFlag && !NewTile->GetTileStatus().EmptyFlag && (NewTile->GetTileStatus().PieceColor != Piece->GetColor())))
		{
			// Perform checks based on directions which the piece can perform
			switch (Piece->GetType())
			{
			case EPieceType::PAWN:
				if (EatFlag || ConsiderOnlyAttackableTiles)
					IsValid = Piece->CheckDirection(GField, EDirection::DIAGONAL, NewGridPosition, CurrGridPosition);
				else
					IsValid = Piece->CheckDirection(GField, EDirection::FORWARD, NewGridPosition, CurrGridPosition);
				break;

			case EPieceType::ROOK:
				IsValid = Piece->CheckDirection(GField, EDirection::FORWARD, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || Piece->CheckDirection(GField, EDirection::BACKWARD, NewGridPosition, CurrGridPosition);
				IsValid = IsValid || Piece->CheckDirection(GField, EDirection::HORIZONTAL, NewGridPosition, CurrGridPosition);
				break;

			case EPieceType::KNIGHT:
				IsValid = Piece->CheckDirection(GField, EDirection::KNIGHT, NewGridPosition, CurrGridPosition);
				break;

			case EPieceType::BISHOP:
				IsValid = Piece->CheckDirection(GField, EDirection::DIAGONAL, NewGridPosition, CurrGridPosition);
				break;

			case EPieceType::QUEEN: // so it works like an OR for Queen and King
			case EPieceType::KING:
				int8 AttackableFrom[2] = {
					GField->TileArray[NewGridPosition[0] * GField->Size + NewGridPosition[1]]->GetTileStatus().AttackableFrom[0],
					GField->TileArray[NewGridPosition[0] * GField->Size + NewGridPosition[1]]->GetTileStatus().AttackableFrom[1] 
				};
				
				if (CastlingFlag)
				{
					FCastlingInfo CastlingInfo = Piece->GetColor() == EPieceColor::WHITE ? CastlingInfoWhite : CastlingInfoBlack;
					
					// If delta y > 0 => then look for the rook on the right side of the king, otherwise the left one 
					int8 RookIdx = DeltaY > 0 ? 1 : 0; 
					
					// 1st validation: if the line is clear and neither king and rook have already moved
					// If DeltaY < 0, it means long castling should be handled, so it is necessary to check that all the tiles between left rook and king are empty
					//	e.g. Short Castling:	CheckLineClearDeltaY = DeltaY + 1	= +3		(From the tile which the king is on to the two at its right)
					//		 Long  Castling:	CheckLineClearDeltaY = DeltaY - 2	= -4	(From the tile which the king is on to the three at its left)
					IsValid = !CastlingInfo.KingMoved
						&& !CastlingInfo.RooksMoved[RookIdx]
						&& GField->IsLineClear(ELine::HORIZONTAL, CurrGridPosition, DeltaX, DeltaY + (DeltaY > 0 ? 1 : -2));
					
					// 2nd validation: looking for every tile involved in the movement if it is attackable from opponent piece,
					//	if so, castling is not allowed (king would passed from, through, to a check situation)
					for (int8 i = 0; i <= FMath::Abs(DeltaY) && IsValid; i++)
					{
						if (CheckCheckFlag
							&& GField->IsValidTile(NewGridPosition[0], CurrGridPosition[1] + i * FMath::Sign(DeltaY)))
						{	
							if (CheckFlag != Piece->GetColor())
							{
								EPieceColor PreviousCheckFlag = CheckFlag;
								EPieceColor NewCheckFlag = IsCheck(
									Piece,
									NewGridPosition[0],
									CurrGridPosition[1] + i * FMath::Sign(DeltaY),
									true
								);
								CheckFlag = PreviousCheckFlag;

								// Possible valid situations after having checked the new check situation
								IsValid = NewCheckFlag == EPieceColor::NONE
									|| (NewCheckFlag == EPieceColor::BLACK && Piece->GetColor() != EPieceColor::BLACK)
									|| (NewCheckFlag == EPieceColor::WHITE && Piece->GetColor() != EPieceColor::WHITE);
							}
							else
								IsValid = false;
						}
					}
				}
				else if (!(Piece->GetType() == EPieceType::KING && AttackableFrom[(static_cast<int>(Piece->GetColor()) == 1) ? 1 : 0]))
				{
					IsValid = Piece->CheckDirection(GField, EDirection::FORWARD, NewGridPosition, CurrGridPosition);
					IsValid = IsValid || Piece->CheckDirection(GField, EDirection::BACKWARD, NewGridPosition, CurrGridPosition);
					IsValid = IsValid || Piece->CheckDirection(GField, EDirection::HORIZONTAL, NewGridPosition, CurrGridPosition);
					IsValid = IsValid || Piece->CheckDirection(GField, EDirection::DIAGONAL, NewGridPosition, CurrGridPosition);
				}
				break;
			}
		}

		// If the move is valid and checking the new check state is required,
		//	computing the new check situation
		// Last condition necessary because if CastlingFlag is set to TRUE, check condition has already been evaluated
		if (IsValid && CheckCheckFlag && !CastlingFlag)
		{
			// Check state after having moved the Piece in the new grid position
			if (GField->IsValidTile(NewGridPosition[0], NewGridPosition[1]) && Piece->GetStatus() == EPieceStatus::ALIVE)
			{
				EPieceColor PreviousCheckFlag = CheckFlag;
				EPieceColor NewCheckFlag = IsCheck(Piece, NewGridPosition[0], NewGridPosition[1]);
				CheckFlag = PreviousCheckFlag;

				// Possible valid situations after having checked the new check situation
				IsValid = NewCheckFlag == EPieceColor::NONE
					|| (NewCheckFlag == EPieceColor::BLACK && Piece->GetColor() != EPieceColor::BLACK)
					|| (NewCheckFlag == EPieceColor::WHITE && Piece->GetColor() != EPieceColor::WHITE);
			}
		}
	}

	return IsValid;
}


/*
 * Function: MakeMove
 * ----------------------------
 * Make the move specified through paramters
 *
 * @param Piece		ABasePiece*		Piece to move
 * @param NewX		const int8		X to end to
 * @param NewY		const int8		Y to end to
 * @param Simulate	bool = false	Determine if the move is just a simulation or not.
 *									If so, graphically moving the piece is not required
 *
 * @return			bool			Flag to notify if a capture happened
 */
bool AChess_GameMode::MakeMove(ABasePiece* Piece, const int8 NewX, const int8 NewY, bool Simulate)
{
	bool EatFlag = false;
	int8 OldX = Piece->GetGridPosition()[0];
	int8 OldY = Piece->GetGridPosition()[1];
	if (GField->IsValidTile(OldX, OldY)
		&& GField->IsValidTile(NewX, NewY))
	{
		TArray<ATile*> TilesArray = GField->TileArray;

		// EatFlag is true if the piece color on the new tile is the opposite of the piece to move
		// e.g. Tile->PieceColor = 1 (white) , Piece->Color = -1 => EatFlag = true
		// e.g. Tile->PieceColor = 0 (empty) , Piece->Color = -1 => EatFlag = false
		EatFlag = static_cast<int>(TilesArray[NewX * GField->Size + NewY]->GetTileStatus().PieceColor) == -static_cast<int>(Piece->GetColor());
		if (EatFlag)
		{
			ABasePiece* PieceToEat = TilesArray[NewX * GField->Size + NewY]->GetPiece();
			if (PieceToEat && PieceToEat->GetType() != EPieceType::KING)
				GField->DespawnPiece(PieceToEat->GetGridPosition()[0], PieceToEat->GetGridPosition()[1], Simulate);
			if (PieceToEat && PieceToEat->GetType() == EPieceType::KING)
				EatFlag = false;

			if (PieceToEat && PieceToEat->GetType() == EPieceType::ROOK)
			{
				FCastlingInfo& CastlingInfo = PieceToEat->GetColor() == EPieceColor::WHITE ? CastlingInfoWhite : CastlingInfoBlack;
				CastlingInfo.RooksMoved[PieceToEat->GetPieceNum() == RookWhiteRightPieceNum || PieceToEat->GetPieceNum() == RookBlackRightPieceNum ? 1 : 0] = true;
			}
		}

		// Clear starting tile (no player owner, no piece on it, ...)
		// Update ending tile (new player owner, new tile status, new piece)
		Piece->Move(TilesArray[OldX * GField->Size + OldY], TilesArray[NewX * GField->Size + NewY], Simulate);


		// Castling Handling (King moves by two tiles)
		FCastlingInfo& CastlingInfo = Piece->GetColor() == EPieceColor::WHITE ? CastlingInfoWhite : CastlingInfoBlack;
		if (Piece->GetType() == EPieceType::KING
			&& FMath::Abs(NewY - OldY) == 2)
		{
			// Move the rook
			bool ShortCastling = (NewY - OldY) > 0;
			int8 RookX = Piece->GetColor() == EPieceColor::WHITE ? 0 : GField->Size - 1;
			int8 OldRookY = ShortCastling ? GField->Size - 1 : 0;
			ATile* OldRookTile = GField->TileArray[RookX * GField->Size + OldRookY];
			ABasePiece* RookToMove = OldRookTile->GetPiece();

			int8 NewRookY = OldRookY + (ShortCastling ? SHORT_CASTLING_OFFSET : LONG_CASTLING_OFFSET);
			if (GField->IsValidTile(RookX, NewRookY))
			{
				ATile* NewRookTile = GField->TileArray[RookX * GField->Size + NewRookY];
				if (RookToMove)
				{
					RookToMove->Move(OldRookTile, NewRookTile, Simulate);
					CastlingInfo.RooksMoved[NewRookY == 0 ? 0 : 1] = true;
				}
			}
			if (!Simulate)
				UGameplayStatics::PlaySound2D(GetWorld(), CastlingSound, 1, 1, 0, NULL, false, true);
		}
		else
		{
			if (!Simulate)
			{
				EatFlag ?
					UGameplayStatics::PlaySound2D(GetWorld(), MoveSound, 1, 1, 0, NULL, false, true) :
					UGameplayStatics::PlaySound2D(GetWorld(), CaptureSound, 1, 1, 0, NULL, false, true);
			}
		}

		switch (Piece->GetType())
		{
		case EPieceType::PAWN: Piece->SetMaxNumberSteps(1); break;
		case EPieceType::KING: CastlingInfo.KingMoved = true; break;
		case EPieceType::ROOK: CastlingInfo.RooksMoved[OldY == 0 ? 0 : 1] = true; break;
		}

		// Update last move (useful when doing pawn promotion)
		LastPiece = Piece;
		LastGridPosition = FVector2D(NewX, NewY);
		PreviousGridPosition = FVector2D(OldX, OldY);
		LastEatFlag = EatFlag;
	}
	return EatFlag;
}


/*
 * Function: SetPawnPromotionChoice
 * ----------------------------
 * Promote the pawn with a new chess piece (type passed as parameter)
 *
 * @param PieceType	EPieceType	Type of the new chess piece to spawn
 */
void AChess_GameMode::SetPawnPromotionChoice(EPieceType PieceType)
{
	if (Players.IsValidIndex(CurrentPlayer))
	{
		int8 X = LastGridPosition.X;
		int8 Y = LastGridPosition.Y;
		PawnPromotionType = PieceType;

		// Despawn & Spawn selected pieces
		GField->DespawnPiece(X, Y);
		ABasePiece* PawnTemp = GField->SpawnPiece(PieceType, Players[CurrentPlayer]->Color, X, Y);

		if (PawnPromotionWidget != nullptr)
			PawnPromotionWidget->RemoveFromParent();

		LastPiece = PawnTemp;
		LastEatFlag = LastEatFlag;
		EndTurn(CurrentPlayer, true);
	}
}


/*
 * Function: SameConfigurationBoard
 * -----------------------------
 * If the same position occurs five times during the course of the game, the game is automatically a draw.
 * 
 * @param Times	int8	Times which the same board configuration has to occured to make the game a draw.
 * 
 * @return		bool	If the same configuration board happened "times" times
 */
bool AChess_GameMode::SameConfigurationBoard(const int8 Times) const
{
	int8 Cnt = 0;
	for (const auto& PieceConfiguration : GameSaving)
	{
		// Init var to store if pieces of PieceConfiguration are the same as the current board
		int8 ArePiecesEqual = 1;
		int8 i = 0;
		for (const auto& Piece : std::get<0>(PieceConfiguration))
		{
			if (std::get<0>(CurrentBoard).IsValidIndex(i))
			{
				if (Piece.Status != std::get<0>(CurrentBoard)[i].Status
					|| Piece.X != std::get<0>(CurrentBoard)[i].X
					|| Piece.Y != std::get<0>(CurrentBoard)[i].Y)
				{
					// Pieces are not equal anymore
					ArePiecesEqual = 0;
				}
			} 
			else
			{
				int EndPieceConfiguration = std::get<0>(PieceConfiguration).Num();
				for (int idx2 = EndPieceConfiguration; idx2 < std::get<0>(CurrentBoard).Num(); idx2++)
				{
					if (GField->PieceArray.IsValidIndex(idx2)
						&& GField->PieceArray[idx2]->GetType() == GField->PieceArray[i]->GetType()
						&& GField->PieceArray[idx2]->GetColor() == GField->PieceArray[i]->GetColor()
						&& GField->PieceArray[idx2]->GetStatus() == GField->PieceArray[i]->GetStatus()
						&& Piece.X == std::get<0>(CurrentBoard)[idx2].X
						&& Piece.Y == std::get<0>(CurrentBoard)[idx2].Y)
						break; // the two pieces has the same color, type and grid position
					else
						ArePiecesEqual = 0; // pieces not equal
				}
			}
			if (ArePiecesEqual == 0)
				break;
			i++;
		}
		Cnt += ArePiecesEqual;
		if (Cnt >= Times)
			break;
	}

	return Cnt >= Times;
}



/*
 * Function: SeventyFive_MoveRule
 * ----------------------------
 * If no capture or no pawn move has occurred in the last 75 moves (by both players), the game is automatically a draw.
 * 
 * @return	bool If this condition happened 
 */
bool AChess_GameMode::SeventyFive_MoveRule() const
{
	return (MoveCounter - LastPawnMoveHappened > 75
		|| MoveCounter - LastCaptureHappened > 75);
	
}


/*
 * Function: ImpossibilityToCheckmate
 * ----------------------------
 * Impossibility of checkmate evaluation:
 *	If a position arises in which neither player could possibly give checkmate by a series of legal moves, the game is a draw. 
 *	Such a position is called a dead position. This is usually because there is insufficient material left.
 * Combinations:
 *  - king versus king
 *	- king and bishop versus king
 *  - king and knight versus king
 *  - king and bishop versus king and bishop with the bishops on the same color.
 * 
 * @return	bool	If this condition happened
 */
bool AChess_GameMode::ImpossibilityToCheckmate() const
{
	bool Result = true;
	bool KingVsKing = true;
	bool KingBishopVsKing[2] = { true, true }; // 1st: white info, 2nd: black info
	bool KingKnightVsKing[2] = { true, true }; // 1st: white info, 2nd: black info
	bool KingBishopVsKingBishop = true;
	int8 PreviousBishopTileColor[2] = { -1, -1 }; // -1: no bishops | 0: light color | 1: dark color

	for (const auto& Piece : GField->PieceArray)
	{
		if (Piece->GetStatus() == EPieceStatus::ALIVE && TurnPossibleMoves[Piece->GetPieceNum()].Num() > 0)
		{
			int8 ColorIdx = Piece->GetColor() == EPieceColor::WHITE ? 0 : 1;

			if (Piece->GetType() != EPieceType::KING)
				KingVsKing = false;

			if (Piece->GetType() != EPieceType::KNIGHT)
				KingKnightVsKing[0] = false; KingKnightVsKing[1] = false;

			if (Piece->GetType() != EPieceType::BISHOP)
			{
				KingBishopVsKing[0] = false; KingBishopVsKing[1] = false;
				KingBishopVsKingBishop = false;
			}
			else
			{
				if (PreviousBishopTileColor[ColorIdx] == -1)
				{
					int8 tmp = Piece->GetGridPosition()[0] + Piece->GetGridPosition()[1];
					PreviousBishopTileColor[ColorIdx] = tmp % 2 ? 0 : 1;
				}
				else
					KingBishopVsKingBishop = false; // two bishops of the same color

				if (PreviousBishopTileColor[FMath::Abs(ColorIdx - 1)] != -1 
					&& PreviousBishopTileColor[0] != PreviousBishopTileColor[1]) 
					KingBishopVsKingBishop = false; // already analyzed opponent's bishop, it is on different color tile		
			}

			Result = KingVsKing
				|| KingBishopVsKing[0] || KingBishopVsKing[1]
				|| KingKnightVsKing[0] || KingKnightVsKing[1]
				|| KingBishopVsKingBishop;

			if (!Result)
				break;
		}
	}
	
	return Result;
}


/*
 * Function: ReplayMove
 * ----------------------------
 * Load the turn specified as parameter (e.g. "1. e4" => Replay of the turn number 4.
 * If user decides to rewind the game (clicking the move N and selecting one of his pieces) at turn N,
 * the board will be bring back to the status at turn N and the user is allowed to start again the turn N+1
 *
 * @param TxtBlock	UTextBlock*		Text block containing the turn to replay (e.g. "1. e4)"
 */
void AChess_GameMode::ReplayMove(UTextBlock* TxtBlock)
{
	if (Players.IsValidIndex(CurrentPlayer) && Cast<AChess_HumanPlayer>(Players[CurrentPlayer]))
	{
		// Human is playing (replay available)
		FString BtnName = TxtBlock->GetText().ToString();
		FString NumMoveStr;
		BtnName.Split(TEXT("."), &NumMoveStr, NULL);
		int NumMove = FCString::Atoi(*NumMoveStr);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("Replay mossa %d"), NumMove));

		if ((NumMove + 1) != MoveCounter)
		{
			ReplayInProgress = NumMove;
			if (GameSaving.IsValidIndex(NumMove - 1))
			{
				// std::tuple<TArray<FPieceSaving>, FCastlingInfo, FCastlingInfo> BoardToLoad = GameSaving[NumMove - 1];
				GField->LoadBoard(GameSaving[NumMove - 1]);
			}
		}
		else
		{
			// GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("Current turn")));
			GField->LoadBoard(CurrentBoard);

			ReplayInProgress = MoveCounter;
			Players[CurrentPlayer]->OnTurn();
		}
	}
	else
	{
		// AI is playing (replay NOT available)
		// GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("AI is playing, replay not available"));
	}
}


/*
 * Function: SaveGameOnFile
 * ----------------------------
 * Save game information on file CSV.
 * Read file to get previous information and update with the data of current game data.
 * Header CSV:
 *	#GAME,
 *	PLAYER_1_STATUS,
 *	PLAYER_2_STATUS,
 *	#MOVES,
 *	DURATION(seconds),
 *	PL_1_WINS/GAMES,
 *	PL_1_LOSSES/GAMES,
 *	PL_2_WINS/GAMES,
 *	PL_2_LOSSES/GAMES,
 * 	DRAWS/GAMES
 *
 * @param FilePath			FString		File path of the file to read/write on
 * @param bOutSuccess		bool&		Notify if operations concluded successfully
 * @param OutInfoMessage	FString&	Output message
 */
void AChess_GameMode::SaveGameOnFile(FString& FilePath, bool& bOutSuccess, FString& OutInfoMessage) const
{
	FString ReadFile = TEXT("");
	int32 GamesCount = 0;
	int32 Player1_Wins = 0;
	int32 Player1_Losses = 0;
	int32 Player1_Draws = 0;
	int32 DrawsCount = 0;

	// Get lines count
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		bOutSuccess = false;
		OutInfoMessage = FString::Printf(TEXT("Read String From File Failed - File doesn't exist - '%s'"), *FilePath);
	}
	else
	{
		if (!FFileHelper::LoadFileToString(ReadFile, *FilePath))
		{
			bOutSuccess = false;
			OutInfoMessage = FString::Printf(TEXT("Read String From File Failed - Was not able to read file. Is this a text file? - '%s'"), *FilePath);
		}
		else
		{
			FString Buffer = TEXT("");
			TArray<TCHAR> WinStr = { 'W', 'I', 'N' };
			TArray<TCHAR> LossStr = { 'L', 'O', 'S', 'S' };
			TArray<TCHAR> DrawStr = { 'D', 'R', 'A', 'W' };
			TCHAR tmp = '\n';
			bool bIsFirstLineRead = false;
			bool bGameResultPlayer1_AlreadyRead = false;
			int Win_Idx = 0;
			int Loss_Idx = 0;
			int Draw_Idx = 0;
			for (const auto Chr : ReadFile)
			{
				if (Chr == tmp)
				{
					bGameResultPlayer1_AlreadyRead = false;
					bIsFirstLineRead = true;
					GamesCount++;
				} 
				else
				{
					// 1st condition used to avoid "WINS" in the title will count in Player1_Wins
					if (bIsFirstLineRead && !bGameResultPlayer1_AlreadyRead)
					{
						if (AChess_GameMode::SearchWordByChar(Chr, WinStr, Win_Idx, bGameResultPlayer1_AlreadyRead))
							Player1_Wins++;
						else if (AChess_GameMode::SearchWordByChar(Chr, LossStr, Loss_Idx, bGameResultPlayer1_AlreadyRead))
							Player1_Losses++;
						else if (AChess_GameMode::SearchWordByChar(Chr, DrawStr, Draw_Idx, bGameResultPlayer1_AlreadyRead))
							Player1_Draws++;
					}
				}
			}
		}
	}
	GamesCount = GamesCount ? GamesCount : 1;


	// Try to write (append) into the file
	// Game result
	FString MatchResult_Player1 = TEXT("");
	FString MatchResult_Player2 = TEXT("");
	switch (MatchStatus)
	{
	case EMatchResult::WHITE: 
		MatchResult_Player1 = "LOSS"; 
		MatchResult_Player2 = "WIN";
		Player1_Losses++;
		break;
	case EMatchResult::BLACK: 
		MatchResult_Player1 = "WIN"; 
		MatchResult_Player2 = "LOSS";
		Player1_Wins++;
		break;
	default: 
		MatchResult_Player1 = MatchResult_Player2 = "DRAW"; 
		Player1_Draws++;
		break;
	}

	// Duration
	int Duration = GameInstance ? GameInstance->Minutes * 60 + GameInstance->Seconds : 0;

	// Ratios
	float Wins_games_ratio_player1 = Player1_Wins / (static_cast<float>(GamesCount));
	float Losses_games_ratio_player1 = Player1_Losses / (static_cast<float>(GamesCount));
	float Draws_games_ratio_player1 = Player1_Draws / (static_cast<float>(GamesCount));

	FString HeaderCSV = TEXT("#GAME,PLAYER_1_STATUS,PLAYER_2_STATUS,#MOVES,DURATION(seconds),PL_1_WINS/GAMES,PL_1_LOSSES/GAMES,PL_2_WINS/GAMES,PL_2_LOSSES/GAMES,DRAWS/GAMES\n");
	FString GameToSaveCSV = FString::FromInt(GamesCount) + "," +
		MatchResult_Player1 + "," +
		MatchResult_Player2 + "," +
		FString::FromInt(MoveCounter - 1) + "," +
		FString::FromInt(Duration) + "," +
		FString::Printf(TEXT("%.2f,"), FMath::Abs(Wins_games_ratio_player1)) +
		FString::Printf(TEXT("%.2f,"), FMath::Abs(Losses_games_ratio_player1)) +
		FString::Printf(TEXT("%.2f,"), FMath::Abs(1 - Wins_games_ratio_player1 - Draws_games_ratio_player1)) +
		FString::Printf(TEXT("%.2f,"), FMath::Abs(1 - Losses_games_ratio_player1 - Draws_games_ratio_player1)) +
		FString::Printf(TEXT("%.2f"), FMath::Abs(Draws_games_ratio_player1));
	FString Str = (GamesCount == 1 ? HeaderCSV : TEXT("")) + GameToSaveCSV + TEXT("\n");
	if (!FFileHelper::SaveStringToFile(Str, 
		*FilePath, 
		FFileHelper::EEncodingOptions::AutoDetect, 
		&IFileManager::Get(), 
		EFileWrite::FILEWRITE_Append))
	{
		bOutSuccess = false;
		OutInfoMessage = FString::Printf(TEXT("Write String to File Failed - Was not able to write to file. Is your file read only? Is the path valid? - '%s'"), *FilePath);
		return;
	}

	bOutSuccess = true;
	OutInfoMessage = FString::Printf(TEXT("Write String to File succeeded - '%s'"), *FilePath);
}


/*
 * Function: SearchWordByChar
 * ----------------------------
 * Look if a word match the sequence of TCHARs.
 * Its repetitively call allows to search if a sequence of TCHARs composes a specific word (TArray<TCHAR>)
 *
 * @param Chr					TChar			Current char to analyse
 * @param WordToSearch			TArray<TCHAR>&	Word to search in the sequence of TCHAR
 * @param WordToSearch_Idx		int&			Index of the last success char analysed in the word to search
 * @param NotifyWordCompletion	bool&			Flag to notify when the word is completed
 * 
 * @return						bool			If Chr matches the char at WordToSearch_Idx in the word to search
 */
bool AChess_GameMode::SearchWordByChar(TCHAR Chr, TArray<TCHAR>& WordToSearch, int& WordToSearch_Idx, bool& NotifyWordCompletion)
{
	if (Chr == WordToSearch[WordToSearch_Idx])
	{
		WordToSearch_Idx++;
		if (WordToSearch_Idx == WordToSearch.Num())
		{
			NotifyWordCompletion = true;
			WordToSearch_Idx = 0;
			return true;
		}
	}
	else
		WordToSearch_Idx = 0;
	return false;
}