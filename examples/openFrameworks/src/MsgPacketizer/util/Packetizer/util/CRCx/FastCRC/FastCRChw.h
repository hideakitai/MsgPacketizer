// 2020.04.24 hideakitai
// for portability, change to header only

/* FastCRC library code is placed under the MIT license
 * Copyright (c) 2014-2019 Frank Bösing
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


// Teensy 3.0, Teensy 3.1:
// See K20P64M72SF1RM.pdf (Kinetis), Pages 638 - 641 for documentation of CRC Device
// See KINETIS_4N30D.pdf for Errata (Errata ID 2776)
//
// So, ALL HW-calculations are done as 32 bit.
//
//
//
// Thanks to:
// - Catalogue of parametrised CRC algorithms, CRC RevEng
// http://reveng.sourceforge.net/crc-catalogue/
//
// - Danjel McGougan (CRC-Table-Generator)
//

#include <Arduino.h>

// Set this to 0 for smaller 32BIT-CRC-Tables:
#define CRC_BIGTABLES 1


#if !defined(FastCRC_h)
#define FastCRC_h
#include "inttypes.h"


// ================= DEFINES ===================
#if defined(KINETISK)
#define CRC_SW 0
#define CRC_FLAG_NOREFLECT         (((1<<31) | (1<<30)) | ((0<<29) | (0<<28))) //refin=false refout=false
#define CRC_FLAG_REFLECT           (((1<<31) | (0<<30)) | ((1<<29) | (0<<28))) //Reflect in- and outgoing bytes (refin=true refout=true)
#define CRC_FLAG_XOR               (1<<26)                                     //Perform XOR on result
#define CRC_FLAG_NOREFLECT_8       (0)                                         //For 8-Bit CRC
#define CRC_FLAG_REFLECT_SWAP      (((1<<31) | (0<<30)) | ((0<<29) | (1<<28))) //For 16-Bit CRC (byteswap)
#else
#define CRC_SW 1
#endif


#if defined(KINETISK)

#include "mk20dx128.h"

// ===============================================

typedef struct {
  union {
	  uint32_t CRC; //CRC Data register
	  struct {
		  uint16_t CRC16;
		  uint16_t CRC16_1;
	  };
	  struct {
		  uint8_t CRC8;
		  uint8_t CRC8_1;
		  uint8_t CRC8_2;
		  uint8_t CRC8_3;
	  };
  };
  uint32_t GPOLY; //CRC Polynomial register
  uint32_t CTRL;  //CRC Control register
} CRC_T;

static volatile CRC_T * const rCRC = (CRC_T *)0x40032000;

#define CRC_CTRL_WAS   25 // Write CRC Data Register As Seed(1) / Data(0)
#define CRC_CTRL_TCRC  24 // Width of CRC protocol (0=16 BIT, 1=32 BIT)
#define CRC_CTRL_TOTR1 29 // TOTR[1]


// ================= 7-BIT CRC ===================
class FastCRC7
{
public:

  /** Constructor
   * Enables CRC-clock
   */
  FastCRC7(){
    SIM_SCGC6 |= SIM_SCGC6_CRC;
  }

  /** CRC 7
   * MultiMediaCard interface
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t crc7(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x09 init=0x00 refin=false refout=false xorout=0x00 check=0x75
    return (generic(0x09, 0, CRC_FLAG_NOREFLECT, data, datalen));
  }

  uint8_t crc7_upd(const uint8_t *data, uint16_t datalen){return update(data, datalen);}

#if !CRC_SW
  /** generic function for all 7-Bit CRCs
   * @param polynom Polynom
   * @param seed Seed
   * @param flags Flags
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t generic(const uint8_t polynom, const uint8_t seed, const uint32_t flags, const uint8_t *data,const uint16_t datalen)
  {

    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC) | (1<<CRC_CTRL_WAS);                      // 32Bit Mode, Prepare to write seed(25)
    rCRC->GPOLY = ((uint32_t)polynom)<<(24 + 1);                                       // Set polynom
    rCRC->CRC   = ((uint32_t)seed<<(24 + 1));                                          // Write seed
    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC);								  		  	 // Clear WAS Bit - prepare to write data

    return update(data, datalen);
  }
#endif
private:
#if CRC_SW
  uint8_t seed;
#else
  /** Update
   * Call for subsequent calculations with previous seed
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t update(const uint8_t *data, const uint16_t datalen)
  {

    const uint8_t *src = data;
    const uint8_t *target = src + datalen;

    while (((uintptr_t)src & 0x03) != 0 && (src < target)) {
      rCRC->CRC8_3 = *src++; //Write 8 BIT
    }

    while (src <= target-4) {
      rCRC->CRC = *( uint32_t  *)src; //Write 32 BIT
      src += 4;
    }

    while (src < target) {
      rCRC->CRC8_3 = *src++; //Write 8 Bit
    }

  //TODO: Check handling of  CRC_CTRL_TOTR1 for other CRC7s
  /*
    if (rCRC->CTRL & (1<<CRC_CTRL_TOTR1))
      return rCRC->CRC8 >> 1;
    else
  */
      return rCRC->CRC8_3 >> 1;
  }
