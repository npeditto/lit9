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


//To compile:  g++ *.cpp -o lit9 -lsqlite3 -lX11 -lpthread -L/usr/lib -lQtGui -lQtCore 



#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QPalette>
#include <QSystemTrayIcon>
#include <QString>
#include <QGraphicsWidget>
#include <QMenu>
#include <QAction>

#include <QObject>
#include <QThread>




#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


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
#include <wchar.h>



#include "mythread.h"

#define N 6


//remote-control's configuration (mapping)
#include "pulsanti.h"





//DICHIARAZIONE VARIABILI GLOBALI----------------------------------------------------------------------------------------------------------------

int flagcaricat9;

//----da configurare------------------------------------------------------
int passo=5;			//mouse step motion
float speed=2.0;		//velocità di digitazione standard 
int wx=0;			//posizione finestra
int wy=0;
char dictionary[4];
int pred=2;

//------------------------------------------------------------------------


int stato=3;            //stato iniziale: classico
int tasto;		//codice Keysym del tasto
int tastoprec;		//per la modalità manuale
int indice;		//per lo scorrimento della list box
int numparoletrovate;	//parole trovate nel DB dal T9
int luncodicet9=0;
char codicet9[30];
int modifier=0;         //modificatore per i vari livelli della tastiera (caratteri accentati, maiuscoli, caratteri extra, etc...)
int modi=0;


sqlite3 *db;
sqlite3_stmt *stmt;

//control state variables
int homepage=1;
int firsttime=1;
int refresh=1;
int inserimento=0; 	//0->modalità di inserimento // 1->inserimento parola nel DB


	


wchar_t wcarattere[6];//per inserimento parola


//global declaration for QT-------------------------------------

QLabel *array[N];
	
QLabel *lbstatus;

QLabel *lbmotion;

QSystemTrayIcon *tray;

QWidget *window;

//--------------------------------------------------------------



struct nodo {
  int frequenza;
  char parola[30];
};

struct nodo vetparole[N];


//standard mode variables
time_t oldtime;		
int y_c=0; int t_prec=0;

int carattere[12][N];   //for characters mapping

int man_let[N][2];	//for selective mode

int lock=0;		//LOCK BUTTON

int t=1; 		//t=1->window.show()  /  t=0 -> window.hide()

int nav=0;		//nav=0 -> mouse / nav=1 -> arrows


//*************************************************************
char parola_maiuscolo[30];
int mistero=0; 
//*************************************************************


char buf[50];

int cont_char=0;
int predizione=1;


//-----------------------------------------------------------------------------------------------------------------------------------------------








//FUNCTION START--------------------------------------------------------------------------------------------------------------------------------


//Xlib function to manage events
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

void azzera_buf(){
bzero(buf,50); cont_char=0;
}

void azzera_vetparole(){
	//initialization of vetparole[]
	int i=0;
	for (i=0; i<N;i++)
	{
		vetparole[i].frequenza=0;
		bzero(vetparole[i].parola,30);
	}
}

char* uppercase(char *parola){
	int num=0;
	char minuscolo=0, maiuscolo=0;
	int SCARTO = 32;                                      			/*1*/
	int lun_minuscolo=0;

	lun_minuscolo=strlen(parola);
	for(num=0;num<lun_minuscolo;num++)
	{
		minuscolo=parola[num]; 						/*2*/
		if (minuscolo>=97 && minuscolo<=122) 
		{                          					/*3*/
			maiuscolo = minuscolo-SCARTO; 			        /*4*/
		//printf("\n Rappresentazione maiuscola %c",maiuscolo); 	/*5*/
		//printf("\n Codice ASCII %d",maiuscolo);			/*6*/
		}
		else 
		{
			printf("\n Carattere non convertibile");
			//maiuscolo= "X";
		}

		parola[num]=maiuscolo;
	}
	return parola;
}

//xlib function to simulate a fake press button
void premitasto(Display *display, Window &winFocus, Window &winRoot, int key, int modifier){

   	XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, key, modifier);
   	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

   	event = createKeyEvent(display, winFocus, winRoot, false, key, modifier);
   	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
}


//function to charge default configuration from "./config"
void caricaconfig(){

	FILE *file;
	int r=0;
	char opzione[10];
	char valore[10];
	file=fopen("config","r");

	sprintf(dictionary,"IT");

	if (file!=NULL){
		while(!feof(file)){ 
			r=fscanf(file,"%s %s\n",opzione,valore);
			if (!strcmp(opzione,"passo")) passo=atoi(valore);
			if (!strcmp(opzione,"speed")) speed=atof(valore);
			if (!strcmp(opzione,"wx")) wx=atoi(valore);
			if (!strcmp(opzione,"wy")) wy=atoi(valore);
			if (!strcmp(opzione,"diz")) sprintf(dictionary,"%s",valore);	
			if (!strcmp(opzione,"pred")) pred=atoi(valore);
				
		}	
		fclose(file);
	}
	else 
		printf("Default values.\n");

}




//Open browser thread
void *apribrowser(void*){

       int sys = system("firefox");
 	//system("google-chrome");
	
}



//Function to transfer words from GUI to window with the focus
void invio_parola(Display* display){

    	int revert;
	Window winRoot = XDefaultRootWindow(display);
	Window winFocus;
	XGetInputFocus(display, &winFocus, &revert);

	
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
/*
	    	int dim_parola = strlen(vetparole[indice].parola);
		char word[dim_parola];
		sprintf(word,"%s",vetparole[indice].parola);


		for (int kk=0; kk < dim_parola; kk++)
		{


			char  *let;
			let = (char*)malloc(2*sizeof(char));
			sprintf(let,"%c",word[kk]);
			
				if(lock==1)
					premitasto(display, winFocus, winRoot, XStringToKeysym(let),1);
				if(lock==0)
					premitasto(display, winFocus, winRoot, XStringToKeysym(let),0);


		}
*/
	    	int dim_parola = strlen(vetparole[indice].parola);
		char word[dim_parola];
		sprintf(word,"%s",vetparole[indice].parola);

		char  *due;
		due = (char*)malloc(2*sizeof(char));

		for (int kk=0; kk < dim_parola; kk++)
		{


			char  *let;
			let = (char*)malloc(2*sizeof(char));

			if(kk==dim_parola-2)
				due[0]=word[kk];
			else if(kk==dim_parola-1)
				due[1]=word[kk];
			else {
				sprintf(let,"%c",word[kk]);
				//printf("\nlet: %s\n", let);

				if(lock==1)
					premitasto(display, winFocus, winRoot, XStringToKeysym(let),1);
				if(lock==0)
					premitasto(display, winFocus, winRoot, XStringToKeysym(let),0);

			}
			

		}
			

			if(strcmp(due,"à")==0)
				premitasto(display, winFocus, winRoot, XK_agrave,0);
			else if(strcmp(due,"è")==0)
				premitasto(display, winFocus, winRoot, XK_egrave,0);
			else if(strcmp(due,"ì")==0)
				premitasto(display, winFocus, winRoot, XK_igrave,0);
			else if(strcmp(due,"ò")==0)
				premitasto(display, winFocus, winRoot, XK_ograve,0);
			else if(strcmp(due,"ù")==0)
				premitasto(display, winFocus, winRoot, XK_ugrave,0);
			else if(strcmp(due,"é")==0)
				premitasto(display, winFocus, winRoot, XK_eacute,0);




	}


	//printf("\n");


	if(stato==2){		//for T9 mode

		premitasto(display, winFocus, winRoot,XK_space,modifier);

		vetparole[indice].frequenza=vetparole[indice].frequenza+1;
		printf("\nNuova frequenza parola selezionata: %d\n",vetparole[indice].frequenza);

		char query[200];
		bzero (query,200);

		
		sprintf (query, "update globale set frequenza =%d where parola =\'%s\';",vetparole[indice].frequenza,vetparole[indice].parola);
		int  retval = sqlite3_exec(db,query,0,0,0);

		if(retval)
		{
			printf("\nDatabase Error: update global table!\n");
			return;
		}
		
		sprintf (query, "update personale set frequenza =%d where parola =\'%s\';",vetparole[indice].frequenza,vetparole[indice].parola);
		retval = sqlite3_exec(db,query,0,0,0);

		if(retval)
		{
			printf("\nDatabase Error: update personal table!\n");
			return;
		}


	}

	luncodicet9 = 0;
	bzero(codicet9,30);


	fflush(stdout);


}





