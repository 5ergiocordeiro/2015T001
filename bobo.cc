#include <gtk/gtk.h>
#include <stdlib.h>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <omp.h>
#include <windows.h>
#include <dde.h>
#include <gdk/gdkwin32.h>
#include <unistd.h>
#include <glib.h>

using namespace std;
#include "mandelplot.h"
#include "menubar.h"


#define difftime_( a , b )			( ( a ) - ( b ) )			// a fun��o difftime tem um bug


// Dados globais
// Esses dados precisam estar dispon�veis para as fun��es de callback
GlobalData Gdata ;
ServerData * Pserver = NULL ;
ServerData Gserver [ NUMSERVERS ] = {
	// Servidor de produ��o
	{ FIX32_SERVER_NAME , FIX32_SERVER_TYPE , NULL , FIX32_APP , FIX32_TOPIC ,
		{ VAR1 , VAR2 , VAR3 , VAR4 , VAR5 , VAR6 , VAR7 , VAR8 , VAR9 } ,
		{ "LC1 spray" , "LC2 spray" , "Lam 1" , "Lam 2" , "Morgan" , "Moldes" , "Torre L3" , "Lam 3" , "LPP3" } ,
		{ SCALE_FACTOR , SCALE_FACTOR , SCALE_FACTOR , SCALE_FACTOR , SCALE_FACTOR , SCALE_FACTOR ,  
			SCALE_FACTOR ,  SCALE_FACTOR ,  SCALE_FACTOR } , false } ,
	// Servidores de teste
	// ... planilha Excel
	{ EXCEL_SERVER_NAME , EXCEL_SERVER_TYPE , NULL , EXCEL_APP , EXCEL_TOPIC , 
		{ "r1c1" , "r2c1" , "r3c1" , "r4c1" , "r5c1" , "r6c1" , "r7c1" , "r8c1" , "r9c1" } ,
		{ "r1c1" , "r2c1" , "r3c1" , "r4c1" , "r5c1" , "r6c1" , "r7c1" , "r8c1" , "r9c1" } ,
		{ 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } , false } ,
	// ... Fix32 Sample System
	{ FIX32_SERVER_NAME , FIX32_SERVER_TYPE , NULL , FIX32_APP , FIX32_TOPIC ,
		{ "DEMO.RAMP.F_CV" , "DEMO.TEMP.F_CV.F_CV" , "DEMO.PRESSURE.F_CV" , "DEMO.VOLUME.F_CV" ,
		"DEMO.LEVEL.F_CV.F_CV" , "DEMO.PV1.F_CV" , "DEMO.PV2.F_CV" , "DEMO.RATE.F_CV" , "DEMO.TEMPTR.F_CV" } ,
		{ "tag0" , "tag1" , "tag2" , "tag3" , "tag4" , "tag5" , "tag6" , "tag7" , "tag8" } ,
		{ 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } , false } ,

	} ;
	

	
// Fun��es
int main ( int argc , char * argv [ ] ) {
// Syntaxe:
//   rdDDE {-s server_no} {-v verbose_level:0} {-zh} {-zd}
//   rdDDE -s3 -t server_type -n tagname1,tagname2 ... tagname9 -m nick1,nick2 ... nick9 {-v verbose_level:0} {-zh} {-zd} {-b}
// server_no: 0 a 3, default 0
// verbose_level: 0 a 3, default 0
//	3: trace para obter informa��es necess�rias para o desenvolvimento
//	2: trace detalhado para acompanhamento da execu��o
//	1: trace resumido para acompanhamento da execu��o
//	0: apenas mensagens de erro s�o exibidas
// -zh, -zd N�o zerar os valores nas fronteiras de hora e dia
// -b remover pontos de milhar
// server_type: Excel ou Fix32
// tagname1 a tagname9: tags
// nick1 a nick9: nicknames
// Exemplos:
//   rdDDE
//     l� do servidor de produ��o com todas as configura��es default
//   rdDDE -s0
//     l� do servidor de produ��o com todas as configura��es default
//   rdDDE -s1
//     l� do servidor de testes 1 com todas as configura��es default
//   rdDDE -s3 -t Fix32 -n 
//     l� do servidor de testes 3, que � um Fix32, as tags 

	Gdata . duplicated = false ;
	EnumWindows ( (WNDENUMPROC) & lpfn , 0 ) ;
	if ( Gdata . duplicated ) {
		cerr << "Duplicado!" ;
		return 1 ;
		}

	// Processa a linha de comando
	Gdata . verbose = 3 ;
	parsecmd ( argc , argv ) ;

	// Inicializa as vari�veis globais
	Gdata . client = NULL ;
	Gdata . Oldfn = NULL ;
	Gdata . posr = Gdata . posw = Gdata . curvar = 0 ;           
	WindowData original ;
	original . first = true ;
	original . gdk_window = NULL ;
	original . window = NULL ;
	original. hwnd = NULL ;
	int wwidth = PLOTAREAWIDTH , wheigth = PLOTAREAHEIGTH ;
	initdata ( original , wwidth , wheigth ) ;
	PanelData pdata ;
	pdata . original = & original ;
	if ( Pserver == NULL ) {
		Pserver = & Gserver [ PSERVER ] ;
		}
	Pserver -> enabled = false ;
	
	// Initializa o GTK+
	int dumbi = 1 ;
	gtk_init ( & dumbi , NULL ) ;
	
	// Cria a janela
	GtkWidget * window = gtk_window_new ( GTK_WINDOW_TOPLEVEL ) ;
	original . window = window ;
	gtkinit ( window , WINDOW_TITLE , GTK_WIN_POS_NONE , wwidth , wheigth , "techsteel.ico" , & pdata ) ;
	
	// L� os �ltimos dados salvos
	readData ( & pdata ) ;

	// Primeira atualiza��o dos dados
	InitP ( & pdata ) ;
	UpdateP ( & pdata ) ;

	// Registra os timers
	g_timeout_add_seconds ( PROC_INTERVAL , ( GSourceFunc ) timeout , & pdata ) ;
	g_timeout_add_seconds ( PROC_LINTERVAL , ( GSourceFunc ) ltimeout , & pdata ) ;
	g_timeout_add_seconds ( PROC_SINTERVAL , ( GSourceFunc ) stimeout , & pdata ) ;

	// Exibe a janela e passa o controle ao GTK+
	gtk_widget_show_all ( window ) ;
	gtk_main ( )  ;
 
	return 0;
	}

	
void defwindow ( GtkWidget * window , const char * title , GtkWindowPosition position , int width , int height , const gchar * filename ) {
// Define par�metros da janela

	gtk_window_set_title ( GTK_WINDOW ( window ) , title ) ;
	gtk_window_set_default_size ( GTK_WINDOW ( window ) , width , height ) ;
	gtk_window_set_position ( GTK_WINDOW ( window ) , position ) ;
	gtk_window_set_icon ( GTK_WINDOW ( window ) , create_pixbuf ( filename ) ) ;
	}


