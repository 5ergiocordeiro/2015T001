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

using namespace std;
#include "mandelplot.h"
#include "menubar.h"


GlobalData Gdata ;

ServerData Gserver[NUMSERVERS] = {
	// production environment
	{ FIX32_SERVER_NAME , FIX32_SERVER_TYPE , NULL , FIX32_APP , FIX32_TOPIC ,
		{ "DEMO.RAMP.F_CV" , "DEMO.TEMP.F_CV.F_CV" , "DEMO.PRESSURE.F_CV" } , false } ,
	// development environment
	{ TSERVER1_SERVER_NAME , EXCEL_SERVER_TYPE , NULL , EXCEL_APP , TSERVER1_TOPIC , 
		{ "r1c1" , "r2c1" , "r3c1" , "r4c1" , "r5c1" , "r6c1" , "r7c1" , "r8c1" , "r9c1" } , false } ,
	// test environment
	{ TSERVER2_SERVER_NAME , FIX32_SERVER_TYPE , NULL , FIX32_APP , TSERVER2_TOPIC ,
		{ "DEMO.RAMP.F_CV" , "DEMO.TEMP.F_CV.F_CV" , "DEMO.PRESSURE.F_CV" } , false } ,
	} ;
	
ServerData * Pserver = NULL ;
	

// Functions
int main ( int argc , char * argv [ ] ) {
// Syntax:
// progname
// Example:
// progname

	// Creates shared data struct
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
	pdata . first = true ;
	pdata . original = & original ;
	Pserver = & Gserver [ TSERVER1 ];
	
	// Initializes the GTK+ ttolkit
	int dumbi = 1 ;
	gtk_init ( & dumbi , NULL ) ;
	
	// Creates window
	GtkWidget * window = gtk_window_new ( GTK_WINDOW_TOPLEVEL ) ;
	original . window = window ;
	gtkinit ( window , "Totalizacao de vazao" , GTK_WIN_POS_NONE , wwidth , wheigth , "techsteel.ico" , & pdata ) ;

	// Updates data
	UpdateP ( & pdata ) ;

	// Shows window
	// From this point on, control is driven by events and its handlers
	gtk_widget_show_all ( window ) ;
	gtk_main ( )  ;
 
	return 0;
	}

	
void defwindow ( GtkWidget * window , const char * title , GtkWindowPosition position , int width , int height , const gchar * filename ) {
// Defines parameters of the window

	gtk_window_set_title ( GTK_WINDOW ( window ) , title ) ;
	gtk_window_set_default_size ( GTK_WINDOW ( window ) , width , height ) ;
	gtk_window_set_position ( GTK_WINDOW ( window ) , position ) ;
	gtk_window_set_icon ( GTK_WINDOW ( window ) , create_pixbuf ( filename ) ) ;
	return ;
	}


GtkWidget * defframe ( GtkWidget * window , int type , const char * caption , int border , int hpos , int vpos , int width , int height ) {
// Defines characteristics of the frame

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
	GtkWidget * ctrlframe = defframe ( mainframe , GTK_SHADOW_OUT , "Control bar" , 0 , width + SEPARATION , MENUHEIGHT , CTRLBARWIDTH , CTRLBARHEIGHT ) ;
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
	// control bar
	GtkWidget * ctrlbar = defframe ( mainframe , -1 , "" , 0 , width + SEPARATION , MENUHEIGHT , CTRLBARWIDTH , CTRLBARHEIGHT ) ;
	// status bar
	GtkWidget * statusbar = gtk_statusbar_new ( ) ;
	gtk_widget_set_size_request ( statusbar , STATBARWIDTH , STATBARHEIGHT ) ;
	gtk_fixed_put ( GTK_FIXED ( mainframe ) , statusbar , 0 , height + SEPARATION + MENUHEIGHT ) ;
	gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ) , gtk_statusbar_get_context_id ( GTK_STATUSBAR ( statusbar ) , "Initialized." ) , "Initialized." ) ;	
	plotdata -> msg = statusbar ;
	// Defines buttons
	GtkWidget * plotbtn = defbutton ( ctrlbar , "Plot" , SEPARATION , SEPARATION + BUTTONHEIGHT ) ;
	g_signal_connect ( plotbtn , "clicked" , G_CALLBACK ( plot ) , plotdata ) ;
	GtkWidget * zoominbtn = defbutton ( ctrlbar , "Zoom in" , SEPARATION , 2 * ( SEPARATION + BUTTONHEIGHT ) ) ;
	g_signal_connect ( zoominbtn , "clicked" , G_CALLBACK ( zoomin ) , plotdata ) ;
	GtkWidget * zoomoutbtn = defbutton ( ctrlbar , "Zoom out" , SEPARATION , 3 * ( SEPARATION + BUTTONHEIGHT ) ) ;
	g_signal_connect ( zoomoutbtn , "clicked" , G_CALLBACK ( zoomout ) , plotdata ) ;
	return ;
	}