//T9 algoritm for text prediction (sqlite based)
void gestionet9 (int tasto){


	numparoletrovate=0;


	int i=0;


	azzera_vetparole();


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


	sprintf (query, "select parola, frequenza, codice, lunghezza from personale where codice like \'%s%%\' union select parola, frequenza, codice, lunghezza from globale where codice like \'%s%%\' order by lunghezza asc, frequenza desc, codice asc limit 0,%d;",codicet9,codicet9,N);

	//printf("\n%s\n",query);


	int  retval = sqlite3_prepare_v2(db,query,-1,&stmt,0);

	if(retval)
	{
        	printf("\nDatabase Error!\n");
        	return;
	}

    	// Read the number of rows fetched
    	int cols = sqlite3_column_count(stmt);


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
				//to manage emphasis char
				//******************************************************************************
				if(lock==1)
				{
					val=(const char*)uppercase((char*)val);
				}
				//******************************************************************************
				printf ("%s",val);
				sprintf(vetparole[numparoletrovate-1].parola,"%s",val);
				
			}
			else if(col==1)
			{
				printf ("\tfr=%s",val);
				vetparole[numparoletrovate-1].frequenza=atoi(val);
			}

		    }

		}
		else
		{
		    // Some error encountered
		    printf("Query Error!\n");
		    return;
		}

    	}

	fflush(stdout);

        tastoprec=0;
	indice=0;

    	printf ("\n");

}




void numerit9(char *parola)
{

	wcarattere[0]= 224;  //à = 224    0xE0
	wcarattere[1]= 232;  //è = 232    0xE8
	wcarattere[2]= 233;  //é = 233    0xE9
	wcarattere[3]= 236;  //ì = 236    0xEC
	wcarattere[4]= 242;  //ò = 242    0xF2
	wcarattere[5]= 249;  //ù = 249    0xF9

	bzero(codicet9,30);

	int lunghezza=strlen(parola);		

	//Associamo ai caratteri i tasti numerici del telecomando

	for(int i=0; i<lunghezza; i++)
	{

		if( (parola[i]== 'a') || (parola[i]== 'b') || (parola[i]== 'c') || (parola[i]== (char)wcarattere[0]) )  //à
			codicet9[i]='2';

		else if( (parola[i]== 'd') || (parola[i]== 'e') || (parola[i]== 'f') || (parola[i]== (char)wcarattere[1]) || (parola[i]== (char)wcarattere[2]) ) //é, è
			codicet9[i]='3';

		else if( (parola[i]== 'g') || (parola[i]== 'h') || (parola[i]== 'i') || (parola[i]== (char)wcarattere[3]) )  //ì
			codicet9[i]='4';

		else if( (parola[i]== 'j') || (parola[i]== 'k') || (parola[i]== 'l') )
			codicet9[i]='5';

		else if( (parola[i]== 'm') || (parola[i]== 'n') || (parola[i]== 'o') || (parola[i]== (char)wcarattere[4]) )  //ò
			codicet9[i]='6';
	
		else if( (parola[i]== 'p') || (parola[i]== 'q') || (parola[i]== 'r') || (parola[i]== 's') )
			codicet9[i]='7';

		else if( (parola[i]== 't') || (parola[i]== 'u') || (parola[i]== 'v') || (parola[i]== (char)wcarattere[5]) )  //ù
			codicet9[i]='8';

		else if( (parola[i]== 'w') || (parola[i]== 'x') || (parola[i]== 'y') || (parola[i]== 'z') )
			codicet9[i]='9';

		printf("%c",codicet9[i]);

	}
	
	//return codice_t9


}





void predire(){
	
	printf("BUFF: %s\n",buf);

	numparoletrovate=0;


	int i=0;


	azzera_vetparole();

	char query[250];
	bzero (query,250);


	sprintf (query, "select parola,frequenza, codice, lunghezza from personale where parola like \'%s%%\' union select parola,frequenza, codice, lunghezza from globale where parola like \'%s%%\' order by lunghezza asc, frequenza desc, parola asc limit 0,%d;",buf,buf,N);

	//printf("\n%s\n",query);


	int  retval = sqlite3_prepare_v2(db,query,-1,&stmt,0);

	if(retval)
	{
        	printf("\nDatabase Error!\n");
        	return;
	}

    	// Read the number of rows fetched
    	int cols = sqlite3_column_count(stmt);


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
				//to manage emphasis char
				//******************************************************************************
				if(lock==1)
				{
					val=(const char*)uppercase((char*)val);
				}
				//******************************************************************************
				printf ("%s",val);
				sprintf(vetparole[numparoletrovate-1].parola,"%s",val);
				
			}
			else if(col==1)
			{
				printf ("\tfr=%s",val);
				vetparole[numparoletrovate-1].frequenza=atoi(val);
			}

		    }

		}
		else
		{
		    // Some error encountered
		    printf("Query Error!\n");
		    return;
		}

    	}

	fflush(stdout);

        tastoprec=0;
	indice=0;

    	printf ("\n");

}



