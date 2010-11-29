//carta vetrata e parole composte con spazi in mezzo (soluzione banale->sostituire spazio con il trattino):-@

/*COSE DA FARE:
1) sistemare ".; '; -; "   FATTO
2) sistemare gestione spazio nel vocabolario (c'è una parola in più)!!!

*/


#include <stdio.h>
#include <string.h>

//Per le estensioni dell'ASCII (caratteri estesi->vocali accentate)
#include <wchar.h>

void main()
{
    	wchar_t wcarattere[6];     // variabile per carattere esteso

	//Mapping per i caratteri accentati

	wcarattere[0]= 224;  //à = 224    0xE0
	wcarattere[1]= 232;  //è = 232    0xE8
	wcarattere[2]= 233;  //é = 233    0xE9
	wcarattere[3]= 236;  //ì = 236    0xEC
	wcarattere[4]= 242;  //ò = 242    0xF2
	wcarattere[5]= 249;  //ù = 249    0xF9

	wcarattere[6]= 32;  //<SPACE> = 32    0x20
	wcarattere[7]= 44;  //, = 44   0x2C
	wcarattere[8]= 45;  //- = 45   0x2D
	wcarattere[9]= 39;  //' = 39   0x27	

	/*
	//Stampa di verifica!!
	printf("wcarattere[0] = '%lc'\n",wcarattere[0]);
	printf("wcarattere[1] = '%lc'\n",wcarattere[1]);
	printf("wcarattere[2] = '%lc'\n",wcarattere[2]);
	printf("wcarattere[3] = '%lc'\n",wcarattere[3]);
	printf("wcarattere[4] = '%lc'\n",wcarattere[4]);
	printf("wcarattere[5] = '%lc'\n",wcarattere[5]);

	printf("wcarattere[6] = '%lc'\n",wcarattere[6]);
	printf("wcarattere[7] = '%lc'\n",wcarattere[7]);
	printf("wcarattere[8] = '%lc'\n",wcarattere[8]);
	printf("wcarattere[9] = '%lc'\n",wcarattere[9]);
	*/

	FILE *file_read, *file_write;
	int N = 30;
	int count=0;
	char buf[N];
	int i=0;
	int j=0;
	int lunghezza=0;


//CONTEGGIO PAROLE VOCABOLARIO E SALVIAMO NELLA PRIMA RIGA DEL FILE IL NUMERO DI PAROLE
//------------------------------------------------------------------------------------------
	file_read = fopen("/home/carmelo/Scrivania/parole_finale.txt", "r");
	file_write = fopen("/home/carmelo/Scrivania/parole+numeri.txt", "w");


	//Conteggio
	while(feof(file_read)==0)
	{
		fscanf(file_read, "%s", buf);

		if(buf!=EOF)
			count++;
		else
			break;
	}

	printf("%d words\n", count-1);
	fprintf(file_write, "%d words\n", count-1);
	fclose(file_read);
	fclose(file_write);
//------------------------------------------------------------------------------------------


//SOSTITUZIONE CARATTERI CON NUMERI TIPICI DELLA MODALITÀ T9
//------------------------------------------------------------------------------------------
	file_read = fopen("/home/carmelo/Scrivania/parole_finale.txt", "r");
	file_write = fopen("/home/carmelo/Scrivania/parole+numeri.txt", "a");


	while(feof(file_read)==0)
	{		
		fscanf(file_read, "%s", buf);
		if( (j<count-1) && (buf!=EOF) )
		{
			lunghezza=strlen(buf);		

			//Associamo ai caratteri i tasti numerici del telecomando
			int codice_t9[lunghezza];
			for(i=0; i<lunghezza; i++)
			{
				codice_t9[lunghezza]=0;

				if( (buf[i]== (char)wcarattere[7]) || (buf[i]== (char)wcarattere[8]) || (buf[i]== (char)wcarattere[9]) ) //, - '
					codice_t9[i]=1;

				else if( (buf[i]== 'a') || (buf[i]== 'b') || (buf[i]== 'c') || (buf[i]== (char)wcarattere[0]) )  //à
					codice_t9[i]=2;

				else if( (buf[i]== 'd') || (buf[i]== 'e') || (buf[i]== 'f') || (buf[i]== (char)wcarattere[1]) || (buf[i]== (char)wcarattere[2]) ) //é, è
					codice_t9[i]=3;

				else if( (buf[i]== 'g') || (buf[i]== 'h') || (buf[i]== 'i') || (buf[i]== (char)wcarattere[3]) )  //ì
					codice_t9[i]=4;

				else if( (buf[i]== 'j') || (buf[i]== 'k') || (buf[i]== 'l') )
					codice_t9[i]=5;

				else if( (buf[i]== 'm') || (buf[i]== 'n') || (buf[i]== 'o') || (buf[i]== (char)wcarattere[4]) )  //ò
					codice_t9[i]=6;
			
				else if( (buf[i]== 'p') || (buf[i]== 'q') || (buf[i]== 'r') || (buf[i]== 's') )
					codice_t9[i]=7;

				else if( (buf[i]== 't') || (buf[i]== 'u') || (buf[i]== 'v') || (buf[i]== (char)wcarattere[5]) )  //ù
					codice_t9[i]=8;

				else if( (buf[i]== 'w') || (buf[i]== 'x') || (buf[i]== 'y') || (buf[i]== 'z') )
					codice_t9[i]=9;

				else if(buf[i]== (char)wcarattere[6]) //<SPACE>
					codice_t9[i]=0;

				fprintf(file_write, "%d", codice_t9[i] );
			}
			fprintf(file_write, " %s\n", buf);
			j++;
		}
	}
	fclose(file_read);
	fclose(file_write);
//------------------------------------------------------------------------------------------
}