void UpdateP ( PanelData * plotdata ) {
// Plots Mandelbrot set

	int var ;
	sprintf ( plotdata -> res , " " ) ;
	if ( plotdata -> first ) {
		for ( var = 0 ; var < NUMVARS ; ++ var ) {
			plotdata -> total [ var ] . dia = plotdata -> total [ var ] . mes =
				plotdata -> total [ var ] . last = 0 ;
			sprintf ( plotdata -> total [ var ] . tag , "tag%d" , var ) ;
			}
		// EnumWindows( (WNDENUMPROC) & lpfn, 0);
		plotdata -> first = false ;
		}

	if ( ( Gdata . client != NULL ) && ( Pserver -> hwnd == NULL ) ) {
		int res = connectDDE ( plotdata , Pserver ) ;
		}
	var = Gdata . curvar ;
	if ( ( Pserver -> hwnd != NULL ) && ( var < 3 ) ) {
		int res = listenDDE ( plotdata , Pserver , var ) ;
		Pserver -> enabled = true ;
		}
	totalize ( plotdata , Pserver ) ;
	for ( var = 0 ; var < NUMVARS ; ++ var ) {
		sprintf ( plotdata -> total [ var ] . cdia , "%4.2f" , plotdata -> total [ var ] . dia ) ;
		sprintf ( plotdata -> total [ var ] . cmes , "%6.2f" , plotdata -> total [ var ] . mes ) ;
		sprintf ( plotdata -> total [ var ] . clast , "%8.2f" , plotdata -> total [ var ] . last ) ;
		}
	sprintf ( plotdata -> res , "%d %d" , Gdata . posr , Gdata . posw ) ;
	return ;
	}
		
long connectDDE ( PanelData * plotdata , ServerData * pserver ) {
	// initiate: broadcast
	ATOM aapp = GlobalAddAtom ( pserver -> app) ;
	ATOM atopic = GlobalAddAtom ( pserver -> topic ) ;
	long res = SendMessage ( (HWND) -1 , WM_DDE_INITIATE , (WPARAM) Gdata . client , MAKELPARAM ( aapp , atopic ) ) ;
	cerr << "Enviou DDE_INITIATE " << pserver -> app << "|" << pserver -> topic << " res = " << res << "\n" ;
	GlobalDeleteAtom (aapp) ;
	GlobalDeleteAtom (atopic) ;	
	sprintf(plotdata -> res , "%ld %ld %ld" , (long) Gdata . client , (long) pserver -> hwnd , (long) res ) ;
	return res ;
	}

long listenDDE ( PanelData * plotdata , ServerData * pserver , int var ) {
	// advise: to the server that answered the "initiate" message
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
	return 0 ;
	}
	
LRESULT CALLBACK WndProc ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) {
	switch ( imsg ) {
	case WM_DDE_ACK :
		receiveACK ( wnd , imsg , wparam , lparam ) ;
		break ;
	case WM_DDE_DATA :
		receiveDATA ( wnd , imsg , wparam , lparam ) ;
		break ;
		}
	return CallWindowProc ( Gdata . Oldfn , wnd , imsg , wparam , lparam ) ;
	}
		
void receiveACK ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) {
	cerr << "ACK W = " << wparam << ", L = " << lparam ;		
	UINT low , high ;
	UnpackDDElParam ( WM_DDE_DATA , lparam , & low, & high ) ;
	FreeDDElParam ( WM_DDE_DATA , lparam ) ;
	if ( Pserver -> hwnd == NULL ) {
		// ACK to WM_DDE_INITIATE
		Pserver -> hwnd = (HWND) wparam ;
		cerr << "\n" ;
		return ;
		}
	// ACK to WM_DDE_ADVISE
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
		Gdata . curvar ++ ;
		}
	ATOM aitem = ( ATOM ) high ;
	sendACK ( Pserver -> hwnd , aitem , true ) ;
	}