//SELECTIVE MODE FUNCTION
void manuale(int tasto){

					
	stato=1;

	int i;


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
/*
		if (tasto==2)
		{
					
			     if (i==0) {sprintf(vetparole[i].parola, "a"); man_let[i][0] = carattere[1][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "b"); man_let[i][0] = carattere[1][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "c"); man_let[i][0] = carattere[1][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "à"); man_let[i][0] = carattere[1][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "2"); man_let[i][0] = carattere[1][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[1][5]; man_let[i][1]=0;}

		}
*/
		if (tasto==2)
		{
					
			if (i==0) 
			{
				if (lock==0)
					sprintf(vetparole[i].parola, "a");
				else if (lock==1)
					sprintf(vetparole[i].parola, "A");
				man_let[i][0] = carattere[1][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if (lock==0) 
					sprintf(vetparole[i].parola, "b"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "B");
				man_let[i][0] = carattere[1][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "c"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "C");
				man_let[i][0] = carattere[1][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) {sprintf(vetparole[i].parola, "à"); man_let[i][0] = carattere[1][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "2"); man_let[i][0] = carattere[1][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[1][5]; man_let[i][1]=0;}

		}
/*
		else if (tasto==3)
	    	{
			     if (i==0) {sprintf(vetparole[i].parola, "d"); man_let[i][0] = carattere[2][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "e"); man_let[i][0] = carattere[2][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "f"); man_let[i][0] = carattere[2][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "è"); man_let[i][0] = carattere[2][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "3"); man_let[i][0] = carattere[2][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[2][5]; man_let[i][1]=0;}

	    	}
*/
		else if (tasto==3)
	    	{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "d"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "D");
				man_let[i][0] = carattere[2][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "e"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "E");
				man_let[i][0] = carattere[2][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{	
				if(lock==0)
					sprintf(vetparole[i].parola, "f"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "F");
				man_let[i][0] = carattere[2][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) {sprintf(vetparole[i].parola, "è"); man_let[i][0] = carattere[2][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "3"); man_let[i][0] = carattere[2][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[2][5]; man_let[i][1]=0;}

	    	}
/*
		else if (tasto==4)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "g"); man_let[i][0] = carattere[3][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "h"); man_let[i][0] = carattere[3][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "i"); man_let[i][0] = carattere[3][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "ì"); man_let[i][0] = carattere[3][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "4"); man_let[i][0] = carattere[3][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[3][5]; man_let[i][1]=0;}
		}
*/
		else if (tasto==4)
		{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "g"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "G");
				man_let[i][0] = carattere[3][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if (lock==0)
					sprintf(vetparole[i].parola, "h"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "H");
				man_let[i][0] = carattere[3][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "i"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "I");
				man_let[i][0] = carattere[3][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) {sprintf(vetparole[i].parola, "ì"); man_let[i][0] = carattere[3][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "4"); man_let[i][0] = carattere[3][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[3][5]; man_let[i][1]=0;}
		}
/*
		else if (tasto==5)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "j"); man_let[i][0] = carattere[4][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "k"); man_let[i][0] = carattere[4][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "l"); man_let[i][0] = carattere[4][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "5"); man_let[i][0] = carattere[4][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[4][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[4][5]; man_let[i][1]=0;}
		}
*/
		else if (tasto==5)
		{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "j"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "J");
				man_let[i][0] = carattere[4][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "k"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "K"); 
				man_let[i][0] = carattere[4][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "l"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "L"); 
				man_let[i][0] = carattere[4][2]; 		
				man_let[i][1]=0;
			}
			else if (i==3) {sprintf(vetparole[i].parola, "5"); man_let[i][0] = carattere[4][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[4][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[4][5]; man_let[i][1]=0;}
		}
/*
		else if (tasto==6)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "m"); man_let[i][0] = carattere[5][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "n"); man_let[i][0] = carattere[5][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "o"); man_let[i][0] = carattere[5][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "ò"); man_let[i][0] = carattere[5][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "6"); man_let[i][0] = carattere[5][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[5][5]; man_let[i][1]=0;}
		}
*/
		else if (tasto==6)
		{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "m"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "M");
				man_let[i][0] = carattere[5][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "n"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "N"); 
				man_let[i][0] = carattere[5][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "o");
				else if(lock==1)
					sprintf(vetparole[i].parola, "O"); 
				man_let[i][0] = carattere[5][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) {sprintf(vetparole[i].parola, "ò"); man_let[i][0] = carattere[5][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "6"); man_let[i][0] = carattere[5][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[5][5]; man_let[i][1]=0;}
		}
/*
		else if (tasto==7)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "p"); man_let[i][0] = carattere[6][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "q"); man_let[i][0] = carattere[6][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "r"); man_let[i][0] = carattere[6][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "s"); man_let[i][0] = carattere[6][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "7"); man_let[i][0] = carattere[6][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[6][5]; man_let[i][1]=0;}
		}
*/
		else if (tasto==7)
		{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "p"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "P");
				man_let[i][0] = carattere[6][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "q"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "Q");
				man_let[i][0] = carattere[6][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "r"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "R");
				man_let[i][0] = carattere[6][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "s"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "S");
				man_let[i][0] = carattere[6][3]; 
				man_let[i][1]=0;
			}
			else if (i==4) {sprintf(vetparole[i].parola, "7"); man_let[i][0] = carattere[6][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[6][5]; man_let[i][1]=0;}
		}
/*
		else if (tasto==8)
		{
			     if (i==0) {sprintf(vetparole[i].parola, "t"); man_let[i][0] = carattere[7][0]; man_let[i][1]=0;}
			else if (i==1) {sprintf(vetparole[i].parola, "u"); man_let[i][0] = carattere[7][1]; man_let[i][1]=0;}
			else if (i==2) {sprintf(vetparole[i].parola, "v"); man_let[i][0] = carattere[7][2]; man_let[i][1]=0;}
			else if (i==3) {sprintf(vetparole[i].parola, "ù"); man_let[i][0] = carattere[7][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "8"); man_let[i][0] = carattere[7][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[7][5]; man_let[i][1]=0;}
		}
*/
		else if (tasto==8)
		{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "t"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "T");
				man_let[i][0] = carattere[7][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "u"); 
				else if (lock==1)
					sprintf(vetparole[i].parola, "U");
				man_let[i][0] = carattere[7][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "v"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "V");
				man_let[i][0] = carattere[7][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) {sprintf(vetparole[i].parola, "ù"); man_let[i][0] = carattere[7][3]; man_let[i][1]=0;}
			else if (i==4) {sprintf(vetparole[i].parola, "8"); man_let[i][0] = carattere[7][4]; man_let[i][1]=0;}
			else if (i==5) {sprintf(vetparole[i].parola, " "); man_let[i][0] = carattere[7][5]; man_let[i][1]=0;}
		}

		else if (tasto==9)
		{
			if (i==0) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "w"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "W");
				man_let[i][0] = carattere[8][0]; 
				man_let[i][1]=0;
			}
			else if (i==1) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "x"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "X");
				man_let[i][0] = carattere[8][1]; 
				man_let[i][1]=0;
			}
			else if (i==2) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "y"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "Y");
				man_let[i][0] = carattere[8][2]; 
				man_let[i][1]=0;
			}
			else if (i==3) 
			{
				if(lock==0)
					sprintf(vetparole[i].parola, "z"); 
				else if(lock==1)
					sprintf(vetparole[i].parola, "Z");
				man_let[i][0] = carattere[8][3]; 
				man_let[i][1]=0;
			}
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
		
		



	}

	indice=0;




}