GtkWidget * defframe ( GtkWidget * window , int type , const char * caption , int border , int hpos , int vpos , int width , int height ) {
// Define caracter�sticas de um frame

	GtkWidget * frame ;
	if ( type < 0 ) {
		frame = gtk_fixed_new ( ) ;
		}
	else {
		frame = gtk_frame_new ( caption ) ;	
		gtk_frame_set_shadow_type ( GTK_FRAME ( frame ) , ( GtkShadowType ) type ) ;
		}
	gtk_container_add ( GTK_CONTAINER ( window ) , frame ) ;
	if ( border >= 0 ) {
		gtk_container_set_border_width ( GTK_CONTAINER ( frame ) , border ) ;
		}
	if ( ( width > 0 ) && ( height > 0 ) ) {
		gtk_widget_set_size_request ( frame , width , height ) ;
		}
	if ( ( hpos > 0 ) || ( vpos > 0 ) ) {
		gtk_fixed_move ( GTK_FIXED ( window ) , frame , hpos , vpos ) ;
		}
	return frame ;
	}

	
void gtkinit ( GtkWidget * window , const char * title , GtkWindowPosition position , int width , int height , const gchar * filename , PanelData * plotdata ) {
// Initializes window

	// Sets window properties
	defwindow ( window , title , position , width + CTRLBARWIDTH + SEPARATION + 2 * WINDOWBORDER , height + STATBARHEIGHT + SEPARATION + MENUHEIGHT + 2 * WINDOWBORDER , filename ) ;
	g_signal_connect_swapped ( G_OBJECT ( window ) , "destroy" , G_CALLBACK ( gtk_main_quit ) , NULL ) ;
	// Defines frames
	GtkWidget * mainframe = defframe ( window , -1 , "" , WINDOWBORDER , 0 , 0 , 0 , 0 ) ;
	GtkWidget * plotframe = defframe ( mainframe , GTK_SHADOW_IN , "Resultados" , 0 , 0 , MENUHEIGHT , width , height ) ;
	// Creates menu bar
	if ( defmenubar ( window , mainframe , plotdata ) == NULL ) {
		return ;
		}
	// Defines areas
	// plot area
	log_( 3 , cerr << "window w = " << width << " h = " << height << "\n" ; ) ;
	defplotarea ( mainframe , width , height , 0 , MENUHEIGHT , plotdata ) ;
	GtkWidget * plotbox = gtk_event_box_new ( ) ;
	gtk_widget_set_size_request ( plotbox , width , height ) ;
	gtk_fixed_put ( GTK_FIXED ( mainframe ) , plotbox, 0 , MENUHEIGHT ) ;
	g_signal_connect ( G_OBJECT ( plotbox ) , "button-release-event" , G_CALLBACK ( userclkoff ) , plotdata ) ; 	
	g_signal_connect ( G_OBJECT ( plotbox ) , "button-press-event" , G_CALLBACK ( userclkon ) , plotdata ) ; 	
	// status bar
	GtkWidget * statusbar = gtk_statusbar_new ( ) ;
	gtk_widget_set_size_request ( statusbar , STATBARWIDTH , STATBARHEIGHT ) ;
	gtk_fixed_put ( GTK_FIXED ( mainframe ) , statusbar , 0 , height + SEPARATION + MENUHEIGHT ) ;
	gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ) , gtk_statusbar_get_context_id ( GTK_STATUSBAR ( statusbar ) , "Initialized." ) , "Initialized." ) ;	
	plotdata -> msg = statusbar ;
	}

	
bool fexists ( char * fname ) {
// Verifica se o arquivo dado pode ser lido	
	FILE * fp = fopen ( fname , "r" ) ;
	if ( fp == NULL ) {
		return false ;
		}
	fclose ( fp ) ;
	return true ;
	}


void readData ( PanelData * plotdata ) {
// L� os �ltimos dados gravados
	
	// Tenta ler do arquivo de dados hor�rios e tamb�m do arquivo de dados di�rios
	char data [2] [ NUMVARS + 2 ] [ MAX_STRSIZ ] ;
	bool rfail1 , rfail2 ;
	sprintf ( plotdata -> hfile , HDATAFILE ) ;
	sprintf ( plotdata -> mfile , MDATAFILE ) ;	
	FILE * fph = openDatafile ( plotdata -> hfile , ( void * ) data [0] , & rfail1 ) ;
	plotdata -> hdfile = fph ;
	FILE * fpm = openDatafile ( plotdata -> mfile , ( void * ) data [1] , & rfail2 ) ;
	plotdata -> mdfile = fpm ;
	// Copia os dados lidos para as vari�veis globais
	int ind = ( rfail1 && rfail2 ) ? -1 : ( rfail1 ? 1 : 0 ) ;
	log_( 2 , cerr << cerr << "readData ind. " << ind << ":" ; ) ;
	if ( ind >= 0 ) {
		for ( int var = 0 ; var < NUMVARS ; ++ var ) {
			float val = atof ( data [ ind ] [ var + 2 ] ) ;
			plotdata -> total [ var ] . hora = 0 ;
			plotdata -> total [ var ] . dia = 0 ;
			plotdata -> total [ var ] . mes = val ;
			log_( 2 , cerr << "var" << var << " = " << val << ", " ; ) ;
			}
		log_( 2 , cerr << "data " << data [ ind ] [0] << "|" << data [ ind ] [1] << ", " ; ) ;
		struct tm tread , * timelast ;
		char ttime [3] [ MAX_STRSIZ ] ;
		mysplit ( data [ ind ] [0] , '-' , ( char * ) ttime , MAX_STRSIZ ) ;
		tread . tm_year = atoi ( ttime [0] ) - 1900 ;
		tread . tm_mon = atoi ( ttime [1] ) - 1 ;		
		tread . tm_mday = atoi ( ttime [2] ) ;
		mysplit ( data [ ind ] [1] , ':' , ( char * ) ttime , MAX_STRSIZ ) ;
		tread . tm_hour = atoi ( ttime [0] ) ;
		tread . tm_min = atoi ( ttime [1] ) ;		
		tread . tm_sec = atoi ( ttime [2] ) ;
		char ctime [ MAX_STRSIZ ] ;
		sprintf ( ctime , RECFMTTIM , tread . tm_year + 1900 , tread . tm_mon + 1 , tread . tm_mday ,
			tread . tm_hour , tread . tm_min , tread . tm_sec ) ;
		log_( 2 , cerr << " time = " << ctime ; ) ;
		time_t tlast = mktime ( & tread ) ;
		timelast = localtime ( & tlast ) ;
		if ( timelast == NULL ) {
			time ( & tlast ) ;
			timelast = localtime ( & tlast ) ;
			}
		Gdata . tlast = tlast ;
		sprintf ( ctime , RECFMTTIM , timelast -> tm_year + 1900 , timelast -> tm_mon + 1 , timelast -> tm_mday 				, timelast -> tm_hour , timelast -> tm_min , timelast -> tm_sec ) ;
		log_( 2 , cerr << " time = " << ctime ; ) ;
		}
	log_( 2 , cerr << "\n" ; ) ;
	}
		

