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

using namespace std;
#include "mandelplot.h"
#include "menubar.h"


#define difftime_( a , b )			( ( a ) - ( b ) )			// a função difftime tem um bug


// Dados globais
// Esses dados precisam estar disponíveis para as funções de callback
GlobalData Gdata ;
ServerData * Pserver = NULL ;
ServerData Gserver [ NUMSERVERS ] = {
	// Servidor de produção
	{ FIX32_SERVER_NAME , FIX32_SERVER_TYPE , NULL , FIX32_APP , FIX32_TOPIC ,
		{ VAR1 , VAR2 , VAR3 , VAR4 , VAR5 , VAR6 , VAR7 , VAR8 } ,
		{ 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 } , false } ,
	// Servidores de teste
	// ... planilha Excel
	{ EXCEL_SERVER_NAME , EXCEL_SERVER_TYPE , NULL , EXCEL_APP , EXCEL_TOPIC , 
		{ "r1c1" , "r2c1" , "r3c1" , "r4c1" , "r5c1" , "r6c1" , "r7c1" , "r8c1" } ,
		{ 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 } , false } ,	
	// ... Fix32 Sample System
	{ FIX32_SERVER_NAME , FIX32_SERVER_TYPE , NULL , FIX32_APP , FIX32_TOPIC ,
		{ "DEMO.RAMP.F_CV" , "DEMO.TEMP.F_CV.F_CV" , "DEMO.PRESSURE.F_CV" , "DEMO.VOLUME.F_CV" ,
		"DEMO.LEVEL.F_CV.F_CV" , "DEMO.PV1.F_CV" , "DEMO.PV2.F_CV" , "DEMO.RATE.F_CV" } ,
		{ 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 , 1.0/3600 } , false } ,
	} ;
	

	
// Funções
int main ( int argc , char * argv [ ] ) {
// Syntaxe:
//   rdDDE {-s server_no} {-v verbose_level:0} {-zh} {-zd}
//   rdDDE -s3 -t server_type -n tagname1,tagname2 ... tagname9 -m multiplier,multiplier2 ... multiplier9 {-v verbose_level:0} {-zh} {-zd}
// server_no: 0 a 3, default 0
// verbose_level: 0 a 3, default 0
// -zh, -zd Zerar os valores nas fronteiras de hora e dia
// server_type: Excel ou Fix32
// tagname1 a tagname9: tags 
// Exemplos:
//   rdDDE
//     lê do servidor de produção com todas as configurações default
//   rdDDE -s0
//     lê do servidor de produção com todas as configurações default
//   rdDDE -s1
//     lê do servidor de testes 1 com todas as configurações default
//   rdDDE -s3 -t Fix32 -n 
//     lê do servidor de testes 3, que é um Fix32, as tags 

	// Processa a linha de comando
	parsecmd ( argc , argv ) ;

	// Inicializa as variáveis globais
	Gdata . client = NULL ;
	Gdata . Oldfn = NULL ;
	Gdata . posr = Gdata . posw = Gdata . curvar = 0 ;           
	WindowData original ;
	original . first = true ;
	original . gdk_window = NULL ;
	original . window = NULL ;
	original. hwnd = NULL ;
	int wwidth = 500 , wheigth = 500 ;
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
	
	// Lê os últimos dados salvos
	readData ( & pdata ) ;

	// Primeira atualização dos dados
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
// Define parâmetros da janela

	gtk_window_set_title ( GTK_WINDOW ( window ) , title ) ;
	gtk_window_set_default_size ( GTK_WINDOW ( window ) , width , height ) ;
	gtk_window_set_position ( GTK_WINDOW ( window ) , position ) ;
	gtk_window_set_icon ( GTK_WINDOW ( window ) , create_pixbuf ( filename ) ) ;
	}


