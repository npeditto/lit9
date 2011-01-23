/*****************************************************************************************************************************
        LIT9 ( Linux Interface T9 ) - Human computer interface based on a TV remote control to interact with a Linux operating system.

        Copyright (C)  2010
                Davide Mulfari: davidemulfari@gmail.com
                Nicola Peditto: n.peditto@gmail.com
                Carmelo Romeo:  carmelo.romeo85@gmail.com
                Fabio Verboso:  fabio.verboso@gmail.com

        This file is part of LIT9.

        LIT9 is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        LIT9 is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with LIT9.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************************************************************

        Developed at Univesity of Messina - Faculty of Engineering - Visilab

        Based on
                - LIRC code http://www.lirc.org/

****************************************************************************************************************************/

//To compile:  g++ ristrutturato.cpp -o lit9 -lsqlite3 -lX11 -lpthread `pkg-config --cflags --libs gtk+-2.0`


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <pthread.h>
#include <sqlite3.h>
#include <ctype.h>
#include <time.h>


#define N 6


//remote-control's configuration (mapping)
#include "pulsanti.h"





//DICHIARAZIONE VARIABILI GLOBALI----------------------------------------------------------------------------------------------------------------

void *thtel (void *arg);		//thread per irw

void *thfilet9 (void *arg);             //thread per caricare il t9
int flagcaricat9;

//----da configurare------------------------------------
gint passo=5;			//mouse step motion
float speed=2.5;		//velocità di digitazione standard 
int wx=0;			//posizione finestra
int wy=0;
//------------------------------------------------------


int stato=3;            //stato iniziale: classico
int tasto;		//codice Keysym del tasto
int tastoprec;		//per la modalità manuale
int indice;		//per lo scorrimento della list box
int numparoletrovate;	//parole trovate nel DB dal T9
int luncodicet9=0;
char codicet9[30];
int modifier=0;         //modificatore per i vari livelli della tastiera (caratteri accentati, maiuscoli, caratteri extra, etc...)

sqlite3 *db;
sqlite3_stmt *stmt;

//global declaration for GTK code-------------------------------
GtkWidget *window;
GtkWidget *gtklist;
GtkWidget *vbox;
GtkWidget *mylabel;
GtkWidget *mylabel2;

GtkStatusIcon *trayIcon;

GtkTreeSelection *selection; 
GtkWidget *label;  
GtkListStore *store;

//--------------------------------------------------------------

struct nodo {
  int frequenza;
  char parola[30];
};

struct nodo vetparole[N];
//char nuovaparola[30];

//variabili per la modalità classica
time_t oldtime;
int y_c=0; int t_prec=0;
int carattere[12][N];   //mapping di tutti i caratteri utilizzabili

int t=1;		//per la gestione della tray-icon

int lock=0;

int man_let[N][2];	//per la modalità manuale


int FUNC=0;		//to change version



//FINE DICHIARAZIONI-----------------------------------------------------------------------------------------------------------------------------








//INIZIO FUNZIONI--------------------------------------------------------------------------------------------------------------------------------

XKeyEvent createKeyEvent(Display *display, Window &win, Window &winRoot, bool press ,int keycode, int modifiers)
{
	   XKeyEvent event;

	   event.display     = display;
	   event.window      = win;
	   event.root        = winRoot;
	   event.subwindow   = None;
	   event.time        = CurrentTime;
	   event.x           = 1;
	   event.y           = 1;
	   event.x_root      = 1;
	   event.y_root      = 1;
	   event.same_screen = True;
	   event.keycode     = XKeysymToKeycode(display, keycode);
	   event.state       = modifiers;

	   if(press)
	      event.type = KeyPress;
	   else
	      event.type = KeyRelease;

	   return event;
}


//Funzione per la stampa del tasto nella finestra in cui c'è il focus
void premitasto(Display *display, Window &winFocus, Window &winRoot, int key, int modifier){

   	XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, key, modifier);
   	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

   	event = createKeyEvent(display, winFocus, winRoot, false, key, modifier);
   	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
}

//Funzione per il caricamento della configurazione del mouse e finestra programma
void caricaconfig(){
	FILE *file;
	char opzione[10];
	char valore[10];
	file=fopen("config","r");
	if (file!=NULL){
		while(!feof(file)){ 
			fscanf(file,"%s %s\n",opzione,valore);
			if (!strcmp(opzione,"passo")) passo=atoi(valore);
			if (!strcmp(opzione,"speed")) speed=atof(valore);
			if (!strcmp(opzione,"wx")) wx=atoi(valore);
			if (!strcmp(opzione,"wy")) wy=atoi(valore);		
		}	
		fclose(file);
	}
	else printf("Valori di default\n");
}



//PER LA GESTIONE DELLA LISTBOX---------------------------------------------------------------------------------------------

enum { LIST_ITEM = 0 , N_COLUMNS };

static void init_list(GtkWidget *glist)
{

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("List Items",renderer, "text", LIST_ITEM, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(glist), column);

	store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(glist), GTK_TREE_MODEL(store));

	g_object_unref(store);

}

static void add_to_list(GtkListStore *store, const gchar *str)
{
	GtkTreeIter iter;
  	gtk_list_store_append(store, &iter);
  	gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);

}


void  on_changed(GtkWidget *widget, gpointer label) 
{
  	GtkTreeIter iter;
  	GtkTreeModel *model;
  	char *value;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {

		gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
		g_free(value);
  	}

}

//-----------------------------------------------------------------------------------------------------------



//PER GESTIRE LA TRAY-ICON-----------------------------------------------------------------------------------------------------------------------
static void trayView(GtkMenuItem *item, gpointer window)
{
    gtk_widget_show(GTK_WIDGET(window));
    gtk_window_deiconify(GTK_WINDOW(window));
}

static void trayExit(GtkMenuItem *item, gpointer user_data)
{
    printf("exit");
    gtk_main_quit();
}

static void trayConf(GtkMenuItem *item, gpointer user_data)
{
	system("gedit ./config");
}




