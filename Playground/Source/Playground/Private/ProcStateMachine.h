#pragma once

template <class Actor, class StateID, class Signal>
class ProcStateMachine {
	virtual ProcStateMachine<Actor, StateID, Signal>* Step(Actor* a, float DeltaTime);
	virtual ProcStateMachine<Actor, StateID, Signal>* Signal(Actor* a, Signal s);
};

