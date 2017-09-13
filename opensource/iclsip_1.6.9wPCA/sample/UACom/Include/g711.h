// G711Codec.h: interface for the G711Codec class.
//
//////////////////////////////////////////////////////////////////////

enum CodecTypes
{
	CODECTYPE_ULAW = 0,
	CODECTYPE_ALAW = 1
};

class G711Codec
{
public:
	G711Codec();
	~G711Codec();
	int Encode(char *input, int inputSizeBytes, char *output, int *outputSizeBytes);
	int Decode(void *input, int inputSizeBytes, void *output, int *outputSizeBytes);
	int SetULaw();
	int SetALaw();
	int GetCodecType() { if (codecType == 1) return CODECTYPE_ALAW; else return CODECTYPE_ULAW; }
private:
	int codecType;	// 0 = ULAW, 1 = ALAW, everything else = ULAW
	unsigned char *compTable;
	short *expTable;
};
