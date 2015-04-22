// Constants
// widgets
const int SEPARATION = 5 ;
const int MAXWIDTH = 1000 ;
const int MAXHEIGHT = 1000 ;
const int BUTTONWIDTH = 80 ; 
const int BUTTONHEIGHT = 35 ;
const int CTRLBARWIDTH = BUTTONWIDTH + 2 * SEPARATION ;
const int CTRLBARHEIGHT = 300 ;
const int STATBARWIDTH = 500 ;
const int STATBARHEIGHT = 30 ;
const int WINDOWBORDER = 5 ;
const int STATUSMSGSZ = 200 ;
const int MENUHEIGHT  = 30 ;
const int POINTRADIUS = 10 ;

const int DRAWAREAWIDTH = 1000 ;
const int MAXDRAWAREA = 1 ;

const int NUMVARS = 9 ;
const int BUFFSIZE = 1000 ;
const int QUALITY_GOOD = 192 ;
const char * TSERVER1_SERVER_NAME = "Microsoft Excel - dde.xls" ;
const char * TSERVER2_SERVER_NAME = "FIX DDE Server" ;
const char * FIX32_SERVER_NAME = "FIX DDE Server" ;
const char * EXCEL_SERVER_TYPE = "Excel" ;
const char * FIX32_SERVER_TYPE = "Fix32" ;
const char * EXCEL_APP = "Excel";
const char * FIX32_APP = "DMDDE";
const char * TSERVER1_TOPIC = "[dde.xls]Sheet1";
const char * TSERVER2_TOPIC = "DATA";
const char * FIX32_TOPIC = "DATA";
const int NUMSERVERS = 3;
const UINT DDE_NACK = 0 ;
const UINT DDE_ACK = 0x8000 ;
const int MAX_STRSIZ = 30 ;

// well-known, reserved colors
const GdkRGBA ColorWhite = { 1 , 1 , 1 , 1 } ;
const GdkRGBA ColorGrey = { 0.4 , 0.4 , 0.4 , 1 } ;
const GdkRGBA ColorBlack = { 0 , 0 , 0 , 1 } ;


// Enumerations
// Mouse modes
typedef enum { MMODE_NONE , MMODE_CENTER , MMODE_PATH , MMODE_REGION } MMode ;
typedef enum { PSERVER , TSERVER1 , TSERVER2 } TServer ;


// Structs to storing data shared among functions
typedef	struct {
	int width , height ;
	GdkWindow * gdk_window ;
	GtkWidget * window ;
	HWND hwnd ;
	bool first ;
	} WindowData ;
	
typedef	struct {
	bool first ;
	double x1 , y1 , x2 , y2 ;
	int i1 , j1 , i2 , j2 ;
	} PointData ;

typedef struct {
	char cdia [ MAX_STRSIZ ] , cmes [ MAX_STRSIZ ] , clast [ MAX_STRSIZ ] ;
	float dia , mes , last ;
	char tag [ MAX_STRSIZ ] ;
	} SumData ;

typedef struct {
	char var [ MAX_STRSIZ ] ;
	char value [ MAX_STRSIZ ] ;
	int quality ;
	char tstamp [ MAX_STRSIZ ] ;
	} ReadData ;
	
typedef struct {
	bool first ;
	WindowData * original ;
	GtkWidget * msg , * draw [ MAXDRAWAREA ] ;
	int num_areas , area_width [ MAXDRAWAREA ] ;
	SumData total [ NUMVARS ] ;
	char res[ MAX_STRSIZ ];
	} PanelData ;

typedef	struct {
	const char * name , * type ;
	HWND hwnd ;
	const char * app , * topic , * tag [ NUMVARS ] ;
	bool enabled ;
	} ServerData ;
	
typedef struct {
	ReadData buffer [ BUFFSIZE ] ;
	int posr , posw ;
	HWND client ;
	WNDPROC Oldfn ;
	int curvar ;
	} GlobalData ;
	
	
#define Min(x,y)	(((x)>(y))?(y):(x))
#define Max(x,y)	(((x)>(y))?(x):(y))


