#include <atlbase.h>
#include <objbase.h>
#include <iostream>
#include "opc_h.h"
#include "header.h"

using namespace std;

#define OPC_MATRIKON L"Matrikon.OPC.Simulation.1"
#define OPC_SERVER_NAME OPC_MATRIKON
//#define REMOTE_SERVER_NAME L"your_path"

#define MAX_ITEMS 9

typedef struct MyOPCItemInfo {
	LPWSTR nome;
	VARTYPE tipo;
	};
typedef struct MyOPCItemData {
	float valor;
	bool ok;
	};
typedef struct MyOPCServerInfo {
	bool inicializado;
	IOPCServer * pServer;
	OPCHANDLE hGroup, hItem[MAX_ITEMS];
	IOPCItemMgt * pGroup;
	int numitems;
	MyOPCItemInfo iteminfo[MAX_ITEMS];
	MyOPCItemData itemdata[MAX_ITEMS];
	};

MyOPCServerInfo ServerInfo = {
		FALSE, NULL, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, NULL, 3,
		{
			{ L"Random.Real8", VT_R8 }, { L"Saw-toothed Waves.Real8", VT_R8 }, { L"Square Waves.Real8", VT_R8 },
			{}, {}, {}, {}, {}, {},
		},
	};

int Main(void) {
	int retcode = InitOPC();
	if (retcode != 0) {
		cout << "Erro " << retcode << " na inicialização do OPC." << endl;
		return retcode;
		}
	//Read the value of the item from device:
	double value = ReadTheItem(0);
	// print the read value:
	cout << "Read value: " << value << endl;
	EndOPC();
	}

void EndOPC(void) {
// Encerra a conexão com o servidor OPC
// Ignora eventuais erros
	// Remove os grupos e itens
	ServerInfo.inicializado = FALSE;
	for (int i = 0; i < ServerInfo.numitems; ++i) {
		RemoveItem(ServerInfo.pGroup, ServerInfo.hItem[i]);
		}
	RemoveGroup (ServerInfo.pServer, ServerInfo.hGroup);
	// Libera as interfaces
	ServerInfo.pGroup -> Release();
	ServerInfo.pServer -> Release();
	// Encerra o COM
	CoUninitialize();
	}

int InitOPC(void) {
// Inicializa a interface com o servidor OPC
	// Inicializa o COM
	CoInitialize(NULL);
	// Conecta-se ao servidor OPC
	IOPCServer * pIOPCServer = InstantiateServer(OPC_SERVER_NAME);
	if (pIOPCServer == NULL) {
		cout << "Erro ao conectar-se ao servidor: " << endl;
		return 1;
		}
	ServerInfo.pServer = pIOPCServer;
	// Adiciona os grupos e itens OPC
	OPCHANDLE hServerGroup, hServerItem;
	IOPCItemMgt * pIOPCItemMgt = AddTheGroup(pIOPCServer, L"Grupo1", hServerGroup);
	if (pIOPCItemMgt == NULL) {
		cout << "Erro ao criar o grupo: " << endl;
		return 1;
		}
	ServerInfo.pGroup = pIOPCItemMgt;
	ServerInfo.hGroup = hServerGroup;
	for (int i = 0; i < ServerInfo.numitems; ++i) {
		hServerItem = AddTheItem(pIOPCItemMgt, ServerInfo.iteminfo[i].nome, ServerInfo.iteminfo[i].tipo);
		if (hServerItem == 0) {
			cout << "Erro ao criar o item " << i << ": " << ServerInfo.iteminfo[i].nome << "." << endl;
			return 1;
			}
		ServerInfo.hItem[i] = hServerItem;
		}
	ServerInfo.inicializado = TRUE;
	return 0;
	}

IOPCServer * InstantiateServer(wchar_t ServerName[]) {
// Instancia a interface com o servidor OPC ...Servername...
// Retorna um ponteiro para a interface.
	// Obtém o ID do servidor OPC
	CLSID CLSID_OPCServer;
	HRESULT hr = CLSIDFromString(ServerName, & CLSID_OPCServer);
	if (hr != S_OK) {
		return NULL;
		}
	LONG cmq = 1;
	MULTI_QI queue[1] = {
		{ & IID_IOPCServer, NULL, 0 }
		};

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
	if (hr != S_OK) {
		return NULL;
		}
	return (IOPCServer*) queue[0].pItf;
	}