static void trayIconActivated(GObject *trayIcon, gpointer window)
{
    gtk_widget_show(GTK_WIDGET(window));

}

static void trayIconPopup(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer popUpMenu)
{
    gtk_menu_popup(GTK_MENU(popUpMenu), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
}


static gboolean window_state_event (GtkWidget *widget, GdkEventWindowState *event, gpointer trayIcon)
{


    if(event->changed_mask == GDK_WINDOW_STATE_ICONIFIED && (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED || event->new_window_state == (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED)))
    {
        gtk_widget_hide (GTK_WIDGET(widget));
        gtk_status_icon_set_visible(GTK_STATUS_ICON(trayIcon), TRUE);
    }
    else if(event->changed_mask == GDK_WINDOW_STATE_WITHDRAWN && (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED || event->new_window_state == (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED)))
    {
        gtk_status_icon_set_visible(GTK_STATUS_ICON(trayIcon), TRUE);
    }

    return TRUE;


}

static gboolean delete_event (GtkWidget *window, GdkEvent *event, gpointer data)
{
    return FALSE;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------




//Funzione per avviare il browser
void *apribrowser(void *arg){
        system("google-chrome");
}







//Funzione per il trasferimento di un'intera parola dalla listbox alla texbox di destinazione
void invio_parola(Display* display){

    	int revert;
	Window winRoot = XDefaultRootWindow(display);
	Window winFocus;
	XGetInputFocus(display, &winFocus, &revert);

	/*
	if ((luncodicet9==0) && (statoiniziale==0))
		return;

	statoiniziale=0;
	*/
	
	if(stato==1){

		if(lock==1){

			if(tasto!=1 || tasto!=42|| tasto!=163){

				if(tasto==7|| tasto==9){

					if(indice<=3){
						if(man_let[indice][1]==0)
							premitasto(display, winFocus, winRoot, man_let[indice][0],1);
						if(man_let[indice][1]==1)
							premitasto(display, winFocus, winRoot, man_let[indice][0],0);

					}
					else
						premitasto(display, winFocus, winRoot, man_let[indice][0],man_let[indice][1]);
					

				}
				else{

					if(indice<=2){
						if(man_let[indice][1]==0)
							premitasto(display, winFocus, winRoot, man_let[indice][0],1);
						if(man_let[indice][1]==1)
							premitasto(display, winFocus, winRoot, man_let[indice][0],0);

					}
					else
						premitasto(display, winFocus, winRoot, man_let[indice][0],man_let[indice][1]);

				}


			}
			else
				premitasto(display, winFocus, winRoot, man_let[indice][0],man_let[indice][1]);
				

			
		}
		else
			premitasto(display, winFocus, winRoot, man_let[indice][0],man_let[indice][1]);

		
	}
	else{

	    	int dim_parola = strlen(vetparole[indice].parola);
		char word[dim_parola];
		sprintf(word,"%s",vetparole[indice].parola);

		/*
		if ((statopredittore ==1) && (flagparolaconpr==1))
		{
			bzero(word,dim_parola);
			char com[dim_parola];
			sprintf(com,"%s",vetparole[indice].parola);
			int j=0;
			for (int i=luncodicet9; i < dim_parola; i++)
			{
				word[j]=com[i];
				j=j+1;
			}
			word[j]='\0';
			dim_parola =j;
		}
		*/

		for (int kk=0; kk < dim_parola; kk++)
		{

			gchar  *let;
			let = (gchar*)malloc(sizeof(gchar));
			sprintf(let,"");

			sprintf(let,"%c",word[kk]);
			//printf("%s", let);fflush(stdout);

			/*
			if (strcmp(let, "à")==0){if (strcmp(codice, Record)==0){


				printf("ciaoooooooooooo");fflush(stdout);

				premitasto(display, winFocus, winRoot, XK_agrave, 0);

			}


			else 
			*/
			if(lock==1)
				premitasto(display, winFocus, winRoot, XStringToKeysym(let),1);
			if(lock==0)
				premitasto(display, winFocus, winRoot, XStringToKeysym(let),0);

		}

	}//chiusura else


	printf("\n");

	if(stato==2){		//T9

		premitasto(display, winFocus, winRoot,XK_space,modifier);

		vetparole[indice].frequenza=vetparole[indice].frequenza+1;
		printf("\nNuova frequenza parola selezionata: %d\n",vetparole[indice].frequenza);

		char query[200];
		bzero (query,200);

		//if (vetparole[indice].frequenza>1)
		sprintf (query, "update personale set frequenza =%d where parola =\'%s\';",vetparole[indice].frequenza,vetparole[indice].parola);
		//else sprintf (query, "insert into personale (codice,parola,frequenza) values (\'%s\',\'%s\',1);",codicet9,vetparole[indice].parola);

		printf("\n%s\n",query);

		int  retval = sqlite3_exec(db,query,0,0,0);


	}

	luncodicet9 = 0;
	bzero(codicet9,30);
	//bzero(nuovaparola,30);


	fflush(stdout);


}





//Algoritmo T9 di predizione del testo basato su liste
void gestionet9 (int tasto, Display* display){


	numparoletrovate=0;


	int i=0;

	gtk_list_store_clear (store);


	//inizializzazione vetparole[]
	for (i=0; i<N;i++)
	{
		vetparole[i].frequenza=0;
		bzero(vetparole[i].parola,30);
	}


	//se passiamo 99 veniamo da una cancellazione
	if ((tasto<99) && (tasto>0))
	{
		luncodicet9=luncodicet9+1;
		sprintf(codicet9,"%s%d",codicet9,tasto);
	}

	if (tasto>0)
		printf("\nTasti premuti: %s\tlunghezza:%d\n",codicet9,luncodicet9);

	char query[250];
	bzero (query,250);

	//if (modo==0)
	sprintf (query, "select parola dist,frequenza, codice from personale where codice like \'%s%%\' union select parola dist,frequenza, codice from globale where codice like \'%s%%\' order by frequenza desc, codice asc limit 0,%d;",codicet9,codicet9,N);

/*
	else if (modo==1)
	{
		sprintf (query, " select parola, frequenza, codice from personale order by 2 desc limit 0,5;");
		statoiniziale=1;
	}
	else if (modo==2)
		sprintf (query, " select parola dist,frequenza, codice from personale where parola like \'%s%%\' union select parola dist,frequenza, codice from globale where parola like \'%s%%\' order by frequenza desc, parola asc limit 0,5;",nuovaparola,nuovaparola);
*/

	//printf("\n%s\n",query);

	//gchar  *str;



	int  retval = sqlite3_prepare_v2(db,query,-1,&stmt,0);

	if(retval)
	{
        	printf("\nerrore database\n");
        	return;
	}

    	// Read the number of rows fetched
    	int cols = sqlite3_column_count(stmt);


	gtk_list_store_clear (store);


   	while(1)
    	{

		// fetch a row's status
		retval = sqlite3_step(stmt);

		if(retval == SQLITE_DONE) break;

                else if(retval == SQLITE_ROW)
		{
		    // SQLITE_ROW means fetched a row
		    numparoletrovate=numparoletrovate+1;

		    printf ("\n");

		    // sqlite3_column_text returns a const void* , typecast it to const char*
		    for(int col=0 ; col<cols-1;col++)
		    {
		        const char *val = (const char*)sqlite3_column_text(stmt,col);
		        //printf("%s = %s\t",sqlite3_column_name(stmt,col),val);

			if (col==0)
			{

				printf ("%s",val);
				sprintf(vetparole[numparoletrovate-1].parola,"%s",val);
				add_to_list(store, val);
				
			}
			else
			{
				printf ("\tfr=%s",val);
				vetparole[numparoletrovate-1].frequenza=atoi(val);
			}

		    }

		}
		else
		{
		    // Some error encountered
		    printf("errori query\n");
		    return;
		}


    	}

	fflush(stdout);

        tastoprec=0;
	indice=0;

	//if ((modo==2) && (numparoletrovate>0)) flagparolaconpr=1;
	/*
	if(numparoletrovate > 0)
	{

		gtk_list_append_items((GtkList*)(gtklist), dlist);
		gtk_list_select_item((GtkList *) gtklist,indice);

	}
	*/

	//se non trova la parola entra in automatico in modalità manuale
	/*else
	{
		//if (statot9==1){
			//statopredittore=0;
			//flagparolaconpr=0;
			//statot9=2;
			//tastoprec=0;
			printf("\nNuova parola");
			bzero(codicet9,30);
			luncodicet9 = 0;
			bzero(nuovaparola,30);

			stato=1

			gtk_container_remove(GTK_CONTAINER(vbox), mylabel2);
			mylabel2 = gtk_label_new (NULL);
			gtk_label_set_text (GTK_LABEL (mylabel2),"Manuale");
			gtk_container_add(GTK_CONTAINER(vbox), mylabel2);
			gtk_widget_show (mylabel2);
			gdk_window_process_all_updates ();
		//}

	}*/



    	//gtk_widget_show_all(window);
	//gtk_widget_show(window);

	if(FUNC)
		gdk_window_process_all_updates ();


    	printf ("\n");


}


void manuale(int tasto){

					
	stato=1;

	int i;

	gtk_list_store_clear (store);


	for (i=0; i<N; i++)
	{


		if (tasto==1)
		{
				
		             if (i==0) {sprintf(vetparole[i].parola, "."); man_let[i][0] = carattere[0][0]; man_let[i][1]=0; }
			else if (i==1) {sprintf(vetparole[i].parola, ","); man_let[i][0] = carattere[0][1]; man_let[i][1]=0; }
			else if (i==2) {sprintf(vetparole[i].parola, "?"); man_let[i][0] = carattere[0][2]; man_let[i][1]=1; }
			else if (i==3) {sprintf(vetparole[i].parola, "!"); man_let[i][0] = carattere[0][3]; man_let[i][1]=1; }
			else if (i==4) {sprintf(vetparole[i].parola, ";"); man_let[i][0] = carattere[0][4]; man_let[i][1]=1; }
			else if (i==5) {sprintf(vetparole[i].parola, ":"); man_let[i][0] = carattere[0][5]; man_let[i][1]=1; }

		}

		if (tasto==2)
		{
					
			     if (i==0) {sprintf(vetparole[i].parola, "a"); man_let[i][0] = carattere[1][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "b"); man_let[i][0] = carattere[1][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "c"); man_let[i][0] = carattere[1][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "à"); man_let[i][0] = carattere[1][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "2"); man_let[i][0] = carattere[1][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[1][5]; man_let[i][1]=0;}

		}


		else if (tasto==3)
	    	{
			     if (i==0) {sprintf(vetparole[i].parola, "d"); man_let[i][0] = carattere[2][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "e"); man_let[i][0] = carattere[2][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "f"); man_let[i][0] = carattere[2][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "è"); man_let[i][0] = carattere[2][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "3"); man_let[i][0] = carattere[2][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[2][5]; man_let[i][1]=0;}

	    	}
		else if (tasto==4)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "g"); man_let[i][0] = carattere[3][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "h"); man_let[i][0] = carattere[3][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "i"); man_let[i][0] = carattere[3][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "ì"); man_let[i][0] = carattere[3][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "4"); man_let[i][0] = carattere[3][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[3][5]; man_let[i][1]=0;}
		}
		else if (tasto==5)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "j"); man_let[i][0] = carattere[4][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "k"); man_let[i][0] = carattere[4][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "l"); man_let[i][0] = carattere[4][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "5"); man_let[i][0] = carattere[4][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[4][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[4][5]; man_let[i][1]=0;}
		}
		else if (tasto==6)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "m"); man_let[i][0] = carattere[5][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "n"); man_let[i][0] = carattere[5][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "o"); man_let[i][0] = carattere[5][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "ò"); man_let[i][0] = carattere[5][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "6"); man_let[i][0] = carattere[5][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[5][5]; man_let[i][1]=0;}
		}
		else if (tasto==7)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "p"); man_let[i][0] = carattere[6][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "q"); man_let[i][0] = carattere[6][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "r"); man_let[i][0] = carattere[6][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "s"); man_let[i][0] = carattere[6][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "7"); man_let[i][0] = carattere[6][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[6][5]; man_let[i][1]=0;}
		}
		else if (tasto==8)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "t"); man_let[i][0] = carattere[7][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "u"); man_let[i][0] = carattere[7][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "v"); man_let[i][0] = carattere[7][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "ù"); man_let[i][0] = carattere[7][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "8"); man_let[i][0] = carattere[7][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[7][5]; man_let[i][1]=0;}
		}
		else if (tasto==9)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "w"); man_let[i][0] = carattere[8][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "x"); man_let[i][0] = carattere[8][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "y"); man_let[i][0] = carattere[8][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "z"); man_let[i][0] = carattere[8][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "9"); man_let[i][0] = carattere[8][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[8][5]; man_let[i][1]=0;}
		}

		else if (tasto==42)	//asterisk
		{
			     if (i==0) {sprintf(vetparole[i].parola, "*"); man_let[i][0] = carattere[9][0]; man_let[i][1]=1;}
			else if (i==1) {sprintf(vetparole[i].parola, "@"); man_let[i][0] = carattere[9][1]; man_let[i][1]=XK_Shift_R; }
			else if (i==2) {sprintf(vetparole[i].parola, "-"); man_let[i][0] = carattere[9][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "_"); man_let[i][0] = carattere[9][3]; man_let[i][1]=1;}
			else if (i==4) {sprintf(vetparole[i].parola, "("); man_let[i][0] = carattere[9][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, ")"); man_let[i][0] = carattere[9][5]; man_let[i][1]=0;}
		}
		else if (tasto==163)	//sterling/hash
		{
			     if (i==0) {sprintf(vetparole[i].parola, "#"); man_let[i][0] = carattere[10][0]; man_let[i][1]=XK_Shift_R; }
			else if (i==1) {sprintf(vetparole[i].parola, "/"); man_let[i][0] = carattere[10][1]; man_let[i][1]=1; }
			else if (i==2) {sprintf(vetparole[i].parola, "="); man_let[i][0] = carattere[10][2]; man_let[i][1]=1; }
			else if (i==3) {sprintf(vetparole[i].parola, "+"); man_let[i][0] = carattere[10][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "$"); man_let[i][0] = carattere[10][4]; man_let[i][1]=1; }
			else if (i==5) {sprintf(vetparole[i].parola, "€"); man_let[i][0] = carattere[10][5]; man_let[i][1]=XK_Shift_R; }
		}
		


		add_to_list(store, vetparole[i].parola);


	}

	indice=0;

	if(FUNC)
		gdk_window_process_all_updates();
	



}



