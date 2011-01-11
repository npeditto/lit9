// g++ -l sqlite3 popoladizionario.cpp -o popola

#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

//Per le estensioni dell'ASCII (caratteri estesi->vocali accentate)
#include <wchar.h>

int main(int argc, char** args)
{
    // Create an int variable for storing the return code for each call
    int retval;
        
    // A prepered statement for fetching tables
    sqlite3_stmt *stmt;
    
    // Create a handle for database connection, create a pointer to sqlite3
    sqlite3 *handle;
    
    // try to create the database. If it doesnt exist, it would be created
    // pass a pointer to the pointer to sqlite3, in short sqlite3**
    retval = sqlite3_open("dictionaryt9.sqlite",&handle);
    // If connection failed, handle returns NULL

    if(retval)
    {
        printf("Database connection failed\n");
        return -1;
    }
    printf("Connection successful\n");

    //svuota precedenti dati

    char querydel[100] = "DELETE FROM italiano";
    retval = sqlite3_exec(handle,querydel,0,0,0);
	

    //Mapping per i caratteri accentati
    wchar_t wcarattere[6];     // variabile per carattere esteso

    wcarattere[0]= 224;  //à = 224    0xE0
    wcarattere[1]= 232;  //è = 232    0xE8
    wcarattere[2]= 233;  //é = 233    0xE9
    wcarattere[3]= 236;  //ì = 236    0xEC
    wcarattere[4]= 242;  //ò = 242    0xF2
    wcarattere[5]= 249;  //ù = 249    0xF9

    printf("prova\n");
    printf("wcarattere[5] = '%lc'\n",wcarattere[5]);
    printf("fine prova\n");

    FILE *f=NULL;
    char codice[100];
    char parola[100];
    char query[300];

    wchar_t aux[100];

    bzero(query,300);

    f=fopen("b.txt","r");
/*
    while (!feof(f))
    {
	fscanf(f,"%s %s",codice, parola);

	for(int i=0; i<100; i++)
	{
	    if(parola[i]==(char)wcarattere[0])
		aux[i]= (char)wcarattere[0];

	    else if(parola[i]==(char)wcarattere[1])
		aux[i]= (char)wcarattere[1];

	    else if(parola[i]==(char)wcarattere[2])
		aux[i]= (char)wcarattere[2];

	    else if(parola[i]==(char)wcarattere[3])
		aux[i]= (char)wcarattere[3];

	    else if(parola[i]==(char)wcarattere[4])
		aux[i]= (char)wcarattere[4];

	    else if(parola[i]==(char)wcarattere[5])
		aux[i]= (char)wcarattere[5];

	    else aux[i]=parola[i];
	}

	//sprintf(query, "INSERT INTO italiano (codice, parola) VALUES('%s','%s')", codice, parola);
	sprintf(query, "INSERT INTO italiano (codice, parola) VALUES('%s','%ls')", codice, aux);

        retval = sqlite3_exec(handle,query,0,0,0);
	if(retval)
        {
            printf("ERRORE\n");
            return -1;
        }
	printf("%s\n",query);

        bzero(query,300);
    }

    fclose(f);
    printf("FATTO\n");
    sqlite3_close(handle);
*/
    return 0;
	
}
