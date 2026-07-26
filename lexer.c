#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/*

SEMI_COLON = ;
OPEN PAREN = (
CLOSE PAREN = )

*/


typedef enum{
    SEMI_COLON,
    OPEN_PAREN,
    CLOSE_PAREN
}TypeSeperator;


typedef enum{
    EXIT,
} TypeKeyword;

typedef enum{
    INT,
} TypeLiteral;


typedef struct{
    TypeKeyword type;
} TokenKeyword;


typedef struct {
    TypeSeperator type;

}TokenSeperator;



/*verinin tipi ve değerini alıyo */

typedef struct{
    TypeLiteral type;
    int value;
}TokenLiteral;


TokenLiteral generate_number(char current, FILE *file){
    TokenLiteral token;
    token.type = INT;
    token.value = 0;

    int value = 0;
    while(isdigit(current) && current !=EOF){
        if(!isdigit(current)){
            break;
        }
        value += (int) current - '0';
        printf("%c", current);
        current = fgetc(file);
    }
    token.value = value;
    fclose(file);

    return (token);
}



void Lexer(FILE *file ){

    if(file == NULL){   
        printf("\nDosya Bulunamadı");
    }
    char current_char = fgetc(file);

    while (current_char !=EOF) {
        if(current_char == ';'){
            printf("FOUND SEMICOLON: \n");
        }else if(current_char == '('){
            printf("FOUND OPEN PAREN: \n");
        }
        else if(current_char == ')'){
            printf("FOUND CLOSE PAREN: \n");
        }else if (isdigit(current_char)){
            generate_number(current_char,file);
            //printf("FOUND DIGIT: %d\n",current_char - '0');
        }else if(isalpha(current_char)){
            printf("FOUND CHARACTER: %c \n",current_char);

        }
        current_char = fgetc(file);

    }


    fclose(file);
}


int main(){
    FILE *file;

    file = fopen("example.br","r");
    
    Lexer(file);

}
