/*
 * frame.c
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */
//=========================INCLUDES=============================
#include "frame.h"
#include "crc.h"
#include "USART_ringbuffer.h"
#include "usart.h"
#include "lcd.h"
#include "font5x7.h"
#include "font5x8.h"
#include "font6x9.h"
#include "hagl.h"
#include "rgb565.h"
#include <ctype.h>
#include <math.h>
#include "crc.h"

//=========================ZMIENNE DO RAMKI=============================

uint8_t bx[MAX_FRAME_LEN];
bool escape_detected = false;
int bx_index = 0;
bool in_frame = false;
uint8_t received_char;
Frame frame;
ScrollingTextState text = {0};



/************************************************************************
* Funkcja: resetFrameState()
* Cel: Resetuje stan maszyny stanów odbierającej ramki
* Funkcja pomocnicza wywoływana gdy:
* 	- Wykryto błąd w ramce
*   - Zakończono przetwarzanie ramki
*   - Potrzebny jest reset stanu odbierania
*
* Zmienne globalne:
*   - in_frame: Flaga oznaczająca czy jesteśmy w trakcie odbierania ramki
*   - escape_detected: Flaga oznaczająca wykrycie znaku escape
*   - bx_index: Indeks w buforze odbiorczym
************************************************************************/
static void resetFrameState() {
    in_frame = false;
    escape_detected = false;
    bx_index = 0;
}

static void stopAnimation(void)
{
	text.isScrolling = false;
}

/************************************************************************
* Funkcja: isWithinBounds()
* Cel: Sprawdza czy podane współrzędne mieszczą się w wymiarach ekranu
*
* Parametry:
*   - x: Współrzędna X (int)
*   - y: Współrzędna Y (int)
*
* Zwraca:
*   - true: Jeśli punkt mieści się w wymiarach ekranu
*   - false: Jeśli punkt jest poza ekranem
*
* Sprawdza:
*   1. x >= 0 && x < LCD_WIDTH
*   2. y >= 0 && y < LCD_HEIGHT
*
* Korzysta z:
*   - LCD_WIDTH: Stała określająca szerokość ekranu
*   - LCD_HEIGHT: Stała określająca wysokość ekranu
************************************************************************/
static bool isWithinBounds(int x, int y)
{
	return (x >= 0 && x < LCD_WIDTH)&&(y >= 0 && y < LCD_HEIGHT);
}

static bool safeCompare(const char* str1, const char* str2, size_t len)
{
	if(str1 == NULL || str2 == NULL)
	{
		return false;
	}
	return memcmp(str1, str2, len) == 0;
}



static bool isValidTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3) {
    // Obliczamy kwadraty długości boków
    int32_t a2 = (int32_t)(x2 - x1) * (x2 - x1) + (int32_t)(y2 - y1) * (y2 - y1);
    int32_t b2 = (int32_t)(x3 - x2) * (x3 - x2) + (int32_t)(y3 - y2) * (y3 - y2);
    int32_t c2 = (int32_t)(x1 - x3) * (x1 - x3) + (int32_t)(y1 - y3) * (y1 - y3);

    // Sprawdzamy nierówność trójkąta używając kwadratów długości
    // Jeśli a² + b² ≤ c² (lub podobnie dla innych kombinacji),
    // to trójkąt nie może istnieć
    if (a2 + b2 <= c2 || b2 + c2 <= a2 || c2 + a2 <= b2) {
        return false;
    }

    // Sprawdzamy czy punkty nie leżą na jednej linii
    int32_t cross = (int32_t)(x2 - x1) * (y3 - y1) - (int32_t)(y2 - y1) * (x3 - x1);
    if (cross == 0) {
        return false;
    }

    return true;
}
/************************************************************************
* Funkcja: parseColor()
* Cel: Konwersja nazwy koloru na wartość Color_t
*
* Parametry:
*   - color_name: Wskaźnik na string z nazwą koloru
*   - color: Wskaźnik na zmienną Color_t, gdzie zostanie zapisany wynik
*
* Zwraca:
*   - true: Jeśli znaleziono kolor w tablicy color_map
*   - false: Jeśli nie znaleziono koloru
*
* Używa:
*   - strcmp(): Porównuje dwa stringi
*   - Parametry: (const char* str1, const char* str2)
*   - Zwraca: 0 jeśli stringi są identyczne
*
* Zasada działania:
*   1. Iteruje przez tablicę color_map
*   2. Porównuje nazwę koloru z każdym elementem tablicy
*   3. Jeśli znajdzie dopasowanie, zapisuje wartość koloru
*
* Korzysta z:
*   - color_map: Globalna tablica struktur ColorMap zawierająca:
*   - name: string z nazwą koloru
*   - value: wartość Color_t w formacie RGB565
************************************************************************/

