#ifndef SIMPLE_OPC_CLIENT_H
#define SIMPLE_OPC_CLIENT_H

#define MAX_ITEMS 9
#define DEFAULT_VERBOSE 3

typedef struct MyOPCItemInfo {
	LPWSTR nome;
	VARTYPE tipo;
	char * tag;
	};
typedef struct MyOPCItemData {
	double factor;
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

// Constants
const int BUFFSIZE = 1000;
const int QUALITY_GOOD = 192;
const int NUMSERVERS = 3;
const int MAX_STRSIZ = 50;
const int PROC_INTERVAL = 10;
const int PROC_SINTERVAL = 1;
const int PROC_LINTERVAL = 100;
const int REQUEST_INTERVAL = 300;
const int TERMINATE_INTERVAL = 1000;
const char * HDATAFILE = "h_dados.txt";
const char * MDATAFILE = "d_dados.txt";
const char * RECFMTWRT = "%s;%8.0f;%8.0f;%8.0f;%8.0f;%8.0f;%8.0f;%8.0f;%8.0f;%8.0f\n";
const char * RECFMTRD = "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s";
const char * RECFMTTIM = "%04d-%02d-%02d;%02d:%02d:%02d";
const char * VAR1 = "FIX.VAZ_FIT1.F_CV";
const char * VAR2 = "FIX.VAZ_FIT2.F_CV";
const char * VAR3 = "FIX.VAZ_FIT3.F_CV";
const char * VAR4 = "FIX.VAZ_FIT4.F_CV";
const char * VAR5 = "FIX.VAZ_FIT5.F_CV";
const char * VAR6 = "FIX.VAZ_FIT6.F_CV";
const char * VAR7 = "FIX.VAZ_FIT9.F_CV";
const char * VAR8 = "FIX.VAZ_FIT11.F_CV";
const char * VAR9 = "FIX.VAZ_FIT12.F_CV";
const int DREC_SIZE = 102;
const char * WINDOW_TITLE = "Consumo de agua STRA";
const float SCALE_FACTOR = 1.0 / 3600;


// Enumerations
typedef enum { PSERVER, TSERVER1, TSERVER2 } TServer;
typedef enum { FGETLAST_OK, FGETLAST_ERROPEN, FGETLAST_ERRSEEK, FGETLAST_ERRREAD } GetlastRes;

typedef struct {
	time_t timestamp;
	double value, hora, dia, mes;
} ReadData;

typedef struct {
	char res[MAX_STRSIZ], hfile[MAX_STRSIZ], mfile[MAX_STRSIZ];
	FILE * hdfile, *mdfile;
} PanelData;

typedef struct {
	time_t tlast;
	int verbose;
	bool duplicated, zh, zd;
	ReadData current[MAX_ITEMS];
	PanelData plotdata;
	} GlobalData;



#define min_( x , y )				( ( ( x ) > ( y ) ) ? ( y ) : ( x ) )
#define max_( x , y )				( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define log_( x , c )				if ( Gdata . verbose >= ( x ) ) { c }
#define difftime_( a , b )			( ( a ) - ( b ) )			// a função difftime tem um bug



// Prototypes
// Initialization
int parseinput(int argc, char * * argv, int & width, int & height, double & minx, double & maxx, double & miny, double & maxy, int & maxiters, int & hue, double & saturation, bool & showMP);
int findvar(const char * list[], char * name);
bool fexists(char * fname);
void readData(PanelData * plotdata);
FILE * openDatafile(char * datafile, void * data, bool * rfail);
GetlastRes fgetlast(char * fname, void * pdata);
void changem(PanelData * plotdata, char * ctime);
void changed(PanelData * plotdata, char * ctime);
void changeh(PanelData * plotdata, char * ctime);
void checktfront(bool * fronth, bool * frontd, bool * frontm, char * ctime);
void runsrv(void);
int mysplit(char * str, char car, char * dst, int siz);
BOOL CALLBACK lpfn(HWND hWnd, int lParam);
IOPCServer* InstantiateServer(wchar_t ServerName[]);
IOPCItemMgt * AddTheGroup(IOPCServer * pIOPCServer, LPWSTR Groupname, OPCHANDLE & hServerGroup);
OPCHANDLE AddTheItem(IOPCItemMgt* pIOPCItemMgt, LPWSTR ItemID, VARTYPE ItemType);
int ReadItem(IUnknown * pGroup, OPCHANDLE hServerItem, VARIANT & varValue);
void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem);
void RemoveGroup(IOPCServer* pIOPCServer, OPCHANDLE hServerGroup);
int InitOPC(void);
void EndOPC(void);
int RunOPC(void);
double ReadTheItem(int item);
void ReadAllItems(double * pval);
int InitData(void);
int readCfg(void);
#endif