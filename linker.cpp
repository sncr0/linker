#include <cstdlib>
#include <exception>
#include <iostream>
#include <cstring>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <map>
#include <cstdio>
#include <sstream>
#include <utility>

#define BUFFER_SIZE 4096
#define MAX_SYMBOL_LENGTH 16
#define MAX_USELIST_LENGTH 16
#define MAX_DEFLIST_LENGTH 16
#define MAX_SYMBOLTABLE_LENGTH 256
#define MAX_MODULETABLE_LENGTH 128
#define MAX_INSTR 512

void __parseerror(int errcode, int linenum, int lineoffset) {
 static const char* errstr[] = {
 "NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either
 "SYM_EXPECTED", // Symbol Expected
 "MARIE_EXPECTED", // Addressing Expected which is M/A/R/I/E
 "SYM_TOO_LONG", // Symbol Name is too long
 "TOO_MANY_DEF_IN_MODULE", // > 16 
 "TOO_MANY_USE_IN_MODULE", // > 16
 "TOO_MANY_INSTR", // total num_instr exceeds memory size (512) 
 };
 printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset,
        errstr[errcode]);
}

int checkEOF(std::ifstream& file) {
  std::streampos originalPos = file.tellg();
  char c;
  while (file.peek() != EOF) {
    file.get(c);
    if (c != ' ' && c != '\t' && c != '\n') {
      file.seekg(originalPos);
      return 1;
        }
    }
  file.seekg(originalPos);
  return 0;
}

int checkEOL(std::ifstream& file) {
  std::streampos originalPos = file.tellg();
  char c;
  while (file.peek() != EOF) {
    file.get(c);
    if (c != ' ' && c != '\t') {
      file.seekg(originalPos);
      return 1;
        }
    }
  file.seekg(originalPos);
  return 0;
}

char* getToken(int* lineNumber, int* offset, int* lineOffset, char* line, std::ifstream& file) {
  char* token;
  constexpr int BufferSize = BUFFER_SIZE;

  if (*lineNumber == 0) {
    file.getline(line, BufferSize);
    (*lineNumber)++;
    (*offset) = 0;
    (*lineOffset) = 0;
  }

  if (*offset == 0) {
    token = strtok(line, " \t\n");
    }
  else {
    token = strtok(nullptr, " \t\n");
  }

  if (token == nullptr) {
    if (checkEOL(file) == 1) {
      file.getline(line, BufferSize);
      (*lineNumber)++;
      (*offset) = 0;
      (*lineOffset) = 0;
      token = getToken(lineNumber, offset, lineOffset, line, file);
    }
    else {
      return nullptr;
    }
  }
  else {
    *offset = token - line + 1;
    *lineOffset = token - line + strlen(token);
    return token;
  }
  return token;
}

int readInt(int* lineNumber, int* offset, int* lineOffset, char* line, std::ifstream& file) {
  char* token;
  token = getToken(lineNumber, offset, lineOffset, line, file);
  #ifdef DEBUG
  std::cout << "read int: " << token << std::endl;
  #endif // DEBUG

  
  if (token != nullptr) {
    for (int i = 0; token[i] != '\0'; i++) {
      if (isdigit(token[i]) == false) {
        __parseerror(0, *lineNumber, *offset);
        std::exit(EXIT_FAILURE);  
      }
    }
  }
  return std::stoi(token);
}

std::string readSymbol(int* lineNumber, int* offset, int* lineOffset, char* line, std::ifstream& file) {
  char* token;
  token = getToken(lineNumber, offset, lineOffset, line, file);
  if (token == nullptr) {
    __parseerror(1, *lineNumber, *lineOffset + 1);
    std::exit(EXIT_FAILURE);  
  }
  else if (strlen(token) > MAX_SYMBOL_LENGTH) {
    __parseerror(3, *lineNumber, *lineOffset + 1);
    std::exit(EXIT_FAILURE);
  }
  else if (token != nullptr) {
    for (int i = 0; token[i] != '\0'; i++) {
      if (i == 0 && isalpha(token[i]) == false) {
        __parseerror(1, *lineNumber, *offset);
        std::exit(EXIT_FAILURE);  
      }
      else if (isalnum(token[i]) == false) {
        __parseerror(0, *lineNumber, *offset);
        std::exit(EXIT_FAILURE);  
      }

      //std::cout << token[i] << std::endl;
    }
  }
  std::string symbol = token;
  return symbol;
}

