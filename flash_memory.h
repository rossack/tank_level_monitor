
#ifndef H_FLASH_MEMORY
#define H_FLAHSH_MEMORY


#ifdef __cplusplus
extern "C" {
#endif

void flash_erase();
bool flash_write(char * buf, uint len);
char * flash_read();

#ifdef __cplusplus
}
#endif

#endif 