#include <stdio.h>
#include <string.h>
#define DEBUG 0

void main()
{

	FILE *file_read, *file_write;
	int N = 30;
	char parola[N];

	int kk=0;


	file_read = fopen("vocabolario.txt", "r");
	file_write = fopen("italiane+numeri.txt", "a");
	
	char codicet9[30];bzero(codicet9,30);
	char codice[30];bzero(codice,30);

	char accentata[2]; bzero(accentata,2);

	while(feof(file_read)==0)
	{
		fscanf(file_read, "%s", parola);
		if(DEBUG) printf("parola presa: %s\n",parola);

		int dim_parola = strlen(parola);
		if(DEBUG) printf("dim parola: %d\n", dim_parola);


		if (dim_parola <=1){
			if( (parola[0]== 'a') || (parola[0]== 'b') || (parola[0]== 'c')){
				codicet9[0]='2';}

			else if( (parola[0]== 'd') || (parola[0]== 'e') || (parola[0]== 'f')){
				codicet9[0]='3';}

			else if( (parola[0]== 'g') || (parola[0]== 'h') || (parola[0]== 'i')){
				codicet9[0]='4';}

			else if( (parola[0]== 'j') || (parola[0]== 'k') || (parola[0]== 'l') ){
				codicet9[0]='5';}

			else if( (parola[0]== 'm') || (parola[0]== 'n') || (parola[0]== 'o') ){
				codicet9[0]='6';}
		
			else if( (parola[0]== 'p') || (parola[0]== 'q') || (parola[0]== 'r') || (parola[0]== 's') ){
				codicet9[0]='7';}

			else if( (parola[0]== 't') || (parola[0]== 'u') || (parola[0]== 'v')){
				codicet9[0]='8';}

			else if( (parola[0]== 'w') || (parola[0]== 'x') || (parola[0]== 'y') || (parola[0]== 'z') ){
				codicet9[0]='9';}
		}
		else {
		char word[dim_parola-2];
		bzero(word,dim_parola-2);
		for (kk=0; kk < dim_parola-2; kk++){
			sprintf(word,"%s%c",word,parola[kk]);
			if( (parola[kk]== 'a') || (parola[kk]== 'b') || (parola[kk]== 'c')){
				codicet9[kk]='2';}

			else if( (parola[kk]== 'd') || (parola[kk]== 'e') || (parola[kk]== 'f')){
				codicet9[kk]='3';}

			else if( (parola[kk]== 'g') || (parola[kk]== 'h') || (parola[kk]== 'i')){
				codicet9[kk]='4';}

			else if( (parola[kk]== 'j') || (parola[kk]== 'k') || (parola[kk]== 'l') ){
				codicet9[kk]='5';}

			else if( (parola[kk]== 'm') || (parola[kk]== 'n') || (parola[kk]== 'o') ){
				codicet9[kk]='6';}
		
			else if( (parola[kk]== 'p') || (parola[kk]== 'q') || (parola[kk]== 'r') || (parola[kk]== 's') ){
				codicet9[kk]='7';}

			else if( (parola[kk]== 't') || (parola[kk]== 'u') || (parola[kk]== 'v')){
				codicet9[kk]='8';}

			else if( (parola[kk]== 'w') || (parola[kk]== 'x') || (parola[kk]== 'y') || (parola[kk]== 'z') ){
				codicet9[kk]='9';}
		}
		

		for (kk=dim_parola-2; kk < dim_parola; kk++)
			sprintf(accentata,"%s%c",accentata,parola[kk]);

		
		if(DEBUG) printf("carattere accentato: %s\n", accentata);
		if(DEBUG) printf("parola senza accento: %s\n", word);

			if(strcmp(accentata,"à")==0) strcat(codicet9,"2");
			else if(strcmp(accentata,"è")==0) strcat(codicet9,"3");
			else if(strcmp(accentata,"é")==0) strcat(codicet9,"3");
			else if(strcmp(accentata,"ì")==0) strcat(codicet9,"4");
			else if(strcmp(accentata,"ò")==0) strcat(codicet9,"6");
			else if(strcmp(accentata,"ù")==0) strcat(codicet9,"8");
			else{
				for (kk=0; kk < 2; kk++){
				if( (accentata[kk]== 'a') || (accentata[kk]== 'b') || (accentata[kk]== 'c'))
					strcat(codicet9,"2");

				else if( (accentata[kk]== 'd') || (accentata[kk]== 'e') || (accentata[kk]== 'f'))
					strcat(codicet9,"3");

				else if( (accentata[kk]== 'g') || (accentata[kk]== 'h') || (accentata[kk]== 'i'))
					strcat(codicet9,"4");

				else if( (accentata[kk]== 'j') || (accentata[kk]== 'k') || (accentata[kk]== 'l') )
					strcat(codicet9,"5");

				else if( (accentata[kk]== 'm') || (accentata[kk]== 'n') || (accentata[kk]== 'o') )
					strcat(codicet9,"6");
			
				else if( (accentata[kk]== 'p') || (accentata[kk]== 'q') || (accentata[kk]== 'r') || (accentata[kk]== 's') )
					strcat(codicet9,"7");

				else if( (accentata[kk]== 't') || (accentata[kk]== 'u') || (accentata[kk]== 'v'))
					strcat(codicet9,"8");

				else if( (accentata[kk]== 'w') || (accentata[kk]== 'x') || (accentata[kk]== 'y') || (accentata[kk]== 'z') )
					strcat(codicet9,"9");
				}
			}
		bzero(word,dim_parola-2);

		if(DEBUG) printf("CODICE: %s\n", codicet9);
		}
		fprintf(file_write,"%s %s\n",codicet9,parola);

		bzero(codicet9,30);

		bzero(accentata,2);

	}



	fclose(file_read);
	fclose(file_write);

}
