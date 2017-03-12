#pragma once

#include "TestWorldResourceLocator.hpp"
#include "../GalaEntityComponents/TestMovementComponent.hpp"

#include "ai/htn/HTNTypes.hpp"
#include "ai/htn/HTNTasks.hpp"
#include "ai/WorldState.hpp"

// Parameters:
//  [0]: Resource type (int)
class TestFindResourceTask : public Htn::PrimitiveTask
{
private:
	TestWorldResourceLocator* ResourceLocator;

public:
	TestFindResourceTask() = default;
	virtual ~TestFindResourceTask() = default;

	void Initialize(TestWorldResourceLocator* newResourceLocator);
	virtual bool StateMeetsPreconditions(const gv::WorldState& state,
	                                     const Htn::ParameterList& parameters) const;
	virtual void ApplyStateChange(gv::WorldState& state, const Htn::ParameterList& parameters);
	virtual Htn::TaskExecuteStatus Execute(gv::WorldState& state,
	                                       const Htn::ParameterList& parameters);
};

// Parameters:
//  None - Entity to move and target come from world state
class TestMoveToTask : public Htn::PrimitiveTask
{
private:
	TestMovementComponent* MovementManager;

public:
	TestMoveToTask() = default;
	virtual ~TestMoveToTask() = default;

	void Initialize(TestMovementComponent* newMovementManager);
	virtual bool StateMeetsPreconditions(const gv::WorldState& state,
	                                     const Htn::ParameterList& parameters) const;
	virtual void ApplyStateChange(gv::WorldState& state, const Htn::ParameterList& parameters);
	virtual Htn::TaskExecuteStatus Execute(gv::WorldState& state,
	                                       const Htn::ParameterList& parameters);
};

// Parameters:
//  [0]: Resource type (int)
class TestGetResourceTask : public Htn::CompoundTask
{
private:
	// Should this be how it is? Should tasks be singletons?
	Htn::Task FindResourceTask;
	Htn::Task MoveToTask;

public:
	TestGetResourceTask() = default;
	virtual ~TestGetResourceTask() = default;

	void Initialize(TestFindResourceTask* newFindResourceTask, TestMoveToTask* newMoveToTask);
	virtual bool StateMeetsPreconditions(const gv::WorldState& state,
	                                     const Htn::ParameterList& parameters) const;
	virtual bool Decompose(Htn::TaskCallList& taskCallList, const gv::WorldState& state,
	                       const Htn::ParameterList& parameters);
};