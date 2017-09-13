#ifndef	MD5_H
#define	MD5_H

#define	MD5_MAC_LEN	16

#define	u32	unsigned int
#define	u8	unsigned char

struct MD5Context {
	u32	buf[4];
	u32	bits[2];
	u8 in[64];
};

void MD5Init(struct	MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned	char *buf,
		   unsigned	len);
void MD5Final(unsigned char	digest[16],	struct MD5Context *context);
void MD5Transform(u32 buf[4], u32 in[16]);

typedef	struct MD5Context MD5_CTX;


void md5_mac(u8	*key, size_t key_len, u8 *data,	size_t data_len, u8	*mac);
void hmac_md5(u8 *key,	size_t key_len,	u8 *data, size_t data_len, u8 *mac);

#define SHA_DIGEST_LEN 20
#endif /* MD5_H	*/

#ifndef	_AES_H
#define	_AES_H

#ifndef	uint8
#define	uint8  unsigned	char
#endif

#ifndef	uint32
#define	uint32 unsigned int
#endif

typedef	struct
{
	uint32 erk[64];		/* encryption round	keys */
	uint32 drk[64];		/* decryption round	keys */
	int	nr;				/* number of rounds	*/
}
aes_context;

int	 aes_set_key( aes_context *ctx,	uint8 *key,	int	nbits );
void aes_encrypt( aes_context *ctx,	uint8 input[16], uint8 output[16] );
void aes_decrypt( aes_context *ctx,	uint8 input[16], uint8 output[16] );

int PasswordHash(char *password, unsigned char *ssid, int ssidlength, unsigned char *output);
#endif /* aes.h	*/

