/**
 * This file is only responsible for exposing the methods needed for creating
 * and destroying the app.  This is required for compatability with AppManager.
 */

#include "TestApp.h"
#include <AppInterface.h>

extern "C" boost::shared_ptr<CAppInterface> CreateApp(void *userData) 
{
	return boost::shared_ptr<CAppInterface>(new TestApp(userData));
}

//extern "C" void DestroyApp(CAppInterface* p) 
//{
//	delete p;
//}
