#include <iostream>
#include <fstream>
//#include <vector>

using namespace std;
typedef unsigned int uint32_t;

#define IN_FILE "example.dat"

class RingBuffer
{
public:
  enum { PHRASE_SIZE = 2 }; 
  RingBuffer(uint32_t size, uint32_t sizeSB);
  uint32_t Add(char* buf, uint32_t len);
  uint32_t GetNextcode();
  uint32_t GetCurrentLABSize();

private:
  char buffer[2048 + 16];
  uint32_t size;
  uint32_t sizeLAB;// look head buffer size
  uint32_t c_sizeLAB;
  uint32_t sizeSearch;// search buffer size
  uint32_t startLAB;
  uint32_t matchPhrase(uint32_t pSB, uint32_t pLAB, uint32_t maxSize);
};

RingBuffer::RingBuffer (uint32_t size, uint32_t sizeSB)
: size (size),
  sizeLAB (size-sizeSB),
  c_sizeLAB (0),
  sizeSearch (sizeSB),
  startLAB(sizeSB)
{
  //empty
}

uint32_t
RingBuffer::Add (char* buf, uint32_t len)
{
  if ((c_sizeLAB + len) > sizeLAB)
    len = sizeLAB - c_sizeLAB;

  if ((startLAB + c_sizeLAB + len) <= size )
  {
  // wrap around not required
    for ( unsigned int i = 0 ; i < len ; i++)
      buffer[startLAB + c_sizeLAB +i] = buf[i];  
  }
  else 
  {
  // wrap around required
    int flow1 = size - startLAB -c_sizeLAB;
    int i;
    for ( i = 0 ; i< flow1 ; i++)
      buffer[startLAB + c_sizeLAB +i] = buf[i];  
    for ( ; i < len ; i++) 
      buffer[i-flow1] = buf[i];  
  }
  c_sizeLAB +=len; 
  return len;
}

uint32_t 
RingBuffer::GetNextcode()
{
  uint32_t index;
  uint32_t len = 0 ;
  uint32_t ret = 0 ;
  
  uint32_t j = startLAB == 0 ? (size-1) : (startLAB -1) ;
  for (int i = 0 ; i < sizeSearch ; i++) {
    int newLen = matchPhrase(j,startLAB,c_sizeLAB);
    if (newLen > len) {
      len = newLen;
      index = i;
    }  
    j = (j == 0) ? (size-1) : (j -1) ;
  }
  // 11 bits for SB index
  // 4 bits for length
  // MSB bit to indicate (0) symbol or (1) index
  if (len >= PHRASE_SIZE) {
    ret = ((0x80 | (len & 0x0F) << 3) | ((index >> 8) & 0x07) ) << 8 | index & 0xFF;  
  }
  else {
   ret = ( 0 << 8 ) | buffer[startLAB] & 0xFF;
  }
  startLAB = (startLAB + (len== 0 ? 1 : len) ) % size ;
  c_sizeLAB = c_sizeLAB - (len== 0 ? 1 : len) ;
  //cout << hex << "LAB Start: "<< startLAB << " LAB Size: " <<c_sizeLAB << endl;
  return ret;
}

uint32_t 
RingBuffer::GetCurrentLABSize()
{
  return  c_sizeLAB;
}

uint32_t 
RingBuffer::matchPhrase(uint32_t pSB, uint32_t pLAB, uint32_t maxSize)
{
  uint32_t len = 0;
  uint32_t indexSB = pSB;
  uint32_t indexLAB = pLAB;
  for (int i = 0 ; i < maxSize ; i++ )
  {
    if ( buffer[indexSB] != buffer[indexLAB] )
      break;
    len++;
    indexSB = (indexSB +1) % size;
    indexLAB = (indexLAB +1) % size;
  }
  return len;  
}

int main()
{
  ifstream inFile;
  ofstream outFile;
  uint32_t sizeLAB = ((1<<4) -1);
  uint32_t sizeTotal = (1<<10) + sizeLAB;
  RingBuffer lzssBuffer( sizeTotal, (1<<10));

  inFile.open(IN_FILE,ifstream::binary);
  outFile.open("LZSS.dat",ifstream::binary);
  uint32_t backUp = 0;
  uint32_t backUpLen = 0;
  while ( inFile.good() )
  { 
    int i = lzssBuffer.GetCurrentLABSize();
    for (int j = 0 ; j < (sizeLAB-i) ; j++ )
    {
      char c[1]; 
      c[0] = inFile.get();
     lzssBuffer.Add(c,1);
    }
    uint32_t code = lzssBuffer.GetNextcode();
    uint32_t ln;
    if (code & 0x08000)
      ln = 16;
    else
      ln = 9;
    if ((backUpLen + ln) <= 32)
    {
      backUp = (backUp << ln) | (code & ((1<<ln)-1));
      backUpLen +=ln;
    }
    else
    {
      uint32_t shift = 32-backUpLen;
      ln = ln - shift;
      backUp = (backUp << shift) | (code & ((1<<shift)-1));
 
      outFile.put((backUp >> 24) & 0xff);
      outFile.put((backUp >> 16) & 0xff);
      outFile.put((backUp >> 8) & 0xff);
      outFile.put(backUp & 0xff);
      backUp = 0;
      backUpLen = 0;
      backUp = (backUp << ln) | (code & ((1<<ln)-1));
      backUpLen +=ln;
    } 
  }
  while (lzssBuffer.GetCurrentLABSize())
  {
    uint32_t code = lzssBuffer.GetNextcode();
    char c = (code >> 8) & 0xFF;
    outFile.put(c);
    c = code & 0xFF;
    outFile.put(c); 
  
  }
  inFile.close();
  outFile.close();  
}
