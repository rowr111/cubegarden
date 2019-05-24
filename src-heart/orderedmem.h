inline void writel(volatile uint32_t *addr, uint32_t data) {
  __asm__ __volatile__("":::"memory");
  *addr = data;
}

inline uint32_t readl(volatile uint32_t *addr) {
  __asm__ __volatile__("":::"memory");
  return *addr;
}


inline void writeb(volatile uint8_t *addr, uint8_t data) {
  __asm__ __volatile__("":::"memory");
  *addr = data;
}

inline uint8_t readb(volatile uint8_t *addr) {
  __asm__ __volatile__("":::"memory");
  return *addr;
}


inline void writew(volatile uint16_t *addr, uint16_t data) {
  __asm__ __volatile__("":::"memory");
  *addr = data;
}

inline uint16_t readw(volatile uint16_t *addr) {
  __asm__ __volatile__("":::"memory");
  return *addr;
}