#endif
};

// ================= 8-BIT CRC ===================

class FastCRC8
{
public:
  /** Constructor
   * Enables CRC-clock
   */
  FastCRC8(){
    SIM_SCGC6 |= SIM_SCGC6_CRC;
  }

  /** SMBUS CRC
   * aka CRC-8
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t smbus(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x07 init=0x00 refin=false refout=false xorout=0x00 check=0xf4
    return generic(0x07, 0, CRC_FLAG_NOREFLECT, data, datalen);
  }

  /** MAXIM 8-Bit CRC
   * equivalent to _crc_ibutton_update() in crc16.h from avr_libc
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t maxim(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x31 init=0x00 refin=true refout=true xorout=0x00  check=0xa1
    return generic(0x31, 0, CRC_FLAG_REFLECT, data, datalen);
  }

  uint8_t smbus_upd(const uint8_t *data, uint16_t datalen){return update(data, datalen);}
  uint8_t maxim_upd(const uint8_t *data, uint16_t datalen){return update(data, datalen);}

#if !CRC_SW
  /** generic function for all 8-Bit CRCs
   * @param polynom Polynom
   * @param seed Seed
   * @param flags Flags
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t generic(const uint8_t polynom, const uint8_t seed, const uint32_t flags, const uint8_t *data,const uint16_t datalen)
  {

    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC) | (1<<CRC_CTRL_WAS);                      // 32Bit Mode, Prepare to write seed(25)
    rCRC->GPOLY = ((uint32_t)polynom)<<24;                                             // Set polynom
    rCRC->CRC   = ((uint32_t)seed<<24);                                                // Write seed
    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC);								  		  	 // Clear WAS Bit - prepare to write data

    return update(data, datalen);
  }
#endif
private:
#if CRC_SW
  uint8_t seed;
#else
  /** Update
   * Call for subsequent calculations with previous seed
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t update(const uint8_t *data, const uint16_t datalen)
  {

    const uint8_t *src = data;
    const uint8_t *target = src + datalen;

    while (((uintptr_t)src & 0x03) != 0 && (src < target)) {
      rCRC->CRC8_3 = *src++; //Write 8 BIT
    }

    while (src <= target-4) {
      rCRC->CRC = *( uint32_t  *)src; //Write 32 BIT
      src += 4;
    }

    while (src < target) {
      rCRC->CRC8_3 = *src++; //Write 8 Bit
    }

    if (rCRC->CTRL & (1<<CRC_CTRL_TOTR1))
      return rCRC->CRC8;
    else
      return rCRC->CRC8_3;
  }
#endif
};

// ================= 14-BIT CRC ===================

class FastCRC14
{
public:
#if !CRC_SW //NO Software-implemenation so far
  /** Constructor
   * Enables CRC-clock
   */
  FastCRC14(){
    SIM_SCGC6 |= SIM_SCGC6_CRC;
  }

  /** CRC-14/DARC
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t darc(const uint8_t *data,const uint16_t datalen)
  {
  // poly=0x0805 init=0x0000 refin=true refout=true xorout=0x0000 check=0x082d residue=0x0000
    return generic(0x0805, 0x0000, CRC_FLAG_REFLECT, data, datalen);
  }

  /** CRC-14/GSM
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t gsm(const uint8_t *data,const uint16_t datalen)
  {
  //  poly=0x202d init=0x0000 refin=false refout=false xorout=0x3fff check=0x30ae residue=0x031e
    return generic(0x202d, 0x0000, CRC_FLAG_NOREFLECT | CRC_FLAG_XOR, data, datalen);
  }

  /** CRC-14/ELORAN
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t eloran(const uint8_t *data,const uint16_t datalen)
  {
  // poly=0x60b1 init=0x0000 refin=false refout=false xorout=0x0000 check=0x38d1
    return generic(0x60b1, 0x0, CRC_FLAG_NOREFLECT , data, datalen);
  }

  /** CRC-14/ft4 : TODO
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  /*
  uint16_t ft4(const uint8_t *data,const uint16_t datalen)
  {

    return generic(, ,  , data, datalen);
  }
  */