GtkWidget * defframe ( GtkWidget * window , int type , const char * caption , int border , int hpos , int vpos , int width , int height ) {
// Define características de um frame

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
	GtkWidget * plotframe = defframe ( mainframe , GTK_SHADOW_IN , "Results" , 0 , 0 , MENUHEIGHT , width , height ) ;
	// Creates menu bar
	if ( defmenubar ( window , mainframe , plotdata ) == NULL ) {
		return ;
		}
	// Defines areas
	// plot area
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
// Lê os últimos dados gravados
	
	// Tenta ler do arquivo de dados horários e também do arquivo de dados diários
	char data [2] [ NUMVARS ] [ MAX_STRSIZ ] ;
	bool rfail1 , rfail2 ;
	int ttime [2] [6] ;
	sprintf ( plotdata -> hfile , HDATAFILE , 2015 ) ;
	sprintf ( plotdata -> mfile , MDATAFILE , 4 ) ;	
	FILE * fph = openDatafile ( plotdata -> hfile , ( void * ) data [0] , & rfail1 , ( void * ) ttime ) ;
	plotdata -> hdfile = fph ;
	FILE * fpm = openDatafile ( plotdata -> mfile , ( void * ) data [1] , & rfail2 , ( void * ) ttime ) ;
	plotdata -> mdfile = fpm ;
/*
	// Copia os dados lidos para as variáveis globais
	int ind = -1 ;
	if ( ! rfail2 ) {
		for ( int var = 0 ; var < NUMVARS ; ++ var ) {
			plotdata -> total [ var ] . hora <- atof ( data [ ind ] [ var ] ) ;
			}
		ind = 1 ;
		}
	if ( ! rfail1 ) {
		for ( int var = 0 ; var < NUMVARS ; ++ var ) {
			plotdata -> total [ var ] . dia <- atof ( data [ ind ] [ var ] ) ;
			}
		ind = 0 ;
		}
	if ( ind >= 0 ) {
		struct tm tread ;
		tread . tm_year = ttime [ ind ] [0] ;
		tread . tm_mon = ttime [ ind ] [1] ;		
		tread . tm_mday = ttime [ ind ] [2] ;
		tread . tm_hour = ttime [ ind ] [3] ;
		tread . tm_min = ttime [ ind ] [4] ;		
		tread . tm_sec = ttime [ ind ] [5] ;
		Gdata . tlast = mktime ( & tread ) ;
		}
*/
	}
		

FILE * openDatafile ( char * datafile , void * data , bool * rfail , void * dtime ) {
// Obtém os dados em um arquivo
	
	* rfail = true ;
	FILE * fp = NULL ;
	if ( ! fexists ( datafile ) ) {
		cerr << "Erro ao ler o arquivo de dados (" << datafile << ")\n" ;
		}
/*
		int res = fgetlast ( datafile , data , dtime ) ;
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
	if ( rfail ) {
		fp = fopen ( datafile , "w" ) ;
		if ( fp == NULL ) {
			cerr << "Erro ao tentar apagar o arquivo de dados (" << datafile << ")\n" ;
			}
		else {
			cerr << "Aberto novo arquivo de dados (" << datafile << ")\n" ;
			}
		}
	else {
*/
	fp = fopen ( datafile , "a" ) ;
	if ( fp == NULL ) {
		cerr << "Erro ao tentar gravar o arquivo de dados (" << datafile << ")\n" ;
		}
	return fp ;
	}

	
// Não usada nesta revisão
GetlastRes fgetlast ( char * fname , void * pdata , void * dtime ) {
// Obtém o último registro do arquivo

	FILE * fp = fopen ( fname , "a+" ) ;
	if ( fp == NULL ) {
		return FGETLAST_ERROPEN ;
		}
	int res = fseek ( fp , - DREC_SIZE , SEEK_END ) ;
	if ( res != 0 ) {
		fclose ( fp ) ;
		return FGETLAST_ERRSEEK ;
		}
	int * ptime = ( int * ) dtime ;
	char * data = ( char * ) pdata ;
	res = fscanf ( fp , RECFMTRD , 
			data , data + MAX_STRSIZ , data + 2 * MAX_STRSIZ , data + 3 * MAX_STRSIZ , data + 4 * MAX_STRSIZ ,
			data + 5 * MAX_STRSIZ , data + 6 * MAX_STRSIZ , data + 7 * MAX_STRSIZ , data + 8 * MAX_STRSIZ ) ;
	if ( res != NUMVARS ) {
		fclose ( fp ) ;
		return FGETLAST_ERRREAD ;
		}
	fclose ( fp ) ;
	return FGETLAST_OK ;
	}

	
