/*
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

static int32_t lastAddr = 0x0;

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
    int lineNumber = 1;

    while(getline(inf,line)) 
    {
        if (hasErrors(line)) 
        {
            std::cout << "Error on line " << std::dec << lineNumber
                << ": " << line << std::endl;
            return;
        }
        if(line[ADDRBEGIN] != 0x20  && line[DATABEGIN] != 0x20) //if both address field and data field are not blank
        {
            int32_t addr = convert(line, ADDRBEGIN, ADDREND + 1); //calculate the address
            for(int i = DATABEGIN; i < COMMENT - 2; i += 2) //loop to add to memory byte by byte
            {
                if(line[i] != 0x20) 
                {
                    int8_t data = convert(line, i, i + 2);
                    mem->putByte(data,addr,error);
                    addr ++;
                }
                lastAddr = addr;
            }
        }
        lineNumber ++;
    }

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

//Check if given line is a comment record.
bool Loader::isCommentRecord(std::string line)
{
    for(int i = 0; i < COMMENT - 1; i ++)
    {
        if(line[i] != 0x20) return 0;
    }
    return line[COMMENT] == '|';
}

//Check if given line is a data record.
bool Loader::isDataRecord(std::string line)
{
    for(int i = 0; i < ADDREND; i ++)
    {
        if(line[i] == 0x20) return 0;
    }
    return line[ADDREND + 1] == ':' 
        && line[DATABEGIN - 1] == 0x20 
        && line[COMMENT - 1] == 0X20 
        && line[COMMENT] == '|';
}

//Check if given line is valid wrt memory.
bool Loader::isValidAddress(std::string line)
{
    if(isDataRecord(line))
    {
        std::string addr = line.substr(ADDRBEGIN, 3);
        int32_t addrNum = convert(line,ADDRBEGIN, ADDREND + 1); 
        return line[0] == 48 && line[1] == 'x' && isHexAddr(addr)
            && addrNum >= lastAddr && (addrNum + (getDataSize(line)/2) <= MEMSIZE);
    }
    return 1;
}


//Returns true if line has valid memory state and is either a comment record or a data record.
bool Loader::hasErrors(std::string line)
{
    return !((isCommentRecord(line) 
                ^ isDataRecord(line)) 
            && isValidData(line) && isValidAddress(line));
}

//Gets the number of hex digits in the data section of a line
int Loader::getDataSize(std::string line)
{
    const char* ptr = line.c_str();
    const char* trav = &ptr[DATABEGIN];
    int count = 0;
    while (*trav != 0x20)
    {
        trav ++;
        count ++;
    }
    return count;
}

/**Iterates over the data section of the given string and checks if each character is a hex value. Returns true if yes.
 * @param : A single line of a valid .yo file.
 */
bool Loader::isHexData(std::string line)
{
    int dataSize = getDataSize(line);
    for(int i = DATABEGIN; i < DATABEGIN + dataSize ; i ++)
    {

        if (!isxdigit(line[i])) return 0;

    }
    return 1;
}

//C
bool Loader::isHexAddr(std::string addr)
{
    for(int i = 0; i < 3; i ++)
    {
        if(!isxdigit(addr[i])) return 0;
    }
    return 1;
}

//Checks if data is ordered properly. No spaces in between bytes.Data should be hex and even in length. 
//Returns true if all conditions are met.
bool Loader::isValidData(std::string line)
{
    if(line[DATABEGIN] == 0x20) //if the first character is a whitespace, then there should be no other characters
    {
        for(int i = DATABEGIN + 1; i < COMMENT; i ++)
        {
            if(line[i] != 0x20) return 0;
        }
    }
    else
    {
        for(int i = DATABEGIN + 1; i < COMMENT; i ++)   //otherwise if at somepoint there is whitespace
            // make sure there are no characters after it has occured.
        {
            if(line[i] == 0x20)
            {
                for(int j = i; j < COMMENT; j ++)
                {
                    if(line[j] != 0x20) return 0;
                }
            }
        }
    }     
    return (getDataSize(line) % 2 == 0) && isHexData(line); 
}

