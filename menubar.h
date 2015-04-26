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
"  		</menubar>"
"</ui>" ;
// normal entries
static GtkActionEntry entries [ ] = {
	// main submenus
	{ "FileMenu"  , NULL           , "_File" } ,
	// File menu options
	{ "Quit"      , GTK_STOCK_QUIT , "_Quit"         , "<control>Q" , "Quit application" , G_CALLBACK ( act_quit ) } ,
	} ;
static guint n_entries = G_N_ELEMENTS ( entries ) ;
// toggle entries
