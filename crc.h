#ifndef CRC_H
#define CRC_H

#define CRC16 1
#define CRC32 2

struct crc {
	int n;
	unsigned char crctype[8];
	unsigned int crc32[8]; /* initialized to 0x0 */
	unsigned short crc16[8]; /* initialized to 0xFFFF */
};

void crc_init(struct crc *crc);
int crc_push(struct crc *crc, int crctype);
int crc_pop(struct crc *crc, int crctype);
int crc_val(struct crc *crc, unsigned int *val, int crctype);
void crc_update(struct crc *crc, const void *data, size_t size);
ssize_t crc_write(int fd, struct crc *crc, const void *buf, size_t count);
ssize_t crc_read(int fd, struct crc *crc, void *buf, size_t count);

#endif
