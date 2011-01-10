//g++ -l sqlite3 t9sqlite.cpp -o t9sqlite

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


    char parola[50]="2254";

    char *query;
    query=(char*)malloc(sizeof(char)*230);

    char start[100] = "SELECT * FROM italiano where italiano.codice like '";
    char end[4] = "%'";


strcat( query, start);
strcat( query, parola);
strcat( query, end);
printf("%s\n",query);


    retval = sqlite3_prepare_v2(handle,query,-1,&stmt,0);    
//    retval = sqlite3_prepare_v2(handle,queries[0],-1,&stmt,0);
    if(retval)
    {
        printf("Selecting data from DB Failed\n");
        return -1;
    }
    
    // Read the number of rows fetched
    int cols = sqlite3_column_count(stmt);
        
    while(1)
    {
        // fetch a row's status
        retval = sqlite3_step(stmt);
        
        if(retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row
            
            // sqlite3_column_text returns a const void* , typecast it to const char*
            for(int col=0 ; col<cols;col++)
            {
                const char *val = (const char*)sqlite3_column_text(stmt,col);
                printf("%s = %s\t",sqlite3_column_name(stmt,col),val);
            }
            printf("\n");
        }
        else if(retval == SQLITE_DONE)
        {
            // All rows finished
            printf("Finito\n");
            break;
        }
        else
        {
            // Some error encountered
            printf("Some error encountered\n");
            return -1;
        }
    
    }
    
    // Close the handle to free memory
    sqlite3_close(handle);


    return 0;
	
}
