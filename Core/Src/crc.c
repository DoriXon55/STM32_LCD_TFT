/*
 * crc.c
 *
 *  Created on: Nov 3, 2024
 *      Author: doria
 */

#include "crc.h"


/************************************************************************
* Zmienna: crc16_table
* Typ: uint16_t[256]
* Cel: Tablica lookup dla algorytmu CRC-16
*
* (Zawiera wstępnie obliczone wartości CRC-16 dla wszystkich możliwych
* 8-bitowych wartości (0-255). Używa wielomianu 0x1021.
* Tablica jest zorganizowana w 32 wiersze po 8 wartości.)
*
* Format wartości:
*   - Każda wartość jest 16-bitowa
*   - Wartości są zapisane w formacie heksadecymalnym
*   - Wartości są obliczone dla standardowego CRC-16/IBM-3740
************************************************************************/
uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};




/************************************************************************
* Funkcja: calculate_crc16()
* Cel: Oblicza CRC-16 dla podanego ciągu danych
* Parametry:
*   - data: Wskaźnik na dane wejściowe
*   - length: Długość danych wejściowych w bajtach
*   - crc_out: Tablica 2 bajtów na wynik CRC
*
* Zwraca: void (wynik zapisywany w crc_out)
*
*   Funkcja implementuje algorytm CRC-16/IBM-3740 używając tablicy lookup.
*   1. Inicjalizuje CRC wartością 0xFFFF
*   2. Dla każdego bajtu danych:
*      - Oblicza indeks do tablicy lookup używając XOR z górnym bajtem CRC
*      - Aktualizuje CRC używając wartości z tablicy
*   3. Zapisuje końcowy wynik w formie dwóch bajtów
*
* Kroki algorytmu:
*   1. crc = 0xFFfF (wartość inicjująca)
*   2. Dla każdego bajtu:
*      - table_index = (crc >> 8) ^ byte
*      - crc = (crc << 8) ^ crc16_table[table_index]
*   3. Zapisz wynik:
*      - crc_out[0] = (crc >> 8) & 0xFF (starszy bajt)
*      - crc_out[1] = crc & 0xFF (młodszy bajt)
*
* Korzysta z:
*   - crc16_table: Tablica lookup z wstępnie obliczonymi wartościami
************************************************************************/
void calculateCrc16(uint8_t *data, size_t length, char crc_out[2]) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        uint8_t table_index = (crc >> 8) ^ byte;
        crc = (crc << 8) ^ crc16_table[table_index];
    }
    crc_out[0] = ((crc >> 8) & 0xFF);
    crc_out[1] = (crc & 0xFF);
}
void debugCRCCalculation(uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    USART_sendFrame((uint8_t*)"CRC Calculation:\r\n", 17);

    char buf[100];
    int idx;

    // Pokaż dane wejściowe
    idx = sprintf(buf, "Input data: ");
    for(size_t i = 0; i < length; i++) {
        idx += sprintf(buf + idx, "%02X ", data[i]);
    }
    sprintf(buf + idx, "\r\n");
    USART_sendFrame((uint8_t*)buf, strlen(buf));

    // Pokaż proces
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        uint8_t table_index = (crc >> 8) ^ byte;
        uint16_t old_crc = crc;
        crc = (crc << 8) ^ crc16_table[table_index];

        idx = sprintf(buf, "Step %zu: byte=%02X, index=%02X, old_crc=%04X, new_crc=%04X\r\n",
                     i, byte, table_index, old_crc, crc);
        USART_sendFrame((uint8_t*)buf, idx);
    }

    // Pokaż wynik końcowy
    sprintf(buf, "Final CRC: %02X %02X\r\n", (crc >> 8) & 0xFF, crc & 0xFF);
    USART_sendFrame((uint8_t*)buf, strlen(buf));
}