void sendStatus(StatusCode_t status) {
    if(status < STATUS_COUNT) {
        prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "%s", STATUS_MESSAGES[status]);
    }
}



bool parseColor(const char* color_name, Color_t* color) {
    if (color_name == NULL || color == NULL) {
        return false;
    }

    for (int i = 0; i < sizeof(color_map) / sizeof(ColorMap); i++) {
        size_t color_len = strlen(color_map[i].name); // Get the expected color name length
        if (safeCompare(color_name, color_map[i].name, color_len)) {
            *color = color_map[i].value;
            return true;
        }
    }
    return false;
}




/************************************************************************
* Funkcja: parseParameters()
* Cel: Parsuje parametry z ciągu znaków według zadanego formatu
*
* Parametry:
*   - data: "String" zawierający dane do sparsowania
*   - format: "String" określający format danych ("uuust" itp.)
*   - ...: Zmienne wskaźniki na miejsca zapisu wartości
*
* Zwraca:
*   - true: Jeśli parsowanie się powiodło
*   - false: W przypadku błędu
*
* Używa:
*   1. isspace(): Sprawdza czy znak jest białym znakiem
*      - Zwraca: !0 jeśli tak, 0 jeśli nie
*
*   2. strtoul(): Konwertuje string na unsigned long
*      - Parametry: (const char* str, char** endptr, int base)
*      - Zwraca: Przekonwertowaną wartość
*
*   3. strncpy(): Kopiuje n znaków z jednego stringa do drugiego
*      - Parametry: (char* dest, const char* src, size_t n)
*
*   4. parseColor(): Opisana wcześniej funkcja pomocnicza
*
* Obsługiwane formaty:
*   u - unsigned int (0-255)
*   s - kolor (string -> Color_t)
*   t - tekst (max 50 znaków)
*
* Zasada działania:
*   1. Sprawdza poprawność danych wejściowych
*   2. Inicjalizuje va_list do obsługi zmiennej liczby argumentów
*   3. Dla każdego znaku w formacie:
*      - Pomija białe znaki
*      - Wyodrębnia token do następnego przecinka
*      - Przetwarza token według odpowiedniego typu
*   4. Zwalnia va_list
************************************************************************/
bool parseParameters(const uint8_t* data, const char* format, ...) {
    if (!data || !format) {
        return false;
    }
    va_list args;
    va_start(args, format);

    const uint8_t* data_ptr = data;
    const char* fmt_ptr = format;
    uint8_t token[51];
    size_t token_idx = 0;

    // Store scrollSpeed for text length validation
    uint8_t scrollSpeed = 0;
    bool hasScrollSpeed = false;

    while (*fmt_ptr) {
        switch (*fmt_ptr) {
            case 'u': {
                uint8_t* value_ptr = va_arg(args, uint8_t*);
                *value_ptr = *data_ptr++;  // Bezpośrednio odczytaj bajt

                // Store scrollSpeed for later validation
                if (!hasScrollSpeed) {
                    scrollSpeed = *value_ptr;
                    hasScrollSpeed = true;
                }

                // Jeśli następny znak to przecinek, pomiń go
                if (*data_ptr == ',') {
                    data_ptr++;
                }
                break;
            }
            case 's': {
                // Zbierz znaki do przecinka
                token_idx = 0;
                while (*data_ptr && *data_ptr != ',' && token_idx <= 10) {
                    token[token_idx++] = *data_ptr++;
                }
                if (*data_ptr == ',') data_ptr++;

                Color_t* color_ptr = va_arg(args, Color_t*);
                if (!parseColor((char*)token, color_ptr)) {
                    va_end(args);
                    return false;
                }
                break;
            }
            case 't': {
                token_idx = 0;
                while (*data_ptr && token_idx < 50) {
                    token[token_idx++] = *data_ptr++;
                }

                size_t maxLength = (scrollSpeed == 0) ? 25 : 50;
                if (token_idx > maxLength) {
                    va_end(args);
                    sendStatus(ERR_TOO_MUCH_TEXT);
                    return false;  // Tekst jest za długi
                }

                char* text_ptr = va_arg(args, char*);
                memcpy(text_ptr, token, token_idx);
                break;
            }
            default:
                va_end(args);
                return false;
        }
        fmt_ptr++;
    }

    va_end(args);
    return true;
}


