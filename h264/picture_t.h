struct picture_t {
	unsigned char *buffer;
	struct timeval timestamp;
	int width, height;
};

enum H264_FRAME_TYPE {FRAME_TYPE_I, FRAME_TYPE_P, FRAME_TYPE_B};

struct encoded_pic_t{
	unsigned char *buffer;
	int length;
	unsigned long long timepoint; //(us)
	enum H264_FRAME_TYPE frame_type;
	void * usr_def;
};