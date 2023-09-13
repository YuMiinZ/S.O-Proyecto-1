// C program to illustrate the regexec() function
#include <regex.h>
#include <stdio.h>

// Function to print the result
void print_result(int value)
{

	// If pattern found
	if (value == 0) {
		printf("Pattern found.\n");
	}

	// If pattern not found
	else if (value == REG_NOMATCH) {
		printf("Pattern not found.\n");
	}

	// If error occurred during Pattern
	// matching
	else {
		printf("An error occurred.\n");
	}
}

// Driver Code
int main()
{
	// Variable to store initial regex()
	regex_t reegex;
  // Variable with the string (seria el buffer de 8k)
	char buffer[8192];


  FILE *file;
	int i = 0;
	char letra;
	file = fopen("a.txt", "r");
	if (file == NULL) {
			printf("Error opening file");
			return 1;
	}
	// Read the file content into the buffer
	while ((letra = fgetc(file)) != EOF){
			buffer[i] = letra;
			i++;
	}

	// Close the file
	fclose(file);
 // Open the file for reading

	// Variable for return type
	int value;

	// Creation of regEx (se ocupa para saber que buscar, seria el primer parametro)
	value = regcomp( &reegex, "Sanco Pansa", REG_ICASE);

	// Comparing C with
	// string in reg
	value = regexec( &reegex, buffer, 0, NULL, 0);

	print_result(value);

	return 0;
}
