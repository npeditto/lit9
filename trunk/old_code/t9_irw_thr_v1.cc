/*****************************************************************************************************************************
	LIT9 ( Linux Interface T9 ) - Human computer interface based on a TV remote control to interact with a Linux operating system.
	
	Copyright (C)  2010 
		Davide Mulfari:	davidemulfari@gmail.com
		Nicola Peditto:	n.peditto@gmail.com
		Carmelo Romeo:	carmelo.romeo85@gmail.com 
		Fabio Verboso:	fabio.verboso@gmail.com	

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

		- Predictive text technology (T9)  
			copyright  (C) 2000 by Markku Korsumäki  
			email: markku.t.korsumaki@mbnet.fi
		
****************************************************************************************************************************/



//To compile  
//g++ -o prova_thr t9_irw_thr.cc -lpthread




//LIRC IRW's libraries -----------
#ifdef HAVE_CONFIG_H
# include <config.h>
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


#include <pthread.h>
#include <semaphore.h>


//T9's  libraries---------
using namespace std;

#include <new>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include <cstdlib>

#include <sstream>

// max word length, used when reading word with fscanf
#define WORD_LEN 40
#define ESC 27
#define DEFAULT_LIBRARY "word_list.txt"
//#define DEFAULT_LIBRARY "dizionario_ita.txt"



char cod[8];		//codice acquisito da IRW


//CREAZIONE SEMAFORI PER I THREAD
sem_t attesa_t9;
sem_t attesa_irw;



//---INIZIO FUNZIONI T9---------------------------------------------------------------

typedef struct {
    string key;
    string word;
} word_struct_t;

vector<word_struct_t> word_list;


// count how many times we iterate in loop
int total_counter=0;
int counter=0;


int find_next_word(const string &str, const int start=0)
{
    int tmp = start;
    counter = 0;
    vector<word_struct_t>::iterator word_iter;
    word_iter = word_list.begin() + start;
    for (; word_iter != word_list.end(); ++word_iter, ++tmp, ++counter, ++total_counter)
    {
        if (word_iter->key.substr(0,str.length()) == str)
            return tmp; //se trova una corrispondenza ritorna tmp
    }
        
    return -1;	// ritorna -1 se non trova la parola
}



void print_word(const string &str, const int i)
{
 /*   int x, y;
    getyx(stdscr, y, x);
    mvprintw(y-1, x, "%s\t%s\t(%s) %d iterations (%d total)\n",
             word_list[i].word.substr(0, str.length()).c_str(),
             str.c_str(),
             word_list[i].word.c_str(),
             counter, total_counter);

*/
//printf( "\nindice: %d - parola: %s", i , word_list[i].word.substr(0, str.length()).c_str() );
printf( "\nParola: %s" , word_list[i].word.substr(0, str.length()).c_str() );


}



void print_screen(void)
{
    printf("\t 1    2    3 \n");
    printf("\t...  abc  def\n");
    printf("\t 4    5    6 \n");
    printf("\tghi  jkl  mno\n");
    printf("\t 7    8    9 \n");
    printf("\tpqrs tuv  wxy\n");
    printf("\t *    0    # \n");
    printf("\t    [spc]    \n\n");
}


//---FINE FUNZIONI T9---------------------------------------------------------------




//---FUNZIONE IRW----------------------------------------------------------------------------

void *thirw(void *arg)
{

  	printf("\nIRW in ascolto!\n");


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
	

	int lun,j,k;

	for(;;)  {

		bzero(buf,128);
		memset(cod,0,sizeof(cod));
		i=read(fd,buf,128);

		if(i==-1)  {
			perror("read");
			exit(errno);
		};
		if(!i)  exit(0);

		
		j=0;
                k=0;
		lun=strlen(buf);

		for (j=9;j<lun;j++)
		{
			if (buf[j]==' ') break;				
			cod[k]=buf[j];
			k=k+1;
		}

		//printf("\nCodice: %s\n",cod);


		sem_post( &attesa_t9 );   //RIATTIVO L'ESECUZIONE DEL T9

		sem_wait( &attesa_irw );  //INTERROMPO L'IRW


	}


}

//-----------------------------------------------------------------------------------------------