void classico(int tasto, Display* display){


	Window winRoot = XDefaultRootWindow(display);
	Window winFocus;

	int revert;

	int x=0;

	if     (tasto==42)  x=9;
	else if(tasto==163) x=10;
	else    x=tasto-1;


	time_t newtime=time(NULL);
	gchar *let;
	let = (gchar*)malloc(sizeof(gchar));
	sprintf(let,"");

	if (newtime - oldtime < speed && tasto==t_prec){

		y_c++;

		if (y_c>0){

        		XGetInputFocus(display, &winFocus, &revert);
			premitasto(display, winFocus, winRoot,XK_BackSpace,modifier);

		}
		if (y_c>N-1) 
			y_c=0;

	}
	else 
		y_c=0;
	

	oldtime=newtime;
	//printf("%c\n",carattere[x][y_c]);
	t_prec=tasto;
	sprintf(let,"%c",carattere[x][y_c]);

	if(carattere[x][y_c]==XK_question || carattere[x][y_c]==XK_exclam || (x==0 && y_c==5) || (x==10 && y_c==1) || (x==10 && y_c==4) || (x==10 && y_c==2) ||(x==9 && y_c==0)|| (x==9 && y_c==3)|| carattere[x][y_c]==XK_semicolon )

			modifier=1;

	if( (x==10 && y_c==5) ||(x==9 && y_c==1) ||(x==10 && y_c==0))

			modifier=XK_Shift_R;



	XGetInputFocus(display, &winFocus, &revert);



if(lock==1){

			if(tasto!=1 || tasto!=42|| tasto!=163){

				if(tasto==7|| tasto==9){

					if(y_c<=3){
						if(modifier==0)
							premitasto(display, winFocus, winRoot, carattere[x][y_c],1);
						if(modifier==1)
							premitasto(display, winFocus, winRoot, carattere[x][y_c],0);

					}
					else
						premitasto(display, winFocus, winRoot, carattere[x][y_c],modifier);
					

				}
				else{

					if(y_c<=2){
						if(modifier==0)
							premitasto(display, winFocus, winRoot, carattere[x][y_c],1);
						if(modifier==1)
							premitasto(display, winFocus, winRoot, carattere[x][y_c],0);

					}
					else
						premitasto(display, winFocus, winRoot, carattere[x][y_c],modifier);

				}


			}
			else
				premitasto(display, winFocus, winRoot, carattere[x][y_c],modifier);
				

			
		}
		else
			premitasto(display, winFocus, winRoot, carattere[x][y_c],modifier);




	//premitasto(display, winFocus, winRoot,carattere[x][y_c],modifier);

	modifier=0;



}