//STANDARD MODE FUNCTION WITH PREDICTION
void classico(int tasto, Display* display){


	Window winRoot = XDefaultRootWindow(display);
	Window winFocus;

	int revert;

	int x=0;


	if     (tasto==42)  x=9;
	else if(tasto==163) x=10;
	else    x=tasto-1;
	
	time_t newtime;
	newtime=time(NULL);

	char  *let;
	let = (char*)malloc(2*sizeof(char));

	if (newtime - oldtime < speed && tasto==t_prec){
		
		y_c++;

		if (y_c>0){

        		XGetInputFocus(display, &winFocus, &revert);
			premitasto(display, winFocus, winRoot,XK_BackSpace,modifier);
			cont_char--;

		}
		if (y_c>N-1) 
			y_c=0;

	}
	else {

		y_c=0;

	}


	oldtime=newtime;
	buf[cont_char]=carattere[x][y_c];

	cont_char++;


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

	fflush(stdout);


}



//NUMERIC MODE FUNCTION
void numerico(int tasto){

	Display *display = XOpenDisplay(0);

	Window winRoot = XDefaultRootWindow(display);
	Window winFocus;
	int revert;
	XGetInputFocus(display, &winFocus, &revert);

	//printf("%d\n",tasto);
	printf("\n");

	char  *let;
	let = (char*)malloc(2*sizeof(char));
	sprintf(let,"%d",tasto);


	premitasto(display, winFocus, winRoot,XStringToKeysym(let),modifier);

	XCloseDisplay(display);

}



//MAPPING OF CHARACTERS
void caricamatrice (){

        carattere[0][0] = XK_period;
        carattere[0][1] = XK_comma;
        carattere[0][2] = XK_question;		
        carattere[0][3] = XK_exclam;		
        carattere[0][4] = XK_semicolon;		//for ";" with modifier=1
        carattere[0][5] = XK_period; 		//for ":" with modifier=1
		

        carattere[1][0] = XK_a;
        carattere[1][1] = XK_b;
        carattere[1][2] = XK_c;
        carattere[1][3] = XK_agrave;
        carattere[1][4] = XK_2;
	carattere[1][5] = XK_space;		

        carattere[2][0] = XK_d;
        carattere[2][1] = XK_e;
        carattere[2][2] = XK_f;
        carattere[2][3] = XK_egrave;
        carattere[2][4] = XK_3;
	carattere[2][5] = XK_space;		

        carattere[3][0] = XK_g;
        carattere[3][1] = XK_h;
        carattere[3][2] = XK_i;
        carattere[3][3] = XK_igrave;
        carattere[3][4] = XK_4;
        carattere[3][5] = XK_space;		

        carattere[4][0] = XK_j;
        carattere[4][1] = XK_k;
        carattere[4][2] = XK_l;
        carattere[4][3] = XK_5;
        carattere[4][4] = XK_space;		
        carattere[4][5] = XK_space;		

        carattere[5][0] = XK_m;
        carattere[5][1] = XK_n;
        carattere[5][2] = XK_o;
        carattere[5][3] = XK_ograve;
        carattere[5][4] = XK_6;
        carattere[5][5] = XK_space;		

        carattere[6][0] = XK_p;
        carattere[6][1] = XK_q;
        carattere[6][2] = XK_r;
        carattere[6][3] = XK_s;
        carattere[6][4] = XK_7;
        carattere[6][5] = XK_space;		

        carattere[7][0] = XK_t;
        carattere[7][1] = XK_u;
        carattere[7][2] = XK_v;
        carattere[7][3] = XK_ugrave;
        carattere[7][4] = XK_8;
        carattere[7][5] = XK_space;		

        carattere[8][0] = XK_w;
        carattere[8][1] = XK_x;
        carattere[8][2] = XK_y;
        carattere[8][3] = XK_z;
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
        carattere[10][5] = XK_E;			//for "€" with modifier=XK_Shift_L



}