/************************************************************************
* Funkcja: clearFrame()
* Cel: Czyści zawartość struktury ramki komunikacyjnej
*   1. Sprawdza czy wskaźnik frame nie jest NULL
*   2. Zeruje pole data struktury
*   3. Zeruje pole command struktury
*
* Parametry:
*   - frame: Wskaźnik na strukturę Receive_Frame do wyczyszczenia
*
* Używa:
*   memset(): Wypełnia blok pamięci zadaną wartością
*   - Parametry: (void* ptr, int value, size_t num)
*   - ptr: Wskaźnik na początek bloku pamięci
*   - value: Wartość do wypełnienia (0 dla wyzerowania)
*   - num: Liczba bajtów do wypełnienia
************************************************************************/
void clearFrame(Frame* frame) {
    if (frame) {
        memset(frame->data, 0, sizeof(frame->data));
        memset(frame->command, 0, sizeof(frame->command));
    }
}



/************************************************************************
* Funkcja: executeONK()
* Cel: Obsługa komendy rysowania koła na wyświetlaczu

* Parametry:
*   - frame: Wskaźnik na strukturę z danymi ramki
*
* Zmienne:
*   - x, y: Współrzędne środka koła (uint8_t)
*   - r: Promień koła (uint8_t)
*   - filling: Tryb wypełnienia (0-kontur, 1-wypełnione)
*   - color: Kolor w formacie RGB565 (Color_t)
*
* Używa:
*   1. parseParameters(): Parsuje parametry z formatu "uuuus"
*      - u: współrzędne x,y,r,filling
*      - s: kolor
*
*   2. hagl_draw_circle() / hagl_fill_circle():
*      Funkcje biblioteki HAGL do rysowania
*      - Parametry: (x, y, r, color)
*
*   3. prepareFrame(): Wysyła odpowiedź w przypadku błędu
************************************************************************/
static void executeONK(Frame *frame)
{
	uint8_t x = 0, y = 0, r = 0, filling = 0;
	Color_t color = 0;
    if (!parseParameters(frame->data, "uuuus", &x, &y, &r, &filling, &color))
    {
    	sendStatus(ERR_NOT_RECOGNIZED);
        return;
    }
	switch(filling)
	{
	case 0:
		hagl_draw_circle(x, y, r, color);
		break;
	case 1:
		hagl_fill_circle(x, y, r, color);
		break;
	}
}



/************************************************************************
* Funkcja: executeONP
* Cel: Obsługa komendy rysowania prostokąta
*
* Parametry:
*   - frame: Wskaźnik na strukturę z danymi ramki
*
* Zmienne:
*   - x, y: Współrzędne lewego górnego rogu (uint8_t)
*   - width, height: Wymiary prostokąta (uint8_t)
*   - filling: Tryb wypełnienia (0-kontur, 1-wypełniony)
*   - color: Kolor w formacie RGB565 (Color_t)
*
* Używa:
*   1. parseParameters(): Parsuje parametry z formatu "uuuuus"
*      - u: x,y,width,height,filling
*      - s: kolor
*
*   2. hagl_draw_rectangle() / hagl_fill_rectangle():
*      Funkcje HAGL do rysowania prostokątów
*      - Parametry: (x, y, width, height, color)
************************************************************************/
static void executeONP(Frame *frame)
{
	uint8_t x = 0, y = 0, width = 0, height = 0, filling = 0;
	Color_t color = 0;
	if (!parseParameters(frame->data, "uuuuus", &x, &y, &width, &height, &filling, &color)) {
    	sendStatus(ERR_NOT_RECOGNIZED);
		return;
	}

	switch(filling)
	{
	case 0:
		hagl_draw_rectangle(x, y, width, height, color);
		break;
	case 1:
		hagl_fill_rectangle(x, y, width, height, color);
		break;
	}
}