FILE * openDatafile ( char * datafile , void * data , bool * rfail ) {
// Obt�m os dados em um arquivo
	
	* rfail = true ;
	FILE * fp = NULL ;
	if ( ! fexists ( datafile ) ) {
		cerr << "Erro ao ler o arquivo de dados (" << datafile << ")\n" ;
		}
	else {
		int res = fgetlast ( datafile , data ) ;
		switch ( res ) {
		case FGETLAST_OK :
			* rfail = false ;
			break ;
		case FGETLAST_ERROPEN :
			cerr << "Erro ao gravar o arquivo de dados (" << datafile << ")\n" ;
			break ;
		case FGETLAST_ERRSEEK :
			cerr << "Erro ao procurar o registro no arquivo de dados (" << datafile << ")\n" ;
			break ;
		case FGETLAST_ERRREAD :
			cerr << "Erro ao ler o registro no arquivo de dados (" << datafile << ")\n" ;
			break ;
			}
		}
	fp = fopen ( datafile , "a" ) ;
	if ( fp == NULL ) {
		cerr << "Erro ao tentar gravar o arquivo de dados (" << datafile << ")\n" ;
		}
	return fp ;
	}

	
GetlastRes fgetlast ( char * fname , void * pdata ) {
// Obt�m o �ltimo registro do arquivo

	FILE * fp = fopen ( fname , "a+" ) ;
	char record [255] ;
	if ( fp == NULL ) {
		return FGETLAST_ERROPEN ;
		}
	int res = fseek ( fp , - DREC_SIZE , SEEK_END ) ;
	if ( res != 0 ) {
		fclose ( fp ) ;
		return FGETLAST_ERRSEEK ;
		}
	char * data = ( char * ) pdata ;
	fgets ( record , 255 , fp ) ;
	log_( 3 , cerr << "fgetlast " << record << "size = " << strlen ( record ) ; ) ;	
	res = mysplit ( record , ';' , data , MAX_STRSIZ ) ;
	log_( 3 , cerr << ", " << res << "\n" ; ) ;
	log_( 3 , cerr << ( char * ) data << "|" << ( char * ) data + MAX_STRSIZ << "\n" ; ) ;
	if ( res != NUMVARS + 2 ) {
		fclose ( fp ) ;
		return FGETLAST_ERRREAD ;
		}
	fclose ( fp ) ;
	return FGETLAST_OK ;
	}

 
int mysplit ( char * str , char car , char * dst , int siz ) {
// Divide a string de acordo com o separador dado

	int index = 0 ;
	char * pstr ;
	while ( true ) {
		for ( pstr = str ; * pstr != car ; ++ pstr ) {
			if ( * pstr == '\0' ) {
				int ncar = pstr - str ;
				strncpy ( dst , str , ncar ) ;
				dst [ ncar ] = '\0' ;
				dst += siz ;
				str = pstr + 1 ;
				return index + 1 ;
				}
			}
		int ncar = pstr - str ;
		strncpy ( dst , str , ncar ) ;
		dst [ ncar ] = '\0' ;
		dst += siz ;
		str = pstr + 1 ;
		++ index ;
		}
	return index ;
 	}

	
void InitP ( PanelData * plotdata ) {
// Primeira atualiza��o
	sprintf ( plotdata -> res , "" ) ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		plotdata -> total [ var ] . last = 0 ;
		plotdata -> total [ var ] . lastval = -1 ;
		Gdata . torequest [ var ] = true ;
		Gdata . tounlisten [ var ] = false ;
		Gdata . tolisten [ var ] = true ;
		sprintf ( plotdata -> total [ var ] . tag , "tag%d" , var ) ;
		}
	}


void InitDDE ( PanelData * plotdata ) {
// Gerencia a conex�o com o servidor DDE

	// Estabelece conex�o via DDE
	if ( Gdata . client == NULL ) {
		return ;
		}
	if ( Pserver -> hwnd == NULL ) {
		sprintf ( plotdata -> res , "Conectando com o servidor..." ) ;
		if ( Pserver -> type == FIX32_SERVER_TYPE ) {
			runsrv ( ) ;
			}
		connectDDE ( plotdata , Pserver ) ;
		return ;
		}
	// L� os dados diretamente
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		if ( Gdata . tounlisten [ var ] ) {
			Gdata . tounlisten [ var ] = false ;
			Gdata . torequest [ var ] = true ;
			unlistenDDE ( plotdata , Pserver , var ) ;
			return ;
			}
		}
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		if ( Gdata . torequest [ var ] ) {
			sprintf ( plotdata -> res , "Atualizando variavel %d..." , var ) ;
			Gdata . curvar = var ;
			Gdata . tolisten [ var ] = true ;
			requestDDE ( plotdata , Pserver , var ) ;
			return ;
			}
		}
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		if ( Gdata . tolisten [ var ] ) {
			sprintf ( plotdata -> res , "Registrando-se para receber atualizacoes da variavel %d..." , var ) ;
			Gdata . curvar = var ;
			listenDDE ( plotdata , Pserver , var ) ;
			return ;
			}
		}
	Pserver -> enabled = true ;
	sprintf ( plotdata -> res , "Buffer: r = %d, w = %d" , Gdata . posr , Gdata . posw ) ;
	}

	
void UpdateP ( PanelData * plotdata ) {
// Atualiza os dados

	// Atualiza os totais
	procbuf ( plotdata , Pserver ) ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		SumData * ptot = & plotdata -> total [ var ];		
		sprintf ( ptot -> chora , "%8.0f" , ptot -> hora ) ;
		sprintf ( ptot -> cdia , "%8.0f" , ptot -> dia ) ;
		sprintf ( ptot -> cmes , "%8.0f" , ptot -> mes ) ;
		float lastval = ptot -> lastval ;
		if ( lastval < 0 ) {
			sprintf ( ptot -> clastval , " - " ) ;
			sprintf ( ptot -> ctlastr , " - " ) ;
			}
		else {
			struct tm * ptime = localtime ( & ptot -> tlastr ) ;
			sprintf ( ptot -> clastval , "%4.2f" , ptot -> lastval ) ;
			sprintf ( ptot -> ctlastr , "%02d/%02d/%04d %02d:%02d:%02d" ,
				ptime -> tm_mday , ptime -> tm_mon + 1 , ptime -> tm_year + 1900 ,
				ptime -> tm_hour , ptime -> tm_min , ptime -> tm_sec ) ;
			}
		}
	// Trata as fronteiras de tempo
	if ( ! Pserver -> enabled ) {
		return ;
		}
	bool fronth , frontd , frontm ;
	char ctime [ MAX_STRSIZ ] ;
	checktfront ( & fronth , & frontd , & frontm , ctime ) ;
	if ( frontm ) {
		changem ( plotdata , ctime ) ;
		}
	if ( frontd ) {
		changed ( plotdata , ctime ) ;
		}
	if ( fronth ) {
		changeh ( plotdata , ctime ) ;
		}
	}