void receiveDATA ( HWND wnd , UINT imsg , WPARAM wparam , LPARAM lparam ) {
	cerr << "DATA W = " << wparam << ", L = " << lparam ;
	UINT low , high ;
	UnpackDDElParam ( WM_DDE_DATA , lparam , & low, & high ) ;
	FreeDDElParam (WM_DDE_DATA , lparam ) ;
	GLOBALHANDLE hdata = ( GLOBALHANDLE ) low ;
	DDEDATA FAR* pdata = ( DDEDATA FAR* ) GlobalLock ( hdata ) ;
	ATOM aitem = ( ATOM ) high ;
	cerr << " = " << high << ", " << low ;
	char item [ MAX_STRSIZ ] ;
	if ( pdata -> cfFormat == CF_TEXT ) {
		GlobalGetAtomName ( aitem , item , sizeof (item) ) ;
		char val [ MAX_STRSIZ ] ;
		strcpy ( val , ( const char * ) pdata -> Value ) ;
		char * nl = strchr ( val , '\r' ) ;
		if ( nl != NULL ) {
			* nl = '\0' ;
			}
		cerr << " : " << item << ", " << val << " @" << Gdata . posr << "\n" ;
		readDDE ( item , val ) ;	
		GlobalDeleteAtom ( aitem ) ;
		if ( pdata -> fAckReq ) {
			sendACK ( Pserver -> hwnd , aitem , true ) ;
			}
		}
	else {
		sendACK ( Pserver -> hwnd , aitem , false ) ;
		}
	bool release = pdata -> fRelease ;
	GlobalUnlock ( hdata ) ;
	if ( release ) {
		GlobalFree ( hdata ) ;
		}
	}
			
void sendACK ( HWND server , ATOM item , bool ack ) {
	PostMessage ( server , WM_DDE_ACK, (WPARAM) Gdata . client , PackDDElParam ( WM_DDE_ACK , DDE_ACK , item ) ) ;
	cerr << "\t sent " << ( ack ? "ACK" : "NACK" ) << "\n" ;
	}
			
void readDDE ( char * var , char * value ) {	
	ReadData * pbuf = & Gdata . buffer [ Gdata . posr ] ;
	strcpy ( pbuf -> value , value ) ;
	strcpy ( pbuf -> var , var ) ;
	pbuf -> quality = QUALITY_GOOD ;
	if ( ++ Gdata . posr >= BUFFSIZE ) {
		Gdata . posr = 0 ;
		}	
	}

	