IOPCItemMgt * AddTheGroup(IOPCServer * pIOPCServer, LPWSTR Groupname, OPCHANDLE & hServerGroup) {
// Adiciona o grupo ...Groupname... ao servidor indicado por ...pIOPCServer...
// Retorna um ponteiro para a interface do grupo e o handle do grupo.
	DWORD dwUpdateRate = 1000;
	OPCHANDLE hClientGroup = 0;
	IOPCItemMgt * pIOPCItemMgt;
	HRESULT hr = pIOPCServer -> AddGroup(Groupname, FALSE, dwUpdateRate, hClientGroup, 0, 0, 0, & hServerGroup, & dwUpdateRate, IID_IOPCItemMgt, (IUnknown**) & pIOPCItemMgt);
	if (hr != S_OK) {
		return NULL;
		}
	return pIOPCItemMgt;
	}

OPCHANDLE AddTheItem(IOPCItemMgt* pIOPCItemMgt, LPWSTR ItemID, VARTYPE ItemType) {
// Adiciona o item ...ItemID..., cujo tipo é ...ItemType..., ao grupo apontado por ...pIOPCItemMgt...
// Retorna um handle para o item.
	OPCITEMDEF ItemArray[1] = {
		{ L"", ItemID, FALSE, 1, 0, NULL, ItemType, 0 }
		};
	OPCITEMRESULT * pAddResult = NULL;
	HRESULT * pErrors = NULL;
	HRESULT hr = pIOPCItemMgt -> AddItems(1, ItemArray, & pAddResult, & pErrors);
	if (hr != S_OK) {
		return (OPCHANDLE)0;
		}
	OPCHANDLE hServerItem = pAddResult[0].hServer;
	// libera a memória alocada pelo servidor ao responder
	CoTaskMemFree (pAddResult -> pBlob);
	CoTaskMemFree(pAddResult);
	pAddResult = NULL;
	CoTaskMemFree(pErrors);
	pErrors = NULL;
	return hServerItem;
	}

double ReadTheItem(int item) {
	VARIANT varValue;
	VariantInit(& varValue);
	ReadItem(ServerInfo.pGroup, ServerInfo.hItem[item], varValue);
	return varValue.dblVal;
	}

void ReadAllItems(double * pval) {
	for (int i = 0; i < ServerInfo.numitems; ++i) {
		* pval ++ = ReadTheItem(i);
		}
	}
///////////////////////////////////////////////////////////////////////////////
// Read from device the value of the item having the "hServerItem" server 
// handle and belonging to the group whose one interface is pointed by
// pGroupIUnknown. The value is put in varValue. 
//
void ReadItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue)
{
	// value of the item:
	OPCITEMSTATE* pValue = NULL;

	//get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);

	// read the item value from the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Read(OPC_DS_DEVICE, 1, &hServerItem, &pValue, &pErrors);
	_ASSERT(!hr);
	_ASSERT(pValue != NULL);

	varValue = pValue[0].vDataValue;

	//Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;

	CoTaskMemFree(pValue);
	pValue = NULL;

	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

void RemoveItem(IOPCItemMgt * pIOPCItemMgt, OPCHANDLE hServerItem) {
// Remove o item ...hServerItem... do grupo apontado por ...pIOPCItemMgt...
// Ignora eventuais erros
	OPCHANDLE hServerArray[1];
	hServerArray[0] = hServerItem;
	HRESULT * pErrors;
	HRESULT hr = pIOPCItemMgt -> RemoveItems(1, hServerArray, & pErrors);
	// Libera a memória alocada pelo servidor para a operação
	CoTaskMemFree(pErrors);
	pErrors = NULL;
	}

void RemoveGroup(IOPCServer * pIOPCServer, OPCHANDLE hServerGroup) {
// Remove o grupo ...hServerGroup... do servidor apontado por ...pIOPCServer...
// Ignora eventuais erros
	HRESULT hr = pIOPCServer -> RemoveGroup(hServerGroup, FALSE);
	}