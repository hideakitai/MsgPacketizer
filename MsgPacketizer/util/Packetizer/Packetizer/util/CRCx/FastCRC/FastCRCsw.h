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


#if !defined(KINETISK)

#include "FastCRC_cpu.h"
#include "FastCRC_tables.h"


// ================= 7-BIT CRC ===================
class FastCRC7
{
public:
  /** Constructor
   */
  FastCRC7(){}

  uint8_t crc7(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x09 init=0x00 refin=false refout=false xorout=0x00 check=0x75
    seed = 0x00;
    return crc7_upd(data, datalen);
  }

  /** SMBUS CRC
   * aka CRC-8
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t crc7_upd(const uint8_t *data, uint16_t datalen)
  {
    uint8_t crc = seed;
    if (datalen) do {
      crc = pgm_read_byte(&crc_table_crc7[crc ^ *data]);
      data++;
    } while (--datalen);
    seed = crc;
    return crc >> 1;
  }

#if !CRC_SW
  uint8_t generic(const uint8_t polyom, const uint8_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen); //Not available in non-hw-variant (not T3.x)
#endif
private:
#if CRC_SW
  uint8_t seed;
#else
  uint8_t update(const uint8_t *data, const uint16_t datalen);
#endif
};

// ================= 8-BIT CRC ===================

class FastCRC8
{
public:

  /** Constructor
   */
  FastCRC8(){}

  uint8_t smbus(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x07 init=0x00 refin=false refout=false xorout=0x00 check=0xf4
    seed = 0x00;
    return smbus_upd(data, datalen);
  }

  uint8_t maxim(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x31 init=0x00 refin=true refout=true xorout=0x00  check=0xa1
    seed = 0x00;
    return maxim_upd(data, datalen);
  }

  /** SMBUS CRC
   * aka CRC-8
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t smbus_upd(const uint8_t *data, uint16_t datalen)
  {
    uint8_t crc = seed;
    if (datalen) do {
      crc = pgm_read_byte(&crc_table_smbus[crc ^ *data]);
      data++;
    } while (--datalen);
    seed = crc;
    return crc;
  }

  /** MAXIM 8-Bit CRC
   * equivalent to _crc_ibutton_update() in crc16.h from avr_libc
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint8_t maxim_upd(const uint8_t *data, uint16_t datalen)
  {
    uint8_t crc = seed;
    if (datalen) do {
      crc = pgm_read_byte(&crc_table_maxim[crc ^ *data]);
      data++;
    } while (--datalen);
    seed = crc;
    return crc;
  }

#if !CRC_SW
  uint8_t generic(const uint8_t polyom, const uint8_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen); //Not available in non-hw-variant (not T3.x)
#endif
private:
#if CRC_SW
  uint8_t seed;
#else
  uint8_t update(const uint8_t *data, const uint16_t datalen);
#endif
};

// ================= 14-BIT CRC ===================

class FastCRC14
{
public:
#if !CRC_SW //NO Software-implemenation so far
  FastCRC14();
  uint16_t darc(const uint8_t *data, const uint16_t datalen);
  uint16_t gsm(const uint8_t *data, const uint16_t datalen);
  uint16_t eloran(const uint8_t *data, const uint16_t datalen);
  uint16_t ft4(const uint8_t *data, const uint16_t datalen);

  uint16_t darc_upd(const uint8_t *data, uint16_t len);
  uint16_t gsm_upd(const uint8_t *data, uint16_t len);
  uint16_t eloran_upd(const uint8_t *data, uint16_t len);
   uint16_t ft4_upd(const uint8_t *data, uint16_t len);
#endif
#if !CRC_SW
  uint16_t generic(const uint16_t polyom, const uint16_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen); //Not available in non-hw-variant (not T3.x)
#endif
private:
#if CRC_SW
  uint16_t seed;
#else
  uint16_t update(const uint8_t *data, const uint16_t datalen);
#endif
};

// ================= 16-BIT CRC ===================

#define crc_n4(crc, data, table) crc ^= data; \
	crc = pgm_read_word(&table[(crc & 0xff) + 0x300]) ^		\
	pgm_read_word(&table[((crc >> 8) & 0xff) + 0x200]) ^	\
	pgm_read_word(&table[((data >> 16) & 0xff) + 0x100]) ^	\
	pgm_read_word(&table[data >> 24]);

class FastCRC16
{
public:
  /** Constructor
   */
  FastCRC16(){}

  uint16_t ccitt(const uint8_t *data,const uint16_t datalen)
  {
  // poly=0x1021 init=0xffff refin=false refout=false xorout=0x0000 check=0x29b1
    seed = 0xffff;
    return ccitt_upd(data, datalen);
  }

  uint16_t mcrf4xx(const uint8_t *data,const uint16_t datalen)
  {
  // poly=0x1021 init=0xffff refin=true refout=true xorout=0x0000 check=0x6f91
    seed = 0xffff;
    return mcrf4xx_upd(data, datalen);
  }