void UpdateL ( PanelData * plotdata ) {
// Atualiza os dados
// Para ser usado em intervalos longos

	// Verifica se j� passou muito tempo desde a �ltima leitura de dados
	bool terminate = true ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		time_t lastt = plotdata -> total [ var ] . tlastr ;
		time_t tnow ;
		time ( & tnow ) ;
		double interval = difftime_( tnow , lastt ) ;
		Gdata . tounlisten [ var ] = ( interval > REQUEST_INTERVAL ) ;
		if ( interval < TERMINATE_INTERVAL ) {
			terminate = false ;
			}
		}
	if ( terminate ) {
		const char * msg = "Reiniciando a conex�o...";
		sprintf ( plotdata -> res , msg ) ; 
		log_( 1 , cerr <<  msg << "\n" ; ) ;
		endDDE ( plotdata , Pserver ) ;
		Pserver -> hwnd = 0 ;
		}
	}


void UpdateS ( PanelData * plotdata ) {
// Atualiza os dados
// Para ser usado em intervalos curtos

	// Trata a conex�o com o servidor
	InitDDE ( plotdata ) ;
	// Atualiza a tela
	gtk_widget_queue_draw_area ( plotdata -> draw [ 0 ] , 0 , 0 , ( plotdata -> area_width ) [ 0 ] ,
		plotdata -> original -> height ) ;

	}

	
#define acum_( var ) 	( plotdata -> total [ var ] . mes )
#define acud_( var ) 	( plotdata -> total [ var ] . dia )
#define acuh_( var ) 	( plotdata -> total [ var ] . hora )

void changeh ( PanelData * plotdata , char * ctime ) {
// Trata a mudan�a da hora

	// Grava registro hor�rio
	FILE * fp = plotdata -> hdfile ;
	const char * extrmsg = "" ;
	if ( fp == NULL ) {
		cerr << "Arquivo de dados horarios nao disponivel.\n" ;
		return ;
		}
	fprintf ( fp , RECFMTWRT , ctime , acum_(0) , acum_(1) , acum_(2) ,
		acum_(3) , acum_(4) , acum_(5) , acum_(6) , acum_(7) , acum_(8) ) ;
	fflush ( fp ) ;
	if ( Gdata . zh ) {
		for ( int var = 0 ; var < NUMVARS ; ++ var ) {
			acuh_( var ) = 0 ;
			}
		extrmsg = "Dados horarios zerados." ;
		}
	sprintf ( plotdata -> res , "Gravou registro horario. %s" , extrmsg ) ;
	}
	

void changed ( PanelData * plotdata , char * ctime ) {
// Trata a mudan�a do dia

	FILE * fp = plotdata -> mdfile ;
	const char * extrmsg = "" ;
	if ( fp == NULL ) {
		cerr << "Arquivo de dados diarios nao disponivel.\n" ;
		return ;
		}
	fprintf ( fp , RECFMTWRT , ctime , acum_(0) , acum_(1) , acum_(2) ,
		acum_(3) , acum_(4) , acum_(5) , acum_(6) , acum_(7) , acum_(8) ) ;
	fflush ( fp ) ;
	if ( Gdata . zd ) {
		for ( int var = 0 ; var < NUMVARS ; ++ var ) {
			acud_( var ) = 0 ;
			}
		extrmsg = "Dados diarios zerados." ;
		}
	sprintf ( plotdata -> res , "Gravou registro diario. %s" , extrmsg ) ;
	}


void changem ( PanelData * plotdata , char * ctime ) {
// Trata a mudan�a do m�s

	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		acum_( var ) = 0 ;
		}
	sprintf ( plotdata -> res , "Dados mensais zerados." ) ;
	}
	
#undef acum_
#undef acud_
#undef acuh_

	
void checktfront ( bool * fronth , bool * frontd , bool * frontm , char * ctime ) {
// Verifica se passou alguma fronteira de tempo

	* fronth = * frontd = * frontm = false ;
	time_t tnow ;
	time ( & tnow ) ;
	struct tm * timenow , * timelast ;
	timenow = localtime ( & tnow ) ;
	int ynow = timenow -> tm_year ;
	int mnow = timenow -> tm_mon ;
	int dnow = timenow -> tm_mday ;
	int hnow = timenow -> tm_hour ;
	int inow = timenow -> tm_min ;
	int snow = timenow -> tm_sec ;
	timelast = localtime ( & Gdata . tlast ) ;
	Gdata . tlast = tnow ;
	if ( timelast == NULL ) {
		return ;
		}
	if ( timelast -> tm_year < 115 ) {
		return ;
		}
	int hlast = timelast -> tm_hour ;
	int mlast = timelast -> tm_mon ;
	int dlast = timelast -> tm_mday ;
	* fronth = ( hlast != hnow ) ;
	* frontm = ( mlast != mnow ) ;
	* frontd = ( dlast != dnow ) ;
	if ( fronth || frontd ) {
		sprintf ( ctime , RECFMTTIM , ynow + 1900 , mnow + 1 , dnow , hnow , inow , snow ) ;
		}
	}

	
long connectDDE ( PanelData * plotdata , ServerData * pserver ) {
// Conecta-se ao servidor DDE

	// Envia broadcast com os dados de aplica��o e t�pico
	ATOM aapp = GlobalAddAtom ( pserver -> app) ;
	ATOM atopic = GlobalAddAtom ( pserver -> topic ) ;
	long res = SendMessage ( (HWND) -1 , WM_DDE_INITIATE , (WPARAM) Gdata . client , MAKELPARAM ( aapp , atopic ) ) ;
	log_( 2 , cerr << "Enviou DDE_INITIATE " << pserver -> app << "|" << pserver -> topic << " res = " << res ; ) ;
	GlobalDeleteAtom (aapp) ;
	GlobalDeleteAtom (atopic) ;	
	log_( 2 , cerr << " from " << Gdata . client << " to " << pserver -> hwnd << "\n" ; ) ;
	return res ;
	}

	
long listenDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
// Registra-se para receber atualiza��es do servidor DDE

	GLOBALHANDLE hOptions = GlobalAlloc ( GMEM_MOVEABLE , sizeof(DDEADVISE) ) ;
	if ( hOptions == NULL ) {
		cerr << "Erro na aloca��o de mem�ria\n" ;
		return -1;
		}
	DDEADVISE * lpOptions = (DDEADVISE FAR*) GlobalLock ( hOptions ) ; 
	if ( lpOptions == NULL ) {
		cerr << "Erro depois da aloca��o de mem�ria\n" ;
		GlobalFree ( hOptions ) ; 
		return -2; 
		} 
	ATOM atag = GlobalAddAtom ( pserver -> tag [ var ] ) ;
	lpOptions -> cfFormat = CF_TEXT ; 
	lpOptions -> fAckReq = FALSE ; 
	lpOptions -> fDeferUpd = FALSE ; 
	GlobalUnlock ( hOptions ) ; 
	long res = PostMessage ( pserver -> hwnd , WM_DDE_ADVISE , ( WPARAM ) Gdata . client ,
		PackDDElParam ( WM_DDE_ADVISE , ( UINT ) hOptions , atag ) ) ;
	log_( 2 , cerr << "Enviou DDE_ADVISE " << pserver -> tag [ var ] << " res = " << res << "\n" ; ) ;
	GlobalFree ( hOptions ) ; 
	GlobalDeleteAtom ( atag ) ;
	return res ;
	}

	
long requestDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
// L� dados do servidor DDE

	ATOM atag = GlobalAddAtom ( pserver -> tag [ var ] ) ;
	long res = PostMessage ( pserver -> hwnd , WM_DDE_REQUEST , ( WPARAM ) Gdata . client ,
		PackDDElParam ( WM_DDE_REQUEST , CF_TEXT , atag ) ) ;
	log_( 2 , cerr << "Enviou DDE_REQUEST " << pserver -> tag [ var ] << " res = " << res << "\n" ; ) ;
	GlobalDeleteAtom ( atag ) ;
	return res ;
	}

	
long unlistenDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
// Avisa ao servidor DDE para n�o mais enviar dados por exce��o

	ATOM atag = GlobalAddAtom ( pserver -> tag [ var ] ) ;
	long res = PostMessage ( pserver -> hwnd , WM_DDE_UNADVISE , ( WPARAM ) Gdata . client ,
		PackDDElParam ( WM_DDE_UNADVISE , 0 , atag ) ) ;
	log_( 2 , cerr << "Enviou DDE_UNADVISE " << pserver -> tag [ var ] << " res = " << res << "\n" ; ) ;
	GlobalDeleteAtom ( atag ) ;
	return res ;
	}
	

long endDDE ( PanelData * plotdata , ServerData * pserver ) {
// Termina a conex�o com o servidor DDE

	long res = PostMessage ( pserver -> hwnd , WM_DDE_TERMINATE , ( WPARAM ) Gdata . client ,
		PackDDElParam ( WM_DDE_TERMINATE , 0 , 0 ) ) ;
	log_( 2 , cerr << "Enviou DDE_TERMinaTe res = " << res << "\n" ; ) ;
	return res ;
	}
	

LRESULT CALLBACK WndProc ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) {
// Processa as mensagens recebidas

	switch ( imsg ) {
	case WM_DDE_ACK :
		receiveACK ( wnd , imsg , wparam , lparam ) ;
		break ;
	case WM_DDE_DATA :
		receiveDATA ( wnd , imsg , wparam , lparam ) ;
		break ;
		}
	// Chama o tratador de mensagens anterior para tratar as demais mensagens
	return CallWindowProc ( Gdata . Oldfn , wnd , imsg , wparam , lparam ) ;
	}

	
void receiveACK ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) {
// Trata recebimento de um WM_DDE_ACK

	log_( 2 , cerr << "ACK W = " << wparam << ", L = " << lparam ; ) ;
	UINT low , high ;
	UnpackDDElParam ( WM_DDE_DATA , lparam , & low, & high ) ;
	FreeDDElParam ( WM_DDE_DATA , lparam ) ;
	if ( Pserver -> hwnd == NULL ) {
		// ACK para o WM_DDE_INITIATE (conex�o inicial)
		Pserver -> hwnd = (HWND) wparam ;
		log_( 2 , cerr << "\n" ; ) ;
		return ;
		}
	// ACK to WM_DDE_ADVISE (registro para recebimento de dados)
	char msg [ MAX_STRSIZ ] ;
	bool ack = false ;
	switch ( low ) {
	case DDE_ACK :
		strcpy( msg , "ACK" ) ;
		ack = true ;
		break ;
	case DDE_NACK :
		strcpy( msg , "NACK" ) ;
		break ;
	default :
		itoa ( low , msg , 16 ) ;
		}
	log_( 2 , cerr << " " << msg << " high = " << high << ", low = " << low << "\n" ; ) ;
	if ( ack ) {
		Gdata . tolisten [ Gdata . curvar ] = false ;
		}
	// Envia ACK ao servidor DDE
	ATOM aitem = ( ATOM ) high ;
	sendACK ( Pserver -> hwnd , aitem , true ) ;
	}

	
void receiveDATA ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) {
// Trata recebimento de um WM_DDE_DATA
	
	time_t tnow ;
	time ( & tnow ) ;
	log_( 2 , cerr << "DATA W = " << wparam << ", L = " << lparam ; ) ;
	UINT low , high ;
	UnpackDDElParam ( WM_DDE_DATA , lparam , & low, & high ) ;
	FreeDDElParam (WM_DDE_DATA , lparam ) ;
	ATOM aitem = ( ATOM ) high ;
	GLOBALHANDLE hdata = ( GLOBALHANDLE ) low ;
	DDEDATA FAR* pdata = ( DDEDATA FAR* ) GlobalLock ( hdata ) ;
	log_( 2 , cerr << " = " << high << ", " << low ; ) ;
	char item [ MAX_STRSIZ ] ;
	// Se os dados recebidos estiverem no formato esperado, grava-os no buffer
	if ( pdata -> cfFormat == CF_TEXT ) {
		GlobalGetAtomName ( aitem , item , sizeof (item) ) ;
		char val [ MAX_STRSIZ ] ;
		valstrcpy ( val , ( char * ) pdata -> Value ) ;
		log_( 2 , cerr << " : " << item << ", " << val << " @" << Gdata . posr ; ) ;
		readDDE ( item , val , tnow ) ;	
		GlobalDeleteAtom ( aitem ) ;
		// Envia ACK ao servidor DDE
		if ( pdata -> fAckReq ) {
			sendACK ( Pserver -> hwnd , aitem , true ) ;
			}
		}
	else {
		// Envia NACK ao servidor DDE
		sendACK ( Pserver -> hwnd , aitem , false ) ;
		}
	bool release = pdata -> fRelease ;
	GlobalUnlock ( hdata ) ;
	if ( release ) {
		GlobalFree ( hdata ) ;
		}
	}
	

