#include "MainState.h"

//LightningCore
#include <AppManager.h>
#include <CGraphics2D.h>
#include <CDrawManager.h>

using namespace LeapFrog::Brio;
using namespace std;

MainState* MainState::mInstance = NULL;

MainState* MainState::Instance() {
	if(!mInstance)
		mInstance = new MainState();
	return mInstance;
}

MainState::MainState() {}
MainState::~MainState() {}

void MainState::Update(CGameStateHandler* sh) {}
void MainState::Suspend() {}
void MainState::Resume() {}

void MainState::Enter(CStateData* userData) {
	system("cp -f /LF/Bulk/ProgramFiles/TS-RAWTEST/bam-wrapper /tmp/");
	system("sudo killall AppManager");
	CAppManager::Instance()->PopApp();
}

void MainState::Exit() {
}