int main(int argc, char *argv[])
{

	int i=0, key=0, words_in_lib;

	string key_str;
	FILE *lib;
	string lib_filename;
	word_struct_t word;


	//INIZIALIZZAZIONE SEMAFORI: finchè il semaforo avrà valore zero farà ciclare il thread rispettivo
	sem_init( &attesa_t9, 0 , 0);
	sem_init( &attesa_irw, 0 , 0);


	printf("T9 Predictive text input. %s %s\n", __TIME__, __DATE__);

   	print_screen();


	//ACQUISISCO LE PAROLE DAL DIZIONARIO------------------------------------------- 
	if (argc<2)
		lib_filename = DEFAULT_LIBRARY;
	else
		lib_filename = argv[1];

	lib = fopen( lib_filename.c_str(), "r" );

	if (NULL == lib) {

		cerr << "Cannot open input file: " << lib_filename << endl;
		exit(-1);
	}

	if (!fscanf(lib, "%d words\n", &words_in_lib))
	{

		cerr << "File error in reading file" << endl;
		exit(-1);
	}
    
	printf("%d words in library\n", words_in_lib);
    
        // Read words from lib
	for (i=0; i<words_in_lib; i++)
	{
		char read_key[WORD_LEN+1], read_word[WORD_LEN+1];

		if (!fscanf(lib, "%s %s\n", read_key , read_word ))
		{
		    cerr << "File error in reading words" << endl;
		    exit(-1);
		}

		word.key = read_key;
		word.word = read_word;
		word_list.push_back(word);

	}
	fclose(lib);
	//---------------------------------------------------------------------------



	//CREO IL THREAD DELL'IRW---------------------------------------
	int res;
	pthread_t irw_thread;

	res = pthread_create(&irw_thread, NULL, thirw, NULL);

	if (res != 0) {
		printf("\nErrore partenza thread!");
		exit(EXIT_FAILURE);
	}
	//--------------------------------------------------------------



    i=0;

    try{


	while(key != ESC)
        {
		
		sem_wait( &attesa_t9 );	//metto in attesa il t9 che arrivi un segnale da telecomando

		//il T9 sarà riattivato dal thread IRW con una post una volta che avrà ricevuto un segnale dal telecomando  


		//MAPPING-----------------------------------------------------------
		if(strcmp(cod,"ff07bfe")==0)	key=1;
		if(strcmp(cod,"ff07bfd")==0)	key=2;
		if(strcmp(cod,"ff07bfc")==0)	key=3;
		if(strcmp(cod,"ff07bfb")==0)	key=4;
		if(strcmp(cod,"ff07bfa")==0)	key=5;
		if(strcmp(cod,"ff07bf9")==0)	key=6;
		if(strcmp(cod,"ff07bf8")==0)	key=7;
		if(strcmp(cod,"ff07bf7")==0)	key=8;
		if(strcmp(cod,"ff07bf6")==0)	key=9;
		if(strcmp(cod,"ff07bff")==0)	key=0;		//space
		if(strcmp(cod,"ff07bf4")==0)	key=13;		//enter
		if(strcmp(cod,"ff07bf5")==0)	key=127;	//backspace-clear
		if(strcmp(cod,"ff07bf3")==0)	key=27;		//ESC/power

		//------------------------------------------------------------------




		switch (key)
		{

			case '0': //space
			    i = 0;
			    total_counter = 0;
			    key_str = "";
		    	continue;


			case 13:  // enter, search next word...
			{
			    string tmp_str;
			    bool stop_the_loop = false;
			    tmp_str = word_list[i].word.substr(0, key_str.length());
			    do
			    {
				i = find_next_word(key_str, i+1);
				if (i == -1)
				{
				    i = find_next_word(key_str, 0);
				    stop_the_loop = true;
				}
			    }
			    while (tmp_str == word_list[i].word.substr(0, key_str.length()) and !stop_the_loop);
			}
			print_word(key_str,i);
			continue;
			    

			case 127: // Backspace-Clear
			    i=0;
			    if (key_str.length() >= 1)
			    {
				key_str.erase(key_str.length()-1, 1);

			    }
			    else
				continue;
		    	break;

		}//chiusura switch


	//Fase di ricerca delle parole-------------------------------------------------------------------
		if (key != 127){

			std::string key2;
			std::stringstream out;
			out << key;
			key2 = out.str();

			key_str = key_str + key2;

			//key_str += key;

		}

		i = find_next_word(key_str, i);


		if (i != -1){

			print_word(key_str,i);
			//printf( "indice: %d - parola: %s", i , word_list[i].word.substr(0, key_str.length()).c_str() );
		
		}
		else
		{
			printf("\nParola non trovata\n");
			key_str.erase(key_str.length()-1, 1);
			i = 0;
			print_word(key_str,i);
		}
	//------------------------------------------------------------------------------------


		printf("\n%s --> tasto premuto %d \n",cod,key);

		sem_post( &attesa_irw );   //riattivo il thread IRW per attendere un segnale dal telecomando



	}//chiusura while
	
	printf("\n");

    }//chiusura try


    catch (const bad_alloc& x) { cout << "Out of memory: " << x.what() << endl; }
    catch (const out_of_range& x) { cout << "Out of range: " << x.what() << endl; }
    catch (const exception& x) { cout << "Exception: " << x.what() << endl; }
    catch (...) { cout << "Unknown error." << endl; }



    return(0);



}
