#include <iostream>
#include <fstream>
#include <vector>

using namespace std;
typedef unsigned int uint32_t;
typedef int int32_t;

#define IN_FILE "example.dat"

class Dictionary
{
public:
  class CodeBlock 
  {
    public:
      CodeBlock (uint32_t a, uint32_t b, uint32_t c)
      : code(a),
        parent_code(b),
        char_value(c)
      {
      };
      int32_t code;
      int32_t parent_code;
      int32_t char_value;
  };
  enum { BITS = 12 ,
         MAX_CODE = (1<<BITS)-1,
         TABLE_SIZE = 5021, // a prime number
         UNUSED = -1 // a prime number
       }; 
  Dictionary();
  int32_t GetChildNode(int32_t paren_code, char c);
  CodeBlock& GetCodeBlock(int32_t index);
  bool AddPhrase(int32_t index, int32_t parent_code, char c);
  void ClearDictionary();

private:
  vector<CodeBlock> CodeBook ;
  int32_t n_codeBlocks;
};

Dictionary::Dictionary()
{
  n_codeBlocks = 257;
  CodeBook.clear();
  CodeBook.resize(TABLE_SIZE, CodeBlock(UNUSED,0,0));
}

int32_t
Dictionary::GetChildNode(int32_t parent_code, char c)
{
  int32_t index;
  int32_t offset;
  index = ( (c&0xFF) << (BITS-8)) ^ (parent_code & ((1<<BITS)-1));
  if (index == 0)
    offset = 1;
  else
    offset = (TABLE_SIZE - index);
  
  for (;;){
    //cout << hex << "Dict: index: " <<index << endl;
    if ( CodeBook[index].code == UNUSED || 
         ( CodeBook[index].parent_code == parent_code &&
           CodeBook[index].char_value == c ) )
      return index;
    index = index -offset;
    if (index < 0 )
      index = index + TABLE_SIZE;
  }
}

Dictionary::CodeBlock&
Dictionary::GetCodeBlock(int32_t index)
{
  return CodeBook[index];
}
bool
Dictionary::AddPhrase(int32_t index, int32_t parent_code,char c)
{
  if (n_codeBlocks > MAX_CODE)
    return false;
  CodeBook[index].code = n_codeBlocks++;
  CodeBook[index].parent_code = parent_code;
  CodeBook[index].char_value = c;
  cout << hex<< "Dict. Statistics: codeBlock: " << n_codeBlocks << " Last char: " << c << " Last P code: "<< parent_code << endl;
  return true;
}
void
Dictionary::ClearDictionary ()
{
  n_codeBlocks = 257;
  CodeBook.clear();
  CodeBook.resize(TABLE_SIZE, CodeBlock(UNUSED,0,0));
}

int main()
{
  ifstream inFile;
  ofstream outFile;
  Dictionary dict;

  inFile.open(IN_FILE,ifstream::binary);
  outFile.open("LZW.dat",ifstream::binary);
  
  int32_t scratchPad = 0;
  bool spState = true;
  int32_t index;
  int32_t cPrev = inFile.get();
  while ( inFile.good() )
  { 
    char c; 
    c = inFile.get();
    
    index = dict.GetChildNode(cPrev,c);  
    
    Dictionary::CodeBlock code = dict.GetCodeBlock(index);
    if (code.code != -1)
    {
      cPrev = code.code;   
    }
    else
    {
      bool succ = dict.AddPhrase(index,cPrev,c);
      if (!succ) {
        dict.ClearDictionary();
        cPrev = c & 0xFF;
        continue;
      }
      scratchPad = scratchPad | ((cPrev & 0xfff) << (spState*12)); 
      if (!spState) {
        outFile.put((scratchPad >> 16) & 0xFF);
        outFile.put((scratchPad >> 8) & 0xFF);
        outFile.put( scratchPad & 0xFF);
        cout << hex << scratchPad << endl;
        scratchPad = 0;
      }
      spState = (!spState);
      cPrev = c & 0xFF;
    }
  }
  outFile.put((scratchPad >> 4) & 0xFF);
  outFile.put((scratchPad << 4) & 0xFF);
  inFile.close();
  outFile.close();  
}