//Thread IRW
void MyThread::run(void){


  	printf("\nIRW started!\n");

 	int fd,i;
        char buf_thr[128];

        struct sockaddr_un addr;
        addr.sun_family=AF_UNIX;
        strcpy(addr.sun_path,"/dev/lircd");
        fd=socket(AF_UNIX,SOCK_STREAM,0);

        if(fd==-1)  {
                perror("socket");
                exit(errno);
        }

        if(::connect(fd,(struct sockaddr *)&addr,sizeof(addr))==-1)  {
                perror("connect");
                exit(errno);
        }

        char cod[8];
        int lun,j,k;
        for(;;)
	{

                bzero(buf_thr,128);
                memset(cod,0,sizeof(cod));
                i=read(fd,buf_thr,128);

                if(i==-1)  {
                        perror("read");
                        exit(errno);
                };
                if(!i)
                    exit(0);

                j=0;
                k=0;
                lun=strlen(buf_thr);
                for (j=9;j<lun;j++)
                {
                        if (buf_thr[j]==' ') break;
                        cod[k]=buf_thr[j];
                        k=k+1;

                }

                //printf("\nCodice:\t%s\nStringa restituita dal driver: \t%s\n",cod,buf_thr);

                //elabora(cod);


		Display *display = XOpenDisplay(0);

		if(display == NULL) return;


		//BUTTON YELLOW TO ACTIVATE T9 MODE
		if (strcmp(cod, yellow)==0){

			if(t==0){
				t=1;
				emit received(QString::fromUtf8(""),0,4);

			}

			printf("\nT9 ON\n");

			stato=2;

			bzero(codicet9,30);
			predizione=0;

			if(lock==1)

				lbstatus->setText("Status: T9 ABC");


			else if(lock==0)

				lbstatus->setText("Status: T9 abc");


		}

		//BUTTON RED TO ACTIVATE SELECTIVE MODE
		else if (strcmp(cod, red)==0){


			if(t==0){
				t=1;
				emit received(QString::fromUtf8(""),0,4);

			}


			printf("\nSelective\n");

			stato=1;
		
			bzero(codicet9,30);
			predizione=0;

			if(lock==1)

				lbstatus->setText("Status: SELECTIVE");


			else if(lock==0)

				lbstatus->setText("Status: Selective");




		}

		//BUTTON GREEN TO ACTIVATE STANDARD MODE
		else if (strcmp(cod, green)==0){

			azzera_buf();

			if(t==0){
				t=1;
				emit received(QString::fromUtf8(""),0,4);

			}

			printf("\nStandard\n");

			stato=3;

			bzero(codicet9,30);
			predizione=1;

			if(lock==1)

				lbstatus->setText("Status: ABC");


			else if(lock==0)

				lbstatus->setText("Status: abc");


		}

		//BUTTON BLUE TO ACTIVATE NUMERIC MODE
		else if (strcmp(cod, blue)==0){



			if(t==1){
				t=0;
				emit received(QString::fromUtf8(""),0,5);

			}

		
			stato=4;

			printf("\nNumeric\n");

			bzero(codicet9,30);

			lbstatus->setText("Status: Numeric");

			//tray->showMessage ( QString("LiT9"), QString("Nmeric mode"), tray->Information, 3000 );


		}


		//BUTTON POWER TO SHUT DOWN LIT9
	   	else  if (strcmp(cod, power)==0) { 
		
			printf("\nLiT9 stop.\n\n");

			emit received(QString::fromUtf8(""),0,6);


		}

		//BUTTONS ARROW TO MOVE MOUSE AND MOVE CURSOR
	   	else if (strcmp(cod, down)==0) {
			if(nav==0)
				XWarpPointer(display, None, None, 0, 0, 0, 0, 0,passo);      //tasto DOWN
			else if(nav==1)
				tasto=XK_Down;

		}
	   	else if (strcmp(cod, up)==0) {
			if(nav==0)
				XWarpPointer(display, None, None, 0, 0, 0, 0, 0,passo*(-1));   //tasto UP
			else if(nav==1)
				tasto=XK_Up;
		}


	   	else if (strcmp(cod, right)==0) {
			if(nav==0)
				XWarpPointer(display, None, None, 0, 0, 0, 0, passo, 0);    //tasto RIGHT
			else if(nav==1)
				tasto=XK_Right;
		}


	   	else if (strcmp(cod, left)==0) {
			if(nav==0)
				XWarpPointer(display, None, None, 0, 0, 0, 0, passo*(-1),0); //tasto LEFT
			else if(nav==1)
				tasto=XK_Left;
		}


		else if (strcmp(cod, music)==0)
		{
			printf("Prova Alt+Tab\n");			
			//tasto=XK_ISO_Left_Tab;
			tasto = XK_Tab;
			modi=XK_Alt_L;
								
		}

			
		else if (strcmp(cod, guide)==0){

			if(nav==1){	
				nav=0;	//mouse mode	
				emit received(QString::fromUtf8("Motion Mode: Mouse"),0,7);
			}
			else if(nav==0)	{
				nav=1;	//arrows mode
				emit received(QString::fromUtf8("Motion Mode: Arrows"),0,7);
			}

		}


		//BUTTON LOCK
		else if (strcmp(cod, caps)==0){

			if(homepage==0){

				if(lock==1){ //MAIUSCOLO -> minuscolo
			
					lock=0;

					if(stato==1)
					{		
						lbstatus->setText("Status: Selective");
						//***********************************************************************************
						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(""),d,2);

		

						manuale(tasto);


						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(vetparole[d].parola),d,1);
		
						emit received(QString::fromUtf8(""),0,3);

						//***********************************************************************************
					}
					if(stato==2)
					{
						lbstatus->setText("Status: T9 abc");

						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(""),d,2);
						
						//codicet9[luncodicet9]='\0'; 
						if (luncodicet9>0){luncodicet9--; codicet9[luncodicet9]='\0';gestionet9(tasto);}
						else {bzero(codicet9,30);}

						


						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(vetparole[d].parola),d,1);
				
						emit received(QString::fromUtf8(""),0,3);

					}
					if(stato==3)
							lbstatus->setText("Status: abc");


				}
				else if(lock==0){ //minuscolo -> MAIUSCOLO

					lock=1;

					if(stato==1)
					{	
						lbstatus->setText("Status: SELECTIVE");
						//***********************************************************************************

						for (int d=0;d<N;d++) emit received(QString::fromUtf8(""),d,2);
						manuale(tasto);
						for (int d=0;d<N;d++) emit received(QString::fromUtf8(vetparole[d].parola),d,1);
						emit received(QString::fromUtf8(""),0,3);

						//***********************************************************************************
					}
					if(stato==2)
					{
						lbstatus->setText("Status: T9 ABC");
						for (int d=0;d<N;d++) emit received(QString::fromUtf8(""),d,2);
						if (luncodicet9>0){luncodicet9--; codicet9[luncodicet9]='\0';gestionet9(tasto);}
						else {bzero(codicet9,30);
						}
						
						for (int d=0;d<N;d++) emit received(QString::fromUtf8(vetparole[d].parola),d,1);
						emit received(QString::fromUtf8(""),0,3);


					}
					if(stato==3)
							lbstatus->setText("Status: ABC");

				}

			}

		}


		//TRAY-ICON
	    	else if (strcmp(cod, Record)==0){


			if (t==0){

		        	printf("Show window.\n");

				emit received(QString::fromUtf8(""),0,4);

				t=1;

			}
			else{

		        	printf("Hide window.\n");

				emit received(QString::fromUtf8(""),0,5);

				t=0;

			}


		}


		//BUTTON CH+ and CH- to select items into the GUI
	   	else if (strcmp(cod, ch_minus)==0)
	   	{

			if(homepage==0){
				if(stato==2 || stato==3){
					//if (indice==(N-1)){
					if (indice==(numparoletrovate-1)){

						emit received(QString::fromUtf8(""),indice,2);
						emit received(QString::fromUtf8(""),0,3);

						indice=0;
					}
					else{
						indice=indice+1;
						emit received(QString::fromUtf8(""),indice-1,2);
						emit received(QString::fromUtf8(""),indice,3);
					}
				}
				else if(stato==1){
					if (indice==(N-1)){
				
						emit received(QString::fromUtf8(""),indice,2);
						emit received(QString::fromUtf8(""),0,3);

						indice=0;
					}
					else{
						indice=indice+1;
						emit received(QString::fromUtf8(""),indice-1,2);
						emit received(QString::fromUtf8(""),indice,3);


					}
				}
				


			}
	   	}

	   	else  if (strcmp(cod, ch_plus)==0)
	   	{

			if(homepage==0){
		       		if (indice==0){
					if (stato==1)indice=N-1;
					else indice=numparoletrovate-1;
					emit received(QString::fromUtf8(""),0,2);
					emit received(QString::fromUtf8(""),indice,3);

				}
		       		else{
					indice = indice-1;
					emit received(QString::fromUtf8(""),indice+1,2);
					emit received(QString::fromUtf8(""),indice,3);

				}
			}

	   	}



		//BUTTON "Home" to start thread of browser
	   	else  if (strcmp(cod, home)==0)
		{
			int res;
			pthread_t tel_thread;
			res = pthread_create(&tel_thread, NULL, apribrowser, NULL);
			if (res != 0) {
				exit(EXIT_FAILURE);
			 	printf("\nStart thread error!\n");

			}

		}


		//BUTTON TO SEND SELECTED WORD ON GUI TO WINDOW WITH FOCUS
		else if (strcmp(cod, tasto_exit)==0) { //BUTTON MUTE

			if(homepage==0){

				if (predizione==1 && cont_char > 0){

					Window winRoot = XDefaultRootWindow(display);
					Window winFocus;
					int revert;
					XGetInputFocus(display, &winFocus, &revert);

					for (int g=0; g<cont_char ;g++) 
						premitasto(display, winFocus, winRoot,XK_BackSpace,modifier);

				}

				invio_parola(display);

				if (predizione==1)
				{		
				azzera_buf();
				}

			}

		}



		//Button "OK" -> mouse left click
	   	else if (strcmp(cod, ok)==0)
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

			//XSetInputFocus(display, PointerRoot ,PointerRoot, CurrentTime);


		}


		//
	   	else if (strcmp(cod, picture)==0){

			// 1 inserimento nel DB / 0 modalità inserimento	

			int cc=0;
			stato=3;

			if(inserimento==0){ //devo digitare una parola

				azzera_buf();
				printf("\nDigit new word:\n");
				emit received(QString::fromUtf8("Digit new word"),0,8);
				
				inserimento=1;
				
			}
			else if(inserimento==1){ //inserisco la parola nel DB

				
				if(cont_char!=0){

					char query2[250];
					bzero (query2,250);

					sprintf (query2, "select COUNT(*) from (select parola from globale where parola like \'%s\' union select parola from personale where parola like \'%s\') ",buf);


					int retval = sqlite3_prepare_v2(db,query2,-1,&stmt,0);

					if(retval)
					{
						printf("\nDatabase Error!\n");
						emit received(QString::fromUtf8("Database Error!"),0,8);
						
					}

    	
			    		int cols = sqlite3_column_count(stmt);


					// fetch a row's status
					retval = sqlite3_step(stmt);

					if(retval == SQLITE_DONE) break;

					else if(retval == SQLITE_ROW)
					{

						//const char *val = (const char*)sqlite3_column_text(stmt,0);

							cc = atoi((const char*)sqlite3_column_text(stmt,0));

						//sprintf(vetparole[numparoletrovate-1].parola,"%s",val);

					}
					else
					{
					    // Some error encountered
					    printf("Query Error!\n");
					    return;
					}

printf("\n%d\n",cc);


				   if(cc==0){
				
					printf("\nWord convertion.\n");
					printf("\n-T9 code genereted: ");
					numerit9(buf);
					printf("\n");
				
					char query[250];
					bzero (query,250);

					sprintf (query, "insert into personale (codice,parola,frequenza,lunghezza) values (\'%s\',\'%s\',\'%d\',\'%d\');",codicet9,buf,5,cont_char-1);

					int retval = sqlite3_exec(db,query,0,0,0);

					if(retval)
					{
						printf("\nDatabase Error! or Word duplicated into database!\n");
						emit received(QString::fromUtf8("Word duplicated!"),0,8);
						//return;
					}
					else{
						printf("\nNew word inserted!\n");
						emit received(QString::fromUtf8("New: ")+ QString::fromUtf8(buf),0,8);

					}

					azzera_buf();
					inserimento=0;

				   }
				   else{

					printf("\nWord duplicated!\n");
					emit received(QString::fromUtf8("Word duplicated!"),0,8);
					azzera_buf();
					inserimento=0;
			           }

				

				}
				else{

					printf("\nEmpty word!\n");
					emit received(QString::fromUtf8("Empty word!"),0,8);
					azzera_buf();
					inserimento=0;

				}

		

			}

	

		}



		//ALL FUNCTIONS ASSOCIATED TO THE PRESSURE OF A NUMERIC BUTTON
		else if ( (strcmp(cod, tasto_1)==0 && stato==4) || strcmp(cod, tasto_2)==0 ||  strcmp(cod, tasto_3)==0 || strcmp(cod, tasto_4)==0 || strcmp(cod, tasto_5)==0 || strcmp(cod, tasto_6)==0 || strcmp(cod, tasto_7)==0 || strcmp(cod, tasto_8)==0 || strcmp(cod, tasto_9)==0) {

			homepage=0;

			if(firsttime==1){


				firsttime=0;

				array[0]->setAlignment(Qt::AlignLeft);
				array[0]->setFont(QFont("Verdana",10));

				QPalette palette;
				QBrush brush(QColor(0, 0, 0, 255));
				//brush.setStyle(Qt::SolidPattern);
				palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
				palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);

				for (int d=1;d<N;d++) {
					array[d]->setFont(QFont("Verdana",10));
					array[d]->setPalette(palette);
				}


			}

			// se non ho premuto un tasto o se si tratta di un tasto numerico
			     if (strcmp(cod, tasto_1)==0) tasto=1;
			else if (strcmp(cod, tasto_2)==0) tasto=2;
			else if (strcmp(cod, tasto_3)==0) tasto=3;
			else if (strcmp(cod, tasto_4)==0) tasto=4;
			else if (strcmp(cod, tasto_5)==0) tasto=5;
			else if (strcmp(cod, tasto_6)==0) tasto=6;
			else if (strcmp(cod, tasto_7)==0) tasto=7;
			else if (strcmp(cod, tasto_8)==0) tasto=8;
			else if (strcmp(cod, tasto_9)==0) tasto=9;


		

			switch(stato){

				case 1://SELECTIVE Mode. 

					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(""),d,2);

				
					manuale(tasto);


					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(vetparole[d].parola),d,1);
				
					emit received(QString::fromUtf8(""),0,3);


				break;


				case 2://T9 Mode.
	/*
					gestionet9(tasto);

					if(numparoletrovate==0 && refresh==1) 
					{
						refresh=0;
						for (int d=0;d<N;d++) {
							array[d]->setStyleSheet( "background-color: white" );
						}
						for (int d=0;d<N;d++) {
							array[d]->setText(QString(""));
						}
					}
					else if(numparoletrovate > 0)
						refresh=1;



					if(refresh){


						for (int d=0;d<N;d++) {
							array[d]->setStyleSheet( "background-color: white" );
						}


						for (int d=0;d<N;d++) {
							array[d]->setText(QString::fromUtf8(vetparole[d].parola));
						}

						array[0]->setStyleSheet( "background-color: rgb( 0,255,0 )" );

					}
	*/


					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(""),d,2);

				

					gestionet9(tasto);


					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(vetparole[d].parola),d,1);
				
					emit received(QString::fromUtf8(""),0,3);



				break;


				case 3://STANDARD with Prediction Mode.


					classico(tasto,display);



					if (predizione==1 && cont_char>pred){

						predire();

						if(numparoletrovate==0 && refresh==1) 
						{
							refresh=0;

							for (int d=0;d<N;d++) 
								emit received(QString::fromUtf8(""),d,2);

							for (int d=0;d<N;d++) 
								emit received(QString::fromUtf8(""),d,1);

						}
						else if(numparoletrovate > 0)
							refresh=1;
					
					
					}

					if(refresh){


						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(""),d,2);
					


						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(vetparole[d].parola),d,1);
					

						emit received(QString::fromUtf8(""),0,3);

					}
	

		/*			

					//if (predizione==1 && cont_char>pred)
						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(""),d,2);

		


					classico(tasto,display);

					if (predizione==1 && cont_char>pred)
						predire();
		
				

					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(vetparole[d].parola),d,1);


					emit received(QString::fromUtf8(""),0,3);

	*/
		


				break;

				case 4://NUMERIC Mode
				
					numerico(tasto);
				break;

			}

		}


		else if (strcmp(cod, tasto_1)==0) {

			tasto=1;

			homepage=0;

			if(firsttime==1){


				firsttime=0;

				array[0]->setAlignment(Qt::AlignLeft);
				array[0]->setFont(QFont("Verdana",10));

				QPalette palette;
				QBrush brush(QColor(0, 0, 0, 255));
				//brush.setStyle(Qt::SolidPattern);
				palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
				palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);

				for (int d=1;d<N;d++) {
					array[d]->setFont(QFont("Verdana",10));
					array[d]->setPalette(palette);
				}


			}


			if(stato==1){

				for (int d=0;d<N;d++) 
					emit received(QString::fromUtf8(""),d,2);
			

				manuale(tasto);

				for (int d=0;d<N;d++) 
					emit received(QString::fromUtf8(vetparole[d].parola),d,1);
				

				emit received(QString::fromUtf8(""),0,3);

			
			}
			else
				classico(tasto,display);



		}

		else if (strcmp(cod, star)==0 ) {

			tasto=42;

			int d=0;

			if(stato==1){

				for (d=0;d<N;d++) 
					emit received(QString::fromUtf8(""),d,2);
			

				manuale(tasto);

				for (d=0;d<N;d++)
					emit received(QString::fromUtf8(vetparole[d].parola),d,1);
			

				emit received(QString::fromUtf8(""),0,3);

			}
			else
				classico(tasto,display);



		}

		else if (strcmp(cod, hash)==0 ) {

			tasto=163;


			int d=0;

			if(stato==1){

				for (d=0;d<N;d++) 
					emit received(QString::fromUtf8(""),d,2);
			

				manuale(tasto);

				for (d=0;d<N;d++)
					emit received(QString::fromUtf8(vetparole[d].parola),d,1);
			

				emit received(QString::fromUtf8(""),0,3);

			}
			else
				classico(tasto,display);



		}



		//Buttons for computer interactions: TAB and BACK_TAB ;BACKSPACE; ENTER; PAG_UP and PAG_DOWN; etc...
		else {

			if (strcmp(cod, videos)==0) tasto=XK_Tab;
			else if (strcmp(cod, mytv)==0) {tasto=XK_Tab; modifier=1;}
	   		else if (strcmp(cod, enter)==0) {tasto =XK_Return;
				if (stato==3){
					azzera_buf();
				}
			}    
			else if (strcmp(cod, vol_plus)==0) tasto=XK_Page_Up;  	//Button Vol+
	   		else if (strcmp(cod, vol_minus)==0) tasto =XK_Page_Down;  	//Button Vol-


		
			//Button space.
	  		else if (strcmp(cod, tasto_0)==0) {

				if(stato==1 || stato==2) tasto = XK_space;

				else if(stato==3) {

					tasto = XK_space; 

					azzera_buf();

				}

		    		else if(stato==4) tasto = XK_0;




			}


			//Button backspace.
	   		else if (strcmp(cod, mute_clear)==0)
			{
			  
			  
				if(stato==2 && luncodicet9>0){

					//codicet9[luncodicet9]='\0';
					codicet9[luncodicet9-1]='\0';
					luncodicet9 --;

					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(""),d,2);

					if (luncodicet9>0){

						numparoletrovate=0;
					
						char query[250];
						bzero (query,250);


		  sprintf (query, "select parola,frequenza, codice, lunghezza from personale where codice like \'%s%%\' union select parola,frequenza, codice,lunghezza from globale where codice like \'%s%%\' order by lunghezza asc,frequenza desc, codice asc limit 0,%d;",codicet9,codicet9,N);


						int  retval = sqlite3_prepare_v2(db,query,-1,&stmt,0);

						if(retval)
						{
							printf("\nDatabase Error: searching in tables!\n");
							return;
						}

					    	// Read the number of rows fetched
					    	int cols = sqlite3_column_count(stmt);


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
									//to manage emphasis char
									//******************************************************************************
									if(lock==1)
									{
										val=(const char*)uppercase((char*)val);
									}
									//******************************************************************************
									printf ("%s",val);
									sprintf(vetparole[numparoletrovate-1].parola,"%s",val);
				
								}
								else if(col==1)
								{
									printf ("\tfr=%s",val);
									vetparole[numparoletrovate-1].frequenza=atoi(val);
								}

							    }

							}
							else
							{
							    // Some error encountered
							    printf("Query Error!\n");
							    return;
							}




						}

						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(vetparole[d].parola),d,1);

						emit received(QString::fromUtf8(""),0,3);


					}


				}




				if(stato==2 && luncodicet9==0) tasto = XK_BackSpace;
				
			
				if(predizione==1 && stato==3 && cont_char>0){
printf("BUFF da clear: %s\n",buf);

					//int l=strlen(buf);
					//printf("Prima: %s\n",buf);
					buf[cont_char]='\0';
					buf[cont_char-1]='\0';
					//printf("Dopo: %s\n",buf);
					cont_char--;

					for (int d=0;d<N;d++) 
						emit received(QString::fromUtf8(""),d,2);

					if (cont_char>pred){

						predire();

						for (int d=0;d<N;d++) 
							emit received(QString::fromUtf8(vetparole[d].parola),d,1);

					}
					
					tasto = XK_BackSpace;

				}






			}


			//BUTTON PRESS EVENT*************************************************************

				// Get the root window for the current display.
				Window winRoot = XDefaultRootWindow(display);

				// Find the window which has the current keyboard focus.
				Window winFocus;
				int revert;
				XGetInputFocus(display, &winFocus, &revert);

				premitasto(display, winFocus, winRoot,tasto,modifier);


			//*******************************************************************************

			
		}

		if (strcmp(cod, mytv)==0) modifier=0;	//for backtab button


		if(nav==1){												

				Window winRoot = XDefaultRootWindow(display);
				Window winFocus;
				int revert;
				XGetInputFocus(display, &winFocus, &revert);
				premitasto(display, winFocus, winRoot,tasto,modifier);

		}

	
		if (strcmp(cod, music)==0){
			Window winRoot = XDefaultRootWindow(display);
			Window winFocus;
			int revert;
			XGetInputFocus(display, &winFocus, &revert);
			premitasto(display, winFocus, winRoot,tasto,modi);
		}
	

		XCloseDisplay(display);


	}


}


