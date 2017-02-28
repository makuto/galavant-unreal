#include "GalavantUnreal.h"

#include "TestMovementTasks.hpp"

#include "world/Position.hpp"

void TestFindResourceTask::Initialize(TestWorldResourceLocator* newResourceLocator)
{
	ResourceLocator = newResourceLocator;
}

bool TestFindResourceTask::StateMeetsPreconditions(const gv::WorldState& state,
                                                   const Htn::ParameterList& parameters) const
{
	return parameters.size() == 1 && parameters[0].Type == Htn::Parameter::ParamType::Int &&
	       ResourceLocator &&
	       ResourceLocator->ResourceExistsInWorld((WorldResourceType)parameters[0].IntValue);
}

void TestFindResourceTask::ApplyStateChange(gv::WorldState& state,
                                            const Htn::ParameterList& parameters)
{
	// TODO: Should this be the case? Should StateMeetsPreconditions find the position? This isn't
	// that much of a problem if ResourceLocator caches searches
	float manhattanTo = 0.f;
	gv::Position targetPosition = ResourceLocator->FindNearestResource(
	    (WorldResourceType)parameters[0].IntValue, state.SourceAgent.position, manhattanTo);
	if (manhattanTo != -1.f)
		state.SourceAgent.TargetPosition = targetPosition;
}

bool TestFindResourceTask::Execute(gv::WorldState& state, const Htn::ParameterList& parameters)
{
	if (ResourceLocator)
	{
		float manhattanTo = 0.f;
		gv::Position targetPosition = ResourceLocator->FindNearestResource(
		    (WorldResourceType)parameters[0].IntValue, state.SourceAgent.position, manhattanTo);
		if (manhattanTo != -1.f)
		{
			state.SourceAgent.TargetPosition = targetPosition;
			// TODO: This task finishes instantly; do we need to return Success, Fail, Running and
			// add a Running() function? That'd make this observer stuff go away
			return true;
		}
	}

	return false;
}

void TestMoveToTask::Initialize(TestMovementComponent* newMovementManager)
{
    MovementManager = newMovementManager;
}

bool TestMoveToTask::StateMeetsPreconditions(const gv::WorldState& state,
                                             const Htn::ParameterList& parameters) const
{
	// We're good to move to a position as long as we have a target
	if (state.SourceAgent.TargetPosition)
		return true;

	return false;
}

void TestMoveToTask::ApplyStateChange(gv::WorldState& state, const Htn::ParameterList& parameters)
{
	state.SourceAgent.position = state.SourceAgent.TargetPosition;
}

bool TestMoveToTask::Execute(gv::WorldState& state, const Htn::ParameterList& parameters)
{
	if (MovementManager)
	{
		gv::EntityList entitiesToMove{state.SourceAgent.SourceEntity};
		std::vector<gv::Position> positions{state.SourceAgent.TargetPosition};
		MovementManager->PathEntitiesTo(entitiesToMove, positions);
		return true;
	}

	return false;
}

// TODO: This isn't generic, so PlanComponentManager has no way of executing it :(
bool TestMoveToTask::ExecuteAndObserve(gv::WorldState& state, const Htn::ParameterList& parameters,
                                       gv::Observer<Htn::TaskEvent>* observer)
{
	if (Execute(state, parameters))
	{
		MovementManager->AddObserver(observer);
		return true;
	}
	else
		return false;
}

void TestGetResourceTask::Initialize(TestFindResourceTask* newFindResourceTask,
                                TestMoveToTask* newMoveToTask)
{
	FindResourceTask.Initialize(newFindResourceTask);
	MoveToTask.Initialize(newMoveToTask);
}

bool TestGetResourceTask::StateMeetsPreconditions(const gv::WorldState& state,
                                                  const Htn::ParameterList& parameters) const
{
	// TODO: What is the purpose of the compound task checking preconditions if they'll be near
	// identical to the combined primitive task preconditions?
	return parameters.size() == 1 && parameters[1].Type == Htn::Parameter::ParamType::Int;
}

bool TestGetResourceTask::Decompose(Htn::TaskCallList& taskCallList, const gv::WorldState& state,
                                    const Htn::ParameterList& parameters)
{
	Htn::ParameterList findResourceParams = parameters;
	Htn::ParameterList moveToParams;

	Htn::TaskCall FindResourceTaskCall{&FindResourceTask, findResourceParams};
	Htn::TaskCall MoveToTaskCall{&MoveToTask, moveToParams};

	// TODO: Make it clear that users should only ever push to this list
	taskCallList.push_back(FindResourceTaskCall);
	taskCallList.push_back(MoveToTaskCall);

	return true;
}