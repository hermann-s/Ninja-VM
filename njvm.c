#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "njvm.h"

#define HALT 0
#define PUSHC 1 /*pushc <n>*/
#define ADD 2 
#define SUB 3
#define MUL 4
#define DIV 5
#define MOD 6

#define RDINT 7
#define WRINT 8

#define ASF 9 /*asf <n>*/
#define RSF 10
#define PUSHL 11 /*pushl <n>*/
#define POPL 12 /*popl <n>*/

#define EQ 13  /* == */
#define NE 14  /* != */
#define LT 15  /* <  */
#define LE 16  /* <= */
#define GT 17  /* >  */
#define GE 18  /* >= */

#define JMP 19 /* jmp <target> */
#define BRF 20 /* brf <target> */
#define BRT 21 /* brt <target> */
#define PO 99

#define IMMEDIATE(x) ((x) & 0x00FFFFFF)
#define SIGN_EXTEND(i) ((i) & 0x00800000 ? (i) | 0xFF000000 : (i))

#define stackSize 1024

const char version[] = "0.2";
int stack[stackSize];
int stackPointer=0;
int framePointer=0;
int programCounter=0; /* zählt die Zeilen bei der Ausgabe */
FILE *codeFile;
int fileSize, numberOfCommands;
unsigned int *programPointer;


/* main */
int main(int argc, char *argv[]){
  if(argc >= 2){
    int i, fileClosed;
    for(i=1;i<argc;i++){
      /* Hilfe ausgeben */
      if(strcmp(argv[i],"--help")==0){
        printHelp();
      }else if(strcmp(argv[i],"--version")==0){
        /* Versionsinformationen ausgeben */
        printf("Ninja Virtual Machine version %s (compiled %s, %s)\n",version,__DATE__,__TIME__); 
      }else{
        codeFile = fopen(argv[i], "r");
        if(codeFile == NULL || (strstr(argv[i], ".bin") == NULL )){
          printf("Couldn't open %s . You need to use a .bin file. Try again. \n", argv[i]);
        }else{
          printf("Opened %s successful. \n", argv[i]);
          /* Zeiger ans Ende setzen, um Größe zu ermitteln */
          fseek(codeFile, 0, SEEK_END);
          fileSize = ftell(codeFile);
          /* Zeiger wieder an den Anfang setzen */
          fseek(codeFile, 0, SEEK_SET);
          programPointer = malloc(fileSize);
          if( programPointer == NULL){
            printf("No RAM available. \n");
            exit(-99);
          }
          printf("Size: %d Bytes \n", fileSize);
          numberOfCommands = fread(programPointer, 4, fileSize/4, codeFile);
          if(numberOfCommands < fileSize/sizeof(programPointer[0])){
            printf("There occured an error reading the file.  \n");
            exit(-99);
          }
          program(programPointer, fileSize/sizeof(programPointer[0]));
          fileClosed = fclose(codeFile);
          if(fileClosed != 0){
            printf("Warning ! Couldn't close the file. \n");
          }
        }    
      }
    }
  }else{
    printf("No Arguments, try --help.\n");
  }

   return 0;
}


/* Ausgabe von Option */
void printHelp(void){
  printf("./njvm [option] [option] ...\n");
  printf("./njvm path/to/filename.bin executes this code\n");
  printf("--help     print this page\n");
  printf("--version  print version of programm\n");
}