  uint16_t darc_upd(const uint8_t *data, uint16_t len)  {return update(data, len);}
  uint16_t gsm_upd(const uint8_t *data, uint16_t len)  {return update(data, len);}
  uint16_t eloran_upd(const uint8_t *data, uint16_t len)  {return update(data, len);}
  //uint16_t ft4(const uint8_t *data, uint16_t len)  {return update(data, len);}
#endif
#if !CRC_SW
  /** generic function for all 14-Bit CRCs
   * @param polynom Polynom
   * @param seed Seed
   * @param flags Flags
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t generic(const uint16_t polynom, const uint16_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen)
  {

    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC) | (1<<CRC_CTRL_WAS);// 32-Bit Mode, prepare to write seed(25)
    rCRC->GPOLY = ((uint32_t)polynom) << (32 - 14);                       // set polynom
    rCRC->CRC   = ((uint32_t)seed << (32 - 14) );                          // this is the seed
    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC);				  	   // Clear WAS Bit - prepare to write data

    return update(data, datalen);
  }
#endif
private:
#if CRC_SW
  uint16_t seed;
#else
  /** Update
   * Call for subsequent calculations with previous seed
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t update(const uint8_t *data, const uint16_t datalen)
  {
    const uint8_t *src = data;
    const uint8_t *target = src + datalen;

    while (((uintptr_t)src & 0x03) !=0 && (src < target))  {
      rCRC->CRC8_3 = *src++; //Write 8 BIT
    }

    while (src <= target-4) {
      rCRC->CRC = *( uint32_t  *)src; //Write 32 BIT
      src += 4;
    }

    while (src < target) {
      rCRC->CRC8_3 = *src++; //Write 8 Bit
    }

    if (rCRC->CTRL & (1<<CRC_CTRL_TOTR1))
      return rCRC->CRC16;
    else
      return rCRC->CRC >> (32 - 14);
  }
#endif
};

// ================= 16-BIT CRC ===================

class FastCRC16
{
public:

  /** Constructor
   * Enables CRC-clock
   */
  FastCRC16(){
    SIM_SCGC6 |= SIM_SCGC6_CRC;
  }

  /** CCITT
   * Alias "false CCITT"
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t ccitt(const uint8_t *data,const uint16_t datalen)
  {
  // poly=0x1021 init=0xffff refin=false refout=false xorout=0x0000 check=0x29b1
    return generic(0x1021, 0XFFFF, CRC_FLAG_NOREFLECT, data, datalen);
  }

  /** MCRF4XX
   * equivalent to _crc_ccitt_update() in crc16.h from avr_libc
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t mcrf4xx(const uint8_t *data,const uint16_t datalen)
  {
  // poly=0x1021 init=0xffff refin=true refout=true xorout=0x0000 check=0x6f91
    return generic(0x1021, 0XFFFF, CRC_FLAG_REFLECT , data, datalen);
  }

  /** KERMIT
   * Alias CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-CCITT
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t kermit(const uint8_t *data, const uint16_t datalen)
  {
  // poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000 check=0x2189
  // sometimes byteswapped presentation of result
    return generic(0x1021, 0x00, CRC_FLAG_REFLECT, data, datalen);
  }

  /** MODBUS
   * equivalent to _crc_16_update() in crc16.h from avr_libc
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t modbus(const uint8_t *data, const uint16_t datalen)
  {
  // poly=0x8005 init=0xffff refin=true refout=true xorout=0x0000 check=0x4b37
    return generic(0x8005, 0XFFFF, CRC_FLAG_REFLECT, data, datalen);
  }

  /** XMODEM
   * Alias ZMODEM, CRC-16/ACORN
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t xmodem(const uint8_t *data, const uint16_t datalen)
  {
    //width=16 poly=0x1021 init=0x0000 refin=false refout=false xorout=0x0000 check=0x31c3
    return generic(0x1021, 0, CRC_FLAG_NOREFLECT, data, datalen);
  }

  /** X25
   * Alias CRC-16/IBM-SDLC, CRC-16/ISO-HDLC, CRC-B
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t x25(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x1021 init=0xffff refin=true refout=true xorout=0xffff check=0x906e
    return generic(0x1021, 0XFFFF, CRC_FLAG_REFLECT | CRC_FLAG_XOR, data, datalen);
  }

  uint16_t ccitt_upd(const uint8_t *data, uint16_t len)  {return update(data, len);}
  uint16_t mcrf4xx_upd(const uint8_t *data, uint16_t len){return update(data, len);}
  uint16_t kermit_upd(const uint8_t *data, uint16_t len) {return update(data, len);}
  uint16_t modbus_upd(const uint8_t *data, uint16_t len) {return update(data, len);}
  uint16_t xmodem_upd(const uint8_t *data, uint16_t len) {return update(data, len);}
  uint16_t x25_upd(const uint8_t *data, uint16_t len)    {return update(data, len);}

#if !CRC_SW
  /** generic function for all 16-Bit CRCs
   * @param polynom Polynom
   * @param seed Seed
   * @param flags Flags
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t generic(const uint16_t polynom, const uint16_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen)
  {

    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC) | (1<<CRC_CTRL_WAS);// 32-Bit Mode, prepare to write seed(25)
    rCRC->GPOLY = ((uint32_t)polynom)<<16;                       // set polynom
    rCRC->CRC   = ((uint32_t)seed<<16);                          // this is the seed
    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC);				  	   // Clear WAS Bit - prepare to write data

    return update(data, datalen);
  }
#endif
private:
#if CRC_SW
  uint16_t seed;
#else
  /** Update
   * Call for subsequent calculations with previous seed
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t update(const uint8_t *data, const uint16_t datalen)
  {
    const uint8_t *src = data;
    const uint8_t *target = src + datalen;

    while (((uintptr_t)src & 0x03) !=0 && (src < target))  {
      rCRC->CRC8_3 = *src++; //Write 8 BIT
    }

    while (src <= target-4) {
      rCRC->CRC = *( uint32_t  *)src; //Write 32 BIT
      src += 4;
    }

    while (src < target) {
      rCRC->CRC8_3 = *src++; //Write 8 Bit
    }

    if (rCRC->CTRL & (1<<CRC_CTRL_TOTR1))
      return rCRC->CRC16;
    else
      return rCRC->CRC16_1;
  }
#endif
};

// ================= 32-BIT CRC ===================

class FastCRC32
{
public:

  /** Constructor
   * Enables CRC-clock
   */
  FastCRC32(){
    SIM_SCGC6 |= SIM_SCGC6_CRC;
  }