void numerico(int tasto){

	Display *display = XOpenDisplay(0);

	Window winRoot = XDefaultRootWindow(display);
	Window winFocus;
	int revert;
	XGetInputFocus(display, &winFocus, &revert);

	//printf("%d\n",tasto);
	printf("\n");

	gchar  *let;
	let = (gchar*)malloc(sizeof(gchar));
	sprintf(let,"%d",tasto);


	premitasto(display, winFocus, winRoot,XStringToKeysym(let),modifier);

	XCloseDisplay(display);

}


void caricamatrice (){

        carattere[0][0] = XK_period;
        carattere[0][1] = XK_comma;
        carattere[0][2] = XK_question;		//for "?" with modifier=1
        carattere[0][3] = XK_exclam;		//for "!" with modifier=1
        carattere[0][4] = XK_semicolon;		//for ";" with modifier=1
        carattere[0][5] = XK_period; 		//for ":" with modifier=1
		

        carattere[1][0] = XK_A;
        carattere[1][1] = XK_B;
        carattere[1][2] = XK_C;
        carattere[1][3] = XK_agrave;
        carattere[1][4] = XK_2;
	carattere[1][5] = XK_space;		

        carattere[2][0] = XK_D;
        carattere[2][1] = XK_E;
        carattere[2][2] = XK_F;
        carattere[2][3] = XK_egrave;
        carattere[2][4] = XK_3;
	carattere[2][5] = XK_space;		

        carattere[3][0] = XK_G;
        carattere[3][1] = XK_H;
        carattere[3][2] = XK_I;
        carattere[3][3] = XK_igrave;
        carattere[3][4] = XK_4;
        carattere[3][5] = XK_space;		

        carattere[4][0] = XK_J;
        carattere[4][1] = XK_K;
        carattere[4][2] = XK_L;
        carattere[4][3] = XK_5;
        carattere[4][4] = XK_space;		
        carattere[4][5] = XK_space;		

        carattere[5][0] = XK_M;
        carattere[5][1] = XK_N;
        carattere[5][2] = XK_O;
        carattere[5][3] = XK_ograve;
        carattere[5][4] = XK_6;
        carattere[5][5] = XK_space;		

        carattere[6][0] = XK_P;
        carattere[6][1] = XK_Q;
        carattere[6][2] = XK_R;
        carattere[6][3] = XK_S;
        carattere[6][4] = XK_7;
        carattere[6][5] = XK_space;		

        carattere[7][0] = XK_T;
        carattere[7][1] = XK_U;
        carattere[7][3] = XK_V;
        carattere[7][2] = XK_ugrave;
        carattere[7][4] = XK_8;
        carattere[7][5] = XK_space;		

        carattere[8][0] = XK_W;
        carattere[8][1] = XK_X;
        carattere[8][2] = XK_Y;
        carattere[8][3] = XK_Z;
        carattere[8][4] = XK_9;
        carattere[8][5] = XK_space;


        carattere[9][0] = XK_plus;			//for "*" with modifier=1
        carattere[9][1] = XK_ograve;			//for "@" with modifier=XK_Shift_R
        carattere[9][2] = XK_minus;
        carattere[9][3] = XK_underscore;
        carattere[9][4] = XK_parenleft;
	carattere[9][5] = XK_parenright;

        carattere[10][0] = XK_agrave;
        carattere[10][1] = XK_slash;
        carattere[10][2] = XK_equal;			//for "=" with modifier=1
        carattere[10][3] = XK_plus;
        carattere[10][4] = XK_dollar;
        carattere[10][5] = XK_E;		//for "€" with modifier=XK_Shift_L



}