std::string readIEAR(int* lineNumber, int* offset, int* lineOffset, char* line, std::ifstream& file) {
  char* token;

  token = getToken(lineNumber, offset, lineOffset, line, file);

  
  #ifdef DEBUG
  std::cout << "read IAER: " << token << std::endl;
  #endif // DEBUGifdef DEBUG



  if (token != nullptr) {
    for (int i = 0; token[i] != '\0'; i++) {
      if (isalpha(token[i]) == false) {
        std::cout << "error: token \"" << token << "\" is incorrect format (wrong operand format)" << std::endl;
      }
    }
  }
  else if (token == nullptr) {
    __parseerror(2, *lineNumber, *lineOffset+1);
    std::exit(EXIT_FAILURE);  
  }
  std::string IAER_symbol = token;
  return IAER_symbol;
}

void Pass1(char* filename, std::map<int, std::pair<std::string, int>>& symbolTable, 
           std::unordered_map<int, int>& moduleBaseTable,
           std::map<int, std::string>& printoutOrder) {
  moduleBaseTable[0] = 0;
    std::ifstream file(filename);


  std::unordered_map<int, std::string> useList;
  std::map<std::string, int> timesUsedMap;
  std::map<int, int> memoryMap;
  std::map<std::string, int> symbolTable_modules;
 // std::map<int, std::string> printoutOrder;
  int moduleBase = 0;
  int moduleCount = 0;
  int location;
  int lineLength = 0;
  char line[BUFFER_SIZE];
  char* token;
  int lineNumber = 0;
  int offset = 0;
  int lineOffset;

  while (checkEOF(file) == 1) {  



    int defcount = readInt(&lineNumber, &offset, &lineOffset, line, file);
    if (defcount > MAX_DEFLIST_LENGTH) {
      __parseerror(4, lineNumber, offset);
      std::exit(EXIT_FAILURE);
    }
    for (int i=0;i<defcount; i++) {
      std::string sym = readSymbol(&lineNumber, &offset, &lineOffset, line, file);
      int relativeWordAdress = readInt(&lineNumber, &offset, &lineOffset, line, file); 
      if (timesUsedMap[sym] == 0) {
        symbolTable[symbolTable.size()] = std::make_pair(sym, relativeWordAdress + moduleBase);
        printoutOrder[symbolTable.size()] = sym;
        symbolTable_modules[sym] = moduleCount + 1;
        timesUsedMap[sym]++;
      } 
      else {
        std::cout << "Warning: Module " << moduleCount + 1 << ": " << sym << " redefinition ignored" << std::endl;
        timesUsedMap[sym]++;
      }
    }

    int usecount = readInt(&lineNumber, &offset, &lineOffset, line, file);
    if (usecount > MAX_USELIST_LENGTH) {
      __parseerror(5, lineNumber, offset);
      std::exit(EXIT_FAILURE);
    }
    for (int i=0;i<usecount;i++) {
      std::string sym = readSymbol(&lineNumber, &offset, &lineOffset, line, file);
    }

    int instcount = readInt(&lineNumber, &offset, &lineOffset, line, file);
    moduleBase = moduleBase + instcount;
    moduleBaseTable[moduleCount+1] = moduleBase;
    if (moduleBase > MAX_INSTR) {
      __parseerror(6, lineNumber, offset);
      std::exit(EXIT_FAILURE);  
    }
    for (int i=0;i<instcount;i++) {
      std::string addressmode = readIEAR(&lineNumber, &offset, &lineOffset, line, file);
      int operand = readInt(&lineNumber, &offset, &lineOffset, line, file);
    }
    moduleCount++;
  }
  file.close();


  std::map<int, std::pair<std::string, int>> keysToModify;
  //for (const auto& pair: symbolTable) {
  //  if (pair.second > moduleBaseTable[symbolTable_modules[pair.first]]) {
  //    std::cout << "Warning: Module " << symbolTable_modules[pair.first] << ": " << 
  //      pair.first << " too big " <<
  //      pair.second - moduleBaseTable[symbolTable_modules[pair.first]-1]
  //      << " (max=" << moduleBaseTable[symbolTable_modules[pair.first]] - 
  //      moduleBaseTable[symbolTable_modules[pair.first]-1] -1<< ") assume zero relative" 
  //      << std::endl;
  //    keysToModify[pair.first] = moduleBaseTable[symbolTable_modules[pair.first]-1];
  //  }
  //}

  for (const auto& pair: symbolTable) {
    if (pair.second.second > moduleBaseTable[symbolTable_modules[pair.second.first]]) {
      std::cout << "Warning: Module " << symbolTable_modules[pair.second.first] << ": " << 
        pair.second.first << " too big " <<
        pair.second.second - moduleBaseTable[symbolTable_modules[pair.second.first]-1]
        << " (max=" << moduleBaseTable[symbolTable_modules[pair.second.first]] - 
        moduleBaseTable[symbolTable_modules[pair.second.first]-1] -1<< ") assume zero relative" 
        << std::endl;
      keysToModify[pair.first] = std::make_pair(pair.second.first, moduleBaseTable[symbolTable_modules[pair.second.first]-1]);
    }
  }





  for (const auto& pair: keysToModify) {
    symbolTable[pair.first] = pair.second;
  }

  std::cout << "Symbol Table" << std::endl;

  //for (const auto& pair : printoutOrder) {
  //  if (timesUsedMap[pair.second] > 1) {
  //    std::cout << pair.second << "=" << symbolTable[pair.second] <<
  //    " Error: This variable is multiple times defined; first value used"
  //    << std::endl;
  //  }
  //  else {
  //    std::cout << pair.second << "=" << symbolTable[pair.second] << std::endl;
  //  }
  //}
  for (const auto& pair : symbolTable) {
    if (timesUsedMap[pair.second.first] > 1) {
      std::cout << pair.second.first << "=" << pair.second.second <<
      " Error: This variable is multiple times defined; first value used"
      << std::endl;
    }
    else {
      std::cout << pair.second.first << "=" << pair.second.second << std::endl;
    }
  }

  std::cout << std::endl;


  //for (const auto& pair : symbolTable) {
  //  if (timesUsedMap[pair.first] > 1) {
  //    std::cout << pair.first << "=" << pair.second <<
  //      " Error: This variable is multiple times defined; first value used"
  //      << std::endl;
  //  }
  //  else {
  //    std::cout << pair.first << "=" << pair.second << std::endl;
  //  }
  //}
  //std::cout << std::endl;

}