// Prototypes
// Window's event handlers
gboolean userclkon ( GtkWidget * widget , GdkEvent * event , gpointer user_data ) ;	
gboolean userclkoff ( GtkWidget * widget , GdkEvent * event , gpointer user_data ) ;	
void plot ( const GtkWidget * button , gpointer frame ) ;
void zoomin ( const GtkWidget * button , gpointer frame ) ;
void zoomout ( const GtkWidget * button , gpointer frame ) ;
gboolean draw ( GtkWidget * widget , cairo_t * cr , gpointer user_data ) ;
// Menu actions
void act_exportas ( GtkAction * action , gpointer user_data ) ;
void act_quit ( GtkAction * action , gpointer user_data ) ;
void act_about ( GtkAction * action , gpointer user_data ) ;
void act_tglMP ( GtkAction * action , gpointer user_data ) ;
void act_mousesel ( GtkAction * action , GtkRadioAction * current , gpointer user_data ) ;
// Mouse user input 
gboolean newcenter ( PanelData * plotdata , int i , int j ) ;
gboolean newpath ( PanelData * plotdata , int i , int j ) ;
gboolean newregion ( PanelData * plotdata , int i , int j , bool released ) ;
// Initialization
void gtkinit ( GtkWidget * window , const char * title , GtkWindowPosition position , int width , int height , const gchar * filename , PanelData * plotdata ) ;
int parseinput ( int argc , char * * argv , int & width , int & height , double & minx , double & maxx , double & miny , double & maxy , int & maxiters , int & hue , double & saturation , bool & showMP ) ;
GtkWidget * defbutton ( GtkWidget * window , const char * caption , int hpos , int vpos ) ; 	
void defwindow ( GtkWidget * window , const char * title , GtkWindowPosition position , int width , int height , const gchar * filename ) ;
GtkWidget * defframe ( GtkWidget * window , int type , const char * caption , int border , int hpos , int vpos , int width , int height ) ;
GtkWidget * defmenubar ( GtkWidget * window , GtkWidget * mainframe , PanelData * plotdata ) ;
void initdata ( WindowData & plotdata , int & width , int & height ) ;
GdkPixbuf * create_pixbuf ( const gchar * filename ) ;
// Problem-domain functions
void UpdateP ( PanelData * plotdata ) ;
void RenderVM ( GdkRGBA * vbuf , int image_size , int effective_iter , vector <int> & niter , int h , double sat , bool showMP , int nMP ) ;
void RenderM (int image_size, int effective_iter, vector <int> & niter, int hue, double saturation, bool showMP, int nMP , PanelData * vbuf ) ;
int ComputeM ( int image_size, float real_start , float real_end , float imag_start , float imag_end, int max_iters, vector <int> & buffer,vector <int> & ziter, bool showMP, int & nMP) ;
void RenderPGMM (int image_size, int effective_iter, vector <int> & buffer2, int h, double sat, bool showMP, int nMP)  ;
bool CheckBd ( int image_size, vector <int> & buffer,int x, int y) ;
int Periodicity (complex <double> z, vector <complex  <double> > & values, int max_iters) ;
// Support functions for plotting
void put_pixel ( GdkRGBA * vbuf , int x , int y , double red , double green , double blue , int width ) ;
void cleardirty ( GdkRectangle * dirty , GdkRGBA * vbuf , GdkRGBA * wbuf , int width , GtkWidget * window ) ;
void markpoint ( int i , int j , GdkRGBA * wbuf , int width , int height , GdkRectangle * dirty ) ;
void markregion ( PointData * region , GdkRGBA * wbuf , int width , GdkRectangle * dirty ) ;

float timeval_subtract ( struct timeval * result, struct timeval * x, struct timeval * y ) ;
void defplotarea ( GtkWidget * window , int width , int height , int posh , int posv , PanelData * plotdata ) ;
void paintdirty ( GdkRectangle * dirty , PanelData * plotdata ) ;

void totalize ( PanelData * plotdata , ServerData * pserver ) ;
BOOL CALLBACK lpfn(HWND hWnd, int lParam) ;
LRESULT CALLBACK WndProc (HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) ;
void write_at ( cairo_t * cr , int x , int y , char * text ) ;
long connectDDE ( PanelData * plotdata , ServerData * pserver ) ;
long listenDDE ( PanelData * plotdata , ServerData * pserver , int var ) ;
void readDDE ( char * var , char * value ) ;
int findvar ( const char * list[] , char * name ) ;
void receiveACK ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) ;
void receiveDATA ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) ;
void sendACK ( HWND server , ATOM item , bool ack ) ;