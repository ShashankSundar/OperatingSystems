#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

void readDir(DIR* dir, int num, char arg1[]) {
	int unique = 0, wordCount = 0, size = 32, s = 0, c;			//s=size of individual word, c is character being read
	char buff[80] = ""; 
	struct stat buf;						
	struct dirent* file;  					//file reader

	char PATH[200];							//path string
	strcpy(PATH, arg1);			

	if ((dir = opendir (PATH)) != NULL) {

		//parallel arrays
		char** words = calloc( size, sizeof( char* ) );			
		int* freq = calloc( size, sizeof( int ) );
		printf("Allocated initial parallel arrays of size %d.\n", size);

  		while ((file = readdir (dir)) != NULL) {
  			strcpy(PATH, arg1);
  			strcat(PATH, "/");
  		    strcat(PATH, file->d_name);
    		if (lstat(PATH, &buf) == -1){
    			perror ("");
    		}

    		if (S_ISREG(buf.st_mode)){
    			FILE* fd = fopen(PATH, "r");

				if ( fd == NULL )
			  	{
			   		perror( "open() failed" );
			  	}

			    while ((c = fgetc(fd)) != EOF){
			    	char ch[2];
			    	sprintf(ch, "%c", c);					//convert int c to char ch
			    	
			    	//if not a alnum, then is space or punctuation mark
			    	if (isalnum(c) != 0) {
			    		strcat(buff, ch);
			    		s++;
			    		continue;
			    	}

			    	else if (s > 1){

			    		//check if reallocation is necessary
			    		if (unique >= size){
			    			size = size + 32;
			    			words = realloc(words, size * sizeof( char* ));
			    			freq = realloc(freq, size * sizeof( int ) );
			    			printf("Re-allocated parallel arrays to be size %d.\n", size);
			    		}

			    		//check if word has already been seen
			    		int check = 0, mark = 0;
			    		for (int i = 0; i < unique; i++){
			    			if (strcmp(words[i], buff) == 0){			
			    				check = 1;
			    				mark = i;
			    				break;
			    			}
			    		}

			    		//unique word found
			    		if (check == 0){
				    		words[unique] = calloc( s + 1, sizeof( char ) );		//allocate s+1 for '/0' character
				    		strcpy(words[unique], buff);
				    		freq[unique] = 1;
				    		unique++;
			    	 	}
			    	 	//word has been seen before
			    	 	else
			    	 		freq[mark] = freq[mark] + 1;

			    	 	//resert s and buff and increment total word count
			    	 	strcpy(buff, "");
				    	s=0;
				    	wordCount++;

			    		continue;
			    	}

			    	s = 0;							
			    	strcpy(buff, "");		
				}
				fclose(fd);
    		}	
  		}
  		closedir (dir);

  		printf("All done (successfully read %d words; %d unique words).\n", wordCount, unique);
  		if (num == 0){
	     	printf("All words (and corresponding counts) are:\n");
	     	for (int k = 0; k < unique; k++){
	            printf("%s -- %d\n", words[k], freq[k]);
	     	}
	    }
	    else{
	     	printf("First %d words (and corresponding counts) are:\n", num);
	     	for (int k = 0; k < num; k++){
	            printf("%s -- %d\n", words[k], freq[k]);
	     	}
	     	printf("Last %d words (and corresponding counts) are:\n", num);
	     	for (int i = unique - num; i < unique ; i++){
	            printf("%s -- %d\n", words[i], freq[i]);             
	     	}
	    }

	    //free up individual words
	    for(int i = 0; i <= unique; i++){
	     	free(words[i]);
	    }
	    free(words);
	    free(freq);
	}

	else {
  		perror ("");
	}
}


int main(int argc, char** argv){ 

	DIR* dir = NULL;
	char oneArg[200]; 		//string to hold directory name to pass to function
	int num = 0;			

	//check if no arguements given
	if (argc <= 1){
		perror ("");
  		return EXIT_FAILURE;
	}

	//sufficient number of arguements
	if(argc == 3){
		num = atoi(argv[2]);
	}

	strcpy(oneArg, argv[1]);

	readDir(dir, num, oneArg);
	
	return EXIT_SUCCESS;
}