/************************************************************************
* Function: executeONT()
* Cel: Obsługa komendy rysowania trójkąta
*
* Parametry:
*   - frame: Wskaźnik na strukturę z danymi ramki
*
* Zmienne:
*   - x1,y1,x2,y2,x3,y3: Współrzędne wierzchołków (uint8_t)
*   - filling: Tryb wypełnienia (0-kontur, 1-wypełniony)
*   - color: Kolor w formacie RGB565 (Color_t)
*
* Używa:
*   1. parseParameters(): Parsuje parametry z formatu "uuuuuuus"
*      - u: x1,y1,x2,y2,x3,y3,filling
*      - s: kolor
*
*   2. hagl_draw_triangle() / hagl_fill_triangle():
*      Funkcje HAGL do rysowania trójkątów
*      - Parametry: (x1,y1, x2,y2, x3,y3, color)
************************************************************************/
static void executeONT(Frame *frame)
{
    uint8_t x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0, filling = 0;
    Color_t color = 0;
    if (!parseParameters(frame->data, "uuuuuuus", &x1, &y1, &x2, &y2, &x3, &y3, &filling, &color))
    {
    	sendStatus(ERR_NOT_RECOGNIZED);
        return;
    }
    if (!isValidTriangle(x1, y1, x2, y2, x3, y3)) {
    	sendStatus(ERR_INVALID_TRIANGLE);
            return;
    }
    switch(filling)
    {
        case 0:
            hagl_draw_triangle(x1, y1, x2, y2, x3, y3, color);
            break;
        case 1:
            hagl_fill_triangle(x1, y1, x2, y2, x3, y3, color);
            break;
    }
}



/************************************************************************
* Funkcja: executeONN()
* Cel: Obsługa komendy wyświetlania tekstu na ekranie LCD
*
* Parametry:
*   - frame: Wskaźnik na strukturę z danymi ramki
*
* Zmienne:
*   - text[50]: Bufor na tekst w formacie char
*   - wtext[50]: Bufor na tekst w formacie wchar_t (szeroki znak)
*   - x, y: Współrzędne początkowe tekstu (uint8_t)
*   - fontSize: Rozmiar czcionki (1-3)
*   - color: Kolor tekstu (domyślnie BLACK)
*
* Używa:
*   1. parseParameters(): Parsuje parametry z formatu "uuust"
*      - u: x,y,fontSize
*      - s: color
*      - t: text
*
*   2. strlen(): Oblicza długość tekstu
*
*   3. hagl_put_text(): Wyświetla tekst
*      - Parametry: (wtext, x, y, color, font)
*      - Dostępne fonty: font5x7, font5x8, font6x9

************************************************************************/
static void executeONN(Frame *frame)
{
    const char charText[50] = {0};
    if (!parseParameters(frame->data, "uuuust", &text.x, &text.y, &text.fontSize, &text.scrollSpeed, &text.color, charText)) {
    	sendStatus(ERR_NOT_RECOGNIZED);
        return;
    }

    // Zapisz początkowe pozycje
    memset(text.displayText, 0, sizeof(text.displayText));
    text.startX = text.x;
    text.startY = text.y;  // Ta wartość będzie używana do stałej pozycji Y
    text.textLength = 0;
    text.firstIteration = true;
    // Konwersja tekstu
    while (charText[text.textLength] && text.textLength < 50) {
        text.displayText[text.textLength] = (wchar_t)charText[text.textLength];
        text.textLength++;
    }

    text.isScrolling = (text.scrollSpeed > 0);
    text.lastUpdate = HAL_GetTick();

    const uint8_t* font;
    switch(text.fontSize) {
        case 1: font = font5x7; break;
        case 2: font = font5x8; break;
        case 3: font = font6x9; break;
        default: font = font5x7;
    }

    if(!text.scrollSpeed) {
        hagl_put_text(text.displayText, text.x, text.y, text.color, font);
    }
}