void elabora(char *codice)
{

	Display *display = XOpenDisplay(0);

	if(display == NULL) return;


	//Modalità T9
	if (strcmp(codice, yellow)==0){

		if(t==0){
			t=1;
			gtk_widget_show (window);
			if(FUNC)
				gdk_window_process_all_updates();
		}

		printf("\nT9 ON\n");

		stato=2;

		bzero(codicet9,30);


		gtk_label_set_text (GTK_LABEL (mylabel2),"Status: T9 abc");
		gtk_widget_show (mylabel2);
		
		//gtk_widget_show_all(window);
		if(FUNC)
			gdk_window_process_all_updates ();   //http://library.gnome.org/devel/gdk/2.13/gdk-Windows.html#gdk-window-process-all-updates

	}

	//Modalità selezione lettera da listbox
	if (strcmp(codice, red)==0){

		if(t==0){
			t=1;
			gtk_widget_show (window);
			if(FUNC)
				gdk_window_process_all_updates();
		}

		printf("\nSelective\n");

		stato=1;
		
		bzero(codicet9,30);

		gtk_label_set_text (GTK_LABEL (mylabel2),"Status: Selective");
		gtk_widget_show (mylabel2);
		if(FUNC)
			gdk_window_process_all_updates ();
		//gtk_widget_show_all(window);

	}

	//Modalità tradizionale
	else if (strcmp(codice, green)==0){

		if(t==1){
			t=0;
			gtk_widget_hide (window);
			if(FUNC)
				gdk_window_process_all_updates();
		}

		printf("\nStandard\n");

		stato=3;

		bzero(codicet9,30);

		gtk_label_set_text (GTK_LABEL (mylabel2),"Status: Standard");
		gtk_widget_show (mylabel2);

		//gtk_widget_show_all(window);		
		if(FUNC)	
			gdk_window_process_all_updates ();


	}

	//Modalità numerica
	else if (strcmp(codice, blue)==0){

		if(t==1){
			t=0;
			gtk_widget_hide (window);
			if(FUNC)
				gdk_window_process_all_updates();
		}

		
		stato=4;

		printf("\nNumeric\n");

		bzero(codicet9,30);

		gtk_label_set_text (GTK_LABEL (mylabel2),"Status: Numeric");
		gtk_widget_show (mylabel2);

		//gtk_widget_show_all(window);
		//gtk_widget_show(window);

		if(FUNC)	
			gdk_window_process_all_updates ();
		

	}


	//Tasto Power per uscire dal programma
   	else  if (strcmp(codice, power)==0) exit(0);

	//Tasti direzionali per gestire il puntatore del mouse
   	else if (strcmp(codice, down)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, 0,passo);      //tasto DOWN
   	else if (strcmp(codice, up)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, 0,passo*(-1));   //tasto UP
   	else if (strcmp(codice, right)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, passo, 0);    //tasto RIGHT
   	else if (strcmp(codice, left)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, passo*(-1),0); //tasto LEFT

	else if (strcmp(codice, caps)==0){

		if(lock==1){
			lock=0;
			
			if(stato==1)
					gtk_label_set_text (GTK_LABEL (mylabel2),"Status: Selective");
			if(stato==2)
					gtk_label_set_text (GTK_LABEL (mylabel2),"Status: T9 abc");
			if(stato==3)
					gtk_label_set_text (GTK_LABEL (mylabel2),"Status: abc");

			gtk_widget_show (mylabel2);
			if(FUNC)
				gdk_window_process_all_updates ();

		}
		else if(lock==0){

			lock=1;

			if(stato==1)
					gtk_label_set_text (GTK_LABEL (mylabel2),"Status: SELECTIVE");
			if(stato==2)
					gtk_label_set_text (GTK_LABEL (mylabel2),"Status: T9 ABC");
			if(stato==3)
					gtk_label_set_text (GTK_LABEL (mylabel2),"Status: ABC");

			gtk_widget_show (mylabel2);
			if(FUNC)
				gdk_window_process_all_updates ();

		}

			

	}


	//TRAY-ICON
    	else if (strcmp(codice, Record)==0){

		if (t==0){

                	printf("visualizza\n");

			gtk_widget_show (window);

			t=1;
		}
		else{

                	printf("nascondi\n");

                	gtk_widget_hide (window);
                	//gtk_status_icon_set_visible(GTK_STATUS_ICON(trayIcon), TRUE);

			t=0;

		}

    		if(FUNC)
			gdk_window_process_all_updates ();


	}


	//Tasti CH+ e CH- per lo scorrimento della listbox
   	else if (strcmp(codice, ch_minus)==0)
   	{
		//if ((numparoletrovate>0) && ((statot9==1) || (statopredittore==1)))
			//if (indice==numparoletrovate-1)
			if (indice==(N-1))
				indice=0;
			else
				indice=indice+1;


		/*
		else if ((statot9==0) || (statot9==2)){
			if (indice==(N-1))
				indice=0;
			else
				indice = indice+1;
		}
		*/

		GtkTreeIter iter;
		gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
		gtk_tree_selection_select_iter ( selection , &iter ) ;

		if(FUNC) 
			gdk_window_process_all_updates ();


   	}
   	else  if (strcmp(codice, ch_plus)==0)
   	{
       		if (indice==0)
			indice=N-1;
       		else
			indice = indice-1;

		GtkTreeIter iter;
		gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
		gtk_tree_selection_select_iter ( selection , &iter ) ;


       		if(FUNC) 
			gdk_window_process_all_updates ();

   	}



	//Tasto Home per lanciare il thread del browser
   	else  if (strcmp(codice, home)==0)
	{
		int res;
		pthread_t tel_thread;
		res = pthread_create(&tel_thread, NULL, apribrowser, NULL);
		if (res != 0) {
			exit(EXIT_FAILURE);
		 	printf("\nerrore partenza thread");

		}

	}


	//La pressione di tale tasto manderà nell'output selezionato la parola o lettera della listbox
	else if (strcmp(codice, tasto_exit)==0) {

		invio_parola(display);
		GtkTreeIter iter;
		gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
		gtk_tree_selection_select_iter ( selection , &iter ) ;

	}


	//corrisponde al click del pulsante sinistro del mouse
   	else if (strcmp(codice, ok)==0)
   	{

        	XEvent event;
        	memset(&event, 0x00, sizeof(event));
        	event.type = ButtonPress;
        	event.xbutton.button = 1;
        	event.xbutton.same_screen = True;
		XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, 				&event.xbutton.x_root,&event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
        	event.xbutton.subwindow = event.xbutton.window;
        	while(event.xbutton.subwindow)
        	{
                	event.xbutton.window = event.xbutton.subwindow;
                	XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, 					&event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
        	}
        	XSendEvent(display, PointerWindow, True, 0xfff, &event);
        	XFlush(display);
        	usleep(100000);
        	event.type = ButtonRelease;
        	event.xbutton.state = 0x100;
        	XSendEvent(display, PointerWindow, True, 0xfff, &event);
        	XFlush(display);

	}



	//Tutte quelle funzioni da lanciare alla pressione di un tasto numerico
	else if ( (strcmp(codice, tasto_1)==0 && stato==4) || strcmp(codice, tasto_2)==0 ||  strcmp(codice, tasto_3)==0 || strcmp(codice, tasto_4)==0 || strcmp(codice, tasto_5)==0 || strcmp(codice, tasto_6)==0 || strcmp(codice, tasto_7)==0 || strcmp(codice, tasto_8)==0 || strcmp(codice, tasto_9)==0) {

		// se non ho premuto un tasto o se si tratta di un tasto numerico
		     if (strcmp(codice, tasto_1)==0) tasto=1;
		else if (strcmp(codice, tasto_2)==0) tasto=2;
		else if (strcmp(codice, tasto_3)==0) tasto=3;
		else if (strcmp(codice, tasto_4)==0) tasto=4;
		else if (strcmp(codice, tasto_5)==0) tasto=5;
		else if (strcmp(codice, tasto_6)==0) tasto=6;
		else if (strcmp(codice, tasto_7)==0) tasto=7;
		else if (strcmp(codice, tasto_8)==0) tasto=8;
		else if (strcmp(codice, tasto_9)==0) tasto=9;


		GtkTreeIter iter;

		switch(stato){

			case 1://MANUALE

				manuale(tasto);

				gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
				gtk_tree_selection_select_iter ( selection , &iter ) ;
				if(FUNC)
					gdk_window_process_all_updates ();

			break;


			case 2://T9

				gestionet9(tasto,display);

				gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
				gtk_tree_selection_select_iter ( selection , &iter ) ;
				if(FUNC)
					gdk_window_process_all_updates ();

			break;


			case 3://STANDARD

				classico(tasto,display);

			break;

			case 4://NUMERICO
				
				numerico(tasto);
			break;

		}

	}


	else if (strcmp(codice, tasto_1)==0) {

		tasto=1;

		GtkTreeIter iter;

		if(stato==1){
			manuale(tasto);
			gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
			gtk_tree_selection_select_iter ( selection , &iter ) ;
			if(FUNC)
				gdk_window_process_all_updates ();
		}
		else
			classico(tasto,display);



	}

	else if (strcmp(codice, star)==0 ) {

		tasto=42;

		GtkTreeIter iter;

		if(stato==1){
			manuale(tasto);
			gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
			gtk_tree_selection_select_iter ( selection , &iter ) ;
			if(FUNC)
				gdk_window_process_all_updates ();
		}
		else
			classico(tasto,display);



	}

	else if (strcmp(codice, hash)==0 ) {

		tasto=163;

		GtkTreeIter iter;

		if(stato==1){
			manuale(tasto);
			gtk_tree_model_iter_nth_child ( (GtkTreeModel *) store , &iter , NULL , indice );
			gtk_tree_selection_select_iter ( selection , &iter ) ;
			if(FUNC)
				gdk_window_process_all_updates ();
		}
		else
			classico(tasto,display);



	}



	//tasti di navigazione web: Tab, Invio, Vol+ e Vol- per lo scorrimento di elenchi
	else {



		if (strcmp(codice, videos)==0) tasto=XK_Tab;
		else if (strcmp(codice, mytv)==0) {tasto=XK_Tab; modifier=1;}
   		else if (strcmp(codice, enter)==0) tasto =XK_Return;    //tasto VERDE
   		else if (strcmp(codice, vol_plus)==0) tasto=XK_Up;      //tasto volume+
   		else if (strcmp(codice, vol_minus)==0) tasto =XK_Down;  //tasto volume-



		
		//Tasto spazio
  		else if (strcmp(codice, tasto_0)==0) {

			if(stato==3 || stato==1 || stato==2) 
				tasto = XK_space;
            		if(stato==4) 
				tasto = XK_0;


			/*
			if ((statot9==2) && (luncodicet9>0))
			{

				char query[200];
				bzero (query,200);
				sprintf (query, "insert into personale (codice,parola,frequenza) values (\'%s\',\'%s\',1);",codicet9,nuovaparola);
				printf("\n%s\n",query);
				int  retval = retval = sqlite3_exec(db,query,0,0,0);
				statot9=1;
				luncodicet9 = 0;
				bzero(codicet9,30);
				gtk_container_remove(GTK_CONTAINER(vbox), mylabel2);
				mylabel2 = gtk_label_new (NULL);
				gtk_label_set_text (GTK_LABEL (mylabel2),"T9 attivo");
				gtk_container_add(GTK_CONTAINER(vbox), mylabel2);
				gtk_widget_show (mylabel2);
				gdk_window_process_all_updates ();


				//XWarpPointer(display, None, None, 0, 0, 0, 0, -10000,-10000);
				//XWarpPointer(display, None, None, 0, 0, 0, 0, 90, 0);
				gdk_window_process_all_updates ();

			}
			*/

		}


		//Tasto per cancellare
   		else if (strcmp(codice, mute_clear)==0)
		{
			tasto = XK_BackSpace;
		}
/*
		else if (strcmp(codice, mute_clear)==0)
		{
			if ((luncodicet9==0) && (stato==2))
				tasto = XK_BackSpace;
			else if ((luncodicet9>0) && (statot9==1))
			{

				//printf("\nlungh_prima: %d", luncodicet9 );
				strncpy (codicet9,codicet9,luncodicet9-1);
				codicet9[luncodicet9-1]='\0';
				luncodicet9=luncodicet9-1;
				//fflush(stdout);
				//printf("\ncodicet9: %s", codicet9 );
				if(luncodicet9 == 0)
				{
					bzero(codicet9,30);
					numparoletrovate=0;
					gtk_list_clear_items ((GtkList *) gtklist,0,N);
					gdk_window_process_all_updates ();
				}
				else gestionet9(99,0);
				//printf("\nlungh_dopo: %d", luncodicet9 );
				return;

			}
			if ((luncodicet9>0) && ((statot9==2) || (statopredittore==1)))
			{

				strncpy (nuovaparola,nuovaparola,luncodicet9-1);
				nuovaparola[luncodicet9-1]='\0';
				strncpy (codicet9,codicet9,luncodicet9-1);
				codicet9[luncodicet9-1]='\0';
				luncodicet9=luncodicet9-1;
				printf("\nTasti premuti: %s\tlunghezza:%d\n",codicet9,luncodicet9);
				printf("\nParola composta: %s\n",nuovaparola);
				if (statopredittore==1)
				{
					if(luncodicet9 == 0)
					{
						bzero(nuovaparola,30);
						numparoletrovate=0;
						gtk_list_clear_items ((GtkList *) gtklist,0,N);
						gdk_window_process_all_updates ();
					}
					else gestionet9(99,2);
				}

			}




		}

	*/
  

		//TASTO**************************************************************************

			// Get the root window for the current display.
			Window winRoot = XDefaultRootWindow(display);

			// Find the window which has the current keyboard focus.
			Window winFocus;
			int revert;
			XGetInputFocus(display, &winFocus, &revert);

			premitasto(display, winFocus, winRoot,tasto,modifier);


		//*******************************************************************************

			
	}

	if (strcmp(codice, mytv)==0) modifier=0;	//per la gestione del backtab



	XCloseDisplay(display);


}



