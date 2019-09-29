/**
 * Names: Nischinth Murari
 * Team:31
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>

#include "Loader.h"
#include "Memory.h"

//first column in file is assumed to be 0
#define ADDRBEGIN 2   //starting column of 3 digit hex address 
#define ADDREND 4     //ending column of 3 digit hext address
#define DATABEGIN 7   //starting column of data bytes
#define COMMENT 28    //location of the '|' character 



/**
 * Loader constructor
 * Opens the .yo file named in the command line arguments, reads the contents of the file
 * line by line and loads the program into memory.  If no file is given or the file doesn't
 * exist or the file doesn't end with a .yo extension or the .yo file contains errors then
 * loaded is set to false.  Otherwise loaded is set to true.
 *
 * @param argc is the number of command line arguments passed to the main; should
 *        be 2
 * @param argv[0] is the name of the executable
 *        argv[1] is the name of the .yo file
 */
Loader::Loader(int argc, char * argv[])
{
    loaded = false;

    if(!Loader::isValidFile(argv[1])) return;
    Loader::inf.open(argv[1]);
    if(!Loader::inf.is_open()) return;
    std::string line;
    bool error = false;
    Memory* mem = Memory::getInstance();          

    while(getline(inf,line)) 
    {
        const char* ptr = line.c_str();
        if(ptr[ADDRBEGIN] != 0x20  && ptr[DATABEGIN] != 0x20)
        {
            int32_t addr = Loader::convert(line, ADDRBEGIN, ADDREND + 1);
            for(int i = DATABEGIN; i < COMMENT - 2; i += 2)
            {
                if(ptr[i] != 0x20)
                {
                    int8_t data = Loader::convert(line, i, i + 2);
                    mem->putByte(data,addr,error);
                    addr ++;
                }
            }
        }
    }
    //Finally, add code to check for errors in the input line.
    //When your code finds an error, you need to print an error message and return.
    //Since your output has to be identical to your instructor's, use this cout to print the
    //error message.  Change the variable names if you use different ones.
    //  std::cout << "Error on line " << std::dec << lineNumber
    //       << ": " << line << std::endl;


    //If control reaches here then no error was found and the program
    //was loaded into memory.
    loaded = true;  

}

/**
 * isLoaded
 * returns the value of the loaded data member; loaded is set by the constructor
 *
 * @return value of loaded (true or false)
 */
bool Loader::isLoaded()
{
    return loaded;
}


//You'll need to add more helper methods to this file.  Don't put all of your code in the
//Loader constructor.  When you add a method here, add the prototype to Loader.h in the private
//section.


//Returns true if file has extension ".yo" and false otherwise.
bool Loader::isValidFile(char* s)
{
    if(strlen(s) < 4) return 0;

    char* ptr = s;
    while(*ptr != '\0')
    {
        ptr ++;
    }
    ptr --;
    return *ptr == 'o' && *(ptr - 1) == 'y' && *(ptr - 2) == '.';
}

//Converts a string to hex value from index begin to index end.
int Loader::convert(std::string l, int  begin, int end)
{
    std::string part = l.substr(begin, (end - begin));  
    return std::stoul(part, 0 , 16);
}