/************************************************************************
* Funkcja: executeOFF()
* Cel: Obsługa komendy wyłączenia/wyczyszczenia wyświetlacza
*
* Parametry:
*   - frame: Wskaźnik na strukturę z danymi ramki
*
* Tryby:
*   case 0: Wyłączenie podświetlenia
*     - Używa HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_RESET)
*
*   case 1: Czyszczenie ekranu
*     - Używa hagl_fill_rectangle(0,0, LCD_WIDTH, LCD_HEIGHT, BLACK)
*
************************************************************************/
static void executeOFF(Frame *frame)
{

	switch(frame->data[0])
	{
	case 0:
		HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_RESET);
		break;
	case 1:
		lcdClear();
		break;
	default:
    	sendStatus(ERR_WRONG_OFF_DATA);
	}
}




/************************************************************************
* Funkcja: parseCoordinates()
* Cel: Parsuje "string" zawierający współrzędne x,y
*
* Parametry:
*   - data: String wejściowy w formacie "x,y"
*   - x: Wskaźnik na zmienną dla współrzędnej X
*   - y: Wskaźnik na zmienną dla współrzędnej Y
*
* Zwraca:
*   - true: Jeśli parsowanie się powiodło
*   - false: W przypadku błędu
*
* Używa:
*   1. strncpy(): Kopiuje string wejściowy
*      - Konieczne bo strtok modyfikuje string
*
*   2. strtok(): Dzieli string według separatora ","
*      - Pierwsze wywołanie z string
*      - Kolejne z NULL
*
*   3. atoi(): Konwertuje string na int
************************************************************************/
bool parseCoordinates(const uint8_t* data, int* x, int* y)
{
	 *x = data[0];  // Pierwszy bajt to x
	 *y = data[2];  // Drugi bajt to y
    return true;
}



/************************************************************************
* Funkcja: byteStuffing()
* Cel: Koduje specjalne znaki w ramce komunikacyjnej
*   1. Iteruje przez każdy bajt wejściowy
*   2. Jeśli znajdzie znak specjalny:
*      - Wstawia znak ESCAPE_CHAR
*      - Wstawia odpowiedni znak zastępczy
*   3. Jeśli zwykły znak - kopiuje bez zmian
*
* Parametry:
*   - input: Wskaźnik na dane wejściowe
*   - input_len: Długość danych wejściowych
*   - output: Wskaźnik na bufor wyjściowy
*
* Zwraca:
*   - size_t: Liczba bajtów w buforze wyjściowym
*
* Sekwencje zamiany:
* 	FRAME_START			-> '~'
* 	FRAME_END			-> '`'
* 	FRAME_START_STUFF	-> '~'
* 	FRAME_END_STUFF		-> '&'
* 	ESCAPE_CHAR      	-> "}"
* 	ESCAPE_CHAR_STUFF	-> ']'

*   '}' 			 	-> "}]"
*   '~'              	-> "}^"
*   '`'              	-> "}&"

************************************************************************/
size_t byteStuffing(uint8_t *input, size_t input_len, uint8_t *output) {
    size_t j = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == ESCAPE_CHAR) {
            output[j++] = ESCAPE_CHAR;
            output[j++] = ESCAPE_CHAR_STUFF;
        } else if (input[i] == FRAME_START) {
            output[j++] = ESCAPE_CHAR;
            output[j++] = FRAME_START_STUFF;
        } else if (input[i] == FRAME_END) {
            output[j++] = ESCAPE_CHAR;
            output[j++] = FRAME_END_STUFF;
        } else {
            output[j++] = input[i];
        }
    }
    return j;
}

