// g++ -l sqlite3 popoladizionario.cpp -o popola

#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

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


    FILE *f=NULL;
    char codice[100];
    char parola[100];
   char queryins1[70]="INSERT INTO italiano (codice,parola)VALUES('";
   char queryins2[6]="','";
   char queryins3[6]="')";
   char *query;
   int i=0;
   query=(char*)malloc(sizeof(char)*230);
   for(i=0; i<230; i++) query[i] = '\0';

    f=fopen("dizionario_ita.txt","r");

	while (!feof(f))
	{
	fscanf(f,"%s %s",codice,parola);
	   strcat( query, queryins1);
	   strcat( query, codice);
	   strcat( query, queryins2);
	   strcat( query, parola);
	   strcat( query, queryins3);
   retval = sqlite3_exec(handle,query,0,0,0);
	if(retval)
    {
        printf("ERRORE\n");
        return -1;
    }
//   printf("%s\n",query);
   for(i=0; i<230; i++) query[i] = '\0';
	}

	fclose(f);
	printf("FATTO\n");
   sqlite3_close(handle);


    return 0;
	
}
