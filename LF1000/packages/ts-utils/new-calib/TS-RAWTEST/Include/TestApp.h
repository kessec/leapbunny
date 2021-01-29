#ifndef TESTAPP_H_
#define TESTAPP_H_

#include <AppInterface.h>
#include <GameStateHandler.h>
#include <GameStateBase.h>
#include <DebugMPI.h>

class TestApp : public CAppInterface
{
	public:
		TestApp(void* userdata);
		~TestApp();
	
		void Enter();
		void Update();
		void Exit();
	
		void Suspend();
		void Resume();

		static void RequestNewState(CGameStateBase* state);

		LeapFrog::Brio::U32	getProductID()		{ return productID_; }
		char *				getPartNumber()		{ return const_cast<char *>(partNumber_.c_str()); }
		char *				getProductVersion()	{ return const_cast<char *>(pkgVersion_.c_str()); }
		LeapFrog::Brio::U8	getLogDataVersion()	{ return logDataVersion_; }
		
	protected:
		LeapFrog::Brio::CDebugMPI			debugMPI_;
		CGameStateHandler *					stateHandler_;

		static CGameStateBase*				newStateRequested_;

		LeapFrog::Brio::U32			productID_;
		LeapFrog::Brio::CString		partNumber_;
		LeapFrog::Brio::CString		pkgVersion_;
		LeapFrog::Brio::U8			logDataVersion_;
		LeapFrog::Brio::CPath		storedArtPath_;
};

#endif //TESTAPP_H_
