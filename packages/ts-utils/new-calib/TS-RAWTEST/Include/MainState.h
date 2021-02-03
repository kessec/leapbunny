#ifndef MAINSTATE_H_
#define MAINSTATE_H_

#include <GameStateBase.h>

class MainState : public CGameStateBase
{
	public:
		static MainState* Instance();
	
		void Enter(CStateData* userData);
		void Update(CGameStateHandler* sh);
		void Suspend();
		void Resume();
		void Exit();

		~MainState();
		MainState();

	protected:
		static MainState*	mInstance;
};

#endif	//MAINSTATE_H_