  uint16_t kermit(const uint8_t *data, const uint16_t datalen)
  {
  // poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000 check=0x2189
  // sometimes byteswapped presentation of result
    seed = 0x0000;
    return kermit_upd(data, datalen);
  }

  uint16_t modbus(const uint8_t *data, const uint16_t datalen)
  {
  // poly=0x8005 init=0xffff refin=true refout=true xorout=0x0000 check=0x4b37
    seed = 0xffff;
    return modbus_upd(data, datalen);
  }

  uint16_t xmodem(const uint8_t *data, const uint16_t datalen)
  {
    //width=16 poly=0x1021 init=0x0000 refin=false refout=false xorout=0x0000 check=0x31c3
    seed = 0x0000;
    return xmodem_upd(data, datalen);
  }

  uint16_t x25(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x1021 init=0xffff refin=true refout=true xorout=0xffff check=0x906e
    seed = 0xffff;
    return x25_upd(data, datalen);
  }

  /** CCITT
   * Alias "false CCITT"
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t ccitt_upd(const uint8_t *data, uint16_t len)
  {
    uint16_t crc = seed;
    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_ccitt[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      crc_n4(crc, ((uint32_t *)data)[0], crc_table_ccitt);
      crc_n4(crc, ((uint32_t *)data)[1], crc_table_ccitt);
      crc_n4(crc, ((uint32_t *)data)[2], crc_table_ccitt);
      crc_n4(crc, ((uint32_t *)data)[3], crc_table_ccitt);
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_ccitt[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    crc = REV16(crc);

    return crc;
  }

  /** MCRF4XX
   * equivalent to _crc_ccitt_update() in crc16.h from avr_libc
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t mcrf4xx_upd(const uint8_t *data, uint16_t len)
  {

    uint16_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_mcrf4xx[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      crc_n4(crc, ((uint32_t *)data)[0], crc_table_mcrf4xx);
      crc_n4(crc, ((uint32_t *)data)[1], crc_table_mcrf4xx);
      crc_n4(crc, ((uint32_t *)data)[2], crc_table_mcrf4xx);
      crc_n4(crc, ((uint32_t *)data)[3], crc_table_mcrf4xx);
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_mcrf4xx[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    return crc;
  }

  /** KERMIT
   * Alias CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-CCITT
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t kermit_upd(const uint8_t *data, uint16_t len)
  {

    uint16_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_kermit[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      crc_n4(crc, ((uint32_t *)data)[0], crc_table_kermit);
      crc_n4(crc, ((uint32_t *)data)[1], crc_table_kermit);
      crc_n4(crc, ((uint32_t *)data)[2], crc_table_kermit);
      crc_n4(crc, ((uint32_t *)data)[3], crc_table_kermit);
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_kermit[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    return crc;
  }

  /** MODBUS
   * equivalent to _crc_16_update() in crc16.h from avr_libc
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t modbus_upd(const uint8_t *data, uint16_t len)
  {

    uint16_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_modbus[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      crc_n4(crc, ((uint32_t *)data)[0], crc_table_modbus);
      crc_n4(crc, ((uint32_t *)data)[1], crc_table_modbus);
      crc_n4(crc, ((uint32_t *)data)[2], crc_table_modbus);
      crc_n4(crc, ((uint32_t *)data)[3], crc_table_modbus);
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_modbus[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    return crc;
  }

  /** XMODEM
   * Alias ZMODEM, CRC-16/ACORN
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t xmodem_upd(const uint8_t *data, uint16_t len)
  {

    uint16_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_xmodem[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      crc_n4(crc, ((uint32_t *)data)[0], crc_table_xmodem);
      crc_n4(crc, ((uint32_t *)data)[1], crc_table_xmodem);
      crc_n4(crc, ((uint32_t *)data)[2], crc_table_xmodem);
      crc_n4(crc, ((uint32_t *)data)[3], crc_table_xmodem);
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_xmodem[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    crc = REV16(crc);
    return crc;
  }

  /** X25
   * Alias CRC-16/IBM-SDLC, CRC-16/ISO-HDLC, CRC-B
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  uint16_t x25_upd(const uint8_t *data, uint16_t len)
  {

    uint16_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_x25[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      crc_n4(crc, ((uint32_t *)data)[0], crc_table_x25);
      crc_n4(crc, ((uint32_t *)data)[1], crc_table_x25);
      crc_n4(crc, ((uint32_t *)data)[2], crc_table_x25);
      crc_n4(crc, ((uint32_t *)data)[3], crc_table_x25);
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_word(&crc_table_x25[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    crc = ~crc;

    return crc;
  }

#if !CRC_SW
  uint16_t generic(const uint16_t polyom, const uint16_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen); //Not available in non-hw-variant (not T3.x)
#endif
private:
#if CRC_SW
  uint16_t seed;
#else
  uint16_t update(const uint8_t *data, const uint16_t datalen);
#endif
};

// ================= 32-BIT CRC ===================

#define crc_n4d(crc, data, table) crc ^= data; \
	crc = pgm_read_dword(&table[(crc & 0xff) + 0x300]) ^	\
	pgm_read_dword(&table[((crc >> 8) & 0xff) + 0x200]) ^	\
	pgm_read_dword(&table[((crc >> 16) & 0xff) + 0x100]) ^	\
	pgm_read_dword(&table[(crc >> 24) & 0xff]);

#define crcsm_n4d(crc, data, table) crc ^= data; \
	crc = (crc >> 8) ^ pgm_read_dword(&table[crc & 0xff]); \
	crc = (crc >> 8) ^ pgm_read_dword(&table[crc & 0xff]); \
	crc = (crc >> 8) ^ pgm_read_dword(&table[crc & 0xff]); \
	crc = (crc >> 8) ^ pgm_read_dword(&table[crc & 0xff]);

class FastCRC32
{
public:

  /** Constructor
   */
  FastCRC32(){}

