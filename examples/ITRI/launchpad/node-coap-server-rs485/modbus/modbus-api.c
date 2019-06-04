/**
 * \file
 *         Read Modbus address from Archmeter power meter using Modbus protocols
 * \author
 *         Germ�n Ramos <german.ramos@sensingcontrol.com>
 *         Joakim Eriksson <joakime@sics.se>
 * \company
 *                 Sensing & Control Systems S.L.
 *                 SICS
 */

/**
 * \brief  This is the main file of modbus protocol
 *
 * We expect:
 *    TX: 0F 04 10 C6 00 02 94 18
 *    RX: 0F 04 04 5A 39 40 1C E6 98
 *
 * This modbus packet will be transmitted over RS485
 */

#include "contiki.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leds.h"


#include "modbusStructs.h"
#include "modbusProtocol.h"
#include "modbusMacros.h"
#include "rs485-dev.h"

#define MODBUS_ADDRESS 1
#define MODBUS_SIZE 1

void init_crc16_tab( void );

/**
 * \brief  It builds query modbus
 * \param  function Function that will be used
 * \param  modbusQuery Query structure where data will be filled to
 * \param  modbusFunctions Pointer to generic structure of all modbus functions
 * \return status of write: 0 sucess, -1 error
 */
static int
modbusRdCoilStatus(unsigned int function,
                   st_modbusIOGeneric *modbusRdCoil,
                   st_modbusQuery *modbusQuery,
                   st_modbusExceptionCode *exceptionCode,
                   unsigned char hasCrc, unsigned int registerModBus)
{
  int status = 0;
  int count = 0;

  /* Number of registers that we want read */
  /* Each value is a float type. A float type uses 2 registers (16 bits * 2 = 32 bits)*/
  count = MODBUS_SIZE;
  /* Modbus device address */

  modbusQuery->address = MODBUS_ADDRESS;
  /* Function code to read registers(Modbus protocol), 0x04 --> Read Input Registers */
  modbusQuery->function = function;
  modbusQuery->startAddressHI = VAL_2_HIGH(registerModBus);
  modbusQuery->startAddressLO = VAL_2_LOW(registerModBus);
  modbusQuery->countHI = VAL_2_HIGH(count);
  modbusQuery->countLO = VAL_2_LOW(count);

  /* TODO: check the zero and set it to what is needed */

  status = modbusReadCoilStatus(modbusQuery, modbusRdCoil,
                                exceptionCode, hasCrc, 0);
  
/*   printf("rdCoilStatus: %d\n\r", status); */
  return status;
}
/***********************************************************************/

/**
 * \brief  Modbus main function
 * \param  void
 * \return 0
 */
static st_modbusQuery modbusQuery;
static st_modbusIOGeneric modbusStatus;
static st_modbusExceptionCode exceptionCode;

int
modbus_read_register(unsigned int registerModBus)
{
  int rv = 0;
  unsigned char hasCrc;
  /* Is necessary the use of CRC-16 (Modbus) */
  hasCrc = 1;

  rv = modbusRdCoilStatus(MODBUS_RD_IN_REG, &modbusStatus,
                          &modbusQuery, &exceptionCode, hasCrc,
                          registerModBus);
  return rv;
}

st_modbusIOGeneric *modbus_get_status() {
  return &modbusStatus;
}

/********************************************************************/

/* simple function to calculate the float value in modbus */
/* low word high byte | low word low byte || high word ... */
/* Hi - low: SEEE EEEE EMMM MMMM MMMM...MMMM */
int16_t
modbus_get_data(int pos) {
  uint16_t value;
  uint8_t *data = modbusStatus.data;

  value = (data[pos] << 8) + data[pos+1];

  return value;
}

void
modbus_init()
{
  rs485_init();
  init_crc16_tab();
}
