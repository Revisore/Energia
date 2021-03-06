#ifndef usci_isr_handler_h
#define usci_isr_handler_h

#ifdef __MSP430_HAS_USCI__

#ifdef __cplusplus
extern "C" {
#endif
void uart_tx_isr(void);
void uart_rx_isr(void);
void i2c_txrx_isr(void);
void i2c_state_isr(void);
void usci_isr_install(void);
#ifdef __cplusplus
}
#endif
#endif /* __MSP430_HAS_USCI__ */
#endif /* usci_isr_handler_h */
