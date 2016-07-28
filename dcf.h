/*

Data Collection Format

Record:
 char magic[6]
 char collectiontype[4]; /* subtype. parent format that uses DCF */
 VARINT dcfversion
 VARINT collectionid (same format as for size) This way we can multiplex records from several collections
 meta: (0|<identsize><identifier><contentsize>><content>) /* repeated until identsize is 0 which is last metadata value */
 <datasize>
 char checksum[32] (meta) -- checksum from magic upto and including datasize
 [data]
 char checksum[32] (data) -- checksum of data

<size> = VARINT
VARINT = NUMSIZEBYTES SIZEBYTE1 .. SIZEBYTEN

 */

struct dcf {
  int fd;
  unsigned char sha256[32]; /* initialized by dcf_init and dcf_checksum_read|write */
  unsigned char sha256cumul[32]; /* initialized by dcf_init */
};

#define DCF_MAGIC "%DCF_%"
#define DCF_VERSION 1

int dcf_init(struct dcf *dcf, int fd, const char *collectiontype);

int dcf_magic_read(struct dcf *dcf);
int dcf_collectiontype_read(struct dcf *dcf);
int dcf_varint_size_read(struct dcf *dcf); /* returns number bytes in varint */
int dcf_varint_value_read(struct dcf *dcf, struct bigint *i);
int dcf_meta_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_checksum_read(struct dcf *dcf);

int dcf_magic_write(struct dcf *dcf);
int dcf_collectiontype_write(struct dcf *dcf, const char *collectiontype);
int dcf_varint_write(struct dcf *dcf, int nvalues, const int *values);
int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content);
int dcf_meta_write_final(struct dcf *dcf);
int dcf_data_write(struct dcf *dcf, const char *buf, int size);
int dcf_checksum_write(struct dcf *dcf, char *chksum);