void MyThread::esegui(QString msg , int i, int mod)
{


//mod=1 -> set text
//mod=2 -> set white
//mod=3 -> set green
//mod=4 -> show
//mod=5 -> hide
//mod=6 -> exit
//mod=7 -> label motion 
//mod=8 -> label status



if(mod==1)
	array[i]->setText(msg);

if(mod==2)
	array[i]->setStyleSheet( "background-color: white" );

if(mod==3)
	array[i]->setStyleSheet( "background-color: rgb( 0,255,0 )" );


if(mod==4)
	window->show();

if(mod==5)
	window->hide();

if(mod==6)
	window->close();

if(mod==7)
	lbmotion->setText(msg);

if(mod==8) 
	lbstatus->setText(msg);


}









//Function to connect to DB
void connDB (){

	int rc;

	if (!strcmp(dictionary,"IT"))
		rc = sqlite3_open("words_IT.sqlite", &db);
	else
		rc = sqlite3_open("words_EN.sqlite", &db);

        if (rc)
 	{
 	       	printf("\nDatabase Error!");
	       	exit(EXIT_FAILURE);

        }
        else
	{
		printf("\nDatabase connection: OK!\n");
		
	}
	fflush(stdout);

}



int main( int argc, char *argv[])
{
	
	QApplication app(argc, argv);

	
	//loading configuration
	caricaconfig();

	QVBoxLayout layout;
	layout.setSpacing(2);


    	MyThread thread;


	window = new QWidget();
	window->setWindowTitle(QString("LiT9"));
	window->setLayout(&layout);
	window->setMinimumSize(185, 185);
	window->move(wx,wy);
	window->setWindowIcon(QIcon("./icon.png"));
	window->setWindowFlags(Qt::SplashScreen);  		//togliamo il bordo
	window->setWindowFlags(Qt::WindowStaysOnTopHint);

	

	//GESTIONE DELLA TRAY ICON-----------------------------------------------------------------------------

		//creo la tray icon
		tray = new QSystemTrayIcon(QIcon("./icon.png"));
		tray->show();
		//tray->showMessage ( QString("LiT9"), QString("Attivato"), tray->Information, 3000 );


		//creo il tray icon menù
		QMenu tray_icon_menu;
		//file.addMenu(QString("Submenu"));
		//tray_icon_menu.addAction(QString("Quit"));
		tray->setContextMenu (&tray_icon_menu);


		//creo l'elemento del menu
		QAction *view_action = tray_icon_menu.addAction(QString("View"));
		QAction *hide_action = tray_icon_menu.addAction(QString("Hide"));
		//QAction *config_action = tray_icon_menu.addAction(QString("Config"));
		QAction *quit_action = tray_icon_menu.addAction(QString("Quit\tAlt+F4"));
		//QAction *about_action = tray_icon_menu.addAction(QString("About"));

		quit_action->setIcon (QIcon("./gnome_logout.png"));
		//config_action->setIcon (QIcon("./icon_setting.png"));

		//associo le azioni ad un elemento del menu
		QObject::connect(view_action, SIGNAL(triggered()), window, SLOT( showNormal() ));
		QObject::connect(hide_action, SIGNAL(triggered()), window, SLOT( hide() ));
		//QObject::connect(config_action, SIGNAL(triggered()),0, SLOT( esegui(QString(""),0,8) ));
		QObject::connect(quit_action, SIGNAL(triggered()), &app, SLOT( quit() ));
		//QObject::connect(about_action, SIGNAL(triggered()), window, SLOT( tray_about() ));


	
	//-----------------------------------------------------------------------------------------------------




	azzera_buf();


	//loading matrix of characters
	caricamatrice();

	

	printf("\nConfiguration loaded.\n");
	printf("- Mouse step motion: %d\n",passo);
	printf("- Speed text entry: %f\n",speed);
	printf("- Window position: (%d , %d) pixel.\n",wx,wy);
	printf("- Number of char to start prediction: %d\n",pred);
	printf("- Dictionary: %s\n",dictionary);


	//INIT VECTOR OF CHAR IN SELECTIVE MODE
	int i=0, j=0;
	for(i=0; i<N; i++)
		for(j=0; j < 2; j++)
			man_let[i][j]=0;


	//Start database connection
	connDB();	


	//QLabel lbstatus("Status: Standard");
	lbstatus= new QLabel("Status: abc");

	if(nav==0)
		lbmotion= new QLabel("Motion Mode: Mouse");
	else if(nav==1)
		lbmotion= new QLabel("Motion Mode: Arrows");



	QPalette paletteG;
	QBrush brushG(QColor(0, 255, 0, 255));
	//brushG.setStyle(Qt::SolidPattern);
	paletteG.setBrush(QPalette::Active, QPalette::WindowText, brushG);
	paletteG.setBrush(QPalette::Inactive, QPalette::WindowText, brushG);

	QPalette paletteB;
	QBrush brushB(QColor(0, 0, 255, 255));
	//brushB.setStyle(Qt::SolidPattern);
	paletteB.setBrush(QPalette::Active, QPalette::WindowText, brushB);
	paletteB.setBrush(QPalette::Inactive, QPalette::WindowText, brushB);

	QPalette paletteR;
	QBrush brushR(QColor(255, 0, 0, 255));
	//brushR.setStyle(Qt::SolidPattern);
	paletteR.setBrush(QPalette::Active, QPalette::WindowText, brushR);
	paletteR.setBrush(QPalette::Inactive, QPalette::WindowText, brushR);

	QPalette paletteY;
	QBrush brushY(QColor(255, 230, 0, 255));
	//brushY.setStyle(Qt::SolidPattern);
	paletteY.setBrush(QPalette::Active, QPalette::WindowText, brushY);
	paletteY.setBrush(QPalette::Inactive, QPalette::WindowText, brushY);





	char str[50];

	for (int i=0; i<N; i++)
	{
	
		if(i==0)
			sprintf(str,"Welcome in LiT9!");
		if(i==1)
			sprintf(str,"      R - Selective");	  
		if(i==2)
			sprintf(str,"      G - Standard");
		if(i==3)
			sprintf(str,"      Y - T9");
		if(i==4)
			sprintf(str,"      B - Numeric");	

		if(i==5)
			sprintf(str," ");		

		array[i]= new QLabel(str);



		if(i==0){

			array[i]->setAlignment(Qt::AlignCenter); 
			array[i]->setFont(QFont("Verdana",10,QFont::Bold));
			
		}

		if(i==1){

			array[i]->setFont(QFont("Verdana",10,QFont::Bold));
			array[i]->setPalette(paletteR);
		}

			
		if(i==2){
			array[i]->setFont(QFont("Verdana",10,QFont::Bold));
			array[i]->setPalette(paletteG);
		}

		if(i==3){
			array[i]->setFont(QFont("Verdana",10,QFont::Bold));
			array[i]->setPalette(paletteY);
		}

		if(i==4){
			array[i]->setFont(QFont("Verdana",10,QFont::Bold));
			array[i]->setPalette(paletteB);
		}

		if(i==5){
			array[i]->setFont(QFont("Verdana",10,QFont::Bold));
			array[i]->setPalette(paletteB);
		}

		
		layout.addWidget(array[i]);

	}

	layout.addWidget(lbstatus);
	lbstatus->setAlignment(Qt::AlignCenter); 
	lbstatus->setFont(QFont("Verdana",10,QFont::Bold));

	layout.addWidget(lbmotion);
	lbmotion->setAlignment(Qt::AlignCenter); 
	lbmotion->setFont(QFont("Verdana",10,QFont::Bold));



	window->show();


	//QObject::connect (&thread, SIGNAL(finished()), qApp, SLOT(quit()));
	QObject::connect (&thread, SIGNAL(received(QString, int, int)), &thread, SLOT(esegui(QString,int,int)));


    	thread.start();
 
	return app.exec();




}
