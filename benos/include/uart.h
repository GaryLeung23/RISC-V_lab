#ifndef	_MINI_UART_H
#define	_MINI_UART_H

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);
void putchar(char c);
char uart_get(void);

void enable_uart_plic();

#endif  /*_MINI_UART_H */