char * valstrcpy ( char * dst , char * src ) {
// Copia os caracteres de src para dst com alguma sutileza

	char * psrc = src , * pdst = dst , car ;
	while ( true ) {
		car = * psrc ++ ;
		switch ( car ) {
		case '\r' :
		case '\n' :
		case '\0' :
			* dst = car ;
			return dst ;
		case '.' :
			if ( Gdata . brazil ) {
				break ;
				}
		default :
			* dst ++ = car ;
			}
		}
		
	}

	
void sendACK ( HWND server , ATOM item , bool ack ) {
// Envia ACK ao servidor
	PostMessage ( server , WM_DDE_ACK, (WPARAM) Gdata . client , PackDDElParam ( WM_DDE_ACK , DDE_ACK , item ) ) ;
	log_( 2 , cerr << "\t sent " << ( ack ? "ACK" : "NACK" ) << "\n" ; ) ;
	}
	
	
void readDDE ( char * var , char * value , time_t tstamp ) {
// Armazena os dados recebidos do servidor DDE
	int nvar = findvar ( Pserver -> tag , var ) ;
	if ( nvar < 0 ) {
		nvar = Gdata . curvar ;
		}
	log_( 2 , cerr << " var " << nvar << "\n" ;  ) ;
	Gdata . torequest [ nvar ] = false ;
	ReadData * pbuf = & Gdata . buffer [ Gdata . posr ] ;
	strcpy ( pbuf -> value , value ) ;
	pbuf -> var = nvar ;
	pbuf -> quality = QUALITY_GOOD ;
	pbuf -> timestamp = tstamp ;
	if ( ++ Gdata . posr >= BUFFSIZE ) {
		Gdata . posr = 0 ;
		}
	}

	
BOOL CALLBACK lpfn ( HWND hWnd , int lParam ) {
// Enumera todas as janelas em busca do servidor DDE
// Pode ser usada para descobrir o t�tulo de uma janela qualquer
	char title [99] ;
	int size ;
	size = GetWindowText ( hWnd , title , 100 ) ;
	static int count = 0 ;
	if ( size > 0 ) {
		if ( strcmp ( title , WINDOW_TITLE ) == 0 ) {
			Gdata . duplicated = true ;
			return false ;
			}
		}
	return true;
	}

	
void totalize ( PanelData * plotdata , int var , float val ) {
// Totaliza os dados
	
	SumData * ptot = plotdata -> total ;
	ptot [ var ] . dia += val ;
	ptot [ var ] . hora += val ;
	ptot [ var ] . mes += val ;
	}

	
void procbuf ( PanelData * plotdata , ServerData * pserver ) {
// Processa os dados que est�o no buffer

	int wpos = Gdata . posw ;
	int rpos = Gdata . posr ;
	ReadData * buf = Gdata . buffer ;
	SumData * ptot = plotdata -> total ;
	float value ;
	int i = wpos ;
	while ( i != rpos ) {
		// Obt�m um valor lido
		if ( buf [ i ] . quality == QUALITY_GOOD ) {
			int var = buf [ i ] . var ;
			if ( var >= 0 ) {
				char * val = buf [ i ] . value ;
				value = atof ( val ) ;
				log_( 2 , cerr << "reg. " << i << ", " ; ) ;
				procvar ( plotdata , pserver , var , value , buf [ i ] . timestamp ) ;
				plotdata -> total [ var ] . tlastr = buf [ i ] . timestamp ;
				}
			}
		if ( ++ i >= BUFFSIZE ) {
			i = 0 ;
			}
		}
	Gdata . posw = i ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		value = ptot [ var ] . lastval ;
		time_t vtime ;
		time ( & vtime ) ;
		procvar ( plotdata , pserver , var , value , vtime ) ;
		}
	return ;
	}
	
void procvar ( PanelData * plotdata , ServerData * pserver , int var , float value , time_t vtime ) {

	SumData * ptot = plotdata -> total ;
	// Obt�m o �ltimo valor processsado
	float last = ptot [ var ] . lastval ;
	time_t lastt = ptot [ var ] . tlastc ;
	float area = 0 ;
	log_( 2 , cerr << "var " << var << ": " ; ) ;
	if ( last < 0 ) {
		log_( 2 , cerr << "valor " << value << " desprezado. Timestamps = " << lastt << " e " << vtime << "\n" ; ) ;
		}
	else {
		// integra
		int interval = difftime_( vtime , lastt ) ;
		area = last * interval * pserver -> factor [ var ];
		log_( 2 , cerr << last << " * " << interval << " -> " << area ; ) ;
		}
	ptot [ var ] . lastval = value ;
	ptot [ var ] . last = area ;
	ptot [ var ] . tlastc = vtime ;
	if ( area > 0 ) {
		totalize ( plotdata , var , area ) ;
		}
	return ;
	}


void totalm ( PanelData * plotdata , ServerData * pserver ) {
	}
	

void defplotarea ( GtkWidget * window , int width , int height , int posh , int posv , PanelData * plotdata ) {
// Define uma drawing area
// Pode ser fatiada em partes independentes para aumentar o desempenho

	int i ;
	for ( i = 0 ; ( i < MAXDRAWAREA ) && ( width > 0 ) ; ++ i , width -= DRAWAREAWIDTH ) {
		int awidth = min_( width , DRAWAREAWIDTH ) ;
		GtkWidget * plotarea = gtk_drawing_area_new ( ) ;
		gtk_widget_set_size_request ( plotarea , awidth , height ) ;
		gtk_fixed_put ( GTK_FIXED ( window ) , plotarea , posh + i * DRAWAREAWIDTH , posv ) ;
		plotdata -> draw [ i ] = plotarea ;
		g_signal_connect ( G_OBJECT ( plotarea ) , "draw" , G_CALLBACK ( draw ) , plotdata ) ;
		plotdata -> area_width [ i ] = awidth ;
		}
	plotdata -> num_areas = i ;
	return ;
	}

	
GtkWidget * defbutton ( GtkWidget * window , const char * caption , int hpos , int vpos ) {
// Define um bot�o

	GtkWidget * button = gtk_button_new_with_label ( caption ) ;
	gtk_widget_set_size_request ( button , BUTTONWIDTH , BUTTONHEIGHT ) ;
	gtk_fixed_put ( GTK_FIXED ( window ) , button , hpos , vpos ) ;	
	return button ;
	}
	
	
void act_quit ( GtkAction * action , gpointer user_data ) {
// Trata o evento "op��o Quit selecionada"

	gtk_main_quit ( ) ;
	return ;
	}


