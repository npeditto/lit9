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


//To compile
//g++ litgtk_t9_list.cpp -o litgtk_t9_list -lX11 -lpthread `pkg-config --cflags --libs gtk+-2.0`

#include "main.h"


//Funzione per la gestione degli eventi associati alla pressione dei tasti
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

//Funzine per lo scorrimento circolare della listbox
void scorri ()
{
    if (indice==(N-1))
        indice=0;
    else
        indice = indice+1;

    gtk_list_select_item((GtkList *) gtklist,indice);
    gdk_window_process_all_updates ();

}


//Funzione per avviare il browser
void *apribrowser(void *arg)
{
    system("google-chrome");
}


//Funzione di mappatura dei tasti tramite KeySyms
void caricamatrice ()
{
    matrice[0][0]=XK_A;
    matrice[0][1]=XK_B;
    matrice[0][2]=XK_C;
    matrice[0][3]=XK_agrave;
    matrice[0][4]=XK_2;
    matrice[1][0]=XK_D;
    matrice[1][1]=XK_E;
    matrice[1][2]=XK_F;
    matrice[1][3]=XK_egrave;
    matrice[1][4]=XK_3;
    matrice[2][0]=XK_G;
    matrice[2][1]=XK_H;
    matrice[2][2]=XK_I;
    matrice[2][3]=XK_igrave;
    matrice[2][4]=XK_4;
    matrice[3][0]=XK_J;
    matrice[3][1]=XK_K;
    matrice[3][2]=XK_L;
    matrice[3][3]=XK_5;
    matrice[3][4]=XK_0;
    matrice[4][0]=XK_M;
    matrice[4][1]=XK_N;
    matrice[4][2]=XK_O;
    matrice[4][3]=XK_ograve;
    matrice[4][4]=XK_6;
    matrice[5][0]=XK_P;
    matrice[5][1]=XK_Q;
    matrice[5][2]=XK_R;
    matrice[5][3]=XK_S;
    matrice[5][4]=XK_7;
    matrice[6][0]=XK_T;
    matrice[6][1]=XK_U;
    matrice[6][3]=XK_ugrave;
    matrice[6][2]=XK_V;
    matrice[6][4]=XK_8;
    matrice[7][0]=XK_W;
    matrice[7][1]=XK_X;
    matrice[7][2]=XK_Y;
    matrice[7][3]=XK_Z;
    matrice[7][4]=XK_9;
}



//Funzione per il trasferimento di un'intera parola dalla listbox alla texbox di destinazione
void parola_t9 ()
{

    //printf("parola_listbox: %s", parole[indice+1]);

    int dim_parola = strlen(parole[indice+1]);

    Display *display = XOpenDisplay(0);
    Window winRoot = XDefaultRootWindow(display);
    Window winFocus;
    int revert;
    XGetInputFocus(display, &winFocus, &revert);

    for (int kk=0; kk < dim_parola; kk++)
    {

        gchar word[dim_parola];
        sprintf(word,"%s",parole[indice+1]);

        gchar  *let;
        let = (gchar*)malloc(sizeof(gchar));
        sprintf(let,"");

        sprintf(let,"%c",word[kk]);
        //printf("%s", let);


        XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, XStringToKeysym(let), 0);
        XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

        event = createKeyEvent(display, winFocus, winRoot, false, XStringToKeysym(let), 0);
        XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
        /**/
    }
    printf("\n");

    XCloseDisplay(display);


}




//Funzione per l'inserimento manuale delle lettere selezionate nella listbox in una textbox selezionata
void tappa()
{

    if (tastocor==0)
        return;

    int tasto=matrice[tastocor-2][indice];

    Display *display = XOpenDisplay(0);

    if(display == NULL)
        return;

    Window winRoot = XDefaultRootWindow(display);

    // Find the window which has the current keyboard focus.
    Window winFocus;
    int revert;
    XGetInputFocus(display, &winFocus, &revert);

    // Send a fake key press event to the window.
    XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, tasto, 0);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

    // Send a fake key release event to the window.
    event = createKeyEvent(display, winFocus, winRoot, false, tasto, 0);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);


    XCloseDisplay(display);

}


