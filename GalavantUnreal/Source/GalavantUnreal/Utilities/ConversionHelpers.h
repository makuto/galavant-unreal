#pragma once

#include "world/Position.hpp"

gv::Position ToPosition(const FVector& vector);

FVector ToFVector(const gv::Position& position);