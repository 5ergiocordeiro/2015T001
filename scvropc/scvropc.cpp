#include <atlbase.h>
#include <objbase.h>
#include <iostream>
#include <time.h>
#include "opc_h.h"
#include "header.h"

using namespace std;

#define OPC_MATRIKON L"Matrikon.OPC.Simulation.1"
#define OPC_SERVER_NAME OPC_MATRIKON
//#define REMOTE_SERVER_NAME L"your_path"

MyOPCServerInfo ServerInfo = {
		FALSE, NULL, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, NULL, 3,
		{
			{ L"Random.Real8", VT_R8 }, { L"Saw-toothed Waves.Real8", VT_R8 }, { L"Square Waves.Real8", VT_R8 },
			{}, {}, {}, {}, {}, {},
		},
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
	int retcode = ReadItem(ServerInfo.pGroup, ServerInfo.hItem[item], varValue);
	if (retcode == 0) {
		return varValue.dblVal;
		}
	return 0;
	}

void ReadAllItems(double * pval) {
	for (int i = 0; i < ServerInfo.numitems; ++i) {
		* pval ++ = ReadTheItem(i);
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
	char data[2][NUMVARS + 2][MAX_STRSIZ];
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
		for (int var = 0; var < NUMVARS; ++var) {
			float val = atof(data[ind][var + 2]);
			plotdata->total[var].hora = 0;
			plotdata->total[var].dia = 0;
			plotdata->total[var].mes = val;
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
	if (res != NUMVARS + 2) {
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


#define acum_( var ) 	( plotdata -> total [ var ] . mes )
#define acud_( var ) 	( plotdata -> total [ var ] . dia )
#define acuh_( var ) 	( plotdata -> total [ var ] . hora )

void changeh(PanelData * plotdata, char * ctime) {
	// Trata a mudança da hora

	// Grava registro horário
	FILE * fp = plotdata->hdfile;
	const char * extrmsg = "";
	if (fp == NULL) {
		cerr << "Arquivo de dados horarios nao disponivel.\n";
		return;
	}
	fprintf(fp, RECFMTWRT, ctime, acum_(0), acum_(1), acum_(2),
		acum_(3), acum_(4), acum_(5), acum_(6), acum_(7), acum_(8));
	fflush(fp);
	if (Gdata.zh) {
		for (int var = 0; var < NUMVARS; ++var) {
			acuh_(var) = 0;
		}
		extrmsg = "Dados horarios zerados.";
	}
	sprintf(plotdata->res, "Gravou registro horario. %s", extrmsg);
}

void changed(PanelData * plotdata, char * ctime) {
	// Trata a mudança do dia

	FILE * fp = plotdata->mdfile;
	const char * extrmsg = "";
	if (fp == NULL) {
		cerr << "Arquivo de dados diarios nao disponivel.\n";
		return;
	}
	fprintf(fp, RECFMTWRT, ctime, acum_(0), acum_(1), acum_(2),
		acum_(3), acum_(4), acum_(5), acum_(6), acum_(7), acum_(8));
	fflush(fp);
	if (Gdata.zd) {
		for (int var = 0; var < NUMVARS; ++var) {
			acud_(var) = 0;
		}
		extrmsg = "Dados diarios zerados.";
	}
	sprintf(plotdata->res, "Gravou registro diario. %s", extrmsg);
}

void changem(PanelData * plotdata, char * ctime) {
	// Trata a mudança do mês

	for (int var = 0; var < NUMVARS; ++var) {
		acum_(var) = 0;
	}
	sprintf(plotdata->res, "Dados mensais zerados.");
}

#undef acum_
#undef acud_
#undef acuh_

void checktfront(bool * fronth, bool * frontd, bool * frontm, char * ctime) {
	// Verifica se passou alguma fronteira de tempo

	*fronth = *frontd = *frontm = false;
	time_t tnow;
	time(&tnow);
	struct tm * timenow, *timelast;
	timenow = localtime(&tnow);
	int ynow = timenow->tm_year;
	int mnow = timenow->tm_mon;
	int dnow = timenow->tm_mday;
	int hnow = timenow->tm_hour;
	int inow = timenow->tm_min;
	int snow = timenow->tm_sec;
	timelast = localtime(&Gdata.tlast);
	Gdata.tlast = tnow;
	if (timelast == NULL) {
		return;
	}
	if (timelast->tm_year < 115) {
		return;
	}
	int hlast = timelast->tm_hour;
	int mlast = timelast->tm_mon;
	int dlast = timelast->tm_mday;
	*fronth = (hlast != hnow);
	*frontm = (mlast != mnow);
	*frontd = (dlast != dnow);
	if (fronth || frontd) {
		sprintf(ctime, RECFMTTIM, ynow + 1900, mnow + 1, dnow, hnow, inow, snow);
	}
}


char * valstrcpy(char * dst, char * src) {
	// Copia os caracteres de src para dst com alguma sutileza

	char * psrc = src, *pdst = dst, car;
	while (true) {
		car = *psrc++;
		switch (car) {
		case '\r':
		case '\n':
		case '\0':
			*dst = car;
			return dst;
		case '.':
			if (Gdata.brazil) {
				break;
			}
		default:
			*dst++ = car;
		}
	}

}

void totalize(PanelData * plotdata, int var, float val) {
	// Totaliza os dados

	SumData * ptot = plotdata->total;
	ptot[var].dia += val;
	ptot[var].hora += val;
	ptot[var].mes += val;
}


void procbuf(PanelData * plotdata, ServerData * pserver) {
	// Processa os dados que estão no buffer

	int wpos = Gdata.posw;
	int rpos = Gdata.posr;
	ReadData * buf = Gdata.buffer;
	SumData * ptot = plotdata->total;
	float value;
	int i = wpos;
	while (i != rpos) {
		// Obtém um valor lido
		if (buf[i].quality == QUALITY_GOOD) {
			int var = buf[i].var;
			if (var >= 0) {
				char * val = buf[i].value;
				value = atof(val);
				log_(2, cerr << "reg. " << i << ", ";);
				procvar(plotdata, pserver, var, value, buf[i].timestamp);
				plotdata->total[var].tlastr = buf[i].timestamp;
			}
		}
		if (++i >= BUFFSIZE) {
			i = 0;
		}
	}
	Gdata.posw = i;
	for (int var = 0; var < NUMVARS; ++var) {
		value = ptot[var].lastval;
		time_t vtime;
		time(&vtime);
		procvar(plotdata, pserver, var, value, vtime);
	}
	return;
}

void procvar(PanelData * plotdata, ServerData * pserver, int var, float value, time_t vtime) {

	SumData * ptot = plotdata->total;
	// Obtém o último valor processsado
	float last = ptot[var].lastval;
	time_t lastt = ptot[var].tlastc;
	float area = 0;
	log_(2, cerr << "var " << var << ": ";);
	if (last < 0) {
		log_(2, cerr << "valor " << value << " desprezado. Timestamps = " << lastt << " e " << vtime << "\n";);
	}
	else {
		// integra
		int interval = difftime_(vtime, lastt);
		area = last * interval * pserver->factor[var];
		log_(2, cerr << last << " * " << interval << " -> " << area;);
	}
	ptot[var].lastval = value;
	ptot[var].last = area;
	ptot[var].tlastc = vtime;
	if (area > 0) {
		totalize(plotdata, var, area);
	}
	return;
}


void totalm(PanelData * plotdata, ServerData * pserver) {
}


int findvar(const char * list[], char * name) {
	// Encontra uma variável pelo nome

	for (int i = 0; i < NUMVARS; ++i) {
		if (strcmp(list[i], name) == 0) {
			return i;
		}
	}
	return -1;
}


int mygetopt(int argc, char * argv[], char *list) {
	return 0;
	}


void parsecmd(int argc, char * argv[]) {
	int vval = 0, sval = 0;
	char * zval = NULL, *tval = NULL, *ppos;
	bool zh = true, zd = true, te = false, tf = false, nval = false, mval = false, bval = false;
	char * nlist[NUMVARS], *mlist[NUMVARS];
	int res;
	int opterr = 0;
	char * optarg;
	int optopt, optind;
	while (-1 != (res = mygetopt(argc, argv, "v:s:n:m:z:b:t:"))) {
		switch (res) {
		case 'b':
			bval = true;
			break;
		case 'v':
			vval = torange(atoi(optarg), 0, 3);
			break;
		case 's':
			sval = torange(atoi(optarg), 0, 3);
			break;
		case 't':
			tval = optarg;
			te = !strcmp(tval, "e");
			tf = !strcmp(tval, "f");
			break;
		case 'n':
			for (int index = 0; index < NUMVARS; ++index) {
				nlist[index] = new char[MAX_STRSIZ];
			}
			unlist(optarg, nlist);
			nval = true;
			break;
		case 'm':
			for (int index = 0; index < NUMVARS; ++index) {
				mlist[index] = new char[MAX_STRSIZ];
			}
			unlist(optarg, mlist);
			mval = true;
			break;
		case 'z':
			zval = optarg;
			zh = strcmp(zval, "h");
			zd = strcmp(zval, "d");
			break;
		case '?':
			ppos = strchr((char *) "vstnmzb", optopt);
			if (ppos != NULL) {
				cerr << "Opção '" << optopt << "' requer um argumento.\n";
			}
			else {
				if (isprint(optopt)) {
					cerr << "Opção inválida: '" << optopt << "'.\n";
				}
				else {
					cerr << "Caracter inválido.\n";
				}
			}
			break;
		}
	}

	log_(2, cerr << "s = " << sval << " , v = " << vval << " , b = " << bval;);
	log_(2, cerr << ((te || tf) ? " , t = " : " , ") << (tf ? "f, " : ""););
	log_(2, cerr << (te ? "e, " : "") << (zh ? "zh, " : "") << (zd ? "zd, " : ""););
	if (nval) {
		log_(2, cerr << "n = ";);
		for (int index = 0; index < NUMVARS; ++index) {
			log_(2, cerr << nlist[index] << "|";);
		}
		log_(2, cerr << " ";);
	}
	if (mval) {
		log_(2, cerr << "m = ";);
		for (int index = 0; index < NUMVARS; ++index) {
			log_(2, cerr << mlist[index] << "|";);
		}
		log_(2, cerr << "\n";);
	}

	for (int index = optind; index < argc; ++index) {
		log_(2, cerr << "Non-option argument " << argv[index] << ".\n";);
	}


	Gdata.zd = zd;
	Gdata.zh = zh;
	Gdata.serverno = sval;
	Gdata.verbose = vval;
	Gdata.brazil = bval;
	Gdata.nick = mval;
	if (nval && mval && (sval == 3)) {
		Gdata.server.name = (te ? EXCEL_SERVER_NAME : FIX32_SERVER_NAME);
		Gdata.server.type = (te ? EXCEL_SERVER_TYPE : FIX32_SERVER_TYPE);
		Gdata.server.app = (te ? EXCEL_APP : FIX32_APP);
		Gdata.server.topic = (te ? EXCEL_TOPIC : FIX32_TOPIC);
		for (int index = 0; index < NUMVARS; ++index) {
			Gdata.server.tag[index] = nlist[index];
			Gdata.server.factor[index] = SCALE_FACTOR;
			Gdata.server.nick[index] = mlist[index];
		}
		//Pserver = &Gdata.server;
	}
	else {
		//Pserver = &Gserver[sval];
	}

	return;
}


float torange(float val, float min, float max) {
	float aux = min_(val, max);
	return max_(aux, min);
}


void unlist(char * list, char * array[]) {
	char * ppos, *pbegin = list;
	int index = 0;
	while (NULL != (ppos = strchr(pbegin, ','))) {
		strncpy(array[index], pbegin, ppos - pbegin);
		if (++index >= NUMVARS) {
			return;
		}
		pbegin = ppos + 1;
	}
	strcpy(array[index], pbegin);
}


int InitData(int argc, const char * argv[]) {
	// Processa a linha de comando
	Gdata.verbose = 3;
	parsecmd(argc, argv);

	// Inicializa as variáveis globais
	Gdata.posr = Gdata.posw = Gdata.curvar = 0;
	int wwidth = PLOTAREAWIDTH, wheigth = PLOTAREAHEIGTH;
	PanelData pdata;
	pdata.original = &original;
	if (Pserver == NULL) {
		Pserver = &Gserver[PSERVER];
	}
	Pserver->enabled = false;

	// Lê os últimos dados salvos
	readData(&pdata);
	return 0;
}