void act_openexc ( GtkAction * action , gpointer user_data ) {
// Trata o evento "op��o Abrir selecionada"

	GPid pid ;
	GError * error = NULL ;
	gchar * cmd [] = { ( gchar * ) "excel.exe" , ( gchar * ) "ts_rateio_mensal.xls" } ;
	g_spawn_async ( NULL , cmd , NULL , G_SPAWN_SEARCH_PATH , NULL , NULL , & pid , & error ) ;
	log_( 1 , cerr << "Tentou partir o Excel, " << ( ( error == NULL ) ? "com" : "sem" ) << " sucesso." ; ) ;
	return ;
	}


void runsrv ( ) {
// Trata o evento "op��o Abrir selecionada"

	GPid pid ;
	GError * error = NULL ;
	gchar * cmd [] = { ( gchar * ) "dmdde.exe" , NULL } ;
	g_spawn_async ( NULL , cmd , NULL , G_SPAWN_SEARCH_PATH , NULL , NULL , & pid , & error ) ;
	log_( 1 , cerr << "Tentou partir o DMDDE, " << ( ( error == NULL ) ? "com" : "sem" ) << " sucesso." ; ) ;
	return ;
	}


void act_exportas ( GtkAction * action , gpointer user_data ) {
	}

	
void act_about ( GtkAction * action , gpointer user_data ) {
	}
	
	
GdkPixbuf * create_pixbuf ( const gchar * filename ) {
// Obt�m bitmap de um arquivo
// A convers�o de formato � feita automaticamente

	GdkPixbuf * pixbuf ;
	GError * error = NULL ;
	pixbuf = gdk_pixbuf_new_from_file ( filename , & error ) ;
	if ( ! pixbuf ) {
		fprintf ( stderr, "%s\n", error -> message ) ;
		g_error_free ( error ) ;
		}
	return pixbuf ;
	}
	

void initdata ( WindowData & plotdata , int & width , int & height ) {

	plotdata . width = width ;	
	plotdata . height = height ;
	return ;
	}

	
GtkWidget * defmenubar ( GtkWidget * window , GtkWidget * mainframe , PanelData * plotdata ) {
// Define a barra de menu
// Ver arquivo menubar.h

    GtkActionGroup * actions = gtk_action_group_new ( "Actions" ) ;
    gtk_action_group_add_actions ( actions , entries , n_entries , plotdata ) ;
    
    GtkUIManager * ui = gtk_ui_manager_new ( ) ;
    gtk_ui_manager_insert_action_group ( ui , actions , 0 ) ;

	gtk_window_add_accel_group ( GTK_WINDOW ( window ) , gtk_ui_manager_get_accel_group ( ui ) ) ;
    GError * error = NULL ;
    if ( ! gtk_ui_manager_add_ui_from_string ( ui , ui_info , -1 , & error ) ) {
		g_message ( "Building menus failed: %s" , error -> message ) ;
		g_error_free ( error ) ;
		return NULL ;
		}
    GtkWidget * menubox = gtk_box_new ( GTK_ORIENTATION_VERTICAL , 0 ) ;
    gtk_container_add ( GTK_CONTAINER ( mainframe ) , menubox ) ;
	gtk_fixed_move ( GTK_FIXED ( mainframe ) , menubox , 0 , 0 ) ;
    gtk_box_pack_start ( GTK_BOX ( menubox) , gtk_ui_manager_get_widget ( ui , "/MenuBar" ) , FALSE , FALSE , 0 ) ;
	return window;
	}

	
void zoomin ( const GtkWidget * button , gpointer frame ) {

	return ;
	}

	
void zoomout ( const GtkWidget * button , gpointer frame ) {

	return ;
	}

	
gboolean userclkoff ( GtkWidget * widget , GdkEvent * event , gpointer user_data ) {

	return false ;
	}
	
	
gboolean userclkon ( GtkWidget * widget , GdkEvent * event , gpointer user_data ) {
// Trata o evento "bot�o do mouse pressionado"

	return false ;
	}

	
void update_txt ( cairo_t * cr , PanelData * plotdata ) {
// Atualiza os dados mostrados na tela

	cairo_set_source_rgb ( cr , 0.1 , 0.1 , 0.1 ) ; 
	cairo_select_font_face ( cr , "Purisa" , CAIRO_FONT_SLANT_NORMAL , CAIRO_FONT_WEIGHT_BOLD ) ;
	cairo_set_font_size ( cr , 13 ) ;
	write_at ( cr , 30 , 30 , "Tag" ) ;
	write_at ( cr , 150 , 30 , "m3 hora" ) ;
	write_at ( cr , 210 , 30 , "m3 dia" ) ;
	write_at ( cr , 270 , 30 , "m3 mes" ) ;
	write_at ( cr , 330 , 30 , "m3/h atual" ) ;
	SumData * result = plotdata -> total ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		write_at ( cr , 30 , 30 * ( var + 2 ) , Pserver -> nick [ var ] ) ;
		write_at ( cr , 150 , 30 * ( var + 2 ) , result [ var ] . chora ) ;  
		write_at ( cr , 210 , 30 * ( var + 2 ) , result [ var ] . cdia ) ;  
		write_at ( cr , 270 , 30 * ( var + 2 ) , result [ var ] . cmes ) ;
		write_at ( cr , 330 , 30 * ( var + 2 ) , result [ var ] . clastval ) ;
		write_at ( cr , 390 , 30 * ( var + 2 ) , result [ var ] . ctlastr ) ;
		}
	gtk_statusbar_push ( GTK_STATUSBAR ( plotdata -> msg ) ,
		gtk_statusbar_get_context_id ( GTK_STATUSBAR ( plotdata -> msg ) , plotdata -> res ) , plotdata -> res ) ;	
	}

	
gboolean draw ( GtkWidget * widget , cairo_t * cr , gpointer user_data ) {
// Trata o evento "draw"
  
	PanelData * plotdata = ( PanelData * ) user_data ;
	update_txt ( cr , plotdata ) ;

	if ( plotdata -> original -> first ) {
		log_( 2 , cerr << "gtk_window: " << plotdata -> original -> window << "\n" ; ) ;
		GdkWindow * gdk_window = gtk_widget_get_window ( plotdata -> original -> window ) ;
		log_( 2 , cerr << "gdk_window: " << gdk_window << "\n" ; ) ;
		if ( gdk_window != NULL ) {
			gboolean b = gdk_win32_window_is_win32 ( gdk_window ) ;
			log_( 2 , cerr << "is win32: " << b << "\n" ; ) ;
			HWND hwnd = ( HWND ) gdk_win32_window_get_handle ( gdk_window ) ;
			log_( 2 , cerr << "HWND: " << hwnd << "\n" ; ) ;
			plotdata -> original -> hwnd = hwnd ;
			Gdata . client = hwnd ;
			SetWindowText ( hwnd , WINDOW_TITLE ) ;
			Gdata . Oldfn = (WNDPROC) GetWindowLong ( hwnd , GWL_WNDPROC ) ;
			log_( 2 , cerr << "Old proc: " << Gdata . Oldfn << "\n" ; ) ;
			SetWindowLong ( hwnd , GWL_WNDPROC , (LONG) WndProc ) ;
			log_( 2 , cerr << "New proc: " << WndProc << "\n" ; ) ;
			plotdata -> original -> first = false ;
			}
		}
	return false;
	}
	
	
