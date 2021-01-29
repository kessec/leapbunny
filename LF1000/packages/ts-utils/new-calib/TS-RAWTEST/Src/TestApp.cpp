#include <LogFile.h>
#include <CGraphics2D.h>
#include <LTM.h>

#include "TestApp.h"
#include "MainState.h"

#define DEBUG_LEVEL kDbgLvlNoteable

using namespace LeapFrog::Brio;
using namespace LogData::Lightning;

CGameStateBase* TestApp::newStateRequested_	= NULL;

TestApp::TestApp(void * userData=NULL)
	: CAppInterface(),
	debugMPI_(0x911),
	productID_(0), partNumber_("000-000000"), pkgVersion_("0.0.0.0"), logDataVersion_(0)
{
}

TestApp::~TestApp()
{
}

void TestApp::Suspend()
{
	debugMPI_.DebugOut(kDbgLvlNoteable, "[TEST] Suspend\n");

	CGameStateBase* topState;
	topState = stateHandler_->GetTopState();
	topState->Suspend();
}

void TestApp::Resume()
{
	debugMPI_.DebugOut(kDbgLvlNoteable, "[TEST] Resume\n");

	CGameStateBase* topState;
	topState = stateHandler_->GetTopState();
	topState->Resume();
}

void TestApp::Update()
{
	debugMPI_.DebugOut(kDbgLvlVerbose, "TestApp::Update()\n");

	// switch states if necessary
	if (newStateRequested_ != NULL) {
		stateHandler_->Request(newStateRequested_);
		newStateRequested_ = NULL;
	}

	// this will call update on the current state
	stateHandler_->Run();
}

void TestApp::Enter()
{
	debugMPI_.SetDebugLevel(DEBUG_LEVEL);
	debugMPI_.DebugOut(kDbgLvlNoteable, "TestApp::Enter()\n");

	stateHandler_ = new CGameStateHandler();

	// TODO: ideally this would go in the constructor, but the current title is not set yet.
	// initialize data used for logging
	LTM::CMetaInfo metaInfo(CSystemData::Instance()->GetCurrentTitlePath());
	if (metaInfo.IsValid())
	{
		productID_ =	metaInfo.GetProductId();
		partNumber_ =	metaInfo.GetPartNumber();
		pkgVersion_ =	metaInfo.GetPackageVersion();
		logDataVersion_ =	0;
	}
	debugMPI_.DebugOut(kDbgLvlNoteable, "ProductID: %lu\n", getProductID());
	debugMPI_.DebugOut(kDbgLvlNoteable, "PartNumber: %s\n", getPartNumber());
	debugMPI_.DebugOut(kDbgLvlNoteable, "productVersion: %s\n", getProductVersion());
	debugMPI_.DebugOut(kDbgLvlNoteable, "log data version: %d\n", getLogDataVersion());

	LTM::CSystem system_settings;
	
	// Log TitleStart event.
	U32 productId = getProductID();
	LogData::Lightning::CTitleStart titleStart(getProductID(), getPartNumber(),
			getProductVersion(), getLogDataVersion());
	CLogFile::Instance()->Append(titleStart);

	// initialize the graphics library
	CGraphics2D::Instance()->init(tVect2(480, 272), tColor(100, 100, 100, 255));

	debugMPI_.DebugOut(kDbgLvlNoteable, "Graphics Initialized\n");
	
	//Set art path
	CPath artpath = CSystemData::Instance()->GetCurrentTitlePath();
	artpath = AppendPathSeparator(artpath);
	artpath += "art" + AppendPathSeparator(artpath);
	//CSystemData::Instance()->SetCustomizedArtPath(artpath);
	
	debugMPI_.DebugOut(kDbgLvlNoteable, "About to switch to picker\n");
	
	// start with this state
	stateHandler_->Request(MainState::Instance());
	RequestNewState(MainState::Instance());
}

void TestApp::Exit()
{
	debugMPI_.DebugOut(kDbgLvlNoteable, "TestApp::Exit()\n");

	//kill the state handler, so they can cleanup before we clean up here
	delete stateHandler_;

	// Log the title exit event
	LogData::Lightning::CTitleExit titleExit(LogData::Lightning::CTitleExit::kReasonUnknown);
	CLogFile::Instance()->Append(titleExit);

	CGraphics2D::Instance()->cleanup();
}

void TestApp::RequestNewState(CGameStateBase* state)
{
	CDebugMPI debugmpi(0x100);
	if(state==NULL)
		debugmpi.DebugOut(kDbgLvlImportant, "TestApp null state requested\n");
	newStateRequested_ = state;
}