/* Gibt das Programm aus */
void printProgram(unsigned int *code, int size){
  int zeile, i;
  
  for(i=0;i<size;i++){
     
    zeile = (code[i]&0xFF000000)>>24;
  
    if(zeile==HALT){
      printf("%03d: halt\n",programCounter);
    }else if(zeile==PUSHC){
      printf("%03d: pushc %5d\n", programCounter, (SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==ADD){
      printf("%03d: add\n",programCounter);
    }else if(zeile==SUB){
      printf("%03d: sub\n",programCounter);
    }else if(zeile==MUL){
      printf("%03d: mul\n",programCounter);
    }else if(zeile==DIV){
      printf("%03d: div\n",programCounter);
    }else if(zeile==MOD){
      printf("%03d: mod\n",programCounter);
    }else if(zeile==RDINT){
      printf("%03d: rdint\n",programCounter);
    }else if(zeile==WRINT){
      printf("%03d: wrint\n",programCounter);
    }else if(zeile==ASF){
      printf("%03d: asf %7d\n",programCounter,(SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==RSF){
      printf("%03d: rsf\n",programCounter);
    }else if(zeile==PUSHL){
      printf("%03d: pushl %5d\n",programCounter,(SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==POPL){
      printf("%03d: popl %6d\n",programCounter,(SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==EQ){
      printf("%03d: eq \n",programCounter);
    }else if(zeile==NE){
      printf("%03d: ne \n",programCounter);
    }else if(zeile==LT){
      printf("%03d: lt \n",programCounter);
    }else if(zeile==LE){
      printf("%03d: le \n",programCounter);
    }else if(zeile==GT){
      printf("%03d: gt \n",programCounter);
    }else if(zeile==GE){
      printf("%03d: ge \n",programCounter);
    }else if(zeile==JMP){
      printf("%03d: jmp %2d\n",programCounter,(SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==BRF){
      printf("%03d: brf %2d\n",programCounter,(SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==BRT){
      printf("%03d: brt %2d\n",programCounter,(SIGN_EXTEND(code[i]&0x00FFFFFF)));
    }else if(zeile==PO){
      printf("%03d: po\n",programCounter);
    }

    programCounter++;
    
  }
  programCounter = 0;
}

/*code1 und code2 ausfuehren*/
void program(unsigned int *code,int size){
  int instruction, n1, n2, eingeleseneZahl;

  printf("Ninja Virtual Machine started\n");

  printProgram(code, size);

  /*geht jede Instruktion der Instruktionstabelle durch*/
  for(programCounter=0;programCounter<size;programCounter++){
    instruction=(code[programCounter]&0xFF000000)>>24;

    if(instruction==HALT){
      break;
    }else if(instruction==PUSHC){ /*schreiben in stack*/
      push(code[programCounter]);
    }else if(instruction==ADD){
      n1=pop();
      n2=pop();
      push(n2+n1);
    }else if(instruction==SUB){
      n1=pop();
      n2=pop();
      push(n1-n2);
    }else if(instruction==MUL){
      n1=pop();
      n2=pop();
      push(n2*n1);
    }else if(instruction==DIV){
      n1=pop();
      n2=pop();
      
      if(n2!=0){
        push(n2/n1);
      }
    }else if(instruction==MOD){
      n1=pop();
      n2=pop();
      push(n1%n2);
    }else if(instruction==RDINT){
        /* liest Zahl auf der konsole ein */
      scanf("%d", &eingeleseneZahl);
      push(eingeleseneZahl);
    }else if(instruction==WRINT){
      printf("%d\n",SIGN_EXTEND(IMMEDIATE(stack[stackPointer-1])));
    }else if(instruction==ASF){ /*stack frame anlegen, mit groesse n*/
      push(framePointer);/*position des letzten frames wird gemerkt*/
      push(IMMEDIATE(code[programCounter]));
      framePointer=stackPointer;
      stackPointer=stackPointer+(IMMEDIATE(code[programCounter]));
    }else if(instruction==RSF){ /*benutzen stack frame entfernen*/
      stackPointer=framePointer;
      pop();
      framePointer=pop(); /*position des letzten frames wird zurueckgeschrieben*/
    }else if(instruction==PUSHL){ /*variable von frame wird in stack geschrieben*/
      n2=popFrame(code[programCounter]);
      push(n2);
    }else if(instruction==POPL){ /*wert vom stack wird in frame geschrieben*/
      n1=pop();
      pushFrame(n1,code[programCounter]);
    }else if((instruction==EQ) ||
             (instruction==NE) ||
             (instruction==LT) ||
             (instruction==LE) ||
             (instruction==GT) ||
             (instruction==GE)){

      n1=pop();
      n2=pop();
      /*printf("%d \n", compare(n1, n2, instruction));*/
      push(compare(n1, n2, instruction));
    }else if(instruction==JMP){
      programCounter = IMMEDIATE(code[programCounter]); /* -1 wegen for-schleifen ++ */
      /*printf("%d\n", code[programCounter]);*/
    }else if(instruction==BRF){
      n1=pop();
      if(n1 == 0){
	programCounter = IMMEDIATE(code[programCounter]); /* -1 wegen for-schleifen ++ */
      }else if(n1 == 1){
	/* nix */	
      }else{
	printf("Error in BRF. Stack element is neither 0 nor 1.");
	exit(-99);	
      }
    }else if(instruction==BRT){
	  n1=pop();
	 /*printf("%d\n", n1);*/
	  if(n1 == 1){
		programCounter = IMMEDIATE(code[programCounter]); /* -1 wegen for-schleifen ++ */
	  }else if(n1 == 0){
	    /* nix */	
	  }else{
		printf("Error in BRT. Stack element is neither 0 nor 1.");
		exit(-99);	
	  }
    }else if(instruction==PO){
      printf("Stackpointer: %d\nFramepointer: %d\n",stackPointer,framePointer);
    }
  }

  stackPointer=0;
  framePointer=0;
  printf("Ninja Virtual Machine stopped\n");
}

/* Vergleicht zwei Zahlen */
int compare(int n1, int n2, int instruction){
  bool result = false;
  switch(instruction) {
	case EQ: result = (n1 == n2);
	         break;
    case NE: result = (n1 != n2);
	         break;
	case LT: result = (n1 < n2);
	         break;
	case LE: result = (n1 <= n2);
			 break;
	case GT: result = (n1 > n2);
	         break;
	case GE: result = (n1 >= n2);
	         break;  
  }

  return (result) ? 1 : 0;
}


/*push und pop funktionen fuer frame stack*/
void pushFrame(int num, int point){
  int i = framePointer+(SIGN_EXTEND(IMMEDIATE(point)));
  /*pruefen das stack frame nicht auf stack l*/
  if(i>=(framePointer+stack[framePointer-1])){
    printf("Frameposition out of Range. Program will be stopped.\n");
    exit(-99);
  }
  stack[i]=num;
}
int popFrame(int point){
  int i=framePointer+(SIGN_EXTEND(IMMEDIATE(point)));
  /*pruefen das stack frame nicht auf stack l*/
  if(i>=(framePointer+stack[framePointer-1])){
    printf("Frameposition out of Range. Program will be stopped.\n");
    exit(-99);
  }
  return stack[i];
}


void push(int num){
  stack[stackPointer]=SIGN_EXTEND(IMMEDIATE(num));/*(num&0x00FFFFFF);*/
  stackPointer++;
  /* Überprüfen, ob die Position innerhalb des Stacks liegt */
  if(stackPointer > stackSize){
    printf("Stackposition out of Range. Program will be stopped.\n");
    exit(-99);
  }
}

int pop(void){
  stackPointer--;
  /* Überprüfen, ob die Position innerhalb des Stacks liegt */
  if(stackPointer < 0){
    printf("Stackposition out of Range. Program will be stopped.\n");
    exit(-99);
  }
  return stack[stackPointer];
}