//Algoritmo T9 di predizione del testo basato su liste
void gestionet9 (int tasto)
{


    sprintf(codicet9,"%s%d",codicet9,tasto);
    printf("\nTasti premuti: %s\n",codicet9);
    luncodicet9=luncodicet9+1;
    struct nodo *t=comodo;			//lista di reinizializzazione
    int trovo=0;

    GtkWidget       *list_item;
    GList           *dlist;
    guint           i;
    gchar           buffer[64];
    gtk_list_clear_items ((GtkList *) gtklist,0,N);
    dlist=NULL;
    gchar  *str;
    str = (gchar*)malloc(sizeof(gchar));
    sprintf(str,"");



    while (t!=NULL && trovo <5)
    {
        if (strncmp(codicet9, t->codice, luncodicet9)==0)
        {
            trovo=trovo+1;
            printf("%s\n",t->parola);

            parole[trovo]=t->parola;  //riempo l'array parole con quelle della listbox

            if (trovo==1) comodo=t;
            list_item=gtk_list_item_new_with_label(t->parola);
            dlist=g_list_append(dlist, list_item);
            gtk_widget_show(list_item);
            gtk_object_set_data(GTK_OBJECT(list_item), list_item_data_key,str);

        }
        t=t->next;
    }


    if(trovo > 0)
    {
        Display *display = XOpenDisplay(0);
        gtk_list_append_items((GtkList*)(gtklist), dlist);
        indice=0;
        gtk_list_select_item((GtkList *) gtklist,indice);
        XWarpPointer(display, None, None, 0, 0, 0, 0, -10000,-10000);
        XWarpPointer(display, None, None, 0, 0, 0, 0, 90, 0);
        gdk_window_process_all_updates ();
        XCloseDisplay(display);

    }

}


//Funzione riempimento listbox modalità inserimento testo manuale
void caricalist (int tasto, Display* display)
{

    if (statot9 == 1)
        gestionet9(tasto);
    else
    {
        if (tasto == tastoprec)
            scorri();
        else
        {
            GtkWidget 	*list_item;
            GList           *dlist;
            guint           i;
            gchar           buffer[64];
            gtk_list_clear_items ((GtkList *) gtklist,0,N);

            dlist=NULL;
            gchar  *str;

            str = (gchar*)malloc(sizeof(gchar));
            sprintf(str,"");
            tastocor=tasto;

            //con questo for infunzione del tasto numerico premuto caricheremo la lista
            //con le corrispondenti lettere associate al tasto numerico
            for (i=0; i<N; i++)
            {
                if (tasto ==2)
                {
                    if (i==0) sprintf(buffer, "a");
                    else if (i==1) sprintf(buffer, "b");
                    else if (i==2) sprintf(buffer, "c");
                    else if (i==3) sprintf(buffer, "à");
                    else if (i==4) sprintf(buffer, "2");
                }
                else if (tasto ==3)
                {
                    if (i==0) sprintf(buffer, "d");
                    else if (i==1) sprintf(buffer, "e");
                    else if (i==2) sprintf(buffer, "f");
                    else if (i==3) sprintf(buffer, "è");
                    else if (i==4) sprintf(buffer, "3");
                }
                else if (tasto ==4)
                {
                    if (i==0) sprintf(buffer, "g");
                    else if (i==1) sprintf(buffer, "h");
                    else if (i==2) sprintf(buffer, "i");
                    else if (i==3) sprintf(buffer, "ì");
                    else if (i==4) sprintf(buffer, "4");
                }
                else if (tasto ==5)
                {
                    if (i==0) sprintf(buffer, "j");
                    else if (i==1) sprintf(buffer, "k");
                    else if (i==2) sprintf(buffer, "l");
                    else if (i==3) sprintf(buffer, "5");
                    else if (i==4) sprintf(buffer, "0");
                }
                else if (tasto ==6)
                {
                    if (i==0) sprintf(buffer, "m");
                    else if (i==1) sprintf(buffer, "n");
                    else if (i==2) sprintf(buffer, "o");
                    else if (i==3) sprintf(buffer, "ò");
                    else if (i==4) sprintf(buffer, "6");
                }
                else if (tasto ==7)
                {
                    if (i==0) sprintf(buffer, "p");
                    else if (i==1) sprintf(buffer, "q");
                    else if (i==2) sprintf(buffer, "r");
                    else if (i==3) sprintf(buffer, "s");
                    else if (i==4) sprintf(buffer, "7");
                }
                else if (tasto ==8)
                {
                    if (i==0) sprintf(buffer, "t");
                    else if (i==1) sprintf(buffer, "u");
                    else if (i==2) sprintf(buffer, "v");
                    else if (i==3) sprintf(buffer, "ù");
                    else if (i==4) sprintf(buffer, "8");
                }
                else if (tasto ==9)
                {
                    if (i==0) sprintf(buffer, "w");
                    else if (i==1) sprintf(buffer, "x");
                    else if (i==2) sprintf(buffer, "y");
                    else if (i==3) sprintf(buffer, "z");
                    else if (i==4) sprintf(buffer, "9");
                }

                //creiamo l'elemento della lista ad ogni ciclo
                list_item=gtk_list_item_new_with_label(buffer);
                dlist=g_list_append(dlist, list_item);
                gtk_widget_show(list_item);
                gtk_object_set_data(GTK_OBJECT(list_item), list_item_data_key,str);

            }//chiusura del for

            tastoprec=tasto;

            //associamo gli elemneti della lista appena creati dal for al widget della listbox
            gtk_list_append_items((GtkList*)(gtklist), dlist);
            indice=0;
            gtk_list_select_item((GtkList *) gtklist,indice);
            //riposizioniamo il puntatore del mouse sulla listbox
            XWarpPointer(display, None, None, 0, 0, 0, 0, -10000,-10000);
            XWarpPointer(display, None, None, 0, 0, 0, 0, 90, 0);
            //facciamo il refresh della finestra
            gdk_window_process_all_updates ();

        }//chiusura else

    }

}