void plot ( const GtkWidget * button , gpointer frame ) {

	return ;
	}

	
void act_mousesel ( GtkAction * action , GtkRadioAction * current , gpointer user_data ) {

	}

	
void act_tglMP ( GtkAction * action , gpointer user_data  ) {

	return ;
	}

	
int findvar ( const char * list[] , char * name ) {
// Encontra uma vari�vel pelo nome

	for ( int i = 0 ; i < NUMVARS ; ++ i ) {
		if ( strcmp ( list [i] , name ) == 0 ) {
			return i ;
			}
		}
	return -1;
	}
	
	
void write_at ( cairo_t * cr , int x , int y , const char * text ) {
// Escreve na tela

	cairo_move_to ( cr , x , y ) ;
	cairo_show_text ( cr , text ) ;
	}


static gboolean timeout ( PanelData * plotdata ) {
// Trata o evento "timeout"

	// Atualiza os dados
	UpdateP ( plotdata ) ;
	return true ;
	}
	
	
static gboolean stimeout ( PanelData * plotdata ) {
// Trata o evento "short timeout"

	UpdateS ( plotdata ) ;
	return true ;
	}
	
	
static gboolean ltimeout ( PanelData * plotdata ) {
// Trata o evento "long timeout"

	UpdateL ( plotdata ) ;
	return true ;
	}

	
void parsecmd (int argc , char * argv [] ) {	
	int vval = 0 , sval = 0 ;
	char * zval = NULL , * tval = NULL , * ppos ;
	bool zh = true , zd = true , te = false , tf = false , nval = false , mval = false , bval = false ;
	char * nlist [ NUMVARS ] , * mlist [ NUMVARS ] ;
	int res ;
	opterr = 0 ;
	while ( -1 != ( res = getopt ( argc , argv , "v:s:n:m:z:b:t:" ) ) ) {
		switch ( res ) {
		case 'b' :
			bval = true ;
			break ;
		case 'v' :
			vval = torange ( atoi ( optarg ) , 0 , 3 ) ;
			break ;
		case 's' :
			sval = torange ( atoi ( optarg ) , 0 , 3 ) ;
			break ;
		case 't' :
			tval = optarg ;
			te = ! strcmp ( tval , "e" ) ;
			tf = ! strcmp ( tval , "f" ) ;
			break ;
		case 'n' :
			for ( int index = 0 ; index < NUMVARS ; ++ index ) {
				nlist [ index ] = new char [ MAX_STRSIZ ] ;
				}
			unlist ( optarg , nlist ) ;
			nval = true ;
			break ;
		case 'm' :
			for ( int index = 0 ; index < NUMVARS ; ++ index ) {
				mlist [ index ] = new char [ MAX_STRSIZ ] ;
				}
 			unlist ( optarg , mlist ) ;
			mval = true ;
			break ;
		case 'z' :
			zval = optarg ;
			zh = strcmp ( zval , "h" ) ;
			zd = strcmp ( zval , "d" ) ;
			break ;
		case '?' :
			ppos = strchr ( "vstnmzb" , optopt ) ;
			if ( ppos != NULL ) {
				cerr << "Op��o '" << optopt << "' requer um argumento.\n" ;
				}
			else {
				if ( isprint ( optopt ) ) {
					cerr << "Op��o inv�lida: '" << optopt << "'.\n" ;
					}
				else {
					cerr << "Caracter inv�lido.\n" ;
					}
				}
			break ;
			}
		}

	log_( 2 , cerr << "s = " << sval << " , v = " << vval << " , b = " << bval ; ) ;
	log_( 2 , cerr << ( ( te || tf ) ? " , t = " : " , " ) << ( tf ? "f, " : "" ) ; ) ;
	log_( 2 , cerr << ( te ? "e, " : "" ) << ( zh ? "zh, " : "" ) << ( zd ? "zd, " : "" ) ; ) ;
	if ( nval ) {
		log_( 2 , cerr << "n = " ; ) ;
		for ( int index = 0 ; index < NUMVARS ; ++ index ) {
			log_( 2 , cerr << nlist [ index ] << "|" ; ) ;
			}
		log_( 2 , cerr << " " ; ) ;
		}
	if ( mval ) {
		log_( 2 , cerr << "m = " ; ) ;
		for ( int index = 0 ; index < NUMVARS ; ++ index ) {
			log_( 2 , cerr << mlist [ index ] << "|" ; ) ;
			}
		log_( 2 , cerr << "\n" ; ) ;
		}

	for ( int index = optind ; index < argc ; ++ index ) {
		log_( 2 , cerr << "Non-option argument " << argv [ index ] << ".\n" ; ) ;
		}
		

	Gdata . zd = zd ;
	Gdata . zh = zh ;
	Gdata . serverno = sval ;
	Gdata . verbose = vval ;
	Gdata . brazil = bval ;
	Gdata . nick = mval ;
	if ( nval && mval && ( sval == 3 ) ) {
		Gdata . server . name = ( te ? EXCEL_SERVER_NAME : FIX32_SERVER_NAME ) ;
		Gdata . server . type = ( te ? EXCEL_SERVER_TYPE : FIX32_SERVER_TYPE ) ;
		Gdata . server . app = ( te ? EXCEL_APP : FIX32_APP ) ;
		Gdata . server . topic = ( te ? EXCEL_TOPIC : FIX32_TOPIC ) ;
		for ( int index = 0 ; index < NUMVARS ; ++ index ) {
			Gdata . server . tag [ index ]	= nlist [ index ] ;
			Gdata . server . factor [ index ] = SCALE_FACTOR ;
			Gdata . server . nick [ index ] = mlist [ index ] ;
			}
		Pserver = & Gdata . server ;
		}
	else {
		Pserver = & Gserver [ sval ] ;		
		}
		
	return ;
	}

	
float torange ( float val , float min , float max ) {
	float aux = min_( val , max ) ;
	return max_( aux , min ) ;
	}

	
void unlist ( char * list , char * array [] ) {
	char * ppos , * pbegin = list ;
	int index = 0 ;
	while ( NULL != ( ppos = strchr ( pbegin , ',' ) ) ) {
		strncpy ( array [ index ] , pbegin , ppos - pbegin ) ;
		if ( ++ index >= NUMVARS ) {
			return ;
			}
		pbegin = ppos + 1 ;
		}
	strcpy ( array [ index ] , pbegin ) ;
	}
