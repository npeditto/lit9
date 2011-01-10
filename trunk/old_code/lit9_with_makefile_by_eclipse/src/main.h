#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk-2.0/gtk/gtk.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <pthread.h>
#define KEYCODE XK_A
#define N 5

//remote-control's configuration (mapping)
#include "pulsanti.h"
//#include "pulsanti_davide.h"


int matrice[8][N];
int tastocor,tastoprec;
int indice;
int statot9;
char codicet9[30];
int luncodicet9;
int flagcaricat9;

gchar* parole[N];

struct nodo
{
    char parola[30];
    char codice[30];
    struct nodo *next;
};


struct nodo *lista;
struct nodo *comodo;

//global declaration for GTK code-----------------------------------------

GtkWidget *window;
GtkWidget *fixed;
GtkWidget *vbox;
GtkWidget *gtklist;
GtkWidget *list_item;
gchar buffer[64];
const gchar *list_item_data_key="list_item_data";




void *thtel (void *arg);		//thread per irw
void *thfilet9 (void *arg);             // thread per caricare il t9
void caricamatrice ();

#endif /* MAIN_H_ */
