// Menubar definition and creation
// To be included into main program code

	
// Menu definition
// overall structure
static const gchar * ui_info =
"<ui>"
"	<menubar name='MenuBar'>"
"   	<menu action='FileMenu'>"
" 			<menuitem action='Export'/>"
"      		<separator/>"
"      		<menuitem action='Quit'/>"
"    		</menu>"
"		<menu action='PlotMenu'>"
"     		<menuitem action='None'/>"
"       	<menuitem action='Center'/>"
"       	<menuitem action='Path'/>"
"       	<menuitem action='Region'/>"
"      		</menu>"
"      	<menu action='ViewMenu'>"
"        	<menuitem action='Misiurewicz'/>"
"      		</menu>"
"    	<menu action='HelpMenu'>"
"      		<menuitem action='About'/>"
"    		</menu>"
"  		</menubar>"
"</ui>" ;
// normal entries
static GtkActionEntry entries [ ] = {
	// main submenus
	{ "FileMenu"  , NULL           , "_File" } ,
	{ "PlotMenu"  , NULL           , "_Plot" } ,
	{ "ViewMenu"  , NULL           , "_View" } ,
	{ "HelpMenu"  , NULL           , "_Help" } ,
	// File menu options
	{ "Export"    , GTK_STOCK_SAVE , "_Export as..." , NULL         , "Export to a file" , G_CALLBACK ( act_exportas ) } ,
	{ "Quit"      , GTK_STOCK_QUIT , "_Quit"         , "<control>Q" , "Quit application" , G_CALLBACK ( act_quit ) } ,
	// Plot menu options
	// View menu options
	// Help menu options
	{ "About"	  , NULL           , "_About"        , "<control>A" , "About"            , G_CALLBACK ( act_about ) } ,
	} ;
static guint n_entries = G_N_ELEMENTS ( entries ) ;
// toggle entries
static GtkToggleActionEntry toggle_entries [ ] = {
	{ "Misiurewicz" , NULL , "Show _Misiurewicz points" , "<control>M" , "" , G_CALLBACK ( act_tglMP ) , FALSE } ,
	} ;
static guint n_toggle_entries = G_N_ELEMENTS ( toggle_entries ) ;
// selection entries
static GtkRadioActionEntry mouse_sel_entries [ ] = {
	{ "None"   , NULL , "_None"   , "<control>N" , "No mouse selection"             , MMODE_NONE } ,
	{ "Center" , NULL , "_Center" , "<control>C" , "Central point selection"        , MMODE_CENTER } ,
    { "Path"   , NULL , "P_ath"   , "<control>A" , "Start and end points selection" , MMODE_PATH } ,
    { "Region" , NULL , "_Region" , "<control>R" , "Region selection"               , MMODE_REGION } ,
	} ;
static guint n_mouse_sel_entries = G_N_ELEMENTS ( mouse_sel_entries ) ;