  /** CRC32
   * Alias CRC-32/ADCCP, PKZIP, Ethernet, 802.3
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint32_t crc32(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x04c11db7 init=0xffffffff refin=true refout=true xorout=0xffffffff check=0xcbf43926
    return generic(0x04C11DB7L, 0XFFFFFFFFL, CRC_FLAG_REFLECT | CRC_FLAG_XOR, data, datalen);
  }

  /** CKSUM
   * Alias CRC-32/POSIX
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint32_t cksum(const uint8_t *data, const uint16_t datalen)
  {
    // width=32 poly=0x04c11db7 init=0x00000000 refin=false refout=false xorout=0xffffffff check=0x765e7680
    return generic(0x04C11DB7L, 0, CRC_FLAG_NOREFLECT | CRC_FLAG_XOR, data, datalen);
  }

  uint32_t crc32_upd(const uint8_t *data, uint16_t len){return update(data, len);}
  uint32_t cksum_upd(const uint8_t *data, uint16_t len){return update(data, len);}

#if !CRC_SW
  /** generic function for all 32-Bit CRCs
   * @param polynom Polynom
   * @param seed Seed
   * @param flags Flags
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint32_t generic(const uint32_t polynom, const uint32_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen)
  {

    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC) | (1<<CRC_CTRL_WAS); // 32Bit Mode, prepare to write seed(25)
    rCRC->GPOLY = polynom;                                        // Set polynom
    rCRC->CRC   = seed;                                           // This is the seed
    rCRC->CTRL  = flags | (1<<CRC_CTRL_TCRC);                     // Clear WAS Bit - prepare to write data

    return update(data, datalen);
  }
#endif
private:
#if CRC_SW
  uint32_t seed;
#else
  /** Update
   * Call for subsequent calculations with previous seed
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  //#pragma GCC diagnostic ignored "-Wpointer-arith"
  uint32_t update(const uint8_t *data, const uint16_t datalen)
  {

    const uint8_t *src = data;
    const uint8_t *target = src + datalen;

    while (((uintptr_t)src & 0x03) != 0  && (src < target)) {
      rCRC->CRC8_3 = *src++; //Write 8 BIT
    }

    while (src <= target-4) {
      rCRC->CRC = *( uint32_t  *)src; //Write 32 BIT
      src += 4;
    }

    while (src < target) {
      rCRC->CRC8_3 = *src++; //Write 8 Bit
    }

    return rCRC->CRC;
  }

#endif
};

#endif // #if defined(KINETISK)

#endif