void InitP ( PanelData * plotdata ) {
// Primeira atualização
	sprintf ( plotdata -> res , "" ) ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		plotdata -> total [ var ] . dia = plotdata -> total [ var ] . mes =
			plotdata -> total [ var ] . hora = plotdata -> total [ var ] . last = 0 ;
		plotdata -> total [ var ] . lastval = -1 ;
		Gdata . torequest [ var ] = true ;
		Gdata . tounlisten [ var ] = false ;
		Gdata . tolisten [ var ] = true ;
		sprintf ( plotdata -> total [ var ] . tag , "tag%d" , var ) ;
		}
	// EnumWindows( (WNDENUMPROC) & lpfn, 0);
	}


void InitDDE ( PanelData * plotdata ) {
// Gerencia a conexão com o servidor DDE

	// Estabelece conexão via DDE
	if ( Gdata . client == NULL ) {
		return ;
		}
	if ( Pserver -> hwnd == NULL ) {
		sprintf ( plotdata -> res , "Conectando com o servidor..." ) ;
		connectDDE ( plotdata , Pserver ) ;
		return ;
		}
	// Lê os dados diretamente
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
		sprintf ( ptot -> chora , "%8.2f" , ptot -> hora ) ;
		sprintf ( ptot -> cdia , "%8.2f" , ptot -> dia ) ;
		sprintf ( ptot -> cmes , "%8.2f" , ptot -> mes ) ;
		float lastval = ptot -> lastval ;
		if ( lastval < 0 ) {
			sprintf ( ptot -> clastval , " - " ) ;
			sprintf ( ptot -> ctlastr , " - " ) ;
			}
		else {
			struct tm * ptime = localtime ( & ptot -> tlastr ) ;
			sprintf ( ptot -> clastval , "%4.2f" , ptot -> lastval ) ;
			sprintf ( ptot -> ctlastr , "%02d/%02d/%04d %02d:%02d:%02d" ,
				ptime -> tm_mday , ptime -> tm_mon , 1900 + ptime -> tm_year , 
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
	// A ordem do processamento é importante, uma vez que alguns dados são zerados
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

	// Verifica se já passou muito tempo desde a última leitura de dados
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		time_t lastt = plotdata -> total [ var ] . tlastr ;
		time_t tnow ;
		time ( & tnow ) ;
		double interval = difftime_( tnow , lastt ) ;
		Gdata . tounlisten [ var ] = ( interval > REQUEST_INTERVAL ) ;
		}
	}


void UpdateS ( PanelData * plotdata ) {
// Atualiza os dados
// Para ser usado em intervalos curtos

	// Trata a conexão com o servidor
	InitDDE ( plotdata ) ;
	// Atualiza a tela
	gtk_widget_queue_draw_area ( plotdata -> draw [ 0 ] , 0 , 0 , ( plotdata -> area_width ) [ 0 ] ,
		plotdata -> original -> height ) ;

	}

	
#define acum_( var ) 	( plotdata -> total [ var ] . mes )
#define acud_( var ) 	( plotdata -> total [ var ] . dia )
#define acuh_( var ) 	( plotdata -> total [ var ] . hora )

void changeh ( PanelData * plotdata , char * ctime ) {
// Trata a mudança da hora

	// Grava registro horário
	FILE * fp = plotdata -> hdfile ;
	if ( fp == NULL ) {
		cerr << "Arquivo de dados horarios nao disponivel.\n" ;
		return ;
		}
	fprintf ( fp , RECFMTWRT , ctime , acuh_(0) , acuh_(1) , acuh_(2) ,
		acuh_(3) , acuh_(4) , acuh_(5) , acuh_(6) , acuh_(7) , acuh_(8) ) ;
	fflush ( fp ) ;
/*
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		acuh_( var ) = 0 ;
		}
	cerr << "Dados horarios zerados. " ;
*/
	cerr << "Gravou registro horario.\n" ;
	}
	

void changed ( PanelData * plotdata , char * ctime ) {
// Trata a mudança do dia

	FILE * fp = plotdata -> mdfile ;
	if ( fp == NULL ) {
		cerr << "Arquivo de dados diarios nao disponivel.\n" ;
		return ;
		}
	fprintf ( fp , RECFMTWRT , ctime , acud_(0) , acud_(1) , acud_(2) ,
		acud_(3) , acud_(4) , acud_(5) , acud_(6) , acud_(7) , acud_(8) ) ;
	fflush ( fp ) ;
/*
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		acud_( var ) = 0 ;
		}
	cerr << "Dados diarios zerados. " ;
*/
	cerr << "Gravou registro diario.\n" ;
	}


void changem ( PanelData * plotdata , char * ctime ) {
// Trata a mudança do mês

	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		acum_( var ) = 0 ;
		}
	cerr << "Dados mensais zerados.\n" ;
	}
	
