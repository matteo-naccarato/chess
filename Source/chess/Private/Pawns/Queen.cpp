// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Queen.h"

AQueen::AQueen()
{
	Type = EPawnType::QUEEN;
	MaxNumberSteps = 8; // TODO magic number
}