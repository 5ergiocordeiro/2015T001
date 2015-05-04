// Menubar definition and creation
// To be included into main program code

	
// Menu definition
// overall structure
static const gchar * ui_info =
"<ui>"
"	<menubar name='MenuBar'>"
"   	<menu action='FileMenu'>"
"      		<menuitem action='Quit'/>"
"    		</menu>"
"   	<menu action='RelatMenu'>"
"      		<menuitem action='Abrir'/>"
"    		</menu>"
"  		</menubar>"
"</ui>" ;
// normal entries
static GtkActionEntry entries [ ] = {
	// main submenus
	{ "FileMenu"  , NULL           , "_Arquivo" } ,
	// File menu options
	{ "Quit"      , GTK_STOCK_QUIT , "_Sair"         , "<control>Q" , "Quit application" , G_CALLBACK ( act_quit ) } ,
	{ "RelatMenu"  , NULL           , "_Relatorio" } ,
	// Relat menu options
	{ "Abrir"      , GTK_STOCK_OPEN , "_Abrir"       , "<control>A" , "Abrir planilha Excel" , G_CALLBACK ( act_openexc ) } ,
	} ;
static guint n_entries = G_N_ELEMENTS ( entries ) ;
// toggle entries