//Funzione thread di IRW
void *thtel(void *arg){

  	printf("\nIRW in ascolto\n");

 	int fd,i;
        char buf[128];

        struct sockaddr_un addr;
        addr.sun_family=AF_UNIX;
        strcpy(addr.sun_path,"/dev/lircd");
        fd=socket(AF_UNIX,SOCK_STREAM,0);

        if(fd==-1)  {
                perror("socket");
                exit(errno);
        };

        if(connect(fd,(struct sockaddr *)&addr,sizeof(addr))==-1)  {
                perror("connect");
                exit(errno);
        };

        char cod[8];
        int lun,j,k;
        for(;;)
	{

                bzero(buf,128);
                memset(cod,0,sizeof(cod));
                i=read(fd,buf,128);

                if(i==-1)  {
                        perror("read");
                        exit(errno);
                };
                if(!i)
                    exit(0);

                j=0;
                k=0;
                lun=strlen(buf);
                for (j=9;j<lun;j++)
                {
                        if (buf[j]==' ') break;
                        cod[k]=buf[j];
                        k=k+1;

                }
                //printf("\nCodice:\t%s\nStringa restituita dal driver: \t%s\n",cod,buf);
                elabora(cod);

        }

}




//Funzione thread che legge il file dizionazrio e riempe la lista per il T9
void *thfilet9 (void *arg){
	int rc;
	rc = sqlite3_open("parole.sqlite", &db);
        if (rc)
 	{
 	       printf("\nerrore database del T9");
	       exit(EXIT_FAILURE);
        }
        else
	{
		printf("\nConnessione al database del T9 avvenuta correttamente\n");
		flagcaricat9=1;
	}
	fflush(stdout);

}





