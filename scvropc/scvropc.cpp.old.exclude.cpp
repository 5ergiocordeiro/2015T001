#include <atlbase.h>
#include <objbase.h>
#include "opc_h.h"
#include "header.h"


#define OPC_MATRIKON L"Matrikon.OPC.Simulation.1"
#define OPC_SERVER_NAME OPC_MATRIKON
//#define REMOTE_SERVER_NAME L"your_path"


//////////////////////////////////////////////////////////////////////
// Instantiate and Release the IOPCServer interface of the OPC server,
// OPC_SERVER_NAME.
void main(void)
{
	// have to be done before using microsoft COM library:
	CoInitialize(NULL);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	IOPCServer* pServer = InstantiateServer(OPC_SERVER_NAME);

	// release IOPServer interface:
	pServer->Release();

	//close the COM library:
	CoUninitialize();
}

////////////////////////////////////////////////////////////////////
// Instantiate the IOPCServer interface of the OPCServer
// having the name ServerName. Return a pointer to this interface
//
IOPCServer* InstantiateServer(wchar_t ServerName[])
{
	CLSID CLSID_OPCServer;
	HRESULT hr;

	// get the CLSID from the OPC Server Name:
	hr = CLSIDFromString(ServerName, &CLSID_OPCServer);
	_ASSERT(!FAILED(hr));


	//queue of the class instances to create
	LONG cmq = 1; // nbr of class instance to create.
	MULTI_QI queue[1] =
	{ { &IID_IOPCServer,
	NULL,
	0 } };

	//Server info:
	//COSERVERINFO CoServerInfo =
	//{
	//	/*dwReserved1*/ 0,
	//	/*pwszName*/ REMOTE_SERVER_NAME,
	//	/*COAUTHINFO*/  NULL,
	//	/*dwReserved2*/ 0
	//}; 

	// create an instance of the IOPCServer
	hr = CoCreateInstanceEx(CLSID_OPCServer, NULL, CLSCTX_SERVER,
		/*&CoServerInfo*/NULL, cmq, queue);
	_ASSERT(!hr);

	// return a pointer to the IOPCServer interface:
	return(IOPCServer*)queue[0].pItf;
}