/************************************************************************
* Funckja: prepareFrame
* Cel: Przygotowuje i wysyła ramkę komunikacyjną
*
* Parametry:
*   - sender: ID nadawcy
*   - receiver: ID odbiorcy
*   - command: 3-znakowy kod komendy
*   - format: Format danych (jak w printf)
*   - ...: Zmienne argumenty dla formatowania
*
* Format ramki zwrotnej:
*   FRAME_START
*   [sender][receiver][command][data][CRC]
*   FRAME_END
*
* Używa:
*   1. va_list/va_start/va_end: Obsługa zmiennej liczby argumentów
*   2. vsnprintf: Formatowanie stringa z argumentami
*   3. calculateCrc16: Obliczanie sumy kontrolnej
*   4. byteStuffing: Kodowanie znaków specjalnych
*   5. USART_fsend: Wysyłanie przez UART
*   6. delay: Opóźnienie między bajtami (do zmiany, znaleźć problem)
*
* Działanie:
*   1. Tworzy strukturę ramki
*   2. Formatuje dane
*   3. Oblicza CRC
*   4. Konwertuje CRC na hex
*   5. Przygotowuje payload
*   6. Wykonuje byte stuffing
*   7. Wysyła ramkę
************************************************************************/
void prepareFrame(uint8_t sender, uint8_t receiver, const char *command, const char *format, ...) {
    Frame frame = {0};
    frame.sender = sender;
    frame.receiver = receiver;
    strncpy((char *)frame.command, command, COMMAND_LENGTH);

    // Stały bufor na sformatowane dane
    uint8_t formattedData[MAX_DATA_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf((char *)formattedData, MAX_DATA_SIZE, format, args);
    va_end(args);

    size_t dataLen = strlen((char *)formattedData);

    // Bufor na dane do obliczenia CRC
    uint8_t crcInput[MAX_FRAME_WITHOUT_STUFFING];
    size_t crcInputLen = 0;

    // Przygotowanie danych do CRC
    crcInput[crcInputLen++] = frame.sender;
    crcInput[crcInputLen++] = frame.receiver;
    memcpy(crcInput + crcInputLen, frame.command, COMMAND_LENGTH);
    crcInputLen += COMMAND_LENGTH;
    memcpy(crcInput + crcInputLen, formattedData, dataLen);
    crcInputLen += dataLen;

    // Obliczenie CRC
    uint8_t crcOutput[2];
    calculateCrc16(crcInput, crcInputLen, crcOutput);

    // Przygotowanie surowej ramki
    uint8_t rawPayload[MAX_FRAME_WITHOUT_STUFFING];
    size_t rawPayloadLen = 0;

    rawPayload[rawPayloadLen++] = frame.sender;
    rawPayload[rawPayloadLen++] = frame.receiver;
    memcpy(rawPayload + rawPayloadLen, frame.command, COMMAND_LENGTH);
    rawPayloadLen += COMMAND_LENGTH;
    memcpy(rawPayload + rawPayloadLen, formattedData, dataLen);
    rawPayloadLen += dataLen;

    rawPayload[rawPayloadLen++] = crcOutput[0];
    rawPayload[rawPayloadLen++] = crcOutput[1];

    uint8_t stuffedPayload[MAX_FRAME_LEN];
    size_t stuffedLen = byteStuffing(rawPayload, rawPayloadLen, stuffedPayload);

    uint8_t finalFrame[MAX_FRAME_LEN + 2];
    size_t finalLen = 0;

    finalFrame[finalLen++] = FRAME_START;
    memcpy(finalFrame + finalLen, stuffedPayload, stuffedLen);
    finalLen += stuffedLen;
    finalFrame[finalLen++] = FRAME_END;
    USART_sendFrame(finalFrame, finalLen);
}

/************************************************************************
* Funkcja: decodeFrame()
* Cel: Dekoduje otrzymaną ramkę i weryfikuje CRC
*
* Parametry:
*   - bx: Bufor z odebranymi danymi
*   - frame: Wskaźnik na strukturę Receive_Frame
*   - len: Długość odebranych danych
*
* Zwraca:
*   - true: Ramka poprawna i CRC się zgadza
*   - false: Błąd w ramce lub niezgodność CRC
*
* Działanie:
*   1. Sprawdza długość ramki
*   2. Kopiuje pola ramki:
*      - receiver, sender
*      - command
*      - data
*   3. Wyodrębnia i porównuje CRC
*
* Używa:
*   - memcpy: Kopiowanie danych
*   - calculateCrc16: Obliczanie sumy kontrolnej
************************************************************************/
bool decodeFrame(uint8_t *bx, Frame *frame, uint8_t len) {
    uint8_t ownCrc[2];
    uint8_t incCrc[2];

    if(len >= MIN_DECODED_FRAME_LEN && len <= MAX_FRAME_WITHOUT_STUFFING) {
        uint8_t k = 0;
        frame->sender = bx[k++];
        frame->receiver = bx[k++];
        if(frame->sender != PC_ADDR)
        {
        	sendStatus(ERR_WRONG_SENDER);
        	return false;
        }

        memcpy(frame->command, &bx[k], COMMAND_LENGTH);
        k += COMMAND_LENGTH;

        uint8_t data_len = len - MIN_DECODED_FRAME_LEN; //to sprawdzic
        memcpy(frame->data, &bx[k], data_len);
        frame->data[data_len] = '\0';
        k += data_len;

        memcpy(incCrc, &bx[k], 2);
        calculateCrc16((uint8_t *)frame, k, ownCrc);
        if(ownCrc[0] != incCrc[0] || ownCrc[1] != incCrc[1]) {
        	sendStatus(ERR_WRONG_CRC);
            return false;
        }
        return true;
    }
    return false;
}

/************************************************************************
* Funkcja: processReceivedChar()
* Cel: Implementacja maszyny stanów odbierającej ramki
*
* Parametry:
*   - received_char: Odebrany znak
*
* Stany:
*   1. Oczekiwanie na początek ramki ('~')
*   2. Odbieranie danych ramki
*   3. Obsługa znaków escape
*   4. Zakończenie ramki ('`')
*
* Używa:
*   - decodeFrame: Dekodowanie kompletnej ramki
*   - handleCommand: Wykonanie komendy z ramki
*   - prepareFrame: Wysłanie odpowiedzi
*   - resetFrameState: Reset stanu maszyny
*
* Odwrócenie kodowania:
*   '}^' -> '~'
*   '}\]' -> '}'
*   '}&' -> '`'
*
* Błędy:
*   - Nieprawidłowe sekwencje escape
*   - Przepełnienie bufora
*   - Nieoczekiwane znaki początku/końca
************************************************************************/

void processReceivedChar(uint8_t received_char) {
    if (received_char == FRAME_START) {
    	resetFrameState();
    	in_frame = true;
    } else if (received_char == FRAME_END && escape_detected == false) {
        if (in_frame) {
            if (decodeFrame(bx, &frame, bx_index)) {
            	stopAnimation();
            	sendStatus(ERR_GOOD);
                handleCommand(&frame);
            } else {
            	sendStatus(ERR_FAIL);
            }
            resetFrameState();
        }
    } else if (in_frame) {
        if (escape_detected) {
            if (received_char == FRAME_START_STUFF) {
                bx[bx_index++] = FRAME_START;
            } else if (received_char == ESCAPE_CHAR_STUFF) {
                bx[bx_index++] = ESCAPE_CHAR;
            } else if (received_char == FRAME_END_STUFF) {
                bx[bx_index++] = FRAME_END;
            } else {
            	sendStatus(ERR_FAIL);
                resetFrameState();
            }
            escape_detected = false;
        } else if (received_char == ESCAPE_CHAR) {
            escape_detected = true;
        } else {
        	bx[bx_index++] = received_char;
        }
    } else if (bx_index >= MAX_FRAME_WITHOUT_STUFFING) {
    	resetFrameState();
    }
}




/************************************************************************
* Funkcja: handleCommand()
* Cel: Rozpoznaje i wykonuje komendy z ramki
*
* Parametry:
*   - frame: Wskaźnik na strukturę z ramką
*
* Wspierane komendy:
*   - ONK: Rysowanie koła
*   - ONP: Rysowanie prostokąta
*   - ONT: Rysowanie trójkąta
*   - ONN: Wyświetlanie tekstu
*   - OFF: Wyłączenie/czyszczenie
*
* Działanie:
*   1. Sprawdza komendę w tablicy commandTable
*   2. Dla komendy OFF:
*      - Specjalna obsługa bez współrzędnych
*   3. Dla pozostałych:
*      - Parsuje współrzędne
*      - Sprawdza zakres
*      - Wykonuje odpowiednią funkcję
*
* Używa:
*   - strncmp: Porównanie komend
*   - parseCoordinates: Parsowanie współrzędnych
*   - isWithinBounds: Sprawdzenie zakresu
*   - lcdClear/lcdCopy: Operacje na wyświetlaczu
*
* Błędy:
*   - Nieznana komenda
*   - Nieprawidłowe współrzędne
*   - Przekroczenie obszaru wyświetlacza
************************************************************************/
void handleCommand(Frame *frame) {
    if (frame == NULL) {
        return;
    }
    CommandEntry commandTable[COMMAND_COUNT] = {
        {"ONK", executeONK},
        {"ONP", executeONP},
        {"ONT", executeONT},
        {"ONN", executeONN},
        {"OFF", executeOFF}
    };
    HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_SET);
    for (int i = 0; i < COMMAND_COUNT; i++) {
            if (safeCompare(frame->command, commandTable[i].command, COMMAND_LENGTH)) {
                if (safeCompare(commandTable[i].command, "OFF", COMMAND_LENGTH)) {
                    lcdClear();
                    commandTable[i].function(frame);
                    lcdCopy();
                    clearFrame(frame);
                    return;
                }

                int x, y;
                if (parseCoordinates(frame->data, &x, &y)) {
                    if (isWithinBounds(x, y)) {
                        lcdClear();
                        commandTable[i].function(frame);
                        lcdCopy();
                        clearFrame(frame);
                        return;
                    } else {
                    sendStatus(ERR_DISPLAY_AREA);
                    return;
                }
            } else {
            	lcdClear();
            	lcdCopy();
            	sendStatus(ERR_NOT_RECOGNIZED);
            	return;
            }
        }
    }
}