int main( int argc, char *argv[])
{

	//*******************************************************************************
	//system("cat /etc/issue");   //Versione del SO installata   //cat /etc/issue
 	FILE *file_read;
	char buf[30];
	int version=0;

	file_read = fopen("/etc/issue", "r");   //Ubuntu 10.04.1 LTS \n \l

	for(version=0;version<2;version++)
	{
		fscanf(file_read, "%s", buf);
	}
	fclose(file_read);


	if( (strcmp(buf,"10.04.1")==0) || (strcmp(buf,"10.10.1")==0) )
	{
		FUNC=0;
		printf("SO: %d 10.x\n", FUNC);
	}
	else if (strcmp(buf, "9.04")==0)
	{
		FUNC=1;
		printf("SO: %d 9.x\n", FUNC);
	}
	//*******************************************************************************

	//loading matrix of characters
	caricamatrice();
	caricaconfig();
	printf("\nConfigurazione:%d %f %d %d\n",passo,speed,wx,wy);

	//
	
	int i=0, j=0;
	for(i=0; i<N; i++)
		for(j=0; j < 2; j++)
			man_let[i][j]=0;


	//Start database connection thread 
	int rest;
	pthread_t t9_thread;
        flagcaricat9=0;
	rest = pthread_create(&t9_thread, NULL, thfilet9, NULL);
	if (rest != 0)
	{
		printf("\nerrore partenza thread del T9");
		exit(EXIT_FAILURE);
	}

	//Start IRW thread
	int res;
	pthread_t tel_thread;
	res = pthread_create(&tel_thread, NULL, thtel, NULL);
	if (res != 0)
	{
		printf("\nerrore partenza thread di gestione irda");
		exit(EXIT_FAILURE);
	}

	//GTK initialization
	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_POPUP);
	//window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Lit9");
	gtk_window_set_default_size(GTK_WINDOW(window), 135, 135);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);		//
	gtk_container_set_border_width (GTK_CONTAINER (window), 1);

	//gtk_window_set_decorated(GTK_WINDOW(window), NULL);



	//TRAYICON------------------------------------------------------------------------------------------------
	
    		trayIcon = gtk_status_icon_new_from_file ("./icon.png");

		//set popup menu for tray icon
		GtkWidget *menu, *menuItemView, *menuItemExit, *menuItemConf;
		menu = gtk_menu_new();
		menuItemView = gtk_menu_item_new_with_label ("View");
		menuItemConf = gtk_menu_item_new_with_label ("Conf");
		menuItemExit = gtk_menu_item_new_with_label ("Exit");
		g_signal_connect (G_OBJECT (menuItemView), "activate" , G_CALLBACK (trayView), window);
		g_signal_connect (G_OBJECT (menuItemConf), "activate" , G_CALLBACK (trayConf), NULL);
		g_signal_connect (G_OBJECT (menuItemExit), "activate" , G_CALLBACK (trayExit), NULL);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemView);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemConf);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemExit);
		gtk_widget_show_all (menu);

		//set tooltip
		gtk_status_icon_set_tooltip (trayIcon, "MsgWatcherGTK");

		//connect handlers for MOUSE events
		g_signal_connect(GTK_STATUS_ICON (trayIcon), "activate", GTK_SIGNAL_FUNC (trayIconActivated), window);
		g_signal_connect(GTK_STATUS_ICON (trayIcon), "popup-menu", GTK_SIGNAL_FUNC (trayIconPopup), menu);

		//connect handlers for WINDOW events
		g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (delete_event), trayIcon);
	    	g_signal_connect (G_OBJECT (window), "window-state-event", G_CALLBACK (window_state_event), trayIcon);

	
		//set visible icon on program startup
	   	gtk_status_icon_set_visible(GTK_STATUS_ICON(trayIcon), TRUE);

	//--------------------------------------------------------------------------------------------------------------------------------------------------


	//MAIN container (vertical position)
	vbox=gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	//listbox definition
	gtklist = gtk_tree_view_new();
  	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(gtklist), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), gtklist, TRUE, TRUE, 4);

	//label definition
	mylabel2 = gtk_label_new (NULL);
	gtk_label_set_text (GTK_LABEL (mylabel2),"Status: abc");



	gtk_container_add(GTK_CONTAINER(vbox), mylabel2);
	gtk_widget_show (mylabel2);

//GdkColor   color;
//gdk_color_parse ("green", &color);
//gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, &color);
//gtk_widget_modify_base (mylabel2, GTK_STATE_NORMAL, &color);
//gtk_widget_modify_text(mylabel2, GTK_STATE_NORMAL, &color);


	char str[30];
	init_list(gtklist);
  	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(gtklist)));

	for (i=0; i<N; i++)
	{
	
		if(i==0)
			sprintf(str, "%s", "   Welcome in Lit9!  ");
		if(i==1)
			sprintf(str, "%s", "R - Selective");	  
		if(i==2)
			sprintf(str, "%s", "G - Standard");
		if(i==3)
			sprintf(str, "%s", "Y - T9");
		if(i==4)
			sprintf(str, "%s", "B - Numeric");	

		if(i==5)
			sprintf(str, "%s", " ");		
		
		add_to_list(store, str);

	}


  	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtklist));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  	g_signal_connect(selection, "changed", G_CALLBACK(on_changed), label);

 	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	//show all widget in the window
  	gtk_widget_show_all(window);


  	gtk_main();

  	return 0;



}
