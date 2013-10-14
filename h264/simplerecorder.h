struct profileid_sps_pps{
	char base64profileid[10];
	char base64sps[30];
	char base64pps[30];
};

int camera_init(struct picture_t *out_info);
int camera_on();
int camera_get_frame(struct picture_t *pic);
int camera_off();
void camera_close();

void output_print(const char *str);
int output_init(struct picture_t *info, const char *str);
int output_write_headers(struct encoded_pic_t *headers,struct profileid_sps_pps *psp);
int output_write_frame(struct encoded_pic_t *encoded);
void output_close();

void osd_print(struct picture_t *pic, const char *str);

int preview_init(struct picture_t *vd_info);
int preview_display(struct picture_t *pic);
void preview_close();

int encoder_init(struct picture_t *info);
int encoder_encode_headers(struct encoded_pic_t *headers_out);
int encoder_encode_frame(struct picture_t *raw_pic, struct encoded_pic_t *output);
void encoder_release(struct encoded_pic_t *output);
void encoder_close();
void ResetTime(struct picture_t *raw_pic,struct encoded_pic_t *output);