void elabora(char *codice)
{
    gint x=0, y=0, passo=5;
    Display *display = XOpenDisplay(0);
    if(display == NULL)
        return;

    gchar  *str;
    str = (gchar*)malloc(sizeof(gchar));
    sprintf(str,"");

    //tasto yellow Attivazione/Disattivazione T9
    if (strcmp(codice, yellow)==0)
    {
        if (statot9==0)
        {
            if (flagcaricat9==1)
            {
                statot9=1;
                comodo=lista;
                sprintf(codicet9,"");
                luncodicet9 = 0;
                printf("\nT9 attivo\n");
            }
        }
        else
        {
            statot9=0;
            printf("\nT9 disattivato");
        }
    }

    //Tasti CH+ e CH- per lo scorrimento della listbox
    else if (strcmp(codice, ch_minus)==0)
    {
        scorri();
    }
    else  if (strcmp(codice, ch_plus)==0)
    {
        if (indice==0)
            indice=N-1;
        else
            indice = indice-1;

        gtk_list_select_item((GtkList *) gtklist,indice);
        gdk_window_process_all_updates ();
    }

    //Tasto Power per uscire dal programma
    else  if (strcmp(codice, power)==0) exit(0);

    //Tasto Blue per lanciare il thread del browser
    else  if (strcmp(codice, blue)==0)
    {
        int res;
        pthread_t tel_thread;
        res = pthread_create(&tel_thread, NULL, apribrowser, NULL);
        if (res != 0)
        {
            exit(EXIT_FAILURE);
            printf("\nerrore partenza thread");

        }
    }
    //Tasti direzionali per gestire il puntatore del mouse
    else if (strcmp(codice, down)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, 0,passo);
    else if (strcmp(codice, up)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, 0,passo*(-1));
    else if (strcmp(codice, right)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, passo, 0);
    else if (strcmp(codice, left)==0) XWarpPointer(display, None, None, 0, 0, 0, 0, passo*(-1),0);

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

    //Alla pressione di un tasto numerico richiameremo la funzione caricalist
    //che riempirà la listbox con i corrispondenti caratteri
    else if (strcmp(codice, tasto_2)==0) caricalist(2,display);
    else if (strcmp(codice, tasto_3)==0) caricalist(3,display);
    else if (strcmp(codice, tasto_4)==0) caricalist(4,display);
    else if (strcmp(codice, tasto_5)==0) caricalist(5,display);
    else if (strcmp(codice, tasto_6)==0) caricalist(6,display);
    else if (strcmp(codice, tasto_7)==0) caricalist(7,display);
    else if (strcmp(codice, tasto_8)==0) caricalist(8,display);
    else if (strcmp(codice, tasto_9)==0) caricalist(9,display);

    //La pressione di tale tasto manderà nella textbox selezionata il carattere selezionato nella listbox
    else if (strcmp(codice, tasto_exit)==0)
        if (statot9 == 1)
            parola_t9();
        else
            tappa();

    else
    {
        //Stamperà il carattere "a" qualora premessimo un tasto non mappato con nessuna funzionalità
        int tasto = XK_A;
        sprintf(codicet9,"");
        luncodicet9 = 0;
        comodo = lista;

        //tasti di navigazione web: Tab, Invio,
        // Vol+ e Vol- per lo scorrimento di elenchi,
        if (strcmp(codice, red)==0) tasto=XK_Tab;
        else if (strcmp(codice, green)==0) tasto =XK_Return;
        else if (strcmp(codice, vol_plus)==0) tasto=XK_Up;
        else if (strcmp(codice, vol_minus)==0) tasto =XK_Down;

        //Tastp spazio
        else if (strcmp(codice, tasto_0)==0) tasto = XK_space;

        // Get the root window for the current display.
        Window winRoot = XDefaultRootWindow(display);

        // Find the window which has the current keyboard focus.
        Window winFocus;
        int revert;
        XGetInputFocus(display, &winFocus, &revert);

        // Send a fake key press event to the window.
        XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, tasto, 0);
        XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

        // Send a fake key release event to the window.
        event = createKeyEvent(display, winFocus, winRoot, false, tasto, 0);
        XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

    }

    XCloseDisplay(display);

}