void Pass2(char* filename, std::map<int, std::pair<std::string, int>>& symbolTable, 
           std::unordered_map<int, int>& moduleBaseTable,
           std::map<int, std::string>& printoutOrder) {

  struct symbolStruct {
    int timesUsed;
    bool actuallyUsed;
    std::string symbol;
    symbolStruct(int input_timesUsed,
                 bool input_actuallyUsed,
                 std::string input_symbol): 
      timesUsed(input_timesUsed), 
      actuallyUsed(input_actuallyUsed),
      symbol(input_symbol) {};
  };

  std::stringstream buffer;

  std::ifstream file(filename);
  std::map<std::string, int> timesUsedMap;
  std::map<std::string, std::pair<int, bool>> checkUsedMap;
  std::map<std::string, std::pair<std::pair<int, int>, bool>> checkActuallyUsedMap;
  std::map<int, bool> usedForInstrMap;
  std::map<int, std::pair<int, symbolStruct>> symbolStructMap;
  std::map<int, std::pair<int, int>> timesUsedMap2;
  //Copy the keys from the original map to the new map
  for (const auto& pair : symbolTable) {
    checkUsedMap[pair.second.first] = std::make_pair(-1,false);
  }
  for (const auto& pair : symbolTable) {
    checkActuallyUsedMap[pair.second.first] = std::make_pair(std::make_pair(-1,-1),false);
  }

  std::map<std::string, int> symbolTable_simple;
  for (const auto& pair : symbolTable) {
    symbolTable_simple[pair.second.first] = pair.second.second;
  }


  std::map<int, std::map<int, std::string>> useList;
  std::map<int, std::map<int, std::pair<std::string, bool>>> useListCount;
  std::map<int, int> memoryMap;
  std::map<int, std::string> memoryMapErrors;
  int moduleBase = 0;
  int moduleCount = 0;
  int location;
  int lineLength = 0;
  char line[BUFFER_SIZE];
  char* token;
  int lineNumber = 0;
  int offset = 0;
  int lineOffset;

  
  std::cout << "Memory Map" << std::endl;
  while (checkEOF(file) == 1) {  

    int defcount = readInt(&lineNumber, &offset, &lineOffset, line, file);
    for (int i=0;i<defcount; i++) {
      std::string sym = readSymbol(&lineNumber, &offset, &lineOffset, line, file);
      int relativeWordAdress = readInt(&lineNumber, &offset, &lineOffset, line, file); 
      if (timesUsedMap[sym] == 0) {
        checkUsedMap[sym].first = moduleCount;
        timesUsedMap[sym]++;
      } 
      else {
        timesUsedMap[sym]++;
      }
    }

    int usecount = readInt(&lineNumber, &offset, &lineOffset, line, file);
    for (int i=0;i<usecount;i++) {
      std::string sym = readSymbol(&lineNumber, &offset, &lineOffset, line, file);

      useList[moduleCount][i] = sym;
      useListCount[moduleCount][i] = std::make_pair(sym, false);
      //checkUsedMap[sym] = std::make_pair(moduleCount, true); terug aanzetten anders
      //checkUsedMap[sym].first = true;
      checkActuallyUsedMap[sym].first = std::make_pair(moduleCount, usecount);
    }

    int instcount = readInt(&lineNumber, &offset, &lineOffset, line, file);
    for (int i=0;i<instcount;i++) {
      std::string addressmode = readIEAR(&lineNumber, &offset, &lineOffset, line, file);
      int operand = readInt(&lineNumber, &offset, &lineOffset, line, file);
      if (operand >= 10000) {
        memoryMapErrors[i+moduleBase] = " Error: Illegal opcode; treated as 9999";
        operand = 9999;
      }
      if (addressmode == "R") {
        if ((operand%1000 > moduleBaseTable[moduleCount+1] - moduleBaseTable[moduleCount]) && memoryMapErrors.count(i+moduleBase) == 0) {
          memoryMap[i+moduleBase] = 1000*(operand/1000) + moduleBase;
          memoryMapErrors[i+moduleBase] = " Error: Relative address exceeds module size; relative zero used";
        }
        else {
          memoryMap[i+moduleBase] = operand + moduleBase; 
        }
      }
      else if (addressmode == "A") {
        if (operand%1000 < MAX_INSTR) {
          memoryMap[i+moduleBase] = operand;
        }
        else if (memoryMapErrors.count(i+moduleBase) == 0){
          memoryMap[i+moduleBase] = 1000*(operand/1000) + 0;
          memoryMapErrors[i+moduleBase] = " Error: Absolute address exceeds machine size; zero used";
        }
        else { // corresponds to 9999 having taken precedence to the absolute adress exceeding machine size
          memoryMap[i+moduleBase] = operand;
        }
      }
      else if (addressmode == "I") {
        if (operand%1000 >= 900 && memoryMapErrors.count(i+moduleBase) == 0) {
          memoryMapErrors[i+moduleBase] = " Error: Illegal immediate operand; treated as 999";
          operand = 1000*(operand/1000) + 999;
          memoryMap[i+moduleBase] = 1000*(operand/1000) + 999;
        }
        else {
          memoryMap[i+moduleBase] = operand;
        }
      }   
      else if (addressmode == "E") {
        if (operand%1000 > usecount - 1 || usecount == 0) {
          memoryMapErrors[i+moduleBase] = " Error: External operand exceeds length of uselist; treated as relative=0";
          memoryMap[i+moduleBase] = 1000*(operand/1000) + moduleBase;
          usedForInstrMap[0] = true;
        }
        else if (symbolTable_simple.count(useList[moduleCount][operand%1000]) == 0) {
          memoryMapErrors[i+moduleBase] = " Error: " + useList[moduleCount][operand%1000] + 
            " is not defined; zero used";
          //checkActuallyUsedMap[useList[moduleCount][operand%1000]].second = true;
          //= std::make_pair(std::make_pair((operand%1000), moduleCount), true);
          memoryMap[i+moduleBase] = 1000*(operand/1000);
          usedForInstrMap[0] = true;
          //useListCount[moduleCount][operand%1000].second = true;
          useListCount[moduleCount][0].second = true;
          checkUsedMap[useList[moduleCount][0]] = std::make_pair(moduleCount, true);
        }
        else {
          checkActuallyUsedMap[useList[moduleCount][operand%1000]].second = true;
          memoryMap[i+moduleBase] = 1000*(operand/1000) + symbolTable_simple[useList[moduleCount][operand%1000]];
          usedForInstrMap[operand%1000] = true;
          useListCount[moduleCount][operand%1000].second = true;
          checkUsedMap[useList[moduleCount][operand%1000]] = std::make_pair(moduleCount, true);
          //checkUsedMap[useList[moduleCount][operand%1000]] = std::make_pair(moduleCount, true); // mss er uit
        }
      } 
      else if (addressmode == "M") {

        //if (moduleBaseTable.count(operand%1000+1) != 0){
        if (operand%1000 < moduleBaseTable.size()-1) {
          //if (moduleBaseTable[operand%1000+1] < 512) {
            memoryMap[i+moduleBase] = 1000*(operand/1000)+ moduleBaseTable[operand%1000];
          //}
          //else {
          //  memoryMap[i+moduleBase] = 1000*(operand/1000);
          //  memoryMapErrors[i+moduleBase] = " Error: Illegal module operand ; treated as module=0";
          //}
        }
        else {//if (memoryMapErrors.count(operand%1000+1) == 0)
          memoryMap[i+moduleBase] = 1000*(operand/1000);
          memoryMapErrors[i+moduleBase] = " Error: Illegal module operand ; treated as module=0";
        }
        

      }
    }
   
    for (const auto& pair : memoryMap) {
      if (pair.first < moduleBase+instcount && pair.first >= moduleBase) {
        printf("%03d: %04d", pair.first, pair.second);
        std::cout << memoryMapErrors[pair.first] << std::endl;
      }
    }
    for (const auto& pair: useListCount[moduleCount]) {
      //if (checkUsedMap[pair.second.first].second == true) {
        if (pair.second.second == false) {
          std::cout << "Warning: Module " 
            << moduleCount+1 << ": " <<
          "uselist[" << pair.first << "]=" << pair.second.first 
            << " was not used" << std::endl;
        }
      //}
    }
    moduleBase = moduleBase + instcount; 
    moduleCount++;
  }


  for (const auto& pair: checkActuallyUsedMap) {
    if (pair.second.second == false) {
      if (pair.second.first.second != -1) {
      }
    } 
  }
  std::cout << std::endl;


  //for (const auto& pair: checkUsedMap) {
  //  if (pair.second.second == false) {
  //    std::cout << "Warning: Module " 
  //      << pair.second.first + 1 << ": " << pair.first 
  //      << " was defined but never used" << std::endl;
  //  } 
  //}

  for (const auto& pair: printoutOrder) {
    if (checkUsedMap[pair.second].second == false) {
      std::cout << "Warning: Module " 
        << checkUsedMap[pair.second].first + 1 << ": " << pair.second
        << " was defined but never used" << std::endl;
    } 
  }

  std::cout << std::endl;
}


int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }
  std::stringstream outputStream;
  std::streambuf* originalStderr = std::cerr.rdbuf(outputStream.rdbuf());
 
  std::map<int, std::pair<std::string, int>> symbolTable;
  std::unordered_map<int, int> moduleBaseTable;
  std::map<int, std::string> printoutOrder;
 
  Pass1(argv[1], symbolTable, moduleBaseTable, printoutOrder);
  
 
  Pass2(argv[1], symbolTable, moduleBaseTable, printoutOrder);
  
  return 0;
}