  uint32_t crc32(const uint8_t *data, const uint16_t datalen)
  {
    // poly=0x04c11db7 init=0xffffffff refin=true refout=true xorout=0xffffffff check=0xcbf43926
    seed = 0xffffffff;
    return crc32_upd(data, datalen);
  }

  uint32_t cksum(const uint8_t *data, const uint16_t datalen)
  {
    // width=32 poly=0x04c11db7 init=0x00000000 refin=false refout=false xorout=0xffffffff check=0x765e7680
    seed = 0x00;
    return cksum_upd(data, datalen);
  }

  /** CRC32
   * Alias CRC-32/ADCCP, PKZIP, Ethernet, 802.3
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  #if CRC_BIGTABLES
  #define CRC_TABLE_CRC32 crc_table_crc32_big
  #else
  #define CRC_TABLE_CRC32 crc_table_crc32
  #endif
  uint32_t crc32_upd(const uint8_t *data, uint16_t len)
  {

    uint32_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_dword(&CRC_TABLE_CRC32[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      #if CRC_BIGTABLES
      crc_n4d(crc, ((uint32_t *)data)[0], CRC_TABLE_CRC32);
      crc_n4d(crc, ((uint32_t *)data)[1], CRC_TABLE_CRC32);
      crc_n4d(crc, ((uint32_t *)data)[2], CRC_TABLE_CRC32);
      crc_n4d(crc, ((uint32_t *)data)[3], CRC_TABLE_CRC32);
      #else
      crcsm_n4d(crc, ((uint32_t *)data)[0], CRC_TABLE_CRC32);
      crcsm_n4d(crc, ((uint32_t *)data)[1], CRC_TABLE_CRC32);
      crcsm_n4d(crc, ((uint32_t *)data)[2], CRC_TABLE_CRC32);
      crcsm_n4d(crc, ((uint32_t *)data)[3], CRC_TABLE_CRC32);
      #endif
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_dword(&CRC_TABLE_CRC32[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    crc = ~crc;

    return crc;
  }

  /** CKSUM
   * Alias CRC-32/POSIX
   * @param data Pointer to Data
   * @param datalen Length of Data
   * @return CRC value
   */
  #if CRC_BIGTABLES
  #define CRC_TABLE_CKSUM crc_table_cksum_big
  #else
  #define CRC_TABLE_CKSUM crc_table_cksum
  #endif
  uint32_t cksum_upd(const uint8_t *data, uint16_t len)
  {

    uint32_t crc = seed;

    while (((uintptr_t)data & 3) && len) {
      crc = (crc >> 8) ^ pgm_read_dword(&CRC_TABLE_CKSUM[(crc & 0xff) ^ *data++]);
      len--;
    }

    while (len >= 16) {
      len -= 16;
      #if CRC_BIGTABLES
      crc_n4d(crc, ((uint32_t *)data)[0], CRC_TABLE_CKSUM);
      crc_n4d(crc, ((uint32_t *)data)[1], CRC_TABLE_CKSUM);
      crc_n4d(crc, ((uint32_t *)data)[2], CRC_TABLE_CKSUM);
      crc_n4d(crc, ((uint32_t *)data)[3], CRC_TABLE_CKSUM);
      #else
      crcsm_n4d(crc, ((uint32_t *)data)[0], CRC_TABLE_CKSUM);
      crcsm_n4d(crc, ((uint32_t *)data)[1], CRC_TABLE_CKSUM);
      crcsm_n4d(crc, ((uint32_t *)data)[2], CRC_TABLE_CKSUM);
      crcsm_n4d(crc, ((uint32_t *)data)[3], CRC_TABLE_CKSUM);
      #endif
      data += 16;
    }

    while (len--) {
      crc = (crc >> 8) ^ pgm_read_dword(&CRC_TABLE_CKSUM[(crc & 0xff) ^ *data++]);
    }

    seed = crc;
    crc = ~REV32(crc);
    return crc;
  }

#if !CRC_SW
  uint32_t generic(const uint32_t polyom, const uint32_t seed, const uint32_t flags, const uint8_t *data, const uint16_t datalen); //Not available in non-hw-variant (not T3.x)
#endif
private:
#if CRC_SW
  uint32_t seed;
#else
  uint32_t update(const uint8_t *data, const uint16_t datalen);
#endif
};

#endif // #if !defined(KINETISK)

#endif
