/*

Data Collection Format

Record:
 OCTETS dcf_magic[6]
 OCTETS collectiontype[4]; Subtype that uses DCF as a container 
 VARINT dcfversion
 VARINT collectionid This way we can multiplex records from several collections with the same collectiontype
 CRC16 headcrc
 meta: (VARINT 0|VARINT identsize, OCTETS identifier, VARINT contentsize, OCTETS content, CRC16 metaitemcrc)  Repeated until identsize is 0 which marks the end of metadata.
 UINT16 padsize
 OCTETS padding[<padsize>]
 OCTETS data_magic[4]
 VARINT data_section_pos. Position in octets of start of data section relative start of record.
 data: (VARINT 0|datasize, VARINT (segmentid|0), VARINT padsize, OCTETS datasegment, CRC32 datasegmentcrc). Repeated until datasize is 0 which marks the end of data. padsize is the number octets at the end of data used for padding (not actual content).
 CRC32 datacrc(data section) -- CRC32 of 'data' section
 signature: (VARINT 0|VARINT signaturetypesize, OCTETS signaturetype, VARINT signaturesize, OCTETS signature, CRC16 sigcrc). Repeated until signaturetypesize is 0.
 UINT16 padsize
 OCTETS padding[<padsize>]
 VARINT recordsize. Size of complete record from dcf_magic upto and including recordsize.
 
Definitions:
 VARINT = NUMINTNIBBLES INTNIBBLE-LOW .. INTNIBBLE-HIGH .. NUMINTNIBBLES(repeated), CRC16.
          Integer zero (0) must be specified with NUMINTNIBBLES zero (0)
 NUMINTNIBBLES = 1 octet.
 INTNIBBLE = 1 nibble (4 bits).
 CRC32 = OCTETS CRC32[4] network byte order
 CRC16 = OCTETS CRC16[2] network byte order
 UINT16 = Unsigned 16bit integer. 2 octets. network byte order
 */
#include "bigint.h"
#include "crc.h"

struct dcf {
  int fd;
  char collectiontype[4];
  unsigned int segmentid;
  struct bigint recordsize;
  struct bigint temp, temp2;
};

#define DCF_MAGIC "%DCF_%"
#define DCF_VERSION 1
#define DCF_DATA_MAGIC "%12:"

int dcf_init(struct dcf *dcf, int fd, char *workbuf, int workbufsize); /* workbuf must be big enough to handle maximum recordsize of collection times three (64 bytes should be enough) */
int dcf_collectiontype_set(struct dcf *dcf, const char *collectiontype);

int dcf_magic_read(struct dcf *dcf, struct crc *crc);
int dcf_collectiontype_read(struct dcf *dcf, struct crc *crc);
int dcf_varint_size_read(struct dcf *dcf, struct crc *crc, int *size);
int dcf_varint_value_read(struct dcf *dcf, struct crc *crc, struct bigint *i, int size);
int dcf_data_read(struct dcf *dcf, struct crc *crc, int datasize, unsigned char *buf);
int dcf_signature_read(struct dcf *dcf, int * typesize, int *sigsize, int typebufsize, char *typebuf, int sigbufsize, unsigned char *sigbuf); /* 0 ok. 1 = no-signature. < 0 on error */
int dcf_crc16_read(struct dcf *dcf, struct crc *crc); /* reads and checks crc16 */
int dcf_crc32_read(struct dcf *dcf, struct crc *crc); /* reads and checks crc32 */
int dcf_recordsize_read(struct dcf *dcf); /* reads and checks recordsize */
int dcf_uint16_read(struct dcf *dcf, struct crc *crc, unsigned int *value);

int dcf_magic_write(struct dcf *dcf, struct crc *crc);
int dcf_collectiontype_write(struct dcf *dcf, struct crc *crc);
int dcf_varint_write(struct dcf *dcf, struct crc *crc, struct bigint *i);
int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content);
int dcf_meta_write_final(struct dcf *dcf);
int dcf_data_write_magic(struct dcf *dcf);
int dcf_data_write(struct dcf *dcf, struct crc *crc, const char *buf, int size, int padsize);
int dcf_data_write_final(struct dcf *dcf, struct crc *crc);
int dcf_signature_write(struct dcf *dcf, const char *sigtype, int sigtypesize, const char *sig, int sigsize);
int dcf_signature_write_final(struct dcf *dcf);
int dcf_crc16_write(struct dcf *dcf, struct crc *crc); /* write accumulated crc16 */
int dcf_crc32_write(struct dcf *dcf, struct crc *crc); /* write accumulated crc16 */
int dcf_pos_write(struct dcf *dcf, struct crc *crc);
int dcf_recordsize_write(struct dcf *dcf, struct crc *crc); /* writes recordsize */
int dcf_write_zeros(struct dcf *dcf, struct crc *crc, int n);
int dcf_uint16_write(struct dcf *dcf, struct crc *crc, unsigned int value);
