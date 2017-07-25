#include "GalavantUnreal.h"

#include "ConversionHelpers.h"

gv::Position ToPosition(const FVector& vector)
{
	gv::Position newPosition(vector.X, vector.Y, vector.Z);
	return newPosition;
}

FVector ToFVector(const gv::Position& position)
{
	FVector newVector(position.X, position.Y, position.Z);
	return newVector;
}