//Funzione thread di IRW
void *thtel(void *arg)
{

    printf("\nIRW in ascolto\n");

    int fd,i;
    char buf[128];

    struct sockaddr_un addr;
    addr.sun_family=AF_UNIX;
    strcpy(addr.sun_path,"/dev/lircd");
    fd=socket(AF_UNIX,SOCK_STREAM,0);

    if(fd==-1)
    {
        perror("socket");
        exit(errno);
    };

    if(connect(fd,(struct sockaddr *)&addr,sizeof(addr))==-1)
    {
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

        if(i==-1)
        {
            perror("read");
            exit(errno);
        };
        if(!i)
            exit(0);

        j=0;
        k=0;
        lun=strlen(buf);
        for (j=9; j<lun; j++)
        {
            if (buf[j]==' ') break;
            cod[k]=buf[j];
            k=k+1;

        }
        printf("\nCodice:\t%s\nStringa restituita dal driver: \t%s\n",cod,buf);
        elabora(cod);

    }

}


//Funzione thread che legge il file dizionazrio e riempe la lista per il T9
void *thfilet9 (void *arg)
{
    lista=NULL;

    struct nodo *p = (struct nodo *)malloc(sizeof(struct nodo));
    FILE *pfile;
    //pfile = fopen ("parole.txt","r");
    pfile = fopen ("dizionario_ita.txt","r");

    if (pfile ==NULL)
    {
        printf("\nerrore file dizionario\n\n");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        if (fscanf(pfile,"%s %s", p->codice, p->parola)==EOF) break;
        //printf ("\n%s %s\n", p->codice, p->parola);


        if (lista ==NULL)
        {
            lista = (struct nodo *)malloc(sizeof(struct nodo));
            sprintf (lista->parola,"%s",p->parola);
            sprintf(lista->codice,"%s",p->codice);
            lista->next=NULL;
        }
        else
        {
            struct nodo *r = lista;
            struct nodo *q = lista;

            while (q!=NULL)
            {
                r=q;
                q=q->next;
            }

            q = (struct nodo *)malloc(sizeof(struct nodo));
            sprintf (q->parola,"%s",p->parola);
            sprintf(q->codice,"%s",p->codice);
            q->next=NULL;
            r->next=q;

        }

    }//close while

    fclose(pfile);
    flagcaricat9=1;


}




int main( int argc, char *argv[])
{
    //Lanciamo il thread per il t9
    int rest;
    pthread_t t9_thread;
    flagcaricat9=0;
    rest = pthread_create(&t9_thread, NULL, thfilet9, NULL);
    if (rest != 0)
    {
        printf("\nerrore partenza thread del T9");
        exit(EXIT_FAILURE);
    }
    //Lanciamo il thread di IRW
    int res;
    pthread_t tel_thread;
    res = pthread_create(&tel_thread, NULL, thtel, NULL);
    if (res != 0)
    {
        printf("\nerrore partenza thread di gestione irda");
        exit(EXIT_FAILURE);
    }


    statot9=0;
    luncodicet9=0;
    sprintf(codicet9,"");
    tastocor=0;
    tastoprec=0;

    //carichiamo la mappatura dei tasti
    caricamatrice();

    //inizializiamo l'interfaccia GTK
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_title(GTK_WINDOW(window), "Prova");
    gtk_window_set_default_size(GTK_WINDOW(window), 105, 105);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
    fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(window), fixed);
    gtklist=gtk_list_new();
    gtk_list_set_selection_mode((GtkList *) gtklist, GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(window), gtklist);
    gtk_widget_show(gtklist);

    int i=0;

    for (i=0; i<N; i++)
    {
        GtkWidget       *label;
        gchar           *string;
        sprintf(buffer, "%d", i);
        label=gtk_label_new(buffer);
        list_item=gtk_list_item_new();
        gtk_container_add(GTK_CONTAINER(list_item), label);
        gtk_widget_show(label);
        gtk_container_add(GTK_CONTAINER(gtklist), list_item);
        gtk_widget_show(list_item);
        gtk_label_get(GTK_LABEL(label), &string);
        gtk_object_set_data(GTK_OBJECT(list_item),list_item_data_key,string);
    }

    indice=0;
    gtk_list_select_item((GtkList *) gtklist,indice);
    gtk_fixed_put(GTK_FIXED(fixed), gtklist, 0, 0);
    gtk_widget_set_size_request(gtklist, 100,100);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;



}