#undef acum_
#undef acud_
#undef acuh_

	
void checktfront ( bool * fronth , bool * frontd , bool * frontm , char * ctime ) {
// Verifica se passou alguma fronteira de tempo

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
	if ( timelast -> tm_year < 115 ) {
		* fronth = * frontd = * frontm = false ;
		return ;
		}
	int hlast = timelast -> tm_hour ;
	int mlast = timelast -> tm_mon ;
	int dlast = timelast -> tm_mday ;
	* fronth = ( hlast != hnow ) ;
	* frontm = ( mlast != mnow ) ;
	* frontd = ( dlast != dnow ) ;
	if ( fronth || frontd ) {
		sprintf ( ctime , RECFMTTIM , ynow + 1900 , mnow , dnow , hnow , inow , snow ) ;
		}
	}

	
long connectDDE ( PanelData * plotdata , ServerData * pserver ) {
// Conecta-se ao servidor DDE

	// Envia broadcast com os dados de aplicação e tópico
	ATOM aapp = GlobalAddAtom ( pserver -> app) ;
	ATOM atopic = GlobalAddAtom ( pserver -> topic ) ;
	long res = SendMessage ( (HWND) -1 , WM_DDE_INITIATE , (WPARAM) Gdata . client , MAKELPARAM ( aapp , atopic ) ) ;
	cerr << "Enviou DDE_INITIATE " << pserver -> app << "|" << pserver -> topic << " res = " << res ;
	GlobalDeleteAtom (aapp) ;
	GlobalDeleteAtom (atopic) ;	
	cerr << " from " << Gdata . client << " to " << pserver -> hwnd << "\n" ;
	return res ;
	}

	
long listenDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
// Registra-se para receber atualizações do servidor DDE

	GLOBALHANDLE hOptions = GlobalAlloc ( GMEM_MOVEABLE , sizeof(DDEADVISE) ) ;
	if ( hOptions == NULL ) {
		cerr << "Erro na alocação de memória\n" ;
		return -1;
		}
	DDEADVISE * lpOptions = (DDEADVISE FAR*) GlobalLock ( hOptions ) ; 
	if ( lpOptions == NULL ) {
		cerr << "Erro depois da alocação de memória\n" ;
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
	cerr << "Enviou DDE_ADVISE " << pserver -> tag [ var ] << " res = " << res << "\n" ;
	GlobalFree ( hOptions ) ; 
	GlobalDeleteAtom ( atag ) ;
	return res ;
	}

	
long requestDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
// Lê dados do servidor DDE

	ATOM atag = GlobalAddAtom ( pserver -> tag [ var ] ) ;
	long res = PostMessage ( pserver -> hwnd , WM_DDE_REQUEST , ( WPARAM ) Gdata . client ,
		PackDDElParam ( WM_DDE_REQUEST , CF_TEXT , atag ) ) ;
	cerr << "Enviou DDE_REQUEST " << pserver -> tag [ var ] << " res = " << res << "\n" ;
	GlobalDeleteAtom ( atag ) ;
	return res ;
	}

	
long unlistenDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
// Avisa ao servidor DDE para não mais enviar dados por exceção

	ATOM atag = GlobalAddAtom ( pserver -> tag [ var ] ) ;
	long res = PostMessage ( pserver -> hwnd , WM_DDE_UNADVISE , ( WPARAM ) Gdata . client ,
		PackDDElParam ( WM_DDE_UNADVISE , 0 , atag ) ) ;
	cerr << "Enviou DDE_UNADVISE " << pserver -> tag [ var ] << " res = " << res << "\n" ;
	GlobalDeleteAtom ( atag ) ;
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

	cerr << "ACK W = " << wparam << ", L = " << lparam ;		
	UINT low , high ;
	UnpackDDElParam ( WM_DDE_DATA , lparam , & low, & high ) ;
	FreeDDElParam ( WM_DDE_DATA , lparam ) ;
	if ( Pserver -> hwnd == NULL ) {
		// ACK para o WM_DDE_INITIATE (conexão inicial)
		Pserver -> hwnd = (HWND) wparam ;
		cerr << "\n" ;
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
	cerr << " " << msg << " high = " << high << ", low = " << low << "\n" ;
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
	cerr << "DATA W = " << wparam << ", L = " << lparam ;
	UINT low , high ;
	UnpackDDElParam ( WM_DDE_DATA , lparam , & low, & high ) ;
	FreeDDElParam (WM_DDE_DATA , lparam ) ;
	ATOM aitem = ( ATOM ) high ;
	GLOBALHANDLE hdata = ( GLOBALHANDLE ) low ;
	DDEDATA FAR* pdata = ( DDEDATA FAR* ) GlobalLock ( hdata ) ;
	cerr << " = " << high << ", " << low ;
	char item [ MAX_STRSIZ ] ;
	// Se os dados recebidos estiverem no formato esperado, grava-os no buffer
	if ( pdata -> cfFormat == CF_TEXT ) {
		GlobalGetAtomName ( aitem , item , sizeof (item) ) ;
		char val [ MAX_STRSIZ ] ;
		strcpy ( val , ( const char * ) pdata -> Value ) ;
		char * nl = strchr ( val , '\r' ) ;
		if ( nl != NULL ) {
			* nl = '\0' ;
			}
		cerr << " : " << item << ", " << val << " @" << Gdata . posr ;
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
	
	
void sendACK ( HWND server , ATOM item , bool ack ) {
// Envia ACK ao servidor
	PostMessage ( server , WM_DDE_ACK, (WPARAM) Gdata . client , PackDDElParam ( WM_DDE_ACK , DDE_ACK , item ) ) ;
	cerr << "\t sent " << ( ack ? "ACK" : "NACK" ) << "\n" ;
	}
	
	
void readDDE ( char * var , char * value , time_t tstamp ) {
// Armazena os dados recebidos do servidor DDE
	int nvar = findvar ( Pserver -> tag , var ) ;
	if ( nvar < 0 ) {
		nvar = Gdata . curvar ;
		}
	cerr << " var " << nvar << "\n" ; 
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

	
// Não usada nesta revisão	
BOOL CALLBACK lpfn ( HWND hWnd , int lParam ) {
// Enumera todas as janelas em busca do servidor DDE
// Pode ser usada para descobrir o título de uma janela qualquer
	char title [99] ;
	int size ;
	size = GetWindowText ( hWnd , title , 100 ) ;
	const char * name = Pserver -> name ;
	if ( size > 0 ) {
		if ( strcmp ( name , title ) == 0 ) {
			Pserver -> hwnd = hWnd ;
			cerr << title << ": " << hWnd << "\n" ;
			}
		// cerr << title << ": " << hWnd << "\n" ;
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
// Processa os dados que estão no buffer

	int wpos = Gdata . posw ;
	int rpos = Gdata . posr ;
	ReadData * buf = Gdata . buffer ;
	SumData * ptot = plotdata -> total ;
	float value ;
	int i = wpos ;
	while ( i != rpos ) {
		// Obtém um valor lido
		if ( buf [ i ] . quality == QUALITY_GOOD ) {
			int var = buf [ i ] . var ;
			if ( var >= 0 ) {
				char * val = buf [ i ] . value ;
				value = atof ( val ) ;
				cerr << "reg. " << i << ", " ;
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
	// Obtém o último valor processsado
	float last = ptot [ var ] . lastval ;
	time_t lastt = ptot [ var ] . tlastc ;
	float area = 0 ;
	cerr << "var " << var << ": " ;
	if ( last < 0 ) {
		cerr << "valor " << value << " desprezado. Timestamps = " << lastt << " e " << vtime << "\n" ;
		}
	else {
		// integra
		int interval = difftime_( vtime , lastt ) ;
		area = last * interval * pserver -> factor [ var ];
		cerr << last << " * " << interval << " -> " << area << "\n" ;
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
// Define um botão

	GtkWidget * button = gtk_button_new_with_label ( caption ) ;
	gtk_widget_set_size_request ( button , BUTTONWIDTH , BUTTONHEIGHT ) ;
	gtk_fixed_put ( GTK_FIXED ( window ) , button , hpos , vpos ) ;	
	return button ;
	}
	
	
void act_quit ( GtkAction * action , gpointer user_data ) {
// Trata o evento "opção Quit selecionada"

	gtk_main_quit ( ) ;
	return ;
	}

	
void act_exportas ( GtkAction * action , gpointer user_data ) {
	}

	
void act_about ( GtkAction * action , gpointer user_data ) {
	}
	
	
GdkPixbuf * create_pixbuf ( const gchar * filename ) {
// Obtém bitmap de um arquivo
// A conversão de formato é feita automaticamente

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
// Trata o evento "botão do mouse pressionado"

	// Atualiza os dados e a tela imediatamente
	PanelData * plotdata = ( PanelData * ) user_data ;
	changed ( plotdata , ( char * ) "2015-4-26\n11:21:00" ) ;
	return false ;
	}

	
void update_txt ( cairo_t * cr , PanelData * plotdata ) {
// Atualiza os dados mostrados na tela

	cairo_set_source_rgb ( cr , 0.1 , 0.1 , 0.1 ) ; 
	cairo_select_font_face ( cr , "Purisa" , CAIRO_FONT_SLANT_NORMAL , CAIRO_FONT_WEIGHT_BOLD ) ;
	cairo_set_font_size ( cr , 13 ) ;
	write_at ( cr , 30 , 30 , (char *) "Tag" ) ;
	write_at ( cr , 90 , 30 , (char *) "Hora" ) ;
	write_at ( cr , 150 , 30 , (char *) "Dia" ) ;
	write_at ( cr , 210 , 30 , (char *) "Mes" ) ;
	write_at ( cr , 270 , 30 , (char *) "Ultima" ) ;
	SumData * result = plotdata -> total ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		write_at ( cr , 30 , 30 * ( var + 2 ) , result [ var ] . tag ) ;  
		write_at ( cr , 90 , 30 * ( var + 2 ) , result [ var ] . chora ) ;  
		write_at ( cr , 150 , 30 * ( var + 2 ) , result [ var ] . cdia ) ;  
		write_at ( cr , 210 , 30 * ( var + 2 ) , result [ var ] . cmes ) ;
		write_at ( cr , 270 , 30 * ( var + 2 ) , result [ var ] . clastval ) ;
		write_at ( cr , 330 , 30 * ( var + 2 ) , result [ var ] . ctlastr ) ;
		}
	write_at ( cr , 30 , 350 , plotdata -> res ) ; 
	}

	
gboolean draw ( GtkWidget * widget , cairo_t * cr , gpointer user_data ) {
// Trata o evento "draw"
  
	PanelData * plotdata = ( PanelData * ) user_data ;
	update_txt ( cr , plotdata ) ;

	if ( plotdata -> original -> first ) {
		cerr << "gtk_window: " << plotdata -> original -> window << "\n" ;
		GdkWindow * gdk_window = gtk_widget_get_window ( plotdata -> original -> window ) ;
		cerr << "gdk_window: " << gdk_window << "\n" ;
		if ( gdk_window != NULL ) {
			gboolean b = gdk_win32_window_is_win32 ( gdk_window ) ;
			cerr << "is win32: " << b << "\n" ;
			HWND hwnd = ( HWND ) gdk_win32_window_get_handle ( gdk_window ) ;
			cerr << "HWND: " << hwnd << "\n" ;
			plotdata -> original -> hwnd = hwnd ;
			Gdata . client = hwnd ;
			Gdata . Oldfn = (WNDPROC) GetWindowLong ( hwnd , GWL_WNDPROC ) ;
			cerr << "Old proc: " << Gdata . Oldfn << "\n" ;
			SetWindowLong ( hwnd , GWL_WNDPROC , (LONG) WndProc ) ;
			cerr << "New proc: " << WndProc << "\n" ;
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
// Encontra uma variável pelo nome

	for ( int i = 0 ; i < NUMVARS ; ++ i ) {
		if ( strcmp ( list [i] , name ) == 0 ) {
			return i ;
			}
		}
	return -1;
	}
	
	
void write_at ( cairo_t * cr , int x , int y , char * text ) {
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
	bool zh = false , zd = false , te = false , tf = false , nval = false , fval = false ;
	char * nlist [ NUMVARS ] , * mlist [ NUMVARS ] ;
	float flist [ NUMVARS ] ; 
	int res ;
	opterr = 0 ;
	while ( -1 != ( res = getopt ( argc , argv , "v:s:n:m:z:t:" ) ) ) {
		switch ( res ) {
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
			for ( int index = 0 ; index < NUMVARS ; ++ index ) {
				flist [ index ] = atof ( mlist [ index ] ) ;
				}
			for ( int index = 0 ; index < NUMVARS ; ++ index ) {
				free ( mlist [ index ] ) ;
				}			
			fval = true ;
			break ;
		case 'z' :
			zval = optarg ;
			zh = ! strcmp ( zval , "h" ) ;
			zd = ! strcmp ( zval , "h" ) ;
			break ;
		case '?' :
			ppos = strchr ( "vstnmz" , optopt ) ;
			if ( ppos != NULL ) {
				cerr << "Opção '" << optopt << "' requer um argumento.\n" ;
				}
			else {
				if ( isprint ( optopt ) ) {
					cerr << "Opção inválida: '" << optopt << "'.\n" ;
					}
				else {
					cerr << "Caracter inválido.\n" ;
					}
				}
			break ;
			}
		}

	cerr << "s = " << sval << " , v = " << vval ;
	cerr << ( ( te || tf ) ? " , t = " : " , " ) << ( tf ? "f, " : "" ) ;
	cerr << ( te ? "e, " : "" ) << ( zh ? "zh, " : "" ) << ( zd ? "zd, " : "" ) ;
	if ( nval ) {
		cerr << "n = " ;
		for ( int index = 0 ; index < NUMVARS ; ++ index ) {
			cerr << nlist [ index ] << ";" ;
			}
		cerr << " " ;
		}
	if ( fval ) {
		cerr << "m = " ;
		for ( int index = 0 ; index < NUMVARS ; ++ index ) {
			cerr << flist [ index ] << ";" ;
			}
		cerr << " " ;
		}

	for ( int index = optind ; index < argc ; ++ index ) {
		cerr << "Non-option argument " << argv [ index ] << ".\n" ;
		}
		

	Gdata . zd = zd ;
	Gdata . zh = zh ;
	Gdata . serverno = sval ;
	Gdata . verbose = vval ;
	if ( nval && fval && ( sval == 3 ) ) {
		Gdata . server . name = ( te ? EXCEL_SERVER_NAME : FIX32_SERVER_NAME ) ;
		Gdata . server . type = ( te ? EXCEL_SERVER_TYPE : FIX32_SERVER_TYPE ) ;
		Gdata . server . app = ( te ? EXCEL_APP : FIX32_APP ) ;
		Gdata . server . topic = ( te ? EXCEL_TOPIC : FIX32_TOPIC ) ;
		for ( int index = 0 ; index < NUMVARS ; ++ index ) {
			Gdata . server . tag [ index ]	= nlist [ index ] ;
			Gdata . server . factor [ index ] = flist [ index ] ;
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
	while ( true ) {
		ppos = strchr ( pbegin , ',' ) ;
		if ( ppos == NULL ) {
			strcpy ( array [ index ] , pbegin ) ;
			return ;
			}
		strncpy ( array [ index ] , pbegin , ppos - pbegin ) ;
		array [ index ] [ ppos - pbegin ] = '\0' ;
		if ( ++ index >= NUMVARS ) {
			return ;
			}
		pbegin = ppos + 1 ;
		}
	}