// Not used in this revision	
BOOL CALLBACK lpfn ( HWND hWnd , int lParam ) {
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

	
void totalize ( PanelData * plotdata , ServerData * pserver ) {
	int wpos = Gdata . posw ;
	int rpos = Gdata . posr ;
	ReadData * buf = Gdata . buffer ;
	float totals [ NUMVARS ] , last [ NUMVARS ] ;
	int var ;

	for ( var = 0 ; var < NUMVARS ; ++ var ) {
		totals [ var ] = plotdata -> total [ var ] . dia ;
		last [ var ] = plotdata -> total [ var ] . last ;
		}
	int i = wpos ;
	while ( i != rpos ) {
		if ( buf [ i ] . quality == QUALITY_GOOD ) {
			var = findvar ( pserver -> tag , buf [ i ] . var ) ;
			if ( var >= 0 ) {
				char * val = buf [ i ] . value ;
				float value = atof ( val ) ;
				cerr << "reg. " << i << ": var " << var << " = " << totals [ var ] << " + " ;
				totals [ var ] = totals [ var ] + value ;
				cerr << value << " -> " << totals [ var ] << "\n" ;
				last [ var ] = value ;
				}
			}
		if ( ++ i >= BUFFSIZE ) {
			i = 0 ;
			}
		}
	Gdata . posw = i ;
	for ( var = 0 ; var < NUMVARS ; ++ var ) {
		plotdata -> total [ var ] . dia = totals [ var ] ;
		plotdata -> total [ var ] . last = last [ var ] ;
		}
	}
	

float timeval_subtract ( struct timeval * result, struct timeval * x, struct timeval * y ) {
/* Perform the carry for the later subtraction by updating y. */

       if (x -> tv_usec < y -> tv_usec) {
         int nsec = (y -> tv_usec - x -> tv_usec) / 1000000 + 1;
         y -> tv_usec -= 1000000 * nsec;
         y -> tv_sec += nsec;
       }
       if (x -> tv_usec - y -> tv_usec > 1000000) {
         int nsec = (x -> tv_usec - y -> tv_usec) / 1000000;
         y -> tv_usec += 1000000 * nsec;
         y -> tv_sec -= nsec;
       }
     
       /* Compute the time remaining to wait.
          tv_usec is certainly positive. */
       result -> tv_sec = x -> tv_sec - y -> tv_sec;
       result -> tv_usec = x -> tv_usec - y -> tv_usec;
     
       /* Return 1 if result is negative. */
       return result -> tv_sec + 1e-6 * result -> tv_usec ;
     }
	

void defplotarea ( GtkWidget * window , int width , int height , int posh , int posv , PanelData * plotdata ) {
// Defines characteristics of the drawing area. It is sliced in some independent parts for improved performance.

	int i ;
	for ( i = 0 ; ( i < MAXDRAWAREA ) && ( width > 0 ) ; ++ i , width -= DRAWAREAWIDTH ) {
		int awidth = Min ( width , DRAWAREAWIDTH ) ;
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
// Defines the characteristics of a button

	GtkWidget * button = gtk_button_new_with_label ( caption ) ;
	gtk_widget_set_size_request ( button , BUTTONWIDTH , BUTTONHEIGHT ) ;
	gtk_fixed_put ( GTK_FIXED ( window ) , button , hpos , vpos ) ;	
	return button ;
	}
	
	
void act_quit ( GtkAction * action , gpointer user_data ) {
// Handles the event "option quit selected"

	gtk_main_quit ( ) ;
	return ;
	}

	
void act_exportas ( GtkAction * action , gpointer user_data ) {
	}

	
void act_about ( GtkAction * action , gpointer user_data ) {
	}
	
	
GdkPixbuf * create_pixbuf ( const gchar * filename ) {
// Uploads bitmap from file. Format conversion is done automatically.

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
// Initializes shared data struct

	// Sets initial values
	plotdata . width = width ;	
	plotdata . height = height ;
	return ;
	}

	
GtkWidget * defmenubar ( GtkWidget * window , GtkWidget * mainframe , PanelData * plotdata ) {
// Defines the menu bar. Most definitions come from the menubar.h file.

    GtkActionGroup * actions = gtk_action_group_new ( "Actions" ) ;
    gtk_action_group_add_actions ( actions , entries , n_entries , plotdata ) ;
    gtk_action_group_add_toggle_actions ( actions , toggle_entries , n_toggle_entries , plotdata ) ;
    gtk_action_group_add_radio_actions ( actions , mouse_sel_entries, n_mouse_sel_entries , MMODE_NONE , G_CALLBACK ( act_mousesel ) , plotdata ) ;

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
// Generates a zoomed-in plot 

	return ;
	}

void zoomout ( const GtkWidget * button , gpointer frame ) {
// Generates a zoomed-out plot

	return ;
	}

gboolean userclkoff ( GtkWidget * widget , GdkEvent * event , gpointer user_data ) {
// Handles the event "mouse button released" event on the drawing areas

	return false ;
	}
		
gboolean userclkon ( GtkWidget * widget , GdkEvent * event , gpointer user_data ) {
// Handles the event "mouse button pressed" event on the drawing areas

	PanelData * plotdata = ( PanelData * ) user_data ;
	UpdateP ( plotdata ) ;
	gtk_widget_queue_draw_area ( plotdata -> draw [ 0 ] , 0 , 0 , ( plotdata -> area_width ) [ 0 ] ,
		plotdata -> original -> height ) ;
	return false ;
	}

void update_txt ( cairo_t * cr , PanelData * plotdata ) {
	cairo_set_source_rgb ( cr , 0.1 , 0.1 , 0.1 ) ; 
	cairo_select_font_face ( cr , "Purisa" , CAIRO_FONT_SLANT_NORMAL , CAIRO_FONT_WEIGHT_BOLD ) ;
	cairo_set_font_size ( cr , 13 ) ;
	write_at ( cr , 30 , 30 , (char *) "Tag" ) ;
	write_at ( cr , 90 , 30 , (char *) "Dia" ) ;
	write_at ( cr , 150 , 30 , (char *) "Mes" ) ;
	write_at ( cr , 210 , 30 , (char *) "Ultima" ) ;
	SumData * result = plotdata -> total ;
	for ( int var = 0 ; var < NUMVARS ; ++ var ) {
		write_at ( cr , 30 , 30 * ( var + 2 ) , result [ var ] . tag ) ;  
		write_at ( cr , 90 , 30 * ( var + 2 ) , result [ var ] . cdia ) ;  
		write_at ( cr , 150 , 30 * ( var + 2 ) , result [ var ] . cmes ) ;
		write_at ( cr , 210 , 30 * ( var + 2 ) , result [ var ] . clast ) ;
		}
	write_at ( cr , 30 , 350 , plotdata -> res ) ; 
	}

gboolean draw ( GtkWidget * widget , cairo_t * cr , gpointer user_data ) {
// handles the "draw" event on the drawing areas.
  
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
// Generates a plot with the new parameters

	return ;
	}

void act_mousesel ( GtkAction * action , GtkRadioAction * current , gpointer user_data ) {
// Handles the event "new selection" in the Plot submenu
	}

void act_tglMP ( GtkAction * action , gpointer user_data  ) {

	return ;
	}

int findvar ( const char * list[] , char * name ) {
	for ( int i = 0 ; i < NUMVARS ; ++ i ) {
		if ( strcmp ( list [i] , name ) == 0 ) {
			return i ;
			}
		}
	return -1;
	}
	
void write_at ( cairo_t * cr , int x , int y , char * text ) {
	cairo_move_to ( cr , x , y ) ;
	cairo_show_text ( cr , text ) ;
	}