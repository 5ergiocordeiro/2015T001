#include <atlbase.h>
#include <objbase.h>
#include <iostream>
#include <time.h>
#include "opc_h.h"
#include "header.h"

#include <direct.h>

using namespace std;

#define OPC_MATRIKON "Matrikon.OPC.Simulation.1"
#define OPC_iFIXClient "Intellution.iFixOPCClient"
#define OPC_iFIXDB "Intellution.OPCiFIX.1"
#define OPC_SIMATIC "OPC.SimaticNET.1"
#define OPC_SERVER_NAME OPC_SIMATIC
//#define REMOTE_SERVER_NAME L"your_path"

MyOPCServerInfo ServerInfo = {
		"", FALSE, NULL, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, NULL, 3,
		{
			// { L"Random.Real8", VT_R8, "Random" }, { L"Saw-toothed Waves.Real8", VT_R8, "Waves" }, { L"Square Waves.Real8", VT_R8, "Real8" },
			// { L"CUT3;ANALOG;S7:[CUT3]DB20,REAL0,1", VT_R8 }, { L"CUT3;ANALOG;S7:[CUT3]DB17,REAL12,1", VT_R8 }, { L"CUT3;ANALOG;S7:[CUT3]DB17.REAL32,1", VT_R8 },
			// { L"S7:[SUB3A18]DB20,REAL14,1", VT_R8, "FT01_UE" }, { L"S7:[SUB3A18]DB20,REAL18,1", VT_R8, "FT02_UE" }, { L"S7:[SUB3A18]DB20, REAL22, 1", VT_R8, "FT03_UE" },
			{}, {}, {}, {}, {}, {}, {}, {}, {}
		},
		// { SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR },
		{
			{}, {}, {}, {}, {}, {}, {}, {}, {}
		}
	};
GlobalData Gdata;


