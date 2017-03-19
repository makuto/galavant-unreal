#include "GalavantUnreal.h"

#include "TestMovementTasks.hpp"

#include "util/Logging.hpp"
#include "world/Position.hpp"
#include "world/WorldResourceLocator.hpp"

void TestFindResourceTask::Initialize()
{
}

bool TestFindResourceTask::StateMeetsPreconditions(const gv::WorldState& state,
                                                   const Htn::ParameterList& parameters) const
{
	return parameters.size() == 1 && parameters[0].Type == Htn::Parameter::ParamType::Int &&
	       gv::WorldResourceLocator::ResourceExistsInWorld(
	           (gv::WorldResourceType)parameters[0].IntValue);
}

void TestFindResourceTask::ApplyStateChange(gv::WorldState& state,
                                            const Htn::ParameterList& parameters)
{
	// TODO: Should this be the case? Should StateMeetsPreconditions find the position? This isn't
	// that much of a problem if ResourceLocator caches searches
	float manhattanTo = 0.f;
	gv::Position targetPosition = gv::WorldResourceLocator::FindNearestResource(
	    (gv::WorldResourceType)parameters[0].IntValue, state.SourceAgent.position, false,
	    manhattanTo);
	if (manhattanTo != -1.f)
		state.SourceAgent.TargetPosition = targetPosition;
}

Htn::TaskExecuteStatus TestFindResourceTask::Execute(gv::WorldState& state,
                                                     const Htn::ParameterList& parameters)
{
	float manhattanTo = 0.f;
	gv::Position targetPosition = gv::WorldResourceLocator::FindNearestResource(
	    (gv::WorldResourceType)parameters[0].IntValue, state.SourceAgent.position, false,
	    manhattanTo);
	if (manhattanTo != -1.f)
	{
		state.SourceAgent.TargetPosition = targetPosition;
		LOGD << "Found resource at " << targetPosition;
		Htn::TaskExecuteStatus status{Htn::TaskExecuteStatus::ExecutionStatus::Succeeded};
		return status;
	}
	else
		LOGD << "Couldn't find resource!";

	LOGD << "Failed to find resource";
	Htn::TaskExecuteStatus status{Htn::TaskExecuteStatus::ExecutionStatus::Failed};
	return status;
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

Htn::TaskExecuteStatus TestMoveToTask::Execute(gv::WorldState& state,
                                               const Htn::ParameterList& parameters)
{
	if (MovementManager)
	{
		gv::EntityList entitiesToMove{state.SourceAgent.SourceEntity};
		std::vector<gv::Position> positions{state.SourceAgent.TargetPosition};
		LOGD << "Moving Ent[" << state.SourceAgent.SourceEntity << "] to "
		     << state.SourceAgent.TargetPosition;
		MovementManager->PathEntitiesTo(entitiesToMove, positions);
		Htn::TaskExecuteStatus status{Htn::TaskExecuteStatus::ExecutionStatus::Subscribe};
		return status;
	}

	Htn::TaskExecuteStatus status{Htn::TaskExecuteStatus::ExecutionStatus::Failed};
	return status;
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
	bool parametersValid =
	    parameters.size() == 1 && parameters[0].Type == Htn::Parameter::ParamType::Int;
	LOGD_IF(!parametersValid) << "TestGetResourceTask parameters invalid! Expected Int";
	return parametersValid;
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