/*******************************************************************************
*   BOLOS Enclave
*   (c) 2017 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "bolos.h"
#include "bolos_printf.h"
#include "platform_al.h"

static const char *const g_pcHex = "0123456789abcdef";
static const char *const g_pcHex_cap = "0123456789ABCDEF";

void screen_printc(char ch) {
    platform_printc(ch);
}

void screen_prints(const char *str, unsigned int charcount) {
    while (charcount--) {
        screen_printc(*str++);
    }
}

void screen_printf(const char *format, ...) {
    unsigned long ulIdx, ulValue, ulPos, ulCount, ulBase, ulNeg, ulStrlen,
        ulCap;
    char *pcStr, pcBuf[16], cFill;
    va_list vaArgP;
    char cStrlenSet;

    //
    // Check the arguments.
    //
    if (format == 0) {
        return;
    }

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, format);

    //
    // Loop while there are more characters in the string.
    //
    while (*format) {
        //
        // Find the first non-% character, or the end of the string.
        //
        for (ulIdx = 0; (format[ulIdx] != '%') && (format[ulIdx] != '\0');
             ulIdx++) {
        }

        //
        // Write this portion of the string.
        //
        screen_prints(format, ulIdx);

        //
        // Skip the portion of the string that was written.
        //
        format += ulIdx;

        //
        // See if the next character is a %.
        //
        if (*format == '%') {
            //
            // Skip the %.
            //
            format++;

            //
            // Set the digit count to zero, and the fill character to space
            // (i.e. to the defaults).
            //
            ulCount = 0;
            cFill = ' ';
            ulStrlen = 0;
            cStrlenSet = 0;
            ulCap = 0;
            ulBase = 10;

        //
        // It may be necessary to get back here to process more characters.
        // Goto's aren't pretty, but effective.  I feel extremely dirty for
        // using not one but two of the beasts.
        //
        again:

            //
            // Determine how to handle the next character.
            //
            switch (*format++) {
            //
            // Handle the digit characters.
            //
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                //
                // If this is a zero, and it is the first digit, then the
                // fill character is a zero instead of a space.
                //
                if ((format[-1] == '0') && (ulCount == 0)) {
                    cFill = '0';
                }

                //
                // Update the digit count.
                //
                ulCount *= 10;
                ulCount += format[-1] - '0';

                //
                // Get the next character.
                //
                goto again;
            }

            //
            // Handle the %c command.
            //
            case 'c': {
                //
                // Get the value from the varargs.
                //
                ulValue = va_arg(vaArgP, unsigned long);

                //
                // Print out the character.
                //
                screen_prints((char *)&ulValue, 1);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Handle the %d command.
            //
            case 'd': {
                //
                // Get the value from the varargs.
                //
                ulValue = va_arg(vaArgP, unsigned long);

                //
                // Reset the buffer position.
                //
                ulPos = 0;

                //
                // If the value is negative, make it positive and indicate
                // that a minus sign is needed.
                //
                if ((long)ulValue < 0) {
                    //
                    // Make the value positive.
                    //
                    ulValue = -(long)ulValue;

                    //
                    // Indicate that the value is negative.
                    //
                    ulNeg = 1;
                } else {
                    //
                    // Indicate that the value is positive so that a minus
                    // sign isn't inserted.
                    //
                    ulNeg = 0;
                }

                //
                // Set the base to 10.
                //
                ulBase = 10;

                //
                // Convert the value to ASCII.
                //
                goto convert;
            }

            //
            // Handle ths %.*s command
            // special %.*H or %.*h format to print a given length of hex digits
            // (case: H UPPER, h lower)
            //
            case '.': {
                // ensure next char is '*' and next one is 's'
                if (format[0] == '*' && (format[1] == 's' || format[1] == 'H' ||
                                         format[1] == 'h')) {
                    // skip '*' char
                    format++;

                    ulStrlen = va_arg(vaArgP, unsigned long);
                    cStrlenSet = 1;

                    // interpret next char (H/h/s)
                    goto again;
                }

                // does not support %.2x for example
                goto error;
            }

            case '*': {
                if (*format == 's') {
                    ulStrlen = va_arg(vaArgP, unsigned long);
                    cStrlenSet = 2;
                    goto again;
                }

                goto error;
            }

            case '-': // -XXs
            {
                cStrlenSet = 0;
                // read a number of space to post pad with ' ' the string to
                // display
                goto again;
            }

            //
            // Handle the %s command.
            // %H and %h also
            case 'H':
                ulCap = 1; // uppercase base 16
                ulBase = 16;
                goto case_s;
            case 'h':
                ulBase = 16; // lowercase base 16
                goto case_s;
            case 's':
            case_s : {
                //
                // Get the string pointer from the varargs.
                //
                pcStr = va_arg(vaArgP, char *);

                //
                // Determine the length of the string. (if not specified using
                // .*)
                //
                switch (cStrlenSet) {
                // compute length with strlen
                case 0:
                    for (ulIdx = 0; pcStr[ulIdx] != '\0'; ulIdx++) {
                    }
                    break;

                // use given length
                case 1:
                    ulIdx = ulStrlen;
                    break;

                // printout prepad
                case 2:
                    // if string is empty, then, ' ' padding
                    if (pcStr[0] == '\0') {
                        // padd ulStrlen white space
                        do {
                            screen_prints(" ", 1);
                        } while (ulStrlen-- > 0);

                        goto s_pad;
                    }
                    goto error; // unsupported if replicating the same string
                                // multiple times
                case 3:
                    // skip '-' still buggy ...
                    goto again;
                }

                //
                // Write the string.
                //
                switch (ulBase) {
                default:
                    screen_prints(pcStr, ulIdx);
                    break;
                case 16: {
                    unsigned char nibble1, nibble2;
                    for (ulCount = 0; ulCount < ulIdx; ulCount++) {
                        nibble1 = (pcStr[ulCount] >> 4) & 0xF;
                        nibble2 = pcStr[ulCount] & 0xF;
                        switch (ulCap) {
                        case 0:
                            screen_printc(g_pcHex[nibble1]);
                            screen_printc(g_pcHex[nibble2]);
                            break;
                        case 1:
                            screen_printc(g_pcHex_cap[nibble1]);
                            screen_printc(g_pcHex_cap[nibble2]);
                            break;
                        }
                    }
                    break;
                }
                }

            s_pad:
                //
                // Write any required padding spaces
                //
                if (ulCount > ulIdx) {
                    ulCount -= ulIdx;
                    while (ulCount--) {
                        screen_prints(" ", 1);
                    }
                }
                //
                // This command has been handled.
                //
                break;
            }

            //
            // Handle the %u command.
            //
            case 'u': {
                //
                // Get the value from the varargs.
                //
                ulValue = va_arg(vaArgP, unsigned long);

                //
                // Reset the buffer position.
                //
                ulPos = 0;

                //
                // Set the base to 10.
                //
                ulBase = 10;

                //
                // Indicate that the value is positive so that a minus sign
                // isn't inserted.
                //
                ulNeg = 0;

                //
                // Convert the value to ASCII.
                //
                goto convert;
            }

            //
            // Handle the %x and %X commands.  Note that they are treated
            // identically; i.e. %X will use lower case letters for a-f
            // instead of the upper case letters is should use.  We also
            // alias %p to %x.
            //
            case 'X':
                ulCap = 1;
            case 'x':
            case 'p': {
                //
                // Get the value from the varargs.
                //
                ulValue = va_arg(vaArgP, unsigned long);

                //
                // Reset the buffer position.
                //
                ulPos = 0;

                //
                // Set the base to 16.
                //
                ulBase = 16;

                //
                // Indicate that the value is positive so that a minus sign
                // isn't inserted.
                //
                ulNeg = 0;

            //
            // Determine the number of digits in the string version of
            // the value.
            //
            convert:
                for (ulIdx = 1; (((ulIdx * ulBase) <= ulValue) &&
                                 (((ulIdx * ulBase) / ulBase) == ulIdx));
                     ulIdx *= ulBase, ulCount--) {
                }

                //
                // If the value is negative, reduce the count of padding
                // characters needed.
                //
                if (ulNeg) {
                    ulCount--;
                }

                //
                // If the value is negative and the value is padded with
                // zeros, then place the minus sign before the padding.
                //
                if (ulNeg && (cFill == '0')) {
                    //
                    // Place the minus sign in the output buffer.
                    //
                    pcBuf[ulPos++] = '-';

                    //
                    // The minus sign has been placed, so turn off the
                    // negative flag.
                    //
                    ulNeg = 0;
                }

                //
                // Provide additional padding at the beginning of the
                // string conversion if needed.
                //
                if ((ulCount > 1) && (ulCount < 16)) {
                    for (ulCount--; ulCount; ulCount--) {
                        pcBuf[ulPos++] = cFill;
                    }
                }

                //
                // If the value is negative, then place the minus sign
                // before the number.
                //
                if (ulNeg) {
                    //
                    // Place the minus sign in the output buffer.
                    //
                    pcBuf[ulPos++] = '-';
                }

                //
                // Convert the value into a string.
                //
                for (; ulIdx; ulIdx /= ulBase) {
                    if (!ulCap) {
                        pcBuf[ulPos++] = g_pcHex[(ulValue / ulIdx) % ulBase];
                    } else {
                        pcBuf[ulPos++] =
                            g_pcHex_cap[(ulValue / ulIdx) % ulBase];
                    }
                }

                //
                // Write the string.
                //
                screen_prints(pcBuf, ulPos);

                //
                // This command has been handled.
                //
                break;
            }

            //
            // Handle the %% command.
            //
            case '%': {
                //
                // Simply write a single %.
                //
                screen_prints(format - 1, 1);

                //
                // This command has been handled.
                //
                break;
            }

            error:
            //
            // Handle all other commands.
            //
            default: {
                //
                // Indicate an error.
                //
                screen_prints("ERROR", 5);

                //
                // This command has been handled.
                //
                break;
            }
            }
        }
    }

    //
    // End the varargs processing.
    //
    va_end(vaArgP);
}