// Funções para leitura do servidor OPC
int Main(void) {
// Usada apenas para testes em ambiente de console.
	int retcode = InitOPC();
	if (retcode != 0) {
		cout << "Erro " << retcode << " na inicialização do OPC." << endl;
		return retcode;
		}
	double value = ReadTheItem(0);
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
	wchar_t wnome[50];
	mbstowcs(wnome, ServerInfo.nome, strlen(ServerInfo.nome) + 1);
	IOPCServer * pIOPCServer = InstantiateServer(wnome);
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
		mbstowcs(wnome, ServerInfo.iteminfo[i].nome, strlen(ServerInfo.iteminfo[i].nome) + 1);
		hServerItem = AddTheItem(pIOPCItemMgt, wnome, ServerInfo.iteminfo[i].tipo);
		if (hServerItem == 0) {
			cout << "Erro ao criar o item " << i << ": " << * ServerInfo.iteminfo[i].nome << "." << endl;
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
// Lê o ...item... no servidor OPC conectado e atualiza os acumuladores
	VARIANT varValue;
	VariantInit(& varValue);
	time_t tnow;
	time(& tnow);
	int retcode = ReadItem(ServerInfo.pGroup, ServerInfo.hItem[item], varValue);
	if (retcode != 0) {
		return 0;
		}
	ReadData * pdata = & Gdata.current[item];
	time_t lastt = pdata -> timestamp;
	double last = varValue.dblVal;
	log_(3, cerr << "item " << item << " = " << last << " (Timestamp = " << tnow << ")\n";);
	if (last < 0) {
		log_(2, cerr << "valor " << last << " desprezado. Timestamps = " << lastt << " e " << tnow << "\n";);
		}
	pdata -> value = last;
	pdata -> timestamp = tnow;
	int interval = difftime_(tnow, lastt);
	double area = last * interval * ServerInfo.itemdata[item].factor;
	pdata -> hora += area;
	pdata -> dia += area;
	pdata -> mes += area;
	return last;
	}

void ReadAllItems(double * pval, char ptime[][50]) {
	for (int i = 0; i < ServerInfo.numitems; ++ i) {
		* pval ++ = ReadTheItem(i);
		}
	char buf[50];
	for (int i = 0; i < ServerInfo.numitems; ++ i) {
		tm * tp = localtime(& Gdata.current[i].timestamp);
		strftime(buf, 50, "%H:%M:%S %d/%m/%Y", tp);
		strcpy(ptime[i], buf);
		}
	}

int ReadItem(IUnknown * pGroup, OPCHANDLE hServerItem, VARIANT & varValue) {
// Lê o valor atual do ...hServerItem... pertencente ao grupo apontado por ...pGroup... e grava-o em ...varValue...
// Retorna 0 se tiver sucesso e 1 em caso de erro.
	// value of the item:
	OPCITEMSTATE* pValue = NULL;
	IOPCSyncIO * pIOPCSyncIO;
	pGroup -> QueryInterface(__uuidof(pIOPCSyncIO), (void**) & pIOPCSyncIO);
	HRESULT* pErrors = NULL;
	HRESULT hr = pIOPCSyncIO -> Read(OPC_DS_DEVICE, 1, & hServerItem, & pValue, & pErrors);
	if ((hr != S_OK) || (pValue == NULL)) {
		return 1;
		}
	varValue = pValue[0].vDataValue;
	// Libera a memória alocada pelo servidor para essa operação
	CoTaskMemFree(pErrors);
	pErrors = NULL;
	CoTaskMemFree(pValue);
	pValue = NULL;
	pIOPCSyncIO -> Release();
	return 0;
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


// Funções para tratamento de arquivos de dados
bool fexists(char * fname) {
// Verifica se o arquivo dado pode ser lido	
	FILE * fp = fopen(fname, "r");
	if (fp == NULL) {
		return false;
		}
	fclose(fp);
	return true;
	}

void readData(PanelData * plotdata) {
// Lê os últimos dados gravados
	// Tenta ler do arquivo de dados horários e também do arquivo de dados diários
	char data[2][MAX_ITEMS + 2][MAX_STRSIZ];
	bool rfail1, rfail2;
	sprintf(plotdata->hfile, HDATAFILE);
	sprintf(plotdata->mfile, MDATAFILE);
	FILE * fph = openDatafile(plotdata->hfile, (void *)data[0], &rfail1);
	plotdata->hdfile = fph;
	FILE * fpm = openDatafile(plotdata->mfile, (void *)data[1], &rfail2);
	plotdata->mdfile = fpm;
	// Copia os dados lidos para as variáveis globais
	int ind = (rfail1 && rfail2) ? -1 : (rfail1 ? 1 : 0);
	log_(2, cerr << "readData ind. " << ind << ":";);
	if (ind >= 0) {
		for (int var = 0; var < MAX_ITEMS; ++var) {
			float val = atof(data[ind][var + 2]);
			Gdata.current[var].hora = 0;
			Gdata.current[var].dia = 0;
			Gdata.current[var].mes = val;
			log_(2, cerr << "var" << var << " = " << val << ", ";);
		}
		log_(2, cerr << "data " << data[ind][0] << "|" << data[ind][1] << ", ";);
		struct tm tread, *timelast;
		char ttime[3][MAX_STRSIZ];
		mysplit(data[ind][0], '-', (char *)ttime, MAX_STRSIZ);
		tread.tm_year = atoi(ttime[0]) - 1900;
		tread.tm_mon = atoi(ttime[1]) - 1;
		tread.tm_mday = atoi(ttime[2]);
		mysplit(data[ind][1], ':', (char *)ttime, MAX_STRSIZ);
		tread.tm_hour = atoi(ttime[0]);
		tread.tm_min = atoi(ttime[1]);
		tread.tm_sec = atoi(ttime[2]);
		char ctime[MAX_STRSIZ];
		sprintf(ctime, RECFMTTIM, tread.tm_year + 1900, tread.tm_mon + 1, tread.tm_mday,
			tread.tm_hour, tread.tm_min, tread.tm_sec);
		log_(2, cerr << " time = " << ctime;);
		time_t tlast = mktime(&tread);
		timelast = localtime(&tlast);
		if (timelast == NULL) {
			time(&tlast);
			timelast = localtime(&tlast);
			}
		Gdata.tlast = tlast;
		sprintf(ctime, RECFMTTIM, timelast->tm_year + 1900, timelast->tm_mon + 1, timelast->tm_mday, timelast->tm_hour, timelast->tm_min, timelast->tm_sec);
		log_(2, cerr << " time = " << ctime;);
		}
	log_(2, cerr << "\n";);
	}

FILE * openDatafile(char * datafile, void * data, bool * rfail) {
	// Obtém os dados em um arquivo

	*rfail = true;
	FILE * fp = NULL;
	if (!fexists(datafile)) {
		cerr << "Erro ao ler o arquivo de dados (" << datafile << ")\n";
	}
	else {
		int res = fgetlast(datafile, data);
		switch (res) {
		case FGETLAST_OK:
			*rfail = false;
			break;
		case FGETLAST_ERROPEN:
			cerr << "Erro ao gravar o arquivo de dados (" << datafile << ")\n";
			break;
		case FGETLAST_ERRSEEK:
			cerr << "Erro ao procurar o registro no arquivo de dados (" << datafile << ")\n";
			break;
		case FGETLAST_ERRREAD:
			cerr << "Erro ao ler o registro no arquivo de dados (" << datafile << ")\n";
			break;
		}
	}
	fp = fopen(datafile, "a");
	if (fp == NULL) {
		cerr << "Erro ao tentar gravar o arquivo de dados (" << datafile << ")\n";
	}
	return fp;
}

GetlastRes fgetlast(char * fname, void * pdata) {
	// Obtém o último registro do arquivo

	FILE * fp = fopen(fname, "a+");
	char record[255];
	if (fp == NULL) {
		return FGETLAST_ERROPEN;
	}
	int res = fseek(fp, -DREC_SIZE, SEEK_END);
	if (res != 0) {
		fclose(fp);
		return FGETLAST_ERRSEEK;
	}
	char * data = (char *)pdata;
	fgets(record, 255, fp);
	log_(3, cerr << "fgetlast " << record << "size = " << strlen(record););
	res = mysplit(record, ';', data, MAX_STRSIZ);
	log_(3, cerr << ", " << res << "\n";);
	log_(3, cerr << (char *)data << "|" << (char *)data + MAX_STRSIZ << "\n";);
	if (res != MAX_ITEMS + 2) {
		fclose(fp);
		return FGETLAST_ERRREAD;
		}
	fclose(fp);
	return FGETLAST_OK;
}

int mysplit(char * str, char car, char * dst, int siz) {
	// Divide a string de acordo com o separador dado

	int index = 0;
	char * pstr;
	while (true) {
		for (pstr = str; *pstr != car; ++pstr) {
			if (*pstr == '\0') {
				int ncar = pstr - str;
				strncpy(dst, str, ncar);
				dst[ncar] = '\0';
				dst += siz;
				str = pstr + 1;
				return index + 1;
			}
		}
		int ncar = pstr - str;
		strncpy(dst, str, ncar);
		dst[ncar] = '\0';
		dst += siz;
		str = pstr + 1;
		++index;
	}
	return index;
}

int readCfg(void) {
// Lê a configuração em disco
	FILE * fp = fopen("scvropc.cfg", "r");
	if (fp == NULL) {
		return 1;
		}
	char record[50], dados[3][50];
	fgets(record, 49, fp);
	mysplit(record, ';', (char *)dados, 50);
	strcpy(ServerInfo.nome, dados[0]);
	int numitems = atoi(dados[1]);
	ServerInfo.numitems = numitems;
	log_(3, cerr << "Servidor = " << ServerInfo.nome << ", nitens = " << numitems << ")\n";);
	for (int i = 0; i < numitems; ++i) {
		fgets(record, 49, fp);
		mysplit(record, ';', (char *) dados, 50);
		strcpy(ServerInfo.iteminfo[i].tag, dados[0]);
		strcpy(ServerInfo.iteminfo[i].nome, dados[1]);
		VARTYPE tipo;
		switch (dados[2][0]) {
			case 'L':
				tipo = VT_I4;
				break;
			case 'I':
				tipo = VT_I2;
				break;
			case 'S':
				tipo = VT_R4;
				break;
			case 'D':
				tipo = VT_R8;
				break;
			}
		ServerInfo.iteminfo[i].tipo = tipo;
		ServerInfo.itemdata[i].factor = atof(dados[3]);
		log_(3, cerr << "Tag = " << ServerInfo.iteminfo[i].tag << ", item = " << ServerInfo.iteminfo[i].nome << ", tipo = " << tipo << ", fator = " << ServerInfo.itemdata[i].factor << ")\n";);
		}
	fclose(fp);
	}


// Funções para tratamento de eventos
void openexc(void) {
// Trata o evento "botão 'Relatório' clicado"

	PROCESS_INFORMATION processInfo;
	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	memset(&processInfo, 0, sizeof(processInfo));
	startupInfo.cb = sizeof startupInfo;
	LPTSTR szCmdline = _tcsdup(TEXT(RELAT_PATH));
	BOOL WINAPI ok = CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
	if (!ok) {
		log_(1, cerr << "Tentou partir o Excel, sem sucesso.\n";);
		errno_t err = errno;
		log_(2, cerr << "Erro = " << errno << ".\n";);
		}
	return;
	}

#define acum_( var ) 	( Gdata.current[ var ].mes )
#define acud_( var ) 	( Gdata.current[ var ].dia )
#define acuh_( var ) 	( Gdata.current[ var ].hora )

void changeh(char * ctime) {
// Trata a mudança da hora
	// Grava registro horário
	FILE * fp = Gdata.plotdata.hdfile;
	const char * extrmsg = "";
	if (fp == NULL) {
		cerr << "Arquivo de dados horarios nao disponivel.\n";
		return;
		}
	fprintf(fp, RECFMTWRT, ctime, acum_(0), acum_(1), acum_(2),
		acum_(3), acum_(4), acum_(5), acum_(6), acum_(7), acum_(8));
	fflush(fp);
	if (Gdata.zh) {
		for (int var = 0; var < MAX_ITEMS; ++var) {
			acuh_(var) = 0;
			}
		extrmsg = "Dados horarios zerados.";
		}
	sprintf(Gdata.plotdata.res, "Gravou registro horario. %s", extrmsg);
	}

void changed(char * ctime) {
// Trata a mudança do dia
	FILE * fp = Gdata.plotdata.mdfile;
	const char * extrmsg = "";
	if (fp == NULL) {
		cerr << "Arquivo de dados diarios nao disponivel.\n";
		return;
		}
	fprintf(fp, RECFMTWRT, ctime, acum_(0), acum_(1), acum_(2),
		acum_(3), acum_(4), acum_(5), acum_(6), acum_(7), acum_(8));
	fflush(fp);
	if (Gdata.zd) {
		for (int var = 0; var < MAX_ITEMS; ++var) {
			acud_(var) = 0;
			}
		extrmsg = "Dados diarios zerados.";
		}
	sprintf(Gdata.plotdata.res, "Gravou registro diario. %s", extrmsg);
	}

void changem(char * ctime) {
// Trata a mudança do mês
	for (int var = 0; var < MAX_ITEMS; ++var) {
		acum_(var) = 0;
		}
	sprintf(Gdata.plotdata.res, "Dados mensais zerados.");
	}

#undef acum_
#undef acud_
#undef acuh_

void checktfront(bool * fronth, bool * frontd, bool * frontm, char * ctime) {
// Verifica se passou alguma fronteira de tempo
	*fronth = *frontd = *frontm = false;
	time_t tnow;
	time(& tnow);
	struct tm * timenow, *timelast;
	timenow = localtime(& tnow);
	int ynow = timenow-> tm_year;
	int mnow = timenow-> tm_mon;
	int dnow = timenow-> tm_mday;
	int hnow = timenow-> tm_hour;
	int inow = timenow-> tm_min;
	int snow = timenow-> tm_sec;
	timelast = localtime(& Gdata.tlast);
	Gdata.tlast = tnow;
	if (timelast == NULL) {
		return;
		}
	if (timelast -> tm_year < 115) {
		return;
		}
	int hlast = timelast -> tm_hour;
	int mlast = timelast -> tm_mon;
	int dlast = timelast -> tm_mday;
	*fronth = (hlast != hnow);
	*frontm = (mlast != mnow);
	*frontd = (dlast != dnow);
	if (* fronth || * frontd) {
		sprintf(ctime, RECFMTTIM, ynow + 1900, mnow + 1, dnow, hnow, inow, snow);
		}
	}

int InitData(char ptags[MAX_ITEMS][50]) {
// Inicializa as variáveis globais
	Gdata.duplicated = false;
	Gdata.zh = Gdata.zd = true;
	Gdata.verbose = DEFAULT_VERBOSE;
/*
	EnumWindows((WNDENUMPROC) & lpfn, 0);
	if (Gdata.duplicated) {
		cerr << "Duplicado!";
		return 1;
		}
*/
	// Lê os arquivos de configuração
	int retcode = readCfg();
	if (retcode != 0) {
		return retcode;
		}
	// Lê os últimos dados salvos
	PanelData * pdata = &Gdata.plotdata;
	readData(pdata);
	time_t tnow;
	time(&tnow);
	for (int i = 0; i < MAX_ITEMS; ++i) {
		Gdata.current[i].timestamp = tnow;
		}
	// Conecta-se ao servidor OPC
	retcode = InitOPC();
	if (retcode != 0) {
		return retcode;
		}
	for (int i = 0; i < ServerInfo.numitems; ++i) {
		strcpy(ptags[i], ServerInfo.iteminfo[i].tag);
		}
	return 0;
	}

BOOL CALLBACK lpfn ( HWND hWnd , int lParam ) {
// Enumera todas as janelas em busca do servidor DDE
// Pode ser usada para descobrir o título de uma janela qualquer
	char title [99] ;
	int size ;
	size = GetWindowText (hWnd , (LPWSTR) title , 100 ) ;
	static int count = 0 ;
	if ( size > 0 ) {
		if ( strcmp ( title , WINDOW_TITLE ) == 0 ) {
			Gdata . duplicated = true ;
			return false ;
			}
		}
	return true;
	}

void ReadAcum(double * pval) {
	for (int i = 0; i < MAX_ITEMS; ++i) {
		*pval++ = Gdata.current[i].hora;
		}
	for (int i = 0; i < MAX_ITEMS; ++i) {
		*pval++ = Gdata.current[i].dia;
		}
	for (int i = 0; i < MAX_ITEMS; ++i) {
		*pval++ = Gdata.current[i].mes;
		}
	bool fronth, frontd, frontm;
	char ctime[MAX_STRSIZ];
	checktfront(&fronth, &frontd, &frontm, ctime);
	if (frontm) {
		changem(ctime);
		}
	if (frontd) {
		changed(ctime);
		}
	if (fronth) {
		changeh(ctime);
		}
	}