void updateScrollingText(void) {
    if (!text.isScrolling || text.scrollSpeed == 0) {
        return;
    }

    uint32_t currentTime = HAL_GetTick();
    if ((currentTime - text.lastUpdate) >= (256 - text.scrollSpeed)) {
        text.lastUpdate = currentTime;

        uint8_t charWidth;
        uint8_t charHeight;
        const uint8_t* font;
        switch(text.fontSize) {
            case 1: charWidth = 5; charHeight = 7; font=font5x7; break;
            case 2: charWidth = 5; charHeight = 8; font=font5x8; break;
            case 3: charWidth = 6; charHeight = 9; font=font6x9; break;
            default: charWidth = 5; charHeight = 7; font=font5x7; break;
        }

        // Oblicz całkowitą szerokość tekstu
        int16_t textWidth = text.textLength * charWidth;

        // Przewijanie od lewej do prawej
        if (!text.firstIteration) {
            text.x+=text.textLength;  // Przesuwamy w prawo

            // Jeśli tekst całkowicie wyszedł z prawej strony
            if (text.x > LCD_WIDTH) {
                text.x = -textWidth; // Wróć na lewą stronę
                text.y += charHeight; // Przejdź do następnej linii

                // Jeśli doszliśmy do dołu ekranu
                if (text.y >= LCD_HEIGHT - charHeight) {
                    text.y = text.startY; // Wróć na początkową wysokość
                }
            }
        } else {
            // Pierwsza iteracja - start z lewej strony
            text.x = -textWidth;
            text.y = text.startY;
            text.firstIteration = false;
        }
            lcdClear();
            hagl_put_text(text.displayText, text.x, text.y, text.color, font);
            lcdCopy();
    }